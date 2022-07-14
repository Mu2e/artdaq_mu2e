basic_plugin(DTCReceiver "generator"
  artdaq::Generators
  artdaq_core_mu2e::artdaq-core-mu2e_Overlays
  canvas
  )

basic_plugin(Mu2eReceiver "generator"
  artdaq::Generators
  artdaq_core_mu2e::artdaq-core-mu2e_Overlays
  canvas
  )
  
basic_plugin(Mu2eEventReceiver "generator"
  artdaq::Generators
  artdaq_core_mu2e::artdaq-core-mu2e_Overlays
  canvas
  )

  basic_plugin(CRVReceiver "generator"
  artdaq::Generators
  artdaq_core_mu2e::artdaq-core-mu2e_Overlays
  canvas
  )

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
