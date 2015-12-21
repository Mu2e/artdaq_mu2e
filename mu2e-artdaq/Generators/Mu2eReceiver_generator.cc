#include "mu2e-artdaq/Generators/Mu2eReceiver.hh"
#include "mu2e-artdaq/Generators/DTCReceiver.hh"

#include "art/Utilities/Exception.h"
#include "artdaq/Application/GeneratorMacros.hh"
#include "cetlib/exception.h"
#include "mu2e-artdaq-core/Overlays/DTCFragment.hh"
#include "mu2e-artdaq-core/Overlays/DTCFragmentWriter.hh"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"
#include "mu2e-artdaq-core/Overlays/mu2eFragmentWriter.hh"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "fhiclcpp/ParameterSet.h"
#include "artdaq-core/Utilities/SimpleLookupPolicy.h"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <iostream>

#include <unistd.h>

#define TRACE_NAME "MU2EDEV"

mu2e::Mu2eReceiver::Mu2eReceiver(fhicl::ParameterSet const & ps)
	: CommandableFragmentGenerator(ps)
	, fragment_type_(toFragmentType("MU2E"))
	, fragment_ids_{ static_cast<artdaq::Fragment::fragment_id_t>(fragment_id()) }
	, fragments_read_(0)
    , mode_(DTCLib::DTC_SimMode_Disabled)
	, board_id_(static_cast<uint8_t>(ps.get<int>("board_id", 0)))
    , theReceiver_(new DTCReceiver(ps))
{
  mode_ = theReceiver_->GetMode();
}

mu2e::Mu2eReceiver::~Mu2eReceiver()
{
  delete theReceiver_;
}

bool mu2e::Mu2eReceiver::getNext_(artdaq::FragmentPtrs & frags)
{
	if (should_stop())
	{
		return false;
	}

	// Set fragment's metadata
	mu2eFragment::Metadata metadata;
	metadata.sim_mode = static_cast<int>(mode_);
	metadata.run_number = run_number();
	metadata.board_id = board_id_;

	// And use it, along with the artdaq::Fragment header information
	// (fragment id, sequence id, and user type) to create a fragment

	// Constructor used is:

	// Fragment(std::size_t payload_size, sequence_id_t sequence_id,
	//  fragment_id_t fragment_id, type_t type, const T & metadata);

	// ...where we'll start off setting the payload (data after the
	// header and metadata) to empty; this will be resized below

	frags.emplace_back(new artdaq::Fragment(0, ev_counter(), fragment_ids_[0],
											fragment_type_, metadata));

	// Now we make an instance of the overlay to put the data into...
	mu2eFragmentWriter newfrag(*frags.back());
    newfrag.set_hdr_fragment_type(theReceiver_->GetFragmentType());
	artdaq::FragmentPtrs* ptrs = new artdaq::FragmentPtrs();

    int count = 0;
    while(count < mu2e::DTC_FRAGMENT_MAX) {
      if(should_stop()) { break; }
      bool sts = theReceiver_->getNextDTCFragment(*ptrs);
      if(!sts) break;
      count++;
	}
    newfrag.addDTCFragments(ptrs, count);

	ev_counter_inc();

	return true;
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::Mu2eReceiver)
