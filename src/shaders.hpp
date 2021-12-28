#ifndef SHADERS_HPP
#define SHADERS_HPP

#include "common.hpp"

struct Attrib {
  GLint position;
  GLint normal;
  GLint uv;
  GLint ao;
  GLint light;

  GLint color;

  GLint MVP;
  GLint light_pos;

  GLint blend_factor;
};

struct Shader {
  GLuint id;
  Attrib attr;
};

namespace shader_storage {

shared_ptr<Shader> get_shader(string const& name);

optional<shared_ptr<Shader>> load_shader(
    string const& name, string const& vs_path, string const& fs_path,
    function<void(Shader& shader)> init_attrs);

}  // namespace shader_storage

#endif
