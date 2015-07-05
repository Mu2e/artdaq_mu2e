#include "mu2e-artdaq/Generators/OverlayTest.hh"

#include "art/Utilities/Exception.h"
#include "artdaq/Application/GeneratorMacros.hh"
#include "cetlib/exception.h"

#include "mu2e-artdaq-core/Overlays/DetectorFragment.hh"
#include "mu2e-artdaq-core/Overlays/TrackerFragmentWriter.hh"
#include "mu2e-artdaq-core/Overlays/CalorimeterFragmentWriter.hh"
#include "mu2e-artdaq-core/Overlays/CosmicVetoFragmentWriter.hh"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"

#include "fhiclcpp/ParameterSet.h"
#include "artdaq-core/Utilities/SimpleLookupPolicy.h"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <iostream>

#include <unistd.h>


// The adc size is fixed to 16 bits. This variable is a vestige from
// ToySimulator and probably won't be used in later versions

namespace {
  size_t typeToADC(mu2e::FragmentType type)
  {
    switch (type) {
    case mu2e::FragmentType::TRK:
      return 16;
      break;
    case mu2e::FragmentType::CAL:
      return 16;
      break;
    case mu2e::FragmentType::CRV:
      return 16;
      break;
    default:
      throw art::Exception(art::errors::Configuration)
        << "Unknown board type "
        << type
        << " ("
        << mu2e::fragmentTypeToString(type)
        << ").\n";
    };
  }

}



mu2e::OverlayTest::OverlayTest(fhicl::ParameterSet const & ps) :
  CommandableFragmentGenerator(ps),
  nADCcounts_(ps.get<size_t>("nADCcounts", 600000)), // No longer used as adc counts are based on data read from the DTC simulator
  fragment_type_(toFragmentType(ps.get<std::string>("fragment_type"))),
  throttle_usecs_(ps.get<size_t>("throttle_usecs", 0)),
  dataIdx(0),
  data_packets_read_(0),
  events_read_(0),
  isSimulatedDTC(false)
{
    std::vector<artdaq::Fragment::type_t> const ftypes = 
    {FragmentType::TOY1, FragmentType::TOY2 , FragmentType::TRK, FragmentType::CAL, FragmentType::CRV};

  if (std::find( ftypes.begin(), ftypes.end(), fragment_type_) == ftypes.end() ) {
    throw cet::exception("Error in OverlayTest: unexpected fragment type supplied to constructor");
  }

  std::cout << "OVERLAYTEST" << std::endl;

  std::cout << "DTC FRAGMENT TYPE: ";
  switch (fragment_type_) {
    case mu2e::FragmentType::TRK:
      std::cout << "TRK";
      break;
    case mu2e::FragmentType::CAL:
      std::cout << "CAL";
      break;
    case mu2e::FragmentType::CRV:
      std::cout << "CRV";
      break;
    default:
      std::cout << "UNKNOWN";
  };
  std::cout << std::endl;
  
  switch (fragment_type_) {
    case mu2e::FragmentType::TRK:
      theInterface = new DTCLib::DTC(DTCLib::DTC_SimMode_Tracker);
      break;
    case mu2e::FragmentType::CAL:
      theInterface = new DTCLib::DTC(DTCLib::DTC_SimMode_Calorimeter);
      break;
    case mu2e::FragmentType::CRV:
      theInterface = new DTCLib::DTC(DTCLib::DTC_SimMode_CosmicVeto);
      break;
    default:
      theInterface = new DTCLib::DTC();
  };

  int ringRocs[] = {
    ps.get<int>("ring_0_roc_count",-1),
    ps.get<int>("ring_1_roc_count",-1),
    ps.get<int>("ring_2_roc_count",-1),
    ps.get<int>("ring_3_roc_count",-1),
    ps.get<int>("ring_4_roc_count",-1),
    ps.get<int>("ring_5_roc_count",-1)
  };

  bool ringTiming[] = {
    ps.get<bool>("ring_0_timing_enabled",true),
    ps.get<bool>("ring_1_timing_enabled",true),
    ps.get<bool>("ring_2_timing_enabled",true),
    ps.get<bool>("ring_3_timing_enabled",true),
    ps.get<bool>("ring_4_timing_enabled",true),
    ps.get<bool>("ring_5_timing_enabled",true)
  };

  bool ringEmulators[] = {
    ps.get<bool>("ring_0_roc_emulator_enabled",false),
    ps.get<bool>("ring_1_roc_emulator_enabled",false),
    ps.get<bool>("ring_2_roc_emulator_enabled",false),
    ps.get<bool>("ring_3_roc_emulator_enabled",false),
    ps.get<bool>("ring_4_roc_emulator_enabled",false),
    ps.get<bool>("ring_5_roc_emulator_enabled",false)
  };

  for(int ring = 0; ring < 6; ++ring) {  
    if(ringRocs[ring] >= 0) {
      theInterface->EnableRing(DTCLib::DTC_Rings[ring],
                                DTCLib::DTC_RingEnableMode(true,true,ringTiming[ring]), 
                                DTCLib::DTC_ROCS[ringRocs[ring]]);
      if(ringEmulators[ring]) {
	theInterface->EnableROCEmulator(DTCLib::DTC_Rings[ring]); 
      } else {
	theInterface->DisableROCEmulator(DTCLib::DTC_Rings[ring]);
      }
    }
  }

  //  theInterface->SetMaxROCNumber(DTCLib::DTC_Ring_0, DTCLib::DTC_ROC_5);
  //  theInterface->EnableRing(DTCLib::DTC_Ring_1, DTCLib::DTC_ROC_5);
  
}

