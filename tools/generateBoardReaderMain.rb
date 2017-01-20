# This function will generate the FHiCL code used to control the
# BoardReaderMain application by configuring its
# artdaq::BoardReaderCore object
  
def generateBoardReaderMain( generatorCode, destinations_fhicl, withGanglia = 0, withMsgFacility = 0, withGraphite = 0)

  brConfig = String.new( "\
  daq: {
  fragment_receiver: {
    mpi_sync_interval: 50

    %{generator_code}

	destinations: {
	  %{destinations_fhicl}
	}
  }

  metrics: {
     ganglia: {
       metricPluginType: \"ganglia\"
       level: 3
       reporting_interval: 15.0
     
       configFile: \"/home/mu2edaq/daqlogs/gmond.conf\"
       group: \"ARTDAQ\"
     }
  }
}"
)
  
  brConfig.gsub!(/\%\{generator_code\}/, String(generatorCode))
  brConfig.gsub!(/\%\{destinations_fhicl\}/, destinations_fhicl)
 
  return brConfig
end
