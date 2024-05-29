/**
 * @file
 */

#pragma once

#include "ModifierButton.h"
#include "ModifierType.h"
#include "Selection.h"
#include "core/IComponent.h"
#include "core/collection/Buffer.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxedit-util/modifier/brush/LineBrush.h"
#include "voxedit-util/modifier/brush/PaintBrush.h"
#include "voxedit-util/modifier/brush/PathBrush.h"
#include "voxedit-util/modifier/brush/PlaneBrush.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxedit-util/modifier/brush/StampBrush.h"
#include "voxedit-util/modifier/brush/TextBrush.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"

namespace voxedit {

class SceneManager;

/**
 * @brief This class is responsible for manipulating the volume with the configured @c Brush and for
 * doing the selection.
 *
 * There are several modes available. E.g. having the starting point of the aabb on a corner - or
 * at the center, mirroring the modifications and so on.
 */
class Modifier : public core::IComponent {
public:
	using Callback = std::function<void(const voxel::Region &region, ModifierType type, bool markUndo)>;

protected:
	Selections _selections;
	bool _selectionValid = false;
	bool _selectStartPositionValid = false;
	bool _locked = false;
	/**
	 * timer value which indicates the next execution time in case you keep the
	 * modifier triggered
	 */
	double _nextSingleExecution = 0;
	ModifierType _modifierType = ModifierType::Place;
	glm::ivec3 _selectStartPosition{0};

	core::Buffer<Brush *> _brushes;
	BrushContext _brushContext;
	BrushType _brushType = BrushType::Shape;
	PlaneBrush _planeBrush;
	ShapeBrush _shapeBrush;
	StampBrush _stampBrush;
	LineBrush _lineBrush;
	PathBrush _pathBrush;
	PaintBrush _paintBrush;
	TextBrush _textBrush;

	ModifierButton _actionExecuteButton;
	ModifierButton _deleteExecuteButton;

	void preExecuteBrush(const voxel::Region &targetVolumeRegion);
	void postExecuteBrush();

	/**
	 * @brief Execute the brush operation on the given node volume
	 */
	bool executeBrush(
		scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, ModifierType modifierType,
		const voxel::Voxel &voxel, const Callback &callback = [](const voxel::Region &, ModifierType, bool) {});

	voxel::Region calcSelectionRegion() const;

public:
	Modifier(SceneManager *sceneMgr);

	/**
	 * @brief Create a Raw Volume Wrapper object while taking the selection into account
	 */
	voxel::RawVolumeWrapper createRawVolumeWrapper(voxel::RawVolume *volume) const;

	void construct() override;
	bool init() override;
	void update(double nowSeconds);

	/**
	 * Allow to lock the modifier to not perform any modification
	 */
	void lock();
	void unlock();
	inline bool isLocked() const {
		return _locked;
	}

	math::Axis lockedAxis() const;
	void setLockedAxis(math::Axis axis, bool unlock);

	void shutdown() override;

	virtual bool select(const glm::ivec3 &mins, const glm::ivec3 &maxs);
	virtual void unselect();
	virtual void invert(const voxel::Region &region);
	const Selections &selections() const;

	ModifierType modifierType() const;
	ModifierType setModifierType(ModifierType type);

	bool isMode(ModifierType modifierType) const;

	const voxel::Voxel &cursorVoxel() const;
	virtual void setCursorVoxel(const voxel::Voxel &voxel);

	/**
	 * @brief Pick the start position of the modifier execution bounding box
	 */
	bool start();
	/**
	 * @brief End the current ModifierType execution and modify the given volume according to the type.
	 * @param[out,in] node The model node to modify
	 * @param callback Called for every region that was modified for the current active modifier.
	 * @note @c start() and @c stop() must be called before and after this method
	 */
	bool execute(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, const Callback &callback);
	void stop();
	/**
	 * @brief Actions could get aborted by some external action like hitting esc
	 */
	bool aborted() const;

	BrushType setBrushType(BrushType type);
	BrushType brushType() const;

