set(TESTGEN_MODEL_INTERFACE_LAUNCHER_TESTS_PREFIX "model_launcher_")
function(build_test)
  set(options CLANG_COMPILER ASM_TARGET_RV32)
  set(oneValueArgs TEST_NAME ASM_FILE)
  cmake_parse_arguments(TestOpt "${options}" "${oneValueArgs}" "" ${ARGN})
  set(LD_SCRIPT "${MODEL_LAUNCHER_TESTS_DIR}/ld/driver.ld")
  if(TestOpt_ASM_TARGET_RV32)
    set(BITS "32")
    set(ABI_PREFIX "i")
  else()
    set(BITS "64")
  endif()
  set(ASM_MARCH "rv${BITS}gc")
  set(ASM_MABI "${ABI_PREFIX}lp${BITS}d")
  set(TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/${TestOpt_TEST_NAME})
  file(MAKE_DIRECTORY ${TEST_DIR})
  if(NOT DEFINED GCC_TOOLCHAIN_PATH)
    message(SEND_ERROR "GCC_TOOLCHAIN_PATH must be specified for building tests")
  endif()
  if(TestOpt_CLANG_COMPILER)
    if(NOT DEFINED CLANG_TOOLCHAIN_PATH)
      message(SEND_ERROR "CLANG_TOOLCHAIN_PATH must be specified for clang compiler")
    endif()
    set(COMPILER "${CLANG_TOOLCHAIN_PATH}/bin/clang")
    set(EXTRA_COMPILER_ARGS "--gcc-toolchain=${GCC_TOOLCHAIN_PATH}" "-target"
                            "riscv${BITS}-unknown-elf")
  else()
    set(COMPILER "${GCC_TOOLCHAIN_PATH}/bin/riscv64-unknown-elf-gcc")
  endif()
  set(TEST_ELF "${TestOpt_TEST_NAME}.elf")
  add_custom_command(
    OUTPUT ${TEST_DIR}/${TEST_ELF}
    COMMAND ${COMPILER} ${EXTRA_COMPILER_ARGS} -march=${ASM_MARCH} -mabi=${ASM_MABI} -nostdlib
            -T${LD_SCRIPT} -static ${TestOpt_ASM_FILE} -o ${TEST_ELF}
    DEPENDS ${TestOpt_ASM_FILE} ${LD_SCRIPT}
    WORKING_DIRECTORY ${TEST_DIR}
    COMMENT "Building ${TEST_ELF}"
    VERBATIM COMMAND_EXPAND_LISTS)
endfunction()

function(add_model_test)
  set(options CLANG_COMPILER CHECK_PLUGIN_OUTPUT CHECK_JSON_EXEC_LOG ASM_TARGET_RV32
              CLEAR_INTERRUPT)
  set(oneValueArgs TEST_NAME ASM_FILE ISA_STRING MODEL_PLUGIN_LIBRARY LAUNCHER REGEX_OF_FAILURE)
  set(multiValueArgs ADDITIONAL_ARGS)
  cmake_parse_arguments(ModelTest "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(TEST_NAME "${TESTGEN_MODEL_INTERFACE_LAUNCHER_TESTS_PREFIX}${ModelTest_TEST_NAME}")
  set(BUILD_TEST_OPTS)
  if(ModelTest_ASM_TARGET_RV32)
    list(APPEND BUILD_TEST_OPTS ASM_TARGET_RV32)
  endif()
  if(ModelTest_CLANG_COMPILER)
    list(APPEND BUILD_TEST_OPTS CLANG_COMPILER)
  endif()
  build_test(TEST_NAME ${TEST_NAME} ASM_FILE
             "${MODEL_LAUNCHER_TESTS_DIR}/asm/${ModelTest_ASM_FILE}" ${BUILD_TEST_OPTS})

  add_custom_target(
    ${TEST_NAME}-target ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}/${TEST_NAME}.elf
    COMMENT "Target for ${TEST_NAME} binary")
  add_dependencies(TestgenModelLauncherTests ${TEST_NAME}-target)

  if(NOT DEFINED ModelTest_LAUNCHER)
    set(ModelTest_LAUNCHER "ModelLauncher")
  endif()
  if(ModelTest_CHECK_JSON_EXEC_LOG)
    set(EXPECTED_JSON_LOG
        "${MODEL_LAUNCHER_TESTS_DIR}/expected/${ModelTest_ASM_FILE}_json_log.expected")
    set(JSON_LOG "${TEST_NAME}.json")
    set(JSON_LOG_OPT "--driver-json-exec-log=${JSON_LOG}")
  endif()

  if(ModelTest_CLEAR_INTERRUPT)
    set(CLEAR_INTERRUPT_ARG "--driver-model-clear-interrupt=1")
  endif()

  set(OUTPUT_STATE_FILE "${TEST_NAME}_output_state.result")
  set(EXPECTED_OUTPUT_STATE_FILE
      "${MODEL_LAUNCHER_TESTS_DIR}/expected/${ModelTest_ASM_FILE}.expected")

  set(MODEL_LOG_FILE "${TEST_NAME}.log")

  set(TEST_ELF "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}/${TEST_NAME}.elf")

  if(ModelTest_ISA_STRING)
    set(MODEL_ISA_STRING_OPT "--rvm-isa-string=${ModelTest_ISA_STRING}")
  endif()

  if(NOT DEFINED ModelTest_MODEL_PLUGIN_LIBRARY)
    message(SEND_ERROR "Path to model plugin library must be specified")
  endif()

  set(TEST_COMMAND
      ${ModelTest_LAUNCHER} --driver-model-lib ${ModelTest_MODEL_PLUGIN_LIBRARY}
      --driver-output-state-file ${OUTPUT_STATE_FILE} --rvm-model-log-path=${MODEL_LOG_FILE}
      --rvm-stop-mode AtLabel --rvm-exec-file-path ${TEST_ELF} ${JSON_LOG_OPT}
      ${MODEL_ISA_STRING_OPT} ${CLEAR_INTERRUPT_ARG} ${ModelTest_ADDITIONAL_ARGS})

  add_test(
    NAME ${TEST_NAME}
    COMMAND ${TEST_COMMAND}
    WORKING_DIRECTORY ${TEST_DIR} COMMAND_EXPAND_LISTS)

  if(ModelTest_REGEX_OF_FAILURE)
    set_tests_properties(${TEST_NAME} PROPERTIES WILL_FAIL TRUE)
    set(CLI_ERROR_OUTPUT_TARGET "${ModelTest_TEST_NAME}_regex")
    add_test(
      NAME "${CLI_ERROR_OUTPUT_TARGET}"
      COMMAND ${TEST_COMMAND}
      WORKING_DIRECTORY ${TEST_DIR} COMMAND_EXPAND_LISTS)
    set_tests_properties(${CLI_ERROR_OUTPUT_TARGET} PROPERTIES PASS_REGULAR_EXPRESSION
                                                               "${ModelTest_REGEX_OF_FAILURE}")
  else()
    set(EXPECTED_OUTPUT_TARGET "${ModelTest_TEST_NAME}_output")
    add_test(
      NAME ${EXPECTED_OUTPUT_TARGET}
      COMMAND diff ${EXPECTED_OUTPUT_STATE_FILE} ${OUTPUT_STATE_FILE}
      WORKING_DIRECTORY ${TEST_DIR})
    set_tests_properties(${EXPECTED_OUTPUT_TARGET} PROPERTIES DEPENDS "${TEST_NAME}")

    if(ModelTest_CHECK_JSON_EXEC_LOG)
      set(EXPECTED_JSON_TARGET "${ModelTest_TEST_NAME}_json")
      add_test(
        NAME ${EXPECTED_JSON_TARGET}
        COMMAND diff ${EXPECTED_JSON_LOG} ${JSON_LOG}
        WORKING_DIRECTORY ${TEST_DIR})
      set_tests_properties(${EXPECTED_JSON_TARGET} PROPERTIES DEPENDS "${TEST_NAME}")
    endif()
  endif()
