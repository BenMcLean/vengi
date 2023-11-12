/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/private/cubeworld/CubFormat.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class CubFormatTest: public AbstractVoxFormatTest {
};

TEST_F(CubFormatTest, testLoad) {
	canLoad("cw.cub");
}

TEST_F(CubFormatTest, testLoadPalette) {
	CubFormat f;
	palette::Palette pal;
	EXPECT_EQ(5, loadPalette("rgb.cub", f, pal));
}

TEST_F(CubFormatTest, testLoadRGB) {
	testRGB("rgb.cub");
}

TEST_F(CubFormatTest, testLoadRGBSmall) {
	testRGBSmall("rgb_small.cub");
}

TEST_F(CubFormatTest, testLoadRGBSmallSaveLoad) {
	testRGBSmallSaveLoad("rgb_small.cub");
}

TEST_F(CubFormatTest, testSaveSmallVoxel) {
	CubFormat f;
	testSaveLoadVoxel("cw-smallvolumesavetest.cub", &f, 0, 1);
}

}