mu2e::OverlayTest::~OverlayTest()
{
  delete theInterface;
}

artdaq::Fragment::fragment_id_t mu2e::OverlayTest::generateFragmentID(DTCLib::DTC_DataHeaderPacket &thePacket) {

  DTCLib::DTC_DataPacket dataPacket = thePacket.ConvertToDataPacket();
  
  std::bitset<128> theArray;
  for(int bitIdx=127, byteIdx = 0; byteIdx<16; byteIdx++) {
    for(int offset = 0; offset<8; offset++) {
      if((dataPacket.GetWord(byteIdx) & (1<<offset)) != 0) {
	theArray.set(bitIdx);
      } else {
	theArray.reset(bitIdx);
      }
      bitIdx--;
    }
  }

  std::bitset<8> fragID;

  // Ring ID occupies the 4 most significant bits
  for(int i=(127-28)+1; i<=(127-24); i++) {
    fragID.set(4+(127-24)-i,theArray[i]);
  }
  // ROC ID occupies the 4 least significant bits
  for(int i=(127-20)+1; i<=(127-16); i++) {
    fragID.set((127-16)-i,theArray[i]);
  }
  
  return fragID.to_ulong();
}


bool mu2e::OverlayTest::getNext_(artdaq::FragmentPtrs & frags) {

  std::cout << "==================== BEGINNING OF EVENT ====================" << std::endl;

  if (should_stop()) {
    return false;
  }

  isSimulatedDTC = theInterface->ReadSimMode();
  if(isSimulatedDTC != DTCLib::DTC_SimMode_Disabled && isSimulatedDTC != DTCLib::DTC_SimMode_Hardware) {
    std::cout << "ALERT: USING SIMULATED DTC" << std::endl;
  } else if(isSimulatedDTC == DTCLib::DTC_SimMode_Hardware) {
    std::cout << "ALERT: USING ACTUAL DTC IN LOOPBACK MODE" << std::endl;
  } else {
    std::cout << "ALERT: USING ACTUAL DTC" << std::endl;
  }

  while(data.empty()){
    // std::vector<void*> DTCLib::DTC::GetData(DTC_Timestamp when, bool sendDReq, bool sendRReq)
    data = theInterface->GetData((uint64_t)0);    
    dataIdx = 0;
  }

  usleep( throttle_usecs_ );

  int totalNumPackets = 0;
  
  std::cout << "Determining total number of packets in payload: " << std::endl;
  for(size_t curDataIdx=0; curDataIdx<data.size(); curDataIdx++) {
    DTCLib::DTC_DataHeaderPacket packet = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[dataIdx]));    
    totalNumPackets += packet.GetPacketCount()+1;
    //    DTCLib::DTC_PacketType theType = packet.GetPacketType();
    //    if(theType == DTCLib::DTC_PacketType::DTC_PacketType_DataHeader) {
    //      std::cout << "\t" << "HEADER" << std::endl;
    //    } else {
    //      std::cout << "\t" << "OTHER" << std::endl;
    //    }
  }
  //  std::cout << "DEBUG: Number of data blocks from data.size(): " << data.size() << std::endl;

  // The fragment payload needs to be large enough to hold the 64*2 bit DTC header
  // packet and all the data from the associated data packets (each is 128 bits long)
  int payloadSize; // Measured in sizeof(adc_t)=16 bit blocks
  // The typeToADC is 16 bits so the number of 16 bit blocks
  // in the payload is 8*(totalNumPackets)
  payloadSize = 128*(totalNumPackets) / typeToADC(fragment_type_);

  // Generate the appropriate fragment ID based on the ROC and ring numbers
  //  artdaq::Fragment::fragment_id_t curFragID = generateFragmentID(packet);
  artdaq::Fragment::fragment_id_t curFragID = 0;
  std::cout << "FragmentID: " << curFragID << std::endl;

  // If this is the first event, we need to add each fragment ID to the list
  // of all fragment IDs handled by this particular fragment generator.
  if(events_read_==0) {
    fragment_ids_.push_back(curFragID);
  }

  // Set fragment's metadata
  mu2e::DetectorFragment::Metadata metadata;
  // The board_serial_number is being hardcoded to 999 for now, but this
  // will eventually need to be changed (DTC board ID?).
  metadata.board_serial_number = 999;
  metadata.num_adc_bits = typeToADC(fragment_type_); // Fixed to 16 bits
  
  // Use the metadata, along with the artdaq::Fragment header information
  // (fragment id, sequence id, and user type) to create a fragment:
  
  // Constructor used is:   
  // Fragment(std::size_t payload_size, sequence_id_t sequence_id,
  //  fragment_id_t fragment_id, type_t type, const T & metadata);
  // ...where we'll start off setting the payload (data after the
  // header and metadata) to empty; this will be resized below
  
  frags.emplace_back( new artdaq::Fragment(0, ev_counter(), curFragID, fragment_type_, metadata) );

  // Use detector-specific FragmentWriters:
  DetectorFragmentWriter* newfrag;
  switch (fragment_type_) {
    case mu2e::FragmentType::TRK:
      newfrag = new TrackerFragmentWriter(*frags.back());
      break;
    case mu2e::FragmentType::CAL:
      newfrag = new CalorimeterFragmentWriter(*frags.back());
      break;
    case mu2e::FragmentType::CRV:
      newfrag = new CosmicVetoFragmentWriter(*frags.back());
      break;
    default:
      newfrag = new TrackerFragmentWriter(*frags.back());
  };
  
  // Currently hardcoding hdr_run_number to 999, but this will
  // need to be changed.
  newfrag->set_hdr_run_number(999);

  newfrag->resize(payloadSize);

  size_t numPacketsRecorded = 0;
  while(!data.empty()) {

    DTCLib::DTC_DataHeaderPacket packet = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[dataIdx]));

    std::cout << "================ DATA INDEX " << dataIdx << "/" << data.size()-1 << " (" << packet.GetPacketCount()+1 << " packets total) ================" << std::endl;

    //    std::cout << "DEBUG: numDataBlocks()=" << newfrag->numDataBlocks() << std::endl;

    std::cout << "Dumping DataHeaderPacket: " << std::endl;

    std::cout << packet.toJSON() << std::endl;

    // The packet count does not include the header packet
    std::cout << "\t" << "PacketCount: " << packet.GetPacketCount() << std::endl;

    if(packet.GetPacketCount()>0) {
      std::cout << "\t" << "DumpingDataPackets: " << std::endl;
    }

    // Take pairs of 8-bit words from the DTC header and data packets, combine them,
    // and store them as 16-bit adc_t values in the fragment:
    for(int packetNum = 0; packetNum<packet.GetPacketCount()+1; packetNum++) {
      DTCLib::DTC_DataPacket curPacket((char*)data[dataIdx] + 16*packetNum);

      // Print the words being written to the adc_t array:
      std::cout << "PACKETNUM: " << packetNum << std::endl;

      std::cout << "\tPacket " << packetNum << " words: ";
      for(int wordNum=15; wordNum>=0; wordNum--) {
	uint8_t curWord = curPacket.GetWord(wordNum);

	for(int offset = 7; offset>=0; offset--) {
	  if((curWord & (1<<offset)) != 0) {
	    std::cout << 1;
	  } else {
	    std::cout << 0;
	  }
	}

	std::cout  << " ";
	if(wordNum%2==0) {
	  uint8_t firstWord = curPacket.GetWord(wordNum);
	  uint8_t secondWord = curPacket.GetWord(wordNum+1);
	  uint16_t combined = ((secondWord << 8) | (firstWord));
	  std::cout << "(" << (int)combined << ") ";
	}
      }
      std::cout << std::endl;
      
      for(int wordNum=0; wordNum<16; wordNum+=2) { // 16 words per DTC packet
	int fragPos = ((numPacketsRecorded+packetNum)*16+wordNum )/2; // The index of the current 16 bit adc_t value in the fragment
	if(wordNum%2==0) { // Not strictly necessary since we increment by 2
	  uint8_t firstWord = curPacket.GetWord(wordNum);
	  uint8_t secondWord = curPacket.GetWord(wordNum+1);
	  uint16_t combined = ((secondWord << 8) | (firstWord));
	  *(newfrag->dataBegin()+fragPos) = combined; // Set the appropriate adc_t entry in the fragment to the combined uint16_t
	}
      }
      data_packets_read_++;
    }
    numPacketsRecorded += packet.GetPacketCount()+1;

    // Check and make sure that no ADC values in this fragment are
    // larger than the max allowed
    newfrag->fastVerify( metadata.num_adc_bits );

    dataIdx++;
    if(dataIdx>=data.size()) {
      data.clear();
      dataIdx=0;
    }
  }

  std::cout << "GREPME: Checking number of stored datablocks: " << newfrag->numDataBlocks() << std::endl;
    
  for(size_t i=0; i<newfrag->numDataBlocks(); i++) {
    newfrag->setDataBlockIndex(i);
    std::cout << "\t************************************************" << std::endl;
    std::cout << "\toffset " << newfrag->offsetIndex() << ": " << newfrag->offset() << std::endl;
    std::cout << "\t************************************************" << std::endl;
    std::cout << "Calling printDTCHeader() on new fragment..."            << std::endl;
    std::cout << "************************************************" << std::endl;
    newfrag->printDTCHeader();
    std::cout << "************************************************" << std::endl;
    std::cout << std::endl << std::endl;
    std::cout << "************************************************" << std::endl;
    std::cout << "Calling printAll() on new fragment..."            << std::endl;
    std::cout << "************************************************" << std::endl;
    switch (fragment_type_) {
      case mu2e::FragmentType::TRK:
        (dynamic_cast<TrackerFragmentWriter*>(newfrag))->printAll();      
        break;
      case mu2e::FragmentType::CAL:
        (dynamic_cast<CalorimeterFragmentWriter*>(newfrag))->printAll();
        break;
      case mu2e::FragmentType::CRV:
        (dynamic_cast<CosmicVetoFragmentWriter*>(newfrag))->printAll();
        break;
      default:
        (dynamic_cast<TrackerFragmentWriter*>(newfrag))->printAll();
      };
    std::cout << "************************************************" << std::endl;
  }
  newfrag->setDataBlockIndex(0);

  delete newfrag;

  // Debug output to check the list of all fragment IDs handled by this
  // fragment generator
  std::cout << "Printing Fragment IDs: " << std::endl;
  for(size_t i=0; i<fragmentIDs_().size(); i++) {
    std::cout << fragmentIDs_()[i] << "\t";
  }
  std::cout << std::endl;

  ev_counter_inc();
  events_read_++;

  std::cout << "==================== END OF EVENT ====================" << std::endl;

  return true;
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::OverlayTest) 
