#include "art/Framework/Core/EDFilter.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "artdaq/DAQdata/Globals.hh"
#define TRACE_NAME "DTCEventVerifier"

#include "canvas/Utilities/InputTag.h"

#include <artdaq-core/Data/ContainerFragment.hh>
#include "artdaq-core/Data/Fragment.hh"

#include "dtcInterfaceLib/DTC.h"
#include "artdaq-core-mu2e/Data/EventHeader.hh"
#include "artdaq-core-mu2e/Overlays/DTCEventFragment.hh"
#include "artdaq-core-mu2e/Overlays/FragmentType.hh"

#include "cetlib_except/exception.h"

#include <iomanip>
#include <sstream>

namespace mu2e {
  class DTCEventVerifier : public art::EDFilter
  {
  public:
    struct Config {
      fhicl::Atom<int>  diagLevel   {fhicl::Name("diagLevel"), fhicl::Comment("diagnostic level")};
      fhicl::Atom<int>  nDTCs       {fhicl::Name("nDTCs")    , fhicl::Comment("N DTCs used")};
    };

    explicit DTCEventVerifier(const art::EDFilter::Table<Config>& config);

    bool filter(art::Event & e) override;
    virtual bool endRun(art::Run& run ) override;


  private:
    std::set<int> dtcs_;
    int           diagLevel_;
    int           nDTCs_;
    bool          isFirstEvent_;
  };
}  // namespace mu2e

mu2e::DTCEventVerifier::DTCEventVerifier(const art::EDFilter::Table<Config>& config)
  : art::EDFilter{config}, 
    diagLevel_(config().diagLevel()),
    nDTCs_(config().nDTCs()),
    isFirstEvent_(true)    
{
  produces<mu2e::EventHeader>();
}

bool mu2e::DTCEventVerifier::filter(art::Event& event)
{
  std::unique_ptr<mu2e::EventHeader> evtHeader(new mu2e::EventHeader);

  artdaq::Fragments fragments;
  artdaq::FragmentPtrs containerFragments;

  std::vector<art::Handle<artdaq::Fragments> > fragmentHandles;
  fragmentHandles = event.getMany<std::vector<artdaq::Fragment> >();

  for (const auto& handle : fragmentHandles)
    {
      if (!handle.isValid() || handle->empty())
	{
	  continue;
	}
      
      if (handle->front().type() == artdaq::Fragment::ContainerFragmentType)
	{
	  for (const auto& cont : *handle)
	    {
	      artdaq::ContainerFragment contf(cont);
	      if (contf.fragment_type() != mu2e::FragmentType::DTCEVT)
		{
		  break;
		}

	      for (size_t ii = 0; ii < contf.block_count(); ++ii)
		{
		  containerFragments.push_back(contf[ii]);
		  fragments.push_back(*containerFragments.back());
		}
	    }
	}
      else
	{
	  if (handle->front().type() == mu2e::FragmentType::DTCEVT)
	    {
	      for (auto frag : *handle)
		{
		  fragments.emplace_back(frag);
		}
	    }
	}
    }
  
  
  if (diagLevel_ > 10) 
    {
      std::cout << "[ArtFragmentsFromDTCEvents::produce] Found nHandlesnFragments  " 
		<< fragments.size() <<std::endl;
    }
  
  evtHeader->initErrorChecks();
  for (const auto &frag : fragments) 
    {
    mu2e::DTCEventFragment bb(frag);
    auto  data = bb.getData();
    const DTCLib::DTC_EventHeader*   dtcHeader = data.GetHeader();    
    const DTCLib::DTC_EventWindowTag evtWTag   = data.GetEventWindowTag();
    if (isFirstEvent_){
      evtHeader->ewt    = evtWTag.GetEventWindowTag(true);
      // evtHeader->mode   = ;
      // evtHeader->rfmTDC = ;
      // evtHeader->flags  = ;
    }

    int evtNDTCs = dtcHeader->num_dtcs;
    //check with the nDTCs from the config
    if (evtNDTCs != nDTCs_) {	evtHeader->dtc_check = 0;}

    //uint8_t evtDTC_ID(0);
    for(const auto& subEvt: data.GetSubEvents())
      {
	//	const DTCLib::DTC_SubEventHeader* subHeader = subEvt.GetHeader();    
	uint8_t dtcID = subEvt.GetDTCID();
      
	if(dtcs_.insert(dtcID).second) 
	  {
	    if (!isFirstEvent_) { evtHeader->dtc_check = 0;} // dtcID wasn't in set
	  } 
	DTCLib::DTC_EventWindowTag subEvtWTag = subEvt.GetEventWindowTag();
      
	if ( subEvtWTag != evtWTag) { evtHeader->ewt_check = 0; } //different EWT in a subEvent
	
	//if (subHeader->dtc_mac == dtcHeader->dtc_mac){ evtDTC_ID = dtcID;}
      }//end loop over the subevents
    
    //check that the evtWTag == dtc_ID
    //FIX ME!
    // if (isFirstEvent_){
    //   refDTCID_ = evtDTC_ID;
    //   refEvtWTag_ = evtWTag;
    // }else{
    //   if ((evtWTag - refEvtTag) % nDTCs_ != 0) { header->rnr_check = 0;}    
    // }
    
    }
  
  bool condition = evtHeader->ewt_check && evtHeader->dtc_check;
  //change the state of isFirstEvent_
  if (isFirstEvent_) { isFirstEvent_ = false;}
  event.put(std::move(evtHeader));
  return condition;
}


bool mu2e::DTCEventVerifier::endRun( art::Run&  ) {
  return true;
}

DEFINE_ART_MODULE(mu2e::DTCEventVerifier)
