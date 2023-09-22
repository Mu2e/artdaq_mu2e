
cet_make_library(LIBRARY_NAME Mu2eReceiverBase
SOURCE 
Mu2eEventReceiverBase.cc
LIBRARIES PUBLIC
artdaq::DAQdata
canvas::canvas
artdaq_core_mu2e::Overlays
artdaq_plugin_types::CommandableFragmentGenerator
  )

include(artdaq::commandableGenerator)

link_directories(/home/mu2estm/STMDAQ_software/STMDAQ-TestBeam/build/lib)
  
cet_build_plugin(Mu2eEventReceiver artdaq::commandableGenerator LIBRARIES REG artdaq_mu2e::Mu2eReceiverBase
  )

  cet_build_plugin(CRVReceiver artdaq::commandableGenerator LIBRARIES REG artdaq_mu2e::Mu2eReceiverBase)

  cet_build_plugin(STMReceiver artdaq::commandableGenerator LIBRARIES REG artdaq_mu2e::artdaq-mu2e_Generators_Mu2eReceiverBase libUtils.so)

#cet_make_exec(stm-packet-sender SOURCE stm-packet-sender.cc LIBRARIES artdaq_mu2e::STMUtils pthread )


#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
