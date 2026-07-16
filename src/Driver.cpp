// This file contains a model launcher - a program to run RVM interface-based
// simulator.

#include "DriverUtils.hpp"

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
    RVMErrorCode Err = RVM_ERRC_SUCCESS;
    size_t i = 0;
    for (; i < MAX_ALLOWED_STEPS; ++i) {
      auto result = retireInstr(RVM, ExecLog);
      if (result == RVM_STEP_SUCCESS)
        continue;
      if (result == RVM_STEP_FINISH) {
        std::cerr << "proper simulation end\n";
        break;
      }
      if (result == RVM_STEP_EXCEPTION) {
        RVMRegT mcause = 0;
        Err = RVM.readCSR(RVM_CSR_MCAUSE, mcause);
        checkAndThrow(RVM, Err);
        if (Options.AllowExceptions == "all")
          continue;
        // historically, breakpoint exception are considered a (great) success
        // TODO: implemenent more elaborate exception filtering
        if (Options.AllowExceptions == "ebreak" && (mcause == 3))
          continue;
        std::cerr << "step failure (unexpected exception " << mcause
                  << "), terminating\n";
        ExecSuccess = false;
        break;
      }
      // NOTE: technically this is an unreachable code, however we don't
      // mark it as such to be forward-compatible
      ExecSuccess = false;
      std::cerr << "unexpected step result\n";
      break;
    }
    if (i == MAX_ALLOWED_STEPS) {
      ExecSuccess = false;
      std::cerr << "maximum number of steps (" << MAX_ALLOWED_STEPS
                << " exceeded\n";
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
