simple_plugin(DTCReceiver "generator"
  mu2e-artdaq-core_Overlays
  artdaq_DAQdata
  artdaq_Application
  artdaq_DAQrate
  ${CANVAS_LIB_LIST}
  ${FHICLCPP}
  ${CETLIB}
  ${CETLIB_EXCEPT}
  MF_MessageLogger
  DTCInterface
  ${Boost_SYSTEM_LIBRARY}
  )

simple_plugin(Mu2eReceiver "generator"
  mu2e-artdaq-core_Overlays
  artdaq-utilities_Plugins
  artdaq-core_Utilities
  artdaq_Application
  artdaq_DAQdata
  artdaq_DAQrate
  artdaq-core_Data
  ${CANVAS_LIB_LIST}
  ${FHICLCPP}
  ${CETLIB}
  ${CETLIB_EXCEPT}
  MF_MessageLogger
  DTCInterface
  ${Boost_SYSTEM_LIBRARY}
  )

simple_plugin(OverlayTest "generator"
  mu2e-artdaq-core_Overlays
  artdaq_Application
  artdaq_DAQdata
  artdaq_DAQrate
  ${CANVAS_LIB_LIST}
  ${FHICLCPP}
  ${CETLIB}
  ${CETLIB_EXCEPT}
  MF_MessageLogger
  DTCInterface
  ${Boost_SYSTEM_LIBRARY}
  )

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
