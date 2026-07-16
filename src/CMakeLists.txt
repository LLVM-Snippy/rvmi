set(CMAKE_CXX_STANDARD 20)

execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE)
add_definitions("-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\"")

find_package(elfio REQUIRED)
function(add_model_launcher)
  set(oneValueArgs EXEC_NAME)
  set(multiValueArgs SOURCE_FILES)
  cmake_parse_arguments(ModelLauncher "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  add_executable(${ModelLauncher_EXEC_NAME} ${ModelLauncher_SOURCE_FILES})
  target_compile_options(${ModelLauncher_EXEC_NAME} PRIVATE -Wall -Wextra -Werror)
  target_link_libraries(${ModelLauncher_EXEC_NAME} PRIVATE rvmi)
  target_link_libraries(${ModelLauncher_EXEC_NAME} PUBLIC elfio::elfio)
  target_link_libraries(${ModelLauncher_EXEC_NAME} PRIVATE dl)
  target_link_options(${ModelLauncher_EXEC_NAME} PRIVATE -gz)
  target_include_directories(${ModelLauncher_EXEC_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
  target_compile_definitions(${ModelLauncher_EXEC_NAME}
                             PRIVATE MODEL_PATH=\"${CMAKE_CURRENT_BINARY_DIR}/libRISCVModel.so\")
  install(TARGETS ${ModelLauncher_EXEC_NAME})
endfunction()

add_model_launcher(EXEC_NAME ModelLauncher SOURCE_FILES Driver.cpp DriverUtils.cpp)
add_model_launcher(EXEC_NAME InterruptLauncher SOURCE_FILES InterruptDriver.cpp DriverUtils.cpp)
