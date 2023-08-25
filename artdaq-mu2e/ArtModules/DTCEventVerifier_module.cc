#include "art/Framework/Core/EDFilter.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "TRACE/tracemf.h"
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
      fhicl::Atom<int>  diagLevel     {fhicl::Name("diagLevel")     , fhicl::Comment("diagnostic level")};
      fhicl::Atom<int>  nDTCs         {fhicl::Name("nDTCs")         , fhicl::Comment("N DTCs used")};
      fhicl::Atom<int>  metrics_level {fhicl::Name("metrics_level") , fhicl::Comment("Metrics reporting level"), 1};
    };

    explicit DTCEventVerifier(const art::EDFilter::Table<Config>& config);

    bool filter(art::Event & e) override;
    virtual bool endRun(art::Run& run ) override;


  private:
    std::set<int> dtcs_;
    int           diagLevel_;
    int           metrics_reporting_level_;
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
  
  
  if (diagLevel_ > 0) 
    {
      std::cout << "[ArtFragmentsFromDTCEvents::produce] Found nFragments  " 
		<< fragments.size() <<std::endl;
    }
  if (metricMan != nullptr)
    {
      metricMan->sendMetric("nFragments", fragments.size(), "Fragments",
			    metrics_reporting_level_, artdaq::MetricMode::LastPoint);
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
	
	if (metricMan != nullptr && diagLevel_ > 10)
	  {
	    metricMan->sendMetric("SubEventID"      , int(&subEvt - &data.GetSubEvents()[0]), "SubEvent",
				 metrics_reporting_level_, artdaq::MetricMode::LastPoint);
	    metricMan->sendMetric("SubEventDTCID"   ,dtcID , "SubEvent",
				 metrics_reporting_level_, artdaq::MetricMode::LastPoint);
	    metricMan->sendMetric("SubEventEWTCHECK", ( (subEvtWTag != evtWTag) ? 0 : 1), "SubEvent",
				 metrics_reporting_level_, artdaq::MetricMode::LastPoint);
	    metricMan->sendMetric("SubEventDTCCHECK", ( ((dtcs_.insert(dtcID).second) && (!isFirstEvent_)) ? 0 : 1), "SubEvent",
				 metrics_reporting_level_, artdaq::MetricMode::LastPoint);

	  }
	
	//if (subHeader->dtc_mac == dtcHeader->dtc_mac){ evtDTC_ID = dtcID;}
      }//end loop over the subevents
    
    if (metricMan != nullptr)
      {
	metricMan->sendMetric("FragmentID"      , int(&frag - &fragments[0]), "Fragment",
			     metrics_reporting_level_, artdaq::MetricMode::LastPoint);
	metricMan->sendMetric("FragmentEWTCHECK", evtHeader->ewt_check, "Fragment",
			     metrics_reporting_level_, artdaq::MetricMode::LastPoint);
	metricMan->sendMetric("FragmentDTCsFRAC", float(evtNDTCs/nDTCs_) , "Fragment",
			     metrics_reporting_level_, artdaq::MetricMode::LastPoint);
	metricMan->sendMetric("FragmentDTCCHECK", evtHeader->dtc_check, "Fragment",
			     metrics_reporting_level_, artdaq::MetricMode::LastPoint);
  
    }
    //check that the evtWTag == dtc_ID
    //FIX ME!
    // if (isFirstEvent_){
    //   refDTCID_ = evtDTC_ID;
    //   refEvtWTag_ = evtWTag;
    // }else{
    //   if ((evtWTag - refEvtTag) % nDTCs_ != 0) { header->rnr_check = 0;}    
    // }
    
    }
  // if (metricMan != nullptr) 
  //   {
  //     std::ostringstream oss;
  //     metricMan->sendMEtric("SubEventsPassed", subEvtsPassed, "Events", metrics_reporting_level_, 
  // 			    artdaq::MetricMode::LastPoint);
  //     TLOG(TLVL_DEBUG) << "Event " << evt.event() << ": total subEvents = " << nSubEvents << oss.str();
  //   }
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
