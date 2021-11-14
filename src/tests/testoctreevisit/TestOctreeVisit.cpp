/**
 * @file
 */
#include "TestOctreeVisit.h"

#include "ui/imgui/IMGUI.h"
#include "math/AABB.h"
#include "video/ScopedLineWidth.h"
#include "core/Color.h"
#include "core/Vector.h"
#include "core/collection/Array.h"
#include "testcore/TestAppMain.h"
#include <glm/gtc/type_ptr.hpp>

TestOctreeVisit::TestOctreeVisit(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testoctreevisit");
	setCameraMotion(true);
	//setRenderPlane(true);
	setRenderAxis(true);
}

app::AppState TestOctreeVisit::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	camera().setFarPlane(5000.0f);

	if (!_shapeRenderer.init()) {
		Log::error("Failed to init the shape renderer");
		return app::AppState::InitFailure;
	}

	_shapeBuilder.sphere(10, 10, 5.0f);
	_shapeRenderer.createOrUpdate(_itemMesh, _shapeBuilder);
	// create the offset buffer - just empty
	_shapeRenderer.updatePositions(_itemMesh, _positions);

	_positions.reserve(4096);

	updateCamera();

	return state;
}

void TestOctreeVisit::updateCamera() {
	_octreeCamera.init(glm::ivec2(0), _ortho ? glm::ivec2(100, 50) : windowDimension());
	_octreeCamera.setOmega(_omega);

	_octreeCamera.setWorldPosition(_pos);
	_octreeCamera.lookAt(_lookAt);
	_octreeCamera.setFarPlane(_farPlane);
	_octreeCamera.setNearPlane(_nearPlane);
	_octreeCamera.setRotationType(video::CameraRotationType::Target);
	if (_ortho) {
		_octreeCamera.setMode(video::CameraMode::Orthogonal);
	} else {
		_octreeCamera.setMode(video::CameraMode::Perspective);
	}
}

app::AppState TestOctreeVisit::onRunning() {
	app::AppState state = Super::onRunning();
	if (state != app::AppState::Running) {
		return state;
	}

	_octreeCamera.update(_deltaFrameSeconds);

	const int minSize = 64;
	_positions.clear();
	_octree.visit(_octreeCamera.frustum(), [this] (const glm::ivec3& mins, const glm::ivec3& maxs) {
		const glm::ivec3 center = (mins + maxs) / 2;
		_positions.push_back(center);
		return true;
	}, glm::ivec3(minSize));

	core_assert_always(_shapeRenderer.updatePositions(_itemMesh, _positions));

	_shapeBuilder.clear();
	_shapeBuilder.frustum(_octreeCamera);
	_shapeRenderer.createOrUpdate(_frustumMesh, _shapeBuilder);

	_shapeBuilder.clear();
	const math::AABB<int>& aabb = math::computeAABB(_octreeCamera.frustum(), glm::vec3(minSize));
	_shapeBuilder.aabb(aabb);
	_shapeRenderer.createOrUpdate(_aabbMesh, _shapeBuilder);

	return state;
}

void TestOctreeVisit::onRenderUI() {
	ImGui::SetNextWindowSize(ImVec2(400, 220));
	ImGui::Begin("Options");
	ImGui::InputFloat3("position", glm::value_ptr(_pos));
	ImGui::InputFloat3("lookat", glm::value_ptr(_lookAt));
	ImGui::InputFloat3("omega", glm::value_ptr(_omega));
	ImGui::InputFloat("farplane", &_farPlane);
	ImGui::InputFloat("nearplane", &_nearPlane);
	ImGui::Checkbox("orthogonal", &_ortho);
	if (ImGui::Button("update")) {
		updateCamera();
	}
	ImGui::Separator();
	Super::onRenderUI();
	ImGui::End();
}

void TestOctreeVisit::doRender() {
	_shapeRenderer.render(_itemMesh, camera());
	_shapeRenderer.render(_aabbMesh, camera());
	_shapeRenderer.render(_frustumMesh, camera());
}

app::AppState TestOctreeVisit::onCleanup() {
	_shapeRenderer.shutdown();
	return Super::onCleanup();
}

TEST_APP(TestOctreeVisit)
