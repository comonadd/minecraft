#include "world.hpp"

#include <GL/glew.h>
#include <fmt/core.h>

#include <glm/glm.hpp>

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
      return BlockType::Water;
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
    default:
      return BlockType::Snow;
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
    BlockType::TopGrass, BlockType::Wood,
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

inline BiomeKind biome_noise_to_kind_at_point(float temp_noise,
                                              float rainfall_noise) {
  if (temp_noise > 0.6) {
    // high temperature
    if (rainfall_noise > 0.5) {
      // high temp, high rainfall
      // needs to be jungle
      return BiomeKind::Forest;
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
    // do something else here
    return BiomeKind::Tundra;
  }
}

inline float temperature_noise_at(World &world, int x, int y) {
  return world.temperature_noise.fractal(16, x, y);
}

inline float rainfall_noise_at(World &world, int x, int y) {
  return world.rainfall_noise.fractal(16, x, y);
}

inline bool is_point_outside_chunk_boundaries(int x, int y) {
  return x < 0 || x > CHUNK_WIDTH || y < 0 || y > CHUNK_LENGTH;
}

inline Biome biome_at_point(World &world, WorldPos pos) {
  auto temp_noise = temperature_noise_at(world, pos.x, pos.z);
  auto rainfall_noise = rainfall_noise_at(world, pos.x, pos.z);
  auto kind = biome_noise_to_kind_at_point(temp_noise, rainfall_noise);
  auto it = world.biomes_by_kind.find(kind);
  if (it == world.biomes_by_kind.end()) {
    logger::error(
        fmt::format("Couldn't find biome info for biome with id={}\n", kind));
    return it->second;
  }
  return it->second;
}

const char *get_biome_name_at(World &world, WorldPos pos) {
  const auto &biome = biome_at_point(world, pos);
  return biome.name;
}

inline float noise_for_biome_at_point(World &world, Biome &biome, int x,
                                      int y) {
  double frequency = 1.0;
  const double fx = NOISE_PICTURE_WIDTH / frequency;
  const double fy = NOISE_PICTURE_WIDTH / frequency;
  auto ng = biome.noise;
  auto noise = ng.fractal(8, (float)x / fx, (float)y / fy);
  return noise;
}

void gen_column_at(World &world, Block *output, int x, int y) {
  auto temp_noise = temperature_noise_at(world, x, y);
  auto rainfall_noise = rainfall_noise_at(world, x, y);
  auto kind = biome_noise_to_kind_at_point(temp_noise, rainfall_noise);
  auto bk = world.biomes_by_kind[kind];
  // auto biome_height_noise = bk.noise.fractal(16, x, y);
  auto noise = world.height_noise.fractal(16, x, y);
  int maxHeight = bk.maxHeight;
  int columnHeight = (noise + 1.0) * ((float)maxHeight / 2.0);
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
    default:
      return false;
  }
}

void build_oak_tree_at(World &world, Chunk &chunk, int x, int y) {
  auto col = &CHUNK_COL_AT(chunk, x, y);
  auto topBlockHeight = get_col_height(col);
  if (!can_tree_grow_on(col[topBlockHeight].type)) {
    return;
  }
  auto r = world.tree_noise();
  auto has_tree_center_here = r < 0.02;
  if (has_tree_center_here) {
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
    u32 crownHeight = round(map(0.0f, 1.0f, CROWN_MIN_HEIGHT, CROWN_MAX_HEIGHT,
                                world.tree_noise()));
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
            auto needBlockHere =
                noise > unlikelinessOfHavingLeavesBasedOnRadius;
            if (needBlockHere) {
              CHUNK_AT(chunk, cx, cy, crownBottom).type = BlockType::Leaves;
            }
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
      auto biome = biome_at_point(world, pos);
      switch (biome.kind) {
        case BiomeKind::Forest: {
          build_oak_tree_at(world, chunk, x, y);
        } break;
      }
    }
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
        int top = IS_TB(CHUNK_AT(chunk, x, y, min(H - 1, height + 1)));
        int bottom = IS_TB(CHUNK_AT(chunk, x, y, max(0, height - 1)));

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
  if (chunk.vao == 0) {
    // Configure the VAO
    glGenVertexArrays(1, &chunk.vao);
    glGenBuffers(1, &chunk.buffer);
#ifdef VAO_ALLOCATION
    fmt::print("Allocating VAO={}\n", chunk.vao);
#endif
  }
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
    auto chunk_key = it->first;
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

void chunk_modify_block_at_global(Chunk *chunk, WorldPos pos, BlockType type) {
  auto local_pos = chunk_global_to_local_pos(chunk, pos);
  Block b;
  b.type = type;
  chunk->blocks[local_pos.x][local_pos.y][local_pos.z] = b;
  chunk->is_dirty = true;
}

void place_block_at(World &world, BlockType type, WorldPos pos) {
  // determine which chunk is affected
  Chunk *chunk = nullptr;
  for (auto &ch : world.chunks) {
    auto pos_inside = (pos.x > ch->x && pos.x < ch->x + CHUNK_WIDTH) &&
                      (pos.y > ch->y && pos.y < ch->y + CHUNK_LENGTH);
    if (pos_inside) chunk = ch;
  }
  if (chunk == nullptr) {
    fmt::print(
        "Failed to determine which chunk to place the block in. Placement "
        "coords: {}, {}, {}\n",
        pos.x, pos.y, pos.z);
    return;
  }
  chunk_modify_block_at_global(chunk, pos, type);
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
    default: {
      return glm::ivec3(255, 255, 0);
    }
  }
}

