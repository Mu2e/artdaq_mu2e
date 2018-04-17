
simple_plugin(SingleNode "transfer"
  artdaq_TransferPlugins
  artdaq_TransferPlugins_Shmem_transfer
  artdaq_DAQdata
  fhiclcpp
${CETLIB}
${CETLIB_EXCEPT}
  MF_MessageLogger
  ${Boost_SYSTEM_LIBRARY}
)

install_headers()
install_source()
