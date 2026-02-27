
cet_build_plugin(DTCEventDump art::module LIBRARIES REG
 artdaq-core-mu2e::Overlays
)

cet_build_plugin(CFODump art::module LIBRARIES REG
 artdaq-core-mu2e::Overlays
)

cet_build_plugin(DTCEventVerifier art::module LIBRARIES REG
 artdaq-core-mu2e::Data_dict
 artdaq-core-mu2e::Overlays
 artdaq::DAQdata
)


cet_build_plugin(OfflineFragmentsDumper art::module LIBRARIES REG
 artdaq-core-mu2e::Overlays
)

cet_build_plugin(Mu2eArtdaqBuildInfo art::module LIBRARIES REG
  artdaq-mu2e::BuildInfo
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
