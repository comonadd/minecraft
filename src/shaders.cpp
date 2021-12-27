#include "shaders.hpp"

#include <iostream>

#include "logger.hpp"
#include "util.hpp"

struct {
  unordered_map<string, shared_ptr<Shader>> loaded{};
} state;

optional<GLuint> _load_shader(char const* vs_path, char const* fs_path) {
  auto vs_source = read_whole_file(vs_path);
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  auto* vss = (vs_source.data());
  glShaderSource(vertexShader, 1, &vss, NULL);
  glCompileShader(vertexShader);
  GLint status;
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    char buffer[512];
    glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
    std::cout << "Error during vertex shader compilation:\n" << buffer;
    return {};
  }
  auto fs_source = read_whole_file(fs_path);
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  auto* fss = fs_source.data();
  glShaderSource(fragmentShader, 1, &fss, NULL);
  glCompileShader(fragmentShader);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    char buffer[512];
    glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
    std::cout << "Error during fragment shader compilation:\n" << buffer;
    return {};
  }
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  return shaderProgram;
}

shared_ptr<Shader> shader_storage::get_shader(string const& name) {
  auto res = state.loaded.find(name);
  if (res == state.loaded.end()) {
    logger::error(fmt::format("Failed to load shader \"{}\"\n", name));
    return {};
  }
  return res->second;
}

optional<shared_ptr<Shader>> shader_storage::load_shader(
    string const& name, string const& vs_path, string const& fs_path,
    function<void(Shader& shader)> init_attrs) {
  if (auto id = _load_shader(vs_path.c_str(), fs_path.c_str())) {
    Shader shader;
    shader.id = *id;
    init_attrs(shader);
    auto res = make_shared<Shader>(shader);
    state.loaded.insert({name, res});
    return res;
  } else {
    logger::error(fmt::format("Failed to load shader \"{}\" at {}, {}", name,
                              vs_path, fs_path));
    return {};
  }
}
