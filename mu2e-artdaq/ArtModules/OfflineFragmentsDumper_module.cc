///////////////////////////////////////////////////////////////////////////////////////
// Class:       Mu2eDump
// Module Type: EDAnalyzer
// File:        Mu2eDump_module.cc
// Description: Prints out mu2eFragments in HWUG Packet format (see mu2e-docdb #4097)
///////////////////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#include "canvas/Utilities/Exception.h"

#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"
#include "artdaq-core/Data/Fragments.hh"
#include "dtcInterfaceLib/DTC_Packets.h"

#include "trace.h"

#include <string>
#include <iostream>
#include <iomanip>
#include <unistd.h>

namespace mu2e
{
  class Mu2eDump;
}

class mu2e::Mu2eDump : public art::EDAnalyzer {
public:

  explicit Mu2eDump(fhicl::ParameterSet const& pset);
  virtual void analyze(art::Event const& evt);

private:
  art::InputTag trkFragmentsTag_;
  art::InputTag caloFragmentsTag_;
};

mu2e::Mu2eDump::Mu2eDump(fhicl::ParameterSet const& pset)
  : EDAnalyzer{pset}
  , trkFragmentsTag_{"daq:trk"}
  , caloFragmentsTag_{"daq:calo"}
{}

void mu2e::Mu2eDump::analyze(art::Event const& e)
{
  std::cout << e.getValidHandle<artdaq::Fragments>(trkFragmentsTag_)->size() << '\n';
  std::cout << e.getValidHandle<artdaq::Fragments>(caloFragmentsTag_)->size() << '\n';
}

DEFINE_ART_MODULE(mu2e::Mu2eDump)
