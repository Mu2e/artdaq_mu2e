///////////////////////////////////////////////////////////////////////////////////////
// Class:       DetectorDump
// Module Type: EDAnalyzer
// File:        DetectorDump_module.cc
// Description: Prints out DTCFragments in HWUG Packet format (see mu2e-docdb #4097)
///////////////////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "canvas/Utilities/Exception.h"

//#include "mu2e-artdaq-core/Overlays/DTCFragment.hh"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/Fragments.hh"
#include "dtcInterfaceLib/DTC_Types.h"

#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "mu2e-artdaq-core/Overlays/DetectorFragment.hh"
#include "mu2e-artdaq-core/Overlays/TrackerFragmentReader.hh"
#include "mu2e-artdaq-core/Overlays/CalorimeterFragmentReader.hh"
#include "mu2e-artdaq-core/Overlays/CosmicVetoFragmentReader.hh"

//#include <mongo/client/dbclient.h>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/core.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/uri.hpp>

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::finalize;

#include <fstream>
#include <ctime>

#include <cstdlib>
#include <sstream>

#include "trace.h"

#include <string>
#include <iostream>
#include <iomanip>
#include <unistd.h>

namespace mu2e
{
	class DetectorDump;
}

//using bsoncxx::builder::stream::document;
//using bsoncxx::builder::stream::open_document;
//using bsoncxx::builder::stream::close_document;
//using bsoncxx::builder::stream::open_array;
//using bsoncxx::builder::stream::close_array;
//using bsoncxx::builder::stream::finalize;

class mu2e::DetectorDump : public art::EDAnalyzer
{
public:
	explicit DetectorDump(fhicl::ParameterSet const& pset);
	virtual ~DetectorDump();

	virtual void analyze(art::Event const& evt);

private:
	std::string raw_data_label_;
	std::string fragment_type_string_;
	FragmentType const fragment_type_; // Type of fragment (see FragmentType.hh)
	uint16_t max_DataPackets_to_show_;
	bool print_json_;
	bool print_packet_format_;

	//  mongocxx::instance inst{};
	mongocxx::client conn{};

	struct conn_holder
	{
		conn_holder(mongocxx::database&& arg) : db(std::move(arg)) { }

		mongocxx::database db;
	};

	struct coll_holder
	{
		coll_holder(mongocxx::collection&& arg) : coll(std::move(arg)) { }

		mongocxx::collection coll;
	};

	conn_holder ch{conn["mydb"]};
	coll_holder cl{ch.db["testData"]};
};

