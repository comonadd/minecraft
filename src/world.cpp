#include "world.hpp"

#include <GL/glew.h>
#include <fmt/core.h>

#include <glm/glm.hpp>

#include "PerlinNoise/PerlinNoise.hpp"

float BLOCK_WIDTH = 2.0f;
float BLOCK_LENGTH = BLOCK_WIDTH;
float BLOCK_HEIGHT = BLOCK_WIDTH;

int round_to_nearest_16(int number) {
  int result = abs(number) + 16 / 2;
  result -= result % 16;
  result *= number > 0 ? 1 : -1;
  return result;
}

inline shared_ptr<Chunk> is_chunk_loaded(World &world, int x, int y) {
  auto ch = world.loaded_chunks.find(chunk_id_from_coords(x, y));
  bool loaded = ch != world.loaded_chunks.end();
  // fmt::print("{},{} is loaded: {}\n", x, y, loaded);
  return loaded ? ch->second : nullptr;
}

inline BlockType block_type_for_height(int height) {
  // fmt::print("height: {}\n", height);
  int top = CHUNK_HEIGHT - BLOCKS_OF_AIR_ABOVE;
  if (height > (top - 6)) {
    return BlockType::Grass;
  } else if (height > (top - 20)) {
    return BlockType::Dirt;
  } else if (height >= 0) {
    return BlockType::Stone;
  }
  return BlockType::Unknown;
}

inline glm::vec2 uv_for_block_type(BlockType bt) {
  // TODO: Implement this
  return {0.0, 0.0};
}

inline WorldPos chunk_global_to_local_pos(shared_ptr<Chunk> chunk,
                                          WorldPos pos) {
  return glm::ivec3(pos.x - chunk->x, pos.y - chunk->y, pos.z);
}

inline Block chunk_get_block(shared_ptr<Chunk> chunk, glm::ivec3 local_pos) {
  return chunk->blocks[local_pos.x][local_pos.y][local_pos.z];
}

inline Block chunk_get_block_at_global(shared_ptr<Chunk> chunk, WorldPos pos) {
  auto local_pos = chunk_global_to_local_pos(chunk, pos);
  return chunk_get_block(chunk, local_pos);
}

int TEXTURE_TILE_HEIGHT = 16;
int TEXTURE_TILE_WIDTH = 16;
int TEXTURE_WIDTH = 256;
int TEXTURE_HEIGHT = TEXTURE_WIDTH;
int TEXTURE_ROWS = TEXTURE_HEIGHT / TEXTURE_TILE_HEIGHT;

glm::vec2 block_type_texture_offset(BlockType bt) {
  size_t block_id = (size_t)bt;
  size_t pixel_offset = block_id * TEXTURE_TILE_WIDTH;
  size_t real_x = pixel_offset % TEXTURE_WIDTH;
  size_t real_y_row = floor(pixel_offset / TEXTURE_WIDTH);
  glm::vec2 res{(float)real_x / (float)TEXTURE_WIDTH,
                (float)real_y_row / (float)TEXTURE_ROWS};
  return res;
}

void make_cube_faces(ChunkMesh &mesh, int left, int right, int top, int bottom,
                     int front, int back, int wleft, int wright, int wtop,
                     int wbottom, int wfront, int wback, float x, float y,
                     float z, float n, BlockType block_type) {
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
      {{0, 0}, {1, 0}, {0, 1}, {1, 1}}, {{1, 0}, {0, 0}, {1, 1}, {0, 1}},
      {{0, 1}, {0, 0}, {1, 1}, {1, 0}}, {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
      {{0, 0}, {0, 1}, {1, 0}, {1, 1}}, {{1, 0}, {1, 1}, {0, 0}, {0, 1}}};
  static const float indices[6][6] = {{0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3},
                                      {0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3},
                                      {0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3}};
  static const float flipped[6][6] = {{0, 1, 2, 1, 3, 2}, {0, 2, 1, 2, 3, 1},
                                      {0, 1, 2, 1, 3, 2}, {0, 2, 1, 2, 3, 1},
                                      {0, 1, 2, 1, 3, 2}, {0, 2, 1, 2, 3, 1}};
  float s = 0.0625;
  float a = 0 + 1 / 2048.0;
  float b = s - 1 / 2048.0;
  int faces[6] = {left, right, top, bottom, front, back};
  int tiles[6] = {wleft, wright, wtop, wbottom, wfront, wback};
  for (int i = 0; i < 6; i++) {
    if (faces[i] == 0) {
      continue;
    }
    float du = (tiles[i] % TEXTURE_TILE_WIDTH) * s;
    float dv = (tiles[i] / TEXTURE_TILE_HEIGHT) * s;
    // int flip = ao[i][0] + ao[i][3] > ao[i][1] + ao[i][2];
    int flip = 1;
    for (int v = 0; v < 6; v++) {
      VertexData vd;
      int j = flip ? flipped[i][v] : indices[i][v];
      auto xpos = positions[i][j][0];
      vd.pos.x = x + n * xpos;
      auto ypos = positions[i][j][1];
      vd.pos.y = y + n * ypos;
      auto zpos = positions[i][j][2];
      vd.pos.z = z + n * zpos;
      vd.normal.x = normals[i][0];
      vd.normal.y = normals[i][1];
      vd.normal.z = normals[i][2];
      vd.uv = {
          texture_offset.x + (uvs[i][j][0] ? b : a),  // x
          texture_offset.y + (uvs[i][j][1] ? b : a)   // y
      };
      // vd.uv.r = dv + (uvs[i][j][1] ? b : a);
      mesh.push_back(vd);
    }
  }
}

