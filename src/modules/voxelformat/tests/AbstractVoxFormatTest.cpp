/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/BufferedReadWriteStream.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/MaterialColor.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/Format.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/FormatThumbnail.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelrender/ImageGenerator.h"
#include "voxelutil/VolumeVisitor.h"

#define WRITE_TO_FILE 0

namespace voxelformat {

const voxel::Voxel AbstractVoxFormatTest::Empty;

image::ImagePtr AbstractVoxFormatTest::testThumbnailCreator(const scenegraph::SceneGraph &sceneGraph,
															const ThumbnailContext &ctx) {
	return image::ImagePtr();
}

void AbstractVoxFormatTest::dump(const core::String &srcFilename, const scenegraph::SceneGraph &sceneGraph) {
	int i = 0;
	const core::String &prefix = core::string::extractFilename(srcFilename);
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		const core::String &file = core::string::format("%s-%02i-%s.txt", prefix.c_str(), i, node.name().c_str());
		const core::String &structName = core::string::format("model_%i", i);
		dump(structName, node.volume(), core::string::sanitizeFilename(file));
		++i;
	}
}

void AbstractVoxFormatTest::dump(const core::String &structName, voxel::RawVolume *v, const core::String &filename) {
	const io::FilePtr &file = open(filename, io::FileMode::SysWrite);
	ASSERT_TRUE(file->validHandle());
	io::FileStream stream(file);
	stream.writeString(core::string::format("struct %s {\n", structName.c_str()), false);
	stream.writeString("static core::SharedPtr<voxel::RawVolume> create() {\n", false);
	const glm::ivec3 &mins = v->region().getLowerCorner();
	const glm::ivec3 &maxs = v->region().getUpperCorner();
	stream.writeString(core::string::format("\tglm::ivec3 mins(%i, %i, %i);\n", mins.x, mins.y, mins.z), false);
	stream.writeString(core::string::format("\tglm::ivec3 maxs(%i, %i, %i);\n", maxs.x, maxs.y, maxs.z), false);
	stream.writeString("\tvoxel::Region region(mins, maxs);\n", false);
	stream.writeString("\tcore::SharedPtr<voxel::RawVolume> v = core::make_shared<voxel::RawVolume>(region);\n", false);
	voxelutil::visitVolume(*v, [&](int x, int y, int z, const voxel::Voxel &voxel) {
		stream.writeString(
			core::string::format("\tv->setVoxel(%i, %i, %i, voxel::createVoxel(voxel::VoxelType::Generic, %i));\n", x, y, z, voxel.getColor()),
			false);
	});
	stream.writeString("\treturn v;\n}\n};\n", false);
}

void AbstractVoxFormatTest::testFirstAndLastPaletteIndex(const core::String &filename, Format *format,
														 voxel::ValidateFlags flags) {
	voxel::Region region(glm::ivec3(0), glm::ivec3(1));
	voxel::RawVolume volume(region);
	EXPECT_TRUE(volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0)));
	EXPECT_TRUE(volume.setVoxel(0, 0, 1, voxel::createVoxel(voxel::VoxelType::Generic, 255)));
	io::BufferedReadWriteStream stream((int64_t)(10 * 1024 * 1024));
	scenegraph::SceneGraph sceneGraphsave(2);
	{
		scenegraph::SceneGraphNode node;
		node.setVolume(&volume, false);
		sceneGraphsave.emplace(core::move(node));
	}
	ASSERT_TRUE(format->save(sceneGraphsave, filename, stream, testSaveCtx));
	stream.seek(0);
	scenegraph::SceneGraph::MergedVolumePalette merged = load(filename, stream, *format);
	ASSERT_NE(nullptr, merged.first);
	core::ScopedPtr<voxel::RawVolume> loaded(merged.first);
	voxel::volumeComparator(volume, voxel::getPalette(), *loaded, merged.second, voxel::ValidateFlags::None);
}

