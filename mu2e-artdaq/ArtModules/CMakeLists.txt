art_make(NO_PLUGINS # WAnt different link lists for modues
  LIB_LIBRARIES mu2e-artdaq_Overlays mu2e-artdaq_Products
)

simple_plugin(ToyDump "module")

# Subdirectories.
add_subdirectory(DQM)
