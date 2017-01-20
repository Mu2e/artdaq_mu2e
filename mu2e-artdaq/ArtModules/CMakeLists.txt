art_make(NO_PLUGINS # WAnt different link lists for modues
  BASENAME_ONLY
  LIBRARY_NAME mu2e-artdaq_ArtModules
  LIB_LIBRARIES mu2e-artdaq-core_Overlays
)

simple_plugin(Mu2eProducer "module"
  mu2e-artdaq-core_Overlays
  DTCInterface
  ${Boost_SYSTEM_LIBRARY}
  ${MONGODB_CXX_LIBRARIES}
)

simple_plugin(DTCDataVerifier "module"
  mu2e-artdaq-core_Overlays
  DTCInterface
  ${MF_MESSAGELOGGER}
)

simple_plugin(Mu2eDump "module"
  mu2e-artdaq-core_Overlays
  DTCInterface
  ${Boost_SYSTEM_LIBRARY}
  ${MONGODB_CXX_LIBRARIES}
)

simple_plugin(OfflineFragmentsDumper "module"
  mu2e-artdaq-core_Overlays
)

simple_plugin(DTCDump "module"
  mu2e-artdaq-core_Overlays
  DTCInterface
  ${Boost_SYSTEM_LIBRARY}
  ${MONGODB_CXX_LIBRARIES}
)

simple_plugin(DetectorDump "module"
  mu2e-artdaq-core_Overlays
  DTCInterface
  ${Boost_SYSTEM_LIBRARY}
  ${MONGODB_CXX_LIBRARIES}
)

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

cet_make_library(LIBRARY_NAME mu2e_artdaq_ArtModules_detail
  SOURCE
  detail/CurrentFragment.cc
  LIBRARIES
  artdaq-core_Core
  artdaq-core_Data
  canvas_Utilities
  cetlib
  DTCInterface
  )

simple_plugin(OfflineFragmentReader "source"
  MF_MessageLogger
  mu2e_artdaq_ArtModules_detail
  )

# Subdirectories.
add_subdirectory(fcl)

install_headers()
install_source()
