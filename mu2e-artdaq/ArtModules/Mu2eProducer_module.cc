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

#include "mu2e-artdaq-core/Overlays/mu2eFragmentReader.hh"
#include "artdaq-core/Data/Fragments.hh"
#include "dtcInterfaceLib/DTC_Packets.h"


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


  using adc_t = mu2e::mu2eFragmentReader::adc_t;

  typedef std::vector<unsigned short> ADCWaveform;
  typedef unsigned long TDCValues[2];
  typedef unsigned long StrawIndex; // COMMENT OUT ONCE INCLUDING StrawIndex.hh
  
  // --- Configuration
  struct Config {
    Atom<bool>        debug { Name("debug"), false };
    Atom<std::string> mode  { Name("mode"), "TRK" };
  };

  // --- C'tor/d'tor:
  using Parameters = EDProducer::Table<Config>;
  explicit  Mu2eProducer( Parameters const & );
  virtual  ~Mu2eProducer()  { }

  // --- Production:
  virtual void produce( Event & );

private:
  bool  debug_;
  std::string mode_;

};  // Mu2eProducer

// ======================================================================

Mu2eProducer::
Mu2eProducer( Parameters const & config )
  : EDProducer( )
  , debug_    ( config().debug() )
  , mode_     ( config().mode() )
{
  produces<EventNumber_t>();
  if(mode_ != "TRK" && mode_ != "CAL" && mode_ != "CRV") {
    std::cout << "WARNING: Unknown mode \"" << mode_ << "\"" << " defaulting to mode TRK" << std::endl;
    mode_ = "TRK";
  }
}

// ----------------------------------------------------------------------

