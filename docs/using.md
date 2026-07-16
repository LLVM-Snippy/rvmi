@page user-tutorial How to use RVM

# Getting started

This guide explains how to use an RVM-compatible simulator from your application.

The purpose of RVM is to provide a common interface for RISC-V simulators. Your application interacts only with the RVM API, while the actual simulator implementation is provided by a dynamically loaded library.

This allows switching between different simulator backends without changing your application code.

Typical use cases include:

* Running a simulator as a library.
* Switching between simulator implementations.
* Building verification environments.
* Running lock-step co-simulation between multiple models.

The typical workflow is:

1. Load an RVM-compatible simulator implementation.
2. Create a @ref rvm::State::Builder.
3. Configure the simulated system.
4. Build a @ref rvm::State instance.
5. Load a program into memory.
6. Execute instructions.
7. Inspect registers, memory and other architectural state.

# Creating a simulator instance

Every simulation starts with a @ref rvm::State::Builder.

The builder collects all simulator configuration and eventually creates a @ref rvm::State object.

A minimal example:

@code{.cpp}
auto Builder = rvm::State::Builder(&VTable);

Builder
  .addMemoryRegion(0x00000000, 0x10000, "rom")
  .addMemoryRegion(0x80000000, 0x100000, "ram")
  .setRV64Isa();

// returns std::string on failure
std::variant<rvm::State, std::string> Result = Builder.build();

if(std::holds_alternative<std::string>(Result)) {
    std::cerr
        << std::get<std::string>(Result)
        << "\n";
    return;
}

rvm::State State =
    std::move(std::get<rvm::State>(Result));

@endcode

The resulting @ref rvm::State object represents a complete simulator instance.

# Configuring memory

The memory map visible to the simulated system is configured using memory regions.

Memory regions are specified using
@ref rvm::State::Builder::addMemoryRegion().

@code{.cpp}
Builder
  .addMemoryRegion(0x00000000, 0x10000, "rom")
  .addMemoryRegion(0x80000000, 0x100000, "ram");
@endcode

The region name is optional and is used only for debugging and diagnostics.

Memory regions may overlap. Implementations are required to merge intersecting regions into a single accessible memory range.

# Selecting RV32 or RV64

RVM supports both RV32 and RV64 models.

Select the ISA width before creating the simulator instance:

@code{.cpp}
Builder.setRV32Isa();
@endcode

or

@code{.cpp}
Builder.setRV64Isa();
@endcode

# Configuring ISA extensions

RVM represents ISA extensions using @ref RVMExtDescriptor.

Before using the descriptor, initialize the ABI compatibility fields:

@code{.cpp}
RVMExtDescriptor Exts = {};
Exts.ZExtSize = sizeof(Exts.ZExt);
Exts.XExtSize = sizeof(Exts.XExt);
@endcode

## Single-letter extensions

Single-letter extensions are configured through
@ref RVMExtDescriptor::MisaExt.

For example, to enable RV64IMAFDC:

@code{.cpp}
Exts.MisaExt[RVM_MISA_I] = true;
Exts.MisaExt[RVM_MISA_M] = true;
Exts.MisaExt[RVM_MISA_A] = true;
Exts.MisaExt[RVM_MISA_F] = true;
Exts.MisaExt[RVM_MISA_D] = true;
Exts.MisaExt[RVM_MISA_C] = true;
@endcode

To enable vector instructions:

@code{.cpp}
Exts.MisaExt[RVM_MISA_V] = true;
@endcode

## Multi-letter standard extensions

Multi-letter standard extensions are configured through
@ref RVMExtDescriptor::ZExt.

For example, to enable Zicsr and Zifencei:

@code{.cpp}
Exts.ZExt[RVM_ZEXT_ICSR] = true;
Exts.ZExt[RVM_ZEXT_IFENCEI] = true;
@endcode

To enable Bit Manipulation:

