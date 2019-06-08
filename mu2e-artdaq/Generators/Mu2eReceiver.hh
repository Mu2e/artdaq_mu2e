#ifndef mu2e_artdaq_Generators_Mu2eReceiver_hh
#define mu2e_artdaq_Generators_Mu2eReceiver_hh

// Mu2eReceiver is designed to call the DTCInterfaceLibrary a certain number of times
// (set in the mu2eFragment header) and pack that data into DTCFragments contained
// in a single mu2eFragment object.

// Some C++ conventions used:

// -Append a "_" to every private member function and variable

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/Generators/CommandableFragmentGenerator.hh"
#include "fhiclcpp/fwd.h"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"

#include "dtcInterfaceLib/DTC.h"
#include "dtcInterfaceLib/DTCSoftwareCFO.h"

#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

namespace mu2e {
class Mu2eReceiver : public artdaq::CommandableFragmentGenerator
{
public:
	explicit Mu2eReceiver(fhicl::ParameterSet const& ps);
	virtual ~Mu2eReceiver();

private:
	// The "getNext_" function is used to implement user-specific
	// functionality; it's a mandatory override of the pure virtual
	// getNext_ function declared in CommandableFragmentGenerator

	bool getNext_(artdaq::FragmentPtrs& output) override;

	bool sendEmpty_(artdaq::FragmentPtrs& output);

	void start() override {}

	void stopNoMutex() override {}

	void stop() override;

	void readSimFile_(std::string sim_file);

	// Like "getNext_", "fragmentIDs_" is a mandatory override; it
	// returns a vector of the fragment IDs an instance of this class
	// is responsible for (in the case of Mu2eReceiver, this is just
	// the fragment_id_ variable declared in the parent
	// CommandableFragmentGenerator class)

	std::vector<artdaq::Fragment::fragment_id_t> fragmentIDs_() { return fragment_ids_; }

	// FHiCL-configurable variables. Note that the C++ variable names
	// are the FHiCL variable names with a "_" appended

	FragmentType const fragment_type_;  // Type of fragment (see FragmentType.hh)

	std::vector<artdaq::Fragment::fragment_id_t> fragment_ids_;

	// State
	size_t timestamps_read_;
	std::chrono::steady_clock::time_point lastReportTime_;
	std::chrono::steady_clock::time_point procStartTime_;
	DTCLib::DTC_SimMode mode_;
	uint8_t board_id_;
	bool simFileRead_;
	bool rawOutput_;
	std::string rawOutputFile_;
	std::ofstream rawOutputStream_;
	size_t nSkip_;
	bool sendEmpties_;
	bool verbose_;

	DTCLib::DTC* theInterface_;
	DTCLib::DTCSoftwareCFO* theCFO_;

	double _timeSinceLastSend()
	{
		auto now = std::chrono::steady_clock::now();
		auto deltaw =
			std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(now - lastReportTime_).count();
		lastReportTime_ = now;
		return deltaw;
	}

	void _startProcTimer() { procStartTime_ = std::chrono::steady_clock::now(); }

	double _getProcTimerCount()
	{
		auto now = std::chrono::steady_clock::now();
		auto deltaw =
			std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(now - procStartTime_).count();
		return deltaw;
	}
};
}  // namespace mu2e

#endif /* mu2e_artdaq_Generators_Mu2eReceiver_hh */
