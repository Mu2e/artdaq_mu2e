
# Generate the FHiCL document which configures the mu2e::ToySimulator class

# Note that if "nADCcounts" is set to nil, its FHiCL
# setting is defined in a separate file called ToySimulator.fcl,
# searched for via "read_fcl"

require File.join( File.dirname(__FILE__), 'demo_utilities' )
  
def generateToy(startingFragmentId, boardId)

  dtcConfig = String.new( "\
    generator: DTCReceiver
    fragment_type: \"DTC\"
    fragment_id: %{startingFragmentId}
    board_id: %{board_id}" \
                          + read_fcl("DTCReceiver.fcl") )
  
  dtcConfig.gsub!(/\%\{starting_fragment_id\}/, String(startingFragmentId))
  dtcConfig.gsub!(/\%\{board_id\}/, String(boardId))

  return dtcConfig

end
