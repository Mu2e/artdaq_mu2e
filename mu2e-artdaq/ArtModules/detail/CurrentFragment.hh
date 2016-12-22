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
        return current_-reader_->dataBegin();
      }

      void advanceOneBlock()
      {
        assert(reader_);
        current_ = reader_->dataAt(processedSuperBlocks() + 1ull);
      }

      bool empty() const
      {
        return current_ == nullptr || current_ == reader_->dataEnd();
      }

      std::unique_ptr<artdaq::Fragments> extractFragmentsFromBlock(DTCLib::Subsystem);

    private:
      artdaq::FragmentPtr fragment_ {nullptr};
      std::unique_ptr<mu2eFragment const> reader_ {nullptr};
      mu2eFragment::Header::data_t const* current_ {nullptr};
    };

  } // detail
} // mu2e

#endif /* mu2e_artdaq_core_Overlays_Offline_detail_CurrentFragment_hh */
