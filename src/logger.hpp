#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "common.hpp"

namespace logger {

inline void error(string const& msg) {
  //
  fmt::print("[Error]: {}", msg);
};

inline void info(string const& msg) {
  //
  fmt::print("[Info]: {}", msg);
};

inline void debug(string const& msg) {
  //
  fmt::print("[Debug]: {}", msg);
};

}  // namespace logger

#endif
