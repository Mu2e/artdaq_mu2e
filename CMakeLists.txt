# ======================================================================
#  artdaq_mu2e main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/artdaq-mu2e/ups/setup_for_development
#  buildtool
# ======================================================================

cmake_minimum_required (VERSION 3.19 FATAL_ERROR)

find_package(cetmodules 3.16.00 REQUIRED)

project(artdaq_mu2e VERSION 1.05.00)

include(CetCMakeEnv)
cet_cmake_env()

cet_set_compiler_flags(DIAGS VIGILANT
 WERROR
 NO_UNDEFINED
 EXTRA_FLAGS -pedantic -pthread
 )
string(TOUPPER ${CMAKE_BUILD_TYPE} BTYPE_UC )
#if( ${BTYPE_UC} MATCHES "DEBUG" )
#  cet_add_compiler_flags(-fsanitize=address)
#endif()

#cet_report_compiler_flags()

find_package(artdaq 3.12.00 REQUIRED EXPORT)
find_package(artdaq_core_mu2e 1.08.00 REQUIRED EXPORT)

include(BuildPlugins)

# source
add_subdirectory(artdaq-mu2e)

# testing
add_subdirectory(test)

# tools
add_subdirectory(tools)

# doc - documentation
if( $ENV{OTS_DOXY} MATCHES "DOIT" )
	add_subdirectory(doc)
endif()

cet_cmake_config()
