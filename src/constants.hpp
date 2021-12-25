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

#endif
