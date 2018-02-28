#ifndef mu2e_artdaq_ArtModules_detail_CurrentFragment_hh
#define mu2e_artdaq_ArtModules_detail_CurrentFragment_hh

#include <cassert>
#include "artdaq-core/Data/Fragment.hh"
#include "dtcInterfaceLib/DTC_Types.h"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"

namespace mu2e {
  namespace detail {

    class CurrentFragment {
    public:

      CurrentFragment() = default;
      CurrentFragment& operator=(CurrentFragment const&) = delete;
      CurrentFragment& operator=(CurrentFragment&&) = default;
      virtual ~CurrentFragment() { reader_.reset(nullptr); fragment_.reset(nullptr); }

      // This is using pass-by-value so that we can take advantage of
      // artdaq::Fragment's move c'tor.
      CurrentFragment(artdaq::Fragment f);

      std::size_t processedSuperBlocks() const
      {
        return block_count_;
      }

      void advanceOneBlock()
      {
        assert(reader_);
        current_ = reinterpret_cast<const uint8_t*>(reader_->dataAt(++block_count_));
      }

      bool empty() const
      {
	if(!reader_ || !fragment_) return true;
        return block_count_ >= reader_->hdr_block_count();
      }

      std::unique_ptr<artdaq::Fragments> extractFragmentsFromBlock(DTCLib::DTC_Subsystem);
	  size_t getFragmentCount(DTCLib::DTC_Subsystem);

    private:
      std::unique_ptr<artdaq::Fragment const> fragment_ {nullptr};
      std::unique_ptr<mu2eFragment const> reader_ {nullptr};
      uint8_t const* current_ {nullptr};
      size_t block_count_;
    };

  } // detail
} // mu2e

#endif /* mu2e_artdaq_core_Overlays_Offline_detail_CurrentFragment_hh */
