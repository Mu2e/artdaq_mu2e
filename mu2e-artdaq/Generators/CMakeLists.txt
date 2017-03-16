simple_plugin(DTCReceiver "generator"
  mu2e-artdaq-core_Overlays
  artdaq_Application
  artdaq_DAQrate
  ${CANVAS_LIB_LIST}
  ${FHICLCPP}
  ${CETLIB}
  MF_MessageLogger
  DTCInterface
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
  MF_MessageLogger
  DTCInterface
  )

simple_plugin(OverlayTest "generator"
  mu2e-artdaq-core_Overlays
  artdaq_Application
  artdaq_DAQrate
  ${CANVAS_LIB_LIST}
  ${FHICLCPP}
  ${CETLIB}
  MF_MessageLogger
  DTCInterface
  )

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
