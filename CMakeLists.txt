cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# project settings
project(cppxx
    VERSION     0.1.0
    DESCRIPTION "Complete C++ framework"
    LANGUAGES   CXX
)

# some options
option(CPPXX_BUILD_APPS   "Build CLI apps"                OFF)
option(CPPXX_BUILD_TESTS  "Build test cases"              OFF)
option(CPPXX_REQUIREMENTS "Only install the requirements" OFF)

# some messages
message(STATUS "CPPXX_VERSION       : ${cppxx_VERSION}")
message(STATUS "CPPXX_BUILD_TESTS   : ${CPPXX_BUILD_TESTS}")
message(STATUS "CPPXX_REQUIREMENTS  : ${CPPXX_REQUIREMENTS}")

# external libraries
include("cmake/cppxx.cmake")
cppxx_github_package("preprocessor:boostorg/preprocessor#boost-1.88.0" DOWNLOAD_ONLY YES)
cppxx_github_package("fmt:fmtlib/fmt#11.2.0"                           DOWNLOAD_ONLY YES)
cppxx_github_package("toml:marzer/tomlplusplus#v3.4.0"                 DOWNLOAD_ONLY YES)
cppxx_github_package("json:nlohmann/json#v3.12.0"                      DOWNLOAD_ONLY YES)
cppxx_github_package("cxxopts:jarro2783/cxxopts#v3.3.1"                DOWNLOAD_ONLY YES)
cppxx_github_package("googletest:google/googletest#v1.17.0"            OPTIONS "INSTALL_GTEST OFF")

# only setup the exeternal libraries
if (CPPXX_REQUIREMENTS)
    return()
endif()

# libraries
## private interface
add_library(cppxx_private INTERFACE)
add_library(cppxx::private ALIAS cppxx_private)

target_include_directories(cppxx_private INTERFACE
    "${preprocessor_SOURCE_DIR}/include"
    "${fmt_SOURCE_DIR}/include"
    "${cxxopts_SOURCE_DIR}/include"
    "${toml_SOURCE_DIR}"
    "${json_SOURCE_DIR}/single_include"
)

target_compile_definitions(cppxx_private INTERFACE
    -DFMT_HEADER_ONLY=1
)

target_compile_options(cppxx_private INTERFACE
    -Wall
    -Wextra
    -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=cppxx
)

find_package(OpenSSL REQUIRED)


## main lib
file(GLOB_RECURSE SOURCES src/*)
add_library(cppxx ${SOURCES})
add_library(cppxx::cppxx ALIAS cppxx)

target_include_directories(cppxx PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(cppxx PRIVATE
    cppxx_private
)

target_link_libraries(cppxx PUBLIC OpenSSL::SSL OpenSSL::Crypto)

file(GLOB_RECURSE CMD_SOURCES cmd/*)
add_executable(cmd ${CMD_SOURCES})
target_link_libraries(cmd PRIVATE
    cppxx
    cppxx_private
)

# apps
if (CPPXX_BUILD_APPS)
	add_subdirectory(apps)
endif()

# tests
if (CPPXX_BUILD_TESTS)
	add_subdirectory(tests)
	enable_testing()
	add_test(NAME test_all COMMAND test_all)
endif()
