#include "mu2e-artdaq/Generators/Mu2eEventReceiver.hh"

#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"
#include "artdaq/Generators/GeneratorMacros.hh"
#include "canvas/Utilities/Exception.h"
#include "cetlib_except/exception.h"
#include "dtcInterfaceLib/DTC_Types.h"
#include "fhiclcpp/ParameterSet.h"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "mu2e-artdaq-core/Overlays/Mu2eEventFragmentWriter.hh"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>

#include <unistd.h>
#include "trace.h"
#define TRACE_NAME "Mu2eEventReceiver"

mu2e::Mu2eEventReceiver::Mu2eEventReceiver(fhicl::ParameterSet const& ps)
	: CommandableFragmentGenerator(ps)
	, fragment_type_(toFragmentType("DTC"))
	, fragment_ids_{static_cast<artdaq::Fragment::fragment_id_t>(fragment_id())}
	, mode_(DTCLib::DTC_SimModeConverter::ConvertToSimMode(ps.get<std::string>("sim_mode", "Disabled")))
	, board_id_(static_cast<uint8_t>(ps.get<int>("board_id", 0)))
	, print_packets_(ps.get<bool>("debug_print", false))
{
	// mode_ can still be overridden by environment!
	theInterface_ = new DTCLib::DTC(mode_, -1, 1, "", false, ps.get<std::string>("simulator_memory_file_name", "mu2esim.bin"));
	theCFO_ = new DTCLib::DTCSoftwareCFO(theInterface_, true);
	mode_ = theInterface_->ReadSimMode();

	TLOG(TLVL_DEBUG) << "Mu2eEventReceiver Initialized with mode " << mode_;

	//	int ringRocs[] = {ps.get<int>("ring_0_roc_count", -1), ps.get<int>("ring_1_roc_count", -1),
	//					  ps.get<int>("ring_2_roc_count", -1), ps.get<int>("ring_3_roc_count", -1),
	//					  ps.get<int>("ring_4_roc_count", -1), ps.get<int>("ring_5_roc_count", -1)};
	//
	//	bool ringTiming[] = {ps.get<bool>("ring_0_timing_enabled", true), ps.get<bool>("ring_1_timing_enabled", true),
	//						 ps.get<bool>("ring_2_timing_enabled", true), ps.get<bool>("ring_3_timing_enabled", true),
	//						 ps.get<bool>("ring_4_timing_enabled", true), ps.get<bool>("ring_5_timing_enabled", true)};
	//
	//	bool ringEmulators[] = {
	//		ps.get<bool>("ring_0_roc_emulator_enabled", false), ps.get<bool>("ring_1_roc_emulator_enabled", false),
	//		ps.get<bool>("ring_2_roc_emulator_enabled", false), ps.get<bool>("ring_3_roc_emulator_enabled", false),
	//		ps.get<bool>("ring_4_roc_emulator_enabled", false), ps.get<bool>("ring_5_roc_emulator_enabled", false)};
	//
	//	for (int ring = 0; ring < 6; ++ring)
	//	{
	//		if (ringRocs[ring] >= 0)
	//		{
	//			theInterface_->EnableRing(DTCLib::DTC_Rings[ring], DTCLib::DTC_RingEnableMode(true, true, ringTiming[ring]),
	//									  DTCLib::DTC_ROCS[ringRocs[ring]]);
	//			if (ringEmulators[ring])
	//			{
	//				theInterface_->EnableROCEmulator(DTCLib::DTC_Rings[ring]);
	//			}
	//			else
	//			{
	//				theInterface_->DisableROCEmulator(DTCLib::DTC_Rings[ring]);
	//			}
	//		}
	//	}

	char* file_c = getenv("DTCLIB_SIM_FILE");

	auto sim_file = ps.get<std::string>("sim_file", "");
	if (file_c != nullptr)
	{
		sim_file = std::string(file_c);
	}
	if (sim_file.size() > 0)
	{
		simFileRead_ = false;
		std::thread reader(&mu2e::Mu2eEventReceiver::readSimFile_, this, sim_file);
		reader.detach();
	}
	else
	{
		simFileRead_ = true;
	}
}

void mu2e::Mu2eEventReceiver::readSimFile_(std::string sim_file)
{
	TLOG(TLVL_INFO) << "Starting read of simulation file " << sim_file << "."
					<< " Please wait to start the run until finished.";
	theInterface_->WriteSimFileToDTC(sim_file, true);
	simFileRead_ = true;
	TLOG(TLVL_INFO) << "Done reading simulation file into DTC memory.";
}

mu2e::Mu2eEventReceiver::~Mu2eEventReceiver()
{
	theInterface_->DisableDetectorEmulator();
	delete theInterface_;
	delete theCFO_;
}

