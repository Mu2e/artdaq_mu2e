#ifndef mu2e_artdaq_ArtModules_detail_CurrentFragment_hh
#define mu2e_artdaq_ArtModules_detail_CurrentFragment_hh

#include "dtcInterfaceLib/DTC_Types.h"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"

namespace mu2e {
  namespace detail {
    class CurrentFragment {
    public:

      CurrentFragment() = default;
      CurrentFragment(artdaq::Fragment const& f) :
        fragment_{std::make_unique<artdaq::Fragment>(f)},
        nBlocks_{mu2eFragment{*fragment_}.nBlocks()}
      {}

      void advanceOneBlock() { ++processedBlocks_; }
      bool empty() { return processedBlocks_ == 0ull && processedBlocks_ == nBlocks_; }

      template <DTCLib::Subsystem>
      std::unique_ptr<artdaq::Fragments> extractFragmentsFromBlock();

    private:
      artdaq::FragmentPtr fragment_ {nullptr};
      mu2eFragment::Header::count_t nBlocks_ {};
      mu2eFragment::Header::count_t processedBlocks_ {};
    };
  } // detail
} // mu2e

#endif /* mu2e_artdaq_core_Overlays_Offline_detail_CurrentFragment_hh */
