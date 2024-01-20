/**
 * @file
 */

#include "SceneUtil.h"
#include "scenegraph/SceneGraph.h"

namespace voxedit {

math::AABB<float> toAABB(const voxel::Region& region) {
	if (region.isValid()) {
		return math::AABB<float>(glm::floor(region.getLowerCornerf()), glm::floor(glm::vec3(region.getUpperCornerf() + 1.0f)));
	}
	return math::AABB<float>(1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f);
}

math::OBB<float> toOBB(bool sceneMode, const voxel::Region &region, const glm::vec3 &normalizedPivot, const scenegraph::FrameTransform &transform) {
	core_assert(region.isValid());
	if (sceneMode) {
		const glm::vec3 pivot =
			(normalizedPivot - 0.5f) * glm::vec3(region.getDimensionsInVoxels()) - region.getLowerCornerf();
		const glm::vec3 &extents = transform.scale * glm::vec3(region.getDimensionsInVoxels()) / 2.0f;
		const glm::vec3 &center = transform.translation;
		const glm::mat4x4 &matrix = transform.worldMatrix();
		return math::OBB<float>(center, pivot, extents, matrix);
	}
	return math::OBB<float>(glm::floor(region.getLowerCornerf()),
							glm::floor(glm::vec3(region.getUpperCornerf() + 1.0f)));
}

} // namespace voxedit
