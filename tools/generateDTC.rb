
# Generate the FHiCL document which configures the mu2e::ToySimulator class

# Note that if "nADCcounts" is set to nil, its FHiCL
# setting is defined in a separate file called ToySimulator.fcl,
# searched for via "read_fcl"

require File.join( File.dirname(__FILE__), 'demo_utilities' )
  
def generateDTC(startingFragmentId, boardId, simMode = 0)

  dtcConfig = String.new( "\
    generator: DTCReceiver
    fragment_type: \"DTC\"
    fragment_id: %{starting_fragment_id}
    board_id: %{board_id} \
    ring_0_roc_count: 1 \
    ring_0_timing_enabled: false \
    ring_0_roc_emulator_enabled: true \
    debug_print: false \
    sim_mode: %{simulation_mode}" \
                          + read_fcl("DTCReceiver.fcl") )
  
  dtcConfig.gsub!(/\%\{starting_fragment_id\}/, String(startingFragmentId))
  dtcConfig.gsub!(/\%\{board_id\}/, String(boardId))
  dtcConfig.gsub!(/\%\{simulation_mode\}/, String(simMode))

  return dtcConfig

end
