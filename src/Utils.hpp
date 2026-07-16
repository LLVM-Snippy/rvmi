// Common file to store utils that are used in several places
#pragma once

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace rvm_utils {

inline std::ostream &dbgs() { return std::cerr; }
inline std::ostream &outs() { return std::cerr; }

[[noreturn]] inline void report_fatal_error(std::string Message) {
  std::cerr << Message << "\n";
  exit(EXIT_FAILURE);
}

inline std::string to_lower(std::string_view str) {
  std::string res;
  std::transform(str.begin(), str.end(), std::back_inserter(res),
                 [](char ch) { return std::tolower(ch); });
  return res;
}

#include "SimulatorOptions.hpp"

#ifdef RVM_OPTION_DESC
#error "Defined internal-only define RVM_OPTION_DESC"
#endif

enum class SimOptions : int {
#define RVM_OPTION_DESC(EN, STR, IS_FLAG) EN,
  D_RVM_OPTIONS
#undef RVM_OPTION_DESC
};

template <SimOptions n> std::string simOpt2Str() {
  assert(!"unknown option specified");
  throw std::runtime_error("unknown option specified");
};

#define RVM_OPTION_DESC(EN, STR, IS_FLAG)                                      \
  template <> inline std::string simOpt2Str<SimOptions::EN>() { return STR; }
D_RVM_OPTIONS
#undef RVM_OPTION_DESC

template <typename RetTy, SimOptions Option> struct OptionGetter;

template <SimOptions Option> struct OptionGetter<bool, Option> {
  inline static bool get() { return std::getenv(simOpt2Str<Option>().c_str()); }
};

template <SimOptions Option> struct OptionGetter<std::string, Option> {
  inline static std::string get() {
    const char *Value = std::getenv(simOpt2Str<Option>().c_str());
    if (!Value)
      return {};
    return Value;
  }
};

} // namespace rvm_utils
