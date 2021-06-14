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
  artdaq::Generators
  mu2e_artdaq_core::mu2e-artdaq-core_Overlays
  canvas
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
