#include "mu2e-artdaq/Generators/DTCReceiver.hh"

#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"
#include "artdaq/Generators/GeneratorMacros.hh"
#include "canvas/Utilities/Exception.h"
#include "cetlib_except/exception.h"
#include "dtcInterfaceLib/DTC_Types.h"
#include "fhiclcpp/ParameterSet.h"
#include "mu2e-artdaq-core/Overlays/DTCFragment.hh"
#include "mu2e-artdaq-core/Overlays/DTCFragmentWriter.hh"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "artdaq-core/Data/ContainerFragmentLoader.hh"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>

#include <unistd.h>
#include "trace.h"

mu2e::DTCReceiver::DTCReceiver(fhicl::ParameterSet const& ps)
	: CommandableFragmentGenerator(ps), fragment_type_(toFragmentType("DTC")), fragment_ids_{static_cast<artdaq::Fragment::fragment_id_t>(fragment_id())}, packets_read_(0), mode_(DTCLib::DTC_SimModeConverter::ConvertToSimMode(ps.get<std::string>("sim_mode", "Disabled"))), board_id_(static_cast<uint8_t>(ps.get<int>("board_id", 0))), print_packets_(ps.get<bool>("debug_print", false))
{
	// mode_ can still be overridden by environment!
	theInterface_ = new DTCLib::DTC(mode_);
	theCFO_ = new DTCLib::DTCSoftwareCFO(theInterface_, true);
	mode_ = theInterface_->ReadSimMode();

	TLOG_DEBUG("DTCReceiver") << "DTCReceiver Initialized with mode " << mode_ << TLOG_ENDL;

	//	int ringRocs[] = {ps.get<int>("ring_0_roc_count", -1), ps.get<int>("ring_1_roc_count", -1),
	//					  ps.get<int>("ring_2_roc_count", -1), ps.get<int>("ring_3_roc_count", -1),
	//					  ps.get<int>("ring_4_roc_count", -1), ps.get<int>("ring_5_roc_count", -1)};
	//
	//	bool ringTiming[] = {ps.get<bool>("ring_0_timing_enabled", true), ps.get<bool>("ring_1_timing_enabled", true),
	//						 ps.get<bool>("ring_2_timing_enabled", true), ps.get<bool>("ring_3_timing_enabled", true),
	//						 ps.get<bool>("ring_4_timing_enabled", true), ps.get<bool>("ring_5_timing_enabled", true)};
	//
	//	bool ringEmulators[] = {
	//		ps.get<bool>("ring_0_roc_emulator_enabled", false), ps.get<bool>("ring_1_roc_emulator_enabled", false),
	//		ps.get<bool>("ring_2_roc_emulator_enabled", false), ps.get<bool>("ring_3_roc_emulator_enabled", false),
	//		ps.get<bool>("ring_4_roc_emulator_enabled", false), ps.get<bool>("ring_5_roc_emulator_enabled", false)};
	//
	//	for (int ring = 0; ring < 6; ++ring)
	//	{
	//		if (ringRocs[ring] >= 0)
	//		{
	//			theInterface_->EnableRing(DTCLib::DTC_Rings[ring], DTCLib::DTC_RingEnableMode(true, true, ringTiming[ring]),
	//									  DTCLib::DTC_ROCS[ringRocs[ring]]);
	//			if (ringEmulators[ring])
	//			{
	//				theInterface_->EnableROCEmulator(DTCLib::DTC_Rings[ring]);
	//			}
	//			else
	//			{
	//				theInterface_->DisableROCEmulator(DTCLib::DTC_Rings[ring]);
	//			}
	//		}
	//	}

	char* file_c = getenv("DTCLIB_SIM_FILE");

	auto sim_file = ps.get<std::string>("sim_file", "");
	if (file_c != nullptr)
	{
		sim_file = std::string(file_c);
	}
	if (sim_file.size() > 0)
	{
		simFileRead_ = false;
		std::thread reader(&mu2e::DTCReceiver::readSimFile_, this, sim_file);
		reader.detach();
	}
	else
	{
		simFileRead_ = true;
	}
}

void mu2e::DTCReceiver::readSimFile_(std::string sim_file)
{
	TLOG_INFO("DTCReceiver") << "Starting read of simulation file " << sim_file << "."
							 << " Please wait to start the run until finished." << TLOG_ENDL;
	theInterface_->WriteSimFileToDTC(sim_file, true);
	simFileRead_ = true;
	TLOG_INFO("DTCReceiver") << "Done reading simulation file into DTC memory." << TLOG_ENDL;
}

