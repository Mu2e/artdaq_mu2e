# This function will generate the FHiCL code used to control the
# EventBuilderMain application by configuring its
# artdaq::EventBuilderCore object

require File.join( File.dirname(__FILE__), 'generateEventBuilder' )


def generateEventBuilderMain(ebIndex, totalAGs, dataDir, onmonEnable, diskWritingEnable, totalFragments,
                         fclDDV, sources_fhicl, destinations_fhicl, sendRequests, withGanglia, withMsgFacility, withGraphite )
  # Do the substitutions in the event builder configuration given the options
  # that were passed in from the command line.  

  ebConfig = String.new( "\

services: {
  scheduler: {
    fileMode: NOMERGE
    errorOnFailureToPut: false
  }
  NetMonTransportServiceInterface: {
    service_provider: NetMonTransportService
    #broadcast_sends: true
	destinations: {	
	  %{destinations_fhicl}
    }
  }

  #SimpleMemoryCheck: { }
}

%{event_builder_code}

outputs: {
  %{rootmpi_output}rootMPIOutput: {
  %{rootmpi_output}  module_type: RootMPIOutput
  %{rootmpi_output}}
  %{root_output}normalOutput: {
  %{root_output}  module_type: RootOutput
  %{root_output}  fileName: \"%{output_file}\"
  %{root_output}}
}

physics: {
  analyzers: {
%{phys_anal_ddv_cfg}
  }

  producers: {
  }

  filters: {
  %{phys_filt_rdf_cfg}
  }
    
	a2: [ ddv ]
	delay: [ randomDelay ] 

  %{rootmpi_output}my_output_modules: [ rootMPIOutput ]
  %{root_output}my_output_modules: [ normalOutput ]
}
source: {
  module_type: Mu2eInput
  waiting_time: 2500000
  resume_after_timeout: true
}
process_name: DAQ" )

verbose = "true"

if Integer(totalAGs) >= 1
  verbose = "false"
end


event_builder_code = generateEventBuilder( totalFragments, verbose, sources_fhicl, sendRequests, withGanglia, withMsgFacility, withGraphite)

ebConfig.gsub!(/\%\{destinations_fhicl\}/, destinations_fhicl)
ebConfig.gsub!(/\%\{event_builder_code\}/, event_builder_code)

if Integer(totalAGs) >= 1
  ebConfig.gsub!(/\%\{rootmpi_output\}/, "")
  ebConfig.gsub!(/\%\{root_output\}/, "#")
else
  ebConfig.gsub!(/\%\{rootmpi_output\}/, "#")
  if Integer(diskWritingEnable) != 0
    ebConfig.gsub!(/\%\{root_output\}/, "")
  else
    ebConfig.gsub!(/\%\{root_output\}/, "#")
  end
end

ebConfig.gsub!(/\%\{phys_anal_ddv_cfg\}/, fclDDV)
ebConfig.gsub!(/\%\{phys_filt_rdf_cfg\}/, String("") + read_fcl("Mu2eFilterSim.fcl"))


currentTime = Time.now
fileName = "mu2e_artdaq_eb%02d_" % ebIndex
fileName += "r%06r_sr%02s_%to"
fileName += ".root"
outputFile = File.join(dataDir, fileName)
ebConfig.gsub!(/\%\{output_file\}/, outputFile)

return ebConfig

end


