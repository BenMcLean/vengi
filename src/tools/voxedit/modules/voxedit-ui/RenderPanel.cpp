/**
 * @file
 */

#include "RenderPanel.h"
#include "core/SharedPtr.h"
#include "imgui.h"
#include "scenegraph/SceneGraph.h"
#include "ui/IMGUIEx.h"
#include "video/Texture.h"

namespace voxedit {

bool RenderPanel::init() {
	_texture = video::createEmptyTexture("pathtracer");

	// default params
	yocto::trace_params &params = _pathTracer.state().params;
	params.samples = 32;
	params.envhidden = true;
	params.resolution = 512;

	return true;
}

void RenderPanel::update(const char *title, const scenegraph::SceneGraph &sceneGraph) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		yocto::trace_params &params = _pathTracer.state().params;
		int changed = 0;
		changed += ImGui::InputInt("Dimensions", &params.resolution, 180, 4096);
		changed += ImGui::InputInt("Samples", &params.samples, 16, 4096);
		changed += ImGui::ComboStl("Tracer", (int *)&params.sampler, yocto::trace_sampler_names);
		changed += ImGui::ComboStl("False color", (int *)&params.falsecolor, yocto::trace_falsecolor_names);
		changed += ImGui::SliderInt("Bounces", &params.bounces, 1, 128);
		changed += ImGui::SliderInt("Batch", &params.batch, 1, 16);
		changed += ImGui::SliderFloat("Clamp", &params.clamp, 10, 1000);
		changed += ImGui::SliderInt("Pratio", &params.pratio, 1, 64);
		changed += ImGui::Checkbox("Hide environment", &params.envhidden);
		changed += ImGui::Checkbox("Filter", &params.tentfilter);
		changed += ImGui::Checkbox("Denoise", &params.denoise);
		if (changed > 0) {
			_pathTracer.restart(sceneGraph);
		}
		if (_pathTracer.started()) {
			if (ImGui::Button("Stop path tracer")) {
				_pathTracer.stop();
			}
			ImGui::TooltipText("Sample %i/%i", _currentSample, _pathTracer.state().params.samples);
			_pathTracer.update(&_currentSample);
			_image = _pathTracer.image();
			if (_image->isLoaded()) {
				_texture->upload(_image);
			}
		} else {
			if (ImGui::Button("Start path tracer")) {
				_pathTracer.start(sceneGraph);
			}
		}
		if (_texture->isLoaded()) {
			ImGui::Image(_texture->handle(), ImVec2((float)_texture->width(), (float)_texture->height()));
		}
	}
	ImGui::End();
}

void RenderPanel::shutdown() {
	if (_texture) {
		_texture->shutdown();
	}
}

} // namespace voxedit