void AbstractVoxFormatTest::testFirstAndLastPaletteIndexConversion(Format &srcFormat, const core::String &destFilename,
																   Format &destFormat, voxel::ValidateFlags flags) {
	voxel::Region region(glm::ivec3(0), glm::ivec3(1));
	voxel::RawVolume original(region);
	const palette::Palette pal1 = voxel::getPalette();
	EXPECT_TRUE(original.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0u)));
	EXPECT_TRUE(original.setVoxel(0, 0, 1, voxel::createVoxel(voxel::VoxelType::Generic, 255u)));
	io::BufferedReadWriteStream srcFormatStream((int64_t)(10 * 1024 * 1024));
	{
		scenegraph::SceneGraph sceneGraphsave(2);
		scenegraph::SceneGraphNode node;
		node.setVolume(&original, false);
		node.setPalette(pal1);
		sceneGraphsave.emplace(core::move(node));
		EXPECT_TRUE(srcFormat.save(sceneGraphsave, destFilename, srcFormatStream, testSaveCtx))
			<< "Could not save " << destFilename;
	}
	srcFormatStream.seek(0);
	scenegraph::SceneGraph::MergedVolumePalette merged = load(destFilename, srcFormatStream, srcFormat);
	ASSERT_NE(nullptr, merged.first);
	core::ScopedPtr<voxel::RawVolume> origReloaded(merged.first);
	voxel::volumeComparator(original, pal1, *origReloaded, merged.second, flags);

	io::BufferedReadWriteStream stream((int64_t)(10 * 1024 * 1024));
	{
		scenegraph::SceneGraph sceneGraphsave(2);
		scenegraph::SceneGraphNode node;
		node.setVolume(merged.first, false);
		node.setPalette(merged.second);
		sceneGraphsave.emplace(core::move(node));
		EXPECT_TRUE(destFormat.save(sceneGraphsave, destFilename, stream, testSaveCtx))
			<< "Could not save " << destFilename;
	}
	stream.seek(0);
	scenegraph::SceneGraph::MergedVolumePalette merged2 = load(destFilename, stream, destFormat);
	core::ScopedPtr<voxel::RawVolume> loaded(merged2.first);
	ASSERT_NE(nullptr, loaded) << "Could not load " << destFilename;
	voxel::volumeComparator(original, pal1, *loaded, merged2.second, flags);
}

void AbstractVoxFormatTest::canLoad(scenegraph::SceneGraph &sceneGraph, const core::String &filename,
									size_t expectedVolumes) {
	const io::FilePtr &file = open(filename);
	ASSERT_TRUE(file->validHandle()) << "Could not open " << filename;
	io::FileStream stream(file);
	io::FileDescription fileDesc;
	fileDesc.set(filename);
	ASSERT_TRUE(voxelformat::loadFormat(fileDesc, stream, sceneGraph, testLoadCtx)) << "Could not load " << filename;
	ASSERT_EQ(expectedVolumes, sceneGraph.size());
}

void AbstractVoxFormatTest::checkColor(core::RGBA c1, const palette::Palette &palette, uint8_t index, float maxDelta) {
	const core::RGBA c2 = palette.color(index);
	const float delta = core::Color::getDistance(c1, c2);
	ASSERT_LE(delta, maxDelta) << "color1[" << core::Color::print(c1) << "], color2[" << core::Color::print(c2)
							   << "], delta[" << delta << "]";
}

void AbstractVoxFormatTest::testRGBSmall(const core::String &filename, io::SeekableReadStream &stream,
										 scenegraph::SceneGraph &sceneGraph) {
	io::FileDescription fileDesc;
	fileDesc.set(filename);
	ASSERT_TRUE(voxelformat::loadFormat(fileDesc, stream, sceneGraph, testLoadCtx));
	EXPECT_EQ(1u, sceneGraph.size());

	palette::Palette palette;
	EXPECT_TRUE(palette.nippon());

	const core::RGBA red(255, 0, 0);
	const core::RGBA green(0, 255, 0);
	const core::RGBA blue(0, 0, 255);

	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		const voxel::RawVolume *volume = node.volume();
		EXPECT_EQ(3, voxelutil::visitVolume(*volume, [](int, int, int, const voxel::Voxel &) {}));
		checkColor(blue, node.palette(), volume->voxel(0, 0, 0).getColor(), 0.0f);
		checkColor(green, node.palette(), volume->voxel(1, 0, 0).getColor(), 0.0f);
		checkColor(red, node.palette(), volume->voxel(2, 0, 0).getColor(), 0.0f);
	}
}

