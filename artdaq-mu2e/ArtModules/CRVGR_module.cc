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
#include "artdaq-core-mu2e/Data/CRVDataDecoder.hh"
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
      fhicl::Atom<int>  diagLevel     {fhicl::Name("diagLevel")     , fhicl::Comment("diagnostic level"), 0};
      fhicl::Atom<int>  metrics_level {fhicl::Name("metricsLevel" ) , fhicl::Comment("Metrics reporting level"), 3};
    };

    explicit CRVGR(const art::EDFilter::Table<Config>& config);

    bool filter(art::Event & e) override;
    virtual bool endRun(art::Run& run ) override;


  private:
    std::set<int> dtcs_;
    int           diagLevel_;
    int           metrics_reporting_level_;
    int           nGrEvents_;
    int           nEvents_;
  };
}  // namespace mu2e

mu2e::CRVGR::CRVGR(const art::EDFilter::Table<Config>& config)
  : art::EDFilter{config}
  , diagLevel_(config().diagLevel())
  , metrics_reporting_level_(config().metrics_level())
  , nGrEvents_(0)   
  , nEvents_(0) 
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

  if (diagLevel_ > 1)
    {
      TLOG(TLVL_INFO) << "[CRVGR::filter] Found nFragments  "
		  << fragments.size();
    }
  if (metricMan != nullptr)
    {
      metricMan->sendMetric("nFragments", fragments.size(), "Fragments",
			    metrics_reporting_level_, artdaq::MetricMode::LastPoint);
    }

  for (const auto& frag : fragments) {
      mu2e::DTCEventFragment bb(frag);
      auto data = bb.getData();
      auto event = &data;
      if (diagLevel_ > 1) 
          TLOG(TLVL_INFO) << "Event tag:\t" << "0x" << std::hex << std::setw(4) << std::setfill('0') << event->GetEventWindowTag().GetEventWindowTag(true);
      // get the event and the relative sub events
      DTCLib::DTC_EventHeader* eventHeader = event->GetHeader();
      if (diagLevel_ > 1) {
	        TLOG(TLVL_INFO) << eventHeader->toJson() << std::endl
	        << "Subevents count: " << event->GetSubEventCount() << std::endl;
      }

      for (unsigned int i = 0; i < event->GetSubEventCount(); ++i) { // In future, use GetSubsystemData to only get CRV subevents
          DTCLib::DTC_SubEvent& subevent = *(event->GetSubEvent(i));
          if (diagLevel_ > 1) {
	            TLOG(TLVL_INFO) << "Subevent [" << i << "]:" << std::endl;
	            TLOG(TLVL_INFO) << subevent.GetHeader()->toJson() << std::endl;
	        }
          if (diagLevel_ > 1) {
              TLOG(TLVL_INFO) << "Number of Data Block: " << subevent.GetDataBlockCount() << std::endl;
          }
          for (size_t bl = 0; bl < subevent.GetDataBlockCount(); ++bl) {
              auto block = subevent.GetDataBlock(bl);
              auto blockheader = block->GetHeader();
              if (diagLevel_ > 1) {
                  TLOG(TLVL_INFO) << blockheader->toJSON() << std::endl;
                  for (int ii = 0; ii < blockheader->GetPacketCount(); ++ii) {
                      TLOG(TLVL_INFO) << DTCLib::DTC_DataPacket(((uint8_t*)block->blockPointer) + ((ii + 1) * 16)).toJSON() << std::endl;
                  }
              }
              // check if we want to decode this data block
              if(blockheader->isValid()
                 && blockheader->GetSubsystem() == 0x2 // 0x2 for CRV: DTC_Subsystem::DTC_Subsystem_CRV
                 //&& blockheader->GetVersion() == 0xff // 0xFF for GR1 packages
              ) {
                  if (metricMan != nullptr) {
                  metricMan->sendMetric("gr.subsystem", blockheader->GetSubsystem(), "subsystem",
			                                  metrics_reporting_level_, artdaq::MetricMode::LastPoint);
                  metricMan->sendMetric("gr.version", blockheader->GetVersion(), "version",
			                                  metrics_reporting_level_, artdaq::MetricMode::LastPoint);
                  metricMan->sendMetric("gr.valid", blockheader->isValid(), "subsystem",
			                                  metrics_reporting_level_, artdaq::MetricMode::LastPoint);
                  }

                  if( blockheader->GetVersion() == 0xff ) { // 0xFF for GR1 packages
                      ++nGrEvents_;
                      auto crvData = CRVGRDataDecoder(subevent); 
                      const mu2e::CRVGRDataDecoder::CRVGRRawPacket& crvRaw = crvData.GetCRVGRRawPacket(bl);
                      if (diagLevel_ > 0) {
                          TLOG(TLVL_INFO) << crvRaw;
                      }
                      auto CRVGRStatus = crvData.GetCRVGRStatusPacket(bl);
                      if (diagLevel_ > 0) {
                          TLOG(TLVL_INFO) << CRVGRStatus;
                      }

                      if (metricMan != nullptr) {
                          metricMan->sendMetric("gr.pllLock", 1-CRVGRStatus.PLLlock, "unlocked",
			                                           metrics_reporting_level_, artdaq::MetricMode::Accumulate|artdaq::MetricMode::Persist);
                          metricMan->sendMetric("gr.LossCnt", CRVGRStatus.LossCnt, "cnt",
			                                           metrics_reporting_level_, artdaq::MetricMode::LastPoint|artdaq::MetricMode::Persist);
                          metricMan->sendMetric("gr.CRCErrorCnt", CRVGRStatus.CRCErrorCnt, "cnt",
			                                      metrics_reporting_level_, artdaq::MetricMode::LastPoint|artdaq::MetricMode::Persist);
                          metricMan->sendMetric("gr.ewt", (int)CRVGRStatus.GetEventWindowTag(), " ",
			                                      metrics_reporting_level_, artdaq::MetricMode::LastPoint|artdaq::MetricMode::Persist);
                          metricMan->sendMetric("gr.BeamOn", CRVGRStatus.BeamOn, "on",
			                                      metrics_reporting_level_, artdaq::MetricMode::LastPoint|artdaq::MetricMode::Persist);
                          metricMan->sendMetric("gr.lastWindow", CRVGRStatus.lastWindow, "cnts",
			                                      metrics_reporting_level_, artdaq::MetricMode::LastPoint|artdaq::MetricMode::Minimum|artdaq::MetricMode::Maximum|artdaq::MetricMode::Persist);
                          metricMan->sendMetric("gr.InjectionTs", CRVGRStatus.InjectionTs, "cnts",
			                                      metrics_reporting_level_, artdaq::MetricMode::LastPoint|artdaq::MetricMode::Minimum|artdaq::MetricMode::Maximum|artdaq::MetricMode::Persist);
                          metricMan->sendMetric("gr.CRCErrorCnt", CRVGRStatus.CRCErrorCnt, "cnt",
			                                      metrics_reporting_level_, artdaq::MetricMode::LastPoint|artdaq::MetricMode::Persist);
                          metricMan->sendMetric("gr.MarkerCnt", CRVGRStatus.MarkerDelayCnt, "cnt",
			                                      metrics_reporting_level_, artdaq::MetricMode::LastPoint|artdaq::MetricMode::Persist);
                          metricMan->sendMetric("gr.HeartBeatCnt", CRVGRStatus.HeartBeatCnt, "cnt",
			                                      metrics_reporting_level_, artdaq::MetricMode::LastPoint|artdaq::MetricMode::Persist);
                          metricMan->sendMetric("gr.n", nGrEvents_, "cnt",
			                                      metrics_reporting_level_, artdaq::MetricMode::LastPoint|artdaq::MetricMode::Persist);
                      }
                  }

                  // wideband data taking mode
                  else if(blockheader->GetVersion() == 0x0 ) { // mode 0x0
                      ++nEvents_;
                      auto crvData = CRVDataDecoder(subevent); 
                      const auto crvStatus = crvData.GetCRVROCStatusPacket(bl);
                      metricMan->sendMetric("status.n", nEvents_, "cnt",
			                                      metrics_reporting_level_, artdaq::MetricMode::LastPoint|artdaq::MetricMode::Persist);
                      metricMan->sendMetric("status.eventWordCount", crvStatus.ControllerEventWordCount, "cnt",
			                                      metrics_reporting_level_, artdaq::MetricMode::LastPoint);
                      metricMan->sendMetric("status.triggerCount", crvStatus.TriggerCount, "cnt",
			                                      metrics_reporting_level_, artdaq::MetricMode::LastPoint);
                      metricMan->sendMetric("status.status", crvStatus.MicroBunchStatus, "status",
			                                      metrics_reporting_level_, artdaq::MetricMode::LastPoint);
                      metricMan->sendMetric("status.ewt", static_cast<int>(crvStatus.GetEventWindowTag()), "ewt",
			                                      metrics_reporting_level_, artdaq::MetricMode::LastPoint);
                      // loop through channel?
                      if (diagLevel_ > 0) {
                          TLOG(TLVL_INFO) << crvStatus;
                          for (auto& hit : crvData.GetCRVHits(bl)) {
                               TLOG(TLVL_INFO) << hit;
                          }
                      }
                  }
              }
          }
      }
  }

  return true;
}


bool mu2e::CRVGR::endRun( art::Run&  ) {
  return true;
}

DEFINE_ART_MODULE(mu2e::CRVGR)