mu2e::DTCReceiver::~DTCReceiver()
{
	theInterface_->DisableDetectorEmulator();
	delete theInterface_;
	delete theCFO_;
}

bool mu2e::DTCReceiver::getNext_(artdaq::FragmentPtrs& frags) { return getNextDTCFragment(frags); }

void mu2e::DTCReceiver::start()
{
	if (fragment_receiver_thread_.joinable()) fragment_receiver_thread_.join();
	TLOG(TLVL_INFO) << "Starting Data Receiver Thread";
	try
	{
		fragment_receiver_thread_ = boost::thread(&DTCReceiver::fragment_thread_, this);
	}
	catch (const boost::exception& e)
	{
		TLOG(TLVL_ERROR) << "Caught boost::exception starting Data Receiver thread: " << boost::diagnostic_information(e) << ", errno=" << errno;
		std::cerr << "Caught boost::exception starting Data Receiver thread: " << boost::diagnostic_information(e) << ", errno=" << errno << std::endl;
		exit(5);
	}
}

bool mu2e::DTCReceiver::getNextDTCFragment(artdaq::FragmentPtrs& frags)
{
	while (!simFileRead_ && !should_stop())
	{
		usleep(5000);
	}

	if (should_stop())
	{
		return false;
	}

	std::unique_lock<std::mutex> lk(fragment_mutex_);
	fragment_ready_condition_.wait(lk, [&]() { return current_container_fragment_ != nullptr; });
	
	frags.emplace_back(std::move(current_container_fragment_));
	ev_counter_inc();
	current_container_fragment_.reset(nullptr);

	return true;
}

void mu2e::DTCReceiver::stop()
{
	if (fragment_receiver_thread_.joinable()) fragment_receiver_thread_.join();
}

void mu2e::DTCReceiver::fragment_thread_()
{
	artdaq::ContainerFragmentLoader* loader = nullptr;
	while (!should_stop())
	{
		std::vector<DTCLib::DTC_DataBlock> data;

		if (mode_ != 0)
		{
			theCFO_->SendRequestForTimestamp(DTCLib::DTC_Timestamp(ev_counter()));
		}

		while (data.size() == 0 && !should_stop())
		{
			try
			{
				data = theInterface_->GetData(DTCLib::DTC_Timestamp());
			}
			catch (std::exception ex)
			{
				std::cerr << ex.what() << std::endl;
			}
		}

		if (should_stop()) return;
		
		auto first = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[0].blockPointer));
		DTCLib::DTC_Timestamp ts = first.GetTimestamp();
		int packetCount = first.GetPacketCount() + 1;
		if (print_packets_)
		{
			std::cout << first.toJSON() << std::endl;
			for (int ii = 0; ii < first.GetPacketCount(); ++ii)
			{
				std::cout << "\t" << DTCLib::DTC_DataPacket(((uint8_t*)data[0].blockPointer) + ((ii + 1) * 16)).toJSON()
						  << std::endl;
			}
		}

		for (size_t i = 1; i < data.size(); ++i)
		{
			auto packet = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[i].blockPointer));
			packetCount += packet.GetPacketCount() + 1;
			if (print_packets_)
			{
				std::cout << packet.toJSON() << std::endl;
				for (int ii = 0; ii < packet.GetPacketCount(); ++ii)
				{
					std::cout << "\t" << DTCLib::DTC_DataPacket(((uint8_t*)data[i].blockPointer) + ((ii + 1) * 16)).toJSON()
							  << std::endl;
				}
			}
		}

		std::unique_lock<std::mutex> lk(fragment_mutex_);
		if (current_container_fragment_ == nullptr)
		{
			current_container_fragment_.reset(new artdaq::Fragment(ev_counter(), fragment_ids_.front(), artdaq::Fragment::ContainerFragmentType, ts));
			loader = new artdaq::ContainerFragmentLoader(*current_container_fragment_, fragment_type_);
		}

		auto newFrag = loader->appendFragment(packetCount * 16 / sizeof(artdaq::RawDataType));
		newFrag->type = fragment_type_;
		newFrag->timestamp = ts;

		size_t packetsProcessed = 0;
		packet_t* dataStart = reinterpret_cast<packet_t*>(newFrag + 1);
		for (size_t i = 0; i < data.size(); ++i)
		{
			auto packet = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[i].blockPointer));
			memcpy((void*)(dataStart + packetsProcessed), data[i].blockPointer,
				   (1 + packet.GetPacketCount()) * sizeof(packet_t));
			packetsProcessed += 1 + packet.GetPacketCount();
		}

		fragment_ready_condition_.notify_all();
	}
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::DTCReceiver)
