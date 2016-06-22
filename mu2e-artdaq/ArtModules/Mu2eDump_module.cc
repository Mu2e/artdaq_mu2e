///////////////////////////////////////////////////////////////////////////////////////
// Class:       Mu2eDump
// Module Type: EDAnalyzer
// File:        Mu2eDump_module.cc
// Description: Prints out mu2eFragments in HWUG Packet format (see mu2e-docdb #4097)
///////////////////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#ifdef CANVAS
#include "canvas/Utilities/Exception.h"
#else
#include "art/Utilities/Exception.h"
#endif

#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"
#include "artdaq-core/Data/Fragments.hh"
#include "dtcInterfaceLib/DTC_Packets.h"

#include "trace.h"

#include <string>
#include <iostream>
#include <iomanip>
#include <unistd.h>

namespace mu2e
{
	class Mu2eDump;
}

class mu2e::Mu2eDump : public art::EDAnalyzer
{
public:
	explicit Mu2eDump(fhicl::ParameterSet const& pset);
	virtual ~Mu2eDump();

	virtual void analyze(art::Event const& evt);

private:
	std::string raw_data_label_;
	uint16_t max_DataPackets_to_show_;
	bool print_json_;
	bool print_packet_format_;
};

mu2e::Mu2eDump::Mu2eDump(fhicl::ParameterSet const& pset)
	: EDAnalyzer(pset)
	  , raw_data_label_(pset.get<std::string>("raw_data_label"))
	  , max_DataPackets_to_show_(static_cast<uint16_t>(pset.get<int>("max_DataPackets_to_show", 0)))
	  , print_json_(pset.get<bool>("print_json", false))
	  , print_packet_format_(pset.get<bool>("print_packet_format", true)) {}

mu2e::Mu2eDump::~Mu2eDump() {}

void mu2e::Mu2eDump::analyze(art::Event const& evt)
{
	art::EventNumber_t eventNumber = evt.event();
	TRACE( 11, "mu2e::Mu2eDump::analyze enter eventNumber=%d", eventNumber );

	art::Handle<artdaq::Fragments> raw;
	evt.getByLabel(raw_data_label_, "MU2E", raw);

	if (raw.isValid())
	{
		std::cout << "######################################################################" << std::endl;
		std::cout << std::endl;
		std::cout << std::dec << "Run " << evt.run() << ", subrun " << evt.subRun()
		             << ", event " << eventNumber << " has " << raw->size()
		             << " mu2eFragment(s)" << std::endl;

//		for (size_t idx = 0; idx < raw->size(); ++idx)
//		{
//			const auto& frag((*raw)[idx]);
//
//			mu2eFragment bb(frag);
//
//			if (frag.hasMetadata())
//			{
//				std::cout << std::endl;
//				std::cout << "mu2eFragment SimMode: ";
//				mu2eFragment::Metadata const* md =
//					frag.metadata<mu2eFragment::Metadata>();
//				std::cout << DTCLib::DTC_SimModeConverter((DTCLib::DTC_SimMode)(md->sim_mode)).toString()
//					<< std::endl;
//				//				std::cout << "Timestamp: 0x" << std::hex << bb.hdr_timestamp() << std::endl;
//				std::cout << "Board ID: " << std::to_string((int)md->board_id) << std::endl;
//				std::cout << std::endl;
//			}
//
//			std::cout << "DTC_DataHeaderPacket:" << std::endl;
//			auto dhpacket = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket((uint8_t*)*(bb.dataBegin())));
//			if (print_json_)
//			{
//				std::cout << dhpacket.toJSON() << std::endl;
//			}
//			if (print_packet_format_)
//			{
//				std::cout << dhpacket.toPacketFormat() << std::endl;
//			}
//			std::cout << std::endl;
//
//
//			std::cout << std::endl;
//			int packetMax = max_DataPackets_to_show_ > (bb.hdr_packet_count() - 1) ? (bb.hdr_packet_count() - 1) : max_DataPackets_to_show_;
//			for (int ii = 0; ii < packetMax; ++ii)
//			{
//				auto packet = DTCLib::DTC_DataPacket((uint8_t*)*(bb.dataBegin() + 1 + ii));
//				if (print_json_)
//				{
//					std::cout << packet.toJSON() << std::endl;
//				}
//				if (print_packet_format_)
//				{
//					std::cout << packet.toPacketFormat() << std::endl;
//				}
//				std::cout << std::endl;
//			}
//		}
	}
	else
	{
		std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()
		             << ", event " << eventNumber << " has zero"
		             << " mu2eFragments." << std::endl;
	}
	std::cout << std::endl;
}

DEFINE_ART_MODULE(mu2e::Mu2eDump)

