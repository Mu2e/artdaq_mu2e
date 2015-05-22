#include "mu2e-artdaq/Generators/ToySimulator.hh"

#include "art/Utilities/Exception.h"
#include "artdaq/Application/GeneratorMacros.hh"
#include "cetlib/exception.h"
#include "mu2e-artdaq-core/Overlays/ToyFragment.hh"
#include "mu2e-artdaq-core/Overlays/ToyFragmentWriter.hh"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "fhiclcpp/ParameterSet.h"
#include "artdaq-core/Utilities/SimpleLookupPolicy.h"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <iostream>

#include <unistd.h>

namespace {

  size_t typeToADC(mu2e::FragmentType type)
  {
    switch (type) {
    case mu2e::FragmentType::TOY1:
      return 12;
      break;
    case mu2e::FragmentType::TOY2:
      return 14;
      break;
    default:
      throw art::Exception(art::errors::Configuration)
        << "Unknown board type "
        << type
        << " ("
        << mu2e::fragmentTypeToString(type)
        << ").\n";
    };
  }

}



mu2e::ToySimulator::ToySimulator(fhicl::ParameterSet const & ps)
  :
  CommandableFragmentGenerator(ps),
  nADCcounts_(ps.get<size_t>("nADCcounts", 600000)),
  fragment_type_(toFragmentType(ps.get<std::string>("fragment_type"))),
  throttle_usecs_(ps.get<size_t>("throttle_usecs", 0)),
  fragment_ids_{ static_cast<artdaq::Fragment::fragment_id_t>(fragment_id() ) },
  engine_(ps.get<int64_t>("random_seed", 314159)),
  uniform_distn_(new std::uniform_int_distribution<int>(0, pow(2, typeToADC( fragment_type_ ) ) - 1 )),
  events_read_(0),
  isSimulatedDTC(false)
//  file_(0),
//  eventTree_(0),
//  wrapper_(0)
{
  theInterface = new DTCLib::DTC();
  theInterface->SetMaxROCNumber(DTCLib::DTC_Ring_0, DTCLib::DTC_ROC_5);
  theInterface->EnableRing(DTCLib::DTC_Ring_1, DTCLib::DTC_RingEnableMode(), DTCLib::DTC_ROC_5);
  // Check and make sure that the fragment type will be one of the "toy" types
  
  std::vector<artdaq::Fragment::type_t> const ftypes = 
    {FragmentType::TOY1, FragmentType::TOY2 };

  if (std::find( ftypes.begin(), ftypes.end(), fragment_type_) == ftypes.end() ) {
    throw cet::exception("Error in ToySimulator: unexpected fragment type supplied to constructor");
  }
    
}

mu2e::ToySimulator::~ToySimulator()
{
  delete theInterface;
}

bool mu2e::ToySimulator::getNext_(artdaq::FragmentPtrs & frags) {

  if (should_stop()) {
    return false;
  }

  
    //theInterface = new DTC::DTC();

    isSimulatedDTC = theInterface->ReadSimMode();
    if(isSimulatedDTC) {
      std::cout << "ALERT: READING SIMULATED DTC" << std::endl;
    } else {
      std::cout << "ALERT: READING ACTUAL DTC" << std::endl;
    }

    //    std::vector<void*> GetData(const DTC_Ring_ID& ring, const DTC_ROC_ID& roc, const DTC_Timestamp& when, int* length);
    //DTC::DTC_Ring_ID ring     = DTC::DTC_Ring_0;
    //DTC::DTC_ROC_ID  roc      = DTC::DTC_ROC_0;
    //DTC::DTC_Timestamp tstamp((uint64_t)0);
    
    std::cout << "ALERT: BEFORE GETTING DATA" << std::endl << std::flush;
    std::vector<void*> data = theInterface->GetData( (uint64_t)0 );    
    for(size_t i = 0; i < data.size(); ++i)
    {
      std::cout << "Dumping DataHeaderPacket: " << std::endl;
      DTCLib::DTC_DataHeaderPacket packet = DTCLib::DTC_DataHeaderPacket(DTCLib::DTC_DataPacket(data[i]));
      std::cout << packet.toJSON() << std::endl;
    }
    std::cout << "ALERT: DONE GETTING DATA" << std::flush << std::endl;

 
  std::cout << "ALERT: DONE WITH DTC CODE" << std::endl;



   





  usleep( throttle_usecs_ );

  // Set fragment's metadata

  ToyFragment::Metadata metadata;
  metadata.board_serial_number = 999;
  metadata.num_adc_bits = typeToADC(fragment_type_);

  // And use it, along with the artdaq::Fragment header information
  // (fragment id, sequence id, and user type) to create a fragment

  // Constructor used is: 

  // Fragment(std::size_t payload_size, sequence_id_t sequence_id,
  //  fragment_id_t fragment_id, type_t type, const T & metadata);

  // ...where we'll start off setting the payload (data after the
  // header and metadata) to empty; this will be resized below

  frags.emplace_back( new artdaq::Fragment(0, ev_counter(), fragment_ids_[0],
  					    fragment_type_, metadata) );

  // Then any overlay-specific quantities next; will need the
  // ToyFragmentWriter class's setter-functions for this

  ToyFragmentWriter newfrag(*frags.back());

  newfrag.set_hdr_run_number(999);

  newfrag.resize(nADCcounts_);

  // And generate nADCcounts ADC values ranging from 0 to max with an
  // equal probability over the full range (a specific and perhaps
  // not-too-physical example of how one could generate simulated
  // data)

  std::generate_n(newfrag.dataBegin(), nADCcounts_,
  		  [&]() {
  		    return static_cast<ToyFragment::adc_t>
  		      ((*uniform_distn_)( engine_ ));
  		  }
  		  );

  // Check and make sure that no ADC values in this fragment are
  // larger than the max allowed

  newfrag.fastVerify( metadata.num_adc_bits );

  ev_counter_inc();

  return true;
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::ToySimulator) 
