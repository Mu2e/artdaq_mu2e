
# Generate the FHiCL document which configures the mu2e::DTCDataVerifier class

require File.join( File.dirname(__FILE__), 'demo_utilities' )

def generateDTCDataVerifier(fragmentIDList, fragmentTypeList)

  dtcDataVerifierConfig = String.new( "\
    ddv: {
      module_type: DTCDataVerifier
      fragment_type_labels: %{fragment_type_labels} " \
      + "    }" )


    fragmentIDListString, fragmentTypeListString = "[ ", "[ "

    typemap = Hash.new

    0.upto(fragmentIDList.length-1) do |i|
      typemap[ fragmentIDList[i]  ] = fragmentTypeList[i]
    end

    fragmentIDList.sort.each { |id| fragmentIDListString += " %d," % [ id ] }
    fragmentIDList.sort.each { |id| fragmentTypeListString += "%s," % [ typemap[ id ] ] }

    fragmentIDListString[-1], fragmentTypeListString[-1] = "]", "]" 

  dtcDataVerifierConfig.gsub!(/\%\{fragment_type_labels\}/, String(fragmentTypeListString))

  return dtcDataVerifierConfig
end

