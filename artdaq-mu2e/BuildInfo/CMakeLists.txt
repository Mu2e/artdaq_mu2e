
# the utcDateTime variable is used by the GetPackageBuildInfo code
# to report the build datetime
execute_process( COMMAND "date" "-u" "+%d-%b-%Y %H:%M:%S %Z" OUTPUT_VARIABLE rawUTCDate )
string( STRIP ${rawUTCDate} utcDateTime )

configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/GetPackageBuildInfo.cc.in
  ${CMAKE_CURRENT_BINARY_DIR}/GetPackageBuildInfo.cc @ONLY )

cet_make_library( LIBRARY_NAME artdaq-mu2e_BuildInfo
                  SOURCE
		  ${CMAKE_CURRENT_BINARY_DIR}/GetPackageBuildInfo.cc
          LIBRARIES PUBLIC
         artdaq_core::artdaq-core_BuildInfo
         artdaq::BuildInfo
          )

install_headers()
install_source()
