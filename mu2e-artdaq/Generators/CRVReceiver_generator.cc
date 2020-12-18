#include <atomic>
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <unistd.h>

#include "trace.h"
#define TRACE_NAME "CRVReceiver"

#include "canvas/Utilities/Exception.h"
#include "cetlib_except/exception.h"
#include "fhiclcpp/ParameterSet.h"

#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"
#include "artdaq/Generators/GeneratorMacros.hh"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/Generators/CommandableFragmentGenerator.hh"

#include "dtcInterfaceLib/DTC.h"
#include "dtcInterfaceLib/DTCSoftwareCFO.h"
#include "dtcInterfaceLib/DTC_Types.h"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "mu2e-artdaq-core/Overlays/DTCFragment.hh"

namespace mu2e {
class CRVReceiver : public artdaq::CommandableFragmentGenerator
{
public:
	explicit CRVReceiver(fhicl::ParameterSet const& ps);
	virtual ~CRVReceiver();

	bool getNextDTCFragment(artdaq::FragmentPtrs& output);

	DTCLib::DTC_SimMode GetMode() { return mode_; }

	FragmentType GetFragmentType() { return fragment_type_; }

private:
	// The "getNext_" function is used to implement user-specific
	// functionality; it's a mandatory override of the pure virtual
	// getNext_ function declared in CommandableFragmentGenerator

	bool getNext_(artdaq::FragmentPtrs& output) override;

	void start() override {}

	void stopNoMutex() override {}

	void stop() override {}

	void readSimFile_(std::string sim_file);

	// Like "getNext_", "fragmentIDs_" is a mandatory override; it
	// returns a vector of the fragment IDs an instance of this class
	// is responsible for (in the case of CRVReceiver, this is just
	// the fragment_id_ variable declared in the parent
	// CommandableFragmentGenerator class)

	std::vector<artdaq::Fragment::fragment_id_t> fragmentIDs_() { return fragment_ids_; }

	// FHiCL-configurable variables. Note that the C++ variable names
	// are the FHiCL variable names with a "_" appended

	FragmentType const fragment_type_;  // Type of fragment (see FragmentType.hh)

	std::vector<artdaq::Fragment::fragment_id_t> fragment_ids_;

	// State
	size_t packets_read_;
	DTCLib::DTC_SimMode mode_;
	uint8_t board_id_;
	bool simFileRead_;
	bool detectorEmulatorMode_{false};

	DTCLib::DTC* theInterface_;
	DTCLib::DTCSoftwareCFO* theCFO_;
	std::shared_ptr<artdaq::RequestBuffer> requests_{nullptr};

	// For Debugging:
	bool print_packets_;

	std::set<artdaq::Fragment::sequence_id_t> seen_sequence_ids_{};
	size_t sequence_id_list_max_size_{1000};
};
}  // namespace mu2e

mu2e::CRVReceiver::CRVReceiver(fhicl::ParameterSet const& ps)
	: CommandableFragmentGenerator(ps), fragment_type_(toFragmentType(ps.get<std::string>("fragment_type", "CRV"))), fragment_ids_{static_cast<artdaq::Fragment::fragment_id_t>(fragment_id())}, packets_read_(0), mode_(DTCLib::DTC_SimModeConverter::ConvertToSimMode(ps.get<std::string>("sim_mode", "Disabled"))), board_id_(static_cast<uint8_t>(ps.get<int>("board_id", 0))), print_packets_(ps.get<bool>("debug_print", false))
{
	// mode_ can still be overridden by environment!
	theInterface_ = new DTCLib::DTC(mode_, -1, 1, "", false, ps.get<std::string>("simulator_memory_file_name", "mu2esim.bin"));
	theCFO_ = new DTCLib::DTCSoftwareCFO(theInterface_, true);
	mode_ = theInterface_->ReadSimMode();

	TLOG(TLVL_DEBUG) << "CRVReceiver Initialized with mode " << mode_;

	char* file_c = getenv("DTCLIB_SIM_FILE");

	auto sim_file = ps.get<std::string>("sim_file", "");
	if (file_c != nullptr)
	{
		sim_file = std::string(file_c);
	}
	if (sim_file.size() > 0)
	{
		simFileRead_ = false;
		detectorEmulatorMode_ = true;
		std::thread reader(&mu2e::CRVReceiver::readSimFile_, this, sim_file);
		reader.detach();
	}
	else
	{
		simFileRead_ = true;
	}
}

void mu2e::CRVReceiver::readSimFile_(std::string sim_file)
{
	TLOG(TLVL_INFO) << "Starting read of simulation file " << sim_file << "."
					<< " Please wait to start the run until finished.";
	theInterface_->WriteSimFileToDTC(sim_file, true);
	simFileRead_ = true;
	TLOG(TLVL_INFO) << "Done reading simulation file into DTC memory.";
}

mu2e::CRVReceiver::~CRVReceiver()
{
	theInterface_->DisableDetectorEmulator();
	delete theInterface_;
	delete theCFO_;
}

bool mu2e::CRVReceiver::getNext_(artdaq::FragmentPtrs& frags) { return getNextDTCFragment(frags); }

