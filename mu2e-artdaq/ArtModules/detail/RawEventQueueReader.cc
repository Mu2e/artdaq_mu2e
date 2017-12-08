#include "mu2e-artdaq/ArtModules/detail/RawEventQueueReader.hh"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "artdaq/DAQdata/Globals.hh"

#include <messagefacility/MessageLogger/MessageLogger.h>

mu2e::detail::RawEventQueueReader::RawEventQueueReader(fhicl::ParameterSet const & ps,
													   art::ProductRegistryHelper & help,
													   art::SourceHelper const & pm) :
	artdaq::detail::RawEventQueueReader(ps, help, pm)
{
	TLOG_INFO("RawEventQueueReader") << "Mu2eInput Constructor!" << TLOG_ENDL;
	for (auto& name : names)
	{
		TLOG_INFO("RawEventQueueReader") << "Adding fragment type " << name << " to fragment_type_map, and registering with the ProductRegistryHelper" << TLOG_ENDL;
		fragment_type_map_[toFragmentType(name)] = name;
		help.reconstitutes<artdaq::Fragments, art::InEvent>(pretend_module_name, name);
	}
}
