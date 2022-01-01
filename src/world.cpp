#include "world.hpp"

#include <GL/glew.h>
#include <fmt/core.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "PerlinNoise/PerlinNoise.hpp"
#include "constants.hpp"
#include "image.hpp"
#include "util.hpp"

using std::array;
using std::byte;
using std::max;
using std::min;

float BLOCK_WIDTH = 2.0f;
float BLOCK_LENGTH = BLOCK_WIDTH;
float BLOCK_HEIGHT = BLOCK_WIDTH;

inline Chunk *is_chunk_loaded(World &world, int x, int y) {
  auto ch = world.loaded_chunks.find(chunk_id_from_coords(x, y));
  bool loaded = ch != world.loaded_chunks.end();
  // fmt::print("{},{} is loaded: {}\n", x, y, loaded);
  return loaded ? ch->second : nullptr;
}

inline BlockType block_type_for_height(Biome &bk, int height, int maxHeight) {
  int top = bk.maxHeight - BLOCKS_OF_AIR_ABOVE;

  switch (bk.kind) {
    case BiomeKind::Desert: {
      if (height == maxHeight) {
        return BlockType::Sand;
      }
      if (height > (top - 6)) {
        return BlockType::Sand;
      } else if (height > (top - 20)) {
        return BlockType::Dirt;
      } else if (height >= 0) {
        return BlockType::Stone;
      }
    } break;

    case BiomeKind::Grassland: {
      if (height == maxHeight) {
        return BlockType::TopGrass;
      } else if (height > (top - 20)) {
        return BlockType::Dirt;
      } else if (height >= 0) {
        return BlockType::Stone;
      }
    } break;

    case BiomeKind::Tundra: {
      if (height == maxHeight) {
        return BlockType::Snow;
      } else if (height > (top - 20)) {
        return BlockType::Snow;
      } else if (height >= 0) {
        return BlockType::Stone;
      }
    } break;

    case BiomeKind::Taiga: {
      if (height == maxHeight) {
        return BlockType::TopSnow;
      } else if (height > (top - 20)) {
        return BlockType::Dirt;
      } else if (height >= 0) {
        return BlockType::Stone;
      }
    } break;

    case BiomeKind::Mountains: {
      if (height > (top - 6)) {
        return BlockType::Snow;
      } else if (height > (top - 20)) {
        return BlockType::Stone;
      } else if (height >= 0) {
        return BlockType::Stone;
      }
    } break;

    case BiomeKind::Ocean: {
      return BlockType::Sand;
    } break;

    case BiomeKind::Coast: {
      return BlockType::Sand;
    } break;

    case BiomeKind::Forest: {
      if (height == maxHeight) {
        return BlockType::TopGrass;
      } else if (height > (top - 20)) {
        return BlockType::Dirt;
      } else if (height >= 0) {
        return BlockType::Stone;
      }
    } break;

    case BiomeKind::Jungle: {
      if (height == maxHeight) {
        return BlockType::JungleTopGrass;
      } else if (height > (top - 20)) {
        return BlockType::Dirt;
      } else if (height >= 0) {
        return BlockType::Stone;
      }
    } break;

    default:
      return BlockType::Unknown;
  }

  return BlockType::Unknown;
}

uint32_t get_col_height(Block *col) {
  int topBlockHeight = CHUNK_HEIGHT - 1;
  while (topBlockHeight > 0 && col[topBlockHeight].type == BlockType::Air) {
    topBlockHeight--;
  }
  return topBlockHeight;
}

inline glm::vec2 uv_for_block_type(BlockType bt) {
  // TODO: Implement this
  return {0.0, 0.0};
}

inline WorldPos chunk_global_to_local_pos(Chunk *chunk, WorldPos pos) {
  return glm::ivec3(pos.x - chunk->x, pos.y - chunk->y, pos.z);
}

inline Block chunk_get_block(Chunk *chunk, glm::ivec3 local_pos) {
  return chunk->blocks[local_pos.x][local_pos.y][local_pos.z];
}

inline Block chunk_get_block_at_global(Chunk *chunk, WorldPos pos) {
  auto local_pos = chunk_global_to_local_pos(chunk, pos);
  return chunk_get_block(chunk, local_pos);
}

static set<BlockType> complex_bt_textures = {
    //
    BlockType::TopGrass,        //
    BlockType::Wood,            //
    BlockType::PineWood,        //
    BlockType::TopSnow,         //
    BlockType::JungleWood,      //
    BlockType::JungleTopGrass,  //
                                //
};

bool bt_is_complex(BlockType bt) {
  return complex_bt_textures.find(bt) != complex_bt_textures.end();
}

glm::vec2 block_type_texture_offset(BlockType bt) {
  size_t block_id = (size_t)bt;
  size_t pixel_offset = block_id * TEXTURE_TILE_WIDTH;
  size_t real_x = pixel_offset % TEXTURE_WIDTH;
  size_t real_y_row = floor(pixel_offset / TEXTURE_WIDTH);
  auto x = ((float)real_x / (float)TEXTURE_WIDTH);
  auto y = 1.0f - (((float)real_y_row + 1) / (float)TEXTURE_ROWS);
  glm::vec2 res{x, y};
  return res;
}

glm::vec2 block_type_texture_offset_int(BlockType bt) {
  auto offsetf = block_type_texture_offset(bt);
  auto w = TEXTURE_WIDTH;
  auto h = TEXTURE_HEIGHT;
  auto offset = glm::ivec2(offsetf.x * (float)w, offsetf.y * (float)h);
  return offset;
}

glm::vec2 block_type_texture_offset_for_face(int face) {
  int debugFaceTexturesOffset = 80;
  return block_type_texture_offset((BlockType)(face + debugFaceTexturesOffset));
}

