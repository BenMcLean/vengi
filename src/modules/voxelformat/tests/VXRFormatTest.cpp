/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "palette/Palette.h"
#include "voxelformat/private/sandbox/VXMFormat.h"
#include "voxelformat/private/sandbox/VXRFormat.h"

#include <glm/gtc/type_ptr.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/string_cast.hpp>

namespace voxelformat {

class VXRFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VXRFormatTest, testSaveSmallVoxel) {
	VXMFormat vxm;
	palette::Palette pal;
	pal.nippon();
	testSave("sandbox-smallvolumesavetest0.vxm", &vxm, pal, voxel::ValidateFlags::AllPaletteMinMatchingColors & ~(voxel::ValidateFlags::Pivot));
	VXRFormat f;
	testSaveLoadVoxel("sandbox-smallvolumesavetest.vxr", &f);
}

TEST_F(VXRFormatTest, testSaveSmallVolume) {
	VXRFormat f;
	testSaveSmallVolume("testSaveSmallVolume.vxr", &f);
}

TEST_F(VXRFormatTest, testSaveLoadVoxel) {
	VXRFormat f;
	testSaveLoadVoxel("testSaveLoadVoxel.vxr", &f);
}

TEST_F(VXRFormatTest, testGiantDinosaur) {
	VXRFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(loadGroups("giant_dinosaur/Reptiles_Biped_Giant_Dinossaur_V2.vxr", f, sceneGraph));
	{
		scenegraph::SceneGraphNode* node = sceneGraph.findNodeByName("Hip");
		ASSERT_NE(nullptr, node);
		const scenegraph::SceneGraphTransform &transform = node->transform(0);

		const float expected[]{
			1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 64.5f, 0.0f, 1.0f};
		const float *actual = glm::value_ptr(transform.worldMatrix());
		for (int i = 0; i < 16; ++i) {
			ASSERT_FLOAT_EQ(expected[i], actual[i]) << i << ": " << glm::to_string(transform.worldMatrix());
		}
		EXPECT_FLOAT_EQ(node->pivot().x, 0.5f);
		EXPECT_FLOAT_EQ(node->pivot().y, 0.29411766f);
		EXPECT_FLOAT_EQ(node->pivot().z, 1.0f);
	}
	{
		scenegraph::SceneGraphNode* node = sceneGraph.findNodeByName("Tail4");
		ASSERT_NE(nullptr, node);
		const scenegraph::SceneGraphTransform &transform = node->transform(0);

		const float expected[]{
			0.941261f, 0.11818516f, -0.31632274f, 0.0f, -0.084998831f, 0.989514f, 0.1167788f, 0.0f, 0.32680732f, -0.083032265f, 0.94143647f, 0.0f, -18.847145f, 51.539429f, -107.957901f, 1.0f};
		const float *actual = glm::value_ptr(transform.worldMatrix());
		for (int i = 0; i < 16; ++i) {
			ASSERT_FLOAT_EQ(expected[i], actual[i]) << i << ": " << glm::to_string(transform.worldMatrix());
		}
		EXPECT_FLOAT_EQ(node->pivot().x, 0.5f);
		EXPECT_FLOAT_EQ(node->pivot().y, 0.45833331f);
		EXPECT_FLOAT_EQ(node->pivot().z, 1.0f);
	}
	{
		scenegraph::SceneGraphNode* node = sceneGraph.findNodeByName("L_Arm");
		ASSERT_NE(nullptr, node);
		const scenegraph::SceneGraphTransform &transform = node->transform(0);

		const float expected[]{
			1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.99974263f, 0.022687117f, 0.0f, 0.0f, -0.022687117f, 0.99974263f, 0.0f, -19.000000f, 52.389652f, 27.726467f, 1.0f};
		const float *actual = glm::value_ptr(transform.worldMatrix());
		for (int i = 0; i < 16; ++i) {
			ASSERT_FLOAT_EQ(expected[i], actual[i]) << i << ": " << glm::to_string(transform.worldMatrix());
		}
		EXPECT_FLOAT_EQ(node->pivot().x, 0.5f);
		EXPECT_FLOAT_EQ(node->pivot().y, 0.76923078f);
		EXPECT_FLOAT_EQ(node->pivot().z, 0.55555558f);
	}
}

}
