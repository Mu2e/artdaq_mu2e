#define TRACE_NAME "OfflineFragmentReader"
#include "artdaq/DAQdata/Globals.hh"

#include "artdaq/ArtModules/ArtdaqSharedMemoryService.h"
#include "art/Framework/Core/InputSourceMacros.h"
#include "art/Framework/IO/Sources/Source.h"
#include "art/Framework/IO/Sources/put_product_in_principal.h"
#include "artdaq-core/Data/Fragment.hh"
#include "canvas/Persistency/Provenance/FileFormatVersion.h"
#include "canvas/Utilities/Exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
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
// instance names of the fragments corresponding to the tracker,
// calorimeter, and crv are "trk", "calo", and "crv", respectively.
constexpr char const* daq_module_label{"daq"};
std::string trk_instance_name() { return "trk"; }
std::string calo_instance_name() { return "calo"; }
std::string crv_instance_name() { return "crv"; }
std::string header_instance_name() { return "header"; }
}  // namespace

mu2e::OfflineFragmentReader::OfflineFragmentReader(fhicl::ParameterSet const& ps, art::ProductRegistryHelper& help,
												   art::SourceHelper const& pm)
	: pMaker_{pm}
	, debugEventNumberMode_(ps.get<bool>("debug_event_number_mode", false))
	, evtHeader_(new artdaq::detail::RawEventHeader(0, 0, 0, 0))
	, readTrkFragments_(ps.get<bool>("readTrkFragments", true))
	, readCaloFragments_(ps.get<bool>("readCaloFragments", true))
	, readCrvFragments_(ps.get<bool>("readCrvFragments", false))
{
	// Instantiate ArtdaqSharedMemoryService to set up artdaq Globals and MetricManager
	art::ServiceHandle<ArtdaqSharedMemoryServiceInterface> shm;

	help.reconstitutes<mu2e::Mu2eEventHeader, art::InEvent>(daq_module_label, header_instance_name());
	if (readTrkFragments_) help.reconstitutes<artdaq::Fragments, art::InEvent>(daq_module_label, trk_instance_name());
	if (readCaloFragments_) help.reconstitutes<artdaq::Fragments, art::InEvent>(daq_module_label, calo_instance_name());
	if (readCrvFragments_) help.reconstitutes<artdaq::Fragments, art::InEvent>(daq_module_label, crv_instance_name());
}

void mu2e::OfflineFragmentReader::readFile(std::string const&, art::FileBlock*& fb)
{
	fb = new art::FileBlock{art::FileFormatVersion{1, "RawEvent2011"}, "nothing"};
}

mu2e::OfflineFragmentReader::~OfflineFragmentReader()
{
}

