#include "art/Framework/IO/Sources/Source.h"
#include "mu2e_artdaq/ArtModules/detail/RawEventQueueReader.hh"
#include "art/Framework/Core/InputSourceMacros.h"

#include <string>
using std::string;

namespace mu2e {

  typedef art::Source<detail::RawEventQueueReader> Mu2eInput;
}

DEFINE_ART_INPUT_SOURCE(mu2e::Mu2eInput)
