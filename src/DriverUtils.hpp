#pragma once

#include "PrintUtils.hpp"
#include "SimulatorOptions.hpp"
#include "Utils.hpp"

#include <RISCVModel/RVM.hpp>

#include <dlfcn.h>
#include <getopt.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <vector>
enum class SimOptions : int {
#define RVM_OPTION_DESC(EN, STR, IS_FLAG) EN,
  D_RVM_OPTIONS
#undef RVM_OPTION_DESC
};

// TODO: make this a configurable value
#define MAX_ALLOWED_STEPS 1000000

class ModelError : public std::runtime_error {
public:
  ModelError(const rvm::State &RVM, RVMErrorCode Err)
      : std::runtime_error([&]() -> std::string {
          return "while executing model: " + RVM.strerror(Err) + ": " +
                 RVM.getErrorContext();
        }()) {}
};

inline void checkAndThrow(const rvm::State &RVM, RVMErrorCode Errc) {
  if (Errc != RVM_ERRC_SUCCESS)
    throw ModelError(RVM, Errc);
}

namespace {

template <SimOptions n> std::string simOpt2Str() {
  assert(!"unknown option specified");
  throw std::runtime_error("unknown option specified");
};

#define RVM_OPTION_DESC(EN, STR, IS_FLAG)                                      \
  template <> std::string simOpt2Str<SimOptions::EN>() { return STR; }
D_RVM_OPTIONS
#undef RVM_OPTION_DESC

} // anonymous namespace

#define D_STRINGIFY(S) #S
#define D_XSTRINGIFY(S) D_STRINGIFY(S)

static constexpr uint64_t kDefaultMemStart = 0x80000000;
static constexpr uint64_t kDefaultMemSize = 0x10000;

struct DriverOptions {
  std::string InterfaceVersionSymbol = D_XSTRINGIFY(RVMAPI_VERSION_SYMBOL);
  std::string VTableSymbol = D_XSTRINGIFY(RVMAPI_ENTRY_POINT_SYMBOL);
  std::string LibraryPath = "-";
  std::string StateFile = "-";
  std::string JsonExecLog;
  std::string AllowExceptions = "none";

  uint64_t RVM_MemoryRomStart = kDefaultMemStart;
  uint64_t RVM_MemoryRomSize = kDefaultMemSize;
  uint64_t RVM_MemoryRamStart = kDefaultMemStart;
  uint64_t RVM_MemoryRamSize = kDefaultMemSize;
  uint64_t RVM_NumStepsBeforeInt = 9;

  std::string RVM_ModelLogPath = "rvm.log";
  std::optional<std::string> RVM_DebugLogPath = std::nullopt;
  std::string RVM_ExecutableFile;
  std::string RVM_IsaStringOverride = "";
  std::string RVM_StopMode;

  bool RVM_ClearInterrupt = false;

  constexpr static const char *kModelLibArg = "driver-model-lib";
  constexpr static const char *kOutputState = "driver-output-state-file";
  constexpr static const char *kJsonExecLog = "driver-json-exec-log";
  constexpr static const char *kAllowExceptions = "driver-allow-exceptions";

  constexpr static const char *kRomStart = "driver-model-mem-rom-start";
  constexpr static const char *kRomSize = "driver-model-mem-rom-size";
  constexpr static const char *kRamStart = "driver-model-mem-ram-start";
  constexpr static const char *kRamSize = "driver-model-mem-ram-size";

  constexpr static const char *kClearInterrupt = "driver-model-clear-interrupt";
  constexpr static const char *kNumStepsBeforeInt =
      "driver-model-num-steps-before-interrupt";

  template <typename T>
  void dumpOptionField(std::string OptString, const T &Value) {
    std::cerr << "--" << OptString << " " << Value << std::endl;
  }

