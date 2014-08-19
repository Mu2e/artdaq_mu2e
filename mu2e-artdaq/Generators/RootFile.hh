#ifndef mu2e_artdaq_Generators_RootFile_hh
#define mu2e_artdaq_Generators_RootFile_hh

// RootFile is a simple type of fragment generator intended to be
// studied by new users of artdaq as an example of how to create such
// a generator in the "best practices" manner. Derived from artdaq's
// CommandableFragmentGenerator class, it can be used in a full DAQ
// simulation, generating all ADC counts with equal probability via
// the std::uniform_int_distribution class

// RootFile is designed to simulate values coming in from one of
// two types of digitizer boards, one called "TOY1" and the other
// called "TOY2"; the only difference between the two boards is the #
// of bits in the ADC values they send. These values are declared as
// FragmentType enum's in mu2e-artdaq's
// mu2e-artdaq/Overlays/FragmentType.hh header.

// Some C++ conventions used:

// -Append a "_" to every private member function and variable

#include "fhiclcpp/fwd.h"
#include "artdaq-core/Data/Fragments.hh" 
#include "artdaq/Application/CommandableFragmentGenerator.hh"
#include "mu2e-artdaq/Overlays/ToyFragment.hh"
#include "mu2e-artdaq/Overlays/FragmentType.hh"
#include "TFile.h"

#include <random>
#include <vector>
#include <atomic>

namespace mu2e {    

  class RootFile : public artdaq::CommandableFragmentGenerator {
  public:
    explicit RootFile(fhicl::ParameterSet const & ps);

  private:

    // The "getNext_" function is used to implement user-specific
    // functionality; it's a mandatory override of the pure virtual
    // getNext_ function declared in CommandableFragmentGenerator

    bool getNext_(artdaq::FragmentPtrs & output) override;

    // Like "getNext_", "fragmentIDs_" is a mandatory override; it
    // returns a vector of the fragment IDs an instance of this class
    // is responsible for (in the case of RootFile, this is just
    // the fragment_id_ variable declared in the parent
    // CommandableFragmentGenerator class)
    
    std::vector<artdaq::Fragment::fragment_id_t> fragmentIDs_() {
      return fragment_ids_;
    }

    // FHiCL-configurable variables. Note that the C++ variable names
    // are the FHiCL variable names with a "_" appended

    std::size_t const nADCcounts_;     // ADC values per fragment per event
    FragmentType const fragment_type_; // Type of fragment (see FragmentType.hh)
    std::size_t const throttle_usecs_;
    
    std::vector<artdaq::Fragment::fragment_id_t> fragment_ids_; 

    // Members needed to generate the simulated data

    std::mt19937 engine_;
    std::unique_ptr<std::uniform_int_distribution<int>> uniform_distn_;
    TFile * file_;
  };
}

#endif /* mu2e_artdaq_Generators_RootFile_hh */
