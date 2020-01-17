#include "mu2e-artdaq/ArtModules/detail/CurrentFragment.hh"
#include "canvas/Utilities/Exception.h"
#include "dtcInterfaceLib/DTC_Packets.h"

namespace mu2e {
namespace detail {

CurrentFragment::CurrentFragment(artdaq::Fragment f)
	: fragment_{std::make_unique<artdaq::Fragment>(std::move(f))}, reader_{std::make_unique<mu2eFragment>(*fragment_)}, current_{reinterpret_cast<const uint8_t*>(reader_->dataBegin())}, block_count_(0) {}

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
	while (data < end) {
		// Construct DTC_DataHeaderPacket to determine byte count of
		// current data block.
		try{
		DTCLib::DTC_DataPacket const dataPacket{data};
		DTCLib::DTC_DataHeaderPacket const headerPacket{dataPacket};
		auto const byteCount = headerPacket.GetByteCount();

		// Use byte count to calculate how many words the current data
		// block should occupy in the new fragment.
		auto const wordCount = byteCount / sizeof(artdaq::RawDataType);
		auto const packetSize = (byteCount % 8 == 0) ? wordCount : wordCount + 1;

		if (headerPacket.GetSubsystem() == subsystem) {
			result->push_back(*artdaq::Fragment::dataFrag(headerPacket.GetTimestamp().GetTimestamp(true),
														  headerPacket.GetEVBMode(),  // Returns evbMode (see mu2e-docdb 4914)
														  reinterpret_cast<artdaq::RawDataType const*>(data), packetSize,
														  fragment_->timestamp())
								   .get());
		}
		data += byteCount;
		} catch(...) { TLOG(TLVL_TRACE) << "Error reading Fragments from block";break; }
	}
	if(data <= end) { return result; }
TLOG(TLVL_ERROR) << "ma::CurrentFragment::extractFragmentsFromBlock: The data pointer has shot past the 'end' pointer. data=" << std::hex << (void*)data << ", end=" << std::hex << (void*)end;
					    throw art::Exception{art::errors::DataCorruption, "CurrentFragment::extractFragmentsFromBlock"}
							 << "The data pointer has shot past the 'end' pointer.";
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
	while (data < end) {
		try {
		// Construct DTC_DataHeaderPacket to determine byte count of
		// current data block.
		DTCLib::DTC_DataPacket const dataPacket{data};
		DTCLib::DTC_DataHeaderPacket const headerPacket{dataPacket};
		auto const byteCount = headerPacket.GetByteCount();

		if (headerPacket.GetSubsystem() == subsystem) {
			result++;
		}
		data += byteCount;
		}
		catch(...) { TLOG(TLVL_TRACE) << "Error reading Fragments from Block"; break;}
	}

	if(data <= end) { return result; }
TLOG(TLVL_ERROR) << "ma::CurrentFragment::getFragmentCount: The data pointer has shot past the 'end' pointer. data=" << std::hex << (void*)data << ", end=" << std::hex << (void*)end;
					    throw art::Exception{art::errors::DataCorruption, "CurrentFragment::getFragmentCount"}
							 << "The data pointer has shot past the 'end' pointer.";
}

  uint64_t CurrentFragment::getCurrentTimestamp()
  {
    uint64_t result = 0;
    auto data = reinterpret_cast<char const*>(current_);
      // Construct DTC_DataHeaderPacket to determine byte count of
      // current data block.
      DTCLib::DTC_DataPacket const dataPacket{data};
      DTCLib::DTC_DataHeaderPacket const headerPacket{dataPacket};

      result = headerPacket.GetTimestamp().GetTimestamp(true);

    // Catch roll-over in Detector Emulator mode
    if(result < (fragment_->sequenceID() -1) * DATA_BLOCKS_PER_MU2E_FRAGMENT && fragment_->sequenceID() < 0xFFFFFFFFFFFF) {
      result += (fragment_->sequenceID() -1) * DATA_BLOCKS_PER_MU2E_FRAGMENT;
    }

    return result;
  }

}  // namespace detail
}  // namespace mu2e
