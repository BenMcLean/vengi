/**
 * @file
 */

#include "PaintBrush.h"
#include "core/Color.h"
#include "palette/Palette.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"
#include "core/GLM.h"

namespace voxedit {

voxel::Voxel PaintBrush::VoxelColor::evaluate(const voxel::Voxel &old) {
	if (_paintMode == PaintMode::Replace) {
		return _voxel;
	}
	if (_paintMode == PaintMode::Random) {
		int n = _palette.colorCount();
		int idx = rand() % n;
		return voxel::Voxel(old.getMaterial(), idx, old.getFlags());
	}

	bool brighten = _paintMode == PaintMode::Brighten;
	if (_paintMode == PaintMode::Variation) {
		if (rand() % _variationThreshold != 0) {
			return old;
		}
		brighten = rand() % 2 == 0;
	}

	const core::RGBA voxelColor = _palette.color(old.getColor());
	core::RGBA newColor;
	if (brighten) {
		newColor = core::Color::brighter(voxelColor, _factor);
	} else {
		newColor = core::Color::darker(voxelColor, _factor);
	}
	const int index = _palette.getClosestMatch(newColor);
	if (index == palette::PaletteColorNotFound) {
		return old;
	}
	return voxel::Voxel(old.getMaterial(), index, old.getFlags());
}

bool PaintBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						  const BrushContext &context, const voxel::Region &region) {
	if (_plane) {
		voxelutil::paintPlane(wrapper, context.cursorPosition, context.cursorFace, context.hitCursorVoxel,
							  context.cursorVoxel);
	} else {
		VoxelColor voxelColor(wrapper.node().palette(), context.cursorVoxel, _paintMode, _factor, _variationThreshold);
		auto visitor = [&](int x, int y, int z, const voxel::Voxel &voxel) {
			wrapper.setVoxel(x, y, z, voxelColor.evaluate(voxel));
		};
		voxelutil::visitVolume(wrapper, region, visitor, voxelutil::SkipEmpty());
	}

	return true;
}

void PaintBrush::setFactor(float factor) {
	_factor = glm::clamp(factor, 0.1f, 10.0f);
}

void PaintBrush::setVariationThreshold(int variationThreshold) {
	_variationThreshold = glm::clamp(variationThreshold, 2, 20);
}

} // namespace voxedit
