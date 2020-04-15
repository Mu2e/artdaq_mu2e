#define TRACE_NAME "OfflineFragmentReader"
#include "artdaq/DAQdata/Globals.hh"

#include "art/Framework/Core/InputSourceMacros.h"
#include "art/Framework/IO/Sources/Source.h"
#include "art/Framework/IO/Sources/put_product_in_principal.h"
#include "artdaq-core/Data/Fragment.hh"
#include "canvas/Persistency/Provenance/FileFormatVersion.h"
#include "canvas/Utilities/Exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"
#include "mu2e-artdaq/ArtModules/OfflineFragmentReader.hh"

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

#ifndef CARE_ABOUT_END_RUN_FRAGMENTS
#define CARE_ABOUT_END_RUN_FRAGMENTS 0
#endif

#if ART_HEX_VERSION < 0x30000
#define RUN_ID id
#define SUBRUN_ID id
#define EVENT_ID id
#else
#define RUN_ID runID
#define SUBRUN_ID subRunID
#define EVENT_ID eventID
#endif

#define build_key(seed) seed + ((GetPartitionNumber() + 1) << 16) + (getppid() & 0xFFFF)

using namespace mu2e::detail;

namespace {
// Per agreement, the fictitious module label is "daq" and the
// instance names of the fragments corresponding to the tracker and
// calorimeter are "trk" and "calo", respectively.
constexpr char const* daq_module_label{"daq"};
std::string trk_instance_name() { return "trk"; }
std::string calo_instance_name() { return "calo"; }
std::string header_instance_name() { return "header"; }
}  // namespace

mu2e::OfflineFragmentReader::OfflineFragmentReader(fhicl::ParameterSet const& ps, art::ProductRegistryHelper& help,
												   art::SourceHelper const& pm)
  : pMaker_{pm}
							      , waitingTime_(ps.get<double>("waiting_time", 30.))
, resumeAfterTimeout_(ps.get<bool>("resume_after_timeout", true))
							      , debugEventNumberMode_(ps.get<bool>("debug_event_number_mode", false))
,evtHeader_(0, 0, 0, 0)
{
	incoming_events.reset(new artdaq::SharedMemoryEventReceiver(
		ps.get<uint32_t>("shared_memory_key", build_key(0xEE000000)),
		ps.get<uint32_t>("broadcast_shared_memory_key", build_key(0xBB000000))));

	help.reconstitutes<artdaq::Fragments, art::InEvent>(daq_module_label, trk_instance_name());
	help.reconstitutes<artdaq::Fragments, art::InEvent>(daq_module_label, calo_instance_name());
}

void mu2e::OfflineFragmentReader::readFile(std::string const&, art::FileBlock*& fb)
{
	fb = new art::FileBlock{art::FileFormatVersion{1, "RawEvent2011"}, "nothing"};
}

mu2e::OfflineFragmentReader::~OfflineFragmentReader()
{
	if (incoming_events) incoming_events.reset(nullptr);
}

