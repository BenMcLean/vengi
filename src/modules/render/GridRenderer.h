/**
 * @file
 */

#pragma once

#include "render/ShapeRenderer.h"
#include "video/ShapeBuilder.h"
#include "math/AABB.h"
#include "core/IComponent.h"

namespace video {
class Video;
}

namespace render {

/**
 * @brief Renders a grid or bounding box for a given region
 *
 * @note Also hides sides of the grid that would occlude the view to the inside
 *
 * @todo Might be a good idea to implement this as a two sides plane view with backface culling
 */
class GridRenderer {
protected:
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	math::AABB<float> _aabb;

	int32_t _aabbMeshIndex = -1;
	int32_t _gridMeshIndexXYNear = -1;
	int32_t _gridMeshIndexXYFar = -1;
	int32_t _gridMeshIndexXZNear = -1;
	int32_t _gridMeshIndexXZFar = -1;
	int32_t _gridMeshIndexYZNear = -1;
	int32_t _gridMeshIndexYZFar = -1;

	int _resolution = 1;
	bool _renderAABB;
	bool _renderGrid;
	bool _dirty = false;
public:
	GridRenderer(bool renderAABB = false, bool renderGrid = true);

	bool setGridResolution(int resolution);
	int gridResolution() const;

	/**
	 * @param aabb The region to do the plane culling with
	 */
	void render(const video::Camera& camera, const math::AABB<float>& aabb);

	bool renderAABB() const;
	void setRenderAABB(bool renderAABB);

	bool renderGrid() const;
	void setRenderGrid(bool renderGrid);

	/**
	 * @brief Update the internal render buffers for the new region.
	 * @param region The region to render the grid for
	 */
	void update(const math::AABB<float>& region);
	void clear();

	/**
	 * @sa shutdown()
	 */
	bool init();

	void shutdown();
};

inline bool GridRenderer::renderAABB() const {
	return _renderAABB;
}

inline bool GridRenderer::renderGrid() const {
	return _renderGrid;
}

inline void GridRenderer::setRenderAABB(bool renderAABB) {
	_renderAABB = renderAABB;
}

inline void GridRenderer::setRenderGrid(bool renderGrid) {
	_renderGrid = renderGrid;
}

}