void make_cube_faces(ChunkMesh &mesh, float ao[6][4], float light[6][4],
                     int left, int right, int top, int bottom, int front,
                     int back, int wleft, int wright, int wtop, int wbottom,
                     int wfront, int wback, float x, float y, float z, float n,
                     BlockType block_type) {
  // has separate textures for top/side/bottom
  bool is_complex = bt_is_complex(block_type);
  glm::vec2 texture_offset = block_type_texture_offset(block_type);
  static const float positions[6][4][3] = {
      {{-1, -1, -1}, {-1, -1, +1}, {-1, +1, -1}, {-1, +1, +1}},
      {{+1, -1, -1}, {+1, -1, +1}, {+1, +1, -1}, {+1, +1, +1}},
      {{-1, +1, -1}, {-1, +1, +1}, {+1, +1, -1}, {+1, +1, +1}},
      {{-1, -1, -1}, {-1, -1, +1}, {+1, -1, -1}, {+1, -1, +1}},
      {{-1, -1, -1}, {-1, +1, -1}, {+1, -1, -1}, {+1, +1, -1}},
      {{-1, -1, +1}, {-1, +1, +1}, {+1, -1, +1}, {+1, +1, +1}}};
  static const float normals[6][3] = {{-1, 0, 0}, {+1, 0, 0}, {0, +1, 0},
                                      {0, -1, 0}, {0, 0, -1}, {0, 0, +1}};
  static const float uvs[6][4][2] = {
      {{0, 0}, {1, 0}, {0, 1}, {1, 1}},                                    //
      {{1, 0}, {0, 0}, {1, 1}, {0, 1}}, {{0, 1}, {0, 0}, {1, 1}, {1, 0}},  //
      {{0, 0}, {0, 1}, {1, 0}, {1, 1}}, {{0, 0}, {0, 1}, {1, 0}, {1, 1}},  //
      {{1, 0}, {1, 1}, {0, 0}, {0, 1}}};
  static const float indices[6][6] = {{0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3},
                                      {0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3},
                                      {0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3}};
  float s = TEXTURE_TILE_WIDTH_F;
  float a = 0 + 1 / 2048.0;
  float b = s - 1 / 2048.0;
  int faces[6] = {left, right, top, bottom, front, back};
  static float complex_uv_offset[6][2] = {
      {0, -(float)TEXTURE_TILE_HEIGHT / TEXTURE_HEIGHT},        // left
      {0, -(float)TEXTURE_TILE_HEIGHT / TEXTURE_HEIGHT},        // right
      {0, 0},                                                   // top
      {0, -((float)TEXTURE_TILE_HEIGHT * 2) / TEXTURE_HEIGHT},  // bottom
      {0, -(float)TEXTURE_TILE_HEIGHT / TEXTURE_HEIGHT},        // front
      {0, -(float)TEXTURE_TILE_HEIGHT / TEXTURE_HEIGHT}         // back
  };
  static float no_uv_offset[6][2] = {
      {0, 0},  // left
      {0, 0},  // right
      {0, 0},  // top
      {0, 0},  // bottom
      {0, 0},  // front
      {0, 0}   // back
  };
  auto *extra_uv_offset = no_uv_offset;
  if (is_complex) {
    extra_uv_offset = complex_uv_offset;
  } else {
    extra_uv_offset = no_uv_offset;
  }
  for (int i = 0; i < 6; i++) {
    if (faces[i] == 0) {
      continue;
    }
    for (int v = 0; v < 6; v++) {
      VertexData vd;
      int j = indices[i][v];
      auto xpos = positions[i][j][0];
      vd.pos.x = x + n * xpos;
      auto ypos = positions[i][j][1];
      vd.pos.y = y + n * ypos;
      auto zpos = positions[i][j][2];
      vd.pos.z = z + n * zpos;
      vd.normal.x = normals[i][0];
      vd.normal.y = normals[i][1];
      vd.normal.z = normals[i][2];
      auto xoffset = texture_offset.x + extra_uv_offset[i][0];
      auto yoffset = texture_offset.y + extra_uv_offset[i][1];
      vd.uv = {
          xoffset + (uvs[i][j][0] ? b : a),  // x
          yoffset + (uvs[i][j][1] ? b : a)
          // y
      };
      vd.ao = ao[i][j];
      vd.light = light[i][j];
      // vd.uv.r = dv + (uvs[i][j][1] ? b : a);
      mesh.push_back(vd);
    }
  }
}

double NOISE_PICTURE_WIDTH = CHUNK_WIDTH * 2;

#define CHUNK_AT(__chunk, __x, __y, __z) (__chunk).blocks[(__x)][(__y)][(__z)]
#define CHUNK_COL_AT(__chunk, __x, __y) CHUNK_AT(__chunk, __x, __y, 0)
#define IS_TB(__block) (__block).type == BlockType::Air

#define IS_TB_AT(__chunk, __x, __y, __z) IS_TB(CHUNK_AT(__chunk, __x, __y, __z))

auto W = CHUNK_WIDTH;
auto H = CHUNK_HEIGHT;
auto L = CHUNK_LENGTH;

inline bool IS_TB_BACK_OF(Chunk &chunk, int x, int y, int z) {
  const auto neary = min(L - 1, y + 1);
  if (neary == L - 1) return true;
  return IS_TB_AT(chunk, x, neary, z);
}

inline bool IS_TB_FRONT_OF(Chunk &chunk, int x, int y, int z) {
  const auto neary = max(0, y - 1);
  if (neary == 0) return true;
  return IS_TB_AT(chunk, x, neary, z);
}

inline bool IS_TB_RIGHT_OF(Chunk &chunk, int x, int y, int z) {
  const auto nearx = min(L - 1, x + 1);
  if (nearx == L - 1) return true;
  return IS_TB_AT(chunk, nearx, y, z);
}

inline bool IS_TB_LEFT_OF(Chunk &chunk, int x, int y, int z) {
  const auto nearx = max(0, x - 1);
  if (nearx == 0) return true;
  return IS_TB_AT(chunk, nearx, y, z);
}

inline bool IS_TB_TOP_OF(Chunk &chunk, int x, int y, int z) {
  const auto nearz = min(H - 1, z + 1);
  if (nearz == H - 1) return true;
  return IS_TB_AT(chunk, x, y, nearz);
}

inline bool IS_TB_BOTTOM_OF(Chunk &chunk, int x, int y, int z) {
  const auto nearz = max(0, z - 1);
  if (nearz == 0) return true;
  return IS_TB_AT(chunk, x, y, nearz);
}

struct PointBiomeNoise {
  float height_noise;
  float rainfall_noise;
  float temp_noise;
};

inline BiomeKind biome_noise_to_kind_at_point(PointBiomeNoise bn) {
  auto temp_noise = bn.temp_noise;
  auto rainfall_noise = bn.rainfall_noise;
  auto height_noise = bn.height_noise;

  if (height_noise > 0.75) {
    return BiomeKind::Mountains;
  }

  if (height_noise < 0.4) {
    return BiomeKind::Ocean;
  }

  // coast near the ocean
  if (height_noise < 0.45) {
    return BiomeKind::Coast;
  }

  if (temp_noise > 0.6) {
    // high temperature
    if (rainfall_noise > 0.5) {
      // high temp, high rainfall
      // needs to be jungle
      return BiomeKind::Jungle;
    } else {
      // high temp, low rainfall
      return BiomeKind::Desert;
    }
  } else if (temp_noise > 0.4) {
    // moderate temperature
    if (rainfall_noise > 0.6) {
      // moderate temp, high rainfall
      return BiomeKind::Forest;
    } else if (rainfall_noise > 0.3) {
      // moderate temp, moderate rainfall
      return BiomeKind::Grassland;
    } else {
      // moderate temp, low rainfall
      // needs to be savannah
      return BiomeKind::Grassland;
    }
  } else {
    if (rainfall_noise > 0.6) {
      return BiomeKind::Taiga;
    } else {
      return BiomeKind::Tundra;
    }
  }
}

inline float temperature_noise_at(World &world, int x, int y) {
  return (world.temperature_noise.noise(4, x, y) + 1.0f) / 2.0f;
}

inline float rainfall_noise_at(World &world, int x, int y) {
  return (world.rainfall_noise.noise(2, x, y) + 1.0f) / 2.0f;
}

inline float simple_height_noise_at(World &world, i32 x, i32 y) {
  return (world.height_noise.noise(2, x, y) + 1.0f) / 2.0f;
}

inline Biome &biome_at_point(World &world, i32 x, i32 y) {
  auto height_noise = simple_height_noise_at(world, x, y);
  auto temp_noise = temperature_noise_at(world, x, y);
  auto rainfall_noise = rainfall_noise_at(world, x, y);
  PointBiomeNoise bn{
      .height_noise = height_noise,
      .rainfall_noise = rainfall_noise,
      .temp_noise = temp_noise,
  };
  auto kind = biome_noise_to_kind_at_point(bn);
  auto it = world.biomes_by_kind.find(kind);
  if (it == world.biomes_by_kind.end()) {
    logger::error(
        fmt::format("Couldn't find biome info for biome with id={}\n", kind));
    return it->second;
  }
  return it->second;
}

inline float blerp(float q11, float q12, float q21, float q22, float x1,
                   float x2, float y1, float y2, float x, float y) {
  float x2x1, y2y1, x2x, y2y, yy1, xx1;
  x2x1 = x2 - x1;
  y2y1 = y2 - y1;
  x2x = x2 - x;
  y2y = y2 - y;
  yy1 = y - y1;
  xx1 = x - x1;
  return 1.0 / (x2x1 * y2y1) *
         (q11 * x2x * y2y + q21 * xx1 * y2y + q12 * x2x * yy1 +
          q22 * xx1 * yy1);
}

