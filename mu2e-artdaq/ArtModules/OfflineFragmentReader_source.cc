#include "art/Framework/Core/InputSourceMacros.h"
#include "art/Framework/IO/Sources/Source.h"
#include "art/Framework/IO/Sources/put_product_in_principal.h"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/Fragments.hh"
#include "canvas/Persistency/Provenance/FileFormatVersion.h"
#include "canvas/Utilities/Exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "mu2e-artdaq/ArtModules/OfflineFragmentReader.hh"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"

#include <sys/time.h>

using namespace mu2e::detail;

namespace {
  constexpr char const* daq_module_label {"daq"};
  std::string trk_instance_name() { return "trk"; }
  std::string calo_instance_name() { return "calo"; }
}

mu2e::OfflineFragmentReader::OfflineFragmentReader(Parameters const& ps,
                                                   art::ProductRegistryHelper& help,
                                                   art::SourceHelper const& pm) :
  pMaker_{pm},
  waitingTime_{ps().waiting_time()},
  resumeAfterTimeout_{ps().resume_after_timeout()}
{
  help.reconstitutes<artdaq::Fragments, art::InEvent>(daq_module_label, trk_instance_name());
  help.reconstitutes<artdaq::Fragments, art::InEvent>(daq_module_label, calo_instance_name());
}

void mu2e::OfflineFragmentReader::readFile(std::string const&, art::FileBlock*& fb)
{
  fb = new art::FileBlock{art::FileFormatVersion{1, "RawEvent2011"}, "nothing"};
}

bool mu2e::OfflineFragmentReader::readNext(art::RunPrincipal* const& inR,
                                           art::SubRunPrincipal* const& inSR,
                                           art::RunPrincipal*& outR,
                                           art::SubRunPrincipal*& outSR,
                                           art::EventPrincipal*& outE)
{
  if (outputFileCloseNeeded_) {
    outputFileCloseNeeded_ = false;
    return false;
  }

  // Establish default 'results'
  outR = nullptr;
  outSR = nullptr;
  outE = nullptr;
  art::Timestamp const currentTime = time(0);

  // Get new fragment
  if (currentFragment_.empty()) {

    // Try to get an event from the queue. We'll continuously loop, either until:
    //   1) we have read a RawEvent off the queue, or
    //   2) we have timed out, AND we are told the when we timeout we
    //      should stop.
    // In any case, if we time out, we emit an informational message.
    bool keep_looping {true};
    bool got_event {false};
    while (keep_looping) {
      keep_looping = false;
      got_event = incomingEvents_.deqTimedWait(poppedEvent_, waitingTime_);
      if (!got_event) {
        mf::LogInfo("InputFailure")
          << "Reading timed out in OfflineFragmentReader::readNext()";
        keep_looping = resumeAfterTimeout_;
      }
    }
    // We return false, indicating we're done reading, if:
    //   1) we did not obtain an event, because we timed out and were
    //      configured NOT to keep trying after a timeout, or
    //   2) the event we read was the end-of-data marker: a null
    //      pointer
    if (!got_event || !poppedEvent_) {
      shutdownMsgReceived_ = true;
      return false;
    }

    // Check the number of fragments in the RawEvent.  If we have a single
    // fragment and that fragment is marked as EndRun or EndSubrun we'll create
    // the special principals for that.

    // make new run if inR is 0 or if the run has changed
    if (inR == nullptr || inR->run() != poppedEvent_->runID()) {
      outR = pMaker_.makeRunPrincipal(poppedEvent_->runID(), currentTime);
    }

    if (poppedEvent_->numFragments() == 1) {
      if (poppedEvent_->releaseProduct(artdaq::Fragment::EndOfRunFragmentType)->size() == 1) {
        art::EventID const evid {art::EventID::flushEvent()};
        outR = pMaker_.makeRunPrincipal(evid.runID(), currentTime);
        outSR = pMaker_.makeSubRunPrincipal(evid.subRunID(), currentTime);
        outE = pMaker_.makeEventPrincipal(evid, currentTime);
        return true;
      }
      else if (poppedEvent_->releaseProduct(artdaq::Fragment::EndOfSubrunFragmentType)->size() == 1) {
        // Check if inR == nullptr or is a new run
        if (inR == nullptr || inR->run() != poppedEvent_->runID()) {
          outSR = pMaker_.makeSubRunPrincipal(poppedEvent_->runID(),
                                              poppedEvent_->subrunID(),
                                              currentTime);
          art::EventID const evid {art::EventID::flushEvent(outSR->id())};
          outE = pMaker_.makeEventPrincipal(evid, currentTime);
        }
        else {
          // If the previous subrun was neither 0 nor flush and was identical with the current
          // subrun, then it must have been associated with a data event.  In that case, we need
          // to generate a flush event with a valid run but flush subrun and event number in order
          // to end the subrun.
          if (inSR != nullptr && !inSR->id().isFlush() && inSR->subRun() == poppedEvent_->subrunID()) {
            art::EventID const evid {art::EventID::flushEvent(inR->id())};
            outSR = pMaker_.makeSubRunPrincipal(evid.subRunID(), currentTime);
            outE = pMaker_.makeEventPrincipal(evid, currentTime);
          }
          else {
            outSR = pMaker_.makeSubRunPrincipal(poppedEvent_->runID(),
                                                poppedEvent_->subrunID(),
                                                currentTime);
            art::EventID const evid {art::EventID::flushEvent(outSR->id())};
            outE = pMaker_.makeEventPrincipal(evid, currentTime);
          }
          outR = nullptr;
        }
        outputFileCloseNeeded_ = true;
        return true;
      }
    }

    // Make new subrun if inSR is NULL or if the subrun has changed
    art::SubRunID const subrun_check {poppedEvent_->runID(), poppedEvent_->subrunID()};
    if (inSR == nullptr || subrun_check != inSR->id()) {
      outSR = pMaker_.makeSubRunPrincipal(poppedEvent_->runID(),
                                          poppedEvent_->subrunID(),
                                          currentTime);
    }

    // Remove uninteresting fragments -- do not store
    poppedEvent_->releaseProduct(artdaq::Fragment::EmptyFragmentType);

    // Only remaining fragment is the interesting one
    auto fragments = poppedEvent_->releaseProduct();
    assert(fragments->size() == 1ull);
    currentFragment_ = CurrentFragment{fragments->front()};
  }


  idHandler_.update(*poppedEvent_);
  outE = pMaker_.makeEventPrincipal(idHandler_.run(),
                                    idHandler_.subRun(),
                                    idHandler_.event(),
                                    currentTime);

  using namespace std::string_literals;
  put_product_in_principal(currentFragment_.extractFragmentsFromBlock(DTCLib::Subsystem::Tracker),
                           *outE,
                           daq_module_label,
                           trk_instance_name());
  put_product_in_principal(currentFragment_.extractFragmentsFromBlock(DTCLib::Subsystem::Calorimeter),
                           *outE,
                           daq_module_label,
                           calo_instance_name());
  currentFragment_.advanceOneBlock();
  return true;
}

DEFINE_ART_INPUT_SOURCE(art::Source<mu2e::OfflineFragmentReader>)
