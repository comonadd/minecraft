#include "util.hpp"

#include <fstream>
#include <iostream>

std::string read_whole_file(char const *fp) {
  std::ifstream vs_f(fp);
  if (!vs_f.is_open()) {
    std::cout << "Failed to open the file at " << fp << '\n';
    exit(-1);
    return std::string();
  }
  std::string str((std::istreambuf_iterator<char>(vs_f)),
                  std::istreambuf_iterator<char>());
  return std::move(str);
}

std::string FormatTime(std::chrono::system_clock::time_point tp) {
  std::stringstream ss;
  auto t = std::chrono::system_clock::to_time_t(tp);
  auto tp2 = std::chrono::system_clock::from_time_t(t);
  if (tp2 > tp)
    t = std::chrono::system_clock::to_time_t(tp - std::chrono::seconds(1));
  ss << std::put_time(std::localtime(&t), "%Y-%m-%d %T") << "."
     << std::setfill('0') << std::setw(3)
     << (std::chrono::duration_cast<std::chrono::milliseconds>(
             tp.time_since_epoch())
             .count() %
         1000);
  return ss.str();
}
