#include "art/Framework/Core/InputSourceMacros.h"
#include "art/Framework/IO/Sources/Source.h"
#include "art/Framework/IO/Sources/put_product_in_principal.h"
#include "artdaq-core/Data/Fragment.hh"
#include "canvas/Persistency/Provenance/FileFormatVersion.h"
#include "canvas/Utilities/Exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "mu2e-artdaq/ArtModules/OfflineFragmentReader.hh"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"
#include "artdaq/DAQdata/Globals.hh"

#include <sys/time.h>

// Notes:
//
// This input source started from the artdaq RawInput source.  Various
// simplifications were introduced based on the specifics required by
// Mu2e.  The implementation for fetching artdaq events and setting
// Run and SubRun principles was taken from the RawInput source
// directly, with minor updates to reflect C++14 syntax.
//
// Where things differ is in how art::Events are emitted.  The
// implemented behavior is as follows:
//
//  - If there is no artdaq event/fragment being processed, fetch an
//    event from the global queue.
//
//  - Set up the Run and SubRun principals as necessary.
//
//  - Dump all uninteresting fragments from the artdaq Event, denoted
//    by the type EmptyFragmentType.
//
//  - Cache the one remaining interesting fragment.
//
//  - Break apart the remaining fragment into its superblocks (around
//    2500 per artdaq fragment).
//
//  - Per superblock, create two objects of type artdaq::Fragments
//    that store the tracker and calorimeter data blocks separately,
//    but where each data block has been converted to an
//    artdaq::Fragment.  There should now be two artdaq::Fragments
//    objects, where one object contains all tracker-related artdaq
//    fragments, and the other contains all calorimeter-related artdaq
//    fragments.
//
//  - The objects mentioned perviously are placed onto an art::Event,
//    with the instance names "trk" and "calo" for the tracker- and
//    calorimeter-related fragments, respectively.
//
//  - An art::Event is then emitted (since readNext returns 'true'
//    with a nonzero EventPrincipal pointer), and the superblock
//    counter is incremented so that a new art::Event can be created
//    from the currently-cached fragment.
//
// To test this source, please specify it as the input source in your
// configuration, and then run the OfflineFragmentsDumper analyzer
// module with it.
//
// N.B. The art::EventIDs are incremental--i.e. the timestamp
//      information from the fragment is not used to create an
//      art::EventID.  Rather, the ID is monotonically increasing
//      based on the number of processed superblocks.  The experiment
//      may decide it wants to change this behavior.

using namespace mu2e::detail;

namespace {
	// Per agreement, the fictitious module label is "daq" and the
	// instance names of the fragments corresponding to the tracker and
	// calorimeter are "trk" and "calo", respectively.
	constexpr char const* daq_module_label{ "daq" };
	std::string trk_instance_name() { return "trk"; }
	std::string calo_instance_name() { return "calo"; }
}

mu2e::OfflineFragmentReader::OfflineFragmentReader(fhicl::ParameterSet const& ps,
												   art::ProductRegistryHelper& help,
												   art::SourceHelper const& pm) :
	pMaker_{ pm },
	waitingTime_(ps.get<double>("waiting_time", 86400.)),
	resumeAfterTimeout_(ps.get<bool>("resume_after_timeout", true))
{
	help.reconstitutes<artdaq::Fragments, art::InEvent>(daq_module_label, trk_instance_name());
	help.reconstitutes<artdaq::Fragments, art::InEvent>(daq_module_label, calo_instance_name());
}

void mu2e::OfflineFragmentReader::readFile(std::string const&, art::FileBlock*& fb)
{
	fb = new art::FileBlock{ art::FileFormatVersion{1, "RawEvent2011"}, "nothing" };
}

