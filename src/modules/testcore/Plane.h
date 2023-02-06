/**
 * @file
 */

#pragma once

#include "video/Camera.h"
#include "core/Color.h"
#include "math/Plane.h"
#include "video/ShapeBuilder.h"
#include "render/ShapeRenderer.h"
#include "core/IComponent.h"

namespace video {
class Video;
}

namespace render {

/**
 * @brief Renders a plane
 *
 * @see video::ShapeBuilder
 * @see ShapeRenderer
 */
class Plane : public core::IComponent {
private:
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	bool _planeMeshes[render::ShapeRenderer::MAX_MESHES] { false };
public:
	void render(const video::Camera& camera, const glm::mat4& model = glm::mat4(1.0f)) const;

	void shutdown() override;

	void clear();

	bool init() override;

	/**
	 * @param[in] position The offset that should be applied to the center of the plane
	 * @param[in] tesselation The amount of splits on the plane that should be made
	 * @param[in] color The color of the plane.
	 */
	bool plane(const glm::vec3& position, int tesselation = 0, const glm::vec4& color = core::Color::White);

	bool plane(const glm::vec3& position, const math::Plane& plane, const glm::vec4& color = core::Color::White);
};

}
