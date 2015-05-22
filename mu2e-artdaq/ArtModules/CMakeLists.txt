art_make(NO_PLUGINS # WAnt different link lists for modues
  BASENAME_ONLY
  LIBRARY_NAME mu2e-artdaq_ArtModules
  LIB_LIBRARIES mu2e-artdaq-core_Overlays
)

simple_plugin(ToyDump "module")

simple_plugin(Mu2eArtdaqBuildInfo "module" 
  artdaq_DAQdata
  artdaq_BuildInfo
  artdaq-core_BuildInfo
  mu2e-artdaq_BuildInfo
  mu2e-artdaq-core_BuildInfo
  ${MF_MESSAGELOGGER}
  ${FHICLCPP}
  ${CETLIB}
)

# Subdirectories.
add_subdirectory(DQM)
