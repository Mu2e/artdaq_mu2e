#include "mu2e-artdaq/Generators/Mu2eReceiver.hh"

#include "canvas/Utilities/Exception.h"

#include "artdaq/Application/GeneratorMacros.hh"
#include "artdaq/DAQdata/Globals.hh"
#include "cetlib/exception.h"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"
#include "mu2e-artdaq-core/Overlays/mu2eFragmentWriter.hh"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "fhiclcpp/ParameterSet.h"
#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"

#include <fstream>
#include <iomanip>
#include <iterator>

#include <unistd.h>
#include "trace.h"

mu2e::Mu2eReceiver::Mu2eReceiver(fhicl::ParameterSet const& ps)
	: CommandableFragmentGenerator(ps)
	, fragment_type_(toFragmentType("MU2E"))
	, fragment_ids_{ static_cast<artdaq::Fragment::fragment_id_t>(fragment_id()) }
	, timestamps_read_(0)
	, lastReportTime_(std::chrono::steady_clock::now())
	, mode_(DTCLib::DTC_SimModeConverter::ConvertToSimMode(ps.get<std::string>("sim_mode", "Disabled")))
	, board_id_(static_cast<uint8_t>(ps.get<int>("board_id", 0)))
	, rawOutput_(ps.get<bool>("raw_output_enable", false))
	, rawOutputFile_(ps.get<std::string>("raw_output_file", "/tmp/Mu2eReceiver.bin"))
	, nSkip_(ps.get<size_t>("fragment_receiver_count", 1))
	, sendEmpties_(ps.get<bool>("send_empty_fragments", false))
	, verbose_(ps.get<bool>("verbose", false))
{
	TRACE(1, "Mu2eReceiver_generator CONSTRUCTOR");
	// mode_ can still be overridden by environment!
	auto version = ps.get<std::string>("dtc_firmware_version", "");
	theInterface_ = new DTCLib::DTC(version, mode_);
	theCFO_ = new DTCLib::DTCSoftwareCFO(theInterface_, true);
	mode_ = theInterface_->ReadSimMode();

	if (!ps.get<bool>("use_detector_emulator", false))
	{
		theInterface_->ClearDetectorEmulatorInUse(); // Needed if we're doing ROC Emulator...make sure Detector Emulation is disabled
	}
	else
	{
		theInterface_->SetDetectorEmulatorInUse();
	}

	TLOG_INFO("Mu2eReceiver") << "The DTC Firmware version string is: " << theInterface_->ReadDesignVersion() << TLOG_ENDL;

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

	int ringEmulatorCount[] = {
	  ps.get<int>("ring_0_roc_emulator_count", 0),
	  ps.get<int>("ring_1_roc_emulator_count", 0),
	  ps.get<int>("ring_2_roc_emulator_count", 0),
	  ps.get<int>("ring_3_roc_emulator_count", 0),
	  ps.get<int>("ring_4_roc_emulator_count", 0),
	  ps.get<int>("ring_5_roc_emulator_count", 0)
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
				theInterface_->SetMaxROCNumber(DTCLib::DTC_Rings[ring], DTCLib::DTC_ROCS[ringEmulatorCount[ring]]);
				theInterface_->EnableROCEmulator(DTCLib::DTC_Rings[ring]);
			}
			else
			{
				theInterface_->DisableROCEmulator(DTCLib::DTC_Rings[ring]);
			}
		}
	}

	if (ps.get<bool>("load_sim_file", false))
	{
		char* file_c = getenv("DTCLIB_SIM_FILE");

		auto sim_file = ps.get<std::string>("sim_file", "");
		if (file_c != nullptr) { sim_file = std::string(file_c); }
		if (sim_file.size() > 0)
		{
			simFileRead_ = false;
			std::thread reader(&mu2e::Mu2eReceiver::readSimFile_, this, sim_file);
			reader.detach();
		}
	}
	else
	{
		simFileRead_ = true;
	}

	if (rawOutput_) rawOutputStream_.open(rawOutputFile_, std::ios::out | std::ios::app | std::ios::binary);
}

