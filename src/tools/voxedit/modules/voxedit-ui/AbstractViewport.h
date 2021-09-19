/**
 * @file
 */

#pragma once

#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "voxedit-util/ViewportController.h"
#include "RenderShaders.h"

namespace voxedit {

/**
 * @brief Voxel editor scene management for input rendering.
 * @see voxedit::ViewportController
 */
class AbstractViewport {
protected:
	shader::EdgeShader& _edgeShader;
	video::FrameBuffer _frameBuffer;
	video::TexturePtr _texture;
	voxedit::ViewportController _controller;
	core::String _cameraMode;

	AbstractViewport();
	void renderToFrameBuffer();
	void resize(const glm::ivec2& frameBufferSize);
	void cursorMove(bool, int x, int y);

public:
	virtual ~AbstractViewport();

	virtual bool init();
	void setMode(ViewportController::SceneCameraMode mode = ViewportController::SceneCameraMode::Free);
	void setRenderMode(ViewportController::RenderMode renderMode = ViewportController::RenderMode::Editor);
	virtual void shutdown();
	virtual void update();
	void resetCamera();
	bool saveImage(const char* filename);

	video::FrameBuffer& frameBuffer();
	video::Camera& camera();
	ViewportController& controller();
};

inline video::FrameBuffer& AbstractViewport::frameBuffer() {
	return _frameBuffer;
}

inline ViewportController& AbstractViewport::controller() {
	return _controller;
}

inline video::Camera& AbstractViewport::camera() {
	return _controller.camera();
}

}
