
#define TRACE_NAME "Mu2eReceiver"
#include "artdaq/DAQdata/Globals.hh"

#include "mu2e-artdaq/Generators/Mu2eReceiver.hh"

#include "canvas/Utilities/Exception.h"

#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"
#include "artdaq/Generators/GeneratorMacros.hh"
#include "cetlib_except/exception.h"
#include "fhiclcpp/ParameterSet.h"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"
#include "mu2e-artdaq-core/Overlays/mu2eFragmentWriter.hh"

#include <fstream>
#include <iomanip>
#include <iterator>

#include <unistd.h>

mu2e::Mu2eReceiver::Mu2eReceiver(fhicl::ParameterSet const& ps)
	: CommandableFragmentGenerator(ps), fragment_type_(toFragmentType("MU2E")), fragment_ids_{static_cast<artdaq::Fragment::fragment_id_t>(fragment_id())}, timestamps_read_(0), lastReportTime_(std::chrono::steady_clock::now()), mode_(DTCLib::DTC_SimModeConverter::ConvertToSimMode(ps.get<std::string>("sim_mode", "Disabled"))), board_id_(static_cast<uint8_t>(ps.get<int>("board_id", 0))), rawOutput_(ps.get<bool>("raw_output_enable", false)), rawOutputFile_(ps.get<std::string>("raw_output_file", "/tmp/Mu2eReceiver.bin")), nSkip_(ps.get<size_t>("fragment_receiver_count", 1)), sendEmpties_(ps.get<bool>("send_empty_fragments", false)), verbose_(ps.get<bool>("verbose", false)), nEvents_(ps.get<size_t>("number_of_events_to_generate", -1))
{
	TLOG(TLVL_DEBUG) << "Mu2eReceiver_generator CONSTRUCTOR";
	// mode_ can still be overridden by environment!
	theInterface_ = new DTCLib::DTC(mode_, ps.get<int>("dtc_id", -1), ps.get<unsigned>("roc_mask", 0x1), "", false, ps.get<std::string>("simulator_memory_file_name", "mu2esim.bin"));
	theCFO_ = new DTCLib::DTCSoftwareCFO(theInterface_, true);
	mode_ = theInterface_->ReadSimMode();

	TLOG_INFO("Mu2eReceiver") << "The DTC Firmware version string is: " << theInterface_->ReadDesignVersion()
							  << TLOG_ENDL;


	if (ps.get<bool>("load_sim_file", false))
	{
		theInterface_->SetDetectorEmulatorInUse();
		theInterface_->ResetDDR();

		char* file_c = getenv("DTCLIB_SIM_FILE");

		auto sim_file = ps.get<std::string>("sim_file", "");
		if (file_c != nullptr)
		{
			sim_file = std::string(file_c);
		}
		if (sim_file.size() > 0)
		{
			simFileRead_ = false;
			std::thread reader(&mu2e::Mu2eReceiver::readSimFile_, this, sim_file);
			reader.detach();
		}
	}
	else
	{
		theInterface_->ClearDetectorEmulatorInUse();  // Needed if we're doing ROC Emulator...make sure Detector Emulation
													  // is disabled
		simFileRead_ = true;
	}

	if (rawOutput_) rawOutputStream_.open(rawOutputFile_, std::ios::out | std::ios::app | std::ios::binary);
}

void mu2e::Mu2eReceiver::readSimFile_(std::string sim_file)
{
	TLOG_INFO("Mu2eReceiver") << "Starting read of simulation file " << sim_file << "."
							  << " Please wait to start the run until finished." << TLOG_ENDL;
	theInterface_->WriteSimFileToDTC(sim_file, true, true);
	simFileRead_ = true;
	TLOG_INFO("Mu2eReceiver") << "Done reading simulation file into DTC memory." << TLOG_ENDL;
}

