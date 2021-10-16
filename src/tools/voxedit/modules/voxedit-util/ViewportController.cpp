/**
 * @file
 */

#include "ViewportController.h"

namespace voxedit {

void ViewportController::resetCamera(const voxel::Region& region) {
	core_assert_msg(region.isValid(), "Invalid region given");
	_camera.setAngles(0.0f, 0.0f, 0.0f);
	_camera.setFarPlane(5000.0f);
	const glm::ivec3& center = region.getCenter();
	const glm::vec3 dim(region.getDimensionsInVoxels());
	const float distance = glm::length(dim);
	_camera.setTarget(center);
	_camera.setTargetDistance(distance * 2.0f);
	if (_renderMode == RenderMode::Animation) {
		_camera.setTarget(glm::zero<glm::vec3>());
		const int height = region.getHeightInCells();
		_camera.setWorldPosition(glm::vec3(-distance, (float)height + distance, -distance));
	} else if (_camMode == SceneCameraMode::Free) {
		const int height = region.getHeightInCells();
		_camera.setWorldPosition(glm::vec3(-distance, (float)height + distance, -distance));
	} else if (_camMode == SceneCameraMode::Top) {
		const int height = region.getHeightInCells();
		_camera.setWorldPosition(glm::vec3(center.x, height + center.y, center.z));
	} else if (_camMode == SceneCameraMode::Left) {
		_camera.setWorldPosition(glm::vec3(-center.x, center.y, center.z));
	} else if (_camMode == SceneCameraMode::Front) {
		const int depth = region.getDepthInCells();
		_camera.setWorldPosition(glm::vec3(center.x, center.y, -depth - center.z));
	}
}

void ViewportController::update(double deltaFrameSeconds) {
	_camera.update(deltaFrameSeconds);
}

void ViewportController::init(ViewportController::SceneCameraMode mode) {
	_camera.setRotationType(video::CameraRotationType::Target);
	_camMode = mode;
	if (mode == ViewportController::SceneCameraMode::Free) {
		_camera.setMode(video::CameraMode::Perspective);
	} else {
		// TODO: activate parallel projection
		//_camera.setMode(video::CameraMode::Orthogonal);
		_camera.setMode(video::CameraMode::Perspective);
	}
	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);
}

void ViewportController::onResize(const glm::ivec2& frameBufferSize, const glm::ivec2& windowSize) {
	if (_camera.mode() == video::CameraMode::Perspective) {
		_camera.init(glm::ivec2(0), frameBufferSize, windowSize);
	} else {
		const glm::ivec2 size = windowSize / 5;
		_camera.init(size / -2, size, size);
	}
}

void ViewportController::move(bool pan, bool rotate, int x, int y) {
	if (rotate) {
		const float yaw = (float)(x - _mouseX);
		const float pitch = (float)(y - _mouseY);
		const float s = _rotationSpeed->floatVal();
		if (_camMode == SceneCameraMode::Free) {
			_camera.turn(yaw * s);
			_camera.setPitch(pitch * s);
		}
	} else if (pan) {
		_camera.pan(_mouseX - x, _mouseY - y);
	}
	_mouseX = x;
	_mouseY = y;
}

}
