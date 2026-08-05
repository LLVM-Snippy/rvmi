cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(BASH_EXECUTABLE bash REQUIRED)
execute_process(
  COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/get-api-version.sh
  OUTPUT_VARIABLE RVMI_MAJOR_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)

project(
  rvmi
  LANGUAGES C CXX
  VERSION ${RVMI_MAJOR_VERSION})

option(BUILD_DOCS "Build Doxygen documentation" OFF)

message(STATUS "RVMI_MAJOR_VERSION: ${PROJECT_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(rvmi STATIC src/RVM.cpp)
target_include_directories(rvmi PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_sources(
  rvmi
  PUBLIC FILE_SET
         public_headers
         TYPE
         HEADERS
         BASE_DIRS
         include
         FILES
         include/RISCVModel/RVM.h
         include/RISCVModel/RVM.hpp
         include/RISCVModel/VTable.h)

add_library(rvmi::rvmi ALIAS rvmi)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
# NOTE: install of file sets with TYPE HEADERS respects CMAKE_INSTALL_INCLUDEDIR when GNUInstallDirs
# is included. DESTINATION is `include` by default.
install(
  TARGETS rvmi
  EXPORT rvmi-targets
  FILE_SET public_headers)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/testgen-model-interface-launcher-tests.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/testgen-model-interface)
install(DIRECTORY tests/rvm/asm tests/rvm/expected tests/rvm/ld
        DESTINATION ${CMAKE_INSTALL_DATADIR}/testgen-model-interface/rvm)

include(gen-pkgconf)

write_basic_package_version_file(rvmi-config-version.cmake COMPATIBILITY SameMinorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rvmi-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rvmi)

install(
  EXPORT rvmi-targets
  NAMESPACE rvmi::
  FILE rvmi-targets.cmake
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/rvmi")

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/rvmi-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/rvmi-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rvmi)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rvmi-config.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rvmi)

add_subdirectory(src)

include(CTest)

if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
  message(STATUS "rvmi: enabling tests")
else()
  message(STATUS "rvmi: disabling tests")
endif()

if(BUILD_DOCS)
  add_subdirectory(docs)
endif()
