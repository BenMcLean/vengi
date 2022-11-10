/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"
#include "voxel/MaterialColor.h"
#include "voxel/Constants.h"
#include "voxel/RawVolume.h"
#include "voxel/PagedVolume.h"

static constexpr int MAX_BENCHMARK_VOLUME_SIZE = 64;
static const int meshSize = voxel::MAX_MESH_CHUNK_HEIGHT;
class CubicSurfaceExtractorBenchmark : public app::AbstractBenchmark {
public:
	void onCleanupApp() override {
	}

	template<class Volume>
	void fill(const voxel::Region& region, Volume* v) const {
		const voxel::Voxel voxel = voxel::createVoxel(1);
		for (int x = region.getLowerX(); x < region.getUpperX(); ++x) {
			for (int y = region.getLowerY(); y < region.getUpperY(); ++y) {
				for (int z = region.getLowerZ(); z < region.getUpperZ(); ++z) {
					if (x + y + z % 2 == 0) {
						v->setVoxel(x, y, z, voxel);
					}
				}
			}
		}
	}

	class BenchmarkPager: public voxel::PagedVolume::Pager {
	public:
		bool pageIn(voxel::PagedVolume::PagerContext& ctx) override {
			return false;
		}

		void pageOut(voxel::PagedVolume::Chunk* chunk) override {
		}
	};
};

BENCHMARK_DEFINE_F(CubicSurfaceExtractorBenchmark, RawVolumeExtractGreedy)(benchmark::State &state) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(state.range(0), meshSize, state.range(0)));
	const voxel::Region volumeRegion(0, MAX_BENCHMARK_VOLUME_SIZE);
	voxel::RawVolume volume(volumeRegion);
	fill(region, &volume);
	voxel::Mesh mesh(1024 * 1024, 1024 * 1024, false);
	for (auto _ : state) {
		voxel::extractCubicMesh(&volume, region, &mesh, voxel::IsQuadNeeded(), region.getLowerCorner(), true, true);
	}
}

BENCHMARK_DEFINE_F(CubicSurfaceExtractorBenchmark, RawVolumeExtract)(benchmark::State &state) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(state.range(0), meshSize, state.range(0)));
	const voxel::Region volumeRegion(0, MAX_BENCHMARK_VOLUME_SIZE);
	voxel::RawVolume volume(volumeRegion);
	fill(region, &volume);
	voxel::Mesh mesh(1024 * 1024, 1024 * 1024, false);
	for (auto _ : state) {
		voxel::extractCubicMesh(&volume, region, &mesh, voxel::IsQuadNeeded(), region.getLowerCorner(), false, false);
	}
}

BENCHMARK_DEFINE_F(CubicSurfaceExtractorBenchmark, RawVolumeExtractGreedyEmpty)(benchmark::State &state) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(state.range(0), meshSize, state.range(0)));
	const voxel::Region volumeRegion(0, MAX_BENCHMARK_VOLUME_SIZE);
	voxel::RawVolume volume(volumeRegion);
	voxel::Mesh mesh(1024 * 1024, 1024 * 1024, false);
	for (auto _ : state) {
		voxel::extractCubicMesh(&volume, region, &mesh, voxel::IsQuadNeeded(), region.getLowerCorner(), true, true);
	}
}

BENCHMARK_DEFINE_F(CubicSurfaceExtractorBenchmark, RawVolumeExtractEmpty)(benchmark::State &state) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(state.range(0), meshSize, state.range(0)));
	const voxel::Region volumeRegion(0, MAX_BENCHMARK_VOLUME_SIZE);
	voxel::RawVolume volume(volumeRegion);
	voxel::Mesh mesh(1024 * 1024, 1024 * 1024, false);
	for (auto _ : state) {
		voxel::extractCubicMesh(&volume, region, &mesh, voxel::IsQuadNeeded(), region.getLowerCorner(), false, false);
	}
}

