/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "VolumeMerger.h"
#include "core/Common.h"

namespace voxelutil {

/**
 * @brief Will skip air voxels on volume cropping
 */
struct CropSkipEmpty {
	inline bool operator() (const voxel::Voxel& voxel) const {
		return isAir(voxel.getMaterial());
	}
};

/**
 * @brief Resizes a volume to cut off empty parts
 */
template<class CropSkipCondition = CropSkipEmpty>
voxel::RawVolume* cropVolume(const voxel::RawVolume* volume, const glm::ivec3& mins, const glm::ivec3& maxs, CropSkipCondition condition = CropSkipCondition()) {
	core_trace_scoped(CropRawVolume);
	const voxel::Region newRegion(mins, maxs);
	if (!newRegion.isValid()) {
		return nullptr;
	}
	voxel::RawVolume* newVolume = new voxel::RawVolume(newRegion);
	voxelutil::mergeVolumes(newVolume, volume, newRegion, voxel::Region(mins, maxs));
	return newVolume;
}

/**
 * @brief Resizes a volume to cut off empty parts
 */
template<class CropSkipCondition = CropSkipEmpty>
voxel::RawVolume* cropVolume(const voxel::RawVolume* volume, CropSkipCondition condition = CropSkipCondition()) {
	core_trace_scoped(CropRawVolume);
	const glm::ivec3& mins = volume->mins();
	const glm::ivec3& maxs = volume->maxs();
	glm::ivec3 newMins((std::numeric_limits<int>::max)() / 2);
	glm::ivec3 newMaxs((std::numeric_limits<int>::min)() / 2);
	voxel::RawVolume::Sampler volumeSampler(volume);
	for (int32_t z = mins.z; z <= maxs.z; ++z) {
		for (int32_t y = mins.y; y <= maxs.y; ++y) {
			volumeSampler.setPosition(mins.x, y, z);
			for (int32_t x = mins.x; x <= maxs.x; ++x) {
				const voxel::Voxel& voxel = volumeSampler.voxel();
				volumeSampler.movePositiveX();
				if (condition(voxel)) {
					continue;
				}
				newMins.x = core_min(newMins.x, x);
				newMins.y = core_min(newMins.y, y);
				newMins.z = core_min(newMins.z, z);

				newMaxs.x = core_max(newMaxs.x, x);
				newMaxs.y = core_max(newMaxs.y, y);
				newMaxs.z = core_max(newMaxs.z, z);
			}
		}
	}
	if (newMaxs.z == (std::numeric_limits<int>::min)() / 2) {
		return nullptr;
	}
	return cropVolume(volume, newMins, newMaxs, condition);
}

}