void AbstractVoxFormatTest::testRGBSmall(const core::String &filename) {
	scenegraph::SceneGraph sceneGraph;
	const io::FilePtr &file = open(filename);
	ASSERT_TRUE(file->validHandle());
	io::FileStream stream(file);
	testRGBSmall(filename, stream, sceneGraph);
}

void AbstractVoxFormatTest::testRGBSmallSaveLoad(const core::String &filename) {
	const core::String formatExt = core::string::extractExtension(filename);
	const core::String saveFilename = "test." + formatExt;
	testRGBSmallSaveLoad(filename, saveFilename);
}

void AbstractVoxFormatTest::testLoadScreenshot(const core::String &filename, int width, int height,
											   const core::RGBA expectedColor, int expectedX, int expectedY) {
	io::FileStream loadStream(open(filename));
	ASSERT_TRUE(loadStream.valid());
	const image::ImagePtr &image = voxelformat::loadScreenshot(filename, loadStream, testLoadCtx);
	ASSERT_TRUE(image);
	EXPECT_EQ(image->width(), width) << image::print(image);
	EXPECT_EQ(image->height(), height) << image::print(image);
	const core::RGBA color = image->colorAt(expectedX, expectedY);
	EXPECT_EQ(color, expectedColor) << "expected " << core::Color::print(expectedColor) << " but got "
									<< core::Color::print(color) << "\n"
									<< image::print(image);
}

void AbstractVoxFormatTest::testRGBSmallSaveLoad(const core::String &filename, const core::String &saveFilename) {
	scenegraph::SceneGraph sceneGraph;
	{
		// load and check that the file contains the expected colors
		io::FileStream loadStream(open(filename));
		ASSERT_TRUE(loadStream.valid());
		testRGBSmall(filename, loadStream, sceneGraph);
	}

	io::BufferedReadWriteStream saveStream((int64_t)(10 * 1024 * 1024));
	ASSERT_TRUE(voxelformat::saveFormat(sceneGraph, saveFilename, nullptr, saveStream, testSaveCtx));
	saveStream.seek(0);

	scenegraph::SceneGraph loadSceneGraph;
	testRGBSmall(saveFilename, saveStream, loadSceneGraph);
}

void AbstractVoxFormatTest::testRGB(const core::String &filename, float maxDelta) {
	scenegraph::SceneGraph sceneGraph;
	const io::FilePtr &file = open(filename);
	ASSERT_TRUE(file->validHandle()) << "Could not open " << filename.c_str();
	io::FileStream stream(file);
	io::FileDescription fileDesc;
	fileDesc.set(filename);
	ASSERT_TRUE(voxelformat::loadFormat(fileDesc, stream, sceneGraph, testLoadCtx))
		<< "Failed to load " << filename.c_str();
	EXPECT_EQ(1u, sceneGraph.size()) << "Unexpected scene graph size for " << filename.c_str();

	palette::Palette palette;
	EXPECT_TRUE(palette.nippon());

	const core::RGBA red = palette.color(37);
	const core::RGBA green = palette.color(149);
	const core::RGBA blue = palette.color(197);

	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		const voxel::RawVolume *volume = node.volume();
		EXPECT_EQ(99, voxelutil::visitVolume(*volume, [](int, int, int, const voxel::Voxel &) {}));
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(0, 0, 0).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(31, 0, 0).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(31, 0, 31).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(0, 0, 31).getMaterial())
			<< "Failed rgb check for " << filename.c_str();

		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(0, 31, 0).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(31, 31, 0).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(31, 31, 31).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(0, 31, 31).getMaterial())
			<< "Failed rgb check for " << filename.c_str();

		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(9, 0, 4).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(9, 0, 12).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(9, 0, 19).getMaterial())
			<< "Failed rgb check for " << filename.c_str();

		checkColor(red, node.palette(), volume->voxel(9, 0, 4).getColor(), maxDelta);
		checkColor(green, node.palette(), volume->voxel(9, 0, 12).getColor(), maxDelta);
		checkColor(blue, node.palette(), volume->voxel(9, 0, 19).getColor(), maxDelta);
	}
}