  void dump() {
    dumpOptionField("VTable", VTableSymbol);
    dumpOptionField(kModelLibArg, LibraryPath);
    dumpOptionField(kOutputState, StateFile);
    dumpOptionField(kJsonExecLog, JsonExecLog);
    dumpOptionField(kAllowExceptions, AllowExceptions);
    dumpOptionField(kRomStart, rvm_utils::tohs(RVM_MemoryRomStart, 0));
    dumpOptionField(kRomSize, rvm_utils::tohs(RVM_MemoryRomSize, 0));
    dumpOptionField(kRamStart, rvm_utils::tohs(RVM_MemoryRamStart, 0));
    dumpOptionField(kRamSize, rvm_utils::tohs(RVM_MemoryRamSize, 0));
    dumpOptionField(simOpt2Str<SimOptions::IsaStringOverride>(),
                    RVM_IsaStringOverride);
    dumpOptionField(simOpt2Str<SimOptions::ModelLogPath>(), RVM_ModelLogPath);
    if (0)
      dumpOptionField(simOpt2Str<SimOptions::DebugLogPath>(), "");
    dumpOptionField(simOpt2Str<SimOptions::ExecFile>(), RVM_ExecutableFile);
    dumpOptionField(simOpt2Str<SimOptions::StopMode>(), RVM_StopMode);
    dumpOptionField(kClearInterrupt, RVM_ClearInterrupt);
    dumpOptionField(kNumStepsBeforeInt, RVM_NumStepsBeforeInt);
  }
};

enum ExtOpts : int {
  ModelLib = 1,
  AllowExceptions,
  OutputState,
  JsonExecLog,
  RomStart,
  RomSize,
  RamStart,
  RamSize,
  ClearInterrupt,
  NumStepsBeforeInt,
};

std::optional<DriverOptions> parseCommandLine(int argc, char *argv[]);

void *loadLibrary(const std::string &LibPath);

void *findModelLibrary(const std::string &Path);

const rvm::RVM_FunctionPointers &loadModelLibrary(const DriverOptions &Options);

std::string dumpState(rvm::State &RVM);

struct StateUpdate {
  // TODO: we can optimize memory management by introducing dedicated cache
  struct MemStateType {
    uint64_t Addr;
    std::vector<unsigned char> Data;
  };

  struct VectorRegUpdateType {
    RVMVReg RegIndex;
    std::vector<unsigned char> Data;
  };

  std::vector<MemStateType> MemReads;
  std::vector<MemStateType> MemWrites;
  std::vector<RVMXReg> GPR;
  std::vector<RVMFReg> FPR;
  std::vector<VectorRegUpdateType> VReg;
  std::vector<RVMCSR> CSR;
  uint64_t PC;
  uint64_t FCSR;

  void clear();
};

constexpr auto kUninitializedValue = std::numeric_limits<uint64_t>::max();

class ExecutionLogger {
public:
  ExecutionLogger(const std::string &ExecLogPath);
  ~ExecutionLogger();

  void logPreRetireState(const rvm::State &ModelState);
  void logRetiredState(const rvm::State &ModelState);
  void terminate();

  void addMemoryRead(uint64_t Addr, const unsigned char *Data, size_t Size);
  void addMemoryUpdate(uint64_t Addr, const unsigned char *Data, size_t Size);
  void addXRegUpdate(RVMXReg Reg);
  void addFRegUpdate(RVMFReg Reg);
  void addVRegUpdate(RVMVReg VReg, const unsigned char *Data, size_t Value);
  void addCSRUpdate(RVMCSR Reg);

private:
  uint64_t PCBefore = kUninitializedValue;
  uint64_t FCSRBefore = kUninitializedValue;
  StateUpdate Update;
  std::ofstream OS;
};

void memReadCallback(RVMCallbackHandler *handler, uint64_t Addr,
                     const char *Data, size_t Size);

void memUpdateCallback(RVMCallbackHandler *handler, uint64_t Addr,
                       const char *Data, size_t Size);

void xRegUpdateCallback(RVMCallbackHandler *handler, RVMXReg Reg, RVMRegT);

void fRegUpdateCallback(RVMCallbackHandler *handler, RVMFReg Reg, RVMRegT);

void vRegUpdateCallback(RVMCallbackHandler *handler, RVMVReg VReg,
                        const char *Data, size_t Len);

void csRegUpdateCallback(RVMCallbackHandler *handler, RVMCSR Reg, RVMRegT);

std::string emitMemoryState(const StateUpdate::MemStateType &MemState);

