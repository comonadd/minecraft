#ifndef WORLD_HPP
#define WORLD_HPP

#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <random>
#include <set>
#include <vector>

#include "block.hpp"
#include "noise.hpp"
#include "shaders.hpp"
#include "texture.hpp"

using WorldPos = glm::ivec3;

const int CHUNK_LENGTH = 16;
const int CHUNK_WIDTH = CHUNK_LENGTH;
const int CHUNK_HEIGHT = 128;
const int BLOCKS_OF_AIR_ABOVE = 20;

extern float BLOCK_WIDTH;
extern float BLOCK_LENGTH;
extern float BLOCK_HEIGHT;

// oak tree
constexpr u32 MIN_TREE_HEIGHT = 6;
constexpr u32 MAX_TREE_HEIGHT = 10;
constexpr u32 CROWN_MIN_HEIGHT = 3;
constexpr u32 CROWN_MAX_HEIGHT = 5;
constexpr u32 TREE_MIN_RADIUS = 2;
constexpr u32 TREE_MAX_RADIUS = 4;

constexpr u32 MAX_JUNGLE_TREE_HEIGHT = 20;
constexpr u32 MIN_JUNGLE_TREE_HEIGHT = 2;
constexpr u32 JUNGLE_TREE_MIN_RADIUS = 4;
constexpr u32 JUNGLE_TREE_MAX_RADIUS = 6;
constexpr u32 JUNGLE_CROWN_MAX_HEIGHT = 6;
constexpr u32 JUNGLE_CROWN_MIN_HEIGHT = 2;

// pine tree
constexpr u32 MIN_PINE_TREE_HEIGHT = 8;
constexpr u32 MAX_PINE_TREE_HEIGHT = 16;
constexpr u32 PINE_CROWN_MIN_HEIGHT = 7;
constexpr u32 PINE_CROWN_MAX_HEIGHT = 18;
constexpr u32 PINE_TREE_MIN_RADIUS = 1;
constexpr u32 PINE_TREE_MAX_RADIUS = 4;

constexpr vec3 rainyDayColor = vec3(0.765625, 0.8671875, 0.90625);
constexpr vec3 colorDay =
    vec3(135.0f / 256.0f, 206.0f / 256.0f, 250.0f / 256.0f);
constexpr vec3 colorNight = vec3(0.0, 0.0, 0.0);

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
  Grassland,
  Mountains,
  Desert,
  Ocean,
  Tundra,
  Taiga,
  Forest,
  Coast,
  Jungle,
  Count,
};

struct World;

struct Biome {
  BiomeKind kind;
  int maxHeight;
  OpenSimplexNoiseWParam noise;
  float treeFrequency;
  std::function<void(World& world, Chunk& chunk, i32 x, i32 y)> treeGen;
  const char* name;
};

const int WATER_LEVEL = (int)(CHUNK_HEIGHT * 0.25);
const int TICKS_PER_SECOND = 100;

// in ticks (12 minutes)
const int DAY_DURATION = TICKS_PER_SECOND * 60 * 12;
const float ONE_HOUR = (float)DAY_DURATION / 24.0f;
const float MORNING = 4.0f * ONE_HOUR;
const float DAY = 12.0f * ONE_HOUR;
const float EVENING = 18.0f * ONE_HOUR;
const float NIGHT = 22.0f * ONE_HOUR;
const float ONE_MINUTE = ONE_HOUR / 60.0f;

struct Atom {
  WorldPos pos;
  Block block;
};

struct World {
  int seed = 3849534;
  std::vector<Chunk*> chunks{};
  std::unordered_map<ChunkId, Chunk*, hash_pair> loaded_chunks;

  std::vector<Atom> changes;

  std::random_device rd;
  std::default_random_engine eng;
  std::uniform_real_distribution<float> _tree_gen{FLOAT_MIN, FLOAT_MAX};

  inline float tree_noise() { return this->_tree_gen(this->eng); }

  // SimplexNoise height_noise{0.005f, 1.0f, 2.0f, 0.5f};
  OpenSimplexNoiseWParam height_noise{0.00025f, 64.0f, 2.0f, 0.6f, 345972};

  // TODO: Use different seed
  OpenSimplexNoiseWParam rainfall_noise{0.0005f, 2.0f, 3.0f, 0.5f, 834952};
  OpenSimplexNoiseWParam temperature_noise{0.00075f, 1.0f, 2.0f, 0.5f, 123871};

  glm::vec4 sun_pos{0.0f, 100.0f, 0.0f, 0.0f};
  // ticks passed since the beginning of the world
  u64 time = 0;
  // time of day [0..DAY_DURATION];
  u32 time_of_day = DAY;

  vec3 origin{0, 0, 0};

  // updated on each frame
  bool is_day = false;

  float celestial_size = 2.5f;
  vec3 sky_color;
  float fog_gradient = 6.0f;
  float fog_density = 0.007f;
  bool fog_enabled = true;

  // this is non-null when the player has a target block
  optional<WorldPos> target_block_pos;
  optional<Block> target_block;

  std::unordered_map<BiomeKind, Biome> biomes_by_kind;
  World() { this->eng = std::default_random_engine(this->seed); }
};

void load_chunks_around_player(World& world, WorldPos center_pos,
                               uint32_t radius);
void place_block_at(World& world, BlockType type, WorldPos pos);
Block chunk_get_block_at_global(Chunk* chunk, WorldPos pos);
void world_dump_heights(World& world, const string& out_dir);
const char* get_biome_name_at(World& world, WorldPos pos);
void foreach_col_in_chunk(Chunk& chunk, std::function<void(int, int)> fun);
void calculate_minimap_tex(Texture& texture, World& world, WorldPos pos,
                           u32 radius);
void unload_chunk(Chunk* chunk);
void unload_distant_chunks(World& world, WorldPos pos, u32 rendering_distance);
void init_world(World& world);
optional<Block> get_block_at_global_pos(World& world, WorldPos pos);
void init_world(World& world, Seed seed);
void world_update(World& world, float dt);

#endif
