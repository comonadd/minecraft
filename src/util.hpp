#ifndef UTIL_HPP
#define UTIL_HPP

#include "common.hpp"

std::string read_whole_file(char const* fp);

inline int round_to_nearest_16(int number) {
  int result = abs(number) + 16 / 2;
  result -= result % 16;
  result *= number > 0 ? 1 : -1;
  return result;
}

// 2D vector wrapper around 1D vector
template <typename T>
class Vector2D : public std::vector<T> {
 protected:
  unsigned _width;
  unsigned _height;

 public:
  unsigned width() { return _width; }
  unsigned height() { return _height; }
  void set_width(unsigned i) { _width = i; }

  T& operator()(int x, int y) { return this->operator[](x + _width * y); }

  T& set(int x, int y, T& val) { this->operator()(x, y) = val; }

  Vector2D(unsigned new_width, unsigned new_height)
      : std::vector<T>(), _width(new_width), _height(new_height) {
    this->resize(new_width * new_height);
  }
};

template <typename T>
vector<T> make_rect_mesh();

inline float map(float minRange, float maxRange, float minDomain,
                 float maxDomain, float value) {
  return minDomain +
         (maxDomain - minDomain) * (value - minRange) / (maxRange - minRange);
}

// pink "missing texture" color
inline glm::vec3 no_color() {
  //
  return {1.0f, 0.0f, 1.0f};
}

std::string FormatTime(std::chrono::system_clock::time_point tp);

inline std::string CurrentTimeStr() {
  return FormatTime(std::chrono::system_clock::now());
}

#endif