// get the height noise for a particular x,y point on the world plane
inline float height_noise_at_blerp(World &world, int x, int y) {
  // calculate biome noise for the adjacent four points

  u32 oct = 4;
  u32 distance = 16;
  float xf = (float)x;
  float yf = (float)y;

  const i32 x1 = x - distance;
  float x1f = (float)x1;
  Biome &leftBiome = biome_at_point(world, x1, y);
  float leftNoise = leftBiome.noise.noise(oct, x1, y);

  const i32 x2 = x + distance;
  float x2f = (float)x2;
  Biome &rightBiome = biome_at_point(world, x2, y);
  float rightNoise = rightBiome.noise.noise(oct, x2, y);

  const i32 y1 = y - distance;
  float y1f = (float)y1;
  Biome &backBiome = biome_at_point(world, x, y1);
  float backNoise = backBiome.noise.noise(oct, x, y1);

  const i32 y2 = y + distance;
  float y2f = (float)y2;
  Biome &frontBiome = biome_at_point(world, x, y2);
  float frontNoise = frontBiome.noise.noise(oct, x, y2);

  auto actual_noise = blerp(leftNoise, rightNoise, backNoise, frontNoise, x1f,
                            x2f, y1f, y2f, xf, yf);
  return (actual_noise + 1.0f) / 2.0f;
}

inline float biome_influence_at_point(Biome const &biome, int x, int y,
                                      PointBiomeNoise bn) {
  switch (biome.kind) {
    case BiomeKind::Grassland: {
      return bn.height_noise * 0.2f + bn.rainfall_noise * 0.4f +
             bn.temp_noise * 0.4f;
    } break;
    case BiomeKind::Mountains: {
      return bn.height_noise * 0.8f + bn.rainfall_noise * 0.1f +
             bn.temp_noise * 0.1f;
    } break;
    case BiomeKind::Desert: {
      return bn.height_noise * 0.2f + bn.rainfall_noise * 0.1f +
             bn.temp_noise * 0.7f;
    } break;
    case BiomeKind::Tundra: {
      return bn.height_noise * 0.5f + bn.rainfall_noise * 0.3f +
             bn.temp_noise * 0.2f;
    } break;
    case BiomeKind::Taiga: {
      return bn.height_noise * 0.4f + bn.rainfall_noise * 0.4f +
             bn.temp_noise * 0.2f;
    } break;
    case BiomeKind::Forest: {
      return bn.height_noise * 0.3f + bn.rainfall_noise * 0.3f +
             bn.temp_noise * 0.4f;
    } break;
    case BiomeKind::Ocean: {
      return bn.height_noise * 0.1f + bn.rainfall_noise * 0.4f +
             bn.temp_noise * 0.5f;
    } break;
    case BiomeKind::Coast: {
      return bn.height_noise * 0.1f + bn.rainfall_noise * 0.4f +
             bn.temp_noise * 0.5f;
    } break;
    default: {
      return 0.0f;
    } break;
  }
}

// get the height noise for a particular x,y point on the world plane
inline u32 height_noise_at(World &world, int x, int y, PointBiomeNoise bn) {
  float actual_noise = 0.0f;
  u32 oct = 4;

  float total_weight = 0.0f;

  u32 biomes_amount = BiomeKind::Count;
  for (auto i = 0; i < biomes_amount; ++i) {
    auto bk = (BiomeKind)i;
    auto &biome = world.biomes_by_kind[bk];
    float biome_noise = (biome.noise.noise(oct, x, y) + 1.0f) / 2.0f;
    float weight = biome_influence_at_point(biome, x, y, bn);
    // fmt::print("biome influence {} at {}, {} = {}\n", bk, x, y, weight);
    total_weight += weight;
    auto pn = biome_noise * weight * 1.8f;
    // fmt::print("pn={}\n", pn);
    actual_noise += pn * biome.maxHeight;
  }
  float total = actual_noise / total_weight;
  // auto noise = (total + 1.0f) / 2.0f;
  // fmt::print("height={}\n", total);

  return (u32)total;
}

inline bool is_point_outside_chunk_boundaries(int x, int y) {
  return x < 0 || x > CHUNK_WIDTH || y < 0 || y > CHUNK_LENGTH;
}

const char *get_biome_name_at(World &world, WorldPos pos) {
  const auto &biome = biome_at_point(world, pos.x, pos.z);
  return biome.name;
}

void gen_column_at(World &world, Block *output, int x, int y) {
  auto height_noise = simple_height_noise_at(world, x, y);
  auto temp_noise = temperature_noise_at(world, x, y);
  auto rainfall_noise = rainfall_noise_at(world, x, y);
  PointBiomeNoise bn{.height_noise = height_noise,
                     .rainfall_noise = rainfall_noise,
                     .temp_noise = temp_noise};
  auto kind = biome_noise_to_kind_at_point(bn);
  auto &bk = world.biomes_by_kind[kind];
  // auto biome_height_noise = bk.noise.fractal(16, x, y);
  // int maxHeight = bk.maxHeight;
  // int columnHeight = noise * (float)maxHeight;
  auto noise = height_noise_at(world, x, y, bn);
  int columnHeight = noise;
  for (int height = columnHeight - 1; height >= 0; --height) {
    Block block;
    block.type = block_type_for_height(bk, height, columnHeight - 1);
    output[height] = block;
  }
  while (columnHeight < WATER_LEVEL) {
    output[columnHeight].type = BlockType::Water;
    columnHeight++;
  }
  // fill the rest with air
  for (int i = columnHeight; i < CHUNK_HEIGHT; ++i) {
    output[i].type = BlockType::Air;
  }
}

bool can_tree_grow_on(BlockType bt) {
  switch (bt) {
    case BlockType::Dirt:
      return true;
    case BlockType::Grass:
      return true;
    case BlockType::Snow:
      return true;
    case BlockType::TopGrass:
      return true;
    case BlockType::TopSnow:
      return true;
    case BlockType::JungleTopGrass:
      return true;
    default:
      return false;
  }
}

void build_oak_tree_at(World &world, Chunk &chunk, int x, int y) {
  auto col = &CHUNK_COL_AT(chunk, x, y);
  auto topBlockHeight = get_col_height(col);
  // start building a tree
  auto height =
      map(0.0f, 1.0f, MIN_TREE_HEIGHT, MAX_TREE_HEIGHT, world.tree_noise());
  auto treeTopHeight = topBlockHeight + height;
  auto h = topBlockHeight;
  // stump
  for (; h < treeTopHeight; ++h) {
    col[h].type = BlockType::Wood;
  }
  // crown
  auto bottomRadius = round(
      map(0.0f, 1.0f, TREE_MIN_RADIUS, TREE_MAX_RADIUS, world.tree_noise()));
  u32 crownHeight = round(
      map(0.0f, 1.0f, CROWN_MIN_HEIGHT, CROWN_MAX_HEIGHT, world.tree_noise()));
  u32 crownBottom = treeTopHeight - round((float)crownHeight / 2.0f);
  u32 crownTop = crownBottom + crownHeight;
  for (; crownBottom <= crownTop; ++crownBottom) {
    auto radius = bottomRadius;
    auto radius2 = radius * radius;
    auto startX = x - radius;
    auto startY = y - radius;
    auto endX = x + radius;
    auto endY = y + radius;
    for (int cx = startX; cx <= endX; ++cx) {
      for (int cy = startY; cy <= endY; ++cy) {
        if (cx == x && cy == y && crownBottom < treeTopHeight) {
          // don't replace the wood
          // TODO: Just place wood after the crown
          continue;
        }
        auto isOutsideChunkBoundaries =
            is_point_outside_chunk_boundaries(cx, cy);
        if (isOutsideChunkBoundaries) {
          // TODO: Transfer this information to the chunk to be
          // rendered with this block through some global state.
        } else {
          // append the block
          // the further from the center, the less likely to have
          // leaves here
          // minus one because we don't count the middle
          i32 dx = abs(cx - x) - 1;
          i32 dy = abs(cy - y) - 1;
          u32 distanceFromCenter2 = abs(dx * dx + dy * dy);
          float distanceFromCenterProp =
              (float)distanceFromCenter2 / (float)radius2;
          // k is the base chance of leaves appearing on the edge
          float k = -0.05;
          float unlikelinessOfHavingLeavesBasedOnRadius =
              max(0.0f, distanceFromCenterProp - k);
          float noise = world.tree_noise();
          auto needBlockHere = noise > unlikelinessOfHavingLeavesBasedOnRadius;
          if (needBlockHere) {
            CHUNK_AT(chunk, cx, cy, crownBottom).type = BlockType::Leaves;
          }
        }
      }
    }
  }
}

