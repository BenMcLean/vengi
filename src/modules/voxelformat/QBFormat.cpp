/**
 * @file
 */

#include "QBFormat.h"
#include "core/Enum.h"
#include "core/Zip.h"
#include "core/Color.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "io/Stream.h"

namespace voxel {

namespace {
const int RLE_FLAG = 2;
const int NEXT_SLICE_FLAG = 6;
}

#define wrapSave(write) \
	if ((write) == false) { \
		Log::error("Could not save qb file: " CORE_STRINGIFY(write) " failed"); \
		return false; \
	}

#define wrapSaveColor(color) \
	wrapSave(stream.writeByte((color).x)) \
	wrapSave(stream.writeByte((color).y)) \
	wrapSave(stream.writeByte((color).z)) \
	wrapSave(stream.writeByte((color).w))


#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load qb file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) == false) { \
		Log::error("Could not load qb file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

#define wrapColor(read) \
	if ((read) != 0) { \
		Log::error("Could not load qb file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return voxel::Voxel(); \
	}

#define setBit(val, index) val &= (1 << (index))

bool QBFormat::saveMatrix(io::FileStream& stream, const VoxelVolume& volume) const {
	if (volume.volume == nullptr) {
		Log::error("Invalid volume given");
		return false;
	}
	const int nameLength = volume.name.size();
	wrapSave(stream.writeByte(nameLength));
	wrapSave(stream.writeString(volume.name, false));

	const voxel::Region& region = volume.volume->region();
	if (!region.isValid()) {
		Log::error("Invalid region");
		return false;
	}
	const glm::ivec3 size = region.getDimensionsInVoxels();
	wrapSave(stream.writeInt(size.x));
	wrapSave(stream.writeInt(size.y));
	wrapSave(stream.writeInt(size.z));

	wrapSave(stream.writeInt(region.getLowerX()));
	wrapSave(stream.writeInt(region.getLowerY()));
	wrapSave(stream.writeInt(region.getLowerZ()));

	constexpr voxel::Voxel Empty;
	const glm::ivec4 EmptyColor(0);

	const glm::ivec3& mins = region.getLowerCorner();
	const glm::ivec3& maxs = region.getUpperCorner();

	glm::ivec4 currentColor = EmptyColor;
	int count = 0;

	for (int z = mins.z; z <= maxs.z; ++z) {
		for (int y = mins.y; y <= maxs.y; ++y) {
			for (int x = mins.x; x <= maxs.x; ++x) {
				const Voxel& voxel = volume.volume->voxel(x, y, z);
				glm::ivec4 newColor;
				if (voxel == Empty) {
					newColor = EmptyColor;
					Log::trace("Save empty voxel: x %i, y %i, z %i", x, y, z);
				} else {
					const glm::vec4& voxelColor = getColor(voxel);
					const uint8_t red = voxelColor.r * 255.0f;
					const uint8_t green = voxelColor.g * 255.0f;
					const uint8_t blue = voxelColor.b * 255.0f;
					const uint8_t alpha = voxelColor.a * 255.0f;
					newColor = glm::ivec4(red, green, blue, alpha);
					Log::trace("Save voxel: x %i, y %i, z %i (color: index(%i) => rgba(%i:%i:%i:%i))",
							x, y, z, (int)voxel.getColor(), (int)red, (int)green, (int)blue, (int)alpha);
				}

				if (newColor != currentColor) {
					if (count == 1) {
						wrapSaveColor(currentColor)
					} else if (count == 2) {
						wrapSaveColor(currentColor)
						wrapSaveColor(currentColor)
					} else if (count == 3) {
						wrapSaveColor(currentColor)
						wrapSaveColor(currentColor)
						wrapSaveColor(currentColor)
					} else if (count > 3) {
						wrapSave(stream.writeInt(RLE_FLAG))
						wrapSave(stream.writeInt(count))
						wrapSaveColor(currentColor)
					}
					count = 0;
					currentColor = newColor;
				}
				count++;
			}
		}
		if (count == 1) {
			wrapSaveColor(currentColor)
		} else if (count == 2) {
			wrapSaveColor(currentColor)
			wrapSaveColor(currentColor)
		} else if (count == 3) {
			wrapSaveColor(currentColor)
			wrapSaveColor(currentColor)
			wrapSaveColor(currentColor)
		} else if (count > 3) {
			wrapSave(stream.writeInt(RLE_FLAG))
			wrapSave(stream.writeInt(count))
			wrapSaveColor(currentColor)
		}
		count = 0;
		wrapSave(stream.writeInt(NEXT_SLICE_FLAG));
	}
	return true;
}

bool QBFormat::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	io::FileStream stream(file.get());
	wrapSave(stream.writeInt(257))
	wrapSave(stream.writeInt((uint32_t)ColorFormat::RGBA))
	wrapSave(stream.writeInt((uint32_t)ZAxisOrientation::Right))
	wrapSave(stream.writeInt((uint32_t)Compression::RLE))
	wrapSave(stream.writeInt((uint32_t)VisibilityMask::AlphaChannelVisibleByValue))
	wrapSave(stream.writeInt((int)volumes.size()))
	for (const auto& v : volumes) {
		if (v.volume == nullptr) {
			continue;
		}
		if (!saveMatrix(stream, v)) {
			return false;
		}
	}
	return true;
}

bool QBFormat::setVoxel(voxel::RawVolume* volume, uint32_t x, uint32_t y, uint32_t z, const glm::ivec3& offset, const voxel::Voxel& voxel) {
	const int32_t fx = offset.x + x;
	const int32_t fy = offset.y + y;
	const int32_t fz = offset.z + z;
	Log::trace("Set voxel %i to %i:%i:%i (z-axis: %i)", (int)voxel.getMaterial(), fx, fy, fz, (int)_zAxisOrientation);
	const voxel::Region& region = volume->region();
	if (!region.containsPoint(glm::ivec3(fx, fy, fz))) {
		const glm::ivec3& mins = region.getLowerCorner();
		const glm::ivec3& maxs = region.getUpperCorner();
		Log::error("Could not set voxel at %i:%i:%i - outside the region [%i:%i:%i][%i:%i:%i]",
				fx, fy, fz, mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
		return false;
	}
	volume->setVoxel(fx, fy, fz, voxel);
	return true;
}

voxel::Voxel QBFormat::getVoxel(io::ReadStream& stream) {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
	wrapColor(stream.readByte(red))
	wrapColor(stream.readByte(green))
	wrapColor(stream.readByte(blue))
	wrapColor(stream.readByte(alpha))
	Log::trace("Red: %i, Green: %i, Blue: %i, Alpha: %i", (int)red, (int)green, (int)blue, (int)alpha);
	if (alpha == 0) {
		return voxel::Voxel();
	}
	glm::vec4 color(0.0f);
	if (_colorFormat == ColorFormat::RGBA) {
		color.r = (float)red / 255.0f;
		color.b = (float)blue / 255.0f;
	} else {
		color.r = (float)blue / 255.0f;
		color.b = (float)red / 255.0f;
	}
	color.g = (float)green / 255.0f;
	color.a = (float)alpha / 255.0f;
	const uint8_t index = findClosestIndex(color);
	voxel::VoxelType voxelType = voxel::VoxelType::Generic;
	if (index == 0 && alpha == 0u) {
		voxelType = voxel::VoxelType::Air;
	}
	return voxel::createVoxel(voxelType, index);
}

bool QBFormat::loadMatrix(io::ReadStream& stream, VoxelVolumes& volumes) {
	char name[260] = "";
	uint8_t nameLength;
	wrap(stream.readByte(nameLength));
	Log::debug("Matrix name length: %u", (uint32_t)nameLength);
	wrapBool(stream.readString(nameLength, name));
	name[nameLength] = '\0';
	Log::debug("Matrix name: %s", name);

	glm::uvec3 size(0);
	wrap(stream.readInt((uint32_t&)size.x));
	wrap(stream.readInt((uint32_t&)size.y));
	wrap(stream.readInt((uint32_t&)size.z));
	Log::debug("Matrix size: %i:%i:%i", size.x, size.y, size.z);

	if (size.x == 0 || size.y == 0 || size.z == 0) {
		Log::error("Invalid size (%i:%i:%i)", size.x, size.y, size.z);
		return false;
	}

	if (size.x > 2048 || size.y > 2048 || size.z > 2048) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", size.x, size.y, size.z);
		return false;
	}

	glm::ivec3 offset(0);
	wrap(stream.readInt((uint32_t&)offset.x));
	wrap(stream.readInt((uint32_t&)offset.y));
	wrap(stream.readInt((uint32_t&)offset.z));
	Log::debug("Matrix offset: %i:%i:%i", offset.x, offset.y, offset.z);

	const glm::ivec3 maxs(offset.x + size.x - 1, offset.y + size.y - 1, offset.z + size.z - 1);
	const voxel::Region region(offset.x, offset.y, offset.z, maxs.x, maxs.y, maxs.z);
	core_assert_msg(region.getDimensionsInVoxels() == glm::ivec3(size),
			"%i:%i:%i versus %i:%i:%i", region.getDimensionsInVoxels().x, region.getDimensionsInVoxels().y, region.getDimensionsInVoxels().z,
			size.x, size.y, size.z);
	if (!region.isValid()) {
		return false;
	}

	if (region.getDepthInVoxels() >= 2048 || region.getHeightInVoxels() >= 2048
		|| region.getWidthInVoxels() >= 2048) {
		Log::error("Region exceeds the max allowed boundaries");
		return false;
	}

	voxel::RawVolume* v = new voxel::RawVolume(region);
	volumes.push_back(VoxelVolume(v, name, true));
	if (_compressed == Compression::None) {
		Log::debug("qb matrix uncompressed");
		for (uint32_t z = 0; z < size.z; ++z) {
			for (uint32_t y = 0; y < size.y; ++y) {
				for (uint32_t x = 0; x < size.x; ++x) {
					const voxel::Voxel& voxel = getVoxel(stream);
					if (!setVoxel(v, x, y, z, offset, voxel)) {
						return false;
					}
				}
			}
		}
		return true;
	}

	Log::debug("Matrix rle compressed");

	uint32_t z = 0u;
	while (z < size.z) {
		uint32_t index = 0;
		for (;;) {
			uint32_t data;
			wrap(stream.peekInt(data))
			if (data == NEXT_SLICE_FLAG) {
				stream.skip(sizeof(data));
				break;
			}

			uint32_t count = 1;
			if (data == RLE_FLAG) {
				stream.skip(sizeof(data));
				wrap(stream.readInt(count))
				Log::trace("%u voxels of the same type", count);
			}

			if (count > 32768) {
				Log::error("Max RLE count exceeded: %i", (int)count);
				return false;
			}
			const voxel::Voxel& voxel = getVoxel(stream);
			for (uint32_t j = 0; j < count; ++j) {
				const uint32_t x = (index + j) % size.x;
				const uint32_t y = (index + j) / size.x;
				if (!setVoxel(v, x, y, z, offset, voxel)) {
					return false;
				}
			}
			index += count;
		}
		++z;
	}
	Log::debug("Matrix read");
	return true;
}

bool QBFormat::loadFromStream(io::ReadStream& stream, VoxelVolumes& volumes) {
	wrap(stream.readInt(_version))
	uint32_t colorFormat;
	wrap(stream.readInt(colorFormat))
	_colorFormat = (ColorFormat)colorFormat;
	uint32_t zAxisOrientation;
	wrap(stream.readInt(zAxisOrientation))
	_zAxisOrientation = ZAxisOrientation::Right; //(ZAxisOrientation)zAxisOrientation;
	uint32_t compressed;
	wrap(stream.readInt(compressed))
	_compressed = (Compression)compressed;
	uint32_t visibilityMaskEncoded;
	wrap(stream.readInt(visibilityMaskEncoded))
	_visibilityMaskEncoded = (VisibilityMask)visibilityMaskEncoded;

	uint32_t numMatrices;
	wrap(stream.readInt(numMatrices))

	Log::debug("Version: %u", _version);
	Log::debug("ColorFormat: %u", core::enumVal(_colorFormat));
	Log::debug("ZAxisOrientation: %u", core::enumVal(_zAxisOrientation));
	Log::debug("Compressed: %u", core::enumVal(_compressed));
	Log::debug("VisibilityMaskEncoded: %u", core::enumVal(_visibilityMaskEncoded));
	Log::debug("NumMatrices: %u", numMatrices);

	volumes.reserve(numMatrices);
	for (uint32_t i = 0; i < numMatrices; i++) {
		Log::debug("Loading matrix: %u", i);
		if (!loadMatrix(stream, volumes)) {
			break;
		}
	}
	return true;
}

bool QBFormat::loadGroups(const core::String& filename, io::ReadStream& stream, VoxelVolumes& volumes) {
	return loadFromStream(stream, volumes);
}

}

#undef wrap
#undef wrapBool
#undef wrapSave
#undef wrapSaveColor
#undef wrapColor
#undef setBit
