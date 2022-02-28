/**
 * @file
 */

#pragma once

#include "app/tests/AbstractTest.h"
#include "voxel/MaterialColor.h"
#include "voxel/tests/TestHelper.h"

namespace voxel {

class AbstractVoxelTest: public app::AbstractTest {
protected:
	class Pager: public PagedVolume::Pager {
		AbstractVoxelTest* _test;
	public:
		Pager(AbstractVoxelTest* test) :
				_test(test) {
		}

		bool pageIn(PagedVolume::PagerContext& ctx) override {
			return _test->pageIn(ctx.region, ctx.chunk);
		}

		void pageOut(PagedVolume::Chunk* chunk) override {
		}
	};

	virtual bool pageIn(const Region& region, const PagedVolume::ChunkPtr& chunk) {
		const glm::vec3 center(region.getCenter());
		for (int z = 0; z < region.getDepthInVoxels(); ++z) {
			for (int y = 0; y < region.getHeightInVoxels(); ++y) {
				for (int x = 0; x < region.getWidthInVoxels(); ++x) {
					const glm::vec3 pos(x, y, z);
					const float distance = glm::distance(pos, center);
					Voxel uVoxelValue;
					if (distance <= 30.0f) {
						uVoxelValue = createRandomColorVoxel(VoxelType::Grass);
					}

					chunk->setVoxel(x, y, z, uVoxelValue);
				}
			}
		}
		return true;
	}

	Pager _pager;
	PagedVolume _volData;
	PagedVolumeWrapper _ctx;
	math::Random _random;
	long _seed = 0;
	const voxel::Region _region { glm::ivec3(0), glm::ivec3(63) };

	AbstractVoxelTest() :
			_pager(this), _volData(&_pager, 128 * 1024 * 1024, 64), _ctx(nullptr, nullptr, voxel::Region()) {
	}

public:
	void SetUp() override {
		_volData.flushAll();
		app::AbstractTest::SetUp();
		ASSERT_TRUE(voxel::initDefaultPalette());
		_random.setSeed(_seed);
		_ctx = PagedVolumeWrapper(&_volData, _volData.chunk(_region.getCenter()), _region);
		VolumePrintThreshold = 10;
	}

	void TearDown() override {
		voxel::shutdownMaterialColors();
		app::AbstractTest::TearDown();
	}
};

}
