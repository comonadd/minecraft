#ifndef IMAGE_HPP
#define IMAGE_HPP

#include "color.hpp"
#include "common.hpp"
#include "logger.hpp"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "util.hpp"

using Pixel = Color;

struct Image {
  u32 width = 0;
  u32 height = 0;
  Pixel* data = nullptr;
  Pixel& operator()(u32 x, u32 y) {
    return *(this->data + x + this->width * y);
  }
  Pixel& set(u32 x, u32 y, Pixel& val) { this->operator()(x, y) = val; }

  Image(u32 width, u32 height, Pixel* _data)
      : width(width), height(height), data(_data) {}
  Image(u32 width, u32 height) : width(width), height(height) {
    this->data = new Pixel[width * height];
  }
};

#define RGBA(__r, __g, __b, __a) (__r | (__g << 8) | (__b << 16) | (__a << 24));
#define IMG_GET_PIXEL_AT(__texture, __h, __x, __y) \
  (*(__texture + (__y * __h + __x)))
#define RGBA_R(__pix) (__pix >> 0) & 0xFF
#define RGBA_G(__pix) (__pix >> 8) & 0xFF
#define RGBA_B(__pix) (__pix >> 16) & 0xFF
#define RGBA_A(__pix) (__pix >> 24) & 0xFF
#define RGBA_TO_IVEC(__pix) \
  glm::ivec4(RGBA_R(__pix), RGBA_G(__pix), RGBA_B(__pix), RGBA_A(__pix))

namespace image_storage {

struct {
  unordered_map<string, shared_ptr<Image>> loaded{};
} state;

shared_ptr<Image> get_image(string const& name);

optional<shared_ptr<Image>> load_image(string const& name, string const& path);

}  // namespace image_storage

#endif
