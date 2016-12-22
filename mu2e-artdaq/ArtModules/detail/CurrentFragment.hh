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

      void advanceOneBlock() { ++processedSuperBlocks_; }
      bool empty() { return processedSuperBlocks_ == 0ull && processedSuperBlocks_ == nBlocks_; }

      std::unique_ptr<artdaq::Fragments> extractFragmentsFromBlock(DTCLib::Subsystem);

    private:
      artdaq::FragmentPtr fragment_ {nullptr};
      mu2eFragment::Header::count_t nBlocks_ {};
      mu2eFragment::Header::count_t processedSuperBlocks_ {};
    };

  } // detail
} // mu2e

#endif /* mu2e_artdaq_core_Overlays_Offline_detail_CurrentFragment_hh */
