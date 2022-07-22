art_make(NO_PLUGINS # WAnt different link lists for modues
  BASENAME_ONLY
  LIBRARY_NAME artdaq-mu2e_ArtModules
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
 artdaq_core_mu2e::artdaq-core-mu2e_Overlays
)

set(MONGODB_CXX_LIBRARIES $ENV{MONGODB_LIB}/libbsoncxx.so $ENV{MONGODB_LIB}/libmongocxx.so)

simple_plugin(Mu2eDump "module"
 artdaq_core_mu2e::artdaq-core-mu2e_Overlays
)

simple_plugin(OfflineFragmentsDumper "module"
 artdaq_core_mu2e::artdaq-core-mu2e_Overlays
)

simple_plugin(DTCDump "module"
 artdaq_core_mu2e::artdaq-core-mu2e_Overlays
)

simple_plugin(Mu2eArtdaqBuildInfo "module"
  artdaq-mu2e_BuildInfo
)

cet_make_library(LIBRARY_NAME artdaq_mu2e_ArtModules_detail
   SOURCE
   detail/CurrentFragment.cc
   LIBRARIES
   artdaq_core::artdaq-core_Data
   artdaq::DAQdata
   artdaq_core_mu2e::artdaq-core-mu2e_Overlays
   canvas
  )

simple_plugin(OfflineFragmentReader "source"
  artdaq_mu2e_ArtModules_detail
  art_Framework_Services_Registry
)

simple_plugin(Mu2eArtdaqTimeTrackerService "service"
	     artdaq::DAQdata
         art_Framework_Principal
         art_Utilities
              ${TBB}
              ${SQLITE3}
          )
  

# Subdirectories.
add_subdirectory(fcl)

install_headers()
install_source()
