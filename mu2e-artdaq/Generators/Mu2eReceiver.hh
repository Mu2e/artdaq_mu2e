#ifndef mu2e_artdaq_Generators_Mu2eReceiver_hh
#define mu2e_artdaq_Generators_Mu2eReceiver_hh

// Mu2eReceiver is designed to call the DTCInterfaceLibrary a certain number of times
// (set in the mu2eFragment header) and pack that data into DTCFragments contained
// in a single mu2eFragment object.

// Some C++ conventions used:

// -Append a "_" to every private member function and variable

#include "fhiclcpp/fwd.h"
#include "artdaq-core/Data/Fragments.hh"
#include "artdaq/Application/CommandableFragmentGenerator.hh"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"

#include "dtcInterfaceLib/DTC.h"
#include "dtcInterfaceLib/DTCSoftwareCFO.h"

#include <vector>
#include <sys/times.h>
#include <atomic>

namespace mu2e {    

  class Mu2eReceiver : public artdaq::CommandableFragmentGenerator {
  public:
    explicit Mu2eReceiver(fhicl::ParameterSet const & ps);
    virtual ~Mu2eReceiver();

  private:

    // The "getNext_" function is used to implement user-specific
    // functionality; it's a mandatory override of the pure virtual
    // getNext_ function declared in CommandableFragmentGenerator

    bool getNext_(artdaq::FragmentPtrs & output) override;

    // Like "getNext_", "fragmentIDs_" is a mandatory override; it
    // returns a vector of the fragment IDs an instance of this class
    // is responsible for (in the case of Mu2eReceiver, this is just
    // the fragment_id_ variable declared in the parent
    // CommandableFragmentGenerator class)
    
    std::vector<artdaq::Fragment::fragment_id_t> fragmentIDs_() {
      return fragment_ids_;
    }

    // FHiCL-configurable variables. Note that the C++ variable names
    // are the FHiCL variable names with a "_" appended

    FragmentType const fragment_type_; // Type of fragment (see FragmentType.hh)
    
    std::vector<artdaq::Fragment::fragment_id_t> fragment_ids_; 

    // State
    size_t timestamps_read_;
    double timestamp_rate_;
    clock_t lastReportTime_;
    DTCLib::DTC_SimMode mode_;
    uint8_t board_id_;

    DTCLib::DTC* theInterface_;
    DTCLib::DTCSoftwareCFO* theCFO_;
    double _timeSinceLastSend(clock_t& currenttime)
    {
      struct tms ctime;
      currenttime = times(&ctime);
      double deltaw = ((double)(currenttime - lastReportTime_))*10000./CLOCKS_PER_SEC;
      return deltaw;
    }
  };
}

#endif /* mu2e_artdaq_Generators_Mu2eReceiver_hh */
