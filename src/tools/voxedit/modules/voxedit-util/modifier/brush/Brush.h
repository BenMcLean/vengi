/**
 * @file
 */

#pragma once

#include "BrushType.h"
#include "core/DirtyState.h"
#include "core/IComponent.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"

namespace scenegraph {
class SceneGraph;
}

namespace voxedit {

class ModifierVolumeWrapper;

struct BrushContext {
	/** the voxel that should get placed */
	voxel::Voxel cursorVoxel;
	/** existing voxel under the cursor */
	voxel::Voxel hitCursorVoxel;
	/** the voxel where the cursor is - can be air */
	voxel::Voxel voxelAtCursor;

	glm::ivec3 referencePos{0};
	glm::ivec3 cursorPosition{0};
	/** the face where the trace hit */
	voxel::FaceNames cursorFace = voxel::FaceNames::Max;
	math::Axis lockedAxis = math::Axis::None;

	// brushes that e.g. span an aabb behave differently if the view is fixed and in ortho mode. As you don't have the
	// chance to really span the aabb by given the mins and maxs.
	bool fixedOrthoSideView = false;
	int gridResolution = 1;
};

/**
 * @brief Base class for all brushes
 * @sa ModifierVolumeWrapper
 */
class Brush : public core::IComponent, public core::DirtyState {
protected:
	const BrushType _brushType;
	const ModifierType _defaultModifier;
	const ModifierType _supportedModifiers;

	Brush(BrushType brushType, ModifierType defaultModifier = ModifierType::Place,
		  ModifierType supportedModifiers = (ModifierType::Place | ModifierType::Erase | ModifierType::Override))
		: _brushType(brushType), _defaultModifier(defaultModifier), _supportedModifiers(supportedModifiers) {
	}

public:
	/**
	 * @brief Execute the brush action on the given volume
	 * @return @c true if the brush action was successful
	 */
	virtual bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						 const BrushContext &ctx) = 0;
	/**
	 * @brief Reset the brush state and force a re-creating of the preview volume
	 */
	virtual void reset();
	/**
	 * @brief E.g. checks whether the brush preview volume needs to be updated
	 * @sa markDirty()
	 */
	virtual void update(const BrushContext &ctx, double nowSeconds);
	/**
	 * @return the type of the brush as string
	 * @sa type()
	 */
	core::String name() const;
	/**
	 * @sa name()
	 */
	BrushType type() const;

	/**
	 * @brief allow to change the modifier type if the brush doesn't support the given mode
	 * @return the supported modifier type
	 */
	ModifierType modifierType(ModifierType type = ModifierType::None) const;

	/**
	 * @brief Determine whether the brush should get rendered
	 */
	virtual bool active() const;
	bool init() override;
	void shutdown() override;
};

inline BrushType Brush::type() const {
	return _brushType;
}

inline core::String Brush::name() const {
	return BrushTypeStr[(int)_brushType];
}

} // namespace voxedit