bool mu2e::CRVReceiver::getNextDTCFragment(artdaq::FragmentPtrs& frags)
{
	while (!simFileRead_ && !should_stop())
	{
		usleep(5000);
	}

	if (requests_ == nullptr)
	{
		requests_ = GetRequestBuffer();
	}
	if (requests_ == nullptr)
	{
		TLOG(TLVL_ERROR) << "Request Buffer pointer is null! Returning false!";
		return false;
	}

	while (!should_stop() && !requests_->WaitForRequests(100))
	{
	}
	auto reqs = requests_->GetAndClearRequests();
	requests_->reset();

	if (should_stop())
	{
		return false;
	}

	for (auto& req : reqs)
	{
		if (seen_sequence_ids_.count(req.first))
		{
			continue;
		}
		else
		{
			seen_sequence_ids_.insert(req.first);
			if (seen_sequence_ids_.size() > sequence_id_list_max_size_)
			{
				seen_sequence_ids_.erase(seen_sequence_ids_.begin());
			}
		}

		TLOG(TLVL_DEBUG) << "Requesting CRV data for Event Window Tag " << req.second;
		std::vector<DTCLib::DTC_DataBlock> data;
		DTCLib::DTC_Timestamp ts(detectorEmulatorMode_ ? 0 : req.second);

		theCFO_->SendRequestForTimestamp(ts);

		auto before_read = std::chrono::steady_clock::now();
		int retryCount = 5;
		while (data.size() == 0 && retryCount >= 0)
		{
			try
			{
				TLOG(TLVL_TRACE + 25) << "Calling theInterface->GetData(ts)";
				data = theInterface_->GetData(ts);
				TLOG(TLVL_TRACE + 25) << "Done calling theInterface->GetData(ts)";
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

		auto first = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[0].blockPointer));
		DTCLib::DTC_Timestamp out_ts = first.GetTimestamp();
		if (out_ts.GetTimestamp(true) != req.second && !detectorEmulatorMode_)
		{
			TLOG(TLVL_TRACE) << "Requested timestamp " << req.second << ", received data with timestamp " << out_ts.GetTimestamp(true);
		}

		int packetCount = first.GetPacketCount() + 1;
		if (print_packets_)
		{
			std::cout << first.toJSON() << std::endl;
			for (int ii = 0; ii < first.GetPacketCount(); ++ii)
			{
				std::cout << "\t" << DTCLib::DTC_DataPacket(((uint8_t*)data[0].blockPointer) + ((ii + 1) * 16)).toJSON()
						  << std::endl;
			}
		}

		for (size_t i = 1; i < data.size(); ++i)
		{
			auto packet = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[i].blockPointer));
			packetCount += packet.GetPacketCount() + 1;
			if (print_packets_)
			{
				std::cout << packet.toJSON() << std::endl;
				for (int ii = 0; ii < packet.GetPacketCount(); ++ii)
				{
					std::cout << "\t" << DTCLib::DTC_DataPacket(((uint8_t*)data[i].blockPointer) + ((ii + 1) * 16)).toJSON()
							  << std::endl;
				}
			}
		}

		//auto after_print = std::chrono::steady_clock::now();
		frags.emplace_back(new artdaq::Fragment(packetCount * sizeof(packet_t) / sizeof(artdaq::RawDataType), req.first, fragment_ids_[0], fragment_type_, req.second));

		TLOG(TLVL_TRACE + 10) << "Copying DTC packets into DTCFragment";
		size_t packetsProcessed = 0;
		packet_t* dataBegin = reinterpret_cast<packet_t*>(frags.back()->dataBegin());
		for (size_t i = 0; i < data.size(); ++i)
		{
			auto packet = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[i].blockPointer));
			memcpy((void*)(dataBegin + packetsProcessed), data[i].blockPointer,
				   (1 + packet.GetPacketCount()) * sizeof(packet_t));
			packetsProcessed += 1 + packet.GetPacketCount();
		}

		auto after_copy = std::chrono::steady_clock::now();

		TLOG(TLVL_DEBUG) << "Incrementing event counter";
		ev_counter_inc();

		TLOG(TLVL_DEBUG) << "Reporting Metrics";
		auto hwTime = theInterface_->GetDevice()->GetDeviceTime();

		double hw_timestamp_rate = 1 / hwTime;
		double hw_data_rate = frags.back()->sizeBytes() / hwTime;

		metricMan->sendMetric("DTC Read Time", artdaq::TimeUtils::GetElapsedTime(after_read, after_copy), "s", 3, artdaq::MetricMode::Average);
		metricMan->sendMetric("Fragment Prep Time", artdaq::TimeUtils::GetElapsedTime(before_read, after_read), "s", 3, artdaq::MetricMode::Average);
		metricMan->sendMetric("HW Timestamp Rate", hw_timestamp_rate, "timestamps/s", 1, artdaq::MetricMode::Average);
		metricMan->sendMetric("PCIe Transfer Rate", hw_data_rate, "B/s", 1, artdaq::MetricMode::Average);
	}
	TLOG(TLVL_DEBUG) << "Returning true";

	return true;
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::CRVReceiver)