mu2e::Mu2eReceiver::~Mu2eReceiver()
{
	rawOutputStream_.close();
	delete theCFO_;
	delete theInterface_;
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

	if (should_stop() || ev_counter() > nEvents_)
	{
		return false;
	}

	if (sendEmpties_)
	{
		int mod = ev_counter() % nSkip_;
		if (mod == board_id_ || (mod == 0 && board_id_ == nSkip_))
		{
			// TLOG_DEBUG("Mu2eReceiver") << "Sending Data  Fragment for sequence id " << ev_counter() << " (board_id " <<
			// std::to_string(board_id_) << ")" << TLOG_ENDL;
		}
		else
		{
			// TLOG_DEBUG("Mu2eReceiver") << "Sending Empty Fragment for sequence id " << ev_counter() << " (board_id " <<
			// std::to_string(board_id_) << ")" << TLOG_ENDL;
			return sendEmpty_(frags);
		}
	}

	_startProcTimer();
	TLOG(10) << "mu2eReceiver::getNext: Starting CFO thread";
	uint64_t z = 0;
	DTCLib::DTC_Timestamp zero(z);
	if (mode_ != 0)
	{
#if 0
		//theInterface_->ReleaseAllBuffers();
		TLOG(TLVL_DEBUG) << "Sending requests for " << mu2e::BLOCK_COUNT_MAX << " timestamps, starting at " << mu2e::BLOCK_COUNT_MAX * (ev_counter() - 1);
		theCFO_->SendRequestsForRange(mu2e::BLOCK_COUNT_MAX, DTCLib::DTC_Timestamp(mu2e::BLOCK_COUNT_MAX * (ev_counter() - 1)));
#else
		theCFO_->SendRequestsForRange(-1, DTCLib::DTC_Timestamp(mu2e::BLOCK_COUNT_MAX * (ev_counter() - 1)));

#endif
	}

	TLOG(10) << "mu2eReceiver::getNext: Initializing mu2eFragment metadata";
	mu2eFragment::Metadata metadata;
	metadata.sim_mode = static_cast<int>(mode_);
	metadata.run_number = run_number();
	metadata.board_id = board_id_;

	// And use it, along with the artdaq::Fragment header information
	// (fragment id, sequence id, and user type) to create a fragment
	TLOG(10) << "mu2eReceiver::getNext: Creating new mu2eFragment!";
	frags.emplace_back(new artdaq::Fragment(0, ev_counter(), fragment_ids_[0], fragment_type_, metadata));
	// Now we make an instance of the overlay to put the data into...
	TLOG(10) << "mu2eReceiver::getNext: Making mu2eFragmentWriter";
	mu2eFragmentWriter newfrag(*frags.back());

	TLOG(10) << "mu2eReceiver::getNext: Reserving space for 16 * 201 * BLOCK_COUNT_MAX bytes";
	newfrag.addSpace(mu2e::BLOCK_COUNT_MAX * 16 * 201);

	// Get data from Mu2eReceiver
	TLOG(10) << "mu2eReceiver::getNext: Starting DTCFragment Loop";
	theInterface_->GetDevice()->ResetDeviceTime();
	size_t totalSize = 0;
	bool first = true;
	while (newfrag.hdr_block_count() < mu2e::BLOCK_COUNT_MAX)
	{
		if (should_stop())
		{
			break;
		}

		TLOG(10) << "Getting DTC Data for block " << newfrag.hdr_block_count() << "/" << mu2e::BLOCK_COUNT_MAX
				 << ", sz=" << totalSize;
		std::vector<DTCLib::DTC_DataBlock> data;
		int retryCount = 5;
		while (data.size() == 0 && retryCount >= 0)
		{
			try
			{
				TLOG(30) << "Calling theInterface->GetData(zero)";
				data = theInterface_->GetData(zero);
				TLOG(30) << "Done calling theInterface->GetData(zero)";
			}
			catch (std::exception const& ex)
			{
				TLOG_ERROR("Mu2eReceiver") << "There was an error in the DTC Library: " << ex.what();
			}
			retryCount--;
			// if (data.size() == 0){usleep(10000);}
		}
		if (retryCount < 0 && data.size() == 0)
		{
			TLOG(TLVL_DEBUG) << "Retry count exceeded. Something is very wrong indeed";
			TLOG_ERROR("Mu2eReceiver") << "Had an error with block " << newfrag.hdr_block_count() << " of event "
									   << ev_counter();
			if (newfrag.hdr_block_count() == 0)
			{
				throw cet::exception(
					"DTC Retry count exceeded in first block of Event. Probably something is very wrong, aborting");
			}
			break;
		}

		TLOG(11) << "Allocating space in Mu2eFragment for DTC packets";
		totalSize = 0;
		for (size_t i = 0; i < data.size(); ++i)
		{
			totalSize += data[i].byteSize;
		}

		int64_t diff = totalSize + newfrag.dataEndBytes() - newfrag.dataSize();
		TLOG(12) << "diff=" << diff << ", totalSize=" << totalSize << ", dataSize=" << newfrag.dataEndBytes()
				 << ", fragSize=" << newfrag.dataSize();
		if (diff > 0)
		{
			auto currSize = newfrag.dataSize();
			auto remaining = 1.0 - (newfrag.hdr_block_count() / static_cast<double>(BLOCK_COUNT_MAX));

			auto newSize = static_cast<size_t>(currSize * remaining);
			TLOG(13) << "mu2eReceiver::getNext: " << totalSize << " + " << newfrag.dataEndBytes() << " > "
					 << newfrag.dataSize() << ", allocating space for " << newSize + diff << " more bytes";
			newfrag.addSpace(diff + newSize);
		}

		TLOG(14) << "Copying DTC packets into Mu2eFragment";
		// auto offset = newfrag.dataBegin() + newfrag.blockSizeBytes();
		size_t offset = newfrag.dataEndBytes();
		for (size_t i = 0; i < data.size(); ++i)
		{
			auto dp = DTCLib::DTC_DataPacket(data[i].blockPointer);
			auto dhp = DTCLib::DTC_DataHeaderPacket(dp);
			TLOG(16) << "Placing DataBlock with timestamp "
					 << static_cast<double>(dhp.GetTimestamp().GetTimestamp(true)) << " into Mu2eFragment"
					 << TLOG_ENDL;
			

				auto fragment_timestamp = dhp.GetTimestamp().GetTimestamp(true);
				if(fragment_timestamp > highest_timestamp_seen_) {
				  highest_timestamp_seen_ = fragment_timestamp;
				}

			if (first)
			{
				first = false;

			if (fragment_timestamp < highest_timestamp_seen_)
			{
			  if(fragment_timestamp == 0) { timestamp_loops_++; }
			  // Timestamps start at 0, so make sure to offset by one so we don't repeat highest_timestamp_seen_
			  fragment_timestamp += timestamp_loops_ * (highest_timestamp_seen_ + 1);
			}

				frags.back()->setTimestamp(fragment_timestamp);
			}

			auto begin = reinterpret_cast<const uint8_t*>(data[i].blockPointer);
			auto size = data[i].byteSize;

			while (data[i + 1].blockPointer == static_cast<const void*>(begin + size))
			{
				size += data[++i].byteSize;
			}

			TLOG(15) << "Copying data from " << begin << " to "
					 << reinterpret_cast<void*>(newfrag.dataAtBytes(offset)) << " (sz=" << size << ")";
			// memcpy(reinterpret_cast<void*>(offset + intraBlockOffset), data[i].blockPointer, data[i].byteSize);
			std::copy(begin, reinterpret_cast<const uint8_t*>(begin) + size, newfrag.dataAtBytes(offset));
			if (rawOutput_) rawOutputStream_.write((char*)begin, size);
			offset += size;
		}

		TLOG(16) << "Ending SubEvt " << newfrag.hdr_block_count();
		newfrag.endSubEvt(offset - newfrag.dataEndBytes());
	}
	TLOG(10) << "Incrementing event counter";
	ev_counter_inc();

	TLOG(10) << "Reporting Metrics";
	timestamps_read_ += newfrag.hdr_block_count();
	auto hwTime = theInterface_->GetDevice()->GetDeviceTime();

	double processing_rate = newfrag.hdr_block_count() / _getProcTimerCount();
	double timestamp_rate = newfrag.hdr_block_count() / _timeSinceLastSend();
	double hw_timestamp_rate = newfrag.hdr_block_count() / hwTime;
	double hw_data_rate = newfrag.dataEndBytes() / hwTime;

	metricMan->sendMetric("Timestamp Count", timestamps_read_, "timestamps", 1, artdaq::MetricMode::LastPoint);
	metricMan->sendMetric("Timestamp Rate", timestamp_rate, "timestamps/s", 1, artdaq::MetricMode::Average);
	metricMan->sendMetric("Generator Timestamp Rate", processing_rate, "timestamps/s", 1, artdaq::MetricMode::Average);
	metricMan->sendMetric("HW Timestamp Rate", hw_timestamp_rate, "timestamps/s", 1, artdaq::MetricMode::Average);
	metricMan->sendMetric("PCIe Transfer Rate", hw_data_rate, "B/s", 1, artdaq::MetricMode::Average);

	TLOG(10) << "Returning true";
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
