/**
 * @file
 */

#pragma once

#include "app/tests/AbstractTest.h"
#include "voxel/PagedVolumeWrapper.h"
#include "voxel/PagedVolume.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxel/MaterialColor.h"
#include "voxel/Constants.h"
#include "math/Random.h"
#include "core/Common.h"

#include <glm/geometric.hpp>

namespace voxelworld {

class AbstractVoxelWorldTest: public app::AbstractTest {
protected:
	class Pager: public voxel::PagedVolume::Pager {
		AbstractVoxelWorldTest* _test;
	public:
		Pager(AbstractVoxelWorldTest* test) :
				_test(test) {
		}

		bool pageIn(voxel::PagedVolume::PagerContext& ctx) override {
			return _test->pageIn(ctx.region, ctx.chunk);
		}

		void pageOut(voxel::PagedVolume::Chunk* chunk) override {
		}
	};

	virtual bool pageIn(const voxel::Region& region, const voxel::PagedVolume::ChunkPtr& chunk) {
		const glm::vec3 center(region.getCenter());
		for (int z = 0; z < region.getDepthInVoxels(); ++z) {
			for (int y = 0; y < region.getHeightInVoxels(); ++y) {
				for (int x = 0; x < region.getWidthInVoxels(); ++x) {
					const glm::vec3 pos(x, y, z);
					const float distance = glm::distance(pos, center);
					voxel::Voxel uVoxelValue;
					if (distance <= 30.0f) {
						uVoxelValue = voxel::createColorVoxel(voxel::VoxelType::Grass, 0);
					}

					chunk->setVoxel(x, y, z, uVoxelValue);
				}
			}
		}
		return true;
	}

	Pager _pager;
	voxel::PagedVolume _volData;
	voxel::PagedVolumeWrapper _ctx;
	math::Random _random;
	long _seed = 0;
	const voxel::Region _region { glm::ivec3(0), glm::ivec3(63) };

	AbstractVoxelWorldTest() :
			_pager(this), _volData(&_pager, 128 * 1024 * 1024, 64), _ctx(nullptr, nullptr, voxel::Region()) {
	}

public:
	void SetUp() override {
		_volData.flushAll();
		app::AbstractTest::SetUp();
		ASSERT_TRUE(voxel::initDefaultPalette());
		_random.setSeed(_seed);
		_ctx = voxel::PagedVolumeWrapper(&_volData, _volData.chunk(_region.getCenter()), _region);
	}
};

}