@code{.cpp}
Exts.ZExt[RVM_ZEXT_BITMANIP] = true;
@endcode

To enable compressed subsets:

@code{.cpp}
Exts.ZExt[RVM_ZEXT_CA] = true;
Exts.ZExt[RVM_ZEXT_CB] = true;
@endcode

To enable vector cryptography extensions:

@code{.cpp}
Exts.ZExt[RVM_ZEXT_VKNED] = true;
Exts.ZExt[RVM_ZEXT_VKSH] = true;
@endcode

## Composite extensions

Several RISC-V extensions are composites consisting of multiple subextensions.

For example:

@code{.cpp}
Exts.MisaExt[RVM_MISA_G] = true;
@endcode

represents:

* I
* M
* A
* F
* D
* Zicsr
* Zifencei

Similarly:

@code{.cpp}
Exts.ZExt[RVM_ZEXT_BITMANIP] = true;
@endcode

represents:

* Zba
* Zbb
* Zbc
* Zbs

Composite extensions are automatically expanded by
@ref rvm::create_isa_string().

## Custom extensions

Implementation-specific extensions are configured through
@ref RVMExtDescriptor::XExt.

For example if you add your custom extension xmyown to @ref RVMXExt enum as RVM_XEXT_MYOWN you can enable it via

@code{.cpp}
Exts.XExt[RVM_XEXT_MYOWN] = true;
@endcode

Just make sure your RVM implementation knows about this extension

## Applying extension configuration

After configuration, pass the descriptor to the builder:

@code{.cpp}
Builder.setExtensions(Exts);
@endcode

To generate a canonical ISA string:

@code{.cpp}
auto ISA = rvm::create_isa_string(Exts, /*IsRV64=*/ true, /*IsLowercase=*/ true);
std::cout << ISA << '\n';
@endcode

# Configuring vector support

If the simulator implements the RISC-V Vector Extension, the vector register length may be configured using
@ref rvm::State::Builder::setVLEN().

@code{.cpp}
Builder.setVLEN(128);
@endcode

The value is specified in bits.

# Logging

Execution logs may be enabled using:

@code{.cpp}
Builder.setLogPath("sim.log");
@endcode

Debug logs may be configured independently:

@code{.cpp}
Builder.setDebugLogPath("debug.log");
@endcode

Special values:

* `nullptr` - disable logging
* `""` - write to stdout
* `"-"` - write to stderr

# Callbacks

RVM supports optional callbacks that allow applications to observe simulator
activity. Model implementation that supports callbacks (i.e.
@ref rvm_queryCallbackSupportPresent return true) it will call these necessary
callbacks if executed instruction did one of:

* Memory reads
* Memory writes
* General-purpose register updates
* Floating-point register updates
* Vector register updates
* CSR updates
* PC updates

Callbacks are registered before creating the simulator instance.

@code{.cpp}
Builder
  .registerCallbackHandler(&MyHandler)
  .registerMemReadCallback(MyMemReadCallback)
  .registerMemUpdateCallback(MyMemWriteCallback)
  .registerXRegUpdateCallback(MyRegCallback);
@endcode

The callback handler pointer is passed back to every callback invocation and can be used to store application-specific context.

Callbacks are commonly used for:

* Instruction tracing
* Register tracing
* Co-simulation
* Coverage collection
* Debugging

# Building the simulator

Once configuration is complete, create the simulator instance:

@code{.cpp}
auto State = Builder.build();
@endcode

After this point all simulator interaction happens through the resulting
@ref rvm::State object.

# Loading programs

RVM provides generic memory access functions that can be used to load machine code and data into simulator memory.

For example:

@code{.cpp}
RVMErrorCode Err = State.writeMem(LoadAddress, ProgramSize, ProgramBytes);
@endcode

The program counter can then be initialized:

@code{.cpp}
auto Err = State.setPC(EntryPoint);
@endcode

Many applications use their own ELF loader and populate memory using
@ref rvm::State::writeMem().

