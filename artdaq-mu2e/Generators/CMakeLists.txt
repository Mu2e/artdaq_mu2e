
cet_make_library(LIBRARY_NAME Mu2eReceiverBase
SOURCE 
Mu2eEventReceiverBase.cc
LIBRARIES PUBLIC
artdaq::DAQdata
canvas::canvas
artdaq-core-mu2e::Overlays
mu2e-pcie-utils::DTCInterface
artdaq_plugin_types::CommandableFragmentGenerator
  )

include(artdaq::commandableGenerator)
  
cet_build_plugin(Mu2eEventReceiver artdaq::commandableGenerator LIBRARIES REG artdaq-mu2e::Mu2eReceiverBase
  )

cet_build_plugin(Mu2eSubEventReceiver artdaq::commandableGenerator
  LIBRARIES REG 
  artdaq-core-mu2e::Overlays
  mu2e-pcie-utils::DTCInterface
  artdaq_plugin_types::CommandableFragmentGenerator
)

cet_build_plugin(CRVReceiver artdaq::commandableGenerator LIBRARIES REG artdaq-mu2e::Mu2eReceiverBase)

cet_build_plugin(STMReceiver artdaq::commandableGenerator LIBRARIES REG artdaq-mu2e::Mu2eReceiverBase)



#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
