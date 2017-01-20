// ======================================================================
//
// Mu2eProducer_plugin:  Add tracker/cal data products to the event
//
// ======================================================================

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "fhiclcpp/ParameterSet.h"

#include "art/Framework/Principal/Handle.h"

#ifdef CANVAS
#include "canvas/Utilities/Exception.h"
#else
#include "art/Utilities/Exception.h"
#endif

//#include "mu2e-artdaq-core/Overlays/mu2eFragmentReader.hh"

#include "mu2e-artdaq-core/Overlays/ArtFragmentReader.hh"

#include "artdaq-core/Data/Fragments.hh"
//#include "dtcInterfaceLib/DTC_Packets.h"


#include <iostream>

#include <string>

#include "trace.h"

#include <memory>

namespace art {
  class Mu2eProducer;
}

using namespace fhicl;
using art::Mu2eProducer;

// ======================================================================

class art::Mu2eProducer
  : public EDProducer
{

public:
  // --- Characteristics:
  using EventNumber_t = art::EventNumber_t;

  using adc_t = mu2e::ArtFragmentReader::adc_t;

  typedef std::vector<adc_t> ADCWaveform;
  typedef unsigned long TDCValues[2];
  typedef unsigned long StrawIndex; // COMMENT OUT ONCE INCLUDING StrawIndex.hh
  
  // --- Configuration
  struct Config {
    Atom<bool>        debug { Name("debug"), false };
  };

  // --- C'tor/d'tor:
  using Parameters = EDProducer::Table<Config>;
  explicit  Mu2eProducer( Parameters const & );
  virtual  ~Mu2eProducer()  { }

  // --- Production:
  virtual void produce( Event & );

private:
  bool  debug_;

  art::InputTag trkFragmentsTag_;
  art::InputTag caloFragmentsTag_;

};  // Mu2eProducer

// ======================================================================

Mu2eProducer::Mu2eProducer( Parameters const & config )
  : EDProducer( )
  , debug_    ( config().debug() )
  , trkFragmentsTag_{"daq:trk"}
  , caloFragmentsTag_{"daq:calo"}
{
  produces<EventNumber_t>(); 

}

// ----------------------------------------------------------------------

