# This function will generate the FHiCL code used to control the
# BoardReaderMain application by configuring its
# artdaq::BoardReaderCore object
  
def generateBoardReaderMain(totalEBs, totalFRs, fragSizeWords, generatorCode)

  brConfig = String.new( "\
  daq: {
  max_fragment_size_words: %{size_words}
  fragment_receiver: {
    mpi_buffer_count: %{buffer_count}
    mpi_sync_interval: 50
    first_event_builder_rank: %{total_frs}
    event_builder_count: %{total_ebs}

    %{generator_code}
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
  
  brConfig.gsub!(/\%\{total_ebs\}/, String(totalEBs))
  brConfig.gsub!(/\%\{total_frs\}/, String(totalFRs))
  brConfig.gsub!(/\%\{buffer_count\}/, String(totalEBs*8))
  brConfig.gsub!(/\%\{size_words\}/, String(fragSizeWords))
  brConfig.gsub!(/\%\{generator_code\}/, String(generatorCode))
  return brConfig
end