template <class InputIt>
std::string emitMemoryStateList(InputIt first, InputIt last) {
  std::stringstream SS;
  std::string Separator;

  std::for_each(first, last, [&](auto &&it) {
    SS << Separator << emitMemoryState(it);
    Separator = ", ";
  });

  return SS.str();
}

template <class InputIt>
std::string emitChangeXRegisters(InputIt first, InputIt last,
                                 const rvm::State &ModelState) {
  std::stringstream SS;
  std::string Separator;

  RVMErrorCode Err = RVM_ERRC_SUCCESS;
  std::for_each(first, last, [&](auto &&it) {
    RVMRegT Value = 0;
    Err = ModelState.readXReg(it, Value);
    checkAndThrow(ModelState, Err);
    SS << Separator << "\"x" << it << "\": \"" << rvm_utils::tohs(Value)
       << "\"";
    Separator = ", ";
  });

  return SS.str();
}

template <class InputIt>
std::string emitChangeFRegisters(InputIt first, InputIt last,
                                 const rvm::State &ModelState) {
  std::stringstream SS;
  std::string Separator;

  RVMErrorCode Err = RVM_ERRC_SUCCESS;
  std::for_each(first, last, [&](auto &&it) {
    RVMRegT Value = 0;
    Err = ModelState.readFReg(it, Value);
    checkAndThrow(ModelState, Err);
    SS << Separator << "\"f" << it << "\": \"" << rvm_utils::tohs(Value)
       << "\"";
    Separator = ", ";
  });

  return SS.str();
}

std::string
emitChangeVRegister(const StateUpdate::VectorRegUpdateType &VUpdate);

template <class InputIt>
std::string emitChangeVRegisters(InputIt first, InputIt last) {
  std::stringstream SS;
  std::string Separator;

  std::for_each(first, last, [&](auto &&vRegUpdate) {
    SS << Separator << emitChangeVRegister(vRegUpdate);
    Separator = ", ";
  });
  return SS.str();
}

RVMSimExecStatus retireInstr(rvm::State &RVM, ExecutionLogger &Logger);

void parseELFAndInit(rvm::State &RVM, std::string_view Path);

class invalid_isa_string : public std::invalid_argument {
public:
  invalid_isa_string(std::string_view ISA, std::string_view Msg)
      : std::invalid_argument("Invalid ISA string \"" + std::string(ISA) +
                              "\": " + std::string(Msg)) {}
};

namespace detail {
inline unsigned parse_isa_bitness(std::string_view Str) {
  unsigned Res = 0;
  auto [Ptr, EC] = std::from_chars(Str.begin(), Str.end(), Res, /*Base*/ 10);
  if (EC == std::errc::invalid_argument)
    throw std::invalid_argument("string should contain bitness");
  if (EC == std::errc::result_out_of_range)
    throw std::invalid_argument("bitness is too large");
  if (Res != 32u && Res != 64u)
    throw std::invalid_argument(
        "only 32 and 64-bit architectures are supported");
  return Res;
}

// Split extensions on underscore
inline std::vector<std::string_view> split_extensions(std::string_view Isa) {
  std::vector<std::string_view> Extensions;
  assert(Isa[0] == '_' || Isa[0] == 'z' || Isa[0] == 'x');
  size_t Current = Isa[0] == '_';
  auto NextUnderscore = Isa.find('_', Current + 1);
  while (NextUnderscore != Isa.npos) {
    Extensions.emplace_back(Isa.substr(Current, NextUnderscore - Current));
    Current = NextUnderscore + 1;
    NextUnderscore = Isa.find('_', Current + 1);
  }
  Extensions.emplace_back(Isa.substr(Current));
  return Extensions;
}

} // namespace detail

#ifdef RVM_ADD_MISA_BITS_CASE
#error RVM_ADD_MISA_BITS_CASE should not be defined at this point
#else
#define RVM_ADD_MISA_BITS_CASE(NAME, name)                                     \
  if (E == #name) {                                                            \
    auto Found = std::find(&Ext.MisaExt[NAME],                                 \
                           &Ext.MisaExt[0] + sizeof(Ext.MisaExt), 1);          \
    if (Found != &Ext.MisaExt[0] + sizeof(Ext.MisaExt))                        \
      throw std::invalid_argument(                                             \
          "extension \"" #name                                                 \
          "\" is on the incorrect position. ISA extensions should be in the "  \
          "following order: IMAFDGQCBPVH");                                    \
    Ext.MisaExt[NAME] = 1;                                                     \
  }
