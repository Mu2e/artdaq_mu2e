#include "artdaq-core-mu2e/Overlays/FragmentType.hh"
#include "artdaq-core-mu2e/Overlays/CFO_Packets/CFO_Event.h"

#include "artdaq-core/Data/MetadataFragment.hh"
#include "artdaq/DAQdata/Globals.hh"
#include "artdaq/Generators/GeneratorMacros.hh"

#include "zmq.hpp"
#include <fstream>

#include "trace.h"
#define TRACE_NAME "CFOZmqReceiver"

namespace mu2e {
class CFOZmqReceiver : public artdaq::CommandableFragmentGenerator
{
public:
	explicit CFOZmqReceiver(fhicl::ParameterSet const& ps);
	virtual ~CFOZmqReceiver();

private:
	void start() override { start_receiver_thread_(); }

	void stopNoMutex() override {}

	void stop() override { stop_receiver_thread_(); }

	// The "getNext_" function is used to implement user-specific
	// functionality; it's a mandatory override of the pure virtual
	// getNext_ function declared in CommandableFragmentGenerator

	bool getNext_(artdaq::FragmentPtrs& output) override;

	void start_receiver_thread_();
	void stop_receiver_thread_();
	void receiveCFOData_();
	void connect_();

	std::mutex frag_mutex_;
	artdaq::FragmentPtrs frags_;
	zmq::context_t context_;
	zmq::socket_t socket_;
	std::string zmq_address_;
	uint64_t event_mode_bitmask_;
	std::atomic<bool> receive_thread_running_{false};
	std::unique_ptr<boost::thread> receiver_thread_;
	uint8_t daq_mode_word_value_{0};
	uint64_t predictive_subrun_offset_{0};
	uint32_t current_subrun_number_{1};
};
}  // namespace mu2e

mu2e::CFOZmqReceiver::CFOZmqReceiver(fhicl::ParameterSet const& ps)
	: CommandableFragmentGenerator(ps)
	, context_()
	, socket_(context_, zmq::socket_type::sub)
	, zmq_address_(ps.get<std::string>("zmqAddress", "inproc://default"))
	, event_mode_bitmask_(ps.get<uint64_t>("eventModeBitmask", 0xFFFFFFFFFFFFFFFF))
	, predictive_subrun_offset_(ps.get<uint64_t>("predictiveSubrunOffset", 0))
{
	socket_.set(zmq::sockopt::rcvtimeo, 1000);  // 1 second timeout
}

mu2e::CFOZmqReceiver::~CFOZmqReceiver()
{
	socket_.close();
	context_.close();
}

bool mu2e::CFOZmqReceiver::getNext_(artdaq::FragmentPtrs& output)
{
	if (should_stop())
	{
		TLOG(TLVL_DEBUG + 33) << "Stopping.";
		return false;
	}

	std::lock_guard<std::mutex> lock(frag_mutex_);
	output.splice(output.end(), frags_);
	return true;
}

void mu2e::CFOZmqReceiver::start_receiver_thread_()
{
	stop_receiver_thread_();
	TLOG(TLVL_INFO) << "Starting Data Receiver Thread";
	receive_thread_running_ = true;
	try
	{
		receiver_thread_.reset(new boost::thread(&mu2e::CFOZmqReceiver::receiveCFOData_, this));
		char tname[16];
		snprintf(tname, 16, "%s", "ZMQRecv");  // NOLINT
		auto handle = receiver_thread_->native_handle();
		pthread_setname_np(handle, tname);
	}
	catch (const boost::exception& e)
	{
		TLOG(TLVL_ERROR) << "Caught boost::exception starting Data Receiver thread: " << boost::diagnostic_information(e) << ", errno=" << errno;
	}
}

void mu2e::CFOZmqReceiver::stop_receiver_thread_()
{
	TLOG(TLVL_INFO) << "Stopping Data Receiver Thread";
	receive_thread_running_ = false;

	if (receiver_thread_ != nullptr && receiver_thread_->joinable())
	{
		receiver_thread_->join();
	}
}

