#ifndef COLOR_HPP
#define COLOR_HPP

#include <fmt/core.h>
#include <fmt/format.h>

#include "common.hpp"

using Color = glm::tvec4<byte>;

inline Color rgba_color(byte r, byte g, byte b, byte a) {
  return Color{r, g, b, a};
}

template <>
struct fmt::formatter<Color> {
  char presentation = 'f';

  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }

  template <typename FormatContext>
  auto format(const Color& r, FormatContext& ctx) -> decltype(ctx.out()) {
    return format_to(ctx.out(), "Color r={}, g={}, b={}, a={}", (u32)r.r,
                     (u32)r.g, (u32)r.b, (u32)r.a);
  }
};

#endif