# Executing instructions

The fundamental execution primitive in RVM is single-instruction execution.

@code{.cpp}
auto Result = State.executeInstr();
@endcode

This executes the instruction currently pointed to by the PC register.

A simple execution loop:

@code{.cpp}
while (true) {
  auto Result = State.executeInstr();

  if (Result != RVM_STEP_SUCCESS)
    break;
}
@endcode

The return value is an @ref RVMSimExecStatus.

Possible results are:

* @ref RVM_STEP_SUCCESS - instruction executed normally.
* @ref RVM_STEP_FINISH - simulation reached a configured stop condition.
* @ref RVM_STEP_EXCEPTION - an exception occurred.

# Stop conditions

By default, simulation continues indefinitely.

To stop execution when a specific PC value is reached:

@code{.cpp}
State.setStopMode(STOP_BY_PC);
RVMErrorCode Err = State.setStopPC(0x80001000);
@endcode

Usually setStopPC can only fail if NewPC can't fit into actual PC (i.e. 64-bit value for rv32 model)

When the PC reaches the configured address, subsequent execution returns
@ref RVM_STEP_FINISH.

# Reading and modifying registers

## General-purpose registers

Read a register:

@code{.cpp}
RVMRegT Reg = 0;
RVMErrorCode Err = State.readXReg(RVM_X_REG_10, Reg);
if (Err != RVM_ERRC_SUCCESS) {
  std::cerr << "error while reading X10: " << State.strerror(Err) <<": " << State.getErrorContext() << '\n';
  fatal();
}
std::cout << "X10 value is " << std::hex << Reg << '\n';
@endcode

In general X, F and CSR register reads can only fail if invalid register was specified.
In our case it is RVM_X_REG_10 (valid) so this should not fail.

Write a register:

@code{.cpp}
RVMErrorCode Err = State.setXReg(RVM_X_REG_10, 1234);
if (Err != RVM_ERRC_SUCCESS) {....}
@endcode

## Program counter

Read PC:

@code{.cpp}
auto PC = State.readPC();
@endcode

Write PC:

@code{.cpp}
RVMErrorCode = State.setPC(NewPC);
if (Err != RVM_ERRC_SUCCESS) {
  std::cerr << "error while trying to setPC: " << State.strerror(Err) << ": " << State.getErrorContext() << '\n';
}
@endcode

Usually setPC can only fail if NewPC can't fit into actual PC (i.e. 64-bit value for rv32 model)

## Floating-point registers

Read:

@code{.cpp}
RVMRegT Bits = 0;
RVMErrorCode Err = State.readFReg(RVM_F_REG_0, Bits);
if (Err != RVM_ERRC_SUCCESS) {
  std::cerr << "error while reading F0: " << State.strerror(Err) <<": " << State.getErrorContext() << '\n';
  fatal();
}
std::cout << "F0 value is " << std::hex << Reg << '\n';
@endcode

In general X, F and CSR register reads can only fail if invalid register was specified.

Write:

@code{.cpp}
auto Err = State.setFReg(RVM_F_REG_0, 0x7fc00000u);
if (Err != RVM_ERRC_SUCCESS) {....}
@endcode

Floating-point registers are accessed as raw bit patterns.

## CSRs

Read:

@code{.cpp}
RVMRegT MStatus = 0;
auto Err = State.readCSR(RVM_CSR_MSTATUS, mstatus);
if (Err != RVM_ERRC_SUCCESS) {
  std::cerr << "error while reading MSTATUS: " << State.strerror(Err) <<": " << State.getErrorContext() << '\n';
  fatal();
}
std::cout << "MSTATUS value is " << std::hex << MStatus << '\n';
@endcode

In general X, F and CSR register reads can only fail if invalid register was specified.

Write:

@code{.cpp}
auto Err = State.setCSR(RVM_CSR_MSTATUS, NewValue);
if (Err != RVM_ERRC_SUCCESS) {....}
@endcode