void build_jungle_tree(World &world, Chunk &chunk, int x, int y) {
  auto col = &CHUNK_COL_AT(chunk, x, y);
  auto topBlockHeight = get_col_height(col);
  // start building a tree
  auto height = map(0.0f, 1.0f, MIN_JUNGLE_TREE_HEIGHT, MAX_JUNGLE_TREE_HEIGHT,
                    world.tree_noise());
  auto treeTopHeight = topBlockHeight + height;
  auto h = topBlockHeight;

  // crown
  auto bottomRadius = round(map(0.0f, 1.0f, JUNGLE_TREE_MIN_RADIUS,
                                JUNGLE_TREE_MAX_RADIUS, world.tree_noise()));
  u32 crownHeight = round(
      map(0.0f, 1.0f, CROWN_MIN_HEIGHT, CROWN_MAX_HEIGHT, world.tree_noise()));
  u32 crownBottom = treeTopHeight - round((float)crownHeight / 2.0f);
  u32 crownTop = crownBottom + crownHeight;
  for (; crownBottom <= crownTop; ++crownBottom) {
    auto radius = bottomRadius;
    auto radius2 = radius * radius;
    auto startX = x - radius;
    auto startY = y - radius;
    auto endX = x + radius;
    auto endY = y + radius;
    for (int cx = startX; cx <= endX; ++cx) {
      for (int cy = startY; cy <= endY; ++cy) {
        auto isOutsideChunkBoundaries =
            is_point_outside_chunk_boundaries(cx, cy);
        if (isOutsideChunkBoundaries) {
          // TODO: Transfer this information to the chunk to be
          // rendered with this block through some global state.
        } else {
          // append the block
          // the further from the center, the less likely to have
          // leaves here
          // minus one because we don't count the middle
          i32 dx = abs(cx - x) - 1;
          i32 dy = abs(cy - y) - 1;
          u32 distanceFromCenter2 = abs(dx * dx + dy * dy);
          float distanceFromCenterProp =
              (float)distanceFromCenter2 / (float)radius2;
          // k is the base chance of leaves appearing on the edge
          float k = -0.05;
          float unlikelinessOfHavingLeavesBasedOnRadius =
              max(0.0f, distanceFromCenterProp - k);
          float noise = world.tree_noise();
          auto needBlockHere = noise > unlikelinessOfHavingLeavesBasedOnRadius;
          if (needBlockHere) {
            CHUNK_AT(chunk, cx, cy, crownBottom).type =
                BlockType::JungleTreeLeaves;
          }
        }
      }
    }
  }

  // stump
  auto TREE_STUMP_RADIUS = 2;
  auto initialH = h;
  for (int xx = x; xx < x + TREE_STUMP_RADIUS; ++xx) {
    for (int yy = y; yy < y + TREE_STUMP_RADIUS; ++yy) {
      for (auto h = initialH; h < treeTopHeight; ++h) {
        auto isOutsideChunkBoundaries =
            is_point_outside_chunk_boundaries(xx, yy);
        if (isOutsideChunkBoundaries) continue;
        auto coll = &CHUNK_COL_AT(chunk, xx, yy);
        coll[h].type = BlockType::JungleWood;
      }
    }
  }
}

void build_pine_tree_at(World &world, Chunk &chunk, int x, int y) {
  auto col = &CHUNK_COL_AT(chunk, x, y);
  auto topBlockHeight = get_col_height(col);
  // start building a tree
  auto height = map(0.0f, 1.0f, MIN_PINE_TREE_HEIGHT, MAX_PINE_TREE_HEIGHT,
                    world.tree_noise());
  auto treeTopHeight = topBlockHeight + height;
  auto h = topBlockHeight;
  // stump
  for (; h < treeTopHeight; ++h) {
    col[h].type = BlockType::PineWood;
  }
  // crown
  auto maxRadius = round(map(0.0f, 1.0f, PINE_TREE_MIN_RADIUS,
                             PINE_TREE_MAX_RADIUS, world.tree_noise()));
  u32 crownHeight = round(map(0.0f, 1.0f, PINE_CROWN_MIN_HEIGHT,
                              PINE_CROWN_MAX_HEIGHT, world.tree_noise()));
  u32 crownBottom = treeTopHeight - round((float)crownHeight / 2.0f);
  u32 crownTop = crownBottom + crownHeight;
  auto radiusMax2 = maxRadius * maxRadius;
  for (; crownBottom <= crownTop; ++crownBottom) {
    auto even = crownBottom % 2 == 0;
    auto radius = even ? maxRadius : PINE_TREE_MIN_RADIUS;
    auto startX = x - radius;
    auto startY = y - radius;
    auto endX = x + radius;
    auto endY = y + radius;
    for (int cx = startX; cx <= endX; ++cx) {
      for (int cy = startY; cy <= endY; ++cy) {
        if (cx == x && cy == y && crownBottom < treeTopHeight) {
          // don't replace the wood
          // TODO: Just place wood after the crown
          continue;
        }
        auto isOutsideChunkBoundaries =
            is_point_outside_chunk_boundaries(cx, cy);
        if (isOutsideChunkBoundaries) {
          // TODO: Transfer this information to the chunk to be
          // rendered with this block through some global state.
        } else {
          // append the block
          // the further from the center, the less likely to have
          // leaves here
          // minus one because we don't count the middle
          i32 dx = abs(cx - x) - 1;
          i32 dy = abs(cy - y) - 1;
          u32 distanceFromCenter2 = abs(dx * dx + dy * dy);
          float distanceFromCenterProp =
              (float)distanceFromCenter2 / (float)radiusMax2;
          // k is the base chance of leaves appearing on the edge
          float k = 0.35f;
          float unlikelinessOfHavingLeavesBasedOnRadius =
              max(0.0f, distanceFromCenterProp - k);
          // how high are we, 0.0 - 1.0
          float howHigh = (float)crownBottom / (float)crownTop;
          float hp = howHigh * 0.45f;
          // float unlikelinessOfHavingLeaves = hp;
          float unlikelinessOfHavingLeaves =
              unlikelinessOfHavingLeavesBasedOnRadius + hp;
          float noise = world.tree_noise();
          auto needBlockHere = noise > unlikelinessOfHavingLeaves;
          if (needBlockHere) {
            CHUNK_AT(chunk, cx, cy, crownBottom).type =
                BlockType::PineTreeLeaves;
          }
        }
      }
    }
  }
}

