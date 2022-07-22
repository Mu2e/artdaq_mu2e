
cet_make_library(LIBRARY_NAME ArtModules
   SOURCE
   detail/CurrentFragment.cc
   LIBRARIES PUBLIC
   artdaq_core_mu2e::Overlays
   canvas::canvas
  )

cet_build_plugin(DTCDataVerifier art::module LIBRARIES REG
 artdaq_core_mu2e::Overlays
 artdaq::DAQdata
)

cet_build_plugin(Mu2eDump art::module LIBRARIES REG
 artdaq_core_mu2e::Overlays
)

cet_build_plugin(OfflineFragmentsDumper art::module LIBRARIES REG
 artdaq_core_mu2e::Overlays
)

cet_build_plugin(DTCDump art::module LIBRARIES REG
 artdaq_core_mu2e::Overlays
)

cet_build_plugin(Mu2eArtdaqBuildInfo art::module LIBRARIES REG
  artdaq_mu2e::artdaq-mu2e_BuildInfo
)

cet_build_plugin(OfflineFragmentReader art::source LIBRARIES REG artdaq::DAQdata
  artdaq_plugin_types::ArtdaqSharedMemoryService
 artdaq_mu2e::ArtModules
)

cet_build_plugin(Mu2eArtdaqTimeTrackerService art::service LIBRARIES REG
cetlib::sqlite
artdaq::DAQdata
art::Framework_Principal
          )
  

# Subdirectories.
add_subdirectory(fcl)

install_headers()
install_source()
