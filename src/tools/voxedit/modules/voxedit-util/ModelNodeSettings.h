/**
 * @file
 */

#pragma once

#include "core/Optional.h"
#include "core/String.h"
#include "palette/Palette.h"
#include "voxel/Region.h"
#include <glm/vec3.hpp>

namespace voxedit {

namespace _priv {
static constexpr const int MaxVolumeSize = 256;
}

/**
 * @brief A struct that holds the information that are needed when creating new model nodes
 */
struct ModelNodeSettings {
	core::String name;
	core::Optional<palette::Palette> palette;
	glm::ivec3 position;
	glm::ivec3 size;
	int parent = 0;

	ModelNodeSettings() {
		reset();
	}

	inline void reset() {
		position = glm::ivec3(0);
		size = glm::ivec3(32);
		parent = 0;
	}

	inline voxel::Region region() {
		const voxel::Region region(position, position + size - 1);
		if (!region.isValid()) {
			reset();
			return voxel::Region {position, position + size - 1};
		}
		const glm::ivec3& dim = region.getDimensionsInCells();
		if (dim.x >= _priv::MaxVolumeSize || dim.y >= _priv::MaxVolumeSize || dim.z >= _priv::MaxVolumeSize) {
			reset();
			return voxel::Region {position, position + size - 1};
		}
		return region;
	}
};

}
