
simple_plugin(SingleNode "transfer"
  artdaq_TransferPlugins
  artdaq_TransferPlugins_Shmem_transfer
  fhiclcpp
  cetlib
  MF_MessageLogger
  ${TRACE}
)

install_headers()
install_source()
