#include "mu2e-artdaq/Generators/Mu2eReceiver.hh"

#include "art/Utilities/Exception.h"
#include "artdaq/Application/GeneratorMacros.hh"
#include "cetlib/exception.h"
#include "mu2e-artdaq-core/Overlays/DTCFragment.hh"
#include "mu2e-artdaq-core/Overlays/DTCFragmentWriter.hh"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"
#include "mu2e-artdaq-core/Overlays/mu2eFragmentWriter.hh"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "fhiclcpp/ParameterSet.h"
#include "artdaq-core/Utilities/SimpleLookupPolicy.h"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <iostream>

#include <unistd.h>
#include "trace.h"

#define TRACE_NAME "MU2EDEV"

mu2e::Mu2eReceiver::Mu2eReceiver(fhicl::ParameterSet const & ps)
	: CommandableFragmentGenerator(ps)
	, fragment_type_(toFragmentType("MU2E"))
	, fragment_ids_{ static_cast<artdaq::Fragment::fragment_id_t>(fragment_id()) }
	, fragments_read_(0)
	, mode_(DTCLib::DTC_SimModeConverter::ConvertToSimMode(ps.get<std::string>("sim_mode", "Disabled")))
	, board_id_(static_cast<uint8_t>(ps.get<int>("board_id", 0)))
{
  TRACE(1, "Mu2eReceiver_generator CONSTRUCTOR");
	// mode_ can still be overridden by environment!
	theInterface_ = new DTCLib::DTC(mode_);
	theCFO_ = new DTCLib::DTCSoftwareCFO(theInterface_, true);
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

	for (int ring = 0; ring < 6; ++ring)
	  {
	    if (ringRocs[ring] >= 0)
	      {
		theInterface_->EnableRing(DTCLib::DTC_Rings[ring],
					  DTCLib::DTC_RingEnableMode(true, true, ringTiming[ring]),
					  DTCLib::DTC_ROCS[ringRocs[ring]]);
		if (ringEmulators[ring]) { theInterface_->EnableROCEmulator(DTCLib::DTC_Rings[ring]); }
		else { theInterface_->DisableROCEmulator(DTCLib::DTC_Rings[ring]); }
	      }
	  }
}

mu2e::Mu2eReceiver::~Mu2eReceiver()
{
	delete theInterface_;
	delete theCFO_;
}