void mu2e::CFOZmqReceiver::connect_()
{
	TLOG(TLVL_INFO) << "Connecting to ZeroMQ address: " << zmq_address_;
	try
	{
		socket_.disconnect(zmq_address_);  // Ensure any previous connection is closed
	}
	catch (const zmq::error_t&)
	{
		// Ignore disconenct errors, as the socket may not be connected yet
	}

	socket_.connect(zmq_address_);
	socket_.set(zmq::sockopt::subscribe, "");  // Subscribe to all messages
	TLOG(TLVL_INFO) << "Connected and subscribed to ZeroMQ address: " << zmq_address_;
}

void mu2e::CFOZmqReceiver::receiveCFOData_()
{
	connect_();
	while (receive_thread_running_)
	{
		try
		{
			zmq::message_t topic_msg;
			zmq::message_t data_msg;
			auto topic_ret = socket_.recv(topic_msg, zmq::recv_flags::none);
			if (!topic_ret.has_value())
			{
				// No data in socket buffer
				continue;
			}
			auto data_ret = socket_.recv(data_msg, zmq::recv_flags::none);
			if (!data_ret.has_value())
			{
				continue;
			}

			auto cfoEvent = CFOLib::CFO_Event(data_msg.data());
			uint64_t eventMode = cfoEvent.GetEventRecord().event_mode;
			uint8_t daqModeWord = static_cast<uint8_t>(eventMode & 0xFF'0000'0000 >> 32);
			auto timestamp = cfoEvent.GetEventWindowTag().GetEventWindowTag(true);
			if (daqModeWord != daq_mode_word_value_)
			{
				TLOG(TLVL_DEBUG + 25) << "Received CFO event " << timestamp << " with DAQ mode word " << std::hex << daqModeWord << " != " << daq_mode_word_value_ << ", checking for subrun transition";
				std::bitset<8> daqModeBits(daqModeWord);
				daq_mode_word_value_ = daqModeWord;
				if (daqModeBits.test(2))
				{
					current_subrun_number_++;
					// Sequence in EndOfSubrunFragment is the _last_ timestamp of the subrun, thus subtracting one to make the next subrun start at timestamp + predictive_subrun_offset_
					TLOG(TLVL_DEBUG + 25) << "Received CFO event " << timestamp << " has predictive bit set, rolling over to subrun " << current_subrun_number_ << " at timestamp " << (timestamp + predictive_subrun_offset_);
					auto endOfSubrunFrag = artdaq::MetadataFragment::CreateEndOfSubrunFragment(my_rank, timestamp + predictive_subrun_offset_ - 1, current_subrun_number_, 0);

					std::lock_guard<std::mutex> lock(frag_mutex_);
					frags_.push_back(std::move(endOfSubrunFrag));
				}
			}
			if ((eventMode & event_mode_bitmask_) == 0)
			{
				TLOG(TLVL_DEBUG + 25) << "Received CFO event " << timestamp << " with mode " << std::hex << eventMode << std::dec << ", skipping due to bitmask";
				continue;
			}

			TLOG(TLVL_DEBUG + 24) << "Received CFO event " << timestamp << " with mode " << std::hex << eventMode << std::dec << ", processing";

			// Process the received data and create an artdaq Fragment
			auto frag = std::make_unique<artdaq::Fragment>(static_cast<artdaq::Fragment::sequence_id_t>(timestamp),
														   fragment_id(),
														   FragmentType::CFO,
														   static_cast<artdaq::Fragment::timestamp_t>(timestamp));
			frag->resizeBytes(sizeof(CFOLib::CFO_EventRecord));
			std::memcpy(frag->dataBegin(), cfoEvent.GetRawBufferPointer(), sizeof(CFOLib::CFO_EventRecord));
			{
				std::lock_guard<std::mutex> lock(frag_mutex_);
				frags_.push_back(std::move(frag));
			}
		}
		catch (const zmq::error_t& e)
		{
			TLOG(TLVL_ERROR) << "ZeroMQ error in receive thread: " << e.what();
			connect_();
		}
		catch (const std::exception& e)
		{
			TLOG(TLVL_ERROR) << "Exception in receive thread: " << e.what();
			connect_();
		}
	}
	socket_.disconnect(zmq_address_);
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::CFOZmqReceiver)
