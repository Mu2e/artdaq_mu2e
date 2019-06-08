#include "artdaq/TransferPlugins/ShmemTransfer.hh"
#include "artdaq/TransferPlugins/TransferInterface.hh"

namespace mu2e {

class SingleNodeTransfer : public artdaq::TransferInterface
{
public:
	SingleNodeTransfer(const fhicl::ParameterSet&, Role);
	~SingleNodeTransfer() = default;

	virtual int receiveFragment(artdaq::Fragment& fragment, size_t receiveTimeout)
	{
		if (!theTransfer_) return -1;
		return theTransfer_->receiveFragment(fragment, receiveTimeout);
	}

	virtual artdaq::TransferInterface::CopyStatus transfer_fragment_min_blocking_mode(artdaq::Fragment const& fragment, size_t send_timeout_usec)
	{
		if (!theTransfer_) return artdaq::TransferInterface::CopyStatus::kSuccess;
		return theTransfer_->transfer_fragment_min_blocking_mode(fragment, send_timeout_usec);
	}

	virtual artdaq::TransferInterface::CopyStatus transfer_fragment_reliable_mode(artdaq::Fragment&& fragment)
	{
		if (!theTransfer_) return artdaq::TransferInterface::CopyStatus::kSuccess;
		return theTransfer_->transfer_fragment_reliable_mode(std::move(fragment));
	}

	virtual int receiveFragmentHeader(artdaq::detail::RawFragmentHeader& hdr, size_t tmo)
	{
		if (!theTransfer_) return -1;
		return theTransfer_->receiveFragmentHeader(hdr, tmo);
	}

	virtual int receiveFragmentData(artdaq::RawDataType* loc, size_t sz)
	{
		if (!theTransfer_) return -1;
		return theTransfer_->receiveFragmentData(loc, sz);
	}

  virtual void flush_buffers() {
    return theTransfer_->flush_buffers();
  }

private:
	std::unique_ptr<artdaq::TransferInterface> theTransfer_;
};

}  // namespace mu2e

mu2e::SingleNodeTransfer::SingleNodeTransfer(const fhicl::ParameterSet& pset, artdaq::TransferInterface::Role role)
	: artdaq::TransferInterface(pset, role)
{
	TLOG_DEBUG(uniqueLabel()) << "Begin SingleNodeTransfer constructor" << TLOG_ENDL;
	std::string srcHost, destHost;
	auto hosts = pset.get<std::vector<fhicl::ParameterSet>>("host_map");
	for (auto& ps : hosts) {
		auto rank = ps.get<int>("rank", -1);
		if (rank == source_rank()) {
			srcHost = ps.get<std::string>("host", "localhost");
		}
		if (rank == destination_rank()) {
			destHost = ps.get<std::string>("host", "localhost");
		}
	}
	TLOG_DEBUG(uniqueLabel()) << "SNT: srcHost=" << srcHost << ", destHost=" << destHost << TLOG_ENDL;
	if (srcHost == destHost) {
		TLOG_DEBUG(uniqueLabel()) << "SNT: Constructing ShmemTransfer" << TLOG_ENDL;
		theTransfer_.reset(new artdaq::ShmemTransfer(pset, role));
	}
	else
	{
		TLOG_DEBUG(uniqueLabel()) << "SNT: Aborting!" << TLOG_ENDL;
		theTransfer_.reset(nullptr);
		throw new cet::exception("Not the same host");
	}
}

DEFINE_ARTDAQ_TRANSFER(mu2e::SingleNodeTransfer)
