/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/KVXFormat.h"

namespace voxelformat {

class KVXFormatTest: public AbstractVoxFormatTest {
};

TEST_F(KVXFormatTest, testLoad) {
	KVXFormat f;
	std::unique_ptr<voxel::RawVolume> volume(load("test.kvx", f));
	ASSERT_NE(nullptr, volume) << "Could not load volume";
}

}