void
  Mu2eProducer::
  produce( Event & event )
{

  // This is just a placeholder so that the module produces *something*. Once the Offline DataProduct
  // dependencies are available, the module will produce Offline DataProducts. For now, it just produces
  // event numbers

  art::EventNumber_t eventNumber = event.event();
  if( debug_ ) {
    TRACE( 11, "mu2e::Mu2eProducer::produce enter eventNumber=%d", eventNumber );
    std::cout << "mu2e::Mu2eProducer::produce enter eventNumber=" << (int)eventNumber << std::endl;

  }
  event.put(std::unique_ptr<EventNumber_t>(new EventNumber_t( eventNumber )));

  auto trkFragments = event.getValidHandle<artdaq::Fragments>(trkFragmentsTag_);
  auto calFragments = event.getValidHandle<artdaq::Fragments>(caloFragmentsTag_);

  if( debug_ ) {
    std::cout << std::dec << "Producer: Run " << event.run() << ", subrun " << event.subRun()
	      << ", event " << eventNumber << " has " << std::endl;
    std::cout << trkFragments->size() << " TRK fragments, and ";
    std::cout << calFragments->size() << " CAL fragments." << std::endl;

    size_t totalSize = 0;
    for(size_t idx = 0; idx < trkFragments->size(); ++idx) {
      auto size = ((*trkFragments)[idx]).size() * sizeof(artdaq::RawDataType);
      totalSize += size;
      //      std::cout << "\tTRK Fragment " << idx << " has size " << size << std::endl;
    }
    for(size_t idx = 0; idx < calFragments->size(); ++idx) {
      auto size = ((*calFragments)[idx]).size() * sizeof(artdaq::RawDataType);
      totalSize += size;
      //      std::cout << "\tCAL Fragment " << idx << " has size " << size << std::endl;
    }
    
    std::cout << "\tTotal Size: " << (int)totalSize << " bytes." << std::endl;  
  }




  std::string curMode = "TRK";
  // Loop over fragments in the event
  for (size_t idx = 0; idx < trkFragments->size(); ++idx) {
    const auto& fragment((*trkFragments)[idx]);
    
    mu2e::ArtFragmentReader cc(fragment);
    
    if( debug_ ) {
      std::cout << std::endl;
      std::cout << "ArtFragmentReader: ";
      std::cout << "\tBlock Count: " << std::dec << cc.block_count() << std::endl;
      std::cout << "\tByte Count: " << cc.byte_count() << std::endl;
      std::cout << std::endl;
      std::cout << "\t" << "====== Example Block Sizes ======" << std::endl;
      for(size_t i=0; i<10; i++) {
	if(i <cc.block_count()) {
	  std::cout << "\t" << i << "\t" << cc.blockIndexBytes(i) << "\t" << cc.blockSizeBytes(i) << std::endl;
	}
      }
      std::cout << "\t" << "=========================" << std::endl;
    }
    
    std::string mode_;

    for(size_t curBlockIdx=0; curBlockIdx<cc.block_count(); curBlockIdx++) {

      size_t blockStartBytes = cc.blockIndexBytes(curBlockIdx);
      size_t blockEndBytes = cc.blockEndBytes(curBlockIdx);

      if( debug_ ) {
	std::cout << "BLOCKSTARTEND: " << blockStartBytes << " " << blockEndBytes << " " << cc.blockSizeBytes(curBlockIdx)<< std::endl;
	std::cout << "IndexComparison: " << cc.blockIndexBytes(0)+16*(0+3*curBlockIdx) << "\t";
	std::cout                        << cc.blockIndexBytes(curBlockIdx)+16*(0+3*0) << std::endl;
      }

      adc_t const *pos = reinterpret_cast<adc_t const *>(cc.dataAtBytes(blockStartBytes));

      if( debug_ ) {
	// Print binary contents the first 3 packets starting at the current position
	// In the case of the tracker simulation, this will be the whole tracker
	// DataBlock. In the case of the calorimeter, the number of data packets
	// following the header packet is variable.
	cc.printPacketAtByte(cc.blockIndexBytes(0)+16*(0+3*curBlockIdx));
	cc.printPacketAtByte(cc.blockIndexBytes(0)+16*(1+3*curBlockIdx));
	cc.printPacketAtByte(cc.blockIndexBytes(0)+16*(2+3*curBlockIdx));
	
	// Print out decimal values of 16 bit chunks of packet data
	for(int i=7; i>=0; i--) {
	  std::cout << (adc_t) *(pos+i);
	  std::cout << " ";
	}
	std::cout << std::endl;
      }	    

      adc_t rocID = cc.DBH_ROCID(pos);
      adc_t ringID = cc.DBH_RingID(pos);
      adc_t valid = cc.DBH_Valid(pos);
      adc_t packetCount = cc.DBH_PacketCount(pos);
	    
      uint32_t timestampLow    = cc.DBH_TimestampLow(pos);
      uint32_t timestampMedium = cc.DBH_TimestampMedium(pos);
      size_t timestamp = timestampLow | (timestampMedium<<16);
      
      adc_t EVBMode = cc.DBH_EVBMode(pos);
      adc_t sysID = cc.DBH_SubsystemID(pos);
      adc_t dtcID = cc.DBH_DTCID(pos);
      
      if(sysID==0) {
	mode_ = "TRK";
      } else if(sysID==1) {
	mode_ = "CAL";
      }

      // Parse phyiscs information from TRK packets
      if(mode_ == "TRK" && packetCount>0) {
	
	adc_t strawIdx = cc.DBT_StrawIndex(pos);
	uint32_t TDC0  = cc.DBT_TDC0(pos);
	uint32_t TDC1  = cc.DBT_TDC1(pos);
	ADCWaveform waveform = cc.DBT_Waveform(pos);
	
	// FILL StrawDigi's here
	TDCValues tdc = {TDC0,TDC1};
	StrawIndex sid = strawIdx;
	// StrawDigi theDigi( sid, tdc, waveform);
	
	if( debug_ ) {
	  std::cout << "MAKEDIGI: " << sid << " " << tdc[0] << " " << tdc[1] << " ";
	  for(size_t i=0; i<waveform.size(); i++) {
	    std::cout << waveform[i];
	    if(i<waveform.size()-1) {
	      std::cout << " ";
	    }
	  }
	  std::cout << std::endl;
	}
	
	if( debug_ ) {
	  std::cout << "timestamp: " << timestamp << std::endl;
	  std::cout << "sysID: " << sysID << std::endl;
	  std::cout << "dtcID: " << dtcID << std::endl;
	  std::cout << "rocID: " << rocID << std::endl;
	  std::cout << "ringID: " << ringID << std::endl;
	  std::cout << "packetCount: " << packetCount << std::endl;
	  std::cout << "valid: " << valid << std::endl;
	  std::cout << "EVB mode: " << EVBMode << std::endl;
	  
	  for(int i=7; i>=0; i--) {
	    std::cout << (adc_t) *(pos+8+i);
	    std::cout << " ";
	  }
	  std::cout << std::endl;
	  
	  for(int i=7; i>=0; i--) {
	    std::cout << (adc_t) *(pos+8*2+i);
	    std::cout << " ";
	  }
	  std::cout << std::endl;
	  
	  std::cout << "strawIdx: " << strawIdx << std::endl;
	  std::cout << "TDC0: " << TDC0 << std::endl;
	  std::cout << "TDC1: " << TDC1 << std::endl;
	  std::cout << "Waveform: {";
	  for(size_t i=0; i<waveform.size(); i++) {
	    std::cout << waveform[i];
	    if(i<waveform.size()-1) {
	      std::cout << ",";
	    }
	  }
	  std::cout << "}" << std::endl;
	  
	  std::cout << "LOOP: " << eventNumber << " " << curBlockIdx << " " << "(" << timestamp << ")" << std::endl;	    
	  
	  // Text format: timestamp strawidx tdc0 tdc1 nsamples sample0-11
	  // 1 1113 36978 36829 12 1423 1390 1411 1354 2373 2392 2342 2254 1909 1611 1525 1438
	  std::cout << "GREPMETRK: " << timestamp << " ";
	  std::cout << strawIdx << " ";
	  std::cout << TDC0 << " ";
	  std::cout << TDC1 << " ";
	  std::cout << waveform.size() << " ";
	  for(size_t i=0; i<waveform.size(); i++) {
	    std::cout << waveform[i];
	    if(i<waveform.size()-1) {
	      std::cout << " ";
	    }
	  }
	  std::cout << std::endl;
	} // End debug output
	
	
	
	
	
      } else if(mode_ == "CAL" && packetCount>0) {	// Parse phyiscs information from CAL packets
	
	adc_t crystalID  = cc.DBC_CrystalID(pos);
	adc_t apdID      = cc.DBC_apdID(pos);
	adc_t time       = cc.DBC_Time(pos);
	adc_t numSamples = cc.DBC_NumSamples(pos);
	ADCWaveform waveform = cc.DBC_Waveform(pos);
	
	if( debug_ ) {
	  std::cout << "timestamp: " << timestamp << std::endl;
	  std::cout << "sysID: " << sysID << std::endl;
	  std::cout << "dtcID: " << dtcID << std::endl;
	  std::cout << "rocID: " << rocID << std::endl;
	  std::cout << "ringID: " << ringID << std::endl;
	  std::cout << "packetCount: " << packetCount << std::endl;
	  std::cout << "valid: " << valid << std::endl;
	  std::cout << "EVB mode: " << EVBMode << std::endl;		
	  
	  for(int i=7; i>=0; i--) {
	    std::cout << (adc_t) *(pos+8+i);
	    std::cout << " ";
	  }
	  std::cout << std::endl;
	  
	  for(int i=7; i>=0; i--) {
	    std::cout << (adc_t) *(pos+8*2+i);
	    std::cout << " ";
	  }
	  std::cout << std::endl;
	  
	  std::cout << "Crystal ID: " << crystalID << std::endl;		
	  std::cout << "APD ID: " << apdID << std::endl;
	  std::cout << "Time: " << time << std::endl;
	  std::cout << "NumSamples: " << numSamples << std::endl;
	  std::cout << "Waveform: {";
	  for(size_t i=0; i<waveform.size(); i++) {
	    std::cout << waveform[i];
	    if(i<waveform.size()-1) {
	      std::cout << ",";
	    }
	  }
	  std::cout << "}" << std::endl;
	  
	  // Text format: timestamp crystalID roID time nsamples samples...
	  // 1 201 402 660 18 0 0 0 0 1 17 51 81 91 83 68 60 58 52 42 33 23 16
	  std::cout << "GREPMECAL: " << timestamp << " ";
	  std::cout << crystalID << " ";
	  std::cout << apdID << " ";
	  std::cout << time << " ";
	  std::cout << waveform.size() << " ";
	  for(size_t i=0; i<waveform.size(); i++) {
	    std::cout << waveform[i];
	    if(i<waveform.size()-1) {
	      std::cout << " ";
	    }
	  }
	  std::cout << std::endl;
	} // End debug output
	
      } // End Cal Mode
      
    } // End loop over DataBlocks within fragment 
      
  } // Close loop over fragments

}  // produce()

// ======================================================================

DEFINE_ART_MODULE(Mu2eProducer)

// ======================================================================
