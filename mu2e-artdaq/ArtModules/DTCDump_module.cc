///////////////////////////////////////////////////////////////////////////////////////
// Class:       DTCDump
// Module Type: EDAnalyzer
// File:        DTCDump_module.cc
// Description: Prints out DTCFragments in HWUG Packet format (see mu2e-docdb #4097)
///////////////////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#include "art/Utilities/Exception.h"
#include "mu2e-artdaq-core/Overlays/DTCFragment.hh"
#include "artdaq-core/Data/Fragments.hh"
#include "dtcInterfaceLib/DTC_Types.h"

#include "trace.h"

#include <string>
#include <iostream>
#include <iomanip>
#include <unistd.h>

namespace mu2e {
  class DTCDump;
}

class mu2e::DTCDump : public art::EDAnalyzer {
public:
  explicit DTCDump(fhicl::ParameterSet const & pset);
  virtual ~DTCDump();

  virtual void analyze(art::Event const & evt);

private:
  std::string raw_data_label_;
  uint16_t max_DataPackets_to_show_;
  bool print_json_;
  bool print_packet_format_;
};

mu2e::DTCDump::DTCDump(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
  , raw_data_label_(pset.get<std::string>("raw_data_label"))
  , max_DataPackets_to_show_(static_cast<uint16_t>(pset.get<int>("max_DataPackets_to_show",0)))
  , print_json_(pset.get<bool>("print_json", false))
  , print_packet_format_(pset.get<bool>("print_packet_format", true))
{}

mu2e::DTCDump::~DTCDump()
{}

void mu2e::DTCDump::analyze(art::Event const & evt)
{
  art::EventNumber_t eventNumber = evt.event();
  TRACE( 11, "mu2e::DTCDump::analyze enter eventNumber=%d", eventNumber );

  art::Handle<artdaq::Fragments> raw;
  evt.getByLabel(raw_data_label_, "DTC", raw);

  if (raw.isValid()) {
    std::cout << "######################################################################" << std::endl;
    std::cout << std::endl;
    std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()
              << ", event " << eventNumber << " has " << raw->size()
              << " DTCFragment(s)"<< std::endl;

    for (size_t idx = 0; idx < raw->size(); ++idx) {
      const auto& frag((*raw)[idx]);

      DTCFragment bb(frag);

      if (frag.hasMetadata()) {
      std::cout << std::endl;
	std::cout << "DTCFragment SimMode: ";
	DTCFragment::Metadata const* md =
          frag.metadata<DTCFragment::Metadata>();
        std::cout << DTCLib::DTC_SimModeConverter((DTCLib::DTC_SimMode)(md->sim_mode)).toString()
                  << std::endl;
	std::cout << "Timestamp: 0x" << std::hex << bb.hdr_timestamp() << std::endl;
	std::cout << std::endl;
      }
      
      std::cout << "DTC_DataHeaderPacket:" << std::endl;
      auto dhpacket = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket((uint8_t*)*(bb.dataBegin())));
	  if(print_json_) { std::cout << dhpacket.toJSON() << std::endl; }
	  if(print_packet_format_) {std::cout << dhpacket.toPacketFormat() << std::endl; }
	  std::cout << std::endl;


      std::cout << std::endl;
      int packetMax = max_DataPackets_to_show_ > bb.hdr_packet_count() ? bb.hdr_packet_count() : max_DataPackets_to_show_;
      for(int ii = 0; ii < packetMax; ++ii)
	{
	  auto packet = DTCLib::DTC_DataPacket((uint8_t*)*(bb.dataBegin() + 1 + ii));
	  if(print_json_) { std::cout << packet.toJSON() << std::endl; }
	  if(print_packet_format_) {std::cout << packet.toPacketFormat() << std::endl; }
	  std::cout << std::endl;
	}
    }
  }
  else {
    std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()
              << ", event " << eventNumber << " has zero"
              << " DTCFragments." << std::endl;
  }
  std::cout << std::endl;

}

DEFINE_ART_MODULE(mu2e::DTCDump)
