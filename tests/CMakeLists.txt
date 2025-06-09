file(GLOB_RECURSE TEST_SOURCES *.*)
add_executable(test_all ${TEST_SOURCES})

target_include_directories(test_all PRIVATE
    "${preprocessor_SOURCE_DIR}/include"
    "${fmt_SOURCE_DIR}/include"
)

target_compile_options(test_all PRIVATE
    -DFMT_HEADER_ONLY=1
)

target_link_libraries(test_all PRIVATE
    cppxx
    gtest_main
)

target_compile_options(test_all PRIVATE
    -Wall
    -Wextra
    -Wno-literal-suffix
    -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=cppxx/tests
)

