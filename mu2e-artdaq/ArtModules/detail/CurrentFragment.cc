
#include "TRACE/tracemf.h"
#define TRACE_NAME "CurrentFragment"

#include "mu2e-artdaq/ArtModules/detail/CurrentFragment.hh"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "canvas/Utilities/Exception.h"
#include "dtcInterfaceLib/DTC_Packets.h"

namespace mu2e {
namespace detail {

static size_t event_num = 0;

CurrentFragment::CurrentFragment(artdaq::Fragment f, bool debug_event_number_mode)
	: fragment_{std::make_unique<artdaq::Fragment>(std::move(f))}
	, reader_{std::make_unique<mu2eFragment>(*fragment_)}
	, block_count_(0)
	, debug_event_number_mode_(debug_event_number_mode)
{
	extractDTCEvents();
}

std::unique_ptr<artdaq::Fragments> CurrentFragment::extractFragmentsFromBlock(DTCLib::DTC_Subsystem const subsystem)
{
	auto result = std::make_unique<artdaq::Fragments>();

	for (auto& evt : current_events_)
	{
		for (auto& subevt : evt.GetSubEvents())
		{
			if (subevt.GetSubsystem() == subsystem)
			{
				for (size_t ii = 0; ii < subevt.GetDataBlockCount(); ++ii)
				{
					TLOG(TLVL_TRACE + 4) << "Getting first block header";
					auto blockStart = subevt.GetDataBlocks()[ii].blockPointer;
					DTCLib::DTC_DataPacket const dataPacket{blockStart};
					DTCLib::DTC_DataHeaderPacket const headerPacket{dataPacket};
					auto byteCount = headerPacket.GetByteCount();

					TLOG(TLVL_TRACE + 4) << "Checking subsequent blocks to see if they are from the same ROC";
					while (ii < subevt.GetDataBlockCount() - 1)
					{
						try
						{
							DTCLib::DTC_DataPacket const newDataPacket{subevt.GetDataBlocks()[++ii].blockPointer};
							DTCLib::DTC_DataHeaderPacket const newHeaderPacket{newDataPacket};

							// Collapse multiple blocks from the same DTC/ROC into one Fragment
							if (newHeaderPacket.GetSubsystem() == subsystem && newHeaderPacket.GetID() == headerPacket.GetID() && newHeaderPacket.GetLinkID() == headerPacket.GetLinkID() && newHeaderPacket.GetHopCount() == headerPacket.GetHopCount())
							{
								TLOG(TLVL_TRACE + 4) << "Adding " << newHeaderPacket.GetByteCount() << " bytes to current block size (" << byteCount << "), as this block is from the same ROC as previous";
								byteCount += newHeaderPacket.GetByteCount();
							}
							else
							{
								--ii;
								break;
							}
						}
						catch (...)
						{
							TLOG(TLVL_ERROR) << "There may be data corruption in the Fragment. Aborting search for same-ROC blocks";
							break;
						}
					}

					// Use byte count to calculate how many words the current data
					// block should occupy in the new fragment.
					auto const wordCount = byteCount / sizeof(artdaq::RawDataType);
					auto const fragmentSize = (byteCount % sizeof(artdaq::RawDataType) == 0) ? wordCount : wordCount + 1;

					auto fragPtr = artdaq::Fragment::dataFrag(getBlockTimestamp_(headerPacket),
																  headerPacket.GetEVBMode(),  // Returns evbMode (see mu2e-docdb 4914)
																  reinterpret_cast<artdaq::RawDataType const*>(blockStart), fragmentSize,
										  fragment_->timestamp());
					artdaq::Fragment::type_t fragment_type = mu2e::detail::FragmentType::INVALID;
					switch(subsystem) {
					case DTCLib::DTC_Subsystem_Tracker: fragment_type = mu2e::detail::FragmentType::TRK; break;
					case DTCLib::DTC_Subsystem_Calorimeter: fragment_type = mu2e::detail::FragmentType::CAL; break;
					case DTCLib::DTC_Subsystem_CRV: fragment_type = mu2e::detail::FragmentType::CRV; break;
					default: break; // Use INVALID from above
					}
					fragPtr->setUserType(fragment_type);
					result->push_back(*fragPtr.get());
				}
			}
		}
	}

	return result;
}

std::unique_ptr<Mu2eEventHeader> CurrentFragment::makeMu2eEventHeader()
{
	std::unique_ptr<Mu2eEventHeader> output = nullptr;

	uint64_t timestamp = (static_cast<uint64_t>(getpid()) << 16) + (++event_num);
	uint8_t evbmode = 0;

	if (!debug_event_number_mode_)
	{
		timestamp = current_events_.front().GetEventWindowTag().GetEventWindowTag(true);
		evbmode = current_events_.front().GetHeader()->evb_mode;
	}

	output.reset(new Mu2eEventHeader(timestamp, evbmode));

	return output;
}

size_t CurrentFragment::getFragmentCount(DTCLib::DTC_Subsystem const subsystem)
{
	auto result = 0;

	for (auto& evt : current_events_)
	{
		for (auto& subevt : evt.GetSubEvents())
		{
			if (subevt.GetSubsystem() == subsystem)
			{
				result += subevt.GetDataBlockCount();
			}
		}
	}

	return result;
}

uint64_t CurrentFragment::getBlockTimestamp_(DTCLib::DTC_DataHeaderPacket const& pkt)
{
	uint64_t result = pkt.GetEventWindowTag().GetEventWindowTag(true);
	if (first_block_timestamp_ == -1)
	{
		first_block_timestamp_ = result;
	}
	result = fragment_->timestamp() + (result - static_cast<uint64_t>(first_block_timestamp_));
	return result;
}

}  // namespace detail
}  // namespace mu2e