bool mu2e::Mu2eReceiver::getNext_(artdaq::FragmentPtrs & frags)
{
  if (should_stop())
	{
	  return false;
	}

  TRACE(1, "mu2eReceiver::getNext: Initializing mu2eFragment metadata");
  mu2eFragment::Metadata metadata;
  metadata.sim_mode = static_cast<int>(mode_);
  metadata.run_number = run_number();
  metadata.board_id = board_id_;

  TRACE(3, "mu2eReceiver::getNext allocating DTCFragment metadata");
  DTCFragment::Metadata dtcmetadata;
  metadata.sim_mode = static_cast<int>(mode_);
  metadata.run_number = run_number();
  metadata.board_id = board_id_;

  // And use it, along with the artdaq::Fragment header information
  // (fragment id, sequence id, and user type) to create a fragment
  TRACE(1, "mu2eReceiver::getNext: Creating new mu2eFragment!");
  frags.emplace_back(new artdaq::Fragment(0, ev_counter(), fragment_ids_[0],
										  fragment_type_, metadata));

  // Now we make an instance of the overlay to put the data into...
  TRACE(1, "mu2eReceiver::getNext: Making mu2eFragmentWriter");
  mu2eFragmentWriter newfrag(*frags.back());
  newfrag.set_hdr_fragment_type(toFragmentType("DTC"));
       
  //Get data from DTCReceiver
  TRACE(1, "mu2eReceiver::getNext: Starting DTCFragment Loop");
  long subEvtId = ev_counter() * mu2e::DTC_FRAGMENT_MAX;
  uint64_t z = 0;
  DTCLib::DTC_Timestamp zero(z);
  if (mode_ != 0) { theCFO_->SendRequestsForRange(mu2e::DTC_FRAGMENT_MAX, DTCLib::DTC_Timestamp(ev_counter())); }

  while(newfrag.subEvtCount() < mu2e::DTC_FRAGMENT_MAX) 
	{
	  if(should_stop()) { break; }

	  TRACE(1, "Getting DTC Data");
	  std::vector<void*> data;
	  int retryCount = 5;
	  while (data.size() == 0 && retryCount >= 0)
		{
		  try
			{
			  data = theInterface_->GetData(zero);
			}
		  catch (std::exception ex)
			{
			  std::cerr << ex.what() << std::endl;
			}
		  retryCount--;
		}
	  if (retryCount < 0 && data.size() == 0) { return false; }

	  auto first = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[0]));
	  DTCLib::DTC_Timestamp ts = first.GetTimestamp();
	  int packetCount = first.GetPacketCount() + 1;
	  TRACE(1, "There are %lu data blocks in timestamp %lu. Packet count of first data block: %i", data.size(), ts.GetTimestamp(true), packetCount);

	  for (size_t i = 1; i < data.size(); ++i)
		{
		  auto packet = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[i]));
		  packetCount += packet.GetPacketCount() + 1;
		}


	  TRACE(1, "mu2eReceiver::getNext Allocating new DTCFragment");
	  auto frag = new artdaq::Fragment(0, subEvtId, fragment_ids_[0],fragment_type_, dtcmetadata);
      auto fragSize = frag->size() * sizeof(artdaq::Fragment::value_type);
      auto dataSize = packetCount * sizeof(packet_t);
      TRACE(1, "mu2eReceiver::getNext Resizing mu2eFragment, adding %lu + %lu = %lu", fragSize, dataSize, fragSize + dataSize );
      newfrag.addSpace(fragSize + dataSize );
      TRACE(1, "mu2eReciever::getNext setting pointer for DTCFragment");
	  artdaq::Fragment* dtcFrag = newfrag.dataEnd();
      TRACE(1, "mu2eReceiver::getNext copying fragment header information into mu2eFragment at %p", (void*)dtcFrag);
      memcpy(dtcFrag, frag, fragSize);

	  // Now we make an instance of the overlay to put the data into...
	  DTCFragmentWriter dtcfragwriter(*dtcFrag);

	  dtcfragwriter.set_hdr_timestamp(ts.GetTimestamp(true));
	  TRACE(3, "DTC Response for timestamp %lu includes %i packets.", ts.GetTimestamp(true), packetCount);
	  dtcfragwriter.resize(packetCount);

      TRACE(3, "Copying DTC packets into DTCFragment");
	  size_t packetsProcessed = 0;
	  for (size_t i = 0; i < data.size(); ++i)
		{
		  TRACE(3, "Creating packet object to determine data block size: i=%lu, data=%p", i, data[i]);
		  auto packet = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[i]));
          TRACE(3, "Copying packet %lu. src=%p, dst=%p, sz=%lu",i,  data[i],(void*)(dtcfragwriter.dataBegin() + packetsProcessed),(1 + packet.GetPacketCount())*sizeof(packet_t));
		  memcpy((void*)(dtcfragwriter.dataBegin() + packetsProcessed), data[i], (1 + packet.GetPacketCount())*sizeof(packet_t));
          TRACE(3, "Incrementing packet counter");
		  packetsProcessed += 1 + packet.GetPacketCount();
		}

      TRACE(3, "Ending SubEvt");
	  newfrag.endSubEvt(dtcFrag->size() * sizeof(artdaq::Fragment::value_type));

	  ++subEvtId;
	}

  TRACE(1, "Incrementing event counter");
  ev_counter_inc();

  TRACE(1, "Returning true");
  return true;
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::Mu2eReceiver)
