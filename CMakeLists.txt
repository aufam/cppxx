cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# project settings
project(cppxx
    VERSION     0.1.0
    DESCRIPTION "Complete C++ framework"
    LANGUAGES   CXX
)

# some options
option(CPPXX_BUILD_CMD    "Build executable tool"                   OFF)
option(CPPXX_BUILD_TESTS  "Build test cases"                        OFF)
option(CPPXX_REQUIREMENTS "Only install the requirements for cmd"   OFF)

# some messages
message(STATUS "CPPXX_VERSION       : ${cppxx_VERSION}")
message(STATUS "CPPXX_BUILD_CMD     : ${CPPXX_BUILD_CMD}")
message(STATUS "CPPXX_BUILD_TESTS   : ${CPPXX_BUILD_TESTS}")
message(STATUS "CPPXX_REQUIREMENTS  : ${CPPXX_REQUIREMENTS}")


# core interface
add_library(cppxx INTERFACE)
add_library(cppxx::cppxx ALIAS cppxx)

target_include_directories(cppxx INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(cppxx INTERFACE cxx_std_23)

if (NOT CPPXX_BUILD_CMD AND NOT CPPXX_BUILD_TESTS)
    return()
endif()


# external libraries
include("cmake/cppxx.cmake")
cppxx_github_package("fmt:fmtlib/fmt#11.2.0"                         DOWNLOAD_ONLY YES)
cppxx_github_package("spdlog:gabime/spdlog#v1.15.3"                  DOWNLOAD_ONLY YES)
cppxx_github_package("toml:marzer/tomlplusplus#v3.4.0"               DOWNLOAD_ONLY YES)
cppxx_github_package("json:nlohmann/json#v3.12.0"                    DOWNLOAD_ONLY YES)
cppxx_github_package("opts:jarro2783/cxxopts#v3.3.1"                 DOWNLOAD_ONLY YES)
cppxx_github_package("googletest:google/googletest#v1.17.0"          OPTIONS "INSTALL_GTEST OFF")
cppxx_github_package("openssl:janbar/openssl-cmake#1.1.1w-20250419"  OPTIONS "WITH_APPS OFF")

set(CPPXX_THIRD_PARTY_INCLUDE_DIRS
    "${fmt_SOURCE_DIR}/include"
    "${spdlog_SOURCE_DIR}/include"
    "${json_SOURCE_DIR}/single_include/nlohmann"
    "${toml_SOURCE_DIR}"
    "${opts_SOURCE_DIR}/include"
)
cppxx_assert_directories_exist(${CPPXX_THIRD_PARTY_INCLUDE_DIRS})

# only setup the exeternal libraries
if (CPPXX_REQUIREMENTS)
    return()
endif()


# private interface
add_library(cppxx_private INTERFACE)
add_library(cppxx::private ALIAS cppxx_private)

target_include_directories(cppxx_private INTERFACE
    ${CPPXX_THIRD_PARTY_INCLUDE_DIRS}
)

target_link_libraries(cppxx_private INTERFACE
    crypto
)

target_compile_definitions(cppxx_private INTERFACE
    -DFMT_HEADER_ONLY=1
    -DSPDLOG_FMT_EXTERNAL=1
)

target_compile_options(cppxx_private INTERFACE
    -Wall
    -Wextra
    -Wno-deprecated-literal-operator
    -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=cppxx
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

target_link_options(cppxx_private INTERFACE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

# executable
if (CPPXX_BUILD_CMD)
    file(GLOB_RECURSE CMD_SOURCES cmd/*)
    add_executable(cmd ${CMD_SOURCES})

    target_link_libraries(cmd PRIVATE
        cppxx
        cppxx_private
    )
endif()

# tests
if (CPPXX_BUILD_TESTS)
	add_subdirectory(tests)
	enable_testing()
	add_test(NAME test_all COMMAND test_all)
endif()
