/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxel {

/**
 * A minecraft chunk contains the terrain and entity information about a grid of the size 16x256x16
 *
 * A section is 16x16x16 and a chunk contains max 16 sections. Section 0 is the bottom, section 15 is the top
 *
 * @note This is stored in NBT format
 *
 * @code
 * root tag (compound)
 *   \-- DataVersion - version of the nbt chunk
 *   \-- Level - chunk data (compound)
 *     \-- xPos - x pos in chunk relative to the origin (not the region)
 *     \-- yPos - y pos in chunk relative to the origin (not the region)
 *     \-- Sections (list)
 *       \-- section (compound)
 *         \-- Y: Range 0 to 15 (bottom to top) - if empty, section is empty
 *         \-- Palette
 *         \-- BlockLight - 2048 bytes
 *         \-- BlockStates
 *         \-- SkyLight
 * @endcode
 *
 * @code
 * byte Nibble4(byte[] arr, int index) {
 *   return index%2 == 0 ? arr[index/2]&0x0F : (arr[index/2]>>4)&0x0F;
 * }
 * int BlockPos = y*16*16 + z*16 + x;
 * compound Block = Palette[change_array_element_size(BlockStates,Log2(length(Palette)))[BlockPos]]
 * string BlockName = Block.Name;
 * compound BlockState = Block.Properties;
 * byte Blocklight = Nibble4(BlockLight, BlockPos);
 * byte Skylight = Nibble4(SkyLight, BlockPos);
 * @endcode
 *
 * @note https://github.com/Voxtric/Minecraft-Level-Ripper/blob/master/WorldConverterV2/Processor.cs
 */
class MCRFormat : public Format {
private:
	static constexpr int VERSION_GZIP = 1;
	static constexpr int VERSION_DEFLATE = 2;
	static constexpr int SECTOR_BYTES = 4096;
	static constexpr int SECTOR_INTS = SECTOR_BYTES / 4;
	static constexpr int CHUNK_HEADER_SIZE = 5;
	static constexpr int MAX_LEVELS = 512;

	enum class TagId : uint8_t {
		END = 0,
		BYTE = 1,
		SHORT = 2,
		INT = 3,
		LONG = 4,
		FLOAT = 5,
		DOUBLE = 6,
		BYTE_ARRAY = 7,
		STRING = 8,
		LIST = 9,
		COMPOUND = 10,
		INT_ARRAY = 11,
		LONG_ARRAY = 12
	};

	struct NamedBinaryTag {
		core::String name;
		TagId id = TagId::END;
		int level = 0;
	};

	struct Offsets {
		uint32_t offset : 24;
		uint32_t sectorCount : 8;
	} _offsets[SECTOR_INTS];
	uint32_t _chunkTimestamps[SECTOR_INTS];

	void reset();

	bool skip(io::SeekableReadStream &stream, TagId id);
	bool getNext(io::SeekableReadStream &stream, NamedBinaryTag& nbt);

	bool parseNBTChunk(SceneGraph& sceneGraph, const uint8_t* buffer, int length);
	bool readCompressedNBT(SceneGraph& sceneGraph, const uint8_t* buffer, int length, io::SeekableReadStream &stream);
	bool loadMinecraftRegion(SceneGraph& sceneGraph, const uint8_t* buffer, int length, io::SeekableReadStream &stream, int chunkX, int chunkZ);
public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override {
		reset();
		return false;
	}
};

}
