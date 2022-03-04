/**
 * @file
 */

#pragma once

#include "Format.h"

namespace io {
class ZipReadStream;
}

namespace voxel {

/**
 * A minecraft chunk contains the terrain and entity information about a grid of the size 16x256x16
 *
 * A section is 16x16x16 and a chunk contains max 16 sections. Section 0 is the bottom, section 15 is the top
 *
 * @note This is stored in NBT format
 *
 * older version:
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
 * newer version
 * the block_states are under a sections compound
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
 * @note https://minecraft.fandom.com/wiki/Region_file_format
 * @note https://minecraft.fandom.com/wiki/Chunk_format
 * @note https://minecraft.fandom.com/wiki/NBT_format
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
		/**
		 * @brief Marks the end of compound tags. There is no name for this tag. It may also be used for empty List tags.
		 */
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
		LONG_ARRAY = 12,

		MAX
	};

	struct NamedBinaryTag {
		core::String name;
		TagId id = TagId::END;
		TagId listId = TagId::END;
		uint32_t listItems = 0;
		int level = 0;
	};

	struct Offsets {
		uint32_t offset;
		uint8_t sectorCount;
	} _offsets[SECTOR_INTS];
	uint32_t _chunkTimestamps[SECTOR_INTS];

	void reset();

	bool skip(io::ZipReadStream &stream, NamedBinaryTag &nbt, bool marker);
	bool getNext(io::ReadStream &stream, NamedBinaryTag &nbt);

	// DataVersion >= 2844
	bool parseSections(SceneGraph &sceneGraph, io::ZipReadStream &stream, NamedBinaryTag &nbt);
	bool parseBlockStates(SceneGraph &sceneGraph, io::ZipReadStream &stream, NamedBinaryTag &nbt);
	bool parsePalette(SceneGraph &sceneGraph, io::ZipReadStream &stream, NamedBinaryTag &nbt);
	bool parseData(SceneGraph &sceneGraph, io::ZipReadStream &stream, NamedBinaryTag &nbt);

	bool parseNBTChunk(SceneGraph& sceneGraph, io::ZipReadStream &stream);
	bool readCompressedNBT(SceneGraph& sceneGraph, io::SeekableReadStream &stream);
	bool loadMinecraftRegion(SceneGraph& sceneGraph, io::SeekableReadStream &stream, int chunkX, int chunkZ);
public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override {
		reset();
		return false;
	}
};

}