	Brush *activeBrush();
	const AABBBrush *activeAABBBrush() const;
	AABBBrush *activeAABBBrush();
	voxel::Region calcBrushRegion();

	ShapeBrush &shapeBrush();
	TextBrush &textBrush();
	StampBrush &stampBrush();
	PathBrush &pathBrush();
	PaintBrush &paintBrush();
	BrushContext &brushContext();
	const BrushContext &brushContext() const;

	/**
	 * @sa needsFurtherAction()
	 */
	void executeAdditionalAction();
	/**
	 * @return @c true if the aabb that was formed has a side that is only 1 voxel
	 * high. This is our indicator for allowing to modify the aabb according to
	 * it's detected axis
	 * @sa executeAdditionalAction()
	 */
	bool needsFurtherAction();

	bool modifierTypeRequiresExistingVoxel() const;

	/**
	 * @param[in] pos The position inside the volume - given in absolute world coordinates
	 * @param[in] face The voxel::FaceNames values where the trace hits an existing voxel
	 */
	void setCursorPosition(const glm::ivec3 &pos, voxel::FaceNames face);
	const glm::ivec3 &cursorPosition() const;
	glm::ivec3 currentCursorPosition();

	const glm::ivec3 &referencePosition() const;
	virtual void setReferencePosition(const glm::ivec3 &pos);

	const voxel::Voxel &hitCursorVoxel() const;
	void setHitCursorVoxel(const voxel::Voxel &);
	void setVoxelAtCursor(const voxel::Voxel &voxel);

	voxel::FaceNames cursorFace() const;

	void setGridResolution(int gridSize);
	int gridResolution() const;

	void reset();
};

inline math::Axis Modifier::lockedAxis() const {
	return _brushContext.lockedAxis;
}

inline BrushContext &Modifier::brushContext() {
	return _brushContext;
}

inline const BrushContext &Modifier::brushContext() const {
	return _brushContext;
}

inline BrushType Modifier::brushType() const {
	return _brushType;
}

inline TextBrush &Modifier::textBrush() {
	return _textBrush;
}

inline ShapeBrush &Modifier::shapeBrush() {
	return _shapeBrush;
}

inline StampBrush &Modifier::stampBrush() {
	return _stampBrush;
}

inline PathBrush &Modifier::pathBrush() {
	return _pathBrush;
}

inline PaintBrush &Modifier::paintBrush() {
	return _paintBrush;
}

inline int Modifier::gridResolution() const {
	return _brushContext.gridResolution;
}

inline const voxel::Voxel &Modifier::hitCursorVoxel() const {
	return _brushContext.hitCursorVoxel;
}

inline const glm::ivec3 &Modifier::referencePosition() const {
	return _brushContext.referencePos;
}

inline ModifierType Modifier::modifierType() const {
	return _modifierType;
}

inline bool Modifier::isMode(ModifierType modifierType) const {
	return (_modifierType & modifierType) == modifierType;
}

inline bool Modifier::aborted() const {
	if (isMode(ModifierType::Select)) {
		return false;
	}
	if (const AABBBrush *brush = activeAABBBrush()) {
		return brush->aborted(_brushContext);
	}
	return false;
}

inline void Modifier::setCursorPosition(const glm::ivec3 &pos, voxel::FaceNames face) {
	_brushContext.cursorPosition = pos;
	_brushContext.cursorFace = face;
}

inline voxel::FaceNames Modifier::cursorFace() const {
	return _brushContext.cursorFace;
}

inline void Modifier::setCursorVoxel(const voxel::Voxel &voxel) {
	if (voxel::isAir(voxel.getMaterial())) {
		return;
	}
	_brushContext.cursorVoxel = voxel;
}

inline const voxel::Voxel &Modifier::cursorVoxel() const {
	return _brushContext.cursorVoxel;
}

inline const glm::ivec3 &Modifier::cursorPosition() const {
	return _brushContext.cursorPosition;
}

inline const Selections &Modifier::selections() const {
	return _selections;
}

} // namespace voxedit
