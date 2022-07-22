
include(artdaq::transfer)

cet_build_plugin(SingleNode artdaq::transfer LIBRARIES REG
  artdaq::TransferPlugins_Shmem_transfer)

install_headers()
install_source()
