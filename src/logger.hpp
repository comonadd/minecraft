#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "common.hpp"
#include "util.hpp"

namespace logger {

inline void error(string const& msg) {
  //
  fmt::print("[{}][Error]: {}\n", CurrentTimeStr(), msg);
};

inline void info(string const& msg) {
  //
  fmt::print("[{}][Info]: {}\n", CurrentTimeStr(), msg);
};

inline void debug(string const& msg) {
  //
  fmt::print("[{}][Debug]: {}\n", CurrentTimeStr(), msg);
};

}  // namespace logger

#endif
