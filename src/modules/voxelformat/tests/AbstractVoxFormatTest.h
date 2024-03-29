/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "voxel/tests/AbstractVoxelTest.h"
#include "voxelformat/Format.h"
#include "scenegraph/SceneGraph.h"
#include "TestHelper.h"

namespace voxelformat {

class AbstractVoxFormatTest: public voxel::AbstractVoxelTest {
protected:
	static const voxel::Voxel Empty;

	SaveContext testSaveCtx;
	LoadContext testLoadCtx;
	AbstractVoxFormatTest() {
		testSaveCtx.thumbnailCreator = testThumbnailCreator;
	}

	static image::ImagePtr testThumbnailCreator(const scenegraph::SceneGraph &sceneGraph, const ThumbnailContext &ctx);

	void checkColor(core::RGBA, const palette::Palette &palette, uint8_t index, float maxDelta);

	void dump(const core::String &srcFilename, const scenegraph::SceneGraph &sceneGraph);
	void dump(const core::String &structName, voxel::RawVolume *v, const core::String &filename);

	void testFirstAndLastPaletteIndex(const core::String &filename, Format *format, voxel::ValidateFlags flags);
	void testFirstAndLastPaletteIndexConversion(Format &srcFormat, const core::String &destFilename, Format &destFormat,
												voxel::ValidateFlags flags = voxel::ValidateFlags::All);

	void canLoad(scenegraph::SceneGraph &sceneGraph, const core::String &filename, size_t expectedVolumes = 1);
	void canLoad(const core::String &filename, size_t expectedVolumes = 1) {
		scenegraph::SceneGraph sceneGraph;
		canLoad(sceneGraph, filename, expectedVolumes);
	}
	void testRGB(const core::String &filename, float maxDelta = 0.001f);
	void testRGBSmall(const core::String &filename, io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph);
	void testRGBSmall(const core::String &filename);
	// save as the same format
	void testRGBSmallSaveLoad(const core::String &filename);
	void testLoadScreenshot(const core::String &filename, int width, int height, const core::RGBA expectedColor, int expectedX, int expectedY);
	// save as any other format
	void testRGBSmallSaveLoad(const core::String &filename, const core::String &saveFilename);

	void testSaveMultipleModels(const core::String &filename, Format *format);
	void testSaveSingleVoxel(const core::String &filename, Format *format);
	void testSaveSmallVolume(const core::String &filename, Format *format);

	void testSave(const core::String &filename, Format *format, const palette::Palette &palette, voxel::ValidateFlags flags = voxel::ValidateFlags::All);

	void testSaveLoadVoxel(const core::String &filename, Format *format, int mins = 0, int maxs = 1,
						   voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette));
	void testSaveLoadVolume(const core::String &filename, const voxel::RawVolume &v, Format *format,
							voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette), float maxDelta = 0.001f);

	void testLoadSaveAndLoad(const core::String &srcFilename, Format &srcFormat, const core::String &destFilename,
							 Format &destFormat, voxel::ValidateFlags flags = voxel::ValidateFlags::All, float maxDelta = 0.001f);
	void testLoadSaveAndLoadSceneGraph(const core::String &srcFilename, Format &srcFormat,
									   const core::String &destFilename, Format &destFormat, voxel::ValidateFlags flags = voxel::ValidateFlags::All,
									   float maxDelta = 0.001f);
	void testLoadSceneGraph(const core::String &srcFilename1, Format &srcFormat1, const core::String &srcFilename2,
							Format &srcFormat2, voxel::ValidateFlags flags = voxel::ValidateFlags::All, float maxDelta = 0.001f);

	io::FilePtr open(const core::String &filename, io::FileMode mode = io::FileMode::Read);

	scenegraph::SceneGraph::MergedVolumePalette load(const core::String& filename, io::SeekableReadStream& stream, Format& format);

	scenegraph::SceneGraph::MergedVolumePalette load(const core::String& filename, Format& format);

	bool loadGroups(const core::String& filename, Format& format, scenegraph::SceneGraph &sceneGraph);

	int loadPalette(const core::String& filename, Format& format, palette::Palette &palette);

	/**
	 * @brief Not a test, but a helper method to store a scenegraph for manual inspection
	 */
	bool saveSceneGraph(scenegraph::SceneGraph &sceneGraph, const core::String &filename);

	bool onInitApp() override;
};

}
