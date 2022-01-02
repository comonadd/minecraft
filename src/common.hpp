#ifndef COMMON_HPP
#define COMMON_HPP

#include <fmt/core.h>
#include <fmt/format.h>
#include <stdint.h>

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <filesystem>

using std::byte;
using std::pair;
using std::set;
using std::string;
using std::unordered_map;
using std::vector;

namespace fs = std::filesystem;

using std::make_pair;
using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::unique_ptr;

using std::optional;

using std::function;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using glm::ivec3;
using glm::ivec4;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

template <typename T>
struct fmt::formatter<glm::tvec4<T>> {
  using K = glm::tvec4<T>;
  char presentation = 'f';

  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }

  template <typename FormatContext>
  auto format(const K& r, FormatContext& ctx) -> decltype(ctx.out()) {
    return format_to(ctx.out(), "x={}, y={}, z={}, w={}", (i32)r.r, (i32)r.g,
                     (i32)r.b, (i32)r.a);
  }
};

template <typename T>
struct fmt::formatter<glm::tvec3<T>> {
  using K = glm::tvec3<T>;
  char presentation = 'f';

  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }

  template <typename FormatContext>
  auto format(const K& r, FormatContext& ctx) -> decltype(ctx.out()) {
    return format_to(ctx.out(), "x={}, y={}, z={}", (i32)r.x, (i32)r.y,
                     (i32)r.z);
  }
};

template <typename T>
struct fmt::formatter<glm::tvec2<T>> {
  using K = glm::tvec2<T>;
  char presentation = 'f';

  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }

  template <typename FormatContext>
  auto format(const K& r, FormatContext& ctx) -> decltype(ctx.out()) {
    return format_to(ctx.out(), "x={}, y={}", (i32)r.r, (i32)r.g);
  }
};

#endif
