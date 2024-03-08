#ifndef mu2e_artdaq_Generators_STMReceiver_hh
#define mu2e_artdaq_Generators_STMReceiver_hh

// STMReceiver is a simple type of fragment generator intended to be
// studied by new users of artdaq as an example of how to create such
// a generator in the "best practices" manner. Derived from artdaq's
// CommandableFragmentGenerator class, it can be used in a full DAQ
// simulation, generating all ADC counts with equal probability via
// the std::uniform_int_distribution class

// STMReceiver is designed to simulate values coming in from one of
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
#include "artdaq-core-mu2e/Overlays/STMFragment.hh"
#include "artdaq-core-mu2e/Overlays/FragmentType.hh"

#include <atomic>
#include <vector>

#include "dtcInterfaceLib/DTC.h"
#include "dtcInterfaceLib/DTCSoftwareCFO.h"

namespace mu2e {
class STMReceiver : public artdaq::CommandableFragmentGenerator
{
public:
	explicit STMReceiver(fhicl::ParameterSet const& ps);
	virtual ~STMReceiver();

	FragmentType GetFragmentType() { return fragment_type_; }

private:
	// The "getNext_" function is used to implement user-specific
	// functionality; it's a mandatory override of the pure virtual
	// getNext_ function declared in CommandableFragmentGenerator

	bool getNext_(artdaq::FragmentPtrs& output) override;

	void start() override {}

	void stopNoMutex() override {}

	void stop() override {}
		
	// STM-specific stuff
	bool fromInputFile_{false};
	std::ifstream inputFileStream_;
	bool toOutputFile_{false};
	std::ofstream outputFileStream_;

	// FHiCL-configurable variables. Note that the C++ variable names
	// are the FHiCL variable names with a "_" appended

	FragmentType fragment_type_{FragmentType::STM};  // Type of fragment (see FragmentType.hh)

	
};
}  // namespace mu2e

#endif /* mu2e_artdaq_Generators_STMReceiver_hh */
