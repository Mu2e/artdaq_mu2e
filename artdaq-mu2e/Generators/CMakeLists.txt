
include(artdaq::commandableGenerator)
  
cet_build_plugin(Mu2eEventReceiver artdaq::commandableGenerator LIBRARIES REG canvas::canvas artdaq_core_mu2e::Overlays artdaq::DAQdata
  )

  cet_build_plugin(CRVReceiver artdaq::commandableGenerator LIBRARIES REG canvas::canvas artdaq_core_mu2e::Overlays artdaq::DAQdata
  )

cet_build_plugin(STMReceiver artdaq::commandableGenerator LIBRARIES REG canvas::canvas artdaq_core_mu2e::Overlays artdaq::DAQdata
  )

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
