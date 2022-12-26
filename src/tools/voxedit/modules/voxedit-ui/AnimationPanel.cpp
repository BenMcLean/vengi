/**
 * @file
 */

#include "AnimationPanel.h"
#include "core/Trace.h"
#include "voxedit-util/SceneManager.h"
#include "ui/IMGUIEx.h"

namespace voxedit {

void AnimationPanel::update(const char *title, command::CommandExecutionListener &listener) {
	const core::DynamicArray<core::String> &animations = sceneMgr().sceneGraph().animations();
	if (animations.empty()) {
		return;
	}
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		core_trace_scoped(AnimationPanel);
		for (const core::String& animation : animations) {
			// TODO: allow to select it
			ImGui::TextDisabled("%s", animation.c_str());
		}
	}
	ImGui::End();
}

}
