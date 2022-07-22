#ifndef artdaq_mu2e_BuildInfo_GetPackageBuildInfo_hh
#define artdaq_mu2e_BuildInfo_GetPackageBuildInfo_hh

#include "artdaq-core/Data/PackageBuildInfo.hh"

#include <string>

namespace mu2e {
struct GetPackageBuildInfo
{
	static artdaq::PackageBuildInfo getPackageBuildInfo();
};
}  // namespace mu2e

#endif /* artdaq_mu2e_BuildInfo_GetPackageBuildInfo_hh */
