@page implementation Tutorial for simulator developers

# How to make your simulator compatible with RVM

This guide explains how to implement the RVM interface for an existing RISC-V simulator.

Once implemented, your simulator can be used by any application that understands the RVM API, including co-simulation frameworks, verification environments, regression suites, and generic RVM-based tooling.

## Overview

An RVM implementation consists of:

1. A model state type represented by @ref RVMState.
2. Implementations of all functions declared in @ref RVM.h.
3. A populated @ref RVM_FunctionPointers table exported as @ref RVMAPI_ENTRY_POINT_SYMBOL.
4. An exported interface version symbol named @ref RVMAPI_VERSION_SYMBOL.

The implementation is usually built as a shared library that can later be loaded by an RVM-compatible application.

# Basic requirements

An implementation shall:

* Create and destroy simulator instances via @ref rvm_modelCreate and @ref rvm_modelDestroy.
* Execute exactly one instruction in @ref rvm_executeInstr.
* Provide access to memory, registers, CSRs and vector registers.
* Support model reset through @ref rvm_modelReset.
* Respect configuration specified by @ref RVMConfig.
* Correctly report execution status through @ref RVMSimExecStatus.
* Provide meaningful error diagnostics through @ref rvm_getErrorContext whenever possible.

# Model creation

The @ref rvm_modelCreate function receives a pointer to @ref RVMConfig.

Implementations are expected to:

* Create a new simulator instance.
* Configure RV32/RV64 mode according to @ref RVMConfig::RV64.
* Configure ISA extensions according to @ref RVMConfig::Extensions.
* Allocate memory regions specified by @ref RVMConfig::MemoryRegions.
* Configure logging, stop mode and callback support.
* Copy any configuration information that must survive after the function returns.

The implementation must not assume that @ref RVMConfig remains valid after @ref rvm_modelCreate returns.

If model creation fails:

* @ref rvm_modelCreate shall return `NULL`.
* The output error code shall describe the failure.
* The implementation should provide a human-readable explanation in the error message buffer.

# Memory regions

Memory regions are specified through @ref RVMMemoryRegion.

Implementations shall:

* accept memory regions in arbitrary order
* make every specified region accessible to software running inside the simulated machine

Overlapping memory regions shall be merged.

For example, the following regions:

* `[0x0000, 0x0FFF]`
* `[0x0800, 0x1FFF]`

shall be treated as a single region:

* `[0x0000, 0x1FFF]`

The resulting region name is implementation-defined.

# Instruction execution

@ref rvm_executeInstr shall execute exactly one instruction located at the current PC.

The function shall return:

* @ref RVM_STEP_SUCCESS if instruction execution completed normally.
* @ref RVM_STEP_FINISH if execution reached a user-defined stopping condition.
* @ref RVM_STEP_EXCEPTION if an exception occurred.

Returning @ref RVM_STEP_SUCCESS after executing multiple instructions is not permitted.

# Stop conditions

When stop mode is set to @ref RVM_STOP_BY_PC, the implementation shall return @ref RVM_STEP_FINISH when execution reaches the address specified by @ref rvm_setStopPC.

Implementations may additionally treat EBREAK as a finish condition if this behavior matches simulator conventions.

# Register access

The register access functions shall provide direct access to architectural state.

This includes:

* @ref rvm_readXReg and @ref rvm_setXReg
* @ref rvm_readFReg and @ref rvm_setFReg
* @ref rvm_readCSR and @ref rvm_setCSR
* @ref rvm_readVReg and @ref rvm_setVReg
* @ref rvm_readPC and @ref rvm_setPC

All values are represented as raw bits. Values smaller than `uint64_t` are zero-extended.

Floating-point values shall not be converted to host floating-point types, but instead use the IEEE754 bit representation.

# Reset semantics

After calling @ref rvm_modelReset, the model shall be indistinguishable from a freshly created instance built from the same configuration.

This includes:

* register contents
* memory contents
* CSR state
* interrupt state
* internal simulator state.

Resetting the model does not require callback generation.

# Error context

Implementations should provide descriptive diagnostic messages through @ref rvm_getErrorContext.

The returned message is implementation-defined, but should help users diagnose failures.

Examples include:

* invalid memory addresses
* unsupported register indices
* invalid configuration parameters
* unsupported ISA extensions

The returned string should always be a valid ASCII string.
# Callback support

RVM callbacks allow applications to observe simulator activity without directly modifying the simulator.

Callbacks are configured through @ref RVMConfig.

Supported callback types include:

