/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/QBFormat.h"
#include "voxelformat/VolumeFormat.h"

namespace voxel {

class QBFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QBFormatTest, testLoad) {
	QBFormat f;
	std::unique_ptr<RawVolume> volume(load("qubicle.qb", f));

	VolumePrintThreshold = 40;
	ASSERT_NE(nullptr, volume) << "Could not load qb file";

	// feet
	ASSERT_NE(Empty, volume->voxel(18, 0, 1)) << *volume;
	ASSERT_NE(Empty, volume->voxel(18, 0, 2)) << *volume;
	ASSERT_EQ(Empty, volume->voxel(18, 0, 3)) << *volume;
	ASSERT_EQ(Empty, volume->voxel(18, 0, 4)) << *volume;
	ASSERT_NE(Empty, volume->voxel(22, 0, 1)) << *volume;
	ASSERT_NE(Empty, volume->voxel(22, 0, 2)) << *volume;
	ASSERT_EQ(Empty, volume->voxel(22, 0, 3)) << *volume;
	ASSERT_EQ(Empty, volume->voxel(22, 0, 4)) << *volume;
}

TEST_F(QBFormatTest, testLoadRGB) {
	QBFormat f;
	std::unique_ptr<RawVolume> volume(load("rgb.qb", f));
	ASSERT_NE(nullptr, volume) << "Could not load qb file";
	testRGB(volume.get());
}

TEST_F(QBFormatTest, testSaveSmallVoxel) {
	QBFormat f;
	Region region(glm::ivec3(0), glm::ivec3(1));
	RawVolume original(region);
	original.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1));
	original.setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, 245));
	original.setVoxel(0, 1, 1, createVoxel(VoxelType::Generic, 127));
	original.setVoxel(0, 1, 0, createVoxel(VoxelType::Generic, 200));
	ASSERT_TRUE(f.save(&original, open("qubicle-smallvolumesavetest.qb", io::FileMode::Write)));
	f = QBFormat();
	std::unique_ptr<RawVolume> loaded(load("qubicle-smallvolumesavetest.qb", f));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

TEST_F(QBFormatTest, testSaveSingleVoxel) {
	QBFormat f;
	Region region(glm::ivec3(0), glm::ivec3(0));
	RawVolume original(region);
	original.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1));
	ASSERT_TRUE(f.save(&original, open("qubicle-singlevoxelsavetest.qb", io::FileMode::Write)));
	f = QBFormat();
	std::unique_ptr<RawVolume> loaded(load("qubicle-singlevoxelsavetest.qb", f));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

TEST_F(QBFormatTest, testSaveMultipleLayers) {
	QBFormat f;
	Region region(glm::ivec3(0), glm::ivec3(0));
	RawVolume layer1(region);
	RawVolume layer2(region);
	RawVolume layer3(region);
	RawVolume layer4(region);
	EXPECT_TRUE(layer1.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	EXPECT_TRUE(layer2.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	EXPECT_TRUE(layer3.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	EXPECT_TRUE(layer4.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	VoxelVolumes volumes;
	volumes.push_back(VoxelVolume(&layer1));
	volumes.push_back(VoxelVolume(&layer2));
	volumes.push_back(VoxelVolume(&layer3));
	volumes.push_back(VoxelVolume(&layer4));
	EXPECT_TRUE(f.saveGroups(volumes, open("qubicle-multiplelayersavetest.qb", io::FileMode::Write)));
	f = QBFormat();
	VoxelVolumes volumesLoad;
	const io::FilePtr &file = open("qubicle-multiplelayersavetest.qb");
	io::FileStream stream(file.get());
	EXPECT_TRUE(f.loadGroups(file->fileName(), stream, volumesLoad));
	EXPECT_EQ(volumesLoad.size(), volumes.size());
	voxelformat::clearVolumes(volumesLoad);
}

TEST_F(QBFormatTest, testLoadSave) {
	QBFormat f;
	std::unique_ptr<RawVolume> original(load("qubicle.qb", f));
	ASSERT_NE(nullptr, original);
	ASSERT_TRUE(f.save(original.get(), open("qubicle-savetest.qb", io::FileMode::Write)));
	ASSERT_TRUE(open("qubicle-savetest.qb")->length() > 177);
	f = QBFormat();
	std::unique_ptr<RawVolume> loaded(load("qubicle-savetest.qb", f));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(*original, *loaded);
}
}