bool mu2e::OfflineFragmentReader::readNext(art::RunPrincipal* const& inR,
										   art::SubRunPrincipal* const& inSR,
										   art::RunPrincipal*& outR,
										   art::SubRunPrincipal*& outSR,
										   art::EventPrincipal*& outE)
{
	if (outputFileCloseNeeded_)
	{
		outputFileCloseNeeded_ = false;
		return false;
	}

	// Establish default 'results'
	outR = nullptr;
	outSR = nullptr;
	outE = nullptr;
	art::Timestamp const currentTime = time(0);

	// Get new fragment if nothing is stored
	if (currentFragment_.empty())
	{

		// Try to get an event from the queue. We'll continuously loop, either until:
		//   1) we have read a RawEvent off the queue, or
		//   2) we have timed out, AND we are told the when we timeout we
		//      should stop.
		// In any case, if we time out, we emit an informational message.
		bool keep_looping{ true };
		bool got_event{ false };
		while (keep_looping)
		{
			keep_looping = false;
			got_event = incomingEvents_.deqTimedWait(poppedEvent_, waitingTime_);
			if (!got_event)
			{
				TLOG_INFO("InputFailure") << "Reading timed out in OfflineFragmentReader::readNext()" << TLOG_ENDL;
				keep_looping = resumeAfterTimeout_;
			}
		}
		// We return false, indicating we're done reading, if:
		//   1) we did not obtain an event, because we timed out and were
		//      configured NOT to keep trying after a timeout, or
		//   2) the event we read was the end-of-data marker: a null
		//      pointer
		if (!got_event || !poppedEvent_)
		{
			shutdownMsgReceived_ = true;
			return false;
		}

		// Check the number of fragments in the RawEvent.  If we have a single
		// fragment and that fragment is marked as EndRun or EndSubrun we'll create
		// the special principals for that.

		// make new run if inR is 0 or if the run has changed
		if (inR == nullptr || inR->run() != poppedEvent_->runID())
		{
			outR = pMaker_.makeRunPrincipal(poppedEvent_->runID(), currentTime);
		}

		if (poppedEvent_->numFragments() == 1)
		{
			if (poppedEvent_->releaseProduct(artdaq::Fragment::EndOfRunFragmentType)->size() == 1)
			{
				art::EventID const evid{ art::EventID::flushEvent() };
				outR = pMaker_.makeRunPrincipal(evid.runID(), currentTime);
				outSR = pMaker_.makeSubRunPrincipal(evid.subRunID(), currentTime);
				outE = pMaker_.makeEventPrincipal(evid, currentTime);
				return true;
			}
			else if (poppedEvent_->releaseProduct(artdaq::Fragment::EndOfSubrunFragmentType)->size() == 1)
			{
				// Check if inR == nullptr or is a new run
				if (inR == nullptr || inR->run() != poppedEvent_->runID())
				{
					outSR = pMaker_.makeSubRunPrincipal(poppedEvent_->runID(),
														poppedEvent_->subrunID(),
														currentTime);
					art::EventID const evid{ art::EventID::flushEvent(outSR->id()) };
					outE = pMaker_.makeEventPrincipal(evid, currentTime);
				}
				else
				{
					// If the previous subrun was neither 0 nor flush and was identical with the current
					// subrun, then it must have been associated with a data event.  In that case, we need
					// to generate a flush event with a valid run but flush subrun and event number in order
					// to end the subrun.
					if (inSR != nullptr && !inSR->id().isFlush() && inSR->subRun() == poppedEvent_->subrunID())
					{
						art::EventID const evid{ art::EventID::flushEvent(inR->id()) };
						outSR = pMaker_.makeSubRunPrincipal(evid.subRunID(), currentTime);
						outE = pMaker_.makeEventPrincipal(evid, currentTime);
					}
					else
					{
						outSR = pMaker_.makeSubRunPrincipal(poppedEvent_->runID(),
															poppedEvent_->subrunID(),
															currentTime);
						art::EventID const evid{ art::EventID::flushEvent(outSR->id()) };
						outE = pMaker_.makeEventPrincipal(evid, currentTime);
					}
					outR = nullptr;
				}
				outputFileCloseNeeded_ = true;
				return true;
			}
		}

		// Make new subrun if inSR is NULL or if the subrun has changed
		art::SubRunID const subrun_check{ poppedEvent_->runID(), poppedEvent_->subrunID() };
		if (inSR == nullptr || subrun_check != inSR->id())
		{
			outSR = pMaker_.makeSubRunPrincipal(poppedEvent_->runID(),
												poppedEvent_->subrunID(),
												currentTime);
		}

		// Remove uninteresting fragments -- do not store
		poppedEvent_->releaseProduct(artdaq::Fragment::EmptyFragmentType);

		// Only remaining fragment is the interesting one
		auto fragments = poppedEvent_->releaseProduct();
		assert(fragments->size() == 1ull);
		currentFragment_ = CurrentFragment{ std::move(fragments->front()) };
	}


	idHandler_.update(*poppedEvent_); // See note in mu2e::detail::EventIDHandler::update()
	outE = pMaker_.makeEventPrincipal(idHandler_.run(),
									  idHandler_.subRun(),
									  idHandler_.event(),
									  currentTime);

	// Making two calls to extractFragmentsFromBlock is likely
	// inefficient.  However, it is used here for now to clean up the
	// interface.  If efficiency becomes important at this stage, then
	// we can alter the call structure to be something like:
	//
	//    auto fragmentsColls = currentFragment_.extractFragmentsFromBlock(Tracker, Calorimeter);
	//    auto const& trkFragments = fragmentsColls[Tracker]; // etc.
	put_product_in_principal(currentFragment_.extractFragmentsFromBlock(DTCLib::DTC_Subsystem_Tracker),
							 *outE,
							 daq_module_label,
							 trk_instance_name());
	put_product_in_principal(currentFragment_.extractFragmentsFromBlock(DTCLib::DTC_Subsystem_Calorimeter),
							 *outE,
							 daq_module_label,
							 calo_instance_name());
	currentFragment_.advanceOneBlock();
	return true;
}


DEFINE_ART_INPUT_SOURCE(art::Source<mu2e::OfflineFragmentReader>)