void AbstractVoxFormatTest::testLoadSaveAndLoad(const core::String &srcFilename, Format &srcFormat,
												const core::String &destFilename, Format &destFormat,
												voxel::ValidateFlags flags, float maxDelta) {
	SCOPED_TRACE("src: " + srcFilename);
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(loadGroups(srcFilename, srcFormat, sceneGraph)) << "Failed to load " << srcFilename;

	io::SeekableReadStream *readStream;
	io::SeekableWriteStream *writeStream;

#if WRITE_TO_FILE
	io::FilePtr sfile = open(destFilename, io::FileMode::SysWrite);
	SCOPED_TRACE("target: " + sfile->name());
	io::FileStream fileWriteStream(sfile);
	writeStream = &fileWriteStream;
#else
	io::BufferedReadWriteStream bufferedStream((int64_t)(10 * 1024 * 1024));
	writeStream = &bufferedStream;
#endif

	ASSERT_TRUE(destFormat.save(sceneGraph, destFilename, *writeStream, testSaveCtx)) << "Could not save " << destFilename;

#if WRITE_TO_FILE
	sfile->close();
	io::FilePtr readfile = open(destFilename);
	io::FileStream fileReadStream(readfile);
	readStream = &fileReadStream;
#else
	readStream = &bufferedStream;
#endif

	readStream->seek(0);

	scenegraph::SceneGraph::MergedVolumePalette mergedLoad = load(destFilename, *readStream, destFormat);
	core::ScopedPtr<voxel::RawVolume> loaded(mergedLoad.first);
	ASSERT_NE(nullptr, loaded) << "Could not load " << destFilename;
	scenegraph::SceneGraph::MergedVolumePalette merged = sceneGraph.merge();
	core::ScopedPtr<voxel::RawVolume> src(merged.first);
	if ((flags & voxel::ValidateFlags::Palette) == voxel::ValidateFlags::Palette) {
		voxel::paletteComparator(merged.second, mergedLoad.second, maxDelta);
	} else if ((flags & voxel::ValidateFlags::PaletteMinMatchingColors) == voxel::ValidateFlags::PaletteMinMatchingColors) {
		voxel::partialPaletteComparator(merged.second, mergedLoad.second, maxDelta);
	} else if ((flags & voxel::ValidateFlags::PaletteColorsScaled) == voxel::ValidateFlags::PaletteColorsScaled) {
		voxel::paletteComparatorScaled(merged.second, mergedLoad.second, (int)maxDelta);
	} else if ((flags & voxel::ValidateFlags::PaletteColorOrderDiffers) == voxel::ValidateFlags::PaletteColorOrderDiffers) {
		voxel::orderPaletteComparator(merged.second, mergedLoad.second, maxDelta);
	}
	voxel::volumeComparator(*src, merged.second, *loaded, mergedLoad.second, flags, maxDelta);
}

void AbstractVoxFormatTest::testLoadSceneGraph(const core::String &srcFilename1, Format &srcFormat1,
									 const core::String &srcFilename2, Format &srcFormat2, voxel::ValidateFlags flags,
									 float maxDelta) {
	SCOPED_TRACE("src1: " + srcFilename1);
	scenegraph::SceneGraph srcSceneGraph1;
	ASSERT_TRUE(loadGroups(srcFilename1, srcFormat1, srcSceneGraph1)) << "Failed to load " << srcFilename1;
	SCOPED_TRACE("src2: " + srcFilename2);
	scenegraph::SceneGraph srcSceneGraph2;
	ASSERT_TRUE(loadGroups(srcFilename2, srcFormat2, srcSceneGraph2)) << "Failed to load " << srcFilename2;
	voxel::sceneGraphComparator(srcSceneGraph1, srcSceneGraph2, flags, maxDelta);
}