bool mu2e::OfflineFragmentReader::readNext(art::RunPrincipal* const& inR, art::SubRunPrincipal* const& inSR,
										   art::RunPrincipal*& outR, art::SubRunPrincipal*& outSR,
										   art::EventPrincipal*& outE)
{
	// Establish default 'results'
	outR = nullptr;
	outSR = nullptr;
	outE = nullptr;

	if (outputFileCloseNeeded_)
	{
		outputFileCloseNeeded_ = false;
		return false;
	}

	art::ServiceHandle<ArtdaqSharedMemoryServiceInterface> shm;

	// Get new fragment if nothing is stored
	if (currentFragment_.empty())
	{
	  std::unordered_map<artdaq::Fragment::type_t, std::unique_ptr<artdaq::Fragments>> eventData;

		while(eventData.count(mu2e::FragmentType::DTC) == 0 && eventData.count(mu2e::FragmentType::MU2E) == 0) {
		 eventData = shm->ReceiveEvent(false);

		  if (eventData.count(mu2e::FragmentType::DTC) > 0 || eventData.count(mu2e::FragmentType::MU2E) > 0)
		    {
		      evtHeader_ = shm->GetEventHeader();
		    }
		  else if (eventData.count(artdaq::Fragment::EndOfDataFragmentType) > 0)
		    {
		      if (evtHeader_ != nullptr)
			{
			  TLOG_INFO("OfflineFragmentReader") << "Received EndOfData Message. The remaining  "
							     << currentFragment_.sizeRemaining() << " blocks from DAQ event "
							     << evtHeader_->sequence_id << " will be lost.";
			}
		      shutdownMsgReceived_ = true;
		      return false;
		    }
		  else if(eventData.size() == 0)
		    {
		      TLOG_INFO("OfflineFragmentReader") << "Did not receive an event from Shared Memory, returning false" << TLOG_ENDL;
		      shutdownMsgReceived_ = true;
		      return false;
		    } else {
		    TLOG_INFO("OfflineFragmentReader") << "Received event of unknown type from shared memory, ignoring";
		    usleep(10000);
		    continue;
		  }
		}
		TLOG_DEBUG("OfflineFragmentReader") << "Got Event!" << TLOG_ENDL;

		// We return false, indicating we're done reading, if:
		//   1) we did not obtain an event, because we timed out and were
		//      configured NOT to keep trying after a timeout, or
		//   2) the event we read was the end-of-data marker: a null
		//      pointer

		// Check the number of fragments in the RawEvent.  If we have a single
		// fragment and that fragment is marked as EndRun or EndSubrun we'll create
		// the special principals for that.

		TLOG_DEBUG("OfflineFragmentReader") << "Iterating through Fragment types";
		for (auto const& fragments : eventData)
		{
			// Remove uninteresting fragments -- do not store
			if (fragments.first == artdaq::Fragment::EmptyFragmentType) continue;

			assert(fragments.second->size() == 1ull);
			TLOG_DEBUG("OfflineFragmentReader") << "Creating CurrentFragment using Fragment of type " << fragments.first;
			currentFragment_ = CurrentFragment{std::move(fragments.second->front()), debugEventNumberMode_};
			break;
		}
	}

	// Making two calls to extractFragmentsFromBlock is likely
	// inefficient.  However, it is used here for now to clean up the
	// interface.  If efficiency becomes important at this stage, then
	// we can alter the call structure to be something like:
	//
	//    auto fragmentsColls = currentFragment_.extractFragmentsFromBlock(Tracker, Calorimeter);
	//    auto const& trkFragments = fragmentsColls[Tracker]; // etc.

	try
	{
		TLOG_DEBUG("OfflineFragmentReader") << "Updating Run/Subrun/Event IDs";
		idHandler_.update(*evtHeader_, currentFragment_.getCurrentTimestamp());  // See note in mu2e::detail::EventIDHandler::update()

		art::Timestamp currentTime = time(0);
		// make new run if inR is 0 or if the run has changed
		if (inR == 0 || inR->run() != idHandler_.run())
		{
			outR = pMaker_.makeRunPrincipal(idHandler_.run(), currentTime);
		}

		// make new subrun if inSR is 0 or if the subrun has changed
		art::SubRunID subrun_check(idHandler_.run(), idHandler_.subRun());
		//		if (inSR == 0 || subrun_check != inSR->id()) {
		if (inSR == 0 || subrun_check != inSR->subRunID())
		{
			outSR = pMaker_.makeSubRunPrincipal(idHandler_.run(), idHandler_.subRun(), currentTime);
		}

		TLOG_DEBUG("OfflineFragmentReader") << "Creating event principal for event " << idHandler_.event();
		outE = pMaker_.makeEventPrincipal(idHandler_.run(), idHandler_.subRun(), idHandler_.event(), currentTime);

		TLOG_DEBUG("OfflineFragmentReader") << "Extracting Mu2e Event Header from CurrentFragment";
		auto mu2eHeader = currentFragment_.makeMu2eEventHeader();
		TLOG_DEBUG("OfflineFragmentReader") << "Putting Mu2e Event Header into Mu2e Event";
		put_product_in_principal(std::move(mu2eHeader), *outE, daq_module_label, header_instance_name());

		TLOG_DEBUG("OfflineFragmentReader") << "Getting Tracker, Calorimeter, and Crv Fragments from CurrentFragment";
		TLOG_TRACE("OfflineFragmentReader") << "This event has "
											<< currentFragment_.getFragmentCount(DTCLib::DTC_Subsystem_Tracker)
											<< (readTrkFragments_ ? "Tracker Fragments, " : "Tracker Fragments (ignored), ")
											<< currentFragment_.getFragmentCount(DTCLib::DTC_Subsystem_Calorimeter)
											<< (readCaloFragments_ ? "Calorimeter Fragments, and " : "Calorimeter Fragments (ignored), and ")
											<< currentFragment_.getFragmentCount(DTCLib::DTC_Subsystem_CRV)
											<< (readCrvFragments_ ? "CRV Fragments." : "CRV Fragments (ignored).");

		if (readTrkFragments_)
		{
			TLOG_TRACE("OfflineFragmentReader") << "Extracting Tracker Fragments from CurrentFragment";
			put_product_in_principal(currentFragment_.extractFragmentsFromBlock(DTCLib::DTC_Subsystem_Tracker), *outE,
									 daq_module_label, trk_instance_name());
		}
		if (readCaloFragments_)
		{
			TLOG_TRACE("OfflineFragmentReader") << "Extracting Calorimeter Fragments from CurrentFragment";
			put_product_in_principal(currentFragment_.extractFragmentsFromBlock(DTCLib::DTC_Subsystem_Calorimeter), *outE,
									 daq_module_label, calo_instance_name());
		}
		if (readCrvFragments_)
		{
			TLOG_TRACE("OfflineFragmentReader") << "Extracting Crv Fragments from CurrentFragment";
			put_product_in_principal(currentFragment_.extractFragmentsFromBlock(DTCLib::DTC_Subsystem_CRV), *outE,
									 daq_module_label, crv_instance_name());
		}
		TLOG_TRACE("OfflineFragmentReader") << "Advancing to next block";
		currentFragment_.advanceOneBlock();
		TLOG_DEBUG("OfflineFragmentReader") << "Done extracting Tracker, Calorimeter, and Crv Fragments from CurrentFragment";

		return true;
	}
	catch (...)
	{
		TLOG(TLVL_ERROR) << "Error retrieving Tracker, Calorimeter, and Crv Fragments from current Event. Shutting down.";
		shutdownMsgReceived_ = true;
		outR = nullptr;
		outSR = nullptr;
		outE = nullptr;
		return false;
	}
}

DEFINE_ART_INPUT_SOURCE(art::Source<mu2e::OfflineFragmentReader>)
