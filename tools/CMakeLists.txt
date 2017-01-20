cet_script( cleanupMu2ePilotSystem.sh DemoControl.rb demo_utilities.rb
            generateAggregatorMain.rb generateAggregator.rb
            generateBoardReaderMain.rb generateDTCDataVerifier.rb
            generateEventBuilderMain.rb generateEventBuilder.rb
            generateFragmentReceiver.rb installArtDaqDemo.sh
            iptables_forward manageMu2ePilotSystem.sh
            manageMu2eSingleNodeSystem.sh runMu2ePilotSystem.sh
            setupDemoEnvironment.sh setup_trace.sh startMu2ePilotSystem.sh
            startMu2eSingleNodeSystem.sh xt_cmd.sh
 )


art_make_exec(driver
  LIBRARIES
  artdaq_Application
  artdaq_DAQrate
  artdaq_DAQdata
  artdaq-core_Utilities
  artdaq-utilities_Plugins
  artdaq-core_Core
  ${ART_FRAMEWORK_ART}
  ${FHICLCPP}
  ${CETLIB}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${MPI_C_LIBRARIES}
  )
  
# Install FCL files needed for configuring the system
add_subdirectory(fcl)

# Is this necessary?
#install_source()
