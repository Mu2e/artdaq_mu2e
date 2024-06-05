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
      fhicl::Atom<int>  metrics_level {fhicl::Name("metricsLevel" ) , fhicl::Comment("Metrics reporting level"), 1};
      fhicl::Atom<bool> skipCheck     {fhicl::Name("skipCheck")     , fhicl::Comment("Skip check")};
    };

    explicit DTCEventVerifier(const art::EDFilter::Table<Config>& config);

    bool filter(art::Event & e) override;
    virtual bool endRun(art::Run& run ) override;


  private:
    std::set<int> dtcs_;
    int           diagLevel_;
    int           metrics_reporting_level_;
    int           nDTCs_;
    bool          skipCheck_;
    bool          isFirstEvent_;
  };
}  // namespace mu2e

mu2e::DTCEventVerifier::DTCEventVerifier(const art::EDFilter::Table<Config>& config)
  : art::EDFilter{config}, 
  diagLevel_(config().diagLevel()),
  nDTCs_(config().nDTCs()),
  skipCheck_(config().skipCheck()),
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
      std::cout << "[DTCEventVerifier::produce] Found nFragments  "
		<< fragments.size() << std::endl;
    }
  if (metricMan != nullptr)
    {
      metricMan->sendMetric("nFragments", fragments.size(), "Fragments",
			    metrics_reporting_level_, artdaq::MetricMode::LastPoint);
    }

  evtHeader->initErrorChecks();


  std::stringstream ostr;
  for (const auto& frag : fragments)
    {
      mu2e::DTCEventFragment bb(frag);
      auto data = bb.getData();
      auto event = &data;
      if (diagLevel_ > 0) ostr << "Event tag:\t" << "0x" << std::hex << std::setw(4) << std::setfill('0') << event->GetEventWindowTag().GetEventWindowTag(true) << std::endl;
      // get the event and the relative sub events
      DTCLib::DTC_EventHeader* eventHeader = event->GetHeader();
      std::vector<DTCLib::DTC_SubEvent> subevents = event->GetSubEvents();

      // print the event header
     if (diagLevel_ > 0)
       {
	 ostr << eventHeader->toJson() << std::endl
	      << "Subevents count: " << event->GetSubEventCount() << std::endl;
       }

      // iterate over the subevents
      for (unsigned int i = 0; i < subevents.size(); ++i)
	{
	  // print the subevents header
	  DTCLib::DTC_SubEvent subevent = subevents[i];
	  if (diagLevel_ > 0) 
	    {
	      ostr << "Subevent [" << i << "]:" << std::endl;
	      ostr << subevent.GetHeader()->toJson() << std::endl;
	    }

	  // check if there is an error on the link
	  if (subevent.GetHeader()->link0_status > 0)
	    {
	      if (diagLevel_ > 0)  ostr << "Error: " << std::endl;
	      std::bitset<8> link0_status(subevent.GetHeader()->link0_status);
	      if (link0_status.test(0))
		{
		  if (diagLevel_ > 0)  ostr << "ROC Timeout Error!" << std::endl;
		}
	      if (link0_status.test(2))
		{
		  if (diagLevel_ > 0)  ostr << "Packet sequence number Error!" << std::endl;
		}
	      if (link0_status.test(3))
		{
		 if (diagLevel_ > 0)   ostr << "CRC Error!" << std::endl;
		}
	      if (link0_status.test(6))
		{
		  if (diagLevel_ > 0)  ostr << "Fatal Error!" << std::endl;
		}

	      continue;
	    }

	  // print the number of data blocks
	 if (diagLevel_ > 0)   ostr << "Number of Data Block: " << subevent.GetDataBlockCount() << std::endl;

	  // iterate over the data blocks
	  std::vector<DTCLib::DTC_DataBlock> dataBlocks = subevent.GetDataBlocks();
	  for (unsigned int j = 0; j < dataBlocks.size(); ++j)
	    {
	      if (diagLevel_ > 0) ostr << "Data block [" << j << "]:" << std::endl;
	      // print the data block header
	      DTCLib::DTC_DataHeaderPacket* dataHeader = dataBlocks[j].GetHeader().get();
	      if (diagLevel_ > 0) ostr << dataHeader->toJSON() << std::endl;

	      // print the data block payload
	      const void* dataPtr = dataBlocks[j].GetData();
	      if (diagLevel_ > 0)  ostr << "Data payload:" << std::endl;
	      for (int l = 0; l < dataHeader->GetByteCount() - 16; l += 2)
		{
		  auto thisWord = reinterpret_cast<const uint16_t*>(dataPtr)[l];
		  if (diagLevel_ > 0)  ostr << "\t0x" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(thisWord) << std::endl;
		}
	    }
	}
      if (diagLevel_ > 0) {
	ostr << std::endl
	   << std::endl;
      }
    }






  event.put(std::move(evtHeader));

  return true;

  
  // std::unique_ptr<mu2e::EventHeader> evtHeader(new mu2e::EventHeader);

  // artdaq::Fragments fragments;
  // artdaq::FragmentPtrs containerFragments;

  // std::vector<art::Handle<artdaq::Fragments> > fragmentHandles;
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
      std::cout << "[DTCEventVerifier::produce] Found nFragments  " 
		<< fragments.size() <<std::endl;
    }
  if (metricMan != nullptr)
    {
      metricMan->sendMetric("nFragments", fragments.size(), "Fragments",
			    metrics_reporting_level_, artdaq::MetricMode::LastPoint);
    }

  evtHeader->initErrorChecks();

  size_t index_frag(0);
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
      size_t index_subEvt(0);
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
	      metricMan->sendMetric("SubEventID"      , index_subEvt, "SubEvent",
				    metrics_reporting_level_, artdaq::MetricMode::LastPoint);
	      metricMan->sendMetric("SubEventDTCID"   ,dtcID , "SubEvent",
				    metrics_reporting_level_, artdaq::MetricMode::LastPoint);
	      metricMan->sendMetric("SubEventEWTCHECK", ( (subEvtWTag != evtWTag) ? 0 : 1), "SubEvent",
				    metrics_reporting_level_, artdaq::MetricMode::LastPoint);
	      metricMan->sendMetric("SubEventDTCCHECK", ( ((dtcs_.insert(dtcID).second) && (!isFirstEvent_)) ? 0 : 1), "SubEvent",
				    metrics_reporting_level_, artdaq::MetricMode::LastPoint);

	    }
	  ++index_subEvt;
	  //if (subHeader->dtc_mac == dtcHeader->dtc_mac){ evtDTC_ID = dtcID;}
	}//end loop over the subevents
    
      if (metricMan != nullptr)
	{
	  metricMan->sendMetric("FragmentID"      , index_frag, "Fragment",
				metrics_reporting_level_, artdaq::MetricMode::LastPoint);
	  metricMan->sendMetric("FragmentEWTCHECK", evtHeader->ewt_check, "Fragment",
				metrics_reporting_level_, artdaq::MetricMode::LastPoint);
	  metricMan->sendMetric("FragmentDTCsFRAC", float(evtNDTCs/nDTCs_) , "Fragment",
				metrics_reporting_level_, artdaq::MetricMode::LastPoint);
	  metricMan->sendMetric("FragmentDTCCHECK", evtHeader->dtc_check, "Fragment",
				metrics_reporting_level_, artdaq::MetricMode::LastPoint);
  
	}
      ++index_frag;
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
  return (condition || skipCheck_);
}


bool mu2e::DTCEventVerifier::endRun( art::Run&  ) {
  return true;
}

DEFINE_ART_MODULE(mu2e::DTCEventVerifier)