// Generates the height map and block types
void gen_chunk(World &world, Chunk &chunk) {
  for (int x = 0; x < CHUNK_WIDTH; ++x) {
    int global_x = chunk.x + x;
    for (int y = 0; y < CHUNK_LENGTH; ++y) {
      int global_y = chunk.y + y;
      gen_column_at(world, &CHUNK_COL_AT(chunk, x, y), global_x, global_y);
    }
  }

  // generate trees
  for (int x = 0; x < CHUNK_WIDTH; ++x) {
    int global_x = chunk.x + x;
    for (int y = 0; y < CHUNK_LENGTH; ++y) {
      int global_y = chunk.y + y;
      WorldPos pos{global_x, 0, global_y};
      auto &biome = biome_at_point(world, global_x, global_y);
      auto col = &CHUNK_COL_AT(chunk, x, y);
      auto topBlockHeight = get_col_height(col);
      if (!can_tree_grow_on(col[topBlockHeight].type)) {
        continue;
      }
      float r = world.tree_noise();
      auto has_tree_center_here = r < biome.treeFrequency;
      if (has_tree_center_here) {
        biome.treeGen(world, chunk, x, y);
      }
    }
  }

  for (auto &atom : world.changes) {
    auto pos = atom.pos;
    auto ch = &chunk;
    auto pos_inside = (pos.x > ch->x && pos.x < ch->x + CHUNK_WIDTH) &&
                      (pos.y > ch->y && pos.y < ch->y + CHUNK_LENGTH);
    if (!pos_inside) continue;
    auto local_pos = chunk_global_to_local_pos(&chunk, pos);
    CHUNK_AT(chunk, local_pos.x, local_pos.y, local_pos.z).type =
        atom.block.type;
  }
}

void occlusion(char neighbors[27], char lights[27], float shades[27],
               float ao[6][4], float light[6][4]) {
  static const int lookup3[6][4][3] = {
      {{0, 1, 3}, {2, 1, 5}, {6, 3, 7}, {8, 5, 7}},
      {{18, 19, 21}, {20, 19, 23}, {24, 21, 25}, {26, 23, 25}},
      {{6, 7, 15}, {8, 7, 17}, {24, 15, 25}, {26, 17, 25}},
      {{0, 1, 9}, {2, 1, 11}, {18, 9, 19}, {20, 11, 19}},
      {{0, 3, 9}, {6, 3, 15}, {18, 9, 21}, {24, 15, 21}},
      {{2, 5, 11}, {8, 5, 17}, {20, 11, 23}, {26, 17, 23}}};
  static const int lookup4[6][4][4] = {
      {{0, 1, 3, 4}, {1, 2, 4, 5}, {3, 4, 6, 7}, {4, 5, 7, 8}},
      {{18, 19, 21, 22}, {19, 20, 22, 23}, {21, 22, 24, 25}, {22, 23, 25, 26}},
      {{6, 7, 15, 16}, {7, 8, 16, 17}, {15, 16, 24, 25}, {16, 17, 25, 26}},
      {{0, 1, 9, 10}, {1, 2, 10, 11}, {9, 10, 18, 19}, {10, 11, 19, 20}},
      {{0, 3, 9, 12}, {3, 6, 12, 15}, {9, 12, 18, 21}, {12, 15, 21, 24}},
      {{2, 5, 11, 14}, {5, 8, 14, 17}, {11, 14, 20, 23}, {14, 17, 23, 26}}};
  static const float curve[4] = {0.0, 0.25, 0.5, 0.75};
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 4; j++) {
      int corner = neighbors[lookup3[i][j][0]];
      int side1 = neighbors[lookup3[i][j][1]];
      int side2 = neighbors[lookup3[i][j][2]];
      int value = side1 && side2 ? 3 : corner + side1 + side2;
      float shade_sum = 0;
      float light_sum = 0;
      int is_light = lights[13] == 15;
      for (int k = 0; k < 4; k++) {
        shade_sum += shades[lookup4[i][j][k]];
        light_sum += lights[lookup4[i][j][k]];
      }
      if (is_light) {
        light_sum = 15 * 4 * 10;
      }
      float total = curve[value] + shade_sum / 4.0;
      ao[i][j] = min<float>(total, 1.0);
      light[i][j] = light_sum / 15.0 / 4.0;
    }
  }
}

void load_chunk_at(World &world, int chunk_x, int chunk_y, Chunk &chunk) {
  auto *mesh = new ChunkMesh();

  // fmt::print("Loading chunk at {}, {}\n", chunk_x, chunk_y);
  chunk.x = chunk_x;
  chunk.y = chunk_y;

  // Determine the height map
  gen_chunk(world, chunk);

  // Generate the mesh
  for (int x = 0; x < CHUNK_WIDTH; ++x) {
    int global_x = chunk.x + x;

    for (int y = 0; y < CHUNK_LENGTH; ++y) {
      int global_y = chunk.y + y;
      Block *bottomBlock = &CHUNK_COL_AT(chunk, x, y);
      // start from the highest block
      Block *column = &(bottomBlock[CHUNK_HEIGHT - 1]);

      // skip the air blocks from above
      while (column->type == BlockType::Air) column--;

      do {
        Block block = *column;
        int height = column - bottomBlock;

        // check which faces are exposed to a Transparent Block
        int left = IS_TB_LEFT_OF(chunk, x, y, height);
        int right = IS_TB_RIGHT_OF(chunk, x, y, height);
        int front = IS_TB_FRONT_OF(chunk, x, y, height);
        int back = IS_TB_BACK_OF(chunk, x, y, height);
        int top = IS_TB_TOP_OF(chunk, x, y, height);
        int bottom = IS_TB_BOTTOM_OF(chunk, x, y, height);

        int wleft = 0;
        int wright = 0;
        int wtop = 0;
        int wbottom = 0;
        int wfront = 0;
        int wback = 0;
        float n = 0.5;  // scaling

        float ao[6][4] = {0};
        float light[6][4] = {{0.5, 0.5, 0.5, 0.5}, {0.5, 0.5, 0.5, 0.5},
                             {0.5, 0.5, 0.5, 0.5}, {0.5, 0.5, 0.5, 0.5},
                             {0.5, 0.5, 0.5, 0.5}, {0.5, 0.5, 0.5, 0.5}};

        // char neighbors[27] = {0};
        // char lights[27] = {0};
        // float shades[27] = {0};
        // int index = 0;
        // for (int dx = -1; dx <= 1; dx++) {
        //   for (int dy = -1; dy <= 1; dy++) {
        //     for (int dz = -1; dz <= 1; dz++) {
        //       neighbors[index] = opaque[XYZ(x + dx, y + dy, z + dz)];
        //       lights[index] = light[XYZ(x + dx, y + dy, z + dz)];
        //       shades[index] = 0;
        //       if (y + dy <= highest[XZ(x + dx, z + dz)]) {
        //         for (int oy = 0; oy < 8; oy++) {
        //           if (opaque[XYZ(x + dx, y + dy + oy, z + dz)]) {
        //             shades[index] = 1.0 - oy * 0.125;
        //             break;
        //           }
        //         }
        //       }
        //       index++;
        //     }
        //   }
        // }
        // occlusion(neighbors, lights, shades, ao, light);

        make_cube_faces(*mesh, ao, light, left, right, top, bottom, front, back,
                        wleft, wright, wtop, wbottom, wfront, wback,
                        (float)global_x, (float)height, (float)global_y, n,
                        block.type);
        column--;
      } while (column != bottomBlock);
    }
  }

  // Only allocate a new buffer if none was allocated before
  auto shader = shader_storage::get_shader("block");
  auto block_attrib = shader->attr;
  //  if (chunk.vao == 0) {
  // Configure the VAO
  glGenVertexArrays(1, &chunk.vao);
  glGenBuffers(1, &chunk.buffer);
#ifdef VAO_ALLOCATION
  fmt::print("Allocating VAO={}\n", chunk.vao);
#endif
  //  }
  glBindVertexArray(chunk.vao);

  // Update chunk buffer mesh data
  glBindBuffer(GL_ARRAY_BUFFER, chunk.buffer);
  // so update the chunk buffer
  auto mesh_size = sizeof((*mesh)[0]) * mesh->size();
  auto *meshp = mesh->data();
  chunk.mesh_size = mesh->size();
  glBufferData(GL_ARRAY_BUFFER, mesh_size, meshp, GL_STATIC_DRAW);

  // Update VAO settings
  GLsizei stride = sizeof(VertexData);
  glVertexAttribPointer(block_attrib.position, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(VertexData, pos));
  glVertexAttribPointer(block_attrib.normal, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(VertexData, normal));
  glVertexAttribPointer(block_attrib.uv, 2, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(VertexData, uv));
  // glVertexAttribPointer(block_attrib.ao, 1, GL_FLOAT, GL_FALSE, stride,
  //                       (void *)offsetof(VertexData, ao));
  // glVertexAttribPointer(block_attrib.light, 1, GL_FLOAT, GL_FALSE, stride,
  //                       (void *)offsetof(VertexData, light));
  glEnableVertexAttribArray(block_attrib.position);
  glEnableVertexAttribArray(block_attrib.normal);
  glEnableVertexAttribArray(block_attrib.uv);
  // glEnableVertexAttribArray(block_attrib.ao);
  // glEnableVertexAttribArray(block_attrib.light);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(0);
  glDeleteBuffers(1, &chunk.buffer);

  // we've regenerated the chunk mesh
  chunk.is_dirty = false;

  delete mesh;
}

