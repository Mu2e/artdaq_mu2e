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
  mu2e-artdaq-core_Overlays
  artdaq-core_Utilities
  artdaq_ArtModules
  art_Framework_Core
  art_Framework_IO_Sources
  art_Framework_Principal
  art_Persistency_Provenance
  ${CANVAS_LIB_LIST}
  MF_MessageLogger
  ${ROOT_CINTEX}
  ${ROOT_RIO}
  ${ROOT_CORE}
  ${ROOT_NET}
  ${ROOT_REFLEX}
  ${CETLIB}
  ${CETLIB_EXCEPT}
)

simple_plugin(DTCDataVerifier "module"
  mu2e-artdaq-core_Overlays
  artdaq_DAQdata
  DTCInterface
  ${MF_MESSAGELOGGER}
  ${CETLIB_EXCEPT}
)

set(MONGODB_CXX_LIBRARIES $ENV{MONGODB_LIB}/libbsoncxx.so $ENV{MONGODB_LIB}/libmongocxx.so)

simple_plugin(Mu2eDump "module"
  mu2e-artdaq-core_Overlays
  DTCInterface
  ${Boost_SYSTEM_LIBRARY}
  ${MONGODB_CXX_LIBRARIES}
  ${CETLIB_EXCEPT}
  ${MF_MESSAGELOGGER}
)

simple_plugin(OfflineFragmentsDumper "module"
  mu2e-artdaq-core_Overlays
  ${MF_MESSAGELOGGER}
  ${CETLIB_EXCEPT}
)

simple_plugin(DTCDump "module"
  mu2e-artdaq-core_Overlays
  DTCInterface
  ${Boost_SYSTEM_LIBRARY}
  ${MONGODB_CXX_LIBRARIES}
  ${CETLIB_EXCEPT}
  ${MF_MESSAGELOGGER}
)

#simple_plugin(DetectorDump "module"
#  mu2e-artdaq-core_Overlays
#  DTCInterface
#  ${Boost_SYSTEM_LIBRARY}
#  ${MONGODB_CXX_LIBRARIES}
#  ${CETLIB_EXCEPT}
#  ${MF_MESSAGELOGGER}
#)

simple_plugin(Mu2eArtdaqBuildInfo "module"
  art_Framework_Core
  art_Framework_IO_Sources
  art_Framework_Principal
  art_Persistency_Provenance
  art_Utilities
  artdaq-core_Utilities
  artdaq_DAQdata
  artdaq_BuildInfo
  artdaq-core_BuildInfo
  mu2e-artdaq_BuildInfo
  mu2e-artdaq-core_BuildInfo
  mu2e-artdaq-core_Overlays
  ${MF_MESSAGELOGGER}
  ${FHICLCPP}
  ${CETLIB}
  ${CETLIB_EXCEPT}
)

cet_make_library(LIBRARY_NAME mu2e_artdaq_ArtModules_detail
  SOURCE
  detail/CurrentFragment.cc
  LIBRARIES
  mu2e-artdaq-core_Overlays
  artdaq-core_Utilities
  artdaq-core_Core
  artdaq-core_Data
  ${CANVAS_LIB_LIST}
  ${CETLIB}
  ${CETLIB_EXCEPT}
  DTCInterface
  ${MF_MESSAGELOGGER}
  )

simple_plugin(OfflineFragmentReader "source"
  MF_MessageLogger
  artdaq_DAQdata
  mu2e_artdaq_ArtModules_detail
  )

simple_plugin(Mu2eArtdaqFragmentNamingService "service"
	      artdaq_ArtModules_ArtdaqFragmentNamingService_service
	      artdaq-core_Data
              mu2e-artdaq-core_Overlays
              art_Framework_Services_Registry
	      MF_MessageLogger
	      fhiclcpp
          )

simple_plugin(Mu2eArtdaqTimeTrackerService "service"
	      artdaq-core_Data
              artdaq_DAQdata
              artdaq_ArtModules
              mu2e-artdaq-core_Overlays
              art_Framework_Principal
              art_Framework_Services_Registry
              art_Persistency_Provenance
              art_Utilities	
               ${CANVAS_PERSISTENCY_PROVENANCE}
              ${CANVAS_LIB_LIST}
              MF_MessageLogger
              ${CETLIB_EXCEPT}
              ${CETLIB}
              ${TBB}
              hep_concurrency
              ${SQLITE3}
              cetlib
	      fhiclcpp
              ${Boost_SYSTEM_LIBRARY}
          )
  

# Subdirectories.
add_subdirectory(fcl)

install_headers()
install_source()
