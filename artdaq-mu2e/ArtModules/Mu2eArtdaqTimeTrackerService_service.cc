#include "artdaq-mu2e/ArtModules/ArtdaqTimeTrackerService.hh"

#include "TRACE/tracemf.h"
#define TRACE_NAME "Mu2eArtdaqTimeTrackerService"

#include "art/Framework/Principal/Event.h"
#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Registry/ServiceMacros.h"
#include "art/Framework/Services/Registry/ServiceTable.h"
#include "art/Framework/Services/System/DatabaseConnection.h"
#include "art/Persistency/Provenance/ModuleContext.h"
#include "art/Persistency/Provenance/ModuleDescription.h"
#include "art/Persistency/Provenance/PathContext.h"
#include "art/Persistency/Provenance/ScheduleContext.h"
#include "art/Utilities/Globals.h"
#include "art/Utilities/PerScheduleContainer.h"
#include "boost/format.hpp"
#include "canvas/Persistency/Provenance/EventID.h"
#include "cetlib/HorizontalRule.h"
#include "cetlib/sqlite/Connection.h"
#include "cetlib/sqlite/Ntuple.h"
#include "cetlib/sqlite/helpers.h"
#include "cetlib/sqlite/statistics.h"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Name.h"
#include "fhiclcpp/types/Table.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "tbb/concurrent_unordered_map.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace art;
using namespace cet;
using namespace hep::concurrency;

using std::chrono::steady_clock;

/**
 * \brief Mu2eArtdaqTimeTrackerService extends ArtdaqTimeTrackerService.
 * This implementation uses artdaq-core-mu2e's FragmentTypeMap and directly assigns names based on it
 */
using Parameters = ServiceTable<ArtdaqTimeTrackerServiceInterface::Config>;

class Mu2eArtdaqTimeTrackerService : public ArtdaqTimeTrackerServiceInterface
{
public:
  /**
   * \brief DefaultArtdaqTimeTrackerService Destructor
   */
  virtual ~Mu2eArtdaqTimeTrackerService() = default;

  /**
   * \brief Mu2eArtdaqTimeTrackerService Constructor
   */
  Mu2eArtdaqTimeTrackerService(Parameters const&, art::ActivityRegistry&);

private:
};

Mu2eArtdaqTimeTrackerService::Mu2eArtdaqTimeTrackerService(Parameters const& config, ActivityRegistry& areg)
  :  ArtdaqTimeTrackerServiceInterface(config, areg)
{
  TLOG(TLVL_DEBUG) << "Mu2eArtdaqTimeTrackerService CONSTRUCTOR START";
  //SetBasicTypes(mu2e::makeFragmentTypeMap());
  TLOG(TLVL_DEBUG) << "Mu2eArtdaqTimeTrackerService CONSTRUCTOR END";
}

DECLARE_ART_SERVICE_INTERFACE_IMPL(Mu2eArtdaqTimeTrackerService, ArtdaqTimeTrackerServiceInterface , LEGACY)
DEFINE_ART_SERVICE_INTERFACE_IMPL(Mu2eArtdaqTimeTrackerService, ArtdaqTimeTrackerServiceInterface)


