/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/Region.h"
#include "voxelrender/RawVolumeRenderer.h"

namespace voxedit {

class ISceneRenderer : public core::IComponent {
public:
	virtual ~ISceneRenderer() = default;

	virtual void update() {
	}
	virtual void clear() {
	}
	bool init() override {
		return true;
	}
	void shutdown() override {
	}
	virtual void updateLockedPlanes(math::Axis lockedAxis, const scenegraph::SceneGraph &sceneGraph,
									const glm::ivec3 &cursorPosition) {
	}
	virtual void updateNodeRegion(int nodeId, const voxel::Region &region, uint64_t renderRegionMillis = 0) {
	}
	virtual void updateGridRegion(const voxel::Region &region) {
	}
	virtual void nodeRemove(int nodeId) {
	}
	/**
	 * @brief Before calling this, make sure to set the SceneGraph pointer in the RenderContext
	 */
	virtual void renderUI(voxelrender::RenderContext &renderContext, const video::Camera &camera) {
	}
	/**
	 * @brief Before calling this, make sure to set the SceneGraph pointer in the RenderContext
	 */
	virtual void renderScene(voxelrender::RenderContext &renderContext, const video::Camera &camera) {
	}
};

using SceneRendererPtr = core::SharedPtr<ISceneRenderer>;

} // namespace voxedit
