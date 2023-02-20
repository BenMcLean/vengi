/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "math/Axis.h"
#include "render/ShapeRenderer.h"
#include "video/ShapeBuilder.h"
#include "../modifier/Selection.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"

namespace voxedit {

class ModifierRenderer : public core::IComponent {
private:
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	int32_t _aabbMeshIndex = -1;
	int32_t _selectionIndex = -1;
	int32_t _mirrorMeshIndex = -1;
	int32_t _voxelCursorMesh = -1;
	int32_t _referencePointMesh = -1;
	glm::mat4 _referencePointModelMatrix{1.0f};

public:
	bool init() override;
	void shutdown() override;

	void render(const video::Camera& camera, const glm::mat4& model);
	void renderAABBMode(const video::Camera& camera);
	void renderSelection(const video::Camera& camera);

	void updateReferencePosition(const glm::ivec3 &pos);
	void updateAABBMesh(const glm::vec3& mins, const glm::vec3& maxs);
	void updateAABBMirrorMesh(const glm::vec3 &mins, const glm::vec3 &maxs, const glm::vec3 &minsMirror,
							  const glm::vec3 &maxsMirror);
	void updateMirrorPlane(math::Axis axis, const glm::ivec3 &mirrorPos);
	void updateSelectionBuffers(const Selection &selection);
	void updateCursor(const voxel::Voxel &voxel, voxel::FaceNames face, bool flip);
};

}
