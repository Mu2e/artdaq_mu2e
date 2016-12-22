#ifndef mu2e_artdaq_ArtModules_detail_CurrentFragment_hh
#define mu2e_artdaq_ArtModules_detail_CurrentFragment_hh

#include "artdaq-core/Data/Fragments.hh"
#include "dtcInterfaceLib/DTC_Types.h"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"

namespace mu2e {
  namespace detail {
    class CurrentFragment {
    public:

      CurrentFragment() = default;
      CurrentFragment(artdaq::Fragment const& f);

      std::size_t processedSuperBlocks() const
      {
        assert(reader_);
        return block_count_;
      }

      void advanceOneBlock()
      {
        assert(reader_);
        current_ = reinterpret_cast<const uint8_t*>(reader_->dataAt(++block_count_));
      }

      bool empty() const
      {
        return current_ == nullptr || current_ >= reinterpret_cast<const uint8_t*>(reader_->dataEnd());
      }

      std::unique_ptr<artdaq::Fragments> extractFragmentsFromBlock(DTCLib::Subsystem);

    private:
      artdaq::FragmentPtr fragment_ {nullptr};
      std::unique_ptr<mu2eFragment const> reader_ {nullptr};
      uint8_t const* current_ {nullptr};
      size_t block_count_;
    };

  } // detail
} // mu2e

#endif /* mu2e_artdaq_core_Overlays_Offline_detail_CurrentFragment_hh */
