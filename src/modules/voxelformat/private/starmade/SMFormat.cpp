/**
 * @file
 */

#include "SMFormat.h"
#include "core/ArrayLength.h"
#include "core/Bits.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/Map.h"
#include "io/Archive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/ZipArchive.h"
#include "io/ZipReadStream.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/Voxel.h"
#include "palette/Palette.h"

// https://starmadepedia.net/wiki/ID_list
#include "SMPalette.h"

namespace voxelformat {

namespace priv {
constexpr int segments = 16;
constexpr int maxSegments = segments * segments * segments;
constexpr int segmentHeaderSize = 26;
constexpr int blocks = 32;
constexpr int maxSegmentDataCompressedSize = ((blocks * blocks * blocks) * 3 / 2) - segmentHeaderSize;
constexpr int planeBlocks = blocks * blocks;

} // namespace priv

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " CORE_FILE ":%i", CORE_LINE);                                  \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " CORE_FILE ":%i", CORE_LINE);                                  \
		return false;                                                                                                  \
	}

static bool readIvec3(io::SeekableReadStream &stream, glm::ivec3 &v) {
	if (stream.readInt32BE(v.x) == -1) {
		return false;
	}
	if (stream.readInt32BE(v.y) == -1) {
		return false;
	}
	if (stream.readInt32BE(v.z) == -1) {
		return false;
	}
	return true;
}

static bool readI16vec3(io::SeekableReadStream &stream, glm::i16vec3 &v) {
	if (stream.readInt16BE(v.x) == -1) {
		return false;
	}
	if (stream.readInt16BE(v.y) == -1) {
		return false;
	}
	if (stream.readInt16BE(v.z) == -1) {
		return false;
	}
	return true;
}

static bool readVec3(io::SeekableReadStream &stream, glm::vec3 &v) {
	if (stream.readFloatBE(v.x) == -1) {
		return false;
	}
	if (stream.readFloatBE(v.y) == -1) {
		return false;
	}
	if (stream.readFloatBE(v.z) == -1) {
		return false;
	}
	return true;
}

bool SMFormat::loadGroupsRGBA(const core::String &filename, io::SeekableReadStream &stream,
							  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
							  const LoadContext &ctx) {
	core::Map<int, int> blockPal;
	for (int i = 0; i < lengthof(BLOCKCOLOR); ++i) {
		blockPal.put(BLOCKCOLOR[i].blockId, palette.getClosestMatch(BLOCKCOLOR[i].color));
	}
	const core::String &extension = core::string::extractExtension(filename);
	if (extension == "smd3") {
		return readSmd3(stream, sceneGraph, blockPal, {0, 0, 0}, palette);
	} else if (extension == "smd2") {
		return readSmd2(stream, sceneGraph, blockPal, {0, 0, 0}, palette);
	} else if (extension == "sment") {
		io::ZipArchive archive;
		if (!archive.init(filename, &stream)) {
			Log::error("Failed to load zip archive from %s", filename.c_str());
			return false;
		}
		const io::ArchiveFiles &files = archive.files();
		for (const io::FilesystemEntry &e : files) {
			const core::String &extension = core::string::extractExtension(e.name);
			const bool isSmd3 = extension == "smd3";
			const bool isSmd2 = extension == "smd2";
			if (isSmd2 || isSmd3) {
				// position is encoded in the filename
				// ENTITY_SHIP_Rexio_1686826017103.0.0.0.smd3
				glm::ivec3 position(0);
				size_t dot = 0;
				for (int i = 0; i < 3; ++i) {
					dot = e.name.find_first_of('.', dot);
					if (dot != core::String::npos) {
						position[i] = core::string::toInt(e.name.substr(dot + 1));
					}
				}
				io::BufferedReadWriteStream modelStream((int64_t)e.size);
				if (!archive.load(e.fullPath, modelStream)) {
					Log::warn("Failed to load zip archive entry %s", e.fullPath.c_str());
					continue;
				}
				if (modelStream.seek(0) == -1) {
					Log::error("Failed to seek back to the start of the stream for %s", e.fullPath.c_str());
					continue;
				}
				if (isSmd3) {
					if (!readSmd3(modelStream, sceneGraph, blockPal, position, palette)) {
						Log::warn("Failed to load %s from %s", e.fullPath.c_str(), filename.c_str());
					}
				} else if (isSmd2) {
					if (!readSmd2(modelStream, sceneGraph, blockPal, position, palette)) {
						Log::warn("Failed to load %s from %s", e.fullPath.c_str(), filename.c_str());
					}
				}
			} else if (extension == "smbph") {
				const io::SeekableReadStreamPtr &headerStream = archive.readStream(e.fullPath);
				wrapBool(readHeader(*headerStream.get()))
			} else if (extension == "smbpl") {
				const io::SeekableReadStreamPtr &headerStream = archive.readStream(e.fullPath);
				wrapBool(readLogic(*headerStream.get()))
			} else if (extension == "smbpm") {
				const io::SeekableReadStreamPtr &headerStream = archive.readStream(e.fullPath);
				wrapBool(readMeta(*headerStream.get()))
			}
		}
	}
	return !sceneGraph.empty();
}

