#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include <random>

#include "common.hpp"
#include "constants.hpp"
#include "noise.hpp"
#include "util.hpp"
#include "world.hpp"

struct SkyVertexData {
  glm::vec3 pos;
  glm::vec3 col;
  glm::vec2 uv;
};

constexpr u32 MAX_CLOUD_WIDTH = 12;
constexpr u32 MIN_CLOUD_WIDTH = 3;
constexpr u32 MIN_CLOUD_LENGTH = 4;
constexpr u32 MAX_CLOUD_LENGTH = 16;
constexpr u32 DISTANCE_BETWEEN_CLOUDS = 64;

inline vector<SkyVertexData> make_clouds_mesh(WorldPos center) {
  static std::random_device rd;
  static std::default_random_engine eng;
  static std::uniform_real_distribution<float> _tree_gen{FLOAT_MIN, FLOAT_MAX};
  static OpenSimplexNoiseWParam noise(0.03f, 1.0f, 1.0f, 1.0f, 234234);
  auto gen = [&](float x, float y) -> float { return noise.noise(1, x, y); };
  vector<SkyVertexData> res;
  i32 CLOUD_HEIGHT = 128.0f;
  i32 CLOUD_VIEW_RADIUS = 128;
  static const float positions[6][4][3] = {
      {{-1, -1, -1}, {-1, -1, +1}, {-1, +1, -1}, {-1, +1, +1}},
      {{+1, -1, -1}, {+1, -1, +1}, {+1, +1, -1}, {+1, +1, +1}},
      {{-1, +1, -1}, {-1, +1, +1}, {+1, +1, -1}, {+1, +1, +1}},
      {{-1, -1, -1}, {-1, -1, +1}, {+1, -1, -1}, {+1, -1, +1}},
      {{-1, -1, -1}, {-1, +1, -1}, {+1, -1, -1}, {+1, +1, -1}},
      {{-1, -1, +1}, {-1, +1, +1}, {+1, -1, +1}, {+1, +1, +1}}};
  static const float uvs[6][4][2] = {
      {{0, 0}, {1, 0}, {0, 1}, {1, 1}}, {{1, 0}, {0, 0}, {1, 1}, {0, 1}},
      {{0, 1}, {0, 0}, {1, 1}, {1, 0}}, {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
      {{0, 0}, {0, 1}, {1, 0}, {1, 1}}, {{1, 0}, {1, 1}, {0, 0}, {0, 1}}};
  static const float indices[6][6] = {{0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3},
                                      {0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3},
                                      {0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3}};
  auto startx = center.x - CLOUD_VIEW_RADIUS;
  auto endx = center.x + CLOUD_VIEW_RADIUS;
  auto starty = center.z - CLOUD_VIEW_RADIUS;
  auto endy = center.z + CLOUD_VIEW_RADIUS;
  for (i32 x = startx; x < endx; x += 1) {
    for (i32 y = starty; y < endy; y += 1) {
      auto r = gen(x, y);
      auto need_block_here = r > 0.6f;
      if (!need_block_here) continue;
      for (int i = 0; i < 6; ++i) {
        for (int v = 0; v < 6; ++v) {
          SkyVertexData p;
          int j = indices[i][v];
          p.pos.x = (float)x + positions[i][j][0];
          p.pos.y = (float)CLOUD_HEIGHT + positions[i][j][1];
          p.pos.z = (float)y + positions[i][j][2];
          p.col = glm::vec3(1.0f, 1.0f, 1.0f);
          p.uv.x = uvs[i][j][0];
          p.uv.y = uvs[i][j][1];
          res.push_back(p);
        }
      }
    }
  }
  return res;
}

inline vector<SkyVertexData> make_skybox_mesh() {
  static const float positions[6][4][3] = {
      {{-1, -1, -1}, {-1, -1, +1}, {-1, +1, -1}, {-1, +1, +1}},
      {{+1, -1, -1}, {+1, -1, +1}, {+1, +1, -1}, {+1, +1, +1}},
      {{-1, +1, -1}, {-1, +1, +1}, {+1, +1, -1}, {+1, +1, +1}},
      {{-1, -1, -1}, {-1, -1, +1}, {+1, -1, -1}, {+1, -1, +1}},
      {{-1, -1, -1}, {-1, +1, -1}, {+1, -1, -1}, {+1, +1, -1}},
      {{-1, -1, +1}, {-1, +1, +1}, {+1, -1, +1}, {+1, +1, +1}}};
  static const float uvs[6][4][2] = {
      {{0, 0}, {1, 0}, {0, 1}, {1, 1}}, {{1, 0}, {0, 0}, {1, 1}, {0, 1}},
      {{0, 1}, {0, 0}, {1, 1}, {1, 0}}, {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
      {{0, 0}, {0, 1}, {1, 0}, {1, 1}}, {{1, 0}, {1, 1}, {0, 0}, {0, 1}}};
  static const float indices[6][6] = {{0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3},
                                      {0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3},
                                      {0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3}};
  vector<SkyVertexData> res;

  // sky box
  for (int i = 0; i < 6; ++i) {
    for (int v = 0; v < 6; ++v) {
      SkyVertexData p;
      int j = indices[i][v];
      p.pos.x = positions[i][j][0];
      p.pos.y = positions[i][j][1];
      p.pos.z = positions[i][j][2];
      p.col = glm::vec3(0.0f, 0.629f, 1.0f);
      p.uv.x = uvs[i][j][0];
      p.uv.y = uvs[i][j][1];
      res.push_back(p);
    }
  }

  return res;
}

inline vector<SkyVertexData> make_celestial_body_mesh() {
  static const float positions[6][4][3] = {
      {{-1, -1, -1}, {-1, -1, +1}, {-1, +1, -1}, {-1, +1, +1}},
      {{+1, -1, -1}, {+1, -1, +1}, {+1, +1, -1}, {+1, +1, +1}},
      {{-1, +1, -1}, {-1, +1, +1}, {+1, +1, -1}, {+1, +1, +1}},
      {{-1, -1, -1}, {-1, -1, +1}, {+1, -1, -1}, {+1, -1, +1}},
      {{-1, -1, -1}, {-1, +1, -1}, {+1, -1, -1}, {+1, +1, -1}},
      {{-1, -1, +1}, {-1, +1, +1}, {+1, -1, +1}, {+1, +1, +1}}};
  static const float uvs[6][4][2] = {
      {{0, 0}, {1, 0}, {0, 1}, {1, 1}}, {{1, 0}, {0, 0}, {1, 1}, {0, 1}},
      {{0, 1}, {0, 0}, {1, 1}, {1, 0}}, {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
      {{0, 0}, {0, 1}, {1, 0}, {1, 1}}, {{1, 0}, {1, 1}, {0, 0}, {0, 1}}};
  static const float indices[6][6] = {{0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3},
                                      {0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3},
                                      {0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3}};
  vector<SkyVertexData> res;
  for (int v = 0; v < 6; ++v) {
    int i = 0;
    SkyVertexData p;
    int j = indices[i][v];
    p.pos.x = positions[i][j][0];
    p.pos.y = positions[i][j][1];
    p.pos.z = positions[i][j][2];
    // p.col = glm::vec3(1.0f, 0.670f, 0.0f);
    p.col = no_color();
    p.uv.x = uvs[i][j][0];
    p.uv.y = uvs[i][j][1];
    res.push_back(p);
  }
  return res;
}

#endif
