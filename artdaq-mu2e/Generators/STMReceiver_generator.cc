#include "artdaq-mu2e/Generators/STMReceiver.hh"

#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"
#include "artdaq/Generators/GeneratorMacros.hh"
#include "canvas/Utilities/Exception.h"
#include "cetlib_except/exception.h"
#include "dtcInterfaceLib/DTC_Types.h"
#include "fhiclcpp/ParameterSet.h"
#include "artdaq-core-mu2e/Overlays/FragmentType.hh"
#include "artdaq/DAQdata/Globals.hh"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>

#include <unistd.h>
#include "trace.h"
#define TRACE_NAME "STMReceiver"

mu2e::STMReceiver::STMReceiver(fhicl::ParameterSet const& ps)
	: CommandableFragmentGenerator(ps)
	, fragment_type_(toFragmentType("STM"))
	, fragment_ids_{static_cast<artdaq::Fragment::fragment_id_t>(fragment_id())}
	, board_id_(static_cast<uint8_t>(ps.get<int>("board_id", 0)))
        , fromInputFile_(ps.get<bool>("from_input_file", false))
	, rawOutput_(ps.get<bool>("raw_output_enable", false))
	, rawOutputFile_(ps.get<std::string>("raw_output_file", "/tmp/Mu2eReceiver.bin"))
{

	if (rawOutput_) { rawOutputStream_.open(rawOutputFile_, std::ios::out | std::ios::binary); }

	if (fromInputFile_) {
	  auto input_file = ps.get<std::string>("input_file", "");
	  inputFileStream_.open(input_file, std::ios::in | std::ios::binary); 
	}
}


mu2e::STMReceiver::~STMReceiver()
{
        inputFileStream_.close();
	rawOutputStream_.close();
}

void mu2e::STMReceiver::stop()
{
}

bool mu2e::STMReceiver::getNext_(artdaq::FragmentPtrs& frags)
{
	while (!fromInputFile_ && !should_stop())
	{
		usleep(5000);
	}

	if (should_stop())
	{
		return false;
	}

	STMFragment::STMDataPacket data[1];
	if (fromInputFile_) {
	  TLOG(TLVL_DEBUG) << "Reading input file...";
	  inputFileStream_.read(reinterpret_cast<char *>(&data), sizeof(STMFragment::STMDataPacket));
	  TLOG(TLVL_DEBUG) << std::hex << "data = " << data;
	  TLOG(TLVL_DEBUG) << "Creating STMFragment...";
	  double fragment_timestamp = 0;
	  frags.emplace_back(new artdaq::Fragment(ev_counter(), fragment_ids_[0], fragment_type_, fragment_timestamp));
	  uint16_t* dataBegin = reinterpret_cast<uint16_t*>(frags.back()->dataBegin());
	  for (auto& evt : data) {
	    TLOG(TLVL_DEBUG) << std::hex << "evt = " << evt;
	    memcpy(dataBegin, &evt, sizeof(evt));
	    dataBegin += sizeof(evt);
	  }
	}

	if (rawOutput_)	{
	  for (auto& evt : data) {
	    TLOG(TLVL_DEBUG) << "Writing to raw output...";
	    TLOG(TLVL_DEBUG) << std::hex << "evt = " << evt;
	    rawOutputStream_.write(reinterpret_cast<char*>(&evt), sizeof(STMFragment::STMDataPacket));
	  }
	}

	TLOG(TLVL_DEBUG) << "Incrementing event counter, frags.size() is now " << frags.size();
	ev_counter_inc();


	TLOG(TLVL_TRACE + 20) << "Returning true";

	return true;
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::STMReceiver)
