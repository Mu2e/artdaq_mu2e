art_make(NO_PLUGINS # WAnt different link lists for modues
  LIB_LIBRARIES mu2e-artdaq_Overlays mu2e-artdaq_Products
)

simple_plugin(ToyDump "module")

simple_plugin(Mu2eArtdaqBuildInfo "module" 
  artdaq_DAQdata
  artdaq_BuildInfo
  artdaq-core_BuildInfo
  mu2e-artdaq_BuildInfo
  ${MF_MESSAGELOGGER}
  ${FHICLCPP}
  ${CETLIB}
)

# Subdirectories.
add_subdirectory(DQM)
