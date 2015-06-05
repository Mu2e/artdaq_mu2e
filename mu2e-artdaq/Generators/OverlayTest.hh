#ifndef mu2e_artdaq_Generators_OverlayTest_hh
#define mu2e_artdaq_Generators_OverlayTest_hh

// OverlayTest is a fragment generator based off of ToySimulator
// that is intended to serve as a test platform for the development
// of mu2e-specific overlay classes.
//
// OverlayTest uses the DTC interface library to fill tracker,
// calorimeter, or cosmic ray veto packets depending on the
// specified "fragment_type" (either "TRK", "CAL", or "CRV").

#include "fhiclcpp/fwd.h"
#include "artdaq-core/Data/Fragments.hh" 
#include "artdaq/Application/CommandableFragmentGenerator.hh"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "mu2e-artdaq-core/Overlays/DetectorFragment.hh"

#include <random>
#include <vector>
#include <atomic>

#include <bitset>

#include <queue>

#include "dtcInterfaceLib/DTC.h"

namespace mu2e {    

  class OverlayTest : public artdaq::CommandableFragmentGenerator {
  public:
    explicit OverlayTest(fhicl::ParameterSet const & ps);
    virtual ~OverlayTest();

  private:

    // The "generateFragmentID" function is used to parse out the
    // ring and ROC numbers from a DTC header packet and combine
    // them to create a fragment_id.

    artdaq::Fragment::fragment_id_t generateFragmentID(DTCLib::DTC_DataHeaderPacket &thePacket);

    // The "getNext_" function is used to implement user-specific
    // functionality; it's a mandatory override of the pure virtual
    // getNext_ function declared in CommandableFragmentGenerator

    bool getNext_(artdaq::FragmentPtrs & output) override;

    // Like "getNext_", "fragmentIDs_" is a mandatory override; it
    // returns a vector of the fragment IDs an instance of this class
    // is responsible for. Currently, the fragment IDs in OverlayTest
    // are just 8-bit words in which the 4 most significant bits are
    // the ring number, and the 4 least significant bits are the ROC
    // number.
    
    std::vector<artdaq::Fragment::fragment_id_t> fragmentIDs_() {
      return fragment_ids_;
    }

    // FHiCL-configurable variables. Note that the C++ variable names
    // are the FHiCL variable names with a "_" appended

    std::size_t const nADCcounts_;     // ADC values per fragment per event
    FragmentType const fragment_type_; // Type of fragment (see FragmentType.hh)
    std::size_t const throttle_usecs_;
    
    std::vector<artdaq::Fragment::fragment_id_t> fragment_ids_; 
    
    size_t dataIdx;
    std::vector<void*> data;

    // State
    size_t events_read_;
    bool isSimulatedDTC;

    DTCLib::DTC* theInterface;

  };
}

#endif /* mu2e_artdaq_Generators_OverlayTest_hh */