BENCHMARK_DEFINE_F(CubicSurfaceExtractorBenchmark, PagedVolumeExtractGreedy)(benchmark::State &state) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(state.range(0), meshSize, state.range(0)));
	BenchmarkPager pager;
	voxel::PagedVolume volume(&pager, 1024 * 1024 * 1024, 256);
	fill(region, &volume);
	voxel::Mesh mesh(1024 * 1024, 1024 * 1024, false);
	for (auto _ : state) {
		voxel::extractCubicMesh(&volume, region, &mesh, voxel::IsQuadNeeded(), region.getLowerCorner(), true, true);
	}
}

BENCHMARK_DEFINE_F(CubicSurfaceExtractorBenchmark, PagedVolumeExtract)(benchmark::State &state) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(state.range(0), meshSize, state.range(0)));
	BenchmarkPager pager;
	voxel::PagedVolume volume(&pager, 1024 * 1024 * 1024, 256);
	fill(region, &volume);
	voxel::Mesh mesh(1024 * 1024, 1024 * 1024, false);
	for (auto _ : state) {
		voxel::extractCubicMesh(&volume, region, &mesh, voxel::IsQuadNeeded(), region.getLowerCorner(), false, false);
	}
}

BENCHMARK_DEFINE_F(CubicSurfaceExtractorBenchmark, PagedVolumeExtractGreedyEmpty)(benchmark::State &state) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(state.range(0), meshSize, state.range(0)));
	BenchmarkPager pager;
	voxel::PagedVolume volume(&pager, 1024 * 1024 * 1024, 256);
	voxel::Mesh mesh(1024 * 1024, 1024 * 1024, false);
	for (auto _ : state) {
		voxel::extractCubicMesh(&volume, region, &mesh, voxel::IsQuadNeeded(), region.getLowerCorner(), true, true);
	}
}

BENCHMARK_DEFINE_F(CubicSurfaceExtractorBenchmark, PagedVolumeExtractEmpty)(benchmark::State &state) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(state.range(0), meshSize, state.range(0)));
	BenchmarkPager pager;
	voxel::PagedVolume volume(&pager, 1024 * 1024 * 1024, 256);
	voxel::Mesh mesh(1024 * 1024, 1024 * 1024, false);
	for (auto _ : state) {
		voxel::extractCubicMesh(&volume, region, &mesh, voxel::IsQuadNeeded(), region.getLowerCorner(), false, false);
	}
}

BENCHMARK_REGISTER_F(CubicSurfaceExtractorBenchmark, RawVolumeExtractGreedy)->RangeMultiplier(2)->Range(16, MAX_BENCHMARK_VOLUME_SIZE);
BENCHMARK_REGISTER_F(CubicSurfaceExtractorBenchmark, RawVolumeExtract)->RangeMultiplier(2)->Range(16, MAX_BENCHMARK_VOLUME_SIZE);
BENCHMARK_REGISTER_F(CubicSurfaceExtractorBenchmark, RawVolumeExtractGreedyEmpty)->RangeMultiplier(2)->Range(16, MAX_BENCHMARK_VOLUME_SIZE);
BENCHMARK_REGISTER_F(CubicSurfaceExtractorBenchmark, RawVolumeExtractEmpty)->RangeMultiplier(2)->Range(16, MAX_BENCHMARK_VOLUME_SIZE);

BENCHMARK_REGISTER_F(CubicSurfaceExtractorBenchmark, PagedVolumeExtractGreedy)->RangeMultiplier(2)->Range(16, MAX_BENCHMARK_VOLUME_SIZE);
BENCHMARK_REGISTER_F(CubicSurfaceExtractorBenchmark, PagedVolumeExtract)->RangeMultiplier(2)->Range(16, MAX_BENCHMARK_VOLUME_SIZE);
BENCHMARK_REGISTER_F(CubicSurfaceExtractorBenchmark, PagedVolumeExtractGreedyEmpty)->RangeMultiplier(2)->Range(16, MAX_BENCHMARK_VOLUME_SIZE);
BENCHMARK_REGISTER_F(CubicSurfaceExtractorBenchmark, PagedVolumeExtractEmpty)->RangeMultiplier(2)->Range(16, MAX_BENCHMARK_VOLUME_SIZE);

BENCHMARK_MAIN();
