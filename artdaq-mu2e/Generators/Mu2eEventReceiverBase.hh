#ifndef mu2e_artdaq_Generators_Mu2eEventReceiver_hh
#define mu2e_artdaq_Generators_Mu2eEventReceiver_hh

// Mu2eEventReceiver is a simple type of fragment generator intended to be
// studied by new users of artdaq as an example of how to create such
// a generator in the "best practices" manner. Derived from artdaq's
// CommandableFragmentGenerator class, it can be used in a full DAQ
// simulation, generating all ADC counts with equal probability via
// the std::uniform_int_distribution class

// Mu2eEventReceiver is designed to simulate values coming in from one of
// two types of digitizer boards, one called "TOY1" and the other
// called "TOY2"; the only difference between the two boards is the #
// of bits in the ADC values they send. These values are declared as
// FragmentType enum's in mu2e-artdaq's
// mu2e-artdaq/Overlays/FragmentType.hh header.

// Some C++ conventions used:

// -Append a "_" to every private member function and variable

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/Generators/CommandableFragmentGenerator.hh"
#include "fhiclcpp/fwd.h"

#include <atomic>
#include <vector>

#include "dtcInterfaceLib/DTC.h"
#include "dtcInterfaceLib/DTCSoftwareCFO.h"

namespace mu2e {
class Mu2eEventReceiverBase : public artdaq::CommandableFragmentGenerator
{
public:
	explicit Mu2eEventReceiverBase(fhicl::ParameterSet const& ps);
	virtual ~Mu2eEventReceiverBase();

	DTCLib::DTC_SimMode GetMode() { return mode_; }

protected:
  bool getNextDTCFragment(artdaq::FragmentPtrs& output, DTCLib::DTC_EventWindowTag ts, artdaq::Fragment::sequence_id_t seq_in = 0);

	void start() override;

	void stopNoMutex() override {}

	void stop() override;

	void readSimFile_(std::string sim_file);

	size_t getCurrentSequenceID();


	// Like "getNext_", "fragmentIDs_" is a mandatory override; it
	// returns a vector of the fragment IDs an instance of this class
	// is responsible for (in the case of Mu2eEventReceiverBase, this is just
	// the fragment_id_ variable declared in the parent
	// CommandableFragmentGenerator class)

	std::vector<artdaq::Fragment::fragment_id_t> fragmentIDs_() { return fragment_ids_; }

	std::vector<artdaq::Fragment::fragment_id_t> fragment_ids_;

	// State
	size_t highest_timestamp_seen_{0};
	size_t timestamp_loops_{0};  // For playback mode, so that we continually generate unique timestamps
	DTCLib::DTC_SimMode mode_;
	bool simFileRead_;
	const bool skip_dtc_init_;
	bool rawOutput_{false};
	std::string rawOutputFile_{""};
	std::ofstream rawOutputStream_;
	bool print_packets_;
	size_t heartbeats_after_{16};

	size_t dtc_offset_{0};
	size_t n_dtcs_{1};
	size_t first_timestamp_seen_{0};

	std::unique_ptr<DTCLib::DTC> theInterface_;
	std::unique_ptr<DTCLib::DTCSoftwareCFO> theCFO_;

        float                   request_rate_;
        std::condition_variable throttle_cv_;
	std::mutex              throttle_mutex_;
        int                     diagLevel_;
        int                     frag_sent_;
        std::chrono::time_point<std::chrono::steady_clock> sending_start_;

};
}  // namespace mu2e

#endif /* mu2e_artdaq_Generators_Mu2eEventReceiver_hh */
