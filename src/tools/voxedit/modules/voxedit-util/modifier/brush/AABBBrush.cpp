/**
 * @file
 */

#include "AABBBrush.h"
#include "command/Command.h"
#include "core/Log.h"
#include "app/I18N.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/Face.h"
#include "voxel/Region.h"

namespace voxedit {

AABBBrush::AABBBrush(BrushType type, ModifierType defaultModifier, ModifierType supportedModifiers)
	: Super(type, defaultModifier, supportedModifiers) {
}

void AABBBrush::construct() {
	Super::construct();
	const core::String &cmdName = name().toLower() + "brush";
	command::Command::registerCommand("set" + cmdName + "center", [this](const command::CmdArgs &args) {
		setMode(BRUSH_MODE_CENTER);
	}).setHelp(_("Set center plane building"));

	command::Command::registerCommand("set" + cmdName + "aabb", [this](const command::CmdArgs &args) {
		setMode(BRUSH_MODE_AABB);
	}).setHelp(_("Set default aabb voxel building mode"));

	command::Command::registerCommand("set" + cmdName + "single", [this](const command::CmdArgs &args) {
		setMode(BRUSH_MODE_SINGLE);
	}).setHelp(_("Set single voxel building mode - continue setting voxels until you release the action button"));
}

void AABBBrush::reset() {
	Super::reset();
	_secondPosValid = false;
	_aabbMode = false;
	_mode = 0u;
	_aabbFace = voxel::FaceNames::Max;
	_aabbFirstPos = glm::ivec3(0);
	_aabbSecondPos = glm::ivec3(0);
}

glm::ivec3 AABBBrush::applyGridResolution(const glm::ivec3 &inPos, int resolution) const {
	glm::ivec3 pos = inPos;
	if (pos.x % resolution != 0) {
		pos.x = (pos.x / resolution) * resolution;
	}
	if (pos.y % resolution != 0) {
		pos.y = (pos.y / resolution) * resolution;
	}
	if (pos.z % resolution != 0) {
		pos.z = (pos.z / resolution) * resolution;
	}
	return pos;
}

bool AABBBrush::needsAdditionalAction(const BrushContext &context) const {
	if (radius() > 0 || context.lockedAxis != math::Axis::None) {
		return false;
	}
	const voxel::Region &region = calcRegion(context);
	const glm::ivec3 &delta = region.getDimensionsInVoxels();
	int greater = 0;
	int equal = 0;
	for (int i = 0; i < 3; ++i) {
		if (delta[i] > context.gridResolution) {
			++greater;
		} else if (delta[i] == context.gridResolution) {
			++equal;
		}
	}
	return greater == 2 && equal == 1;
}

voxel::Region AABBBrush::extendRegionInOrthoMode(const voxel::Region &brushRegion, const voxel::Region &volumeRegion, const BrushContext &context) const {
	if (context.fixedOrthoSideView) {
		if (radius() > 0) {
			// TODO:
			return brushRegion;
		}
		glm::ivec3 mins = brushRegion.getLowerCorner();
		glm::ivec3 maxs = brushRegion.getUpperCorner();
		switch (context.cursorFace) {
		case voxel::FaceNames::PositiveX:
		case voxel::FaceNames::NegativeX:
			mins.x = volumeRegion.getLowerX();
			maxs.x = volumeRegion.getUpperX();
			break;
		case voxel::FaceNames::PositiveY:
		case voxel::FaceNames::NegativeY:
			mins.y = volumeRegion.getLowerY();
			maxs.y = volumeRegion.getUpperY();
			break;
		case voxel::FaceNames::PositiveZ:
		case voxel::FaceNames::NegativeZ:
			mins.z = volumeRegion.getLowerZ();
			maxs.z = volumeRegion.getUpperZ();
			break;
		default:
			break;
		}
		Log::debug("extend region in fixed ortho side view: %s to mins: %i:%i:%i, maxs: %i:%i:%i, face: %i",
				   brushRegion.toString().c_str(), mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z,
				   (int)context.cursorFace);
		return voxel::Region{mins, maxs};
	}
	return brushRegion;
}

bool AABBBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						const BrushContext &context) {
	voxel::Region region = calcRegion(context);
	region = extendRegionInOrthoMode(region, wrapper.region(), context);
	glm::ivec3 minsMirror = region.getLowerCorner();
	glm::ivec3 maxsMirror = region.getUpperCorner();
	if (!getMirrorAABB(minsMirror, maxsMirror)) {
		generate(sceneGraph, wrapper, context, region);
	} else {
		Log::debug("Execute mirror action");
		const voxel::Region second(minsMirror, maxsMirror);
		if (voxel::intersects(region, second)) {
			generate(sceneGraph, wrapper, context, voxel::Region(region.getLowerCorner(), maxsMirror));
		} else {
			generate(sceneGraph, wrapper, context, region);
			generate(sceneGraph, wrapper, context, second);
		}
	}
	markDirty();
	return true;
}

