// This file contains a test driver for interrupt raise/clear functionality.
// It raises interrupt after executing a couple instructions and then clear
// interrupt if option is provided.

#include "DriverUtils.hpp"

#include <RISCVModel/RVM.hpp>

#include <optional>

int main(int argc, char *argv[]) try {
  auto Options = [](int argc, char *argv[]) {
    auto OptVal = parseCommandLine(argc, argv);
    if (!OptVal)
      throw std::invalid_argument("could not parse command line\n");
    return std::move(*OptVal);
  }(argc, argv);

  const auto &VTable = loadModelLibrary(Options);

  ExecutionLogger ExecLog(Options.JsonExecLog);

  bool IsRV64 = true;
  RVMExtDescriptor Exts = {};
  Exts.ZExtSize = sizeof(Exts.ZExt);
  Exts.XExtSize = sizeof(Exts.XExt);

  if (!Options.RVM_IsaStringOverride.empty()) {
    std::tie(Exts, IsRV64) = parse_isa_string(Options.RVM_IsaStringOverride);
  } else {
    Exts.MisaExt[RVM_MISA_I] = true;
    Exts.MisaExt[RVM_MISA_M] = true;
    Exts.MisaExt[RVM_MISA_A] = true;
    Exts.MisaExt[RVM_MISA_C] = true;
    Exts.MisaExt[RVM_MISA_F] = true;
    Exts.MisaExt[RVM_MISA_D] = true;
    Exts.MisaExt[RVM_MISA_V] = true;
    Exts.ZExtSize = sizeof(Exts.ZExt);
    Exts.XExtSize = sizeof(Exts.XExt);
    Exts.ZExt[RVM_ZEXT_ICSR] = true;
  }
  auto Builder = rvm::State::Builder(&VTable);
  Builder
      .addMemoryRegion(Options.RVM_MemoryRomStart, Options.RVM_MemoryRomSize,
                       "rom")
      .addMemoryRegion(Options.RVM_MemoryRamStart, Options.RVM_MemoryRamSize,
                       "ram")
      .setVLEN(128)
      .setLogPath(Options.RVM_ModelLogPath.c_str())
      .enableMisalignedAccess()
      .setExtensions(Exts)
      .registerCallbackHandler((RVMCallbackHandler *)&ExecLog)
      .registerMemReadCallback(memReadCallback)
      .registerMemUpdateCallback(memUpdateCallback)
      .registerXRegUpdateCallback(xRegUpdateCallback)
      .registerFRegUpdateCallback(fRegUpdateCallback)
      .registerVRegUpdateCallback(vRegUpdateCallback)
      .registerCSRUpdateCallback(csRegUpdateCallback);
  if (IsRV64)
    Builder.setRV64Isa();
  else
    Builder.setRV32Isa();

  if (Options.RVM_DebugLogPath)
    Builder.setDebugLogPath(Options.RVM_DebugLogPath->c_str());
  auto RVMOrErr = std::move(Builder).build();
  if (std::holds_alternative<std::string>(RVMOrErr))
    throw std::runtime_error("failed to build RVMState: " +
                             std::get<std::string>(RVMOrErr));
  auto &RVM = std::get<rvm::State>(RVMOrErr);

  bool ExecSuccess = true;
  if (!Options.RVM_ExecutableFile.empty()) {
    auto StopMode = RVM_STOP_NEVER;
    if (Options.RVM_StopMode == "AtLabel")
      StopMode = RVM_STOP_BY_PC;
    parseELFAndInit(RVM, Options.RVM_ExecutableFile);
    RVM.setStopMode(StopMode);
    constexpr size_t max_step = 1000000;

    size_t i = 0;
    size_t num_steps_before = Options.RVM_NumStepsBeforeInt;
    for (i = 0; i < num_steps_before; ++i) {
      auto result = retireInstr(RVM, ExecLog);

      if (result < 0) {
        std::cerr << "termination by step failure\n";
        ExecSuccess = false;
        break;
      }

      if (result == 1) {
        std::cerr << "proper simulation end\n";
        break;
      }
    }
    if (i == max_step) {
      ExecSuccess = false;
      std::cerr << "maximum step number exceeded\n";
    }
    // (1 << 63) + 3 = interrupt bit + Machine software interrupt
    constexpr uint64_t interrupt_cause = 0x8000000000000003;
    auto Err = RVM.raiseInterrupt(interrupt_cause);
    checkAndThrow(RVM, Err);
    auto max_iter = max_step;
    if (!Options.RVM_ClearInterrupt)
      max_iter = num_steps_before;
    for (i = 0; i < max_iter; ++i) {
      auto result = retireInstr(RVM, ExecLog);

      if (result < 0) {
        std::cerr << "termination by step failure\n";
        ExecSuccess = false;
        break;
      }

      if (result == 1) {
        std::cerr << "proper simulation end\n";
        break;
      }
      // Clear interrupt after first step
      if (i == 0 && Options.RVM_ClearInterrupt) {
        Err = RVM.clearInterrupt(0);
        checkAndThrow(RVM, Err);
      }
    }
    if (i == max_step) {
      ExecSuccess = false;
      std::cerr << "maximum step number exceeded\n";
    }
  }
  if (!Options.StateFile.empty()) {
    auto SerializedState = dumpState(RVM);
    if (Options.StateFile == "-") {
      std::cerr << SerializedState;
    } else {
      std::ofstream Out(Options.StateFile);
      Out << SerializedState;
      Out.close();
    }
  }

  return ExecSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
} catch (std::exception &E) {
  std::cerr << "error: " << E.what() << '\n';
  return EXIT_FAILURE;
}