double NOISE_PICTURE_WIDTH = CHUNK_WIDTH * 2;

void load_chunk_at(World &world, int chunk_x, int chunk_y, Chunk &chunk,
                   Attrib block_attrib) {
  chunk.mesh = {};

  fmt::print("Loading chunk at {}, {}\n", chunk_x, chunk_y);
  chunk.x = chunk_x;
  chunk.y = chunk_y;

  double frequency = 1.0;
  const double fx = NOISE_PICTURE_WIDTH / frequency;
  const double fy = NOISE_PICTURE_WIDTH / frequency;

  // blocks
  for (int local_chunk_x = 0; local_chunk_x < CHUNK_WIDTH; ++local_chunk_x) {
    int global_x = chunk.x + local_chunk_x;

    for (int local_chunk_y = 0; local_chunk_y < CHUNK_LENGTH; ++local_chunk_y) {
      int global_y = chunk.y + local_chunk_y;

      auto noise =
          world.simplex.fractal(8, (float)global_x / fx, (float)global_y / fy);

      int maxHeight = CHUNK_HEIGHT;
      int columnHeight = (noise + 1.0) * ((float)maxHeight / 2.0);

      // fmt::print("Noise at x={}, y={}: {}, determinedHeight={}\n", global_x,
      //            global_y, noise, columnHeight);

      for (int height = columnHeight; height >= 0; --height) {
        Block block;
        block.type = block_type_for_height(height);
        chunk.blocks[local_chunk_y][local_chunk_x][height] = block;

        // mesh
        int left = 1;
        int right = 1;
        int top = 1;
        int bottom = 1;
        int front = 1;
        int back = 1;
        int wleft = 0;
        int wright = 0;
        int wtop = 0;
        int wbottom = 0;
        int wfront = 0;
        int wback = 0;
        float n = 0.5;  // scaling
        make_cube_faces(chunk.mesh, left, right, top, bottom, front, back,
                        wleft, wright, wtop, wbottom, wfront, wback,
                        (float)global_x, (float)height, (float)global_y, n,
                        block.type);
      }
    }
  }

  glGenBuffers(1, &chunk.buffer);
  glBindBuffer(GL_ARRAY_BUFFER, chunk.buffer);

  // we've regenerated the chunk mesh
  chunk.is_dirty = false;

  // so update the chunk buffer
  auto mesh_size = sizeof(chunk.mesh[0]) * chunk.mesh.size();
  auto *meshp = &chunk.mesh[0];

  glGenVertexArrays(1, &chunk.VAO);
  glBindVertexArray(chunk.VAO);

  glBufferData(GL_ARRAY_BUFFER, mesh_size, meshp, GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void chunk_modify_block_at_global(shared_ptr<Chunk> chunk, WorldPos pos,
                                  BlockType type) {
  auto local_pos = chunk_global_to_local_pos(chunk, pos);
  Block b;
  b.type = type;
  chunk->blocks[local_pos.x][local_pos.y][local_pos.z] = b;
  chunk->is_dirty = true;
}

void place_block_at(World &world, BlockType type, WorldPos pos) {
  // determine which chunk is affected
  shared_ptr<Chunk> chunk = nullptr;
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

void load_chunks_around_player(World &world, WorldPos center_pos) {
  int center_x = center_pos.x;
  int center_y = center_pos.z;

  world.chunks = {};

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
        loaded_ch = make_shared<Chunk>(Chunk());
        load_chunk_at(world, chunk_x, chunk_y, *loaded_ch, world.block_attrib);
        world.loaded_chunks.insert(
            {chunk_id_from_coords(chunk_x, chunk_y), loaded_ch});
      } else if (loaded_ch->is_dirty) {
        load_chunk_at(world, chunk_x, chunk_y, *loaded_ch, world.block_attrib);
      } else {
        // no need to regenerate
      }

      world.chunks.push_back(loaded_ch);

      ++chunk_idx;
    }
  }
}
