#include "artdaq/ArtModules/BuildInfo_module.hh"

#include "artdaq-core/BuildInfo/GetPackageBuildInfo.hh"
#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"
#include "mu2e-artdaq/BuildInfo/GetPackageBuildInfo.hh"

#include <string>

namespace mu2e {
static std::string instanceName = "Mu2eArtdaqBuildInfo";
typedef artdaq::BuildInfo<&instanceName, artdaqcore::GetPackageBuildInfo, artdaq::GetPackageBuildInfo,
                          mu2e::GetPackageBuildInfo>
    Mu2eArtdaqBuildInfo;

DEFINE_ART_MODULE(Mu2eArtdaqBuildInfo)
}  // namespace mu2e
