#include "artdaq/TransferPlugins/TransferInterface.hh"
#include "artdaq/TransferPlugins/ShmemTransfer.hh"

namespace mu2e {

  class SingleNodeTransfer : public artdaq::TransferInterface {
  public:
    SingleNodeTransfer(const fhicl::ParameterSet&, Role);
    ~SingleNodeTransfer() = default;

    virtual int receiveFragment(artdaq::Fragment& fragment,
				size_t receiveTimeout) 
    { if(!theTransfer_) return -1;
    return theTransfer_->receiveFragment(fragment, receiveTimeout); }

    virtual artdaq::TransferInterface::CopyStatus copyFragment(artdaq::Fragment& fragment,
				    size_t send_timeout_usec = std::numeric_limits<size_t>::max())
    { if(!theTransfer_) return artdaq::TransferInterface::CopyStatus::kSuccess;
	  return theTransfer_->copyFragment(fragment, send_timeout_usec); }
    virtual artdaq::TransferInterface::CopyStatus moveFragment(artdaq::Fragment&& fragment,
				    size_t send_timeout_usec = std::numeric_limits<size_t>::max())
    { if(!theTransfer_) return artdaq::TransferInterface::CopyStatus::kSuccess;
	  return theTransfer_->moveFragment(std::move(fragment), send_timeout_usec); }
  private:
    std::unique_ptr<artdaq::TransferInterface> theTransfer_;
  };

}

mu2e::SingleNodeTransfer::SingleNodeTransfer(const fhicl::ParameterSet& pset, artdaq::TransferInterface::Role role)
  : artdaq::TransferInterface(pset, role)
{
  mf::LogDebug(uniqueLabel()) << "Begin SingleNodeTransfer constructor";
  std::string srcHost, destHost;
 	auto hosts = pset.get<std::vector<fhicl::ParameterSet>>("host_map");
	for (auto& ps : hosts) {
		auto rank = ps.get<int>("rank", -1);
		if(rank == source_rank()) {
		  srcHost = ps.get<std::string>("host", "localhost");
		}
		if(rank == destination_rank()) {
		  destHost = ps.get<std::string>("host", "localhost");
		}
	}
	mf::LogDebug(uniqueLabel()) << "ADT: srcHost=" << srcHost << ", destHost=" << destHost;
	if(srcHost == destHost) {
	  mf::LogDebug(uniqueLabel()) << "ADT: Constructing ShmemTransfer";
	  theTransfer_.reset(new artdaq::ShmemTransfer(pset, role));
	} else {
	  theTransfer_.reset(nullptr);
	}
}

DEFINE_ARTDAQ_TRANSFER(mu2e::SingleNodeTransfer)
