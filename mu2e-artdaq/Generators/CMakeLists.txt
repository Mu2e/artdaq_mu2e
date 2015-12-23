simple_plugin(DTCReceiver "generator"
  mu2e-artdaq-core_Overlays
  ${ARTDAQ_APPLICATION}
  ${ARTDAQ_DAQDATA}
  ${ARTDAQ_UTILITIES}
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${CETLIB}
  DTCInterface
  )

simple_plugin(Mu2eReceiver "generator"
  mu2e-artdaq_Generators_DTCReceiver_generator
  mu2e-artdaq-core_Overlays
  ${ARTDAQ_APPLICATION}
  ${ARTDAQ_DAQDATA}
  ${ARTDAQ_UTILITIES}
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${CETLIB}
  DTCInterface
  )

simple_plugin(OverlayTest "generator"
  mu2e-artdaq-core_Overlays
  ${ARTDAQ_APPLICATION}
  ${ARTDAQ_DAQDATA}
  ${ARTDAQ_UTILITIES}
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${CETLIB}
  DTCInterface
  )

install_headers()
install_source()
