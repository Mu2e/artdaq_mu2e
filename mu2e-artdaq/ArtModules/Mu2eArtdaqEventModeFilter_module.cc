///////////////////////////////////////////////////////////////////////////////////////
// Class:       DetectorDump
// Module Type: EDAnalyzer
// File:        DetectorDump_module.cc
// Description: Prints out DTCFragments in HWUG Packet format (see mu2e-docdb #4097)
///////////////////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDFilter.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "canvas/Utilities/Exception.h"

#include <ctime>
#include <fstream>

#include <cstdlib>
#include <sstream>

#include "trace.h"

#include <cstdint>
#include <iostream>
#include <string>

namespace mu2e {
class Mu2eArtdaqEventModeFilter;
}

class mu2e::Mu2eArtdaqEventModeFilter : public art::EDFilter
{
public:
	explicit Mu2eArtdaqEventModeFilter(fhicl::ParameterSet const& pset);
	virtual ~Mu2eArtdaqEventModeFilter();

	virtual bool filter(art::Event const& evt);

private:
  std::string  evtHeaderLabel_;
  uint8_t      eventModeToSelect_;
  

};

mu2e::Mu2eArtdaqEventModeFilter::Mu2eArtdaqEventModeFilter(fhicl::ParameterSet const& pset) : 
  EDFilter(pset)
  , evtHeaderLabel_    (pset.get<std::string>("EventHeaderModuleLabel"))
  , eventModeToSelect_ (pset.get<uint8_t>("EventModeToSelect"))
											      
{
  
}
mu2e::Mu2eArtdaqEventModeFilter::~Mu2eArtdaqEventModeFilter() {}

bool mu2e::Mu2eArtdaqEventModeFilter::filter(art::Event const& evt)
{
  art::Handle<artdaq::Mu2eEventHeader> evtHeaderH;
  evt.getByLabel(evtHeaderLabel_, evtHeaderHraw);
  
  if (!evtHeaderH.isValid()) {
    std::cout << "[mu2e::Mu2eArtdaqEventModeFilter::filter] no Mu2eEventHeader found with string: " << evtHeaderLabel_ << std::endl;
    return false;
  }

  if (evtHeaderH->EventMode == eventModeToSelect_) 
    return true;
  
  return false;
}

DEFINE_ART_MODULE(mu2e::Mu2eArtdaqEventModeFilter)