void mu2e::Mu2eReceiver::readSimFile_(std::string sim_file)
{
	TLOG_INFO("Mu2eReceiver") << "Starting read of simulation file " << sim_file << "." << " Please wait to start the run until finished." << TLOG_ENDL;
	theInterface_->WriteSimFileToDTC(sim_file, true);
	simFileRead_ = true;
	TLOG_INFO("Mu2eReceiver") << "Done reading simulation file into DTC memory." << TLOG_ENDL;
}

mu2e::Mu2eReceiver::~Mu2eReceiver()
{
	rawOutputStream_.close();
	delete theInterface_;
	delete theCFO_;
}

void mu2e::Mu2eReceiver::stop()
{
	theInterface_->DisableDetectorEmulator();
	theInterface_->DisableCFOEmulation();
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

	if (sendEmpties_)
	{
		int mod = ev_counter() % nSkip_;
		if (mod == board_id_ || (mod == 0 && board_id_ == nSkip_))
		{
			//TLOG_DEBUG("Mu2eReceiver") << "Sending Data  Fragment for sequence id " << ev_counter() << " (board_id " << std::to_string(board_id_) << ")" << TLOG_ENDL;
		}
		else
		{
			//TLOG_DEBUG("Mu2eReceiver") << "Sending Empty Fragment for sequence id " << ev_counter() << " (board_id " << std::to_string(board_id_) << ")" << TLOG_ENDL;
			return sendEmpty_(frags);
		}
	}

	_startProcTimer();
	TRACE(1, "mu2eReceiver::getNext: Starting CFO thread");
	uint64_t z = 0;
	DTCLib::DTC_Timestamp zero(z);
	if (mode_ != 0)
	{
		//theInterface_->ReleaseAllBuffers();
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

	TRACE(1, "mu2eReceiver::getNext: Reserving space for 16 * 201 * BLOCK_COUNT_MAX bytes");
	newfrag.addSpace(mu2e::BLOCK_COUNT_MAX * 16 * 201);

	//Get data from Mu2eReceiver
	TRACE(1, "mu2eReceiver::getNext: Starting DTCFragment Loop");
	theInterface_->GetDevice()->ResetDeviceTime();
	size_t totalSize = 0;
	while (newfrag.hdr_block_count() < mu2e::BLOCK_COUNT_MAX)
	{
		if (should_stop())
		{
			break;
		}

		TRACE(3, "Getting DTC Data for block %lu/%i, sz=%lui", newfrag.hdr_block_count(), mu2e::BLOCK_COUNT_MAX, totalSize);
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
				TLOG_ERROR("Mu2eReceiver") << "There was an error in the DTC Library: " << ex.what() << TLOG_ENDL;
			}
			retryCount--;
			//if (data.size() == 0){usleep(10000);}
		}
		if (retryCount < 0 && data.size() == 0)
		{
			TRACE(1, "Retry count exceeded. Something is very wrong indeed");
			TLOG_ERROR("Mu2eReceiver") << "Had an error with block " << newfrag.hdr_block_count() << " of event " << ev_counter() << TLOG_ENDL;
			if (newfrag.hdr_block_count() == 0)
			{
				throw cet::exception("DTC Retry count exceeded in first block of Event. Probably something is very wrong, aborting");
			}
			break;
		}

		TRACE(5, "Allocating space in Mu2eFragment for DTC packets");
		totalSize = 0;
		for (size_t i = 0; i < data.size(); ++i)
		{
			totalSize += data[i].byteSize;
		}

		int64_t diff = totalSize + newfrag.dataEndBytes() - newfrag.dataSize();
		TRACE(7, "diff=%lli, totalSize=%llu, dataSize=%llu, fragSize=%llu", (long long)diff, (long long unsigned)totalSize, (long long unsigned)newfrag.dataEndBytes(), (long long unsigned)newfrag.dataSize());
		if (diff > 0)
		{
			auto currSize = newfrag.dataSize();
			auto remaining = 1.0 - (newfrag.hdr_block_count() / static_cast<double>(BLOCK_COUNT_MAX));

			auto newSize = static_cast<size_t>(currSize * remaining);
			TRACE(3, "mu2eReceiver::getNext: %lu + %lu > %lu, allocating space for %lu more bytes", totalSize, newfrag.dataEndBytes(), newfrag.dataSize(), newSize + diff);
			newfrag.addSpace(diff + newSize);
		}

		TRACE(5, "Copying DTC packets into Mu2eFragment");
		//auto offset = newfrag.dataBegin() + newfrag.blockSizeBytes();
		size_t offset = newfrag.dataEndBytes();
		for (size_t i = 0; i < data.size(); ++i)
		{
			if (verbose_)
			{
				auto dp = DTCLib::DTC_DataPacket(data[i].blockPointer);
				auto dhp = DTCLib::DTC_DataHeaderPacket(dp);
				TLOG_TRACE("Mu2eReceiver") << "Placing DataBlock with timestamp " << static_cast<double>(dhp.GetTimestamp().GetTimestamp(true)) << " into Mu2eFragment" << TLOG_ENDL;
			}

			auto begin = data[i].blockPointer;
			auto size = data[i].byteSize;

			while (data[i + 1].blockPointer == data[i].blockPointer + (data[i].byteSize / sizeof(DTCLib::DTC_DataBlock::pointer_t)))
			{
				size += data[++i].byteSize;
			}

			TRACE(5, "Copying data from %p to %p (sz=%llu)", reinterpret_cast<void*>(begin), reinterpret_cast<void*>(newfrag.dataAtBytes(offset)), (unsigned long long)size);
			//memcpy(reinterpret_cast<void*>(offset + intraBlockOffset), data[i].blockPointer, data[i].byteSize);
			std::copy(begin, begin + (size / sizeof(DTCLib::DTC_DataBlock::pointer_t)), newfrag.dataAtBytes(offset));
			if (rawOutput_) rawOutputStream_.write((char*)begin, size);
			offset += size;
		}

		TRACE(3, "Ending SubEvt %lu", newfrag.hdr_block_count());
		newfrag.endSubEvt(offset - newfrag.dataEndBytes());
	}
	TRACE(1, "Incrementing event counter");
	ev_counter_inc();

	TRACE(1, "Reporting Metrics");
	timestamps_read_ += newfrag.hdr_block_count();
	auto hwTime = theInterface_->GetDevice()->GetDeviceTime();

	double processing_rate = newfrag.hdr_block_count() / _getProcTimerCount();
	double timestamp_rate = newfrag.hdr_block_count() / _timeSinceLastSend();
	double hw_timestamp_rate = newfrag.hdr_block_count() / hwTime;
	double hw_data_rate = newfrag.dataEndBytes() / hwTime;

	metricMan->sendMetric("Timestamp Count", timestamps_read_, "timestamps", 1, false, "", true);
	metricMan->sendMetric("Timestamp Rate", timestamp_rate, "timestamps/s", 1, true, "", true);
	metricMan->sendMetric("Generator Timestamp Rate", processing_rate, "timestamps/s", 1, true, "", true);
	metricMan->sendMetric("HW Timestamp Rate", hw_timestamp_rate, "timestamps/s", 1, true, "", true);
	metricMan->sendMetric("PCIe Transfer Rate", hw_data_rate, "B/s", 1, true, "", true);

	TRACE(1, "Returning true");
	return true;
}

bool mu2e::Mu2eReceiver::sendEmpty_(artdaq::FragmentPtrs& frags)
{
	frags.emplace_back(new artdaq::Fragment());
	frags.back()->setSystemType(artdaq::Fragment::EmptyFragmentType);
	frags.back()->setSequenceID(ev_counter());
	frags.back()->setFragmentID(fragment_ids_[0]);
	ev_counter_inc();
	return true;
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::Mu2eReceiver)

