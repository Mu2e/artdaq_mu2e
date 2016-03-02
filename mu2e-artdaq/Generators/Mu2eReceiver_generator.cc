#include "mu2e-artdaq/Generators/Mu2eReceiver.hh"

#include "art/Utilities/Exception.h"
#include "artdaq/Application/GeneratorMacros.hh"
#include "cetlib/exception.h"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"
#include "mu2e-artdaq-core/Overlays/mu2eFragmentWriter.hh"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "fhiclcpp/ParameterSet.h"
#include "artdaq-core/Utilities/SimpleLookupPolicy.h"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <iostream>

#ifndef _WIN32
#include <unistd.h>
#include "trace.h"
#endif

#define TRACE_NAME "MU2EDEV"

mu2e::Mu2eReceiver::Mu2eReceiver(fhicl::ParameterSet const& ps)
	: CommandableFragmentGenerator(ps)
	  , fragment_type_(toFragmentType("MU2E"))
	  , fragment_ids_{static_cast<artdaq::Fragment::fragment_id_t>(fragment_id())}
	  , timestamps_read_(0)
	  , lastReportTime_(std::chrono::high_resolution_clock::now())
	  , mode_(DTCLib::DTC_SimModeConverter::ConvertToSimMode(ps.get<std::string>("sim_mode", "Disabled")))
	  , board_id_(static_cast<uint8_t>(ps.get<int>("board_id", 0)))
{
	TRACE(1, "Mu2eReceiver_generator CONSTRUCTOR");
	// mode_ can still be overridden by environment!
	theInterface_ = new DTCLib::DTC(mode_);
	theCFO_ = new DTCLib::DTCSoftwareCFO(theInterface_, true);
	mode_ = theInterface_->ReadSimMode();

	int ringRocs[] = {
		ps.get<int>("ring_0_roc_count", -1),
		ps.get<int>("ring_1_roc_count", -1),
		ps.get<int>("ring_2_roc_count", -1),
		ps.get<int>("ring_3_roc_count", -1),
		ps.get<int>("ring_4_roc_count", -1),
		ps.get<int>("ring_5_roc_count", -1)
	};

	bool ringTiming[] = {
		ps.get<bool>("ring_0_timing_enabled", true),
		ps.get<bool>("ring_1_timing_enabled", true),
		ps.get<bool>("ring_2_timing_enabled", true),
		ps.get<bool>("ring_3_timing_enabled", true),
		ps.get<bool>("ring_4_timing_enabled", true),
		ps.get<bool>("ring_5_timing_enabled", true)
	};

	bool ringEmulators[] = {
		ps.get<bool>("ring_0_roc_emulator_enabled", false),
		ps.get<bool>("ring_1_roc_emulator_enabled", false),
		ps.get<bool>("ring_2_roc_emulator_enabled", false),
		ps.get<bool>("ring_3_roc_emulator_enabled", false),
		ps.get<bool>("ring_4_roc_emulator_enabled", false),
		ps.get<bool>("ring_5_roc_emulator_enabled", false)
	};

	for (int ring = 0; ring < 6; ++ring)
	{
		if (ringRocs[ring] >= 0)
		{
			theInterface_->EnableRing(DTCLib::DTC_Rings[ring],
			                          DTCLib::DTC_RingEnableMode(true, true, ringTiming[ring]),
			                          DTCLib::DTC_ROCS[ringRocs[ring]]);
			if (ringEmulators[ring])
			{
				theInterface_->EnableROCEmulator(DTCLib::DTC_Rings[ring]);
			}
			else
			{
				theInterface_->DisableROCEmulator(DTCLib::DTC_Rings[ring]);
			}
		}
	}

	auto sim_file = ps.get<std::string>("sim_file", "");
	if (sim_file.size() > 0)
	{
		simFileRead_ = false;
		std::thread reader(&mu2e::Mu2eReceiver::readSimFile_, this, sim_file);
		reader.detach();
	}
}

void mu2e::Mu2eReceiver::readSimFile_(std::string sim_file)
{
	mf::LogInfo("Mu2eReceiver") << "Starting read of simulation file " << sim_file << "." << " Please wait to start the run until finished.";
	theInterface_->WriteSimFileToDTC(sim_file, true);
	simFileRead_ = true;
	mf::LogInfo("Mu2eReceiver") << "Done reading simulation file into DTC memory.";
}

mu2e::Mu2eReceiver::~Mu2eReceiver()
{
	theInterface_->DisableDetectorEmulator();
	delete theInterface_;
	delete theCFO_;
}