Commonly used CSRs include:

* @ref RVM_CSR_MSTATUS
* @ref RVM_CSR_MISA
* @ref RVM_CSR_MEPC
* @ref RVM_CSR_MCAUSE
* @ref RVM_CSR_MTVAL

# Accessing memory

Memory may be accessed directly.

Read memory:

@code{.cpp}
uint32_t Value;
RVMErrorCode Err = State.readMem(0x80000000, 1, &Value);
if (Err != RVM_ERRC_SUCCESS) {....}
@endcode

Write memory:

@code{.cpp}
RVMErrorCode Err = State.writeMem(0x80000000, 1, &Value);
if (Err != RVM_ERRC_SUCCESS) {....}
@endcode

The template type determines the element size.

For example:

@code{.cpp}
uint8_t Byte;
uint32_t Word;
uint64_t DoubleWord;

auto Err = State.readMem(Addr, 1, &Byte);
if (Err != RVM_ERRC_SUCCESS) {....}
Err = State.readMem(Addr, 1, &Word);
if (Err != RVM_ERRC_SUCCESS) {....}
Err = State.readMem(Addr, 1, &DoubleWord);
if (Err != RVM_ERRC_SUCCESS) {....}
@endcode

# Accessing vector registers

Vector registers are transferred as raw byte arrays.

Read:

@code{.cpp}
unsigned VLENB = 0;
RVMErrorCode Err = State.readVReg(RVM_V_REG_0, nullptr, VLENB);
std::vector<char> Buffer(VLENB);



RVMErrorCode Err = State.readVReg(RVM_V_REG_0, Buffer.data(), Buffer.size());
@endcode

Write:

@code{.cpp}
RVMErrorCode Err = State.setVReg(RVM_V_REG_0, Buffer.data(), Buffer.size());
@endcode

The required buffer size can be obtained through the @ref RVM_CSR_VLENB CSR.

# Interrupts

External interrupts can be injected into the simulator.

Raise an interrupt:

@code{.cpp}
RVMErrorCode Err = State.raiseInterrupt(Cause);
@endcode

Clear an interrupt:

@code{.cpp}
RVMErrorCode Err = State.clearInterrupt(Cause);
@endcode

The exact interpretation of the cause value follows the RISC-V privileged architecture and the simulator implementation.

# Resetting simulation

A simulator instance can be reset at any time.

@code{.cpp}
State.reset();
@endcode

After reset, the simulator must behave identically to a newly created simulator built from the same configuration.

# Error handling

Textual error description:

```cpp
std::cerr
    << rvm::State::strerror(Err)
    << "\n";
```

Implementation-specific context:

```cpp
std::string ErrMsg = State.getErrorContext();
std::cerr << ErrMsg << "\n";
```

Full error message could look as follows:

```cpp
std::string ErrBase = State.strerror(Err);
std::string ErrMsg = State.getErrorContext();
std::cerr << "error: " << ErrBase << ": " << ErrMsg << "\n";
```

# Co-simulation

One of the primary goals of RVM is to simplify co-simulation.

Multiple simulators can be instantiated and driven using the same API.

@code{.cpp}
auto Sim1Builder = rvm::State::Builder(&VTableForSim1);
auto Sim2Builder = rvm::State::Builder(&VTableForSim2);
auto Sim1 = unwrap_variant(Sim1Builder.build());
auto Sim2 = unwrap_variant(Sim2Builder.build());

while (true) {
  auto A = Sim1.executeInstr();
  auto B = Sim2.executeInstr();

  if (A != RVM_STEP_SUCCESS || B != RVM_STEP_SUCCESS)
    break;

  if (Sim1.readPC() != Sim2.readPC()) {
    std::cerr << "PC mismatch\n";
    break;
  }
}
@endcode

Because every simulator exposes the same interface, the same verification infrastructure can be reused regardless of the underlying implementation.

