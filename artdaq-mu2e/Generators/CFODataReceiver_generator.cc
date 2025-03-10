// This file reads out DTCs that are NOT in HW Event-building mode
// It can be used as an exmaple for developing more specific functionality.

#include "artdaq-core-mu2e/Overlays/FragmentType.hh"
#include "dtcInterfaceLib/DTC.h"
#include "cfoInterfaceLib/CFO.h"
#include "dtcInterfaceLib/DTCSoftwareCFO.h"

#include "artdaq-core/Data/ContainerFragmentLoader.hh"
#include "artdaq-core/Data/MetadataFragment.hh"
#include "artdaq/DAQdata/Globals.hh"
#include "artdaq/Generators/GeneratorMacros.hh"

#include <fstream>

#include "trace.h"
#define TRACE_NAME "CFODataReceiver"

namespace mu2e {
class CFODataReceiver : public artdaq::CommandableFragmentGenerator
{
public:
	explicit CFODataReceiver(fhicl::ParameterSet const& ps);
	virtual ~CFODataReceiver();

	DTCLib::DTC_SimMode GetMode() { return mode_; }

private:
	bool getNextDTCFragment(artdaq::FragmentPtrs& output, DTCLib::DTC_EventWindowTag ts);

	void start() override;

	void stopNoMutex() override {}

	void stop() override;

	size_t getCurrentSequenceID();

	// Like "getNext_", "fragmentIDs_" is a mandatory override; it
	// returns a vector of the fragment IDs an instance of this class
	// is responsible for (in the case of CFODataReceiver, this is just
	// the fragment_id_ variable declared in the parent
	// CommandableFragmentGenerator class)

	std::vector<artdaq::Fragment::fragment_id_t> fragmentIDs_() { return fragment_ids_; }

	std::vector<artdaq::Fragment::fragment_id_t> fragment_ids_;

	// State
	DTCLib::DTC_SimMode mode_;  //!=0 is simulation mode
	const bool skip_cfo_init_;
	bool print_packets_;

	size_t first_timestamp_seen_{size_t(-1)}, last_fragment_timestamp_{size_t(-1)};

	std::unique_ptr<CFOLib::CFO> theCFO_;
	//	  std::unique_ptr<DTCLib::DTCSoftwareCFO> theCFO_;

