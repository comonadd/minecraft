#ifndef SHADERS_HPP
#define SHADERS_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>

struct Attrib {
  GLint shader;
  GLint position;
  GLint normal;
  GLint uv;
  GLint ao;
  GLint light;
  GLint MVP;
};

GLuint load_shader(char const *vs_path, char const *fs_path);

#endif
