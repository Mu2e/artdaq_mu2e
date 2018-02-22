
#include "artdaq/DAQdata/Globals.hh"

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

	start:
		bool keep_looping = true;
		bool got_event = false;
		auto sleepTimeUsec = waitingTime_ * 1000; // waiting_time * 1000000 us/s / 1000 reps = us/rep
		if (sleepTimeUsec > 100000) sleepTimeUsec = 100000; // Don't wait longer than 1/10th of a second
		while (keep_looping)
		{
			keep_looping = false;
			auto start_time = std::chrono::steady_clock::now();
			while (!got_event && artdaq::TimeUtils::GetElapsedTime(start_time) < waitingTime_)
			{
				got_event = incoming_events->ReadyForRead();
				if (!got_event)
				{
					usleep(sleepTimeUsec);
					//TLOG_INFO("SharedMemoryReader") << "Waited " << std::to_string(TimeUtils::GetElapsedTime(start_time)) << " of " << std::to_string(waiting_time) << TLOG_ENDL;
				}
			}
			if (!got_event)
			{
				TLOG_INFO("OfflineFragmentReader")
					<< "InputFailure: Reading timed out in SharedMemoryReader::readNext()" << TLOG_ENDL;
				keep_looping = resumeAfterTimeout_;
			}
		}

		if (!got_event)
		{
			TLOG_INFO("OfflineFragmentReader") << "Did not receive an event from Shared Memory, returning false" << TLOG_ENDL;
			shutdownMsgReceived_ = true;
			return false;
		}
		TLOG_DEBUG("OfflineFragmentReader") << "Got Event!" << TLOG_ENDL;

		auto errflag = false;
		evtHeader.reset(incoming_events->ReadHeader(errflag));
		if (errflag) goto start; // Buffer was changed out from under reader!
		auto fragmentTypes = incoming_events->GetFragmentTypes(errflag);
		if (errflag) goto start; // Buffer was changed out from under reader!
		if (fragmentTypes.size() == 0)
		{
			TLOG_ERROR("OfflineFragmentReader") << "Event has no Fragments! Aborting!" << TLOG_ENDL;
			incoming_events->ReleaseBuffer();
			return false;
		}
		auto firstFragmentType = *fragmentTypes.begin();

		// We return false, indicating we're done reading, if:
		//   1) we did not obtain an event, because we timed out and were
		//      configured NOT to keep trying after a timeout, or
		//   2) the event we read was the end-of-data marker: a null
		//      pointer
		if (firstFragmentType == artdaq::Fragment::EndOfDataFragmentType)
		{
			TLOG_DEBUG("SharedMemoryReader") << "Received shutdown message, returning false" << TLOG_ENDL;
			shutdownMsgReceived_ = true;
			incoming_events->ReleaseBuffer();
			return false;
		}


		// Check the number of fragments in the RawEvent.  If we have a single
		// fragment and that fragment is marked as EndRun or EndSubrun we'll create
		// the special principals for that.
		art::Timestamp currentTime = time(0);

		// make new run if inR is 0 or if the run has changed
		if (inR == 0 || inR->run() != evtHeader->run_id)
		{
			outR = pMaker_.makeRunPrincipal(evtHeader->run_id, currentTime);
		}

		if (firstFragmentType == artdaq::Fragment::EndOfRunFragmentType)
		{
			art::EventID const evid(art::EventID::flushEvent());
			outR = pMaker_.makeRunPrincipal(evid.runID(), currentTime);
			outSR = pMaker_.makeSubRunPrincipal(evid.subRunID(), currentTime);
			outE = pMaker_.makeEventPrincipal(evid, currentTime);
			incoming_events->ReleaseBuffer();
			return true;
		}
		else if (firstFragmentType == artdaq::Fragment::EndOfSubrunFragmentType)
		{
			// Check if inR == 0 or is a new run
			if (inR == 0 || inR->run() != evtHeader->run_id)
			{
				outSR = pMaker_.makeSubRunPrincipal(evtHeader->run_id,
					evtHeader->subrun_id,
					currentTime);
				art::EventID const evid(art::EventID::flushEvent(outSR->id()));
				outE = pMaker_.makeEventPrincipal(evid, currentTime);
			}
			else
			{
				// If the previous subrun was neither 0 nor flush and was identical with the current
				// subrun, then it must have been associated with a data event.  In that case, we need
				// to generate a flush event with a valid run but flush subrun and event number in order
				// to end the subrun.
				if (inSR != 0 && !inSR->id().isFlush() && inSR->subRun() == evtHeader->subrun_id)
				{
					art::EventID const evid(art::EventID::flushEvent(inR->id()));
					outSR = pMaker_.makeSubRunPrincipal(evid.subRunID(), currentTime);
					outE = pMaker_.makeEventPrincipal(evid, currentTime);
					// If this is either a new or another empty subrun, then generate a flush event with
					// valid run and subrun numbers but flush event number
					//} else if(inSR==0 || inSR->id().isFlush()){
				}
				else
				{
					outSR = pMaker_.makeSubRunPrincipal(evtHeader->run_id,
						evtHeader->subrun_id,
						currentTime);
					art::EventID const evid(art::EventID::flushEvent(outSR->id()));
					outE = pMaker_.makeEventPrincipal(evid, currentTime);
					// Possible error condition
					//} else {
				}
				outR = 0;
			}
			//outputFileCloseNeeded = true;
			incoming_events->ReleaseBuffer();
			return true;
		}

		// make new subrun if inSR is 0 or if the subrun has changed
		art::SubRunID subrun_check(evtHeader->run_id, evtHeader->subrun_id);
		if (inSR == 0 || subrun_check != inSR->id())
		{
			outSR = pMaker_.makeSubRunPrincipal(evtHeader->run_id,
				evtHeader->subrun_id,
				currentTime);
		}


		for (auto& type_code : fragmentTypes)
		{
			// Remove uninteresting fragments -- do not store
			if (type_code == artdaq::Fragment::EmptyFragmentType) continue;

			auto product = incoming_events->GetFragmentsByType(errflag, type_code);
			if (errflag) goto start; // Buffer was changed out from under reader!

			assert(product->size() == 1ull);
			currentFragment_ = CurrentFragment{ std::move(product->front()) };
			break;
		}
	}


	idHandler_.update(*evtHeader); // See note in mu2e::detail::EventIDHandler::update()
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
