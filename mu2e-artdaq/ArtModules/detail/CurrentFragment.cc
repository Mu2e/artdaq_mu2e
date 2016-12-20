#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/Fragments.hh"
#include "canvas/Utilities/Exception.h"
#include "dtcInterfaceLib/DTC_Packets.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "mu2e-artdaq/ArtModules/detail/CurrentFragment.hh"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"

namespace mu2e {
  namespace detail {

    template <DTCLib::Subsystem S>
    std::unique_ptr<artdaq::Fragments>
    CurrentFragment::extractFragmentsFromBlock()
    {
      mu2eFragment const reader {*fragment_};

      artdaq::Fragments fragments;

      auto const begin = reader.dataAt(processedBlocks_);
      auto const end = begin + reader.blockSize(processedBlocks_);
      auto data = begin;

      while (data != end) {
        auto const non_const_data // Yep, you read that right.  This is a const pointer to non-const data.
          = const_cast<mu2eFragment::Header::data_t*>(data);
        DTCLib::DTC_DataPacket const dataPacket {reinterpret_cast<void*>(non_const_data)};
        DTCLib::DTC_DataHeaderPacket headerPacket {dataPacket};
        if (headerPacket.GetSubsystem() != S)
          continue;

        auto const byteCount = headerPacket.GetByteCount();

        // DTC_DataHeaderPacket -- add GetSystemID
        //   - 4914 - 0 (tracker), 1 (calorimeter)

        auto const wordCount = byteCount/sizeof(artdaq::RawDataType);
        auto const packetSize = (byteCount%8 == 0 )? wordCount : wordCount+1;
        fragments.emplace_back(artdaq::Fragment::dataFrag(fragment_->sequenceID(),
                                                          fragment_->fragmentID(),
                                                          reinterpret_cast<artdaq::RawDataType const*>(data),
                                                          packetSize,
                                                          fragment_->timestamp()));
        data += byteCount;
      }
      return std::make_unique<decltype(fragments)>(std::move(fragments));
    }
  }
}
