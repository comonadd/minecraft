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
#include "texture.hpp"

using WorldPos = glm::ivec3;

enum class BlockType : u8 {
  Dirt,
  Grass,
  Stone,
  Water,
  Sand,
  Snow,
  Air,
  Unknown
};

#pragma pack(push, 1)
struct Block {
  BlockType type = BlockType::Air;
};
#pragma pack(pop)

const int CHUNK_LENGTH = 16;
const int CHUNK_WIDTH = CHUNK_LENGTH;
const int CHUNK_HEIGHT = 64;
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
  float ao;
  float light;
};

using ChunkMesh = std::vector<VertexData>;

struct Chunk {
  Block blocks[CHUNK_LENGTH][CHUNK_WIDTH][CHUNK_HEIGHT];
  uint32_t height = 0;

  int x;
  int y;
  // this flag is set to true if any of the blocks in the chunk has been changed
  bool is_dirty = false;

  // contains information on the size of the mesh stored in chunk.VAO
  u32 mesh_size = 0;

  // GL buffers
  GLuint buffer = 0;
  GLuint vao = 0;
};

// A hash function used to hash a pair of any kind
using std::hash;
struct hash_pair {
  template <class T1, class T2>
  size_t operator()(const pair<T1, T2>& p) const {
    auto hash1 = hash<T1>{}(p.first);
    auto hash2 = hash<T2>{}(p.second);
    return hash1 ^ hash2;
  }
};

using ChunkId = pair<int, int>;
inline ChunkId chunk_id_from_coords(int x, int y) { return make_pair(x, y); }

enum BiomeKind {
  Grassland = 0,
  Mountains = 1,
  Desert = 2,
  Ocean = 3,
  Forest = 4
};

struct Biome {
  BiomeKind kind;
  int maxHeight;
  SimplexNoise noise;
  const char* name;
};

const int WATER_LEVEL = 30;

struct World {
  int seed = 3849534;
  std::vector<Chunk*> chunks{};
  std::unordered_map<ChunkId, Chunk*, hash_pair> loaded_chunks;

  siv::PerlinNoise perlin;
  SimplexNoise simplex{0.1f, 1.0f, 2.0f, 0.5f};

  SimplexNoise height_noise{0.005f, 1.0f, 2.0f, 0.5f};
  SimplexNoise biome_noise{0.0005f, 1.0f, 2.0f, 0.5f};

  std::unordered_map<BiomeKind, Biome> biomes_by_kind = {
      // Desert
      {BiomeKind::Desert,
       Biome{
           .kind = BiomeKind::Desert,
           .maxHeight = CHUNK_HEIGHT,
           .noise = SimplexNoise{0.01f, 1.0f, 2.0f, 0.5f},
           .name = "Desert",
       }},

      // Forest
      {BiomeKind::Forest,
       Biome{
           .kind = BiomeKind::Forest,
           .maxHeight = CHUNK_HEIGHT,
           .noise = SimplexNoise{0.03f, 1.0f, 2.0f, 0.5f},
           .name = "Forest",
       }},

      // Grassland
      {BiomeKind::Grassland,
       Biome{
           .kind = BiomeKind::Grassland,
           .maxHeight = CHUNK_HEIGHT,
           .noise = SimplexNoise{0.025f, 1.0f, 2.0f, 0.5f},
           .name = "Grassland",
       }},

      // Oceans
      {BiomeKind::Ocean,
       Biome{
           .kind = BiomeKind::Ocean,
           .maxHeight = WATER_LEVEL,
           .noise = SimplexNoise{0.001f, 1.0f, 2.0f, 0.5f},
           .name = "Ocean",
       }},

      // Mountains
      {BiomeKind::Mountains,
       Biome{
           .kind = BiomeKind::Mountains,
           .maxHeight = CHUNK_HEIGHT,
           .noise = SimplexNoise{1.00f, 1.0f, 2.0f, 0.5f},
           .name = "Mountains",
       }},
  };
};

void load_chunks_around_player(World& world, WorldPos center_pos,
                               uint32_t radius);

void place_block_at(World& world, BlockType type, WorldPos pos);

Block chunk_get_block_at_global(Chunk* chunk, WorldPos pos);

void world_dump_heights(World& world);

const char* get_biome_name_at(World& world, WorldPos pos);

void foreach_col_in_chunk(Chunk& chunk, std::function<void(int, int)> fun);

void calculate_minimap_tex(Texture& texture, World& world, WorldPos pos,
                           u32 radius);

#endif
