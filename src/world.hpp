#ifndef WORLD_HPP
#define WORLD_HPP

#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <set>
#include <vector>

#include "PerlinNoise/PerlinNoise.hpp"
#include "SimplexNoise/src/SimplexNoise.h"
#include "shaders.hpp"

using WorldPos = glm::ivec3;

enum class BlockType { Dirt, Grass, Stone, Water, Sand, Snow, Air, Unknown };

struct Block {
  BlockType type = BlockType::Air;
};

const int CHUNK_LENGTH = 16;
const int CHUNK_WIDTH = CHUNK_LENGTH;
const int CHUNK_HEIGHT = 64;
const float radius = 16;
const int BLOCKS_OF_AIR_ABOVE = 20;

extern float BLOCK_WIDTH;
extern float BLOCK_LENGTH;
extern float BLOCK_HEIGHT;

using std::make_pair;
using std::make_shared;
using std::shared_ptr;

struct VertexData {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
};

using ChunkMesh = std::vector<VertexData>;

struct Chunk {
  Block blocks[CHUNK_LENGTH][CHUNK_WIDTH][CHUNK_HEIGHT];
  uint32_t height = 0;
  ChunkMesh mesh;

  int x;
  int y;
  // this flag is set to true if any of the blocks in the chunk has been changed
  bool is_dirty = false;

  // array buffer
  GLuint buffer = 0;

  GLuint VAO = 0;
};

using ChunkId = long;

// Uses Cantor pairing function for mapping two integers to one
inline ChunkId chunk_id_from_coords(int x, int y) {
  return ((x + y) * (x + y + 1) / 2) + y;
}

struct World {
  int seed = 3849534;
  std::vector<shared_ptr<Chunk>> chunks{(size_t)(4 * (radius * radius))};
  std::unordered_map<ChunkId, std::shared_ptr<Chunk>> loaded_chunks;

  Attrib block_attrib;
  siv::PerlinNoise perlin;
  SimplexNoise simplex{0.1f, 1.0f, 2.0f, 0.5f};
};

void load_chunks_around_player(World& world, WorldPos center_pos);

void place_block_at(World& world, BlockType type, WorldPos pos);

Block chunk_get_block_at_global(Chunk* chunk, WorldPos pos);

#endif
