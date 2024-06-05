#include "artdaq-mu2e/Generators/Mu2eEventReceiverBase.hh"
#include "artdaq-core-mu2e/Overlays/FragmentType.hh"

#include "artdaq-core/Data/ContainerFragmentLoader.hh"
#include "artdaq/DAQdata/Globals.hh"

#include "trace.h"
#define TRACE_NAME "Mu2eEventReceiverBase"

mu2e::Mu2eEventReceiverBase::Mu2eEventReceiverBase(fhicl::ParameterSet const& ps)
	: CommandableFragmentGenerator(ps)
	, fragment_ids_{static_cast<artdaq::Fragment::fragment_id_t>(fragment_id())}
	, mode_(DTCLib::DTC_SimModeConverter::ConvertToSimMode(ps.get<std::string>("sim_mode", "Disabled")))
	, skip_dtc_init_(ps.get<bool>("skip_dtc_init", false))
	, rawOutput_(ps.get<bool>("raw_output_enable", false))
	, rawOutputFile_(ps.get<std::string>("raw_output_file", "/tmp/Mu2eReceiver.bin"))
	, print_packets_(ps.get<bool>("debug_print", false))
	, heartbeats_after_(ps.get<size_t>("null_heartbeats_after_requests", 16))
	, dtc_offset_(ps.get<size_t>("dtc_position_in_chain", 0))
	, n_dtcs_(ps.get<size_t>("n_dtcs_in_chain", 1))
        , request_rate_(ps.get<float>("request_rate", -1.))// Hz
        , diagLevel_(ps.get<int>("diagLevel", 0))
        , frag_sent_(0)
{
	// mode_ can still be overridden by environment!
	theInterface_ = std::make_unique<DTCLib::DTC>(mode_,
									ps.get<int>("dtc_id", -1),
									ps.get<unsigned>("roc_mask", 0x1),
									ps.get<std::string>("dtc_fw_version", ""),
									skip_dtc_init_,
									ps.get<std::string>("simulator_memory_file_name", "mu2esim.bin"));

	mode_ = theInterface_->GetSimMode();
	TLOG(TLVL_DEBUG) << "Mu2eEventReceiverBase Initialized with mode " << mode_;

	if(request_rate_ <= 0) request_rate_ = std::numeric_limits<double>::max();
	sending_start_ = std::chrono::steady_clock::now();

	//if in simulation mode, setup CFO
	if (mode_ != 0)
	{
		fhicl::ParameterSet cfoConfig = ps.get<fhicl::ParameterSet>("cfo_config", fhicl::ParameterSet());
		theCFO_ = std::make_unique<DTCLib::DTCSoftwareCFO>(theInterface_.get(),
											cfoConfig.get<bool>("use_dtc_cfo_emulator", true),
											cfoConfig.get<size_t>("debug_packet_count", 0),
											DTCLib::DTC_DebugTypeConverter::ConvertToDebugType(cfoConfig.get<std::string>("debug_type", "2")),
											cfoConfig.get<bool>("sticky_debug_type", false),
											cfoConfig.get<bool>("quiet", false),
											cfoConfig.get<bool>("asyncRR", false),
											cfoConfig.get<bool>("force_no_debug_mode", false),
											cfoConfig.get<bool>("useCFODRP", false));
	}
	
	if(skip_dtc_init_) return; //skip any control of DTC 

	if (ps.get<bool>("load_sim_file", false))
	{
		theInterface_->SetDetectorEmulatorInUse();
		theInterface_->ResetDDR();
		theInterface_->SoftReset();

		char* file_c = getenv("DTCLIB_SIM_FILE");

		auto sim_file = ps.get<std::string>("sim_file", "");
		if (file_c != nullptr)
		{
			sim_file = std::string(file_c);
		}
		if (sim_file.size() > 0)
		{
			simFileRead_ = false;
			std::thread reader(&mu2e::Mu2eEventReceiverBase::readSimFile_, this, sim_file);
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
mu2e::Mu2eEventReceiverBase::~Mu2eEventReceiverBase() {}

void mu2e::Mu2eEventReceiverBase::readSimFile_(std::string sim_file)
{
	TLOG(TLVL_INFO) << "Starting read of simulation file " << sim_file << "."
					<< " Please wait to start the run until finished.";
	theInterface_->WriteSimFileToDTC(sim_file, true);
	simFileRead_ = true;
	TLOG(TLVL_INFO) << "Done reading simulation file into DTC memory.";
}

void mu2e::Mu2eEventReceiverBase::stop()
{
	rawOutputStream_.close();
	
	if(skip_dtc_init_) return; //skip any control of DTC

	theInterface_->DisableDetectorEmulator();
	theInterface_->DisableCFOEmulation();
}

void mu2e::Mu2eEventReceiverBase::start()
{
	if (rawOutput_)
	{
		std::string fileName = rawOutputFile_;
		if (fileName.find(".bin") != std::string::npos)
		{
			std::string timestr = "_" + std::to_string(time(0));
			fileName.insert(fileName.find(".bin"), timestr);
		}
		rawOutputStream_.open(fileName, std::ios::out | std::ios::app | std::ios::binary);
	}
}

bool mu2e::Mu2eEventReceiverBase::getNextDTCFragment(artdaq::FragmentPtrs& frags, DTCLib::DTC_EventWindowTag ts_in, artdaq::Fragment::sequence_id_t seq_in)
{
	auto before_read = std::chrono::steady_clock::now();
	int retryCount = 5;
	std::vector<std::unique_ptr<DTCLib::DTC_Event>> data;
	while (data.size() == 0 && retryCount >= 0)
	{
		try
		{
			TLOG(TLVL_TRACE + 25) << "Calling theInterface->GetData(" << ts_in.GetEventWindowTag(true) << ")";
			data = theInterface_->GetData(ts_in);
			TLOG(TLVL_TRACE + 25) << "Done calling theInterface->GetData(" << ts_in.GetEventWindowTag(true) << ") data.size()=" << data.size() << ", retryCount=" << retryCount;
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

	DTCLib::DTC_EventWindowTag ts_out = data[0]->GetEventWindowTag();
	artdaq::Fragment::sequence_id_t seq_out = seq_in == 0 ? getCurrentSequenceID() : seq_in;
	if (ts_out.GetEventWindowTag(true) != ts_in.GetEventWindowTag(true))
	{
		TLOG(TLVL_TRACE) << "Requested timestamp " << ts_in.GetEventWindowTag(true) << ", received data with timestamp " << ts_out.GetEventWindowTag(true);
	}

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

	auto fragment_timestamp = ts_out.GetEventWindowTag(true);

	if (first_timestamp_seen_ == 0)
	{
		first_timestamp_seen_ = fragment_timestamp;
	}

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

	if (data.size() == 1)
	{
		TLOG(TLVL_TRACE + 20) << "Creating Fragment, sz=" << data[0]->GetEventByteCount();
		frags.emplace_back(new artdaq::Fragment(seq_out, fragment_ids_[0], FragmentType::DTCEVT, fragment_timestamp));
		frags.back()->resizeBytes(data[0]->GetEventByteCount());
		memcpy(frags.back()->dataBegin(), data[0]->GetRawBufferPointer(), data[0]->GetEventByteCount());
	}
	else
	{
		TLOG(TLVL_TRACE + 20) << "Creating ContainerFragment, sz=" << data.size();
		frags.emplace_back(new artdaq::Fragment(seq_out, fragment_ids_[0]));
		frags.back()->setTimestamp(fragment_timestamp);
		artdaq::ContainerFragmentLoader cfl(*frags.back());
		cfl.set_missing_data(false);

		for (auto& evt : data)
		{
			TLOG(TLVL_TRACE + 20) << "Creating Fragment, sz=" << data[0]->GetEventByteCount();
			artdaq::Fragment frag(seq_out, fragment_ids_[0], FragmentType::DTCEVT, fragment_timestamp);
			frag.resizeBytes(evt->GetEventByteCount());
			memcpy(frags.back()->dataBegin(), evt->GetRawBufferPointer(), evt->GetEventByteCount());
			cfl.addFragment(frag);
		}
	}

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

size_t mu2e::Mu2eEventReceiverBase::getCurrentSequenceID()
{
	return ((ev_counter()-1) * n_dtcs_) + dtc_offset_ + 1;
}
