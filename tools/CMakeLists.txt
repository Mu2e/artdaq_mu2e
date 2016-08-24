cet_script(setupDemoEnvironment.sh DemoControl.rb 
startMu2ePilotSystem.sh manageMu2ePilotSystem.sh
startMu2eSingleNodeSystem.sh manageMu2eSingleNodeSystem.sh
CreateNewFragmentGenerator.rb  generateEventBuilderMain.rb
generateWFViewer.rb generateDTCDataVerifier.rb
generateEventBuilder.rb demo_utilities.rb generateMu2e.rb
generateDTC.rb generateAggregator.rb generateAggregatorMain.rb generateBoardReaderMain.rb RunDriver.rb)


art_make_exec(driver
  LIBRARIES
  artdaq_Application
  artdaq_DAQrate
  artdaq-core_Utilities
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