bool mu2e::Mu2eEventReceiver::getNext_(artdaq::FragmentPtrs& frags)
{
	while (!simFileRead_ && !should_stop())
	{
		usleep(5000);
	}

	if (should_stop())
	{
		return false;
	}

	// And use it, along with the artdaq::Fragment header information
	// (fragment id, sequence id, and user type) to create a fragment

	// Constructor used is:

	// Fragment(std::size_t payload_size, sequence_id_t sequence_id,
	//  fragment_id_t fragment_id, type_t type, const T & metadata);

	// ...where we'll start off setting the payload (data after the
	// header and metadata) to empty; this will be resized below

	// Now we make an instance of the overlay to put the data into...

	std::vector<std::unique_ptr<DTCLib::DTC_Event>> data;
	uint64_t z = 0;
	DTCLib::DTC_EventWindowTag zero(z);

	if (mode_ != 0)
	{
		theCFO_->SendRequestForTimestamp(DTCLib::DTC_EventWindowTag(ev_counter()));
	}

	auto before_read = std::chrono::steady_clock::now();
	int retryCount = 5;
	while (data.size() == 0 && retryCount >= 0)
	{
		try
		{
			TLOG(TLVL_TRACE + 25) << "Calling theInterface->GetData(zero)";
			data = theInterface_->GetData(zero);
			TLOG(TLVL_TRACE + 25) << "Done calling theInterface->GetData(zero)";
		}
		catch (std::exception const& ex)
		{
			TLOG(TLVL_ERROR) << "There was an error in the DTC Library: " << ex.what();
		}
		retryCount--;
	}
	if (retryCount < 0 && data.size() == 0)
	{
		return false;
	}
	auto after_read = std::chrono::steady_clock::now();

	DTCLib::DTC_EventWindowTag ts = data[0]->GetEventWindowTag();

	if (print_packets_)
	{
		for (auto& evt : data)
		{
			for (size_t se = 0; se < evt->GetSubEventCount(); ++se)
			{
				auto subevt = evt->GetSubEvent(se);
				for (size_t bl = 0; bl < subevt->GetDataBlockCount(); ++bl)
				{
					auto block = subevt->GetDataBlock(bl);
					auto first = block->GetHeader();
					TLOG(TLVL_INFO) << first.toJSON();
					for (int ii = 0; ii < first.GetPacketCount(); ++ii)
					{
						std::cout << "\t" << DTCLib::DTC_DataPacket(((uint8_t*)block->blockPointer) + ((ii + 1) * 16)).toJSON()
								  << std::endl;
					}
				}
			}
		}
	}

	size_t total_size = 0;
	for (auto& evt : data)
	{
		total_size += evt->GetEventByteCount();
	}

	//auto after_print = std::chrono::steady_clock::now();

	auto fragment_timestamp = ts.GetEventWindowTag(true);
	if (fragment_timestamp < highest_timestamp_seen_)
	{
		fragment_timestamp += timestamp_loops_ * highest_timestamp_seen_;
	}
	else if (fragment_timestamp > highest_timestamp_seen_)
	{
		highest_timestamp_seen_ = fragment_timestamp;
	}
	else
	{
		fragment_timestamp += timestamp_loops_ * highest_timestamp_seen_;
		timestamp_loops_++;
	}

	frags.emplace_back(new artdaq::Fragment(ev_counter(), fragment_ids_[0], fragment_type_, fragment_timestamp));
	Mu2eEventFragmentWriter evtfrag(*frags.back(), ts.GetEventWindowTag(true), data[0]->GetHeader()->evb_mode);
	evtfrag.fill_event(data, fragment_timestamp);

	auto after_copy = std::chrono::steady_clock::now();
	TLOG(TLVL_TRACE + 20) << "Incrementing event counter";
	ev_counter_inc();

	TLOG(TLVL_TRACE + 20) << "Reporting Metrics";
	auto hwTime = theInterface_->GetDevice()->GetDeviceTime();

	double hw_timestamp_rate = 1 / hwTime;
	double hw_data_rate = frags.back()->sizeBytes() / hwTime;

	metricMan->sendMetric("DTC Read Time", artdaq::TimeUtils::GetElapsedTime(after_read, after_copy), "s", 3, artdaq::MetricMode::Average);
	metricMan->sendMetric("Fragment Prep Time", artdaq::TimeUtils::GetElapsedTime(before_read, after_read), "s", 3, artdaq::MetricMode::Average);
	metricMan->sendMetric("HW Timestamp Rate", hw_timestamp_rate, "timestamps/s", 1, artdaq::MetricMode::Average);
	metricMan->sendMetric("PCIe Transfer Rate", hw_data_rate, "B/s", 1, artdaq::MetricMode::Average);

	TLOG(TLVL_TRACE + 20) << "Returning true";

	return true;
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::Mu2eEventReceiver)
