/**
 * @file
 */

#pragma once

#include "../ShapeType.h"
#include "AABBBrush.h"

namespace voxedit {

class ShapeBrush : public AABBBrush {
private:
	using Super = AABBBrush;

protected:
	ShapeType _shapeType = ShapeType::AABB;
	bool generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const voxel::Region &region,
				  const voxel::Voxel &voxel) override;
	void setShapeType(ShapeType type);

public:
	ShapeBrush() : Super("shapebrush") {
	}
	virtual ~ShapeBrush() = default;
	void construct() override;
	void reset() override;

	ShapeType shapeType() const;
};

} // namespace voxedit
