
def generateEventBuilder( totalFragments, verbose, sources_fhicl, sendRequests = 0, withGanglia = 0, withMsgFacility = 0, withGraphite = 0)

ebConfig = String.new( "\
daq: {
  event_builder: {
    expected_fragments_per_event: 1
    use_art: true
    print_event_store_stats: true
    verbose: %{verbose}
    send_requests: %{requests_enabled}

	sources: {
		%{sources_fhicl}
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
} "
)

  ebConfig.gsub!(/\%\{total_fragments\}/, String(totalFragments))
  ebConfig.gsub!(/\%\{verbose\}/, String(verbose))
  ebConfig.gsub!(/\%\{sources_fhicl\}/, sources_fhicl)

  if Integer(sendRequests) > 0
    ebConfig.gsub!(/\%\{requests_enabled\}/, "true")
  else
    ebConfig.gsub!(/\%\{requests_enabled\}/, "false")
  end

  return ebConfig

end