glm::ivec3 AABBBrush::currentCursorPosition(const BrushContext &brushContext) const {
	glm::ivec3 pos = brushContext.cursorPosition;
	if (_secondPosValid) {
		if (radius() > 0) {
			return _aabbSecondPos;
		}
		const math::Axis axis = voxel::faceToAxis(_aabbFace);
		if (axis != math::Axis::None) {
			const int idx = math::getIndexForAxis(axis);
			pos[(idx + 1) % 3] = _aabbSecondPos[(idx + 1) % 3];
			pos[(idx + 2) % 3] = _aabbSecondPos[(idx + 2) % 3];
		}
	}
	return pos;
}

bool AABBBrush::wantAABB() const {
	return !singleMode();
}

bool AABBBrush::start(const BrushContext &context) {
	if (_aabbMode) {
		return false;
	}

	// the order here matters - don't change _aabbMode earlier here
	_aabbFirstPos = applyGridResolution(context.cursorPosition, context.gridResolution);
	_secondPosValid = false;
	_aabbMode = wantAABB();
	_aabbFace = context.cursorFace;
	markDirty();
	return true;
}

void AABBBrush::update(const BrushContext &ctx, double nowSeconds) {
	Super::update(ctx, nowSeconds);

	// in single mode we want to update the preview each time we move the cursor
	if (radius() > 0 && ctx.cursorPosition != _aabbFirstPos) {
		markDirty();
	}
	if (_aabbMode && ctx.cursorPosition != _aabbSecondPos) {
		markDirty();
	}
}

bool AABBBrush::active() const {
	return _aabbMode || singleMode();
}

bool AABBBrush::aborted(const BrushContext &context) const {
	return _aabbFace == voxel::FaceNames::Max && context.lockedAxis == math::Axis::None;
}

void AABBBrush::step(const BrushContext &context) {
	if (!_aabbMode || radius() > 0 || context.lockedAxis != math::Axis::None) {
		return;
	}
	_aabbSecondPos = currentCursorPosition(context);
	// TODO: why is this set again?
	_aabbFirstPos = applyGridResolution(_aabbFirstPos, context.gridResolution);
	_secondPosValid = true;
	markDirty();
}

void AABBBrush::stop(const BrushContext &context) {
	_secondPosValid = false;
	_aabbMode = false;
	_aabbFace = voxel::FaceNames::Max;
	markDirty();
}

bool AABBBrush::isMode(uint32_t mode) const {
	return _mode == mode;
}

void AABBBrush::setMode(uint32_t mode) {
	_mode = mode;
}

void AABBBrush::setRadius(int radius) {
	_radius = glm::abs(radius);
	markDirty();
}

voxel::Region AABBBrush::calcRegion(const BrushContext &context) const {
	const glm::ivec3 &pos = currentCursorPosition(context);
	if (!singleMode() && centerMode()) {
		const glm::ivec3 &first = applyGridResolution(_aabbFirstPos, context.gridResolution);
		const glm::ivec3 &delta = glm::abs(pos - first);
		return voxel::Region(first - delta, first + delta);
	}
	const glm::ivec3 &first = singleMode() ? pos : applyGridResolution(_aabbFirstPos, context.gridResolution);
	const int rad = radius();
	if (rad > 0) {
		// TODO: _radius should only go into one direction (see BrushContext::_cursorFace) (only paint the surface)
		return voxel::Region(first - rad, first + rad);
	}

	const int size = context.gridResolution;
	const glm::ivec3 &mins = (glm::min)(first, pos);
	const glm::ivec3 &maxs = (glm::max)(first, pos) + (size - 1);
	return voxel::Region(mins, maxs);
}

} // namespace voxedit
