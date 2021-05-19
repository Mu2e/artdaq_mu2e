art_make(NO_PLUGINS # WAnt different link lists for modues
  BASENAME_ONLY
  LIBRARY_NAME mu2e-artdaq_ArtModules
 SUBDIRS detail
 EXCLUDE  detail/CurrentFragment.cc
  NO_PLUGINS
  DICT_DICTIONARIES
  ${ART_PERSISTENCY_PROVENANCE}
  ${CANVAS_PERSISTENCY_PROVENANCE}
 LIB_LIBRARIES 
  
)

simple_plugin(DTCDataVerifier "module"
 artdaq::DAQdata
 mu2e_artdaq_core::mu2e-artdaq-core_Overlays
)

set(MONGODB_CXX_LIBRARIES $ENV{MONGODB_LIB}/libbsoncxx.so $ENV{MONGODB_LIB}/libmongocxx.so)

simple_plugin(Mu2eDump "module"
 mu2e_artdaq_core::mu2e-artdaq-core_Overlays
)

simple_plugin(OfflineFragmentsDumper "module"
 mu2e_artdaq_core::mu2e-artdaq-core_Overlays
)

simple_plugin(DTCDump "module"
 mu2e_artdaq_core::mu2e-artdaq-core_Overlays
)

simple_plugin(Mu2eArtdaqBuildInfo "module"
  mu2e-artdaq_BuildInfo
)

cet_make_library(LIBRARY_NAME mu2e_artdaq_ArtModules_detail
   SOURCE
   detail/CurrentFragment.cc
   LIBRARIES
   artdaq_core::artdaq-core_Data
   artdaq::DAQdata
   mu2e_artdaq_core::mu2e-artdaq-core_Overlays
   canvas
  )

simple_plugin(OfflineFragmentReader "source"
  mu2e_artdaq_ArtModules_detail
)

simple_plugin(Mu2eArtdaqTimeTrackerService "service"
	     artdaq::DAQdata
         art::Framework_Principal
              ${TBB}
              ${SQLITE3}
          )
  

# Subdirectories.
add_subdirectory(fcl)

install_headers()
install_source()
