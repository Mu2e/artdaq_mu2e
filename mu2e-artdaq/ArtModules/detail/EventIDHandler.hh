#ifndef mu2e_artdaq_core_Overlays_Offline_detail_EventIDHandler_hh
#define mu2e_artdaq_core_Overlays_Offline_detail_EventIDHandler_hh

#include "artdaq-core/Data/RawEvent.hh"
#include "canvas/Persistency/Provenance/IDNumber.h"

namespace mu2e {
namespace detail {

class EventIDHandler
{
public:
	// N.B. The current behavior is that if the run and subRun IDs
	//      are the same as the cached values, then the event number
	//      is just incremented.  This behavior may need to change
	//      if encapsulating the timestamp into the art::EventID is
	//      desired.
  void update(artdaq::detail::RawEventHeader re, uint64_t timestamp)
	{
		if (run_ != re.run_id) {
			run_ = re.run_id;
			event_ = 0;
		}
		subRun_ = static_cast<uint32_t>(timestamp >> 32) + 1; // Subruns are 1-based!
		event_ = static_cast<uint32_t>(timestamp) + 1; // Events are 1-based
	}

	auto run() const { return run_; }
	auto subRun() const { return subRun_; }
	auto event() const { return event_; }

private:
	artdaq::detail::RawEventHeader::run_id_t run_{};
	artdaq::detail::RawEventHeader::subrun_id_t subRun_{};
	art::EventNumber_t event_{};
};

}  // namespace detail
}  // namespace mu2e

#endif
