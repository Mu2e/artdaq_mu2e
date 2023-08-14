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

	if (should_stop())
	{
		return false;
	}

	uint64_t z = 0;
	DTCLib::DTC_EventWindowTag zero(z);

	if (mode_ != 0)
	{
		TLOG(TLVL_INFO) << "Sending request for timestamp " << getCurrentEventWindowTag().GetEventWindowTag(true);
		theCFO_->SendRequestForTimestamp(getCurrentEventWindowTag(), heartbeats_after_);
	}

	return getNextDTCFragment(frags, zero);
}

DTCLib::DTC_EventWindowTag mu2e::Mu2eEventReceiver::getCurrentEventWindowTag() {
	if(first_timestamp_seen_ > 0) {
	return DTCLib::DTC_EventWindowTag(getCurrentSequenceID() + first_timestamp_seen_);
	}

	return DTCLib::DTC_EventWindowTag(uint64_t(0));
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::Mu2eEventReceiver)
