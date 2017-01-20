# This function will generate the FHiCL code used to control the
# AggregatorMain application by configuring its
# artdaq::AggregatorCore object

require File.join( File.dirname(__FILE__), 'generateAggregator' )

def generateAggregatorMain(dataDir, bunchSize, onmonEnable,
                           diskWritingEnable, demoPrescale, agIndex, totalAGs, fragSizeWords,
						   sources_fhicl, logger_rank, dispatcher_rank,
                           xmlrpcClientList, fileSizeThreshold, fileDuration,
                           fileEventCount, onmonEventPrescale, 
                           onmon_modules, onmonFileEnable, onmonFileName,
                           withGanglia, withMsgFacility, withGraphite)

agConfig = String.new( "\
services: {
  scheduler: {
    fileMode: NOMERGE
    errorOnFailureToPut: false
  }
  NetMonTransportServiceInterface: {
    service_provider: NetMonTransportService
  }

  #SimpleMemoryCheck: { }
}

%{aggregator_code}

source: {
  module_type: NetMonInput
}
outputs: {
  %{root_output}normalOutput: {
  %{root_output}  module_type: RootOutput
  %{root_output}  fileName: \"%{output_file}\"
  %{root_output}}
  
}
physics: {
  analyzers: {

  }

  producers: {

     BuildInfo:
     {
       module_type: Mu2eArtdaqBuildInfo
       instance_name: Mu2eArtdaq
     }
   }

  filters: {

  }

  p2: [ BuildInfo ]
  
  %{root_output}my_output_modules: [ normalOutput ]
}
process_name: DAQAG"
)

  queueDepth, queueTimeout = -999, -999

  if agIndex < (totalAGs - 1)
    if totalAGs > 1
      onmonEnable = 0
    end
    queueDepth = 20
    queueTimeout = 5
    agType = "data_logger"
  else
    diskWritingEnable = 0
    queueDepth = 2
    queueTimeout = 1
    agType = "online_monitor"
  end

  aggregator_code = generateAggregator( bunchSize, fragSizeWords, sources_fhicl,
                                        xmlrpcClientList, fileSizeThreshold, fileDuration, 
										fileEventCount, queueDepth, queueTimeout, onmonEventPrescale,
										agType, logger_rank, dispatcher_rank,
										withGanglia, withMsgFacility, withGraphite )
  agConfig.gsub!(/\%\{aggregator_code\}/, aggregator_code)

  puts "Initial aggregator " + String(agIndex) + " disk writing setting = " +
  String(diskWritingEnable)
  # Do the substitutions in the aggregator configuration given the options
  # that were passed in from the command line.  Assure that files written out
  # by each AG are unique by including a timestamp in the file name.
  currentTime = Time.now
  fileName = "mu2e_artdaq_"
  fileName += "r%06r_sr%02s_%to"
  if totalAGs > 2
    fileName += "_"
    fileName += String(agIndex)
  end
  fileName += ".root"
  outputFile = File.join(dataDir, fileName)

  agConfig.gsub!(/\%\{output_file\}/, outputFile)
  
  puts "agIndex = %d, totalAGs = %d, onmonEnable = %d" % [agIndex, totalAGs, onmonEnable]

  puts "Final aggregator " + String(agIndex) + " disk writing setting = " +
  String(diskWritingEnable)
  if Integer(diskWritingEnable) != 0
      agConfig.gsub!(/\%\{root_output\}/, "")
  else
    agConfig.gsub!(/\%\{root_output\}/, "#")
  end

  return agConfig  
end