bool mu2e::Mu2eReceiver::getNext_(artdaq::FragmentPtrs& frags)
{
	while (!simFileRead_ && !should_stop())
	{
		usleep(5000);
	}

	if (should_stop())
	{
		return false;
	}

	TRACE(1, "mu2eReceiver::getNext: Starting CFO thread");
	uint64_t z = 0;
	DTCLib::DTC_Timestamp zero(z);
	if (mode_ != 0)
	{
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
	frags.emplace_back(new artdaq::Fragment(0, ev_counter(), fragment_ids_[0], fragment_type_, metadata));
	// Now we make an instance of the overlay to put the data into...
	TRACE(1, "mu2eReceiver::getNext: Making mu2eFragmentWriter");
	mu2eFragmentWriter newfrag(*frags.back());

	TRACE(1, "mu2eReceiver::getNext: Reserving space for 64 * BLOCK_COUNT_MAX bytes");
	newfrag.addSpace(mu2e::BLOCK_COUNT_MAX * 64);

	//Get data from DTCReceiver
	TRACE(1, "mu2eReceiver::getNext: Starting DTCFragment Loop");
	theInterface_->GetDevice()->ResetDeviceTime();
		size_t totalSize = 0;
	while (newfrag.hdr_block_count() < mu2e::BLOCK_COUNT_MAX)
	{
		if (should_stop())
		{
			break;
		}

		TRACE(1, "Getting DTC Data");
		std::vector<DTCLib::DTC_DataBlock> data;
		int retryCount = 5;
		while (data.size() == 0 && retryCount >= 0)
		{
			try
			{
			  //TRACE(4, "Calling theInterface->GetData(zero)");
				data = theInterface_->GetData(zero);
				//TRACE(4, "Done calling theInterface->GetData(zero)");
			}
			catch (std::exception ex)
			{
				std::cerr << ex.what() << std::endl;
			}
			retryCount--;
			//if (data.size() == 0){usleep(10000);}
		}
		if (retryCount < 0 && data.size() == 0)
		{
			TRACE(1, "Retry count exceeded. Something is very wrong indeed");
			std::cout << "Had an error with block " << newfrag.hdr_block_count() << " of event " << ev_counter() << std::endl;
			break;
		}

		TRACE(3, "Copying DTC packets into Mu2eFragment");
                totalSize = 0;
		for (size_t i = 0; i < data.size(); ++i)
		{
			totalSize += data[i].byteSize;
		}

		int64_t diff = totalSize + newfrag.blockSizeBytes() - newfrag.dataSize();
                TRACE(4, "diff=%lli, totalSize=%llu, dataSize=%llu, fragSize=%llu", (long long)diff, (long long unsigned)totalSize, (long long unsigned)newfrag.blockSizeBytes(), (long long unsigned)newfrag.dataSize());	
		if (diff > 0)
		{
			auto currSize = newfrag.dataSize();
			auto remaining = 1 - (newfrag.hdr_block_count() / static_cast<double>(BLOCK_COUNT_MAX));

			auto newSize = static_cast<size_t>(currSize * remaining);
			TRACE(1, "mu2eReceiver::getNext: %lu + %lu > %lu, allocating space for %lu more bytes", totalSize, newfrag.blockSizeBytes(), newfrag.dataSize(), newSize + diff);
			newfrag.addSpace(diff + newSize);
		}

		TRACE(3, "Copying DTC packets into Mu2eFragment");
		auto offset = newfrag.dataBegin() + newfrag.blockSizeBytes();
		size_t intraBlockOffset = 0;
		for (size_t i = 0; i < data.size(); ++i)
		{
			TRACE(4, "Copying data from %p to %p (sz=%llu)", reinterpret_cast<void*>(data[i].blockPointer), reinterpret_cast<void*>(offset + intraBlockOffset), (unsigned long long)data[i].byteSize);
			memcpy(reinterpret_cast<void*>(offset + intraBlockOffset), data[i].blockPointer, data[i].byteSize);
			intraBlockOffset += data[i].byteSize;
		}

		TRACE(3, "Ending SubEvt %lu", newfrag.hdr_block_count());
		newfrag.endSubEvt(intraBlockOffset);
	}
	TRACE(1, "Incrementing event counter");
	ev_counter_inc();

	TRACE(1, "Reporting Metrics");
	timestamps_read_ += newfrag.hdr_block_count();
	auto hwTime = theInterface_->GetDevice()->GetDeviceTime();
	double timestamp_rate = newfrag.hdr_block_count() / _timeSinceLastSend();
	double hw_timestamp_rate = newfrag.hdr_block_count() / hwTime;
	double hw_data_rate = totalSize / hwTime;

	metricMan_->sendMetric("Timestamp Count", timestamps_read_, "timestamps", 1, true, false);
	metricMan_->sendMetric("Timestamp Rate", timestamp_rate, "timestamps/s", 1, true, true);
	metricMan_->sendMetric("HW Timestamp Rate", hw_timestamp_rate, "timestamps/s", 1, true, true);
	metricMan_->sendMetric("PCIe Transfer Rate", hw_data_rate, "B/s",1, true ,true);


	TRACE(1, "Returning true");
	return true;
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::Mu2eReceiver)