void unload_chunk(Chunk *chunk) {
  fmt::print("Unloading chunk at {}, {}\n", chunk->x, chunk->y);
#ifdef VAO_ALLOCATION
  fmt::print("Deallocating VAO={}\n", chunk->vao);
#endif
  glDeleteVertexArrays(1, &chunk->vao);
}

// distance from one chunk to another, in chunks
// u32 distance_between_chunks(Chunk& a, Chunk& b) {
//     return 0;
// }
//
// u32 distance_to_chunk(Chunk& chunk, WorldPos pos) {
//     auto& curr_player_chunk = world.player;
// }

inline u32 distance_to_chunk(WorldPos pos, Chunk *chunk) {
  return (pos.x - chunk->x) * (pos.x - chunk->x) +
         (pos.y - chunk->y) * (pos.y - chunk->y);
}

void unload_distant_chunks(World &world, WorldPos center_pos, u32 radius) {
  int center_x = center_pos.x;
  int center_y = center_pos.z;

  for (auto it = world.loaded_chunks.begin();
       it != world.loaded_chunks.end();) {
    auto chunkp = it->second;

    if (chunkp == nullptr) {
      it = world.loaded_chunks.erase(it);
      continue;
    }

    int first_chunk_x = round_to_nearest_16(center_x) - (CHUNK_WIDTH * radius);
    int first_chunk_y = round_to_nearest_16(center_y) - (CHUNK_LENGTH * radius);
    int last_chunk_x = first_chunk_x + CHUNK_WIDTH * radius * 2;
    int last_chunk_y = first_chunk_y + CHUNK_LENGTH * radius * 2;
    bool in_radius = chunkp->x >= first_chunk_x && chunkp->x <= last_chunk_x &&
                     chunkp->y >= first_chunk_y && chunkp->y <= last_chunk_y;

    if (!in_radius) {
      unload_chunk(chunkp);
      it = world.loaded_chunks.erase(it);
    } else {
      ++it;
    }
  }
}

// Find the loaded chunk which contains the specified position
Chunk *find_chunk_with_pos(World &world, WorldPos pos) {
  Chunk *chunk = nullptr;
  for (auto &ch : world.chunks) {
    auto pos_inside = (pos.x > ch->x && pos.x < ch->x + CHUNK_WIDTH) &&
                      (pos.y > ch->y && pos.y < ch->y + CHUNK_LENGTH);
    if (pos_inside) chunk = ch;
  }
  return chunk;
}

void chunk_modify_block_at_global(World &world, Chunk *chunk, WorldPos pos,
                                  BlockType type) {
  fmt::print("Modified block at {},{},{}\n", pos.x, pos.y, pos.z);
  world.changes.push_back({.pos = pos, .block = {.type = type}});
  // auto local_pos = chunk_global_to_local_pos(chunk, pos);
  // Block b;
  // b.type = type;
  // chunk->blocks[local_pos.x][local_pos.y][local_pos.z] = b;
  chunk->is_dirty = true;
}

optional<Block> get_block_at_global_pos(World &world, WorldPos pos) {
  auto *chunk = find_chunk_with_pos(world, pos);
  if (chunk == nullptr) return {};
  return chunk_get_block_at_global(chunk, pos);
}

void place_block_at(World &world, BlockType type, WorldPos pos) {
  // determine which chunk is affected
  auto *chunk = find_chunk_with_pos(world, pos);
  if (chunk == nullptr) {
    fmt::print(
        "Failed to determine which chunk to place the block in. Placement "
        "coords: {}, {}, {}\n",
        pos.x, pos.y, pos.z);
    return;
  }
  chunk_modify_block_at_global(world, chunk, pos, type);
}

inline bool can_place_at_block(BlockType type) {
  switch (type) {
    case BlockType::Water: {
      return false;
    } break;
    default: {
      return true;
    } break;
  }
}

void load_chunks_around_player(World &world, WorldPos center_pos,
                               uint32_t radius) {
  world.chunks = {};

  int center_x = center_pos.x;
  int center_y = center_pos.z;
  int first_chunk_x = round_to_nearest_16(center_x) - (CHUNK_WIDTH * radius);
  int first_chunk_y = round_to_nearest_16(center_y) - (CHUNK_LENGTH * radius);

  int chunk_cols = radius * 2;
  int chunk_rows = radius * 2;
  int chunk_idx = 0;

  for (int chunk_row = 0; chunk_row < chunk_rows; ++chunk_row) {
    int chunk_y = first_chunk_y + chunk_row * CHUNK_LENGTH;
    for (int chunk_col = 0; chunk_col < chunk_cols; ++chunk_col) {
      int chunk_x = first_chunk_x + chunk_col * CHUNK_WIDTH;
      auto loaded_ch = is_chunk_loaded(world, chunk_x, chunk_y);
      bool loaded = loaded_ch != nullptr;

      if (!loaded) {
        loaded_ch = new Chunk();
        load_chunk_at(world, chunk_x, chunk_y, *loaded_ch);
        world.loaded_chunks.insert(
            {chunk_id_from_coords(chunk_x, chunk_y), loaded_ch});
      } else if (loaded_ch->is_dirty) {
        fmt::print("Chunk is dirty, reloading mesh...\n");
        load_chunk_at(world, chunk_x, chunk_y, *loaded_ch);
      } else {
        // no need to regenerate
      }

      world.chunks.push_back(loaded_ch);

      ++chunk_idx;
    }
  }
}

