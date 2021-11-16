#include "shaders.hpp"

#include <iostream>

#include "util.hpp"

GLuint load_shader(char const *vs_path, char const *fs_path) {
  GLint res;
  auto vs_source = read_whole_file(vs_path);
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  auto *vss = (vs_source.data());
  glShaderSource(vertexShader, 1, &vss, NULL);
  glCompileShader(vertexShader);
  GLint status;
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    char buffer[512];
    glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
    std::cout << "Error during vertex shader compilation:\n" << buffer;
    exit(-1);
  }
  auto fs_source = read_whole_file(fs_path);
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  auto *fss = fs_source.data();
  glShaderSource(fragmentShader, 1, &fss, NULL);
  glCompileShader(fragmentShader);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    char buffer[512];
    glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
    std::cout << "Error during fragment shader compilation:\n" << buffer;
    exit(-1);
  }
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  return shaderProgram;
}
