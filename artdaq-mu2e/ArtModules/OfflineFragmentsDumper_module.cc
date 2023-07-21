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

#include "canvas/Utilities/Exception.h"

#include "artdaq-core/Data/Fragment.hh"

#include "trace.h"

#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <string>

namespace mu2e {
class Mu2eDump;
}

class mu2e::Mu2eDump : public art::EDAnalyzer
{
public:
	explicit Mu2eDump(fhicl::ParameterSet const& pset);
	virtual void analyze(art::Event const& evt);

private:
	art::InputTag trkFragmentsTag_;
	art::InputTag caloFragmentsTag_;
	art::InputTag crvFragmentsTag_;
};

mu2e::Mu2eDump::Mu2eDump(fhicl::ParameterSet const& pset)
	: EDAnalyzer{pset}, trkFragmentsTag_{"daq:trk"}, caloFragmentsTag_{"daq:calo"}, crvFragmentsTag_{"daq:crv"} {}
void mu2e::Mu2eDump::analyze(art::Event const& e)
{
	auto trkFragments = e.getValidHandle<artdaq::Fragments>(trkFragmentsTag_);
	auto calFragments = e.getValidHandle<artdaq::Fragments>(caloFragmentsTag_);
	auto crvFragments = e.getValidHandle<artdaq::Fragments>(crvFragmentsTag_);
	std::cout << "Event " << e.event() << " has ";
	std::cout << trkFragments->size() << " TRK fragments, and ";
	std::cout << calFragments->size() << " CAL fragments." << std::endl;
	std::cout << crvFragments->size() << " CRV fragments." << std::endl;
	size_t totalSize = 0;
	for (size_t idx = 0; idx < trkFragments->size(); ++idx) {
		auto size = ((*trkFragments)[idx]).size() * sizeof(artdaq::RawDataType);
		totalSize += size;
		std::cout << "\tTRK Fragment " << idx << " has size " << size << std::endl;
	}
	for (size_t idx = 0; idx < calFragments->size(); ++idx) {
		auto size = ((*calFragments)[idx]).size() * sizeof(artdaq::RawDataType);
		totalSize += size;
		std::cout << "\tCAL Fragment " << idx << " has size " << size << std::endl;
	}
	for (size_t idx = 0; idx < crvFragments->size(); ++idx) {
		auto size = ((*crvFragments)[idx]).size() * sizeof(artdaq::RawDataType);
		totalSize += size;
		std::cout << "\tCRV Fragment " << idx << " has size " << size << std::endl;
	}
	std::cout << "\tTotal Size: " << (int)totalSize << " bytes." << std::endl;
}

DEFINE_ART_MODULE(mu2e::Mu2eDump)