bool mu2e::OfflineFragmentReader::readNext(art::RunPrincipal* const& inR, art::SubRunPrincipal* const& inSR,
										   art::RunPrincipal*& outR, art::SubRunPrincipal*& outSR,
										   art::EventPrincipal*& outE)
{
	// Establish default 'results'
	outR = nullptr;
	outSR = nullptr;
	outE = nullptr;

	if (outputFileCloseNeeded_) {
		outputFileCloseNeeded_ = false;
		return false;
	}

	// Check for broadcast fragments
	bool err = true;
	while (err) {
		if (!incoming_events->ReadyForRead(true, 0)) break;

		auto types = incoming_events->GetFragmentTypes(err);
		if (err) continue;

		if (types.size() == 0) {
			TLOG_ERROR("OfflineFragmentReader") << "Event has no Fragments! Aborting!" << TLOG_ENDL;
			incoming_events->ReleaseBuffer();
			return false;
		}

		auto firstFragmentType = *types.begin();
		if (firstFragmentType == artdaq::Fragment::EndOfDataFragmentType) {
			TLOG_INFO("OfflineFragmentReader") << "Received EndOfData Message. The remaining  "
											   << currentFragment_.sizeRemaining() << " blocks from DAQ event "
											   << evtHeader_.sequence_id << " will be lost.";
			shutdownMsgReceived_ = true;
			incoming_events->ReleaseBuffer();
			return false;
		}

			// ELF 03/08/2018: Ignoring EndOfRun and EndOfSubrun messages for now
#if CARE_ABOUT_END_RUN_FRAGMENTS
		auto oldDAQEvent = evtHeader_.sequence_id;
		auto hdrPtr = incoming_events->ReadHeader(err);
		if (err) continue;
		if (!hdrPtr) return false;
		evtHeader_ = artdaq::detail::RawEventHeader(*hdrPtr);
		if (firstFragmentType == artdaq::Fragment::EndOfRunFragmentType) {
			TLOG_INFO("OfflineFragmentReader") << "Received EndOfRun Message. The remaining "
											   << currentFragment_.sizeRemaining() << " block from DAQ event " << oldDAQEvent
											   << " will be in the next run.";
			art::EventID const evid(art::EventID::flushEvent());
			outR = pMaker_.makeRunPrincipal(evid.runID(), currentTime);
			outSR = pMaker_.makeSubRunPrincipal(evid.subRunID(), currentTime);
			outE = pMaker_.makeEventPrincipal(evid, currentTime);
			incoming_events->ReleaseBuffer();
			return true;
		}
		else if (firstFragmentType == artdaq::Fragment::EndOfSubrunFragmentType)
		{
			TLOG_INFO("OfflineFragmentReader") << "Received EndOfSubrun Message. The remaining "
											   << currentFragment_.sizeRemaining() << " block from DAQ event " << oldDAQEvent
											   << " will be in the next subrun.";
			// Check if inR == 0 or is a new run
			if (inR == 0 || inR->run() != evtHeader_.run_id) {
				outSR = pMaker_.makeSubRunPrincipal(evtHeader_.run_id, evtHeader_.subrun_id, currentTime);
				art::EventID const evid(art::EventID::flushEvent(outSR->SUBRUN_ID()));
				outE = pMaker_.makeEventPrincipal(evid, currentTime);
			}
			else
			{
				// If the previous subrun was neither 0 nor flush and was identical with the current
				// subrun, then it must have been associated with a data event.  In that case, we need
				// to generate a flush event with a valid run but flush subrun and event number in order
				// to end the subrun.
				if (inSR != 0 && !inSR->SUBRUN_ID().isFlush() && inSR->subRun() == evtHeader_.subrun_id) {
					art::EventID const evid(art::EventID::flushEvent(inR->RUN_ID()));
					outSR = pMaker_.makeSubRunPrincipal(evid.subRunID(), currentTime);
					outE = pMaker_.makeEventPrincipal(evid, currentTime);
					// If this is either a new or another empty subrun, then generate a flush event with
					// valid run and subrun numbers but flush event number
					//} else if(inSR==0 || inSR->id().isFlush()){
				}
				else
				{
					outSR = pMaker_.makeSubRunPrincipal(evtHeader_.run_id, evtHeader_.subrun_id, currentTime);
					art::EventID const evid(art::EventID::flushEvent(outSR->SUBRUN_ID()));
					outE = pMaker_.makeEventPrincipal(evid, currentTime);
					// Possible error condition
					//} else {
				}
				outR = 0;
			}
			incoming_events->ReleaseBuffer();
			return true;
		}
#else
		incoming_events->ReleaseBuffer();
		err = false;
#endif
	}

	// Get new fragment if nothing is stored
	if (currentFragment_.empty()) {
	start:
		bool keep_looping = true;
		bool got_event = false;
		auto sleepTimeUsec = waitingTime_ * 1000;            // waiting_time * 1000000 us/s / 1000 reps = us/rep
		if (sleepTimeUsec > 100000) sleepTimeUsec = 100000;  // Don't wait longer than 1/10th of a second
		while (keep_looping) {
			keep_looping = false;
			auto start_time = std::chrono::steady_clock::now();
			while (!got_event && artdaq::TimeUtils::GetElapsedTime(start_time) < waitingTime_) {
				got_event = incoming_events->ReadyForRead();
				if (!got_event) {
					usleep(sleepTimeUsec);
					// TLOG_INFO("SharedMemoryReader") << "Waited " << std::to_string(TimeUtils::GetElapsedTime(start_time)) << "
					// of " << std::to_string(waiting_time) << TLOG_ENDL;
				}
			}
			if (!got_event) {
				TLOG_INFO("OfflineFragmentReader")
					<< "InputFailure: Reading timed out in SharedMemoryReader::readNext()" << TLOG_ENDL;
				keep_looping = resumeAfterTimeout_;
			}
		}

		if (!got_event) {
			TLOG_INFO("OfflineFragmentReader") << "Did not receive an event from Shared Memory, returning false" << TLOG_ENDL;
			shutdownMsgReceived_ = true;
			return false;
		}
		TLOG_DEBUG("OfflineFragmentReader") << "Got Event!" << TLOG_ENDL;

		auto errflag = false;
		{
			auto hdrPtr = incoming_events->ReadHeader(errflag);
			if (errflag) goto start;
			if (!hdrPtr) return false;
			evtHeader_ = artdaq::detail::RawEventHeader(*hdrPtr);
		}
		TLOG_DEBUG("OfflineFragmentReader") << "Calling GetFragmentTypes";
		auto fragmentTypes = incoming_events->GetFragmentTypes(errflag);
		if (errflag) goto start;  // Buffer was changed out from under reader!
		if (fragmentTypes.size() == 0) {
			TLOG_ERROR("OfflineFragmentReader") << "Event has no Fragments! Aborting!" << TLOG_ENDL;
			incoming_events->ReleaseBuffer();
			return false;
		}

		auto firstFragmentType = *fragmentTypes.begin();
		if (firstFragmentType == artdaq::Fragment::EndOfDataFragmentType) {
			TLOG_INFO("OfflineFragmentReader") << "Received EndOfData Message. The remaining  "
											   << currentFragment_.sizeRemaining() << " blocks from DAQ event "
											   << evtHeader_.sequence_id << " will be lost.";
			shutdownMsgReceived_ = true;
			incoming_events->ReleaseBuffer();
			return false;
		}


		// We return false, indicating we're done reading, if:
		//   1) we did not obtain an event, because we timed out and were
		//      configured NOT to keep trying after a timeout, or
		//   2) the event we read was the end-of-data marker: a null
		//      pointer

		// Check the number of fragments in the RawEvent.  If we have a single
		// fragment and that fragment is marked as EndRun or EndSubrun we'll create
		// the special principals for that.


		TLOG_DEBUG("OfflineFragmentReader") << "Iterating through Fragment types";
		  for (auto& type_code : fragmentTypes) {
		    // Remove uninteresting fragments -- do not store
		    if (type_code == artdaq::Fragment::EmptyFragmentType) continue;

		    auto product = incoming_events->GetFragmentsByType(errflag, type_code);
		    if (errflag) goto start;  // Buffer was changed out from under reader!

		    assert(product->size() == 1ull);
		    TLOG_DEBUG("OfflineFragmentReader") << "Creating CurrentFragment using Fragment of type " << type_code;
		currentFragment_ = CurrentFragment{std::move(product->front()), debugEventNumberMode_};
		    break;
		  }
		  incoming_events->ReleaseBuffer();
		}

		// Making two calls to extractFragmentsFromBlock is likely
		// inefficient.  However, it is used here for now to clean up the
		// interface.  If efficiency becomes important at this stage, then
		// we can alter the call structure to be something like:
		//
		//    auto fragmentsColls = currentFragment_.extractFragmentsFromBlock(Tracker, Calorimeter);
		//    auto const& trkFragments = fragmentsColls[Tracker]; // etc.


		try {

		TLOG_DEBUG("OfflineFragmentReader") << "Updating Run/Subrun/Event IDs";
		  idHandler_.update(evtHeader_, currentFragment_.getCurrentTimestamp());  // See note in mu2e::detail::EventIDHandler::update()

		art::Timestamp currentTime = time(0);
		  // make new run if inR is 0 or if the run has changed
		  if (inR == 0 || inR->run() != idHandler_.run()) {
		    outR = pMaker_.makeRunPrincipal(idHandler_.run(), currentTime);
		  }

		  // make new subrun if inSR is 0 or if the subrun has changed
		  art::SubRunID subrun_check(idHandler_.run(), idHandler_.subRun());
		  //		if (inSR == 0 || subrun_check != inSR->id()) {
		  if (inSR == 0 || subrun_check != inSR->subRunID()) {
		    outSR = pMaker_.makeSubRunPrincipal(idHandler_.run(), idHandler_.subRun(), currentTime);
		  }

		TLOG_DEBUG("OfflineFragmentReader") << "Creating event principal for event " << idHandler_.event();
		outE = pMaker_.makeEventPrincipal(idHandler_.run(), idHandler_.subRun(), idHandler_.event(), currentTime);

		put_product_in_principal(currentFragment_.makeMu2eEventHeader(), *outE, daq_module_label, header_instance_name());

		TLOG_TRACE("OfflineFragmentReader") << "This event has "
						    << currentFragment_.getFragmentCount(DTCLib::DTC_Subsystem_Tracker)
						    << " Tracker Fragments and "
						    << currentFragment_.getFragmentCount(DTCLib::DTC_Subsystem_Calorimeter)
						    << " Calorimeter Fragments.";
		put_product_in_principal(currentFragment_.extractFragmentsFromBlock(DTCLib::DTC_Subsystem_Tracker), *outE,
					 daq_module_label, trk_instance_name());
		put_product_in_principal(currentFragment_.extractFragmentsFromBlock(DTCLib::DTC_Subsystem_Calorimeter), *outE,
					 daq_module_label, calo_instance_name());
		currentFragment_.advanceOneBlock();

		return true;
		} catch(...) {

		  TLOG(TLVL_ERROR) << "Error retrieving Tracker and Calorimeter Fragments from current Event. Shutting down.";
		  shutdownMsgReceived_ = true;
		  outR = nullptr;
		  outSR = nullptr;
		  outE = nullptr;
		  return false;
		}
}

DEFINE_ART_INPUT_SOURCE(art::Source<mu2e::OfflineFragmentReader>)
