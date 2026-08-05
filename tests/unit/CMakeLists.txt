find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(rvmi-unit-test)

target_sources(rvmi-unit-test PRIVATE unit-test.cpp build-test.c build-test.cpp)

target_link_libraries(rvmi-unit-test PRIVATE rvmi::rvmi GTest::gtest_main)

# for DriverUtils.hpp
target_include_directories(rvmi-unit-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../src")

gtest_discover_tests(rvmi-unit-test)
