#ifndef COMMON_HPP
#define COMMON_HPP

#include <fmt/core.h>
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

using std::byte;
using std::pair;
using std::set;
using std::string;
using std::unordered_map;
using std::vector;

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
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

#endif
