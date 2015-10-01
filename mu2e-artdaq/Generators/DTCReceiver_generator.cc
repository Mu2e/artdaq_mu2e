#include "mu2e-artdaq/Generators/DTCReceiver.hh"

#include "art/Utilities/Exception.h"
#include "artdaq/Application/GeneratorMacros.hh"
#include "cetlib/exception.h"
#include "mu2e-artdaq-core/Overlays/DTCFragment.hh"
#include "mu2e-artdaq-core/Overlays/DTCFragmentWriter.hh"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "fhiclcpp/ParameterSet.h"
#include "artdaq-core/Utilities/SimpleLookupPolicy.h"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <iostream>

#include <unistd.h>


mu2e::DTCReceiver::DTCReceiver(fhicl::ParameterSet const & ps)
  : CommandableFragmentGenerator(ps)
  , fragment_type_(toFragmentType("DTC"))
  , fragment_ids_{ static_cast<artdaq::Fragment::fragment_id_t>(fragment_id() ) }
  , packets_read_(0)
  , mode_(DTCLib::DTC_SimModeConverter::ConvertToSimMode(ps.get<std::string>("sim_mode","Disabled")))
  , board_id_(static_cast<uint8_t>(ps.get<int>("board_id",0)))
  , print_packets_(ps.get<bool>("debug_print",false))
{
  // mode_ can still be overridden by environment!
  theInterface_ = new DTCLib::DTC(mode_);
  theCFO_ = new DTCLib::DTCSoftwareCFO(theInterface_,false);
  mode_ = theInterface_->ReadSimMode();

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

  for(int ring = 0; ring < 6; ++ring)
    {  
   if(ringRocs[ring] >= 0) 
     {
       theInterface_->EnableRing(DTCLib::DTC_Rings[ring],
                                DTCLib::DTC_RingEnableMode(true,true,ringTiming[ring]), 
                                DTCLib::DTC_ROCS[ringRocs[ring]]);
       if(ringEmulators[ring]) { theInterface_->EnableROCEmulator(DTCLib::DTC_Rings[ring]); }
       else                    { theInterface_->DisableROCEmulator(DTCLib::DTC_Rings[ring]); }
     }
    }
}

mu2e::DTCReceiver::~DTCReceiver()
{
  delete theInterface_;
  delete theCFO_;
}

bool mu2e::DTCReceiver::getNext_(artdaq::FragmentPtrs & frags) {

  if (should_stop()) {
    return false;
  }

  // Set fragment's metadata
  DTCFragment::Metadata metadata;
  metadata.sim_mode = static_cast<int>(mode_);
  metadata.run_number = run_number();
  metadata.board_id = board_id_;

  // And use it, along with the artdaq::Fragment header information
  // (fragment id, sequence id, and user type) to create a fragment

  // Constructor used is: 

  // Fragment(std::size_t payload_size, sequence_id_t sequence_id,
  //  fragment_id_t fragment_id, type_t type, const T & metadata);

  // ...where we'll start off setting the payload (data after the
  // header and metadata) to empty; this will be resized below

  frags.emplace_back( new artdaq::Fragment(0, ev_counter(), fragment_ids_[0],
					   fragment_type_, metadata) );


  // Now we make an instance of the overlay to put the data into...
  DTCFragmentWriter newfrag(*frags.back());

  std::vector<void*> data;

  if(mode_ != 0) { theCFO_->SendRequestForTimestamp(DTCLib::DTC_Timestamp(ev_counter()));  }

  while(data.size() == 0){ 
    try {
     data = theInterface_->GetData( (uint64_t)0 ); 
    }
    catch (std::exception ex) {
      std::cerr << ex.what() << std::endl;
    }
  }
     
  auto first = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[0]));
  DTCLib::DTC_Timestamp ts = first.GetTimestamp();
  int packetCount = first.GetPacketCount() + 1;
  if(print_packets_) {
    std::cout << first.toJSON() << std::endl;
    for(int ii = 0; ii < first.GetPacketCount(); ++ii) {
      std::cout << "\t" << DTCLib::DTC_DataPacket(((uint8_t*)data[0]) + ((ii + 1) * 16)).toJSON() << std::endl;
    }
  }

  for(size_t i = 1; i < data.size(); ++i)
    {
      auto packet = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[i]));
      packetCount += packet.GetPacketCount() + 1;
      if(print_packets_) {
	std::cout << packet.toJSON() << std::endl;
        for(int ii = 0; ii < packet.GetPacketCount(); ++ii) {
	  std::cout << "\t" << DTCLib::DTC_DataPacket(((uint8_t*)data[i]) + ((ii + 1) * 16)).toJSON() << std::endl;
	}
      }
    }

  newfrag.set_hdr_timestamp(ts.GetTimestamp(true));

  newfrag.resize(packetCount);

  size_t packetsProcessed = 0;
  for(size_t i = 0; i < data.size(); ++i)
    {
      auto packet = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[i]));
      memcpy((void*)(newfrag.dataBegin() + packetsProcessed), data[i],(1 + packet.GetPacketCount())*sizeof(packet_t));
      packetsProcessed += 1 + packet.GetPacketCount();
    }

  ev_counter_inc();

  return true;
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::DTCReceiver) 
