#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "common.hpp"
#include "image.hpp"
#include "logger.hpp"

struct Texture {
  GLuint texture;
};

namespace texture_storage {

struct {
  unordered_map<string, shared_ptr<Texture>> loaded{};
} state;

optional<shared_ptr<Texture>> get_texture(std::string const &name);

optional<shared_ptr<Texture>> load_texture(std::string const &name,
                                           std::string const &path);

}  // namespace texture_storage

#endif
