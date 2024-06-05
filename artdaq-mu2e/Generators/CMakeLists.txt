
cet_make_library(LIBRARY_NAME artdaq-mu2e_Generators_Mu2eReceiverBase
SOURCE 
Mu2eEventReceiverBase.cc
LIBRARIES PUBLIC
artdaq::DAQdata
canvas::canvas
artdaq_core_mu2e::artdaq-core-mu2e_Overlays
mu2e_pcie_utils::DTCInterface
artdaq_plugin_types::CommandableFragmentGenerator
  )

include(artdaq::commandableGenerator)

link_directories(/home/mu2estm/STMDAQ_software/STMDAQ-TestBeam/build/lib)
  
cet_build_plugin(Mu2eEventReceiver artdaq::commandableGenerator LIBRARIES REG artdaq_mu2e::artdaq-mu2e_Generators_Mu2eReceiverBase
  )

cet_build_plugin(Mu2eSubEventReceiver artdaq::commandableGenerator
  LIBRARIES REG 
  artdaq_core_mu2e::artdaq-core-mu2e_Overlays
  mu2e_pcie_utils::DTCInterface
  artdaq_plugin_types::CommandableFragmentGenerator
)

cet_build_plugin(STMReceiver artdaq::commandableGenerator LIBRARIES REG artdaq_mu2e::artdaq-mu2e_Generators_Mu2eReceiverBase libUtils.so)
cet_build_plugin(CRVReceiver artdaq::commandableGenerator LIBRARIES REG artdaq_mu2e::artdaq-mu2e_Generators_Mu2eReceiverBase)


#cet_make_exec(stm-packet-sender SOURCE stm-packet-sender.cc LIBRARIES artdaq_mu2e::STMUtils pthread )


#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
