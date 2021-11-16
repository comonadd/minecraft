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