glm::ivec3 biome_color(BiomeKind bk) {
  switch (bk) {
    case BiomeKind::Desert: {
      return glm::ivec3(194, 178, 128);
    }
    case BiomeKind::Forest: {
      return glm::ivec3(53, 88, 41);
    }
    case BiomeKind::Grassland: {
      return glm::ivec3(143, 177, 78);
    }
    case BiomeKind::Mountains: {
      return glm::ivec3(174, 162, 149);
    }
    case BiomeKind::Ocean: {
      return glm::ivec3(0, 0, 255);
    }
    case BiomeKind::Taiga: {
      return glm::ivec3(205, 205, 205);
    }
    case BiomeKind::Tundra: {
      return glm::ivec3(255, 255, 255);
    }
    default: {
      return glm::ivec3(255, 255, 0);
    }
  }
}

// average color for a block
Color block_kind_color(BlockType bt) {
  if (auto atlas_image = image_storage::get_image("block")) {
    auto offset = block_type_texture_offset_int(bt);
    // calculate the average
    ivec4 res{(byte)0, (byte)0, (byte)0, (byte)0};
    u32 npixels = 0;
    for (i32 x = 0; x < 1; ++x) {
      for (i32 y = 0; y < 1; ++y) {
        Color pix = (*atlas_image)(offset.x + x, offset.y - y);
        res.r += static_cast<i32>(pix.r);
        res.g += static_cast<i32>(pix.g);
        res.b += static_cast<i32>(pix.b);
        res.a += static_cast<i32>(pix.a);
        npixels++;
      }
    }
    float npixelsf = (float)npixels;
    Color col{
        static_cast<byte>(static_cast<float>(res.r) / npixelsf),
        static_cast<byte>(static_cast<float>(res.g) / npixelsf),
        static_cast<byte>(static_cast<float>(res.b) / npixelsf),
        static_cast<byte>(static_cast<float>(res.a) / npixelsf),
    };
    return res;
  }
  return MISSING_COLOR;
}

Block get_top_block_of_column(Chunk &chunk, int x, int y) {
  auto col = &CHUNK_COL_AT(chunk, x, y);
  auto topBlockHeight = get_col_height(col);
  Block topBlock = col[topBlockHeight];
  return topBlock;
}

void foreach_col_in_chunk(Chunk &chunk, std::function<void(int, int)> fun) {
  for (int __x = 0; __x < CHUNK_WIDTH; ++__x) {
    for (int __y = 0; __y < CHUNK_LENGTH; ++__y) {
      fun(__x, __y);
    }
  }
}

constexpr int NM_W = 1024;
constexpr int NM_H = NM_W;

void world_dump_heights(World &world, const string &out_dir) {
  fs::create_directory(out_dir);

  auto *ortho_view = new array<array<Pixel, NM_W>, NM_H>();
  {
    auto *chunk = new Chunk();
    fmt::print("Generating complete orthogonal view image\n");
    // full map ortho view (by chunk)
    for (i32 x = 0; x < NM_W; x += CHUNK_WIDTH) {
      for (i32 y = 0; y < NM_H; y += CHUNK_LENGTH) {
        chunk->x = x;
        chunk->y = y;
        gen_chunk(world, *chunk);
        for (i32 lx = 0; lx < CHUNK_WIDTH; ++lx) {
          for (i32 ly = 0; ly < CHUNK_LENGTH; ++ly) {
            auto xx = x + lx;
            auto yy = y + ly;
            auto col = &CHUNK_COL_AT(*chunk, lx, ly);
            uint32_t height = get_col_height(col);
            Block topBlock = col[height];
            auto c = block_kind_color(topBlock.type);
            (*ortho_view)[xx][yy] = c;
          }
        }
      }
    }
  }

  fmt::print("Generating other noise maps...\n");
  auto *whm = new array<array<Pixel, NM_W>, NM_H>();
  auto *biome_kind_map = new array<array<Pixel, NM_W>, NM_H>();
  auto *world_height_map = new array<array<Pixel, NM_W>, NM_H>();
  auto *temp_map = new array<array<Pixel, NM_W>, NM_H>();

  for (int x = 0; x < NM_W; ++x) {
    for (int y = 0; y < NM_H; ++y) {
      auto temp_noise = temperature_noise_at(world, x, y);
      auto rainfall_noise = rainfall_noise_at(world, x, y);

      // rainfall noise
      {
        int noise_value = round(rainfall_noise * 256);
        int r = noise_value;
        int g = noise_value;
        int b = noise_value;
        int a = 255;
        (*whm)[x][y] = rgba_color(byte(r), byte(g), byte(b), byte(a));
      }

      // temperature noise
      {
        int noise_value = round(temp_noise * 256);
        int r = noise_value;
        int g = noise_value;
        int b = noise_value;
        int a = 255;
        (*temp_map)[x][y] = rgba_color(byte(r), byte(g), byte(b), byte(a));
      }

      auto height_noise = simple_height_noise_at(world, x, y);
      PointBiomeNoise bn{
          .height_noise = height_noise,
          .rainfall_noise = rainfall_noise,
          .temp_noise = temp_noise,
      };
      auto kind = biome_noise_to_kind_at_point(bn);
      auto &bk = world.biomes_by_kind[kind];
      auto noise = height_noise_at(world, x, y, bn);

      // biome kind
      {
        auto bk_color = biome_color(bk.kind);
        int r = bk_color.r;
        int g = bk_color.g;
        int b = bk_color.b;
        int a = 255;
        (*biome_kind_map)[x][y] =
            rgba_color(byte(r), byte(g), byte(b), byte(a));
      }

      // heightmap
      {
        int noise_value = round(noise * 256);
        int r = noise_value;
        int g = noise_value;
        int b = noise_value;
        int a = 255;
        (*world_height_map)[x][y] =
            rgba_color(byte(r), byte(g), byte(b), byte(a));
      }
    }
  }

  fmt::print("Done with noise.\n");
  fmt::print("Writing the images into directory at {}...\n", out_dir);
  stbi_write_png(fmt::format("{}/heightmap.png", out_dir).c_str(), NM_W, NM_H,
                 4, world_height_map->data(), 0);
  stbi_write_png(fmt::format("{}/rain_map.png", out_dir).c_str(), NM_W, NM_H, 4,
                 whm->data(), 0);
  stbi_write_png(fmt::format("{}/temp_map.png", out_dir).c_str(), NM_W, NM_H, 4,
                 temp_map->data(), 0);
  stbi_write_png(fmt::format("{}/biome_kind.png", out_dir).c_str(), NM_W, NM_H,
                 4, biome_kind_map->data(), 0);
  stbi_write_png(fmt::format("{}/ortho.png", out_dir).c_str(), NM_W, NM_H, 4,
                 ortho_view->data(), 0);
  fmt::print("Done.\n");
  delete whm;
  delete biome_kind_map;
  delete world_height_map;
  delete temp_map;
  delete ortho_view;
}

// Calculate a PNG image representing the orthogonal view of the
// currently loaded world chunks around the player
Image *calculate_minimap_image(World &world, WorldPos pos, u32 radius) {
  auto center_pos = pos;
  int center_x = center_pos.x;
  int center_y = center_pos.z;
  int first_chunk_x = round_to_nearest_16(center_x) - (CHUNK_WIDTH * radius);
  int first_chunk_y = round_to_nearest_16(center_y) - (CHUNK_LENGTH * radius);
  int width = CHUNK_WIDTH * radius;
  int height = CHUNK_LENGTH * radius;
  auto *image = new Image(width, height);
  for (auto *chunk : world.chunks) {
    int texture_pos_x = chunk->x - first_chunk_x;
    int texture_pos_y = chunk->y - first_chunk_y;
    foreach_col_in_chunk(*chunk, [&](int x, int y) -> void {
      auto topBlock = get_top_block_of_column(*chunk, x, y);
      auto c = block_kind_color(topBlock.type);
      auto image_x = texture_pos_x + x;
      auto image_y = texture_pos_y + y;
      image->set(image_x, image_y, c);
    });
  }
  return image;
}

