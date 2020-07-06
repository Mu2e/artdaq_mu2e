
#include "TRACE/tracemf.h"
#define TRACE_NAME "CurrentFragment"

#include "mu2e-artdaq/ArtModules/detail/CurrentFragment.hh"
#include "canvas/Utilities/Exception.h"
#include "dtcInterfaceLib/DTC_Packets.h"

namespace mu2e {
namespace detail {

static size_t event_num = 0;

CurrentFragment::CurrentFragment(artdaq::Fragment f, bool debug_event_number_mode)
	: fragment_{std::make_unique<artdaq::Fragment>(std::move(f))}
	, reader_{std::make_unique<mu2eFragment>(*fragment_)}
	, current_{reinterpret_cast<const uint8_t*>(reader_->dataBegin())}
	, block_count_(0)
	, debug_event_number_mode_(debug_event_number_mode)
{}

std::unique_ptr<artdaq::Fragments> CurrentFragment::extractFragmentsFromBlock(DTCLib::DTC_Subsystem const subsystem)
{
	auto result = std::make_unique<artdaq::Fragments>();

	// Each fragment has N super blocks--these super blocks are what
	// will be broken up into art::Events.  For a given super block,
	// there are M data blocks.

	// Increment through the data blocks of the current super block.
	auto const begin = current_;
	auto const end = reinterpret_cast<char const*>(current_ + reader_->blockSize(processedSuperBlocks()));
	auto data = reinterpret_cast<char const*>(begin);
	while (data < end)
	{
		// Construct DTC_DataHeaderPacket to determine byte count of
		// current data block.
		try
		{
			TLOG(TLVL_TRACE) << "Getting first block header";
			DTCLib::DTC_DataPacket const dataPacket{data};
			DTCLib::DTC_DataHeaderPacket const headerPacket{dataPacket};
			auto byteCount = headerPacket.GetByteCount();

			if (headerPacket.GetSubsystem() == subsystem)
			{
				TLOG(TLVL_TRACE) << "Checking subsequent blocks to see if they are from the same ROC";
				while (data + byteCount < end)
				{
					try
					{
						DTCLib::DTC_DataPacket const newDataPacket{data + byteCount};
						DTCLib::DTC_DataHeaderPacket const newHeaderPacket{newDataPacket};

						// Collapse multiple blocks from the same DTC/ROC into one Fragment
						if (newHeaderPacket.GetSubsystem() == subsystem && newHeaderPacket.GetID() == headerPacket.GetID() && newHeaderPacket.GetRingID() == headerPacket.GetRingID() && newHeaderPacket.GetHopCount() == headerPacket.GetHopCount())
						{
							TLOG(TLVL_TRACE) << "Adding " << newHeaderPacket.GetByteCount() << " bytes to current block size (" << byteCount << "), as this block is from the same ROC as previous";
							byteCount += newHeaderPacket.GetByteCount();
						}
						else
						{
							break;
						}
					}
					catch (...)
					{
						TLOG_ERROR("CurrentFragment") << "There may be data corruption in the Fragment. Aborting search for same-ROC blocks";
						break;
					}
				}

				// Use byte count to calculate how many words the current data
				// block should occupy in the new fragment.
				auto const wordCount = byteCount / sizeof(artdaq::RawDataType);
				auto const fragmentSize = (byteCount % sizeof(artdaq::RawDataType) == 0) ? wordCount : wordCount + 1;

				result->push_back(*artdaq::Fragment::dataFrag(headerPacket.GetTimestamp().GetTimestamp(true),
															  headerPacket.GetEVBMode(),  // Returns evbMode (see mu2e-docdb 4914)
															  reinterpret_cast<artdaq::RawDataType const*>(data), fragmentSize,
															  fragment_->timestamp())
									   .get());
			}
			data += byteCount;
		}
		catch (...)
		{
			TLOG(TLVL_TRACE) << "Error reading Fragments from block";
			break;
		}
	}
	if (data <= end) { return result; }
	TLOG_ERROR("CurrentFragment") << "ma::CurrentFragment::extractFragmentsFromBlock: The data pointer has shot past the 'end' pointer. data=" << std::hex << (void*)data << ", end=" << std::hex << (void*)end;
	throw art::Exception{art::errors::DataCorruption, "CurrentFragment::extractFragmentsFromBlock"}
		<< "The data pointer has shot past the 'end' pointer.";
}

std::unique_ptr<Mu2eEventHeader> CurrentFragment::makeMu2eEventHeader()
{
	std::unique_ptr<Mu2eEventHeader> output = nullptr;

	uint64_t timestamp = getCurrentTimestamp();
	uint8_t evbmode = 0;

	if (!debug_event_number_mode_)
	{
		// Increment through the data blocks of the current super block.
		auto const begin = current_;
		auto data = reinterpret_cast<char const*>(begin);

		// Construct DTC_DataHeaderPacket to determine byte count of
		// current data block.
		try
		{
			DTCLib::DTC_DataPacket const dataPacket{data};
			DTCLib::DTC_DataHeaderPacket const headerPacket{dataPacket};
			timestamp = headerPacket.GetTimestamp().GetTimestamp(true);
			evbmode = headerPacket.GetEVBMode();
		}
		catch (...)
		{
			TLOG(TLVL_TRACE) << "Error reading Fragments from block when trying to create Mu2eEventHeader product!";
		}
	}

	output.reset(new Mu2eEventHeader(timestamp, evbmode));

	return output;
}

size_t CurrentFragment::getFragmentCount(DTCLib::DTC_Subsystem const subsystem)
{
	auto result = 0;

	// Each fragment has N super blocks--these super blocks are what
	// will be broken up into art::Events.  For a given super block,
	// there are M data blocks.

	// Increment through the data blocks of the current super block.
	auto const begin = current_;
	auto const end = reinterpret_cast<char const*>(current_ + reader_->blockSize(processedSuperBlocks()));
	auto data = reinterpret_cast<char const*>(begin);
	TLOG(TLVL_TRACE) << "data is " << (void*)data << ", end is " << (void*)end;

	while (data < end)
	{
		try
		{
			//TLOG(TLVL_TRACE) << "Constructing DTC_DataHeaderPacket with data from " << (void*)data << ", (end=" << (void*)end <<")";
			// Construct DTC_DataHeaderPacket to determine byte count of
			// current data block.
			DTCLib::DTC_DataPacket const dataPacket{data};
			DTCLib::DTC_DataHeaderPacket const headerPacket{dataPacket};
			auto const byteCount = headerPacket.GetByteCount();

			if (headerPacket.GetSubsystem() == subsystem)
			{
				result++;
			}
			data += byteCount;
		}
		catch (...)
		{
			TLOG(TLVL_TRACE) << "Error reading Fragments from Block";
			break;
		}
	}

	if (data <= end) { return result; }
	TLOG_ERROR("CurrentFragment") << "ma::CurrentFragment::getFragmentCount: The data pointer has shot past the 'end' pointer. data=" << std::hex << (void*)data << ", end=" << std::hex << (void*)end;
	throw art::Exception{art::errors::DataCorruption, "CurrentFragment::getFragmentCount"}
		<< "The data pointer has shot past the 'end' pointer.";
}

uint64_t CurrentFragment::getCurrentTimestamp()
{
	uint64_t result = 0;
	if (!debug_event_number_mode_)
	{
		auto data = reinterpret_cast<char const*>(current_);
		// Construct DTC_DataHeaderPacket to determine byte count of
		// current data block.
		DTCLib::DTC_DataPacket const dataPacket{data};
		DTCLib::DTC_DataHeaderPacket const headerPacket{dataPacket};

		result = headerPacket.GetTimestamp().GetTimestamp(true);
	}
	else
	{
		return (static_cast<uint64_t>(getpid()) << 16) + (++event_num);
	}

	return result;
}

}  // namespace detail
}  // namespace mu2e
