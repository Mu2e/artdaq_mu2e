#include "mu2e-artdaq/Generators/RootFile.hh"

#include "art/Utilities/Exception.h"
#include "artdaq/Application/GeneratorMacros.hh"
#include "cetlib/exception.h"
#include "mu2e-artdaq/Overlays/ToyFragment.hh"
#include "mu2e-artdaq/Overlays/ToyFragmentWriter.hh"
#include "mu2e-artdaq/Overlays/FragmentType.hh"
#include "fhiclcpp/ParameterSet.h"
#include "artdaq/Utilities/SimpleLookupPolicy.h"
#include "TTree.h"
#include "TSystem.h"
#include "Cintex/Cintex.h"
#include "trace.h"		// TRACE

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



mu2e::RootFile::RootFile(fhicl::ParameterSet const & ps)
  :
  CommandableFragmentGenerator(ps),
  nADCcounts_(ps.get<size_t>("nADCcounts", 600000)),
  fragment_type_(toFragmentType(ps.get<std::string>("fragment_type"))),
  throttle_usecs_(ps.get<size_t>("throttle_usecs", 0)),
  fragment_ids_{ static_cast<artdaq::Fragment::fragment_id_t>(fragment_id() ) },
  engine_(ps.get<int64_t>("random_seed", 314159)),
  uniform_distn_(new std::uniform_int_distribution<int>(0, pow(2, typeToADC( fragment_type_ ) ) - 1 ))
{

  // Check and make sure that the fragment type will be one of the "toy" types
  
  std::vector<artdaq::Fragment::type_t> const ftypes = 
    {FragmentType::TOY1, FragmentType::TOY2 };

  if (std::find( ftypes.begin(), ftypes.end(), fragment_type_) == ftypes.end() ) {
    throw cet::exception("Error in RootFile: unexpected fragment type supplied to constructor");
  }

  artdaq::Fragment *frag = new artdaq::Fragment;
  ROOT::Cintex::Cintex::Enable();
  //gSystem->Load("libartdaq-core_Data_dict.so");
  file_=new TFile("artdaqdemo_eb02_r000101_sr01.root");
  TRACE( 1, "hello file_=%p %p", (void*)file_, (void*)frag );
  if (file_)
  {   TTree * tree;
      file_->GetObject("Events",tree);
      if (tree)
	  tree->Print("all");
      else
	  std::cout << "There is no TTree object named \"Events\"\n";
  }
}


bool mu2e::RootFile::getNext_(artdaq::FragmentPtrs & frags) {

  if (should_stop()) {
    return false;
  }

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
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::RootFile) 