endfunction()

function(locate_model_launcher_tests_directory)
  if(DEFINED MODEL_LAUNCHER_TESTS_DIR)
    return()
  endif()

  set(DEFAULT_MODEL_LAUNCHER_TESTS_DIR "${PROJECT_SOURCE_DIR}/tests/rvm")
  if(NOT IS_DIRECTORY "${DEFAULT_MODEL_LAUNCHER_TESTS_DIR}")
    message(SEND_ERROR "Cannot find model launcher tests!")
  endif()
  set(MODEL_LAUNCHER_TESTS_DIR "${DEFAULT_MODEL_LAUNCHER_TESTS_DIR}")
  message(STATUS "-- MODEL_LAUNCHER_TESTS_DIR set to ${MODEL_LAUNCHER_TESTS_DIR}")
endfunction()

function(add_model_tests)
  set(oneValueArgs MODEL_PLUGIN_LIBRARY)
  cmake_parse_arguments(ModelTest "" "${oneValueArgs}" "" ${ARGN})
  add_custom_target(TestgenModelLauncherTests ALL COMMENT "Building testgen model launcher tests")

  locate_model_launcher_tests_directory()

  add_model_test(
    TEST_NAME
    basic
    ASM_FILE
    basic.S
    MODEL_PLUGIN_LIBRARY
    ${MODEL_PLUGIN_LIBRARY}
    CHECK_JSON_EXEC_LOG)

  add_model_test(
    TEST_NAME
    basic_invalid_arg
    ASM_FILE
    basic.S
    MODEL_PLUGIN_LIBRARY
    ${MODEL_PLUGIN_LIBRARY}
    ADDITIONAL_ARGS
    --invalid-cli-arg
    bla
    REGEX_OF_FAILURE
    "could not parse command line")

  add_model_test(
    TEST_NAME
    basic_clang
    ASM_FILE
    basic.S
    CLANG_COMPILER
    MODEL_PLUGIN_LIBRARY
    ${MODEL_PLUGIN_LIBRARY})

  add_model_test(
    TEST_NAME
    basic32
    ASM_FILE
    basic32.S
    MODEL_PLUGIN_LIBRARY
    ${MODEL_PLUGIN_LIBRARY}
    ASM_TARGET_RV32
    ISA_STRING
    "rv32gcv")

  add_model_test(
    TEST_NAME
    basic32_clang
    ASM_FILE
    basic32.S
    CLANG_COMPILER
    MODEL_PLUGIN_LIBRARY
    ${MODEL_PLUGIN_LIBRARY}
    ASM_TARGET_RV32
    ISA_STRING
    "rv32gcv")

  add_model_test(
    TEST_NAME
    interrupt_set
    LAUNCHER
    InterruptLauncher
    ASM_FILE
    interrupt_set.S
    MODEL_PLUGIN_LIBRARY
    ${MODEL_PLUGIN_LIBRARY})

  add_model_test(
    TEST_NAME
    interrupt_clear
    LAUNCHER
    InterruptLauncher
    ASM_FILE
    interrupt_clear.S
    MODEL_PLUGIN_LIBRARY
    ${MODEL_PLUGIN_LIBRARY}
    CLEAR_INTERRUPT)
endfunction()

set(TESTGEN_MODEL_INTERFACE_LAUNCHER_TESTS_MODULE_LOADED ON)
