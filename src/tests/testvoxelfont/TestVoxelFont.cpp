/**
 * @file
 */
#include "TestVoxelFont.h"
#include "video/Renderer.h"
#include "voxel/MaterialColor.h"
#include "testcore/TestAppMain.h"
#include <SDL.h>

TestVoxelFont::TestVoxelFont(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, timeProvider) {
	init(ORGANISATION, "testvoxelfont");
}

app::AppState TestVoxelFont::onConstruct() {
	_rawVolumeRenderer.construct();
	return Super::onConstruct();
}

app::AppState TestVoxelFont::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (!_rawVolumeRenderer.init(video::getWindowSize())) {
		Log::error("Failed to initialize the raw volume renderer");
		return app::AppState::InitFailure;
	}

	if (!changeFontSize(0)) {
		Log::error("Failed to start voxel font test application - could not load the given font file");
		return app::AppState::InitFailure;
	}

	camera().setFarPlane(4000.0f);

	return state;
}

app::AppState TestVoxelFont::onCleanup() {
	app::AppState state = Super::onCleanup();
	_voxelFont.shutdown();
	const core::DynamicArray<voxel::RawVolume*>& old = _rawVolumeRenderer.shutdown();
	for (voxel::RawVolume* v : old) {
		delete v;
	}
	return state;
}

bool TestVoxelFont::changeFontSize(int delta) {
	_vertices = 0;
	_indices = 0;
	_voxelFont.shutdown();
	_fontSize = glm::clamp(_fontSize + delta, 2, 250);
	if (!_voxelFont.init("font.ttf", _fontSize, _thickness, _mergeQuads ? voxelfont::VoxelFont::MergeQuads : 0u, " Helowrd!NxtLin")) {
		return false;
	}

	voxel::VertexArray vertices;
	voxel::IndexArray indices;

	const char* str = "Hello world!\nNext Line";
	const int renderedChars = _voxelFont.render(str, vertices, indices);
	if ((int)SDL_strlen(str) != renderedChars) {
		Log::error("Failed to render string '%s' (chars: %i)", str, renderedChars);
		return false;
	}

	if (indices.empty() || vertices.empty()) {
		Log::error("Failed to render voxel font");
		return false;
	}

	if (!_rawVolumeRenderer.updateBufferForVolume(0, vertices, indices)) {
		return false;
	}
	_vertices = vertices.size();
	_indices = indices.size();

	return true;
}

bool TestVoxelFont::onMouseWheel(int32_t x, int32_t y) {
	const SDL_Keymod mods = SDL_GetModState();
	if (mods & KMOD_SHIFT) {
		changeFontSize(y);
		return true;
	}

	return Super::onMouseWheel(x, y);
}

bool TestVoxelFont::onKeyPress(int32_t key, int16_t modifier) {
	const bool retVal = Super::onKeyPress(key, modifier);
	if (modifier & KMOD_SHIFT) {
		int delta = 0;
		if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
			delta = -1;
		} else if (key == SDLK_PLUS || key == SDLK_KP_PLUS) {
			delta = 1;
		}

		if (delta != 0) {
			changeFontSize(delta);
			return true;
		}
	}
	if (modifier & KMOD_CTRL) {
		int delta = 0;
		if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
			delta = -1;
		} else if (key == SDLK_PLUS || key == SDLK_KP_PLUS) {
			delta = 1;
		}

		if (delta != 0) {
			_thickness = glm::clamp(_thickness + delta, 1, 250);
			changeFontSize(0);
			return true;
		}
	}
	return retVal;
}

void TestVoxelFont::onRenderUI() {
	ImGui::Text("Fontsize: %i", _fontSize);
	ImGui::Text("Thickness: %i", _thickness);
	if (ImGui::Checkbox("Merge Quads", &_mergeQuads)) {
		changeFontSize(0);
	}
	if (ImGui::Checkbox("Upper left (origin)", &_upperLeft)) {
		changeFontSize(0);
	}
	ImGui::Text("Font vertices: %i, indices: %i", _vertices, _indices);
	ImGui::Text("Ctrl/+ Ctrl/-: Change font thickness");
	ImGui::Text("Space: Toggle merge quads");
	ImGui::Text("Shift/+ Shift/-: Change font size");
	ImGui::Text("Shift/Mousewheel: Change font size");
	Super::onRenderUI();
}

void TestVoxelFont::doRender() {
	_rawVolumeRenderer.update();
	_rawVolumeRenderer.render(camera());
}

TEST_APP(TestVoxelFont)
