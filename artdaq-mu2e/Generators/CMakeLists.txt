
cet_make_library(LIBRARY_NAME artdaq-mu2e_Generators_Mu2eReceiverBase
SOURCE 
Mu2eEventReceiverBase.cc
LIBRARIES PUBLIC
artdaq::DAQdata
canvas::canvas
artdaq_core_mu2e::artdaq-core-mu2e_Overlays
artdaq_plugin_types::CommandableFragmentGenerator
  )

include(artdaq::commandableGenerator)
  
cet_build_plugin(Mu2eEventReceiver artdaq::commandableGenerator LIBRARIES REG artdaq_mu2e::artdaq-mu2e_Generators_Mu2eReceiverBase
  )

  cet_build_plugin(CRVReceiver artdaq::commandableGenerator LIBRARIES REG artdaq_mu2e::artdaq-mu2e_Generators_Mu2eReceiverBase)

  cet_build_plugin(STMReceiver artdaq::commandableGenerator LIBRARIES REG artdaq_mu2e::artdaq-mu2e_Generators_Mu2eReceiverBase)



#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