void
  Mu2eProducer::
  produce( Event & event )
{

  art::EventNumber_t eventNumber = event.event();
  if( debug_ ) {
    TRACE( 11, "mu2e::Mu2eProducer::produce enter eventNumber=%d", eventNumber );
    std::cout << "mu2e::Mu2eProducer::produce enter eventNumber=" << (int)eventNumber << std::endl;
    
    std::cout << "SIM_MODE: " << mode_ << std::endl;

  }

  event.put(std::unique_ptr<EventNumber_t>(new EventNumber_t( eventNumber )));


  art::Handle<artdaq::Fragments> raw;
  event.getByLabel("daq", "MU2E", raw);
  if (raw.isValid()) {

    if( debug_ ) {
      std::cout << std::dec << "Producer: Run " << event.run() << ", subrun " << event.subRun()
		<< ", event " << eventNumber << " has " << raw->size()
		<< " mu2eFragmentReader(s)" << std::endl;
    }

    // Loop over fragments in the event
    for (size_t idx = 0; idx < raw->size(); ++idx) {
      const auto& fragment((*raw)[idx]);
      
      mu2e::mu2eFragmentReader cc(fragment);
          
      if (fragment.hasMetadata()) {

	mu2e::mu2eFragmentReader::Metadata const* md = fragment.metadata<mu2e::mu2eFragmentReader::Metadata>();

	if( debug_ ) {
	  std::cout << std::endl;
	  std::cout << "mu2eFragmentReader SimMode: ";
	  std::cout << DTCLib::DTC_SimModeConverter((DTCLib::DTC_SimMode)(md->sim_mode)).toString()
		    << std::endl;
	  std::cout << "Block Count: " << std::dec << cc.hdr_block_count() << std::endl;
	  std::cout << "Fragment Type: " << cc.hdr_fragment_type() << std::endl;
	  std::cout << "Block Size in Bytes: " << cc.blockSizeBytes() << std::endl;
	  std::cout << std::endl;

	  std::cout << "\t" << "====== Block Sizes ======" << std::endl;
	  for(size_t i=0; i<10; i++) {
	    std::cout << "\t" << i << "\t" << cc.blockIndexBytes(i) << "\t" << cc.blockSizeBytes(i) << std::endl;
	  }
	  std::cout << "\t" << "=========================" << std::endl;
	}

	for(size_t curBlockIdx=0; curBlockIdx<cc.hdr_block_count(); curBlockIdx++) {

	  size_t blockStartBytes = cc.blockIndexBytes(curBlockIdx);
	  size_t blockEndBytes = cc.blockEndBytes(curBlockIdx);

	  if( debug_ ) {
	    std::cout << "BLOCKSTARTEND: " << blockStartBytes << " " << blockEndBytes << " " << cc.blockSizeBytes(curBlockIdx)<< std::endl;
	    std::cout << "IndexComparison: " << cc.blockIndexBytes(0)+16*(0+3*curBlockIdx) << "\t";
	    std::cout                        << cc.blockIndexBytes(curBlockIdx)+16*(0+3*0) << std::endl;
	  }

	  size_t curPosBytes = blockStartBytes;
	  while(curPosBytes<blockEndBytes) {

	    adc_t const *pos = reinterpret_cast<adc_t const *>(cc.dataAtBytes(curPosBytes));

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

	    adc_t IDline = (adc_t) *(pos+1);
	    adc_t rocID = IDline & 15; // 15 = 0x1111
	    adc_t ringID = (IDline >> 8) & 7; // 7 = 0x111 
	    adc_t valid = IDline >> 15;
	    adc_t packetCount = (adc_t) *(pos+2);
	    
	    uint32_t timestampLow    = (adc_t) *(pos+3);
	    uint32_t timestampMedium = (adc_t) *(pos+4);
	    size_t timestamp = timestampLow | (timestampMedium<<16);

	    adc_t EVBMode_andSYSID_andDTCID = (adc_t) *(pos+7);
	    
	    adc_t EVBMode = (EVBMode_andSYSID_andDTCID >> 8);
	    adc_t sysID = (EVBMode_andSYSID_andDTCID >> 6) & 0x0003;
	    adc_t dtcID = EVBMode_andSYSID_andDTCID & 0x003F;

	    if(sysID==0) {
	      mode_ = "TRK";
	    } else if(sysID==1) {
	      mode_ = "CAL";
	    }

	    // Parse phyiscs information from TRK packets
	    if(mode_ == "TRK" && packetCount>0) {

	      adc_t strawIdx              = (adc_t) *(pos+8+0);
	      uint32_t TDC0_low           = (adc_t) *(pos+8+1);
	      uint32_t TDC0_high_TDC1_low = (adc_t) *(pos+8+2);
	      uint32_t TDC1_high          = (adc_t) *(pos+8+3);
	      
	      uint32_t TDC0_high = TDC0_high_TDC1_low & 255;
	      uint32_t TDC1_low = TDC0_high_TDC1_low >> 8;
	      
	      uint32_t TDC0 = (TDC0_high << 16) | TDC0_low;
	      uint32_t TDC1 = (TDC1_high << 8) | TDC1_low;
	      
	      ADCWaveform waveform;
	      for(size_t i=0; i<12; i++) {
		waveform.push_back((adc_t) *(pos+8+4+i));
	      }
	      
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
		
		std::cout << "LOOP: " << eventNumber << " " << curBlockIdx << " " << (curPosBytes-blockStartBytes)/48 << "/" << (blockEndBytes-blockStartBytes)/48-1 << " " << "(" << timestamp << ")" << std::endl;	    
		
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

	      adc_t ID_Entry              = (adc_t) *(pos+8+0);
	      adc_t crystalID = ID_Entry & 4095; // 4095 = 0b0000111111111111
	      adc_t apdID = ID_Entry >> 12;

	      adc_t time = (adc_t) *(pos+8+1);
	      adc_t numSamples = (adc_t) *(pos+8+2);
	      
	      ADCWaveform waveform;
	      for(size_t i=0; i<numSamples; i++) {
		waveform.push_back((adc_t) *(pos+8+3+i));
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
	      
	    // Increment position by 16 bytes times the total number of packets in the DataBlock
	    curPosBytes += (packetCount+1) * 16; 

	  } // End loop over DataBlocks within larger block
	  
	} // End loop over 2500 timestamps

      } // Close hasmetadata loop
      
    } // Close loop over fragments
        
  }

}  // produce()

// ======================================================================

DEFINE_ART_MODULE(Mu2eProducer)

// ======================================================================
