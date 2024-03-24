#include "art/Framework/Core/EDFilter.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "TRACE/tracemf.h"
#include "artdaq/DAQdata/Globals.hh"
#define TRACE_NAME "CRVGR"

#include "canvas/Utilities/InputTag.h"

#include <artdaq-core/Data/ContainerFragment.hh>
#include "artdaq-core/Data/Fragment.hh"

#include "artdaq-core-mu2e/Data/EventHeader.hh"
#include "artdaq-core-mu2e/Data/CRVGRDataDecoder.hh"
#include "artdaq-core-mu2e/Overlays/DTCEventFragment.hh"
#include "artdaq-core-mu2e/Overlays/FragmentType.hh"

#include "cetlib_except/exception.h"

#include <iomanip>
#include <sstream>

namespace mu2e {
  class CRVGR : public art::EDFilter
  {
  public:
    struct Config {
      fhicl::Atom<int>  diagLevel     {fhicl::Name("diagLevel")     , fhicl::Comment("diagnostic level"), 100};
      fhicl::Atom<int>  metrics_level {fhicl::Name("metricsLevel" ) , fhicl::Comment("Metrics reporting level"), 1};
    };

    explicit CRVGR(const art::EDFilter::Table<Config>& config);

    bool filter(art::Event & e) override;
    virtual bool endRun(art::Run& run ) override;


  private:
    std::set<int> dtcs_;
    int           diagLevel_;
    int           metrics_reporting_level_;
    bool          isFirstEvent_; // not used
    int           nDTCs_; // not used
  };
}  // namespace mu2e

mu2e::CRVGR::CRVGR(const art::EDFilter::Table<Config>& config)
  : art::EDFilter{config}, 
    isFirstEvent_(true)    
{
  //produces<mu2e::EventHeader>();
}

bool mu2e::CRVGR::filter(art::Event& event)
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
      std::cout << "[CRVGR::filter] Found nFragments  "
		<< fragments.size() << std::endl;
    }
  if (metricMan != nullptr)
    {
      metricMan->sendMetric("nFragments", fragments.size(), "Fragments",
			    metrics_reporting_level_, artdaq::MetricMode::LastPoint);
    }

  //evtHeader->initErrorChecks();

  for (const auto& frag : fragments) {
      mu2e::DTCEventFragment bb(frag);
      auto data = bb.getData();
      auto event = &data;
      if (diagLevel_ > 0) 
          std::cout << "Event tag:\t" << "0x" << std::hex << std::setw(4) << std::setfill('0') << event->GetEventWindowTag().GetEventWindowTag(true) << std::endl;
      // get the event and the relative sub events
      DTCLib::DTC_EventHeader* eventHeader = event->GetHeader();
      std::vector<DTCLib::DTC_SubEvent> subevents = event->GetSubEvents(); // In future, use GetSubsystemData to only get CRV subevents
      if (diagLevel_ > 0) {
	        std::cout << eventHeader->toJson() << std::endl
	        << "Subevents count: " << event->GetSubEventCount() << std::endl;
      }

      for (unsigned int i = 0; i < subevents.size(); ++i) {
          DTCLib::DTC_SubEvent subevent = subevents[i];
          if (diagLevel_ > 0) {
	            std::cout << "Subevent [" << i << "]:" << std::endl;
	            std::cout << subevent.GetHeader()->toJson() << std::endl;
	        }
          std::vector<DTCLib::DTC_DataBlock> dataBlocks = subevent.GetDataBlocks();
          if (diagLevel_ > 0) {
              std::cout << "Number of Data Block: " << subevent.GetDataBlockCount() << " (" 
                        << dataBlocks.size() << ")" << std::endl;
          }
	        for (unsigned int j = 0; j < dataBlocks.size(); ++j) {
	             // print the data block header
	             DTCLib::DTC_DataHeaderPacket* dataHeader = dataBlocks[j].GetHeader().get();
	             if (diagLevel_ > 0) {
                   std::cout << "Data block [" << j << "]:" << std::endl;
                   std::cout << dataHeader->toJSON();
               } 

               // print the data block payload
	             const void* dataPtr = dataBlocks[j].GetData();
	             if (diagLevel_ > 0)  std::cout << "Data payload:" << std::endl;
	             for (int l = 0; l < dataHeader->GetByteCount(); l += 2) {
		                auto thisWord = reinterpret_cast<const uint16_t*>(dataPtr)[l];
		                 if (diagLevel_ > 0)  std::cout << "\t0x" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(thisWord) << std::endl;
		           }


               // check if we want to decode
               if(dataHeader->isValid() &&
                  dataHeader->GetSubsystem() == 0 && // 0x2 for CRV in future DTC_Subsystem::DTC_Subsystem_CRV
                  dataHeader->GetVersion() == 0 // in future 0xFF for GR1 packages
               ) {
                  auto crvData = CRVGRDataDecoder(subevent); // not sure this can work because we have now multiple data ?
                  //std::cout << "DEBUG" << std::endl;
                  //std::cout << "DEBUG" << std::endl;
                  //mu2e::CRVGRDataDecoder::CRVGRStatusPacket& crvHeader = *crvData.GetCRVGRStatusPacket(j);
                  //std::cout << crvHeader;
                  mu2e::CRVGRDataDecoder::CRVGRRawPacket& crvRaw = *crvData.GetCRVGRRawPacket(j);
                  std::cout << crvRaw;
                  mu2e::CRVGRDataDecoder::CRVGRStatusPacket& crvHeader = *crvData.GetCRVGRStatusPacket(j);
                  std::cout << crvHeader;
                  
               }
               //const void* dataPtr = dataBlocks[j].GetData(); // gets the payload
               //if (diagLevel_ > 0) {
               //    std::cout << "Data byte count " << j << "]:" << std::endl;
               //}
          }
      }
  }

  return true;
}


bool mu2e::CRVGR::endRun( art::Run&  ) {
  return true;
}

DEFINE_ART_MODULE(mu2e::CRVGR)
