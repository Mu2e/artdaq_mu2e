#include "canvas/Utilities/Exception.h"
#include "dtcInterfaceLib/DTC_Packets.h"
#include "mu2e-artdaq/ArtModules/detail/CurrentFragment.hh"

namespace mu2e {
  namespace detail {

    CurrentFragment::CurrentFragment(artdaq::Fragment const& f) :
      fragment_{std::make_unique<artdaq::Fragment>(f)},
      reader_{std::make_unique<mu2eFragment>(*fragment_)},
      current_{reinterpret_cast<const uint8_t*>(reader_->dataBegin())},
      block_count_(0)
    {}

    std::unique_ptr<artdaq::Fragments>
    CurrentFragment::extractFragmentsFromBlock(DTCLib::Subsystem const subsystem)
    {
      auto result = std::make_unique<artdaq::Fragments>();

      // Each fragment has N super blocks--these super blocks are what
      // will be broken up into art::Events.  For a given super block,
      // there are M data blocks.

      // Increment through the data blocks of the current super block.
      auto const begin = current_;
      auto const end = current_ + reader_->blockSize(processedSuperBlocks());
      auto data = begin;
      while (data < end) {
        // Construct DTC_DataHeaderPacket to determine byte count of
        // current data block.
        DTCLib::DTC_DataPacket const dataPacket {data};
        DTCLib::DTC_DataHeaderPacket const headerPacket {dataPacket};
        auto const byteCount = headerPacket.GetByteCount();

        // Use byte count to calculate how many words the current data
        // block should occupy in the new fragment.
        auto const wordCount = byteCount/sizeof(artdaq::RawDataType);
        auto const packetSize = (byteCount%8 == 0) ? wordCount : wordCount+1;

        if (headerPacket.GetSubsystem() == subsystem) {
          result->emplace_back(artdaq::Fragment::dataFrag(headerPacket.GetTimestamp().GetTimestamp(true),
                                                          headerPacket.GetData(), // Returns evbMode (see mu2e-docdb 4914)
                                                          reinterpret_cast<artdaq::RawDataType const*>(data),
                                                          packetSize,
                                                          fragment_->timestamp()));
        }
        data += byteCount;
      }
      return data == end ? std::move(result) :
        throw art::Exception{art::errors::DataCorruption, "CurrentFragment::extractFragmentsFromBlock"}
          << "The data pointer has shot past the 'end' pointer.";
    }

  } // detail
} // mu2e
