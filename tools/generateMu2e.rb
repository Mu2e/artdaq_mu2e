
# Generate the FHiCL document which configures the mu2e::ToySimulator class

# Note that if "nADCcounts" is set to nil, its FHiCL
# setting is defined in a separate file called ToySimulator.fcl,
# searched for via "read_fcl"

require File.join( File.dirname(__FILE__), 'demo_utilities' )
  
def generateMu2e(startingFragmentId, boardId, simMode = 0, useSimFile = 0, simFile = "", count = 1)

  mu2eConfig = String.new( "\
    generator: Mu2eReceiver
    fragment_type: \"MU2E\"
    fragment_id: %{starting_fragment_id}
    board_id: %{board_id} \
    ring_0_roc_count: 1 \
    ring_0_timing_enabled: false \
    ring_0_roc_emulator_enabled: true \
    ring_0_roc_emulator_count: 1 \
    fragment_receiver_count: %{fragment_recvr_count}
    send_empty_fragments: true \
    debug_print: false \
    sim_mode: %{simulation_mode} \
    %{use_sim_file}sim_file: \"%{sim_file}\"" \
                          + read_fcl("DTCReceiver.fcl") )
  
  mu2eConfig.gsub!(/\%\{starting_fragment_id\}/, String(startingFragmentId))
  mu2eConfig.gsub!(/\%\{board_id\}/, String(boardId))
  mu2eConfig.gsub!(/\%\{simulation_mode\}/, String(simMode))
  mu2eConfig.gsub!(/\%\{fragment_recvr_count\}/,String(count))

  if Integer(useSimFile) != 0
    mu2eConfig.gsub!(/\%\{use_sim_file\}/, "")
  else
    mu2eConfig.gsub!(/\%\{use_sim_file\}/, "#")
  end
  mu2eConfig.gsub!(/\%\{sim_file\}/, String(simFile))

  return mu2eConfig

end