void calculate_minimap_tex(Texture &texture, World &world, WorldPos pos,
                           u32 radius) {
  auto *img = calculate_minimap_image(world, pos, radius);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture.texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, img->data);
  glGenerateMipmap(GL_TEXTURE_2D);
}

void init_world(World &world, Seed seed) {
  world.height_noise =
      OpenSimplexNoiseWParam{0.0025f, 32.0f, 2.0f, 0.6f, seed + 28394723234234};
  world.rainfall_noise =
      OpenSimplexNoiseWParam{0.0005f, 2.0f, 3.0f, 0.5f, seed + 89273492837497};
  world.temperature_noise =
      OpenSimplexNoiseWParam{0.00075f, 1.0f, 2.0f, 0.5f, seed + 89213674293468};

  int bseed = 0;

  // Desert
  world.biomes_by_kind.insert(
      {BiomeKind::Desert,
       Biome{
           .kind = BiomeKind::Desert,
           .maxHeight = (int)(CHUNK_HEIGHT * 0.68),
           .noise = OpenSimplexNoiseWParam{0.00002f, 0.5f, 2.0f, 0.5f, bseed},
           .treeFrequency = 0.0f,
           .treeGen = build_oak_tree_at,
           .name = "Desert",
       }});

  // Forest
  world.biomes_by_kind.insert(
      {BiomeKind::Forest,
       Biome{
           .kind = BiomeKind::Forest,
           .maxHeight = (int)(CHUNK_HEIGHT * 0.88),
           .noise = OpenSimplexNoiseWParam{0.004f, 1.0f, 2.0f, 0.5f, bseed},
           .treeFrequency = 0.035f,
           .treeGen = build_oak_tree_at,
           .name = "Forest",
       }});

  // Jungle
  world.biomes_by_kind.insert(
      {BiomeKind::Jungle,
       Biome{
           .kind = BiomeKind::Jungle,
           .maxHeight = (int)(CHUNK_HEIGHT * 0.84),
           .noise = OpenSimplexNoiseWParam{0.01f, 1.0f, 2.0f, 0.5f, bseed},
           .treeFrequency = 0.065f,
           .treeGen = build_jungle_tree,
           .name = "Jungle",
       }});

  // Grassland
  world.biomes_by_kind.insert(
      {BiomeKind::Grassland,
       Biome{
           .kind = BiomeKind::Grassland,
           .maxHeight = (int)(CHUNK_HEIGHT * 0.75),
           .noise = OpenSimplexNoiseWParam{0.00001f, 1.0f, 2.0f, 0.5f, bseed},
           .treeFrequency = 0.0012f,
           .treeGen = build_oak_tree_at,
           .name = "Grassland",
       }});

  // Tundra
  world.biomes_by_kind.insert(
      {BiomeKind::Tundra,
       Biome{
           .kind = BiomeKind::Tundra,
           .maxHeight = (int)(CHUNK_HEIGHT * 0.8),
           .noise = OpenSimplexNoiseWParam{0.002f, 1.0f, 2.0f, 0.5f, bseed},
           .treeFrequency = 0.001f,
           .treeGen = build_pine_tree_at,
           .name = "Tundra",
       }});

  // Taiga
  world.biomes_by_kind.insert(
      {BiomeKind::Taiga,
       Biome{
           .kind = BiomeKind::Taiga,
           .maxHeight = (int)(CHUNK_HEIGHT * 0.8),
           .noise = OpenSimplexNoiseWParam{0.004f, 1.0f, 2.0f, 0.5f, bseed},
           .treeFrequency = 0.25f,
           .treeGen = build_pine_tree_at,
           .name = "Taiga",
       }});

  // Oceans
  world.biomes_by_kind.insert(
      {BiomeKind::Ocean,
       Biome{
           .kind = BiomeKind::Ocean,
           .maxHeight = (int)(WATER_LEVEL * 0.8),
           .noise = OpenSimplexNoiseWParam{0.001f, 1.0f, 2.0f, 0.5f, bseed},
           .treeFrequency = 0.0f,
           .treeGen = build_oak_tree_at,
           .name = "Ocean",
       }});

  // Coast
  world.biomes_by_kind.insert(
      {BiomeKind::Coast,
       Biome{
           .kind = BiomeKind::Coast,
           .maxHeight = WATER_LEVEL + 3,
           .noise = OpenSimplexNoiseWParam{0.001f, 1.0f, 2.0f, 0.5f, bseed},
           .treeFrequency = 0.000001f,
           .treeGen = build_oak_tree_at,
           .name = "Coast",
       }});

  // Mountains
  world.biomes_by_kind.insert(
      {BiomeKind::Mountains,
       Biome{
           .kind = BiomeKind::Mountains,
           .maxHeight = CHUNK_HEIGHT,
           .noise = OpenSimplexNoiseWParam{0.001f, 64.0f, 2.0f, 0.5f, bseed},
           .treeFrequency = 0.002f,
           .treeGen = build_oak_tree_at,
           .name = "Mountains",
       }});
}

void world_update(World &world, float dt) {
  // update time
  int ticks_passed = dt * TICKS_PER_SECOND;
  world.time += ticks_passed;
  if (world.time_of_day >= DAY_DURATION) {
    world.time_of_day = 0;
  } else {
    world.time_of_day += ticks_passed;
  }
  world.is_day = world.time_of_day < (DAY_DURATION / 2);

  // calculate celestial bdy position
  float sun_degrees = 720.0f * ((float)world.time_of_day / (float)DAY_DURATION);
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::rotate(model, glm::radians(sun_degrees),
                      glm::vec3(0.0f, 0.0f, 1.0f));
  world.sun_pos = model * glm::vec4(0.0, 100.0f, 0.0f, 0.0f);

  // day/night & sky color
  float tdf = (float)world.time_of_day;
  float blend_factor = 0;
  float NIGHT_START_BF = 0.0f;
  float MORNING_START_BF = 0.25f;
  float DAY_START_BF = 1.0f;
  float EVENING_START_BF = 0.6f;
  // float total_day_passed =
  //     (float)state.world.time_of_day / (float)DAY_DURATION;
  if (tdf >= 0 && tdf < MORNING) {
    // night
    // float night_passed = (float)state.world.time_of_day / (float)MORNING;
    blend_factor = map(0.0f, MORNING, 0.0f, MORNING_START_BF, tdf);
  } else if (tdf > MORNING && tdf < DAY) {
    // morning
    // float morning_passed = (float)state.world.time_of_day / (float)DAY;
    // blend_factor = MORNING_START_BF + (DAY_START_BF * morning_passed *
    //                                    (1.0f - MORNING_START_BF));
    blend_factor = map(MORNING, DAY, MORNING_START_BF, DAY_START_BF, tdf);
  } else if (tdf < EVENING) {
    // day
    // float day_passed = (float)state.world.time_of_day / (float)EVENING;
    // blend_factor = DAY_START_BF + (1.0f / day_passed);
    blend_factor = map(DAY, EVENING, DAY_START_BF, EVENING_START_BF, tdf);
  } else if (tdf < NIGHT) {
    // evening
    // float ev_passed = (float)state.world.time_of_day / (float)NIGHT;
    // blend_factor = ev_passed * 0.8f;
    blend_factor = map(EVENING, NIGHT, EVENING_START_BF, NIGHT_START_BF, tdf);
  } else {
  }
  world.sky_color = mix(colorNight, colorDay, blend_factor);
}