bool SMFormat::readHeader(io::SeekableReadStream &stream) const {
	int32_t version;
	wrap(stream.readInt32BE(version))
	// Ship = 0
	// Shop = 1
	// SpaceStation = 2
	// Asteroid = 3
	// Planet = 4
	int32_t entityType;
	wrap(stream.readInt32BE(entityType))
	glm::vec3 lowerBound;
	wrapBool(readVec3(stream, lowerBound))
	glm::vec3 upperBound;
	wrapBool(readVec3(stream, upperBound))
	int32_t blockEntries;
	wrap(stream.readInt32BE(blockEntries))
	for (int32_t i = 0; i < blockEntries; ++i) {
		int16_t blockId;
		wrap(stream.readInt16BE(blockId))
		int32_t blockCount;
		wrap(stream.readInt32BE(blockCount))
	}
	return true;
}

bool SMFormat::readMeta(io::SeekableReadStream &stream) const {
#if 0
	int32_t version;
	wrap(stream.readInt32BE(version))
	uint8_t unknown2;
	wrap(stream.readUInt8(unknown2))
	int32_t dockEntries;
	wrap(stream.readInt32BE(dockEntries))
	for (int32_t i = 0; i < dockEntries; ++i) {
		core::String subfolder;
		wrap(stream.readPascalStringUInt16BE(subfolder))
		glm::ivec3 position;
		wrapBool(readIvec3(stream, position))
		glm::vec3 unknown3;
		wrapBool(readVec3(stream, unknown3))
		int16_t blockId;
		wrap(stream.readInt16BE(blockId))
		uint8_t unknown4;
		wrap(stream.readUInt8(unknown4))
	}
	uint8_t unknown5;
	wrap(stream.readUInt8(unknown5))
#endif
	return true;
}

bool SMFormat::readLogic(io::SeekableReadStream &stream) const {
	int32_t unknown1;
	wrap(stream.readInt32BE(unknown1))
	int32_t controllers;
	wrap(stream.readInt32BE(controllers))
	for (int32_t i = 0; i < controllers; ++i) {
		glm::i16vec3 position;
		wrapBool(readI16vec3(stream, position))
		int32_t numGroups;
		wrap(stream.readInt32BE(numGroups))
		for (int32_t j = 0; j < numGroups; ++j) {
			int16_t blockId;
			wrap(stream.readInt16BE(blockId))
			int32_t numBlocks;
			wrap(stream.readInt32BE(numBlocks))
			for (int32_t k = 0; k < numBlocks; ++k) {
				glm::i16vec3 blockPos;
				wrapBool(readI16vec3(stream, blockPos))
			}
		}
	}
	return true;
}

bool SMFormat::readSmd2(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
						const core::Map<int, int> &blockPal, const glm::ivec3 &position,
						const palette::Palette &palette) {
	uint32_t version;
	wrap(stream.readUInt32BE(version))

	core::Map<uint16_t, uint16_t> segmentsMap;

	for (int i = 0; i < priv::maxSegments; i++) {
		uint16_t segmentId;
		wrap(stream.readUInt16BE(segmentId))
		uint16_t segmentSize;
		wrap(stream.readUInt16BE(segmentSize))
		if (segmentId > 0) {
			Log::debug("segment %i with size: %i", (int)segmentId, (int)segmentSize);
			segmentsMap.put(segmentId, segmentSize);
		}
	}
	for (int i = 0; i < priv::maxSegments; i++) {
		uint64_t timestamp;
		wrap(stream.readUInt64BE(timestamp))
	}
	while (!stream.eos()) {
		if (!readSegment(stream, sceneGraph, blockPal, version, 2, palette)) {
			Log::error("Failed to read segment");
			return false;
		}
	}

	return true;
}

bool SMFormat::readSmd3(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
						const core::Map<int, int> &blockPal, const glm::ivec3 &position,
						const palette::Palette &palette) {
	uint32_t version;
	wrap(stream.readUInt32BE(version))

	core::Map<uint16_t, uint16_t> segmentsMap;

	for (int i = 0; i < priv::maxSegments; i++) {
		uint16_t segmentId;
		wrap(stream.readUInt16BE(segmentId))
		uint16_t segmentSize;
		wrap(stream.readUInt16BE(segmentSize))
		if (segmentId > 0) {
			Log::debug("segment %i with size: %i", (int)segmentId, (int)segmentSize);
			segmentsMap.put(segmentId, segmentSize);
		}
	}
	while (!stream.eos()) {
		if (!readSegment(stream, sceneGraph, blockPal, version, 3, palette)) {
			Log::error("Failed to read segment");
			return false;
		}
	}

	return true;
}

