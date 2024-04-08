// This file reads out DTCs that are NOT in HW Event-building mode
// It can be used as an exmaple for developing more specific functionality.

#include "artdaq-core-mu2e/Overlays/FragmentType.hh"
#include "artdaq-core-mu2e/Overlays/DTC_Packets.h"
#include "dtcInterfaceLib/DTC.h"
#include "dtcInterfaceLib/DTCSoftwareCFO.h"

#include "artdaq-core/Data/ContainerFragmentLoader.hh"
#include "artdaq/DAQdata/Globals.hh"
#include "artdaq/Generators/GeneratorMacros.hh"

#include <fstream>

#include "trace.h"
#define TRACE_NAME "Mu2eSubEventReceiver"

namespace mu2e {
class Mu2eSubEventReceiver : public artdaq::CommandableFragmentGenerator
{
public:
	explicit Mu2eSubEventReceiver(fhicl::ParameterSet const& ps);
	virtual ~Mu2eSubEventReceiver();

	DTCLib::DTC_SimMode GetMode() { return mode_; }

private:
	bool getNextDTCFragment(artdaq::FragmentPtrs& output, DTCLib::DTC_EventWindowTag ts);

	void start() override;

	void stopNoMutex() override {}

	void stop() override;

	void readSimFile_(std::string sim_file);

	size_t getCurrentSequenceID();

	// Like "getNext_", "fragmentIDs_" is a mandatory override; it
	// returns a vector of the fragment IDs an instance of this class
	// is responsible for (in the case of Mu2eSubEventReceiver, this is just
	// the fragment_id_ variable declared in the parent
	// CommandableFragmentGenerator class)

	std::vector<artdaq::Fragment::fragment_id_t> fragmentIDs_() { return fragment_ids_; }

	std::vector<artdaq::Fragment::fragment_id_t> fragment_ids_;

	// State
	size_t highest_timestamp_seen_{0};
	size_t timestamp_loops_{0};  // For playback mode, so that we continually generate unique timestamps
	DTCLib::DTC_SimMode mode_;
  bool simFileRead_{true};
	const bool skip_dtc_init_;
	bool rawOutput_{false};
	std::string rawOutputFile_{""};
	std::ofstream rawOutputStream_;
	bool print_packets_;
	size_t heartbeats_after_{16};

	size_t dtc_offset_{0};
	size_t n_dtcs_{1};
	size_t first_timestamp_seen_{0};

	std::unique_ptr<DTCLib::DTC> theInterface_;
	std::unique_ptr<DTCLib::DTCSoftwareCFO> theCFO_;

	std::size_t const throttle_usecs_;
	std::condition_variable throttle_cv_;
	std::mutex throttle_mutex_;
	int diagLevel_;
	// The "getNext_" function is used to implement user-specific
	// functionality; it's a mandatory override of the pure virtual
	// getNext_ function declared in CommandableFragmentGenerator

	bool getNext_(artdaq::FragmentPtrs& output) override;
	DTCLib::DTC_EventWindowTag getCurrentEventWindowTag();
};
}  // namespace mu2e

mu2e::Mu2eSubEventReceiver::~Mu2eSubEventReceiver()
{
}

bool mu2e::Mu2eSubEventReceiver::getNext_(artdaq::FragmentPtrs& frags)
{
	while (!simFileRead_ && !should_stop())
	{
		usleep(5000);
	}

	if(throttle_usecs_ > 0) {
	  std::unique_lock<std::mutex> throttle_lock(throttle_mutex_);
	  throttle_cv_.wait_for(throttle_lock, std::chrono::microseconds(throttle_usecs_), [&]() { return should_stop(); });
	}

	if (should_stop())
	{
		return false;
	}

	uint64_t z = 0;
	DTCLib::DTC_EventWindowTag zero(z);

	if (mode_ != DTCLib::DTC_SimMode_Disabled)
	{
		if (diagLevel_ > 0) TLOG(TLVL_INFO) << "Sending request for timestamp " << getCurrentEventWindowTag().GetEventWindowTag(true);
		theCFO_->SendRequestForTimestamp(getCurrentEventWindowTag(), heartbeats_after_);
	}

	return getNextDTCFragment(frags, zero);
}

DTCLib::DTC_EventWindowTag mu2e::Mu2eSubEventReceiver::getCurrentEventWindowTag()
{
	if (first_timestamp_seen_ > 0)
	{
		return DTCLib::DTC_EventWindowTag(getCurrentSequenceID() + first_timestamp_seen_);
	}

	return DTCLib::DTC_EventWindowTag(uint64_t(0));
}

