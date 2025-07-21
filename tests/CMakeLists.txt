file(GLOB_RECURSE TEST_SOURCES *.*)
add_executable(test_all ${TEST_SOURCES})

target_link_libraries(test_all PRIVATE
    cppxx::cppxx
    cppxx::private
    gtest_main
)