static glm::ivec3 posByIndex(uint32_t blockIndex) {
	const int z = (int)blockIndex / priv::planeBlocks;
	const int divR = (int)blockIndex % priv::planeBlocks;
	const int y = divR / priv::blocks;
	const int x = divR % priv::blocks;
	return glm::ivec3(x, y, z);
}

size_t SMFormat::loadPalette(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette,
							 const LoadContext &ctx) {
	core::Array<core::RGBA, lengthof(BLOCKCOLOR)> colors;
	for (int i = 0; i < lengthof(BLOCKCOLOR); ++i) {
		colors[i] = BLOCKCOLOR[i].color;
	}
	palette.quantize(colors.data(), colors.size());
	return palette.size();
}

bool SMFormat::readSegment(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
						   const core::Map<int, int> &blockPal, int headerVersion, int fileVersion,
						   const palette::Palette &palette) {
	const int64_t startHeader = stream.pos();
	Log::debug("read segment");

	if (headerVersion != 0) {
		uint8_t segmentVersion;
		wrap(stream.readUInt8(segmentVersion))
		Log::debug("segmentVersion: %i", (int)segmentVersion);
	}

	uint64_t timestamp;
	wrap(stream.readUInt64BE(timestamp))

	glm::ivec3 segmentPosition;
	wrapBool(readIvec3(stream, segmentPosition))
	Log::debug("segmentPosition: %i:%i:%i", segmentPosition.x, segmentPosition.y, segmentPosition.z);

	bool hasValidData;
	uint32_t compressedSize;
	if (headerVersion == 0) {
		int32_t dataLength;
		wrap(stream.readInt32BE(dataLength))
		uint8_t segmentType;
		wrap(stream.readUInt8(segmentType))
		hasValidData = dataLength > 0;
		compressedSize = dataLength;
	} else { // Valid as of 0.1867, smd file version 1
		hasValidData = stream.readBool();
		wrap(stream.readUInt32BE(compressedSize))
	}
	Log::debug("hasValidData: %i", (int)hasValidData);

	if (!hasValidData) {
		stream.seek(startHeader + priv::maxSegmentDataCompressedSize);
		return true;
	}

	core_assert(stream.pos() - startHeader == priv::segmentHeaderSize);

	io::ZipReadStream blockDataStream(stream, (int)compressedSize);

	const voxel::Region region(segmentPosition, segmentPosition + (priv::blocks - 1));
	voxel::RawVolume *volume = new voxel::RawVolume(region);

	scenegraph::SceneGraphNode node;
	node.setVolume(volume, true);
	node.setPalette(palette);
	bool empty = true;
	int index = -1;
	while (!blockDataStream.eos()) {
		++index;
		uint8_t buf[3];
		// byte orientation : 3
		// byte isActive: 1
		// byte hitpoints: 9
		// ushort blockId: 11
		wrap(blockDataStream.readUInt8(buf[0]))
		wrap(blockDataStream.readUInt8(buf[1]))
		wrap(blockDataStream.readUInt8(buf[2]))
		const uint32_t blockData = buf[0] | (buf[1] << 8) | (buf[2] << 16);
		if (blockData == 0u) {
			continue;
		}
		const uint32_t blockId = core::bits(blockData, 0, 11);
		if (blockId == 0u) {
			continue;
		}
		// const uint32_t hitpoints = core::bits(blockData, 11, 9);
		// const uint32_t active = core::bits(blockData, 20, 1);
		// const uint32_t orientation = core::bits(blockData, 21, 3);
		auto palIter = blockPal.find((int)blockId);
		uint8_t palIndex = 0;
		if (palIter == blockPal.end()) {
			Log::trace("Skip block id %i", (int)blockId);
		} else {
			palIndex = palIter->value;
		}

		glm::ivec3 pos = segmentPosition + posByIndex(index);

		volume->setVoxel(pos, voxel::createVoxel(palette, palIndex));
		empty = false;
	}

	core_assert(stream.pos() - startHeader == (int)compressedSize + priv::segmentHeaderSize);

	stream.seek(startHeader + priv::maxSegmentDataCompressedSize + priv::segmentHeaderSize);
	if (empty) {
		return true;
	}

	sceneGraph.emplace(core::move(node));

	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