* @ref MemReadCallbackTy
* @ref MemUpdateCallbackTy
* @ref XRegUpdateCallbackTy
* @ref FRegUpdateCallbackTy
* @ref VRegUpdateCallbackTy
* @ref CSRUpdateCallbackTy
* @ref PCUpdateCallbackTy

## Callback handler

All callbacks receive a pointer to @ref RVMCallbackHandler as their first argument.

The implementation shall pass the exact pointer supplied in:

@code{.c}
Config->CallbackHandler
@endcode

without modification.

The implementation must never dereference this pointer itself.

Its contents are completely user-defined.

## When callbacks should be invoked

Callbacks shall be invoked whenever the corresponding architectural state changes as a result of instruction execution.

Examples:

* A store instruction should trigger @ref MemUpdateCallbackTy.
* A load instruction should trigger @ref MemReadCallbackTy.
* Writing x5 should trigger @ref XRegUpdateCallbackTy.
* Updating a CSR should trigger @ref CSRUpdateCallbackTy.
* Changing PC should trigger @ref PCUpdateCallbackTy.

If a single instruction updates multiple architectural objects, multiple callbacks may be generated.

The order of callback invocation is implementation-defined. The only requirement is that @ref RVMConfig::PCUpdateCallback must be invoked first.

## Disabling callbacks

Callbacks shall not be invoked when:

@code{.c}
Config->CallbackHandler == NULL
@endcode

This allows applications to disable callback processing entirely.

## Optional callback support

Some simulators may be unable to efficiently provide callback information.

For such implementations callback support is optional.

If callback support is not implemented:

* No callbacks shall ever be invoked.
* Callback-related fields in @ref RVMConfig may be ignored.
* @ref rvm_queryCallbackSupportPresent shall return 0.

If callback support is implemented:

* Configured callbacks shall be invoked whenever appropriate.
* @ref rvm_queryCallbackSupportPresent shall return non-zero.

Applications may use @ref rvm_queryCallbackSupportPresent to detect whether execution tracing through callbacks is available.

# Logging

Implementations should honor:

* @ref RVMConfig::LogFilePath
* @ref RVMConfig::DebugLogFilePath

Recommended behavior:

* NULL disables logging  (in an efficient manner)
* Empty string redirects output to stdout.
* "-" redirects output to stderr.

# ISA extensions

Supported ISA extensions are described by @ref RVMExtDescriptor.

Single-letter extensions are stored in:

@code{.c}
Exts.MisaExt[]
@endcode

Multi-letter standard extensions are stored in:

@code{.c}
Exts.ZExt[]
@endcode

Custom extensions are stored in:

@code{.c}
Exts.XExt[]
@endcode

Implementations should enable all extensions requested by the user whenever supported.

Unsupported extensions should result in a clear initialization failure.

# Versioning

Every implementation shall export:

* @ref RVMAPI_ENTRY_POINT_SYMBOL
* @ref RVMAPI_VERSION_SYMBOL

The version symbol shall contain:

@code{.c}
RVMAPI_CURRENT_INTERFACE_VERSION
@endcode

Applications use this value to verify ABI compatibility before loading the simulator.

# Running the RVM smoke tests

RVM is distributed with a lightweight conformance test suite that can be used to verify that a simulator correctly implements the interface.

The tests verify common interface requirements such as:

* model creation and destruction
* memory access
* register access
* reset behavior
* instruction execution
* stop conditions
* callback support
* ISA configuration handling

## CMake integration

The test suite can be integrated into an existing CMake build with only a few lines:

@code{.cmake}
include(testgen-model-interface-launcher-tests)

if(NOT TESTGEN_MODEL_INTERFACE_LAUNCHER_TESTS_MODULE_LOADED)
  message(FATAL_ERROR "testgen-model-interface-launcher-tests module was not properly included")
endif()

set(MODEL_LAUNCHER_TESTS_DIR
${TESTGEN_MODEL_INTERFACE_LAUNCHER_TESTS_DIR})

add_model_tests()

add_dependencies(TestDependencies
TestgenModelLauncherTests)
@endcode

After configuring the project, the tests become available through the normal CTest workflow:

@code{.bash}
ctest
@endcode

or

@code{.bash}
ctest --output-on-failure
@endcode

## Interpreting failures

A failing test usually indicates that the simulator behavior differs from the RVM specification.

Common causes include:

* incorrect reset behavior
* executing more than one instruction in @ref rvm_executeInstr
* incomplete register or CSR support
* incorrect memory handling
* callback notifications not matching architectural updates
* configuration values from @ref RVMConfig not being honored

The test suite is intended to catch interface compatibility issues early and should be considered a mandatory validation step for every new RVM implementation.


A correctly implemented simulator should be usable by existing RVM-based applications without requiring any simulator-specific code.

