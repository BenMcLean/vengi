/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "modifier/Selection.h"

namespace voxedit {
namespace tool {

extern voxel::RawVolume* copy(const voxel::RawVolume* volume, const Selections& selections);
extern voxel::RawVolume* cut(voxel::RawVolume* volume, const Selections& selections, voxel::Region& modifiedRegion);
extern void paste(voxel::RawVolume* out, const voxel::RawVolume* in, const glm::ivec3& referencePosition, voxel::Region& modifiedRegion);

}
}
