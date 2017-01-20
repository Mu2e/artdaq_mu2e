
def generateAggregator(bunchSize, fragSizeWords, sources_fhicl,
                       xmlrpcClientList, fileSizeThreshold, fileDuration,
                       fileEventCount, queueDepth, queueTimeout, onmonEventPrescale,
                       agType, logger_rank, dispatcher_rank,
					   withGanglia = 0, withMsgFacility = 0, withGraphite = 0)

agConfig = String.new( "\
daq: {
  aggregator: {
    expected_events_per_bunch: %{bunch_size}
    print_event_store_stats: true
    event_queue_depth: %{queue_depth}
    event_queue_wait_time: %{queue_timeout}
    onmon_event_prescale: %{onmon_event_prescale}
    xmlrpc_client_list: \"%{xmlrpc_client_list}\"
    file_size_MB: %{file_size}
    file_duration: %{file_duration}
    file_event_count: %{file_event_count}
    %{ag_type_param_name}: true

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

  transfer_to_dispatcher: {
    transferPluginType: Shmem
	source_rank: %{logger_rank}
	destination_rank: %{dispatcher_rank}
    max_fragment_size_words: %{size_words}
  }

}" )

agConfig.gsub!(/\%\{sources_fhicl\}/, sources_fhicl)
  agConfig.gsub!(/\%\{size_words\}/, String(fragSizeWords))
  agConfig.gsub!(/\%\{bunch_size\}/, String(bunchSize))  
  agConfig.gsub!(/\%\{queue_depth\}/, String(queueDepth))  
  agConfig.gsub!(/\%\{queue_timeout\}/, String(queueTimeout))  
  agConfig.gsub!(/\%\{onmon_event_prescale\}/, String(onmonEventPrescale))
  agConfig.gsub!(/\%\{xmlrpc_client_list\}/, String(xmlrpcClientList))
  agConfig.gsub!(/\%\{file_size\}/, String(fileSizeThreshold))
  agConfig.gsub!(/\%\{file_duration\}/, String(fileDuration))
  agConfig.gsub!(/\%\{file_event_count\}/, String(fileEventCount))
  if agType == "online_monitor"
    #agConfig.gsub!(/\%\{ag_type_param_name\}/, "is_online_monitor")
    agConfig.gsub!(/\%\{ag_type_param_name\}/, "is_dispatcher")
  else
    agConfig.gsub!(/\%\{ag_type_param_name\}/, "is_data_logger")
  end
  agConfig.gsub!(/\%\{logger_rank\}/, String(logger_rank))
  agConfig.gsub!(/\%\{dispatcher_rank\}/, String(dispatcher_rank))
  
  return agConfig
end
