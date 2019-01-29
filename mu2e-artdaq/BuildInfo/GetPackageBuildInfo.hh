#ifndef mu2e_artdaq_BuildInfo_GetPackageBuildInfo_hh
#define mu2e_artdaq_BuildInfo_GetPackageBuildInfo_hh

#include "artdaq-core/Data/PackageBuildInfo.hh"

#include <string>

namespace mu2e {
struct GetPackageBuildInfo {
  static artdaq::PackageBuildInfo getPackageBuildInfo();
};
}  // namespace mu2e

#endif /* mu2e_artdaq_BuildInfo_GetPackageBuildInfo_hh */
