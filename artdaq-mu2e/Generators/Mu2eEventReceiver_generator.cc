// This file implements the basic data readout functionality on top of Mu2eEventReceiverBase. 
// It can be used as an exmaple for developing more specific functionality.

#include "artdaq-mu2e/Generators/Mu2eEventReceiverBase.hh"

#include "artdaq/Generators/GeneratorMacros.hh"

#include "trace.h"
#define TRACE_NAME "Mu2eEventReceiver"

namespace mu2e {
class Mu2eEventReceiver : public mu2e::Mu2eEventReceiverBase
{
public:
	explicit Mu2eEventReceiver(fhicl::ParameterSet const& ps);
	virtual ~Mu2eEventReceiver();

private:
	// The "getNext_" function is used to implement user-specific
	// functionality; it's a mandatory override of the pure virtual
	// getNext_ function declared in CommandableFragmentGenerator

	bool getNext_(artdaq::FragmentPtrs& output) override;
	DTCLib::DTC_EventWindowTag getCurrentEventWindowTag();
};
}  // namespace mu2e

mu2e::Mu2eEventReceiver::Mu2eEventReceiver(fhicl::ParameterSet const& ps)
  : Mu2eEventReceiverBase(ps)
{
	TLOG(TLVL_DEBUG) << "Mu2eEventReceiver Initialized with mode " << mode_;
}

mu2e::Mu2eEventReceiver::~Mu2eEventReceiver()
{
}

bool mu2e::Mu2eEventReceiver::getNext_(artdaq::FragmentPtrs& frags)
{
	while (!simFileRead_ && !should_stop())
	{
		usleep(5000);
	}

	std::unique_lock<std::mutex> throttle_lock(throttle_mutex_);
	auto throttle_usecs = 1000000 / request_rate_;
	TLOG(TLVL_INFO) << "[mu2e::Mu2eEventReceiver::getNext_] request_rate= " << request_rate_
			<< " wait_time= " << throttle_usecs;
	throttle_cv_.wait_for(throttle_lock, std::chrono::microseconds(static_cast<int>( throttle_usecs)), [&]() { return should_stop(); });

	// if (frag_sent_ == 0) 
        // {
	//         sending_start_ = std::chrono::steady_clock::now();
	// }

	// auto target = sending_start_ + std::chrono::microseconds(static_cast<int>((frag_sent_+1) * 1000000 / request_rate_));
	// auto now    = std::chrono::steady_clock::now();
	// if ((now < target))
	// {
	//         std::this_thread::sleep_until(target);
	// }
	

	if (should_stop())
	{
		return false;
	}

	uint64_t z = 0;
	DTCLib::DTC_EventWindowTag zero(z);

	if (mode_ != 0)
	{
	        TLOG_DEBUG(2) << "Sending request for timestamp " << getCurrentEventWindowTag().GetEventWindowTag(true);
		theCFO_->SendRequestForTimestamp(getCurrentEventWindowTag(), heartbeats_after_);
	}

	++frag_sent_;
	return getNextDTCFragment(frags, zero);
}

DTCLib::DTC_EventWindowTag mu2e::Mu2eEventReceiver::getCurrentEventWindowTag()
{
	if(first_timestamp_seen_ > 0) 
	{
		return DTCLib::DTC_EventWindowTag(getCurrentSequenceID() + first_timestamp_seen_);
	}

	return DTCLib::DTC_EventWindowTag(uint64_t(0));
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::Mu2eEventReceiver)
