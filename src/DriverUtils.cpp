#include "DriverUtils.hpp"

#include <elfio/elfio.hpp>

namespace {
uint64_t parseMemoryTrait(const std::string MemTrait) {
  return std::stoull(MemTrait, nullptr, 0);
}
} // namespace

std::optional<DriverOptions> parseCommandLine(int argc, char *argv[]) {
  DriverOptions Result;
  static struct option long_options[] = {
#define RVM_OPTION_DESC(EN, STR, IS_ARG)                                       \
  {STR, (IS_ARG) ? required_argument : no_argument, 0, 0},
      D_RVM_OPTIONS
#undef RVM_OPTION_DESC
#define D_EXTRA_DRIVER_OPTION(Option, IntId)                                   \
  {Option, required_argument, 0, static_cast<int>(IntId)}
          D_EXTRA_DRIVER_OPTION(DriverOptions::kModelLibArg, ExtOpts::ModelLib),
      D_EXTRA_DRIVER_OPTION(DriverOptions::kAllowExceptions,
                            ExtOpts::AllowExceptions),
      D_EXTRA_DRIVER_OPTION(DriverOptions::kOutputState, ExtOpts::OutputState),
      D_EXTRA_DRIVER_OPTION(DriverOptions::kJsonExecLog, ExtOpts::JsonExecLog),
      D_EXTRA_DRIVER_OPTION(DriverOptions::kRomStart, ExtOpts::RomStart),
      D_EXTRA_DRIVER_OPTION(DriverOptions::kRomSize, ExtOpts::RomSize),
      D_EXTRA_DRIVER_OPTION(DriverOptions::kRamStart, ExtOpts::RamStart),
      D_EXTRA_DRIVER_OPTION(DriverOptions::kRamSize, ExtOpts::RamSize),
      D_EXTRA_DRIVER_OPTION(DriverOptions::kClearInterrupt,
                            ExtOpts::ClearInterrupt),
      D_EXTRA_DRIVER_OPTION(DriverOptions::kNumStepsBeforeInt,
                            ExtOpts::NumStepsBeforeInt),
      {0, 0, 0, 0}};
#undef D_EXTRA_DRIVER_OPTION
  int c = 0;
  int option_index = 0;
  while ((c = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
    switch (c) {
    case 0: {
      SimOptions OptIndex = (SimOptions)option_index;
      switch (OptIndex) {
      case SimOptions::ModelLogPath:
        Result.RVM_ModelLogPath = optarg;
        break;
      case SimOptions::DebugLogPath:
        Result.RVM_DebugLogPath = optarg;
        break;
      case SimOptions::ExecFile:
        Result.RVM_ExecutableFile = optarg;
        break;
      case SimOptions::IsaStringOverride:
        Result.RVM_IsaStringOverride = optarg;
        break;
      case SimOptions::StopMode:
        Result.RVM_StopMode = optarg;
        break;
      default:
        assert(!"should not happen");
        return {};
      }
      break;
    }
    case ExtOpts::OutputState:
      Result.StateFile = optarg;
      break;
    case ExtOpts::ModelLib:
      Result.LibraryPath = optarg;
      break;
    case ExtOpts::AllowExceptions:
      Result.AllowExceptions = optarg;
      break;
    case ExtOpts::JsonExecLog:
      Result.JsonExecLog = optarg;
      break;
    case ExtOpts::ClearInterrupt:
      Result.RVM_ClearInterrupt = true;
      break;
    case ExtOpts::NumStepsBeforeInt:
      Result.RVM_NumStepsBeforeInt = parseMemoryTrait(optarg);
      break;
    case ExtOpts::RomStart:
      Result.RVM_MemoryRomStart = parseMemoryTrait(optarg);
      break;
    case ExtOpts::RomSize:
      Result.RVM_MemoryRomSize = parseMemoryTrait(optarg);
      break;
    case ExtOpts::RamStart:
      Result.RVM_MemoryRamStart = parseMemoryTrait(optarg);
      break;
    case ExtOpts::RamSize:
      Result.RVM_MemoryRamSize = parseMemoryTrait(optarg);
      break;
    default:
      assert(c == '?');
      return {};
    }
  }
  Result.dump();
  return Result;
}

void *loadLibrary(const std::string &LibPath) {
  void *Result = dlopen(LibPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
  if (!Result) {
    std::cerr << "  could not load: " << dlerror() << "\n";
  }
  return Result;
}

void *findModelLibrary(const std::string &Path) {
  void *Result = nullptr;

  if (Path != "-")
    return loadLibrary(Path);

  std::cerr << "  model library path not specified, trying default "
            << MODEL_PATH << "...\n";
  Result = loadLibrary(MODEL_PATH);
  if (Result)
    return Result;

  std::string ModelLib = std::filesystem::path(MODEL_PATH).filename();
  std::string CwdLibPath = "./" + ModelLib;
  std::cerr << "  trying to load from current dir " << CwdLibPath << "...\n";
  return loadLibrary(CwdLibPath);
}

const rvm::RVM_FunctionPointers &
loadModelLibrary(const DriverOptions &Options) {
  void *LibHandle = findModelLibrary(Options.LibraryPath);

  const void *InterfaceVersionPtr =
      dlsym(LibHandle, Options.InterfaceVersionSymbol.c_str());
  if (!InterfaceVersionPtr)
    throw std::runtime_error(std::string("could not get ") +
                             Options.InterfaceVersionSymbol +
                             " symbol: " + dlerror() + "\n");

  auto Version = *reinterpret_cast<const unsigned char *>(InterfaceVersionPtr);
  if (Version != RVMAPI_CURRENT_INTERFACE_VERSION) {
    std::ostringstream ErrorMess;
    ErrorMess << "incompatible model interface version! "
              << "got " << static_cast<unsigned>(Version)
              << ", expecting: " << RVMAPI_CURRENT_INTERFACE_VERSION << "\n";
    throw std::runtime_error(ErrorMess.str());
  }

  const void *VTablePointer = dlsym(LibHandle, Options.VTableSymbol.c_str());
  if (!VTablePointer)
    throw std::runtime_error(std::string("could not get ") +
                             Options.VTableSymbol + " symbol: " + dlerror() +
                             "\n");

  const auto *VTable =
      reinterpret_cast<const rvm::RVM_FunctionPointers *>(VTablePointer);
  std::cout << "VTablePtr: " << VTable << "\n";
  return *VTable;
}

std::string dumpState(rvm::State &RVM) {
  std::ostringstream OS;
  RVMErrorCode Err = RVM_ERRC_SUCCESS;
  for (size_t i = 0; i < 32; ++i) {
    RVMRegT GPRVal = 0;
    Err = RVM.readXReg(static_cast<RVMXReg>(i), GPRVal);
    checkAndThrow(RVM, Err);
    OS << "GPR(" << rvm_utils::printRegNumber(i)
       << "): " << rvm_utils::tohs(GPRVal) << "\n";
  }
  for (size_t i = 0; i < 32; ++i) {
    RVMRegT FPRVal = 0;
    Err = RVM.readFReg(static_cast<RVMFReg>(i), FPRVal);
    checkAndThrow(RVM, Err);
    OS << "FPR(" << rvm_utils::printRegNumber(i)
       << "): " << rvm_utils::tohs(FPRVal) << "\n";
  }
  size_t DataSize = 16;
  Err = RVM.readVReg(RVM_V_REG_0, nullptr, DataSize);
  assert(Err == RVM_ERRC_SUCCESS);
  std::vector<char> RVVData(DataSize);
  for (size_t i = 0; i < 32; ++i) {
    Err = RVM.readVReg(static_cast<RVMVReg>(i), RVVData.data(), DataSize);
    checkAndThrow(RVM, Err);
    OS << "V(" << rvm_utils::printRegNumber(i)
       << "): " << rvm_utils::tohs(RVVData) << "\n";
  }
  OS << "___\n";
  RVMRegT Misa = 0;
  Err = RVM.readCSR(RVM_CSR_MISA, Misa);
  checkAndThrow(RVM, Err);
  OS << "MISA: " << rvm_utils::tohs(Misa) << "\n";
  RVMRegT Mstatus = 0;
  Err = RVM.readCSR(RVM_CSR_MSTATUS, Mstatus);
  checkAndThrow(RVM, Err);
  OS << "MSTATUS: " << rvm_utils::tohs(Mstatus) << "\n";
  RVMRegT Mcause = 0;
  Err = RVM.readCSR(RVM_CSR_MCAUSE, Mcause);
  checkAndThrow(RVM, Err);
  OS << "MCAUSE: " << rvm_utils::tohs(Mcause) << "\n";
  RVMRegT Mtvec = 0;
  Err = RVM.readCSR(RVM_CSR_MTVEC, Mtvec);
  checkAndThrow(RVM, Err);
  OS << "MTVEC: " << rvm_utils::tohs(Mtvec) << "\n";
  return OS.str();
}

void StateUpdate::clear() {
  MemReads.clear();
  MemWrites.clear();
  GPR.clear();
  FPR.clear();
  VReg.clear();
  PC = kUnitializedValue;
  FCSR = kUnitializedValue;
}

void memReadCallback(RVMCallbackHandler *handler, uint64_t Addr,
                     const char *Data, size_t Size) {
  auto *AsLogger = (ExecutionLogger *)handler;
  AsLogger->addMemoryRead(Addr, reinterpret_cast<const unsigned char *>(Data),
                          Size);
}

void memUpdateCallback(RVMCallbackHandler *handler, uint64_t Addr,
                       const char *Data, size_t Size) {
  auto *AsLogger = (ExecutionLogger *)handler;
  AsLogger->addMemoryUpdate(Addr, reinterpret_cast<const unsigned char *>(Data),
                            Size);
}

void xRegUpdateCallback(RVMCallbackHandler *handler, RVMXReg Reg, RVMRegT) {
  auto *AsLogger = (ExecutionLogger *)handler;
  AsLogger->addXRegUpdate(Reg);
}

void fRegUpdateCallback(RVMCallbackHandler *handler, RVMFReg Reg, RVMRegT) {
  auto *AsLogger = (ExecutionLogger *)handler;
  AsLogger->addFRegUpdate(Reg);
}

void vRegUpdateCallback(RVMCallbackHandler *handler, RVMVReg VReg,
                        const char *Data, size_t Len) {
  auto *AsLogger = (ExecutionLogger *)handler;
  AsLogger->addVRegUpdate(VReg, reinterpret_cast<const unsigned char *>(Data),
                          Len);
}

void csRegUpdateCallback(RVMCallbackHandler *handler, RVMCSR Reg, RVMRegT) {
  auto *AsLogger = (ExecutionLogger *)handler;
  AsLogger->addCSRUpdate(Reg);
}

std::string emitMemoryState(const StateUpdate::MemStateType &MemState) {
  std::stringstream SS;
  std::string Separator;

  const auto &Data = MemState.Data;
  SS << "\"" << rvm_utils::tohs(MemState.Addr) << "\": [";
  for (auto DataIt = Data.begin(), DataEnd = Data.end(); DataIt != DataEnd;
       ++DataIt) {
    SS << Separator << "\"" << rvm_utils::tohs(*DataIt, 2) << "\"";
    Separator = ", ";
  }
  SS << "]";

  return SS.str();
}

std::string
emitChangeVRegister(const StateUpdate::VectorRegUpdateType &VUpdate) {
  std::stringstream SS;
  std::string Separator;
  const auto &Data = VUpdate.Data;
  SS << "\"v" << VUpdate.RegIndex << "\": [";
  std::for_each(Data.begin(), Data.end(), [&](auto &&char_update) {
    SS << Separator << "\"" << rvm_utils::tohs(char_update, 2) << "\"";
    Separator = ", ";
  });
  SS << "]";

  return SS.str();
}

ExecutionLogger::ExecutionLogger(const std::string &ExecLogPath) {
  if (ExecLogPath.empty())
    return;
  OS = std::ofstream(ExecLogPath);
  if (!OS.is_open())
    throw std::runtime_error(std::string("could not open exectution log: ") +
                             ExecLogPath + "\n");

  OS << "{\n";
  OS << "  \"execution\": [\n";
  OS << "    {},\n";
}

void ExecutionLogger::logPreRetireState(const rvm::State &ModelState) {
  if (!OS.is_open())
    return;
  PCBefore = ModelState.readPC();
  RVMErrorCode Err = ModelState.readCSR(RVM_CSR_FCSR, FCSRBefore);
  checkAndThrow(ModelState, Err);
  Update.clear();
}

void ExecutionLogger::logRetiredState(const rvm::State &ModelState) {
  if (!OS.is_open())
    return;
  Update.PC = ModelState.readPC();
  auto Err = ModelState.readCSR(RVM_CSR_FCSR, Update.FCSR);
  checkAndThrow(ModelState, Err);
  const auto &GPR = Update.GPR;
  const auto &FPR = Update.FPR;
  const auto &VReg = Update.VReg;
  const auto &MemWrites = Update.MemWrites;
  const auto &MemReads = Update.MemReads;

  OS << "    { ";
  OS << "\"pc_before\": \"" << rvm_utils::tohs(PCBefore) << "\"";
  OS << ", \"pc_after\": \"" << rvm_utils::tohs(Update.PC) << "\"";
  if (!GPR.empty())
    OS << ", \"gpr\": {"
       << emitChangeXRegisters(GPR.begin(), GPR.end(), ModelState) << "}";
  if (!FPR.empty())
    OS << ", \"fpr\": {"
       << emitChangeFRegisters(FPR.begin(), FPR.end(), ModelState) << "}";
  if (!VReg.empty()) {
    OS << ", \"vreg\": {" << emitChangeVRegisters(VReg.begin(), VReg.end())
       << "}";
  }
  if (!MemWrites.empty())
    OS << ", \"mem\": {"
       << emitMemoryStateList(MemWrites.begin(), MemWrites.end()) << "}";
  if (!MemReads.empty())
    OS << ", \"mem_reads\": {"
       << emitMemoryStateList(MemReads.begin(), MemReads.end()) << "}";
  if (FCSRBefore != Update.FCSR)
    OS << ", \"fcsr\": \"" << rvm_utils::tohs(Update.FCSR) << "\"";
  OS << " },\n";
}

void ExecutionLogger::addMemoryRead(uint64_t Addr, const unsigned char *Data,
                                    size_t Size) {
  Update.MemReads.push_back({Addr, {Data, Data + Size}});
}

void ExecutionLogger::addMemoryUpdate(uint64_t Addr, const unsigned char *Data,
                                      size_t Size) {
  Update.MemWrites.push_back({Addr, {Data, Data + Size}});
}

void ExecutionLogger::addXRegUpdate(RVMXReg Reg) {
  Update.GPR.emplace_back(Reg);
}

void ExecutionLogger::addFRegUpdate(RVMFReg Reg) {
  Update.FPR.emplace_back(Reg);
}

void ExecutionLogger::addVRegUpdate(RVMVReg VReg, const unsigned char *Data,
                                    size_t Size) {
  Update.VReg.push_back({VReg, {Data, Data + Size}});
}

void ExecutionLogger::addCSRUpdate(RVMCSR Reg) { Update.CSR.emplace_back(Reg); }

ExecutionLogger::~ExecutionLogger() {
  if (!OS.is_open())
    return;
  OS << "    {}\n";
  OS << "  ]\n";
  OS << "}";
}

RVMSimExecStatus retireInstr(rvm::State &RVM, ExecutionLogger &Logger) {
  Logger.logPreRetireState(RVM);
  auto result = RVM.executeInstr();
  Logger.logRetiredState(RVM);
  return result;
}

void parseELFAndInit(rvm::State &RVM, std::string_view Path) {
  ELFIO::elfio Reader;
  if (!Reader.load(std::string(Path)))
    throw std::runtime_error(std::string("could not load ELF at \"") +
                             Path.data() + "\"");
  if (Reader.get_machine() != ELFIO::EM_RISCV)
    throw std::runtime_error(std::string("Invalid file \"") + Path.data() +
                             "\": expected RISC-V ELF file");
  auto ElfClass = Reader.get_class();
  if (ElfClass == ELFIO::ELFCLASS64) {
    if (!RVM.getConfig().RV64)
      std::cerr
          << "warning: launching 64-bit executable on a 32-bit simulator. "
             "Hope, you know what you are doing\n";
  } else if (ElfClass == ELFIO::ELFCLASS32) {
    if (RVM.getConfig().RV64)
      std::cerr
          << "warning: launching 32-bit executable on a 64-bit simulator. "
             "Hope, you know what you are doing\n";
  } else {
    throw std::runtime_error("Invalid ELF class");
  }
  // FIXME: loading ELFs with no entry point is unsupported
  auto Err = RVM.setPC(Reader.get_entry());
  checkAndThrow(RVM, Err);
  constexpr static const char *kSymEnd = "_sim_end";
  for (auto &Seg : Reader.segments) {
    if (Seg->get_type() != ELFIO::PT_LOAD)
      continue;
    if (Seg->get_file_size() > Seg->get_memory_size()) {
      // File size must be <= memory size.
      throw std::runtime_error("Invalid ELF segment: file size (" +
                               std::to_string(Seg->get_file_size()) +
                               ") is greater than memory size (" +
                               std::to_string(Seg->get_memory_size()) + ")");
    }
    auto Size = Seg->get_file_size();
    auto Addr = Seg->get_physical_address();
    if (!Size)
      continue;
    Err = RVM.writeMem<char>(Addr, Size, Seg->get_data());
    checkAndThrow(RVM, Err);
  }
  auto Symtab = std::find_if(Reader.sections.begin(), Reader.sections.end(),
                             [](auto &Seg) {
                               return Seg->get_type() == ELFIO::SHT_SYMTAB ||
                                      Seg->get_type() == ELFIO::SHT_DYNSYM;
                             });
  if (Symtab == Reader.sections.end())
    std::cerr << "warning: failed to find symbol table in ELF. Default entry "
                 "addresses will be used.";
  ELFIO::symbol_section_accessor Symbols(Reader, Symtab->get());
  uint64_t StopPC = 1;
  for (auto i = 0u; i < Symbols.get_symbols_num(); ++i) {
    std::string Name;
    ELFIO::Elf64_Addr Value = 0;
    ELFIO::Elf_Xword Size = 0;
    unsigned char Bind = 0, Type = 0;
    ELFIO::Elf_Half SectionIndex = 0;
    unsigned char Other = 0;
    Symbols.get_symbol(i, Name, Value, Size, Bind, Type, SectionIndex, Other);
    if (Name == kSymEnd) {
      StopPC = Value;
      break;
    }
  }
  Err = RVM.setStopPC(StopPC);
  checkAndThrow(RVM, Err);
}
