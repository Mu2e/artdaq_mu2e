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
	, timestamps_read_(0)
    , lastReportTime_(0)
    , hwStartTime_(0)
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

  TRACE(1, "mu2eReceiver::getNext: Starting CFO thread");
  uint64_t z = 0;
  DTCLib::DTC_Timestamp zero(z);
  if (mode_ != 0) { 
	TRACE(1, "Sending requests for %i timestamps, starting at %lu", mu2e::BLOCK_COUNT_MAX, mu2e::BLOCK_COUNT_MAX * ev_counter());
	theCFO_->SendRequestsForRange(mu2e::BLOCK_COUNT_MAX, DTCLib::DTC_Timestamp(mu2e::BLOCK_COUNT_MAX * ev_counter())); 
  }

  TRACE(1, "mu2eReceiver::getNext: Initializing mu2eFragment metadata");
  mu2eFragment::Metadata metadata;
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

  TRACE(1, "mu2eReceiver::getNext: Reserving space for 2 * BLOCK_COUNT_MAX packets");
  newfrag.addSpace(mu2e::BLOCK_COUNT_MAX * 2 * sizeof(packet_t));
       
  //Get data from DTCReceiver
  TRACE(1, "mu2eReceiver::getNext: Starting DTCFragment Loop");
  struct tms ctime;
  hwStartTime_ = times(&ctime);
  while(newfrag.hdr_block_count() < mu2e::BLOCK_COUNT_MAX) 
	{
	  if(should_stop()) { break; }

	  TRACE(1, "Getting DTC Data");
	  std::vector<void*> data;
	  int retryCount = 5;
	  while (data.size() == 0 && retryCount >= 0)
		{
		  try
			{
			  TRACE(4, "Calling theInterface->GetData(zero)" );
			  data = theInterface_->GetData(zero);
			  TRACE(4, "Done calling theInterface->GetData(zero)");
			}
		  catch (std::exception ex)
			{
			  std::cerr << ex.what() << std::endl;
			}
		  retryCount--;
		}
	  if (retryCount < 0 && data.size() == 0) { 
		TRACE(1, "Retry count exceeded. Something is very wrong indeed");
		return false; 
	  }

	  auto first = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[0]));
	  DTCLib::DTC_Timestamp ts = first.GetTimestamp();
	  int packetCount = first.GetPacketCount() + 1;
	  TRACE(1, "There are %lu data blocks in timestamp %lu. Packet count of first data block: %i", data.size(), ts.GetTimestamp(true), packetCount);

	  for (size_t i = 1; i < data.size(); ++i)
		{
		  auto packet = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[i]));
		  packetCount += packet.GetPacketCount() + 1;
		}

      auto dataSize = packetCount * sizeof(packet_t);
      int64_t diff = dataSize + newfrag.dataSize() - (newfrag.fragSize() * sizeof(artdaq::Fragment::value_type));
      if(diff > 0) {
		TRACE(1, "mu2eReceiver::getNext: %lu + %lu > %lu, allocating space for 1%% BLOCK_COUNT_MAX more packets", dataSize, newfrag.dataSize(), newfrag.fragSize() * sizeof(artdaq::Fragment::value_type));
		newfrag.addSpace(diff + (mu2e::BLOCK_COUNT_MAX / 100) * sizeof(packet_t)); 
	  }      
      
      TRACE(3, "Copying DTC packets into Mu2eFragment");
	  size_t packetsProcessed = 0;
      packet_t* offset = reinterpret_cast<packet_t*>((uint8_t*)newfrag.dataBegin() + newfrag.dataSize());
	  for (size_t i = 0; i < data.size(); ++i)
		{
		  TRACE(3, "Creating packet object to determine data block size: i=%lu, data=%p", i, data[i]);
		  auto packet = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[i]));
          TRACE(3, "Copying packet %lu. src=%p, dst=%p, sz=%lu", i, data[i],(void*)(offset + packetsProcessed),(1 + packet.GetPacketCount())*sizeof(packet_t));
		  memcpy((void*)(offset + packetsProcessed), data[i], (1 + packet.GetPacketCount())*sizeof(packet_t));
          TRACE(3, "Incrementing packet counter");
		  packetsProcessed += 1 + packet.GetPacketCount();
		}

      TRACE(3, "Ending SubEvt");
	  newfrag.endSubEvt( packetsProcessed * sizeof(packet_t) );
	}

  TRACE(1, "Incrementing event counter");
  ev_counter_inc();

  TRACE(1, "Reporting Metrics");
  timestamps_read_ += newfrag.hdr_block_count();
  double timestamp_rate = newfrag.hdr_block_count() / _timeSinceLastSend( );
  double hw_timestamp_rate = newfrag.hdr_block_count() / _timeSinceHWStart( );

  metricMan_->sendMetric("Timestamp Count", timestamps_read_, "timestamps", 1, true, false);
  metricMan_->sendMetric("Timestamp Rate", timestamp_rate, "timestamps/s", 1);
  metricMan_->sendMetric("HW Timestamp Rate", hw_timestamp_rate, "timestamps/s", 1);
  

  TRACE(1, "Returning true");
  return true;
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::Mu2eReceiver)