mu2e::Mu2eSubEventReceiver::Mu2eSubEventReceiver(fhicl::ParameterSet const& ps)
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
	, throttle_usecs_(ps.get<size_t>("throttle_usecs", 0))  // in units of us
	, diagLevel_(ps.get<int>("diagLevel", 0))
{
	// mode_ can still be overridden by environment!
	theInterface_ = std::make_unique<DTCLib::DTC>(mode_,
												  ps.get<int>("dtc_id", -1),
												  ps.get<unsigned>("roc_mask", 0x1),
												  ps.get<std::string>("dtc_fw_version", ""),
												  skip_dtc_init_,
												  ps.get<std::string>("simulator_memory_file_name", "mu2esim.bin"));

	mode_ = theInterface_->GetSimMode();
	TLOG(TLVL_DEBUG) << "Mu2eSubEventReceiver Initialized with mode " << mode_;

	// if in simulation mode, setup CFO
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

	if (skip_dtc_init_) return;  // skip any control of DTC

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
			std::thread reader(&mu2e::Mu2eSubEventReceiver::readSimFile_, this, sim_file);
			reader.detach();
		}
	}
	else
	{
		theInterface_->ClearDetectorEmulatorInUse();  // Needed if we're doing ROC Emulator...make sure Detector Emulation
													  // is disabled
	}
}

void mu2e::Mu2eSubEventReceiver::readSimFile_(std::string sim_file)
{
	TLOG(TLVL_INFO) << "Starting read of simulation file " << sim_file << "."
					<< " Please wait to start the run until finished.";
	theInterface_->WriteSimFileToDTC(sim_file, true);
	simFileRead_ = true;
	TLOG(TLVL_INFO) << "Done reading simulation file into DTC memory.";
}

void mu2e::Mu2eSubEventReceiver::stop()
{
	rawOutputStream_.close();

	if (skip_dtc_init_) return;  // skip any control of DTC

	theInterface_->DisableDetectorEmulator();
	theInterface_->DisableCFOEmulation();
}

void mu2e::Mu2eSubEventReceiver::start()
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

