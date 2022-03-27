/**
 * @file
 */

#include "voxelformat/SchematicFormat.h"
#include "AbstractVoxFormatTest.h"
#include "io/FileStream.h"
#include "voxelformat/VolumeFormat.h"

namespace voxelformat {

class SchematicFormatTest : public AbstractVoxFormatTest {};

TEST_F(SchematicFormatTest, testLoad) {
	SchematicFormat f;
	// https://www.planetminecraft.com/project/viking-island-4911284/
	std::unique_ptr<voxel::RawVolume> volume(load("viking_island.schematic", f));
	ASSERT_NE(nullptr, volume) << "Could not load schematic file";
}

} // namespace voxelformat
