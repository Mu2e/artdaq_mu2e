
cet_build_plugin(DTCEventDump art::module LIBRARIES REG
 artdaq_core_mu2e::artdaq-core-mu2e_Overlays
)

#cet_build_plugin(DTCEventVerifier art::module LIBRARIES REG
# artdaq_core_mu2e::artdaq-core-mu2e_Overlays
# artdaq::DAQdata
#)


cet_build_plugin(OfflineFragmentsDumper art::module LIBRARIES REG
 artdaq_core_mu2e::artdaq-core-mu2e_Overlays
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
