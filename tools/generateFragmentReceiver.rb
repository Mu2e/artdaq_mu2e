
# Generate the FHiCL document which configures the FragmentGenerator class

require File.join( File.dirname(__FILE__), 'demo_utilities' )

def generateFragmentReceiver(startingFragmentId, boardId, fragmentType, configDoc, simMode = 0,useSimFile = 0, simFile = "" )

  generator = nil
  if fragmentType == "MU2E"
    generator = "Mu2eReceiver"
  elsif fragmentType == "DTC"
    generator = "DTCReceiver"
  end

  if configDoc == nil
    configDoc = generator + ".fcl"
  end

  fgConfig = String.new( 
    "# CommandableFragmentGenerator Configuration: " +
    read_fcl("CommandableFragmentGenerator.fcl") +
    "

    # Generated Parameters: 
    generator: %{generator}
    fragment_type: %{fragment_type}
    fragment_id: %{starting_fragment_id}
    board_id: %{board_id}
    starting_fragment_id: %{starting_fragment_id}
	
    ring_0_roc_count: 1 \
    ring_0_timing_enabled: false \
    ring_0_roc_emulator_enabled: true \
    debug_print: false \
    sim_mode: %{simulation_mode} \

    # Generator-Specific Configuration:
    " + read_fcl(configDoc) )
  
  fgConfig.gsub!(/\%\{generator\}/, String(generator))
  fgConfig.gsub!(/\%\{fragment_type\}/, String(fragmentType)) 
  fgConfig.gsub!(/\%\{starting_fragment_id\}/, String(startingFragmentId))
  fgConfig.gsub!(/\%\{board_id\}/, String(boardId))
  
  fgConfig.gsub!(/\%\{simulation_mode\}/, String(simMode))

  if Integer(useSimFile) != 0
    fgConfig.gsub!(/\%\{use_sim_file\}/, "")
  else
    fgConfig.gsub!(/\%\{use_sim_file\}/, "#")
  end
  fgConfig.gsub!(/\%\{sim_file\}/, String(simFile))

  return fgConfig

end
