#ifndef mu2e_artdaq_core_Overlays_Offline_OfflineFragmentReader_hh
#define mu2e_artdaq_core_Overlays_Offline_OfflineFragmentReader_hh

#include "art/Framework/Core/Frameworkfwd.h"
#include "art/Framework/Core/FileBlock.h"
#include "art/Framework/Core/ProductRegistryHelper.h"
#include "art/Framework/IO/Sources/SourceHelper.h"
#include "art/Framework/IO/Sources/SourceTraits.h"
#include "art/Framework/Principal/EventPrincipal.h"
#include "art/Framework/Principal/RunPrincipal.h"
#include "art/Framework/Principal/SubRunPrincipal.h"
#include "art/Utilities/ConfigTable.h"
#include "artdaq-core/Core/GlobalQueue.hh"
#include "fhiclcpp/types/OptionalSequence.h"
#include "fhiclcpp/types/Sequence.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/Tuple.h"
#include "mu2e-artdaq/ArtModules/detail/CurrentFragment.hh"
#include "mu2e-artdaq/ArtModules/detail/EventIDHandler.hh"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"

#include <string>

namespace mu2e {

  class OfflineFragmentReader {
  public:

    OfflineFragmentReader(OfflineFragmentReader const&) = delete;
    OfflineFragmentReader& operator=(OfflineFragmentReader const&) = delete;

    struct Config {
      fhicl::Atom<std::string> module_type { fhicl::Name("module_type") };
      fhicl::Atom<int64_t> maxSubRuns { fhicl::Name("maxSubRuns"), -1 };
      fhicl::Atom<int64_t> maxEvents { fhicl::Name("maxEvents"), -1 };
      fhicl::Atom<double> waiting_time { fhicl::Name("waiting_time"), fhicl::Comment("Units are in seconds for below parameter."), 86400. };
      fhicl::Atom<bool> resume_after_timeout { fhicl::Name("resume_after_timeout"), true };
      struct KeysToIgnore {
        std::set<std::string> operator()() { return {"module_label"}; }
      };
    };

    using Parameters = art::ConfigTable<Config, Config::KeysToIgnore>;

    OfflineFragmentReader(Parameters const& ps,
                          art::ProductRegistryHelper& help,
                          art::SourceHelper const& pm);

    OfflineFragmentReader(fhicl::ParameterSet const& ps,
                          art::ProductRegistryHelper& help,
                          art::SourceHelper const& pm) :
      OfflineFragmentReader{Parameters{ps}, help, pm}
    {};

    void closeCurrentFile() {}
    void readFile(std::string const& name, art::FileBlock*& fb);

    bool hasMoreData() const {return !shutdownMsgReceived_;}

    bool readNext(art::RunPrincipal* const& inR,
                  art::SubRunPrincipal* const& inSR,
                  art::RunPrincipal*& outR,
                  art::SubRunPrincipal*& outSR,
                  art::EventPrincipal*& outE);

  private:

    art::SourceHelper const pMaker_;
    artdaq::RawEventQueue& incomingEvents_ {artdaq::getGlobalQueue()};
    daqrate::seconds waitingTime_;
    bool resumeAfterTimeout_;
    bool shutdownMsgReceived_ {false};
    bool outputFileCloseNeeded_ {false};
    detail::EventIDHandler idHandler_ {};
    artdaq::RawEvent_ptr poppedEvent_ {nullptr};
    detail::CurrentFragment currentFragment_ {};
  };

} // mu2e

// Specialize an art source trait to tell art that we don't care about
// source.fileNames and don't want the files services to be used.
namespace art {
  template <>
  struct Source_generator<mu2e::OfflineFragmentReader> : std::true_type {};
}

#endif /* mu2e_artdaq_core_Overlays_Offline_OfflineFragmentReader_hh */