void AbstractVoxFormatTest::testLoadSaveAndLoadSceneGraph(const core::String &srcFilename, Format &srcFormat,
														  const core::String &destFilename, Format &destFormat,
														  voxel::ValidateFlags flags, float maxDelta) {
	SCOPED_TRACE("src: " + srcFilename);
	SCOPED_TRACE("target: " + destFilename);
	scenegraph::SceneGraph srcSceneGraph;
	ASSERT_TRUE(loadGroups(srcFilename, srcFormat, srcSceneGraph)) << "Failed to load " << srcFilename;
#if WRITE_TO_FILE
	{
		io::FileStream stream(open(destFilename, io::FileMode::SysWrite));
		ASSERT_TRUE(destFormat.save(srcSceneGraph, destFilename, stream, testSaveCtx))
			<< "Could not save " << destFilename;
	}
#else
	io::BufferedReadWriteStream stream((int64_t)(10 * 1024 * 1024));
	ASSERT_TRUE(destFormat.save(srcSceneGraph, destFilename, stream, testSaveCtx)) << "Could not save " << destFilename;
	stream.seek(0);
#endif
	scenegraph::SceneGraph destSceneGraph;
#if WRITE_TO_FILE
	{
		io::FileStream stream(open(destFilename));
		ASSERT_TRUE(destFormat.load(destFilename, stream, destSceneGraph, testLoadCtx))
			<< "Failed to load the target format";
	}
#else
	ASSERT_TRUE(destFormat.load(destFilename, stream, destSceneGraph, testLoadCtx))
		<< "Failed to load the target format";
#endif
	voxel::sceneGraphComparator(srcSceneGraph, destSceneGraph, flags, maxDelta);
}

void AbstractVoxFormatTest::testSaveSingleVoxel(const core::String &filename, Format *format) {
	voxel::Region region(glm::ivec3(0), glm::ivec3(0));
	voxel::RawVolume original(region);
	original.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	io::BufferedReadWriteStream bufferedStream((int64_t)(10 * 1024 * 1024));
	scenegraph::SceneGraph sceneGraphsave(2);
	{
		scenegraph::SceneGraphNode node;
		node.setVolume(&original, false);
		sceneGraphsave.emplace(core::move(node));
	}
	ASSERT_TRUE(format->save(sceneGraphsave, filename, bufferedStream, testSaveCtx));
	bufferedStream.seek(0);
	scenegraph::SceneGraph::MergedVolumePalette mergedLoad = load(filename, bufferedStream, *format);
	core::ScopedPtr<voxel::RawVolume> loaded(mergedLoad.first);
	ASSERT_NE(nullptr, loaded) << "Could not load single voxel file " << filename;
	voxel::volumeComparator(original, voxel::getPalette(), *loaded, mergedLoad.second,
							voxel::ValidateFlags::Color | voxel::ValidateFlags::Region);
}

void AbstractVoxFormatTest::testSaveSmallVolume(const core::String &filename, Format *format) {
	palette::Palette pal;
	pal.magicaVoxel();
	voxel::Region region(glm::ivec3(0), glm::ivec3(0, 1, 1));
	voxel::RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 0, voxel::createVoxel(pal, 0)));
	ASSERT_TRUE(original.setVoxel(0, 0, 1, voxel::createVoxel(pal, 200)));
	ASSERT_TRUE(original.setVoxel(0, 1, 1, voxel::createVoxel(pal, 201)));
	ASSERT_TRUE(original.setVoxel(0, 0, 0, voxel::createVoxel(pal, pal.colorCount() - 1)));
	io::BufferedReadWriteStream bufferedStream((int64_t)(10 * 1024 * 1024));
	scenegraph::SceneGraph sceneGraphsave(2);
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(&original, false);
		node.setPalette(pal);
		sceneGraphsave.emplace(core::move(node));
	}
	ASSERT_TRUE(format->save(sceneGraphsave, filename, bufferedStream, testSaveCtx));
	bufferedStream.seek(0);
	scenegraph::SceneGraph::MergedVolumePalette mergedLoad = load(filename, bufferedStream, *format);
	core::ScopedPtr<voxel::RawVolume> loaded(mergedLoad.first);
	ASSERT_NE(nullptr, loaded) << "Could not load single voxel file " << filename;
	voxel::volumeComparator(original, pal, *loaded, mergedLoad.second,
							voxel::ValidateFlags::Color | voxel::ValidateFlags::Region);
}

