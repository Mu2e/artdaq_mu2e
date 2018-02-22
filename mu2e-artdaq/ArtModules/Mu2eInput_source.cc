#include "art/Framework/IO/Sources/Source.h"
#include "art/Framework/Core/InputSourceMacros.h"

#include "artdaq/ArtModules/detail/SharedMemoryReader.hh"
#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "art/Framework/IO/Sources/SourceTraits.h"

#include <string>
using std::string;


namespace art
{
	/**
	* \brief  Specialize an art source trait to tell art that we don't care about
	* source.fileNames and don't want the files services to be used.
	*/
	template <>
	struct Source_generator<artdaq::detail::SharedMemoryReader<mu2e::makeFragmentTypeMap>>
	{
		static constexpr bool value = true; ///< Used to suppress use of file services on art Source
	};
}

namespace mu2e {

typedef art::Source< artdaq::detail::SharedMemoryReader<mu2e::makeFragmentTypeMap> > Mu2eInput;
}

DEFINE_ART_INPUT_SOURCE(mu2e::Mu2eInput)
