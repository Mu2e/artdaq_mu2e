# ======================================================================
#  mu2e_artdaq main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/mu2e-artdaq/ups/setup_for_development
#  buildtool
# ======================================================================


# use cmake 2.8 or later
cmake_minimum_required (VERSION 2.8)


project(mu2e-artdaq)
# cetbuildtools contains our cmake modules
SET ( CETBUILDTOOLS_VERSION $ENV{CETBUILDTOOLS_VERSION} )
IF (NOT CETBUILDTOOLS_VERSION)
    MESSAGE (FATAL_ERROR "ERROR: setup cetbuildtools to get the cmake modules")
ENDIF()

set(CMAKE_MODULE_PATH
        $ENV{ART_DIR}/Modules
        $ENV{CETBUILDTOOLS_DIR}/Modules
        ${CMAKE_MODULE_PATH})


include(CetCMakeEnv)
cet_cmake_env()

cet_set_compiler_flags(DIAGS VIGILANT
 WERROR
 NO_UNDEFINED
 EXTRA_FLAGS -pedantic -pthread
 EXTRA_CXX_FLAGS -D_GLIBCXX_USE_NANOSLEEP
 )
string(TOUPPER ${CMAKE_BUILD_TYPE} BTYPE_UC )
#if( ${BTYPE_UC} MATCHES "DEBUG" )
#  cet_add_compiler_flags(-fsanitize=address)
#endif()

cet_report_compiler_flags()

# these are minimum required versions, not the actual product versions
find_ups_product( pcie_linux_kernel_module v1_04_00 )
find_ups_product( cetbuildtools v4_01_04 )
find_ups_product( artdaq v1_11_01 )
find_ups_product( artdaq_utilities v1_00_00 )
find_ups_product( artdaq_core v1_03_04 )
find_ups_product( mu2e_artdaq_core v0_00_01 )
find_ups_product( art v1_10_00 )
find_ups_product( fhiclcpp v2_16_00 )
find_ups_product( messagefacility v1_10_00 )
find_ups_product( TRACE v3_03_00 )
find_ups_product( cetlib v1_03_00 )
find_ups_product( mongodb v3_0_4 )
find_ups_root( v5_34_00 )
find_ups_boost( v1_50_0 filesystem
                        program_options
                        unit_test_framework
 )

# have to make sure cmake doesn't select the system libraries
# define libsigc
cet_find_library( SIGC    NAMES sigc-2.0  PATHS ENV LIBSIGCPP_LIB NO_DEFAULT_PATH )

check_ups_version(canvas "$ENV{CANVAS_VERSION}" v1_06_00 PRODUCT_OLDER_VAR CANVAS_MULTILIB)
if(CANVAS_MULTILIB)
  set(CANVAS_LIB_LIST canvas_Utilities canvas_Persistency_Provenance)
else()
 set(CANVAS_LIB_LIST canvas)
endif(CANVAS_MULTILIB)

string(REGEX MATCHALL "[0-9][0-9]?[0-9]?" ART_VERSION_LIST "$ENV{ART_VERSION}")
list(GET ART_VERSION_LIST 0 ART_MAJOR_VERSION)
list(GET ART_VERSION_LIST 1 ART_MINOR_VERSION)
list(GET ART_VERSION_LIST 2 ART_PATCH_VERSION)

string(LENGTH ${ART_MINOR_VERSION} TMP_LEN)
if (${TMP_LEN} GREATER 1)
    string(SUBSTRING ${ART_MINOR_VERSION} 0 1 TMP_STRING)
    if (${TMP_STRING} EQUAL "0")
        string(SUBSTRING ${ART_MINOR_VERSION} 1 ${TMP_LEN} ART_MINOR_VERSION)
    endif()
endif()

string(LENGTH ${ART_PATCH_VERSION} TMP_LEN)
if (${TMP_LEN} GREATER 1)
    string(SUBSTRING ${ART_PATCH_VERSION} 0 1 TMP_STRING)
    if (${TMP_STRING} EQUAL "0")
        string(SUBSTRING ${ART_PATCH_VERSION} 1 ${TMP_LEN} ART_PATCH_VERSION)
    endif()
endif()

message("-DART_MAJOR_VERSION=${ART_MAJOR_VERSION} -DART_MINOR_VERSION=${ART_MINOR_VERSION} -DART_PATCH_VERSION=${ART_PATCH_VERSION}")
add_definitions(-DART_MAJOR_VERSION=${ART_MAJOR_VERSION} -DART_MINOR_VERSION=${ART_MINOR_VERSION} -DART_PATCH_VERSION=${ART_PATCH_VERSION})

# macros for art_dictionary and simple_plugin
include(ArtDictionary)
include(ArtMake)
include(BuildPlugins)
include(CetTest)

# source
add_subdirectory(mu2e-artdaq)

# testing
add_subdirectory(test)

# tools
add_subdirectory(tools)

# ups - table and config files
add_subdirectory(ups)

# packaging utility
include(UseCPack)