void AbstractVoxFormatTest::testSaveMultipleModels(const core::String &filename, Format *format) {
	voxel::Region region(glm::ivec3(0), glm::ivec3(0));
	voxel::RawVolume model1(region);
	voxel::RawVolume model2(region);
	voxel::RawVolume model3(region);
	voxel::RawVolume model4(region);
	EXPECT_TRUE(model1.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(model2.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(model3.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(model4.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode node1;
	node1.setVolume(&model1, false);
	scenegraph::SceneGraphNode node2;
	node2.setVolume(&model2, false);
	scenegraph::SceneGraphNode node3;
	node3.setVolume(&model3, false);
	scenegraph::SceneGraphNode node4;
	node4.setVolume(&model4, false);
	sceneGraph.emplace(core::move(node1));
	sceneGraph.emplace(core::move(node2));
	sceneGraph.emplace(core::move(node3));
	sceneGraph.emplace(core::move(node4));
	io::BufferedReadWriteStream bufferedStream((int64_t)(10 * 1024 * 1024));
	ASSERT_TRUE(format->save(sceneGraph, filename, bufferedStream, testSaveCtx));
	bufferedStream.seek(0);
	scenegraph::SceneGraph sceneGraphLoad;
	EXPECT_TRUE(format->load(filename, bufferedStream, sceneGraphLoad, testLoadCtx));
	EXPECT_EQ(sceneGraphLoad.size(), sceneGraph.size());
}

void AbstractVoxFormatTest::testSave(const core::String &filename, Format *format) {
	voxel::Region region(glm::ivec3(0), glm::ivec3(0));
	voxel::RawVolume model1(region);
	EXPECT_TRUE(model1.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode node1;
	node1.setVolume(&model1, false);
	sceneGraph.emplace(core::move(node1));
	io::BufferedReadWriteStream bufferedStream((int64_t)(10 * 1024 * 1024));
	ASSERT_TRUE(format->save(sceneGraph, filename, bufferedStream, testSaveCtx));
	scenegraph::SceneGraph sceneGraphLoad;
	bufferedStream.seek(0);
	EXPECT_TRUE(format->load(filename, bufferedStream, sceneGraphLoad, testLoadCtx));
	EXPECT_EQ(sceneGraphLoad.size(), sceneGraph.size());
}

void AbstractVoxFormatTest::testSaveLoadVoxel(const core::String &filename, Format *format, int mins, int maxs,
											  voxel::ValidateFlags flags) {
	const voxel::Region region(mins, maxs);
	voxel::RawVolume original(region);

	original.setVoxel(mins, mins, mins, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(mins, mins, maxs, voxel::createVoxel(voxel::VoxelType::Generic, 244));
	original.setVoxel(mins, maxs, maxs, voxel::createVoxel(voxel::VoxelType::Generic, 126));
	original.setVoxel(mins, maxs, mins, voxel::createVoxel(voxel::VoxelType::Generic, 254));

	original.setVoxel(maxs, maxs, maxs, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	original.setVoxel(maxs, maxs, mins, voxel::createVoxel(voxel::VoxelType::Generic, 245));
	original.setVoxel(maxs, mins, mins, voxel::createVoxel(voxel::VoxelType::Generic, 127));
	original.setVoxel(maxs, mins, maxs, voxel::createVoxel(voxel::VoxelType::Generic, 200));

	testSaveLoadVolume(filename, original, format, flags);
}

void AbstractVoxFormatTest::testSaveLoadVolume(const core::String &filename, const voxel::RawVolume &original,
											   Format *format, voxel::ValidateFlags flags, float maxDelta) {
	palette::Palette pal;
	pal.magicaVoxel();

	scenegraph::SceneGraph sceneGraph;
	int nodeId = 0;
	{
		scenegraph::SceneGraphNode node;
		node.setName("first level #1");
		node.setVolume(&original);
		node.setPalette(pal);
		nodeId = sceneGraph.emplace(core::move(node), nodeId);
	}
	{
		scenegraph::SceneGraphNode node;
		node.setName("second level #1");
		node.setVolume(&original);
		node.setPalette(pal);
		sceneGraph.emplace(core::move(node), nodeId);
	}
	{
		scenegraph::SceneGraphNode node;
		node.setName("second level #2");
		node.setVolume(&original);
		node.setPalette(pal);
		/*nodeId =*/sceneGraph.emplace(core::move(node), nodeId);
	}

	io::SeekableReadStream *readStream;
	io::SeekableWriteStream *writeStream;

#if WRITE_TO_FILE
	io::FilePtr sfile = open(filename, io::FileMode::SysWrite);
	io::FileStream fileWriteStream(sfile);
	writeStream = &fileWriteStream;
#else
	io::BufferedReadWriteStream bufferedStream((int64_t)(10 * 1024 * 1024));
	writeStream = &bufferedStream;
#endif

	ASSERT_TRUE(format->save(sceneGraph, filename, *writeStream, testSaveCtx)) << "Could not save the scene graph";

#if WRITE_TO_FILE
	sfile->close();
	io::FilePtr readfile = open(filename);
	io::FileStream fileReadStream(readfile);
	readStream = &fileReadStream;
#else
	readStream = &bufferedStream;
#endif

	readStream->seek(0);

	scenegraph::SceneGraph::MergedVolumePalette merged = load(filename, *readStream, *format);
	core::ScopedPtr<voxel::RawVolume> loaded(merged.first);
	ASSERT_NE(nullptr, loaded) << "Could not load the merged volumes";
	if ((flags & voxel::ValidateFlags::Palette) == voxel::ValidateFlags::Palette) {
		voxel::paletteComparator(pal, merged.second, maxDelta);
	} else if ((flags & voxel::ValidateFlags::PaletteMinMatchingColors) == voxel::ValidateFlags::PaletteMinMatchingColors) {
		voxel::partialPaletteComparator(pal, merged.second, maxDelta);
	} else if ((flags & voxel::ValidateFlags::PaletteColorOrderDiffers) == voxel::ValidateFlags::PaletteColorOrderDiffers) {
		voxel::orderPaletteComparator(pal, merged.second, maxDelta);
	}
	voxel::volumeComparator(original, pal, *loaded, merged.second, flags);
}

#undef WRITE_TO_FILE

io::FilePtr AbstractVoxFormatTest::open(const core::String &filename, io::FileMode mode) {
	const io::FilePtr &file = io::filesystem()->open(core::String(filename), mode);
	return file;
}

scenegraph::SceneGraph::MergedVolumePalette
AbstractVoxFormatTest::load(const core::String &filename, io::SeekableReadStream &stream, Format &format) {
	scenegraph::SceneGraph sceneGraph;
	if (!format.load(filename, stream, sceneGraph, testLoadCtx)) {
		Log::error("Failed to load %s", filename.c_str());
		return scenegraph::SceneGraph::MergedVolumePalette{};
	}
	if (sceneGraph.empty()) {
		Log::error("Success - but no nodes");
		return scenegraph::SceneGraph::MergedVolumePalette{};
	}
	Log::debug("Loaded %s - merging", filename.c_str());
	return sceneGraph.merge();
}

scenegraph::SceneGraph::MergedVolumePalette AbstractVoxFormatTest::load(const core::String &filename, Format &format) {
	scenegraph::SceneGraph sceneGraph;
	if (!loadGroups(filename, format, sceneGraph)) {
		return scenegraph::SceneGraph::MergedVolumePalette{};
	}
	if (sceneGraph.empty()) {
		Log::error("Success - but no nodes");
		return scenegraph::SceneGraph::MergedVolumePalette{};
	}
	return sceneGraph.merge();
}

bool AbstractVoxFormatTest::loadGroups(const core::String &filename, Format &format,
									   scenegraph::SceneGraph &sceneGraph) {
	const io::FilePtr &file = open(filename);
	if (!file->validHandle()) {
		Log::error("Could not open %s for reading", filename.c_str());
		return false;
	}
	io::FileStream stream(file);
	return format.load(filename, stream, sceneGraph, testLoadCtx);
}

int AbstractVoxFormatTest::loadPalette(const core::String &filename, Format &format, palette::Palette &palette) {
	const io::FilePtr &file = open(filename);
	if (!file->validHandle()) {
		Log::error("Could not open %s for reading the palette", filename.c_str());
		return 0;
	}
	io::FileStream stream(file);
	const int size = (int)format.loadPalette(filename, stream, palette, testLoadCtx);
	const core::String paletteFilename = core::string::extractFilename(filename) + ".png";
	palette.save(paletteFilename.c_str());
	return size;
}

bool AbstractVoxFormatTest::onInitApp() {
	if (!AbstractVoxelTest::onInitApp()) {
		return false;
	}
	FormatConfig::init();
	voxel::getPalette().nippon();
	return true;
}

} // namespace voxelformat
