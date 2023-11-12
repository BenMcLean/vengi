/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "voxel/Face.h"
#include <functional>
#include <glm/fwd.hpp>

namespace voxel {
class RawVolume;
class Region;
class Voxel;
class RawVolumeWrapper;
}
namespace palette {
class Palette;
}

namespace voxelutil {

bool copyIntoRegion(const voxel::RawVolume &in, voxel::RawVolume &out, const voxel::Region &targetRegion);

void fillInterpolated(voxel::RawVolume *v, const palette::Palette &palette);

bool copy(const voxel::RawVolume &in, const voxel::Region &inRegion, voxel::RawVolume &out,
		  const voxel::Region &outRegion);
/**
 * @brief Checks if there is a solid voxel around the given position
 */
bool isTouching(const voxel::RawVolume *volume, const glm::ivec3& pos);

/**
 * @brief Checks whether the given region of the volume is only filled with air
 * @return @c true if no blocking voxel is inside the region, @c false otherwise
 * @sa voxel::isBlocked()
 */
bool isEmpty(const voxel::RawVolume &in, const voxel::Region &region);

using WalkCheckCallback = std::function<bool(const voxel::RawVolumeWrapper &, const glm::ivec3 &)>;
using WalkExecCallback = std::function<bool(voxel::RawVolumeWrapper &, const glm::ivec3 &)>;

int fillPlane(voxel::RawVolumeWrapper &in, const image::ImagePtr &image, const voxel::Voxel &searchedVoxel,
			  const glm::ivec3 &position, voxel::FaceNames face);

/**
 * @param pos The position where the first voxel should be placed
 * @param face The face where the trace enters the ground voxel. This is about the direction of the plane that is
 * extruded. If e.g. face x was detected, the plane is created on face y and z.
 * @param groundVoxel The voxel we want to extrude on
 * @param newPlaneVoxel The voxel to put at the given position
 */
int extrudePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face, const voxel::Voxel &groundVoxel,
			const voxel::Voxel &newPlaneVoxel);
int erasePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face, const voxel::Voxel &groundVoxel);
int paintPlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
			   const voxel::Voxel &searchVoxel, const voxel::Voxel &replaceVoxel);
int overridePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
				  const voxel::Voxel &replaceVoxel);
void fillHollow(voxel::RawVolumeWrapper &in, const voxel::Voxel &voxel);
/**
 * @brief Remaps or converts the voxel colors to the new given palette by searching for the closest color
 * @param skipColorIndex One particular palette color index that is not taken into account. This can be used to e.g. search for replacements
 */
voxel::Region remapToPalette(voxel::RawVolume *v, const palette::Palette &oldPalette, const palette::Palette &newPalette, int skipColorIndex = -1);

/**
 * @brief Creates a diff between the two given volumes
 * @note The caller has to free the volume
 * @return nullptr if the volumes don't differ in the shared region dimensions
 */
voxel::RawVolume *diffVolumes(const voxel::RawVolume *v1, const voxel::RawVolume *v2);

} // namespace voxelutil
