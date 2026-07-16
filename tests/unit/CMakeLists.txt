find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(testgen-model-interface-unit-test)

target_sources(testgen-model-interface-unit-test PRIVATE unit-test.cpp build-test.c build-test.cpp)

target_link_libraries(testgen-model-interface-unit-test PRIVATE rvmi::rvmi GTest::gtest_main)

# for DriverUtils.hpp
target_include_directories(testgen-model-interface-unit-test
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../src")

gtest_discover_tests(testgen-model-interface-unit-test)
