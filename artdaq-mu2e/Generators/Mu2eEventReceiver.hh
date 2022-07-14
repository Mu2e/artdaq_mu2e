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
#include "artdaq-core-mu2e/Overlays/Mu2eEventFragment.hh"
#include "artdaq-core-mu2e/Overlays/FragmentType.hh"

#include <atomic>
#include <vector>

#include "dtcInterfaceLib/DTC.h"
#include "dtcInterfaceLib/DTCSoftwareCFO.h"

namespace mu2e {
class Mu2eEventReceiver : public artdaq::CommandableFragmentGenerator
{
public:
	explicit Mu2eEventReceiver(fhicl::ParameterSet const& ps);
	virtual ~Mu2eEventReceiver();

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
	// is responsible for (in the case of Mu2eEventReceiver, this is just
	// the fragment_id_ variable declared in the parent
	// CommandableFragmentGenerator class)

	std::vector<artdaq::Fragment::fragment_id_t> fragmentIDs_() { return fragment_ids_; }

	// FHiCL-configurable variables. Note that the C++ variable names
	// are the FHiCL variable names with a "_" appended

	FragmentType const fragment_type_;  // Type of fragment (see FragmentType.hh)

	std::vector<artdaq::Fragment::fragment_id_t> fragment_ids_;

	// State
	size_t highest_timestamp_seen_{0};
	size_t timestamp_loops_{0};  // For playback mode, so that we continually generate unique timestamps
	DTCLib::DTC_SimMode mode_;
	uint8_t board_id_;
	bool simFileRead_;

	DTCLib::DTC* theInterface_;
	DTCLib::DTCSoftwareCFO* theCFO_;

	// For Debugging:
	bool print_packets_;

	//    //    std::pair<std::vector<std::string>::const_iterator, uint64_t> next_point_;
	//    //    std::atomic<bool> should_stop_;
	//    //    std::independent_bits_engine<std::minstd_rand, 2, V172xFragment::adc_type> twoBits_;
	//
	//    // Root Input
	//    int64_t gen_from_file_;
	//    TFile * file_;
	//    TTree *eventTree_;
	//    art::Wrapper< std::vector<artdaq::Fragment> >* wrapper_;
};
}  // namespace mu2e

#endif /* mu2e_artdaq_Generators_Mu2eEventReceiver_hh */
