#include "texture.hpp"

struct {
  unordered_map<string, shared_ptr<Texture>> loaded{};
} state;

void texture_storage::init() {
  stbi_set_flip_vertically_on_load(true);
}

optional<shared_ptr<Texture>> texture_storage::get_texture(
    std::string const &name) {
  auto it = state.loaded.find(name);
  if (it == state.loaded.end()) {
    logger::error(fmt::format("Failed to load texture \"{}\"\n", name));
    return {};
  }
  return state.loaded.at(name);
}

optional<shared_ptr<Texture>> texture_storage::load_texture(
    std::string const &name, std::string const &path) {
  if (auto image = image_storage::load_image(name, path)) {
    auto img = image->get();
    unsigned int texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, img->data);
    glGenerateMipmap(GL_TEXTURE_2D);
    Texture tex;
    tex.texture = texture;
    auto res = make_shared<Texture>(tex);
    state.loaded[name] = res;
    return res;
  } else {
    logger::error(
        fmt::format("Failed to load texture \"{}\" at \"{}\"", name, path));
    return {};
  }
}