constexpr int NM_W = 1024;
constexpr int NM_H = 1024;

Color block_kind_color(BlockType bt) {
  if (auto atlas_image = image_storage::get_image("block")) {
    auto offsetf = block_type_texture_offset(bt);
    auto w = atlas_image->width;
    auto h = atlas_image->height;
    auto offset = glm::ivec2(offsetf.x * (float)w, offsetf.y * (float)h);
    auto pix = (*atlas_image)(offset.x, offset.y);
    return pix;
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

void world_dump_heights(World &world) {
  int width, height, nrChannels;
  unsigned char *texture =
      stbi_load("./images/texture.png", &width, &height, &nrChannels, 0);
  if (!texture) {
    fmt::print("Failed to load texture\n");
    return;
  }

  // biome noise map
  {
    fmt::print("Generating a biome noise map dump...\n");
    auto *whm = new array<array<Pixel, NM_W>, NM_H>();
    auto *biome_kind_map = new array<array<Pixel, NM_W>, NM_H>();
    auto *world_height_map = new array<array<Pixel, NM_W>, NM_H>();
    auto *ortho_view = new array<array<Pixel, NM_W>, NM_H>();
    fmt::print("Calculating noise...\n");
    Block col[CHUNK_HEIGHT];
    for (int x = 0; x < NM_W; ++x) {
      for (int y = 0; y < NM_H; ++y) {
        gen_column_at(world, col, x, y);
        uint32_t height = get_col_height(col);
        Block topBlock = col[height];

        // full map ortho view
        {
          auto c = block_kind_color(topBlock.type);
          (*ortho_view)[x][y] = c;
        }

        // rainfall noise
        {
          auto biome_noise = rainfall_noise_at(world, x, y);
          int noise_value = round(biome_noise * 256);
          int r = noise_value;
          int g = noise_value;
          int b = noise_value;
          int a = 255;
          (*whm)[x][y] = rgba_color(byte(r), byte(g), byte(b), byte(a));
        }

        // temperature noise
        {
          auto biome_noise = temperature_noise_at(world, x, y);
          int noise_value = round(biome_noise * 256);
          int r = noise_value;
          int g = noise_value;
          int b = noise_value;
          int a = 255;
          (*whm)[x][y] = rgba_color(byte(r), byte(g), byte(b), byte(a));
        }

        auto temp_noise = temperature_noise_at(world, x, y);
        auto rainfall_noise = rainfall_noise_at(world, x, y);
        auto kind = biome_noise_to_kind_at_point(temp_noise, rainfall_noise);
        auto bk = world.biomes_by_kind[kind];

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
          auto noise = noise_for_biome_at_point(world, bk, x, y);
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
    fmt::print("Writing the images...\n");
    stbi_write_png("temp/world_biome_noise.png", NM_W, NM_H, 4, whm->data(), 0);
    stbi_write_png("temp/biome_kind.png", NM_W, NM_H, 4, biome_kind_map->data(),
                   0);
    stbi_write_png("temp/heightmap.png", NM_W, NM_H, 4,
                   world_height_map->data(), 0);
    stbi_write_png("temp/ortho.png", NM_W, NM_H, 4, ortho_view->data(), 0);
    fmt::print("Done.\n");
    delete whm;
  }
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