	std::size_t const throttle_usecs_;
	std::size_t const rollover_subrun_interval_;
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

mu2e::CFODataReceiver::~CFODataReceiver()
{
}

bool mu2e::CFODataReceiver::getNext_(artdaq::FragmentPtrs& frags)
{
	TLOG(TLVL_TRACE + 30) << "getNext_";
	while (!should_stop())
	{
		TLOG(TLVL_TRACE + 31) << "Sleeping...";
		usleep(5000);
	}

	if (throttle_usecs_ > 0)
	{
		TLOG(TLVL_TRACE + 32) << "Throttling... " << throttle_usecs_;
		std::unique_lock<std::mutex> throttle_lock(throttle_mutex_);
		throttle_cv_.wait_for(throttle_lock, std::chrono::microseconds(throttle_usecs_), [&]() { return should_stop(); });
	}

	if (should_stop())
	{
		TLOG(TLVL_TRACE + 33) << "Stopping.";
		return false;
	}

	uint64_t z = 0;
	DTCLib::DTC_EventWindowTag zero(z);

	//--------------------------------------------------------------------------------
	// temporary sub-run transition
	//--------------------------------------------------------------------------------
	if (rollover_subrun_interval_ > 0 && ev_counter() % rollover_subrun_interval_ == 0 && fragment_id() == 0)
	{
		auto endOfSubrunFrag = artdaq::MetadataFragment::CreateEndOfSubrunFragment(my_rank, ev_counter() + 1, 1 + (ev_counter() / rollover_subrun_interval_), 0);
		frags.emplace_back(std::move(endOfSubrunFrag));
	}

	TLOG(TLVL_TRACE + 34) << "getNext_ req";
	auto start_time = std::chrono::steady_clock::now();
	bool retVal = true;
	do
	{
		retVal = getNextDTCFragment(frags, zero);
		TLOG(TLVL_TRACE + 35) << "getNext_ req retry? " << retVal << " " << frags.size();
	} while (1 && retVal && frags.size() < 900 &&
			 artdaq::TimeUtils::GetElapsedTimeMicroseconds(start_time) < 100000 /* 100 ms */);
	TLOG(TLVL_TRACE + 36) << "getNext_ req done" << retVal << " " << frags.size();

	return retVal;
}  // end getNext_()

DTCLib::DTC_EventWindowTag mu2e::CFODataReceiver::getCurrentEventWindowTag()
{
	if (first_timestamp_seen_ != size_t(-1))
	{
		return DTCLib::DTC_EventWindowTag(getCurrentSequenceID() + first_timestamp_seen_);
	}

	return DTCLib::DTC_EventWindowTag(uint64_t(0));
}

mu2e::CFODataReceiver::CFODataReceiver(fhicl::ParameterSet const& ps)
	: CommandableFragmentGenerator(ps)
	, fragment_ids_{static_cast<artdaq::Fragment::fragment_id_t>(fragment_id())}
	, mode_(DTCLib::DTC_SimModeConverter::ConvertToSimMode(ps.get<std::string>("sim_mode", "Disabled")))
	, skip_cfo_init_(ps.get<bool>("skip_cfo_init", false))
	, print_packets_(ps.get<bool>("debug_print", false))
	, throttle_usecs_(ps.get<size_t>("throttle_usecs", 0))  // in units of us
	, rollover_subrun_interval_(ps.get<size_t>("rollover_subrun_interval", 20000))
	, diagLevel_(ps.get<int>("diagLevel", 0))
{
	// mode_ can still be overridden by environment!
	theCFO_ = std::make_unique<CFOLib::CFO>(mode_,
											ps.get<int>("cfo", -1),
											ps.get<std::string>("expectedDesignVersion", ""),
											skip_cfo_init_,
											ps.get<std::string>("uid", ""));

	mode_ = theCFO_->GetSimMode();
	TLOG(TLVL_DEBUG) << "CFODataReceiver Initialized with mode " << mode_;

	if (skip_cfo_init_) return;  // skip any control of DTC

	theCFO_->ReleaseAllBuffers(DTC_DMA_Engine_DAQ);
}

void mu2e::CFODataReceiver::stop()
{
	// if (skip_cfo_init_) return;  // skip any control of DTC
}

void mu2e::CFODataReceiver::start()
{
	theCFO_->ReleaseAllBuffers(DTC_DMA_Engine_DAQ);
}

bool mu2e::CFODataReceiver::getNextDTCFragment(artdaq::FragmentPtrs& frags, DTCLib::DTC_EventWindowTag ts_in)
{
	auto before_read = std::chrono::steady_clock::now();
	int retryCount = 5;
	std::vector<std::unique_ptr<CFOLib::CFO_Event>> data;
	while (data.size() == 0 && retryCount >= 0)
	{
		try
		{
			theCFO_->GetData(data, ts_in /* not used when not matching */);  // do we need to set matchEventWindowTag = true??
			TLOG(TLVL_TRACE + 25) << "Done calling theCFO->GetData() data.size()=" << data.size() << ", retryCount=" << retryCount;
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
	TLOG(TLVL_TRACE) << "Received data with timestamp " << ts_out.GetEventWindowTag(true);

	// GetSubEventData can return multiple EWTs, and we can assume that there is ONE DTC_SubEvent per EWT!
	for (auto& cfoevt : data)
	{
		TLOG(TLVL_TRACE + 20) << "Initializing a CFO_Event ";
		auto evt = std::make_unique<CFOLib::CFO_Event>(&cfoevt);
		// TLOG(TLVL_TRACE + 23) << "Setting Eventmode to " << (uint64_t) evt->GetEventMode();

		auto ptr = reinterpret_cast<const uint8_t*>(evt->GetRawBufferPointer());

		TLOG(TLVL_TRACE + 21) << "Calling memcpy(" << (const void*)ptr << ", " << (void*)cfoevt->GetRawBufferPointer() << ", " << cfoevt->GetEventByteCount() << ")";
		memcpy(const_cast<uint8_t*>(ptr), cfoevt->GetRawBufferPointer(), cfoevt->GetEventByteCount());
		ptr += cfoevt->GetEventByteCount();

		TLOG(TLVL_TRACE + 23) << "Setting EventWindowTag to " << ts_out.GetEventWindowTag(true);
		evt->SetEventWindowTag(ts_out);

		// auto after_print = std::chrono::steady_clock::now();

		auto fragment_timestamp = ts_out.GetEventWindowTag(true);
		TLOG(TLVL_TRACE + 24) << "fragment_timestamp=" << fragment_timestamp;  // << " while timestamp_to_use=" << timestamp_to_use;

		// frags.emplace_back(new artdaq::Fragment(getCurrentSequenceID(), fragment_ids_[0], FragmentType::DTCEVT, fragment_timestamp));
		TLOG(TLVL_TRACE + 25) << "Creating Fragment, sz=" << evt->GetEventByteCount() << ", seqid=" << getCurrentSequenceID();
		frags.emplace_back(new artdaq::Fragment(fragment_timestamp, fragment_ids_[0], FragmentType::DTCEVT, fragment_timestamp));
		frags.back()->resizeBytes(evt->GetEventByteCount());
		memcpy(frags.back()->dataBegin(), evt->GetRawBufferPointer(), evt->GetEventByteCount());
		metricMan->sendMetric("Average Event Size", evt->GetEventByteCount(), "Bytes", 3, artdaq::MetricMode::Average);
		TLOG(TLVL_TRACE + 26) << "Incrementing event counter";
		ev_counter_inc();
	}

	auto after_copy = std::chrono::steady_clock::now();
	TLOG(TLVL_TRACE + 27) << "Reporting Metrics";
	auto hwTime = theCFO_->GetDevice()->GetDeviceTime();

	double hw_timestamp_rate = 1 / hwTime;

	metricMan->sendMetric("CFO Read Time", artdaq::TimeUtils::GetElapsedTime(after_read, after_copy), "s", 3, artdaq::MetricMode::Average);
	metricMan->sendMetric("Fragment Prep Time", artdaq::TimeUtils::GetElapsedTime(before_read, after_read), "s", 3, artdaq::MetricMode::Average);
	metricMan->sendMetric("HW Timestamp Rate", hw_timestamp_rate, "timestamps/s", 1, artdaq::MetricMode::Average);

	TLOG(TLVL_TRACE + 28) << "Returning true";

	return true;
}

size_t mu2e::CFODataReceiver::getCurrentSequenceID()
{
	return ev_counter();
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::CFODataReceiver)
