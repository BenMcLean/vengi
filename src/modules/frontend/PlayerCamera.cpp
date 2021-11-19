/**
 * @file
 */

#include "PlayerCamera.h"
#include "command/Command.h"
#include "core/Trace.h"

namespace frontend {

void PlayerCamera::construct() {
	_maxTargetDistance = core::Var::get(cfg::ClientCameraMaxTargetDistance, "28.0");
	_cameraZoomSpeed = core::Var::get(cfg::ClientCameraZoomSpeed, "10.0");

	command::Command::registerActionButton("zoom_in", _zoomIn).setBindingContext(_keyBindingContext);
	command::Command::registerActionButton("zoom_out", _zoomOut).setBindingContext(_keyBindingContext);
	command::Command::registerCommand("togglecamera", [this] (const command::CmdArgs& args) {
		toggleCameraType();
	}).setBindingContext(_keyBindingContext);
}

void PlayerCamera::toggleCameraType() {
	if (_camera.type() == video::CameraType::Free) {
		setCameraTarget();
	} else if (_camera.type() == video::CameraType::FirstPerson) {
		setCameraFirstPerson();
	}
}

void PlayerCamera::setCameraFirstPerson() {
	_camera.setRotationType(video::CameraRotationType::Eye);
	_camera.setType(video::CameraType::FirstPerson);
	_camera.update(0.0);
}

void PlayerCamera::setCameraTarget() {
	_camera.setRotationType(video::CameraRotationType::Target);
	_camera.setType(video::CameraType::Free);
	_camera.update(0.0);
}

bool PlayerCamera::init(const glm::ivec2& windowSize) {
	_camera.setSize(windowSize);
	_camera.setFarPlane(10.0f);
	_camera.setRotationType(video::CameraRotationType::Target);
	_camera.setFieldOfView(_fieldOfView);
	_camera.setTargetDistance(_targetDistance);
	_camera.setWorldPosition(_cameraPosition);
	_camera.setTarget(glm::vec3(0.0f));
	_camera.setAngles(0.0f, 0.0f, 0.0f);
	_camera.update(0.0);

	return true;
}

void PlayerCamera::shutdown() {
	command::Command::unregisterActionButton("zoom_in");
	command::Command::unregisterActionButton("zoom_out");
}

void PlayerCamera::zoom(float level) {
	const float value = _cameraZoomSpeed->floatVal() * level;
	_targetDistance = glm::clamp(_targetDistance + value, 1.0f, _maxTargetDistance->floatVal());
}

void PlayerCamera::rotate(float pitch, float turn, float speed) {
	_pendingPitch = pitch;
	_pendingTurn = turn;
	if (glm::abs(pitch + turn) > glm::epsilon<float>()) {
		_pendingSpeed = speed;
	}
}

void PlayerCamera::update(const glm::vec3& entityPosition, double nowSeconds, double deltaFrameSeconds, double speed, float farPlane) {
	core_trace_scoped(UpdatePlayerCamera);
	if (_zoomIn.pressed()) {
		_zoomIn.execute(nowSeconds, 0.02, [&] () {
			zoom(1.0f);
		});
	} else if (_zoomOut.pressed()) {
		_zoomOut.execute(nowSeconds, 0.02, [&] () {
			zoom(-1.0f);
		});
	}

	// TODO: fix this magic number with the real character eye height.
	static const glm::vec3 eye(0.0f, 1.8f, 0.0f);
	const glm::vec3 targetpos = glm::mix(_camera.target(), entityPosition + eye, deltaFrameSeconds * 10.0);
	_camera.setTarget(targetpos);

	if (_pendingSpeed > 0.0f) {
		// TODO: optimize this
		video::Camera clone = camera();
		const glm::vec3 radians(_pendingPitch * _pendingSpeed, _pendingTurn * _pendingSpeed, 0.0f);
		clone.rotate(radians);
		if (clone.pitch() >= glm::radians(1.0f)) {
			_camera.rotate(radians);
			_pendingSpeed = -1.0f;
		} else {
			_pendingPitch *= 0.5f;
		}
	}

	_camera.setTargetDistance(_targetDistance);
	_camera.setFarPlane(farPlane);
	_camera.update(deltaFrameSeconds);
}

}
