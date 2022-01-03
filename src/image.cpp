#include "image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

struct {
  unordered_map<string, shared_ptr<Image>> loaded{};
} state;

shared_ptr<Image> image_storage::get_image(string const& name) {
  return state.loaded.at(name);
}

optional<shared_ptr<Image>> image_storage::load_image(string const& name,
                                                      string const& path) {
  int width, height, nrChannels;
  auto* data = (Color*)stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
  if (!data) {
    logger::error(
        fmt::format("Failed to load image \"{}\" at \"{}\"", name, path));
    return {};
  }
  auto image = make_shared<Image>(Image(width, height, data));
  state.loaded.insert({name, image});
  return image;
}
