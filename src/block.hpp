#ifndef BLOCK_HPP
#define BLOCK_HPP

#include "constants.hpp"

#define TCOORD(__x, __y)                             \
  (__y) + static_cast<int>(static_cast<float>(__x) / \
                           static_cast<float>(TEXTURE_TILE_WIDTH))

enum class BlockType : u8 {
  Dirt = TCOORD(0, 0),
  Grass = TCOORD(16, 0),
  Stone = TCOORD(32, 0),
  Water = TCOORD(48, 0),
  Sand = TCOORD(64, 0),
  Snow = TCOORD(80, 0),
  Air = TCOORD(96, 0),

  Leaves = TCOORD(112, 0),
  PineTreeLeaves = TCOORD(128, 0),

  TopGrass = TCOORD(0, 208),
  Wood = TCOORD(16, 208),
  TopSnow = TCOORD(32, 208),
  PineWood = TCOORD(48, 208),

  Unknown
};

inline std::string const& get_block_name(BlockType block_type) {
  static unordered_map<BlockType, std::string> block_name{
      {BlockType::Dirt, "Dirt"},       {BlockType::Air, "Air"},
      {BlockType::Grass, "Grass"},     {BlockType::TopGrass, "TopGrass"},
      {BlockType::Snow, "Snow"},       {BlockType::TopSnow, "TopSnow"},
      {BlockType::Wood, "Wood"},       {BlockType::Water, "Water"},
      {BlockType::Unknown, "Unknown"},
  };
  auto it = block_name.find(block_type);
  if (it != block_name.end()) return it->second;
  return block_name[BlockType::Unknown];
}

#pragma pack(push, 1)
struct Block {
  BlockType type = BlockType::Unknown;
};
#pragma pack(pop)

#endif
