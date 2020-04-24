#include "mu2e-artdaq-core/Overlays/FragmentType.hh"
#include "artdaq/ArtModules/ArtdaqFragmentNamingService.h"

#include "TRACE/tracemf.h"
#define TRACE_NAME "Mu2eArtdaqFragmentNamingService"

/**
 * \brief Mu2eArtdaqFragmentNamingService extends ArtdaqFragmentNamingService.
 * This implementation uses mu2e-artdaq-core's FragmentTypeMap and directly assigns names based on it
 */
class Mu2eArtdaqFragmentNamingService : public ArtdaqFragmentNamingService
{
public:
	/**
	 * \brief DefaultArtdaqFragmentNamingService Destructor
	 */
	virtual ~Mu2eArtdaqFragmentNamingService() = default;

	/**
	 * \brief Mu2eArtdaqFragmentNamingService Constructor
	 */
	Mu2eArtdaqFragmentNamingService(fhicl::ParameterSet const&, art::ActivityRegistry&);

private:
};

Mu2eArtdaqFragmentNamingService::Mu2eArtdaqFragmentNamingService(fhicl::ParameterSet const& ps, art::ActivityRegistry& r)
    : ArtdaqFragmentNamingService(ps, r)
{
	TLOG(TLVL_DEBUG) << "Mu2eArtdaqFragmentNamingService CONSTRUCTOR START";
	SetBasicTypes(mu2e::makeFragmentTypeMap());
	TLOG(TLVL_DEBUG) << "Mu2eArtdaqFragmentNamingService CONSTRUCTOR END";
}

DECLARE_ART_SERVICE_INTERFACE_IMPL(Mu2eArtdaqFragmentNamingService, ArtdaqFragmentNamingServiceInterface, LEGACY)
DEFINE_ART_SERVICE_INTERFACE_IMPL(Mu2eArtdaqFragmentNamingService, ArtdaqFragmentNamingServiceInterface)
