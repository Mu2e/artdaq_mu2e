#include "artdaq-mu2e/Generators/Mu2eEventReceiver.hh"

#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"
#include "artdaq/Generators/GeneratorMacros.hh"
#include "canvas/Utilities/Exception.h"
#include "cetlib_except/exception.h"
#include "dtcInterfaceLib/DTC_Types.h"
#include "fhiclcpp/ParameterSet.h"
#include "artdaq-core-mu2e/Overlays/FragmentType.hh"
#include "artdaq-core-mu2e/Overlays/Mu2eEventFragmentWriter.hh"
#include "artdaq/DAQdata/Globals.hh"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>

#include <unistd.h>
#include "trace.h"
#define TRACE_NAME "Mu2eEventReceiver"

mu2e::Mu2eEventReceiver::Mu2eEventReceiver(fhicl::ParameterSet const& ps)
	: CommandableFragmentGenerator(ps)
	, fragment_type_(toFragmentType("MU2EEVENT"))
	, fragment_ids_{static_cast<artdaq::Fragment::fragment_id_t>(fragment_id())}
	, mode_(DTCLib::DTC_SimModeConverter::ConvertToSimMode(ps.get<std::string>("sim_mode", "Disabled")))
	, board_id_(static_cast<uint8_t>(ps.get<int>("board_id", 0)))
	, rawOutput_(ps.get<bool>("raw_output_enable", false))
	, rawOutputFile_(ps.get<std::string>("raw_output_file", "/tmp/Mu2eReceiver.bin"))
	, print_packets_(ps.get<bool>("debug_print", false))
	, heartbeats_after_(ps.get<size_t>("null_heartbeats_after_requests", 16))
{
	// mode_ can still be overridden by environment!
	theInterface_ = new DTCLib::DTC(mode_,
									ps.get<int>("dtc_id", -1),
									ps.get<unsigned>("roc_mask", 0x1),
									ps.get<std::string>("dtc_fw_version", ""),
									ps.get<bool>("skip_dtc_init", false),
									ps.get<std::string>("simulator_memory_file_name", "mu2esim.bin"));

	fhicl::ParameterSet cfoConfig = ps.get<fhicl::ParameterSet>("cfo_config", fhicl::ParameterSet());
	theCFO_ = new DTCLib::DTCSoftwareCFO(theInterface_,
										 cfoConfig.get<bool>("use_dtc_cfo_emulator", true),
										 cfoConfig.get<size_t>("debug_packet_count", 0),
										 DTCLib::DTC_DebugTypeConverter::ConvertToDebugType(cfoConfig.get<std::string>("debug_type", "2")),
										 cfoConfig.get<bool>("sticky_debug_type", false),
										 cfoConfig.get<bool>("quiet", false),
										 cfoConfig.get<bool>("asyncRR", false),
										 cfoConfig.get<bool>("force_no_debug_mode", false),
										 cfoConfig.get<bool>("useCFODRP", false));
	mode_ = theInterface_->ReadSimMode();

	TLOG(TLVL_DEBUG) << "Mu2eEventReceiver Initialized with mode " << mode_;

	TLOG(TLVL_INFO) << "The DTC Firmware version string is: " << theInterface_->ReadDesignVersion();

	if (ps.get<bool>("load_sim_file", false))
	{
		theInterface_->SetDetectorEmulatorInUse();
		theInterface_->ResetDDR();

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
	}
	else
	{
		theInterface_->ClearDetectorEmulatorInUse();  // Needed if we're doing ROC Emulator...make sure Detector Emulation
													  // is disabled
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
	delete theInterface_;
	delete theCFO_;
}

void mu2e::Mu2eEventReceiver::stop()
{
	rawOutputStream_.close();
	theInterface_->DisableDetectorEmulator();
	theInterface_->DisableCFOEmulation();
}

void mu2e::Mu2eEventReceiver::start()
{
	if (rawOutput_) {
		std::string fileName = rawOutputFile_;
		if(fileName.find(".bin") != std::string::npos) {
			std::string timestr = "_" + std::to_string(time(0));
			fileName.insert(fileName.find(".bin"), timestr);
		}
		rawOutputStream_.open(fileName, std::ios::out | std::ios::app | std::ios::binary);
		}
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

	std::vector<std::unique_ptr<DTCLib::DTC_Event>> data;
	uint64_t z = 0;
	DTCLib::DTC_EventWindowTag zero(z);

	if (mode_ != 0)
	{
		TLOG(TLVL_INFO) << "Sending request for timestamp " << ev_counter();
		theCFO_->SendRequestForTimestamp(DTCLib::DTC_EventWindowTag(ev_counter()), heartbeats_after_);
	}

	auto before_read = std::chrono::steady_clock::now();
	int retryCount = 5;
	while (data.size() == 0 && retryCount >= 0)
	{
		try
		{
			TLOG(TLVL_TRACE + 25) << "Calling theInterface->GetData(zero)";
			data = theInterface_->GetData(zero);
			TLOG(TLVL_TRACE + 25) << "Done calling theInterface->GetData(zero) data.size()=" << data.size() << ", retryCount=" << retryCount;
		}
		catch (std::exception const& ex)
		{
			TLOG(TLVL_ERROR) << "There was an error in the DTC Library: " << ex.what();
		}
		retryCount--;
	}
	if (retryCount < 0 && data.size() == 0)
	{
		// Return true if no data in external CFO mode, otherwise false
		return mode_ == 0;
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
					TLOG(TLVL_INFO) << first->toJSON();
					for (int ii = 0; ii < first->GetPacketCount(); ++ii)
					{
						TLOG(TLVL_INFO) << DTCLib::DTC_DataPacket(((uint8_t*)block->blockPointer) + ((ii + 1) * 16)).toJSON()
								  << std::endl;
					}
				}
			}
		}
	}
	if (rawOutput_)
	{
		for (auto& evt : data)
		{
			evt->WriteEvent(rawOutputStream_, false);
		}
	}

	// auto after_print = std::chrono::steady_clock::now();

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

	TLOG(TLVL_TRACE + 20) << "Creating Mu2eEventFragment";
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
