#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <GL/glew.h>

#include "common.hpp"
#include "image.hpp"

extern const GLfloat vertices[];
extern const GLfloat g_color_buffer_data[];
extern const GLfloat g_uv_buffer_data[];

const Color MISSING_COLOR =
    rgba_color(byte(255), byte(0), byte(255), byte(255));

constexpr int TEXTURE_TILE_HEIGHT = 16;
constexpr int TEXTURE_TILE_WIDTH = 16;
constexpr int TEXTURE_WIDTH = 256;
constexpr float TEXTURE_TILE_WIDTH_F = 16.0f / (float)TEXTURE_WIDTH;
constexpr int TEXTURE_HEIGHT = TEXTURE_WIDTH;
constexpr int TEXTURE_ROWS = TEXTURE_HEIGHT / TEXTURE_TILE_HEIGHT;

#endif
