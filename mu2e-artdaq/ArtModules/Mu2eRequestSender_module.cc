#include <iomanip>
#include <sstream>

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "canvas/Utilities/InputTag.h"
#include "cetlib_except/exception.h"

#include "artdaq/DAQdata/Globals.hh"
#include "artdaq/DAQrate/RequestSender.hh"

namespace mu2e {
class Mu2eRequestSender : public art::EDAnalyzer
{
public:
	/// <summary>
	/// Configuration of the Mu2eRequestSender. May be used for parameter validation
	/// </summary>
	struct Config
	{
		/// "send_requests" (Default: false): Whether to send DataRequests when new sequence IDs are seen
		fhicl::Atom<bool> send_requests{fhicl::Name{"send_requests"}, fhicl::Comment{"Enable sending Data Request messages"}, false};
		/// "request_port" (Default: 3001): Port to send DataRequests on
		fhicl::Atom<int> request_port{fhicl::Name{"request_port"}, fhicl::Comment{"Port to send DataRequests on"}, 3001};
		/// "request_delay_ms" (Default: 10): How long to wait before sending new DataRequests
		fhicl::Atom<size_t> request_delay_ms{fhicl::Name{"request_delay_ms"}, fhicl::Comment{"How long to wait before sending new DataRequests"}, 10};
		/// "request_shutdown_timeout_us" (Default: 100000 us): How long to wait for pending requests to be sent at shutdown
		fhicl::Atom<size_t> request_shutdown_timeout_us{fhicl::Name{"request_shutdown_timeout_us"}, fhicl::Comment{"How long to wait for pending requests to be sent at shutdown"}, 100000};
		/// "multicast_interface_ip" (Default: "0.0.0.0"): Use this hostname for multicast output (to assign to the proper NIC)
		fhicl::Atom<std::string> output_address{fhicl::Name{"multicast_interface_ip"}, fhicl::Comment{"Use this hostname for multicast output(to assign to the proper NIC)"}, "0.0.0.0"};
		/// "request_address" (Default: "227.128.12.26"): Multicast address to send DataRequests to
		fhicl::Atom<std::string> request_address{fhicl::Name{"request_address"}, fhicl::Comment{"Multicast address to send DataRequests to"}, "227.128.12.26"};
		/// "max_request_count" (Default: 100): Number of requests to store in circular buffer (equivalent to how many times each request will be sent)
		fhicl::Atom<size_t> max_request_count{fhicl::Name{"max_request_count"}, fhicl::Comment{"Number of requests to store in circular buffer (equivalent to how many times each request will be sent)"}, 100};	
	};
	/// Used for ParameterSet validation (if desired)
	using Parameters = fhicl::WrappedTable<Config>;


	explicit Mu2eRequestSender(fhicl::ParameterSet const& p);
	virtual ~Mu2eRequestSender() = default;

	void analyze(art::Event const& e) override;
	void beginRun(art::Run const& r) override;
	void endRun(art::Run const& r) override;

private:
	std::list<artdaq::Fragment::sequence_id_t> sent_requests_{};
	size_t max_list_size_;
	std::unique_ptr<artdaq::RequestSender> request_sender_{nullptr};
};
}  // namespace mu2e

mu2e::Mu2eRequestSender::Mu2eRequestSender(fhicl::ParameterSet const& ps)
	: art::EDAnalyzer(ps), max_list_size_(ps.get<size_t>("max_request_count", 100))
{
	request_sender_.reset(new artdaq::RequestSender(ps));
}

void mu2e::Mu2eRequestSender::analyze(art::Event const& e)
{
	artdaq::Fragment::sequence_id_t seq;
	artdaq::Fragment::timestamp_t ts;

	std::vector<art::Handle<artdaq::detail::RawEventHeader>> rawHeaderHandles;
	e.getManyByType(rawHeaderHandles);

	for (auto const& hdr : rawHeaderHandles) {
		if (hdr.isValid()) {
			seq = hdr->sequence_id;
			ts = hdr->timestamp;
			break;
		}
	}

	request_sender_->AddRequest(seq, ts);
	sent_requests_.push_back(seq);

	while (sent_requests_.size() > max_list_size_) {
		request_sender_->RemoveRequest(sent_requests_.front());
		sent_requests_.pop_front();
	}
}

void mu2e::Mu2eRequestSender::beginRun(art::Run const& r)
{
	request_sender_->SetRequestMode(artdaq::detail::RequestMessageMode::Normal);
	request_sender_->SetRunNumber(r.run());
}

void mu2e::Mu2eRequestSender::endRun(art::Run const& r)
{
	request_sender_->SetRequestMode(artdaq::detail::RequestMessageMode::EndOfRun);
	request_sender_->SendRequest();
}

DEFINE_ART_MODULE(mu2e::Mu2eRequestSender)
