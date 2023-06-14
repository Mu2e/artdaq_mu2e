
cet_build_plugin(DTCEventDump art::module LIBRARIES REG
 artdaq_core_mu2e::Overlays
)

cet_build_plugin(OfflineFragmentsDumper art::module LIBRARIES REG
 artdaq_core_mu2e::Overlays
)

cet_build_plugin(ArtFragmentsFromDTCEvents art::module LIBRARIES REG
 artdaq_core_mu2e::Data
 artdaq_core_mu2e::Overlays
)

cet_build_plugin(Mu2eArtdaqBuildInfo art::module LIBRARIES REG
  artdaq_mu2e::artdaq-mu2e_BuildInfo
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
