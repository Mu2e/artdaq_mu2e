simple_plugin(DTCReceiver "generator"
  artdaq::Generators
  mu2e_artdaq_core::mu2e-artdaq-core_Overlays
  canvas
  )

simple_plugin(Mu2eReceiver "generator"
  artdaq::Generators
  mu2e_artdaq_core::mu2e-artdaq-core_Overlays
  canvas
  )
  
simple_plugin(Mu2eEventReceiver "generator"
  mu2e-artdaq-core_Overlays
  artdaq-utilities_Plugins
  artdaq-core_Utilities
  artdaq_Generators
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

  simple_plugin(CRVReceiver "generator"
  artdaq::Generators
  mu2e_artdaq_core::mu2e-artdaq-core_Overlays
  canvas
  
  )

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
