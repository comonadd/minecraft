#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include "common.hpp"
#include "util.hpp"

struct SkyVertexData {
  glm::vec3 pos;
  glm::vec3 col;
  glm::vec2 uv;
};

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