mu2e::DetectorDump::DetectorDump(fhicl::ParameterSet const& pset)
	: EDAnalyzer(pset)
	  , raw_data_label_(pset.get<std::string>("raw_data_label"))
	  , fragment_type_string_(pset.get<std::string>("frag_type"))
	  , fragment_type_(toFragmentType(pset.get<std::string>("frag_type")))
	  , max_DataPackets_to_show_(static_cast<uint16_t>(pset.get<int>("max_DataPackets_to_show", 0)))
	  , print_json_(pset.get<bool>("print_json", false))
	  , print_packet_format_(pset.get<bool>("print_packet_format", true))
{
	std::cout << "DetectorDump FRAGMENT TYPE: ";
	switch (fragment_type_)
	{
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

	std::cout << "Connected to mongodb database..." << std::endl;
	std::cout << "\t" << "Database entry count: " << cl.coll.count({}) << std::endl;


	//  auto cursor = cl.coll.find({});
	//  for (auto&& doc : cursor) {
	//    std::cout << bsoncxx::to_json(doc) << std::endl;
	//  }


	std::cout << "\t" << "Removing events from database..." << std::endl;
	cl.coll.delete_many(document{} << "type" << "event" << finalize);
	cl.coll.delete_many(document{} << "type" << "config" << finalize);
	std::cout << "\t" << "Entries remaining: " << cl.coll.count({}) << std::endl;
	auto variables_doc = document{}
		<< "type" << "config"

		   << "CAL.crystalID.type" << "I"
		   << "CAL.apdID.type" << "I"
		   << "CAL.time.type" << "I"
		   << "CAL.numSamples.type" << "I"
		   << "CAL.ADCintegral.type" << "I"

		   << "CAL.crystalID.min" << 0
		   << "CAL.apdID.min" << 0
		   << "CAL.time.min" << 0
		   << "CAL.numSamples.min" << 0
		   << "CAL.ADCIntegral.min" << 0

		   << "CAL.crystalID.max" << 10000
		   << "CAL.apdID.max" << 2
		   << "CAL.time.max" << 100000
		   << "CAL.numSamples.max" << 30
		   << "CAL.ADCIntegral.max" << 1000

		   << "HEADER.rocID.type" << "I"
		   << "HEADER.ringID.type" << "I"
		   << "HEADER.packetType.type" << "I"

		   << "HEADER.rocID.min" << 0
		   << "HEADER.ringID.min" << 0
		   << "HEADER.packetType.min" << 0

		   << "HEADER.rocID.max" << 5
		   << "HEADER.ringID.max" << 5
		   << "HEADER.packetType.max" << 10

		   << finalize;

	cl.coll.insert_one(variables_doc.view());

	std::cout << "\t" << "Done adding variable list to database" << std::endl;
}

mu2e::DetectorDump::~DetectorDump() {}

void mu2e::DetectorDump::analyze(art::Event const& evt)
{
	art::EventNumber_t eventNumber = evt.event();
	TRACE(11, "mu2e::DetectorDump::analyze enter eventNumber=%d", eventNumber);

	art::Handle<artdaq::Fragments> raw;
	//  evt.getByLabel(raw_data_label_, "CAL", raw);
	evt.getByLabel(raw_data_label_, fragment_type_string_, raw);
	//  evt.getByLabel("CAL",raw);
	//  evt.Print();

	if (raw.isValid())
	{
		std::cout << "######################################################################" << std::endl;
		std::cout << std::endl;
		std::cout << std::dec << "Run " << evt.run() << ", subrun " << evt.subRun()
		             << ", event " << eventNumber << " has " << raw->size()
		             << " DetectorFragment(s)" << std::endl;


		//    // Remove events outside the window
		//    int window = 10;
		//    std::time_t initial_time = std::time(0);
		//    int initial_epoch = initial_time;
		//    int min_epoch = initial_epoch - window;
		//    cl.coll.delete_many(document{} << "type" << "event" << "epoch" << mongocxx::LT << min_epoch << finalize);


		for (size_t idx = 0; idx < raw->size(); ++idx)
		{
			const auto& frag((*raw)[idx]);

			DetectorFragment* bb;
			switch (fragment_type_)
			{
			case mu2e::FragmentType::TRK:
				bb = new TrackerFragmentReader(frag);
				break;
			case mu2e::FragmentType::CAL:
				{
					bb = new CalorimeterFragmentReader(frag);

					std::time_t t = std::time(0); // t is an integer type
					int epoch = t; // Seconds since 01-Jan-1970

					for (size_t blockIdx = 0; blockIdx < bb->numDataBlocks(); blockIdx++)
					{
						bb->setDataBlockIndex(blockIdx);

						uint16_t CAL_crystalID = (dynamic_cast<CalorimeterFragmentReader*>(bb))->crystalID();
						uint16_t CAL_apdID = (dynamic_cast<CalorimeterFragmentReader*>(bb))->apdID();
						uint16_t CAL_time = (dynamic_cast<CalorimeterFragmentReader*>(bb))->time();
						uint16_t CAL_numSamples = (dynamic_cast<CalorimeterFragmentReader*>(bb))->numSamples();

						uint16_t HEADER_rocID = bb->rocID();
						uint16_t HEADER_ringID = bb->ringID();
						uint16_t HEADER_packetType = bb->packetType();

						std::vector<mu2e::DetectorFragment::adc_t> CAL_adcVec = (dynamic_cast<CalorimeterFragmentReader*>(bb))->calorimeterADC();
						int CAL_ADCintegral = 0;
						for (size_t i = 0; i < CAL_adcVec.size(); i++)
						{
							CAL_ADCintegral += CAL_adcVec[i];
						}

						auto event_doc = document{}
							<< "epoch" << epoch
							   << "type" << "event"
							   //	        << "system"                  << "CAL"

							   << "CAL.crystalID" << CAL_crystalID
							   << "CAL.apdID" << CAL_apdID
							   << "CAL.time" << CAL_time
							   << "CAL.numSamples" << CAL_numSamples
							   << "CAL.ADCintegral" << CAL_ADCintegral

							   << "HEADER.rocID" << HEADER_rocID
							   << "HEADER.ringID" << HEADER_ringID
							   << "HEADER.packetType" << HEADER_packetType

							   << finalize;

						cl.coll.insert_one(event_doc.view());
					}
				}
				break;
			case mu2e::FragmentType::CRV:
				bb = new CosmicVetoFragmentReader(frag);
				break;
			default:
				bb = new TrackerFragmentReader(frag);
			};
			std::cout << "numDataBlocks: " << bb->numDataBlocks() << std::endl;

			int count = cl.coll.count({});
			if (count % 1000 == 0) std::cout << "DBcount: " << count << std::endl;

			//      bb->printAll();


			//      if (frag.hasMetadata()) {
			//      std::cout << std::endl;
			//	std::cout << "DetectorFragment SimMode: ";
			//	DetectorFragment::Metadata const* md = frag.metadata<DetectorFragment::Metadata>();
			//        std::cout << DTCLib::DTC_SimModeConverter((DTCLib::DTC_SimMode)(md->sim_mode)).toString() << std::endl;
			//	std::cout << "Timestamp: 0x" << std::hex << bb->hdr_timestamp() << std::endl;
			//	//	std::cout << "Hostname: " << std::string(md->hostname) << std::endl;
			//	std::cout << std::endl;
			//      }

			//      std::cout << "DTC_DataHeaderPacket:" << std::endl;
			//      //      auto dhpacket = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket((uint8_t*)*(bb->dataBegin())));
			//      auto dhpacket = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket((uint16_t*)*(bb->dataBegin())));
			//      if(print_json_) { std::cout << dhpacket.toJSON() << std::endl; }
			//      if(print_packet_format_) {std::cout << dhpacket.toPacketFormat() << std::endl; }
			//      std::cout << std::endl;

			//      std::cout << std::endl;
			//      int packetMax = max_DataPackets_to_show_ > bb->hdr_packet_count() ? bb->hdr_packet_count() : max_DataPackets_to_show_;
			//      for(int ii = 0; ii < packetMax; ++ii) {
			//	auto packet = DTCLib::DTC_DataPacket((uint8_t*)*(bb->dataBegin() + 1 + ii));
			//	if(print_json_) { std::cout << packet.toJSON() << std::endl; }
			//	if(print_packet_format_) {std::cout << packet.toPacketFormat() << std::endl; }
			//	std::cout << std::endl;
			//      }

			delete bb;
		} // End of for loop
	}
	else
	{
		std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()
		             << ", event " << eventNumber << " has zero"
		             << " DetectorFragments." << std::endl;
	}
	std::cout << std::endl;
}

DEFINE_ART_MODULE(mu2e::DetectorDump)

