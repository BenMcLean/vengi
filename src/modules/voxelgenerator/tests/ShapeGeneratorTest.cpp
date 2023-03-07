/**
 * @file
 */

#include "app/App.h"
#include "core/Color.h"
#include "core/ScopedPtr.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "app/tests/AbstractTest.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/QBFormat.h"
#include "scenegraph/SceneGraph.h"
#include "voxelgenerator/ShapeGenerator.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxel/tests/VoxelPrinter.h"

namespace voxelgenerator {

class ShapeGeneratorTest: public app::AbstractTest {
private:
	using Super = app::AbstractTest;
protected:
	static const voxel::Region _region;
	static constexpr glm::ivec3 _center { 15 };
	static constexpr int _width { 32 };
	static constexpr int _height { 32 };
	static constexpr int _depth { 32 };
	static constexpr voxel::Voxel _voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	voxel::RawVolume *_volume = nullptr;

	inline void volumeComparator(const voxel::RawVolume& volume1, const voxel::Palette &pal1, const voxel::RawVolume& volume2, const voxel::Palette &pal2) {
		const voxel::Region& r1 = volume1.region();
		const voxel::Region& r2 = volume2.region();
		ASSERT_EQ(r1, r2) << "regions differ: " << r1.toString() << " vs " << r2.toString();

		const int32_t lowerX = r1.getLowerX();
		const int32_t lowerY = r1.getLowerY();
		const int32_t lowerZ = r1.getLowerZ();
		const int32_t upperX = r1.getUpperX();
		const int32_t upperY = r1.getUpperY();
		const int32_t upperZ = r1.getUpperZ();
		const int32_t lower2X = r2.getLowerX();
		const int32_t lower2Y = r2.getLowerY();
		const int32_t lower2Z = r2.getLowerZ();
		const int32_t upper2X = r2.getUpperX();
		const int32_t upper2Y = r2.getUpperY();
		const int32_t upper2Z = r2.getUpperZ();

		voxel::RawVolume::Sampler s1(volume1);
		voxel::RawVolume::Sampler s2(volume2);
		for (int32_t z1 = lowerZ, z2 = lower2Z; z1 <= upperZ && z2 <= upper2Z; ++z1, ++z2) {
			for (int32_t y1 = lowerY, y2 = lower2Y; y1 <= upperY && y2 <= upper2Y; ++y1, ++y2) {
				for (int32_t x1 = lowerX, x2 = lower2X; x1 <= upperX && x2 <= upper2X; ++x1, ++x2) {
					s1.setPosition(x1, y1, z1);
					s2.setPosition(x2, y2, z2);
					const voxel::Voxel& voxel1 = s1.voxel();
					const voxel::Voxel& voxel2 = s2.voxel();
					ASSERT_EQ(voxel1.getMaterial(), voxel2.getMaterial())
						<< "Voxel differs at " << x1 << ":" << y1 << ":" << z1 << " in material - voxel1["
						<< voxel::VoxelTypeStr[(int)voxel1.getMaterial()] << ", " << (int)voxel1.getColor() << "], voxel2["
						<< voxel::VoxelTypeStr[(int)voxel2.getMaterial()] << ", " << (int)voxel2.getColor() << "], color1["
						<< core::Color::print(voxel1.getColor()) << "], color2[" << core::Color::print(voxel2.getColor())
						<< "]";
				}
			}
		}
	}

	void verify(const char* filename) {
		voxelformat::QBFormat format;
		const io::FilePtr& file = io::filesystem()->open(filename);
		ASSERT_TRUE(file) << "Can't open " << filename;
		io::FileStream stream(file);
		voxelformat::SceneGraph sceneGraph;
		voxelformat::LoadContext loadCtx;
		ASSERT_TRUE(format.load(file->fileName(), stream, sceneGraph, loadCtx));
		voxelformat::SceneGraph::MergedVolumePalette merged = sceneGraph.merge();
		core::ScopedPtr<voxel::RawVolume> v( merged.first);
		ASSERT_NE(nullptr, v) << "Can't load " << filename;
		volumeComparator(*v, voxel::getPalette(), *_volume, voxel::getPalette());
	}
public:
	void SetUp() override {
		Super::SetUp();
		voxelformat::FormatConfig::init();
		_volume = new voxel::RawVolume(_region);
	}

	void TearDown() override {
		delete _volume;
		Super::TearDown();
	}
};

const voxel::Region ShapeGeneratorTest::_region = voxel::Region{0, 31};

TEST_F(ShapeGeneratorTest, testCreateCubeNoCenter) {
	voxel::RawVolumeWrapper wrapper(_volume);
	shape::createCubeNoCenter(wrapper, _region.getLowerCorner(), _width, _height, _depth, _voxel);
	verify("cube.qb");
}

TEST_F(ShapeGeneratorTest, testCreateCube) {
	voxel::RawVolumeWrapper wrapper(_volume);
	shape::createCube(wrapper, _center, _width, _height, _depth, _voxel);
	int count = 0;
	voxelutil::visitVolume(*_volume, [&count] (int, int, int, const voxel::Voxel&) {
		++count;
	});
	// this -1 is due to rounding errors - but those are expected. The shape generator doesn't
	// know anything about the region position.
	ASSERT_EQ((_width - 1) * (_height - 1) * (_depth - 1), count);
}

TEST_F(ShapeGeneratorTest, testCreateEllipse) {
	voxel::RawVolumeWrapper wrapper(_volume);
	shape::createEllipse(wrapper, _center, _width, _height, _depth, _voxel);
	verify("ellipse.qb");
}

TEST_F(ShapeGeneratorTest, testCreateCone) {
	voxel::RawVolumeWrapper wrapper(_volume);
	shape::createCone(wrapper, _center, _width, _height, _depth, _voxel);
	verify("cone.qb");
}

TEST_F(ShapeGeneratorTest, testCreateDome) {
	voxel::RawVolumeWrapper wrapper(_volume);
	shape::createDome(wrapper, _center, _width, _height, _depth, _voxel);
	verify("dome.qb");
}

TEST_F(ShapeGeneratorTest, testCreateCylinder) {
	voxel::RawVolumeWrapper wrapper(_volume);
	glm::ivec3 centerBottom = _center;
	centerBottom.y = _region.getLowerY();
	shape::createCylinder(wrapper, centerBottom, math::Axis::Y, _width / 2, _height, _voxel);
	verify("cylinder.qb");
}

}
