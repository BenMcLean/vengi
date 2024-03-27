/**
 * @file
 */

#include "voxelformat/private/minecraft/SchematicFormat.h"
#include "AbstractVoxFormatTest.h"

namespace voxelformat {

class SchematicFormatTest : public AbstractVoxFormatTest {};

TEST_F(SchematicFormatTest, testLoadLitematic) {
	canLoad("test.litematic");
}

TEST_F(SchematicFormatTest, DISABLED_testLoadVikingIsland) {
	// https://www.planetminecraft.com/project/viking-island-4911284/
	canLoad("viking_island.schematic");
}

TEST_F(SchematicFormatTest, DISABLED_testLoadStructory) {
	// https://www.planetminecraft.com/data-pack/structory/
	canLoad("brick_chimney_1.nbt");
}

TEST_F(SchematicFormatTest, DISABLED_testSaveSmallVoxel) {
	SchematicFormat f;
	testSaveLoadVoxel("minecraft-smallvolumesavetest.schematic", &f);
}

} // namespace voxelformat