#endif

#ifdef RVM_ADD_ZEXT_BITS
#error RVM_ADD_ZEXT_BITS should not be defined at this point
#else
#define RVM_ADD_ZEXT_BITS(NAME, name)                                          \
  if (E.substr(1) == #name)                                                    \
    Ext.ZExt[NAME] = 1;                                                        \
  else
#endif

#ifdef RVM_ADD_XEXT_BITS
#error RVM_ADD_XEXT_BITS should not be defined at this point
#else
#define RVM_ADD_XEXT_BITS(NAME, name)                                          \
  if (E.substr(1) == #name)                                                    \
    Ext.XExt[NAME] = 1;                                                        \
  else
#endif
inline std::pair<RVMExtDescriptor, bool>
parse_isa_string(std::string_view InputIsa) {
  constexpr static std::string_view IsaPrefix = "rv";
  std::string IsaStr;
  IsaStr.reserve(InputIsa.size());
  std::transform(InputIsa.begin(), InputIsa.end(), std::back_inserter(IsaStr),
                 [](auto C) { return std::tolower(C); });
  std::string_view Isa = IsaStr;
  // TODO: replace with starts_with after moving to C++20
  if (Isa.find(IsaPrefix) != 0)
    throw invalid_isa_string(InputIsa,
                             "isa string should start with \"rv\" prefix");
  Isa.remove_prefix(IsaPrefix.size());
  auto FirstExtPos = Isa.find_first_not_of("0123456789");
  unsigned Bits = 0;
  try {
    Bits = detail::parse_isa_bitness(Isa.substr(0, FirstExtPos));
  } catch (std::exception &e) {
    throw invalid_isa_string(InputIsa, e.what());
  }
  Isa.remove_prefix(FirstExtPos);

  RVMExtDescriptor Ext = {};
  Ext.ZExtSize = sizeof(Ext.ZExt);
  Ext.XExtSize = sizeof(Ext.XExt);

  auto FirstStandard = Isa.find_first_of("_zx");
  auto MaxMisa = FirstStandard == Isa.npos ? Isa.size() : FirstStandard;
  // parse all single-letter extensions
  for (auto i = 0u; i < MaxMisa; ++i) {
    auto E = Isa.substr(i, 1);
    RVM_FOR_EACH_MISA_EXT(RVM_ADD_MISA_BITS_CASE)
  }
  if (FirstStandard != Isa.npos) {
    Isa.remove_prefix(FirstStandard);

    // split on underscore and parse standard/custom extensions
    auto ExtensionStrings = detail::split_extensions(Isa);
    for (auto E : ExtensionStrings) {
      if (E[0] == 'z') {
        RVM_FOR_EACH_ZEXT(RVM_ADD_ZEXT_BITS)
        throw invalid_isa_string(InputIsa, "unknown standard extension \"" +
                                               std::string(E) + "\" specified");
      } else if (E[0] == 'x') {
        RVM_FOR_EACH_XEXT(RVM_ADD_XEXT_BITS)
        throw invalid_isa_string(InputIsa, "unknown custom extension \"" +
                                               std::string(E) + "\" specified");
      } else {
        throw invalid_isa_string(
            InputIsa, "extension should have 'z' or 'x' after an underscore");
      }
    }
  }
  Ext = rvm::detail::normalize_extensions(Ext, (Bits == 64u));
  if (!Ext.MisaExt[RVM_MISA_I] && !Ext.MisaExt[RVM_MISA_E])
    throw invalid_isa_string(InputIsa, "either E or I should be available");
  return std::make_pair(std::move(Ext), /*IsRV64*/ (Bits == 64u));
}
#undef RVM_ADD_MISA_BITS_CASE
#undef RVM_ADD_ZEXT_BITS
#undef RVM_ADD_XEXT_BITS

