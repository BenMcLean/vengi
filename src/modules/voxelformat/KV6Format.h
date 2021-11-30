/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "io/File.h"
#include "core/String.h"

namespace voxel {
/**
 * @brief Voxel sprite format used by the SLAB6 editor, voxlap and Ace of Spades
 */
class KV6Format : public VoxFileFormat {
public:
	bool loadGroups(const core::String &filename, io::ReadStream& stream, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
