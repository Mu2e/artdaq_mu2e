///////////////////////////////////////////////////////////////////////////////////////
// Class:       DTCEventDump
// Module Type: EDAnalyzer
// File:        DTCEventDump_module.cc
// Description: Prints out DTCFragments in HWUG Packet format (see mu2e-docdb #4097)
///////////////////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "canvas/Utilities/Exception.h"

#include "artdaq-core/Data/Fragment.hh"
#include "dtcInterfaceLib/DTC_Packets.h"
#include "artdaq-core-mu2e/Overlays/FragmentType.hh"
#include "artdaq-core-mu2e/Overlays/DTCEventFragment.hh"
#include "artdaq-core/Data/ContainerFragment.hh"

#include "trace.h"

#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <string>

namespace mu2e {
class DTCEventDump;
}

class mu2e::DTCEventDump : public art::EDAnalyzer
{
public:
	explicit DTCEventDump(fhicl::ParameterSet const& pset);
	virtual ~DTCEventDump();

	virtual void analyze(art::Event const& evt);

	virtual void beginJob();
	virtual void endJob();

private:
	std::string binary_file_name_;
	bool detemu_format_;
	std::ofstream output_file_;
};

mu2e::DTCEventDump::DTCEventDump(fhicl::ParameterSet const& pset)
	: EDAnalyzer(pset)
	, binary_file_name_(pset.get<std::string>("raw_output_file", "DTCEventDump.bin"))
	, detemu_format_(pset.get<bool>("raw_output_in_detector_emulator_format", false))
{}

mu2e::DTCEventDump::~DTCEventDump() {}

void mu2e::DTCEventDump::endJob()
{
	output_file_.close();
}

void mu2e::DTCEventDump::beginJob()
{
		std::string fileName = binary_file_name_;
		if (fileName.find(".bin") != std::string::npos)
		{
			std::string timestr = "_" + std::to_string(time(0));
			fileName.insert(fileName.find(".bin"), timestr);
		}
		output_file_.open(fileName, std::ios::out | std::ios::app | std::ios::binary);
	
}


void mu2e::DTCEventDump::analyze(art::Event const& evt)
{
	art::EventNumber_t eventNumber = evt.event();
	TRACE(11, "mu2e::DTCEventDump::analyze enter eventNumber=%d", eventNumber);
	
  
	artdaq::Fragments fragments;
	artdaq::FragmentPtrs containerFragments;

	std::vector<art::Handle<artdaq::Fragments>> fragmentHandles;
	fragmentHandles = evt.getMany<std::vector<artdaq::Fragment>>();

	for (const auto& handle : fragmentHandles)
	{
		if (!handle.isValid() || handle->empty())
		{
			continue;
		}

		if (handle->front().type() == artdaq::Fragment::ContainerFragmentType)
		{
			for (const auto& cont : *handle)
			{
				artdaq::ContainerFragment contf(cont);
				if (contf.fragment_type() != mu2e::FragmentType::DTCEVT)
				{
					break;
				}

				for (size_t ii = 0; ii < contf.block_count(); ++ii)
				{
					containerFragments.push_back(contf[ii]);
					fragments.push_back(*containerFragments.back());
				}
			}
		}
		else
		{
			if (handle->front().type() == mu2e::FragmentType::DTCEVT)
			{
				for (auto frag : *handle)
				{
					fragments.emplace_back(frag);
				}
			}
		}
	}
	// look for raw Toy data
	TLOG(TLVL_INFO) << "Run " << evt.run() << ", subrun " << evt.subRun() << ", event " << eventNumber << " has "
	                << fragments.size() << " fragment(s) of type DTCEVT";

	for (const auto& frag : fragments)
	{
		DTCEventFragment bb(frag);
		auto evt = bb.get_data();
		TLOG(TLVL_DEBUG) << "Event " << evt.GetEventWindowTag().GetEventWindowTag(true) << " has size " << evt.GetEventByteCount() << " (fragment size " << frag.sizeBytes()<< ")";

		if(output_file_) {
		
			evt.WriteEvent(output_file_, detemu_format_);
		}


	}
}

DEFINE_ART_MODULE(mu2e::DTCEventDump)