bool mu2e::Mu2eSubEventReceiver::getNextDTCFragment(artdaq::FragmentPtrs& frags, DTCLib::DTC_EventWindowTag ts_in)
{
	auto before_read = std::chrono::steady_clock::now();
	int retryCount = 5;
	std::vector<std::unique_ptr<DTCLib::DTC_SubEvent>> data;
	while (data.size() == 0 && retryCount >= 0)
	{
		try
		{
			TLOG(TLVL_TRACE + 25) << "Calling theInterface->GetData(" << ts_in.GetEventWindowTag(true) << ")";
			data = theInterface_->GetSubEventData(ts_in);
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
	if (ts_out.GetEventWindowTag(true) != ts_in.GetEventWindowTag(true))
	{
		TLOG(TLVL_TRACE) << "Requested timestamp " << ts_in.GetEventWindowTag(true) << ", received data with timestamp " << ts_out.GetEventWindowTag(true);
	}

	// GetSubEventData can return multiple EWTs, and we can assume that there is ONE DTC_SubEvent per EWT!
	for (auto& subevt : data)
	{
		size_t size_bytes = sizeof(DTCLib::DTC_EventHeader);
		size_bytes += subevt->GetSubEventByteCount();
		TLOG(TLVL_TRACE + 20) << "Size of event will be " << static_cast<int>(size_bytes) << "B";

		auto evt = std::make_unique<DTCLib::DTC_Event>(size_bytes);
		
		DTCLib::DTC_EventHeader evtHdr;
		evtHdr.inclusive_event_byte_count = size_bytes;
		evtHdr.num_dtcs = 1;
                evtHdr.event_tag_low  = ts_out.GetEventWindowTag(true) & 0xFFFFFFFF;
                evtHdr.event_tag_high = (ts_out.GetEventWindowTag(true) >> 32) & 0xFFFF;
		memcpy(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(evt->GetRawBufferPointer())), &evtHdr, sizeof(DTCLib::DTC_EventHeader));
		auto ptr = reinterpret_cast<const uint8_t*>(evt->GetRawBufferPointer()) + sizeof(DTCLib::DTC_EventHeader);

		TLOG(TLVL_TRACE + 20) << "Calling memcpy(" << (const void*)ptr << ", " << (void*)subevt->GetRawBufferPointer() << ", " << subevt->GetSubEventByteCount() << ")";
		memcpy(const_cast<uint8_t*>(ptr), subevt->GetRawBufferPointer(), subevt->GetSubEventByteCount());
		ptr += subevt->GetSubEventByteCount();
		TLOG(TLVL_TRACE + 20) << "Calling SetupEvent";
		evt->SetupEvent();
		TLOG(TLVL_TRACE + 20) << "Setting EventWindowTag to " << ts_out.GetEventWindowTag(true);
		evt->SetEventWindowTag(ts_out);

		for (size_t se = 0; se < evt->GetSubEventCount(); ++se)
		  {
		    auto subevt = evt->GetSubEvent(se);
		    auto subevtheader =  subevt->GetHeader();
		    metricMan->sendMetric("ROC link0 status", subevtheader->link0_status, "status", 3, artdaq::MetricMode::Maximum);
		    metricMan->sendMetric("ROC link1 status", subevtheader->link1_status, "status", 3, artdaq::MetricMode::Maximum);
		    metricMan->sendMetric("ROC link2 status", subevtheader->link2_status, "status", 3, artdaq::MetricMode::Maximum);
		    metricMan->sendMetric("ROC link3 status", subevtheader->link3_status, "status", 3, artdaq::MetricMode::Maximum);
		    metricMan->sendMetric("ROC link4 status", subevtheader->link4_status, "status", 3, artdaq::MetricMode::Maximum);
		    metricMan->sendMetric("ROC link5 status", subevtheader->link5_status, "status", 3, artdaq::MetricMode::Maximum);

		    for (size_t bl = 0; bl < subevt->GetDataBlockCount(); ++bl)
		      {
			auto block = subevt->GetDataBlock(bl);
			auto first = block->GetHeader();
			std::string  nn  = "Packages per ROC " + std::to_string(bl);
			metricMan->sendMetric(nn,  first->GetPacketCount(), "Packages", 3, artdaq::MetricMode::Average);
			std::string  rocLink = "ROC "+std::to_string(first->GetLinkID())+" status";
			metricMan->sendMetric(rocLink, first->GetStatus(), "status", 3, artdaq::MetricMode::Maximum);			
		      }
		  }
		
		if (print_packets_)
		{
		        TLOG(TLVL_INFO) << "[print_packets starts] subEventCounts: "<<  evt->GetSubEventCount();
			for (size_t se = 0; se < evt->GetSubEventCount(); ++se)
			{
				auto subevt = evt->GetSubEvent(se);
				auto subevtheader =  subevt->GetHeader();
				TLOG(TLVL_INFO) << subevtheader->toJson();
				for (size_t bl = 0; bl < subevt->GetDataBlockCount(); ++bl)
				{
					auto block = subevt->GetDataBlock(bl);
					auto first = block->GetHeader();
					TLOG(TLVL_INFO) << first->toJSON();
					for (int ii = 0; ii < first->GetPacketCount(); ++ii)
					{
					  TLOG(TLVL_INFO) << DTCLib::DTC_DataPacket(((uint8_t*)block->blockPointer) + ((ii + 1) * 16)).toJSON();
					}
				}
			}
		}
		if (rawOutput_)
		{
			evt->WriteEvent(rawOutputStream_, false);
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

		TLOG(TLVL_TRACE + 20) << "Creating Fragment, sz=" << evt->GetEventByteCount() << ", seqid=" << getCurrentSequenceID();
		frags.emplace_back(new artdaq::Fragment(getCurrentSequenceID(), fragment_ids_[0], FragmentType::DTCEVT, fragment_timestamp));
		frags.back()->resizeBytes(evt->GetEventByteCount());
		memcpy(frags.back()->dataBegin(), evt->GetRawBufferPointer(), evt->GetEventByteCount());
		metricMan->sendMetric("Average Event Size",  evt->GetEventByteCount(), "Bytes", 3, artdaq::MetricMode::Average);
		TLOG(TLVL_TRACE + 20) << "Incrementing event counter";
		ev_counter_inc();
	}
	auto after_copy = std::chrono::steady_clock::now();
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

size_t mu2e::Mu2eSubEventReceiver::getCurrentSequenceID()
{
	return ((ev_counter() - 1) * n_dtcs_) + dtc_offset_ + 1;
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::Mu2eSubEventReceiver)
