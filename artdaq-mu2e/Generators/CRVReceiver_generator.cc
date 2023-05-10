#include "artdaq-mu2e/Generators/Mu2eEventReceiverBase.hh"

#include "artdaq/Generators/GeneratorMacros.hh"

#include "trace.h"
#define TRACE_NAME "CRVReceiver"


namespace mu2e {
class CRVReceiver : public mu2e::Mu2eEventReceiverBase
{
public:
	explicit CRVReceiver(fhicl::ParameterSet const& ps);
	virtual ~CRVReceiver();

private:
	// The "getNext_" function is used to implement user-specific
	// functionality; it's a mandatory override of the pure virtual
	// getNext_ function declared in CommandableFragmentGenerator

	bool getNext_(artdaq::FragmentPtrs& output) override;

	std::shared_ptr<artdaq::RequestBuffer> requests_{nullptr};

	std::set<artdaq::Fragment::sequence_id_t> seen_sequence_ids_{};
	size_t sequence_id_list_max_size_{1000};
};
}  // namespace mu2e

mu2e::CRVReceiver::CRVReceiver(fhicl::ParameterSet const& ps)
	: Mu2eEventReceiverBase(ps)
{

	TLOG(TLVL_DEBUG) << "CRVReceiver Initialized with mode " << mode_;
}

mu2e::CRVReceiver::~CRVReceiver()
{
}

bool mu2e::CRVReceiver::getNext_(artdaq::FragmentPtrs& frags) 
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
	//requests_->reset();

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
		auto ret = getNextDTCFragment(frags, DTCLib::DTC_EventWindowTag(req.second));
		if (!ret) return false;
	}

	return true;
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::CRVReceiver)
