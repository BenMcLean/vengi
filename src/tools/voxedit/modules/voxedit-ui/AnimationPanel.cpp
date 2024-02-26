/**
 * @file
 */

#include "AnimationPanel.h"
#include "core/Log.h"
#include "ui/IconsLucide.h"
#include "command/CommandHandler.h"
#include "voxedit-ui/AnimationTimeline.h"
#include "voxedit-util/SceneManager.h"
#include "ui/IMGUIEx.h"

namespace voxedit {

void AnimationPanel::update(const char *title, command::CommandExecutionListener &listener, AnimationTimeline *animationTimeline) {
	scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const scenegraph::SceneGraphAnimationIds &animations = sceneGraph.animations();
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		ImGui::InputText("##nameanimationpanel", &_newAnimation);
		ImGui::SameLine();
		if (ImGui::IconButton(ICON_LC_PLUS, _("Add"))) {
			if (!_sceneMgr->duplicateAnimation(sceneGraph.activeAnimation(), _newAnimation)) {
				Log::error("Failed to add animation %s", _newAnimation.c_str());
			} else {
				_newAnimation = "";
			}
			animationTimeline->resetFrames();
		}

		const core::String& currentAnimation = sceneGraph.activeAnimation();
		if (ImGui::BeginCombo(_("Animation"), currentAnimation.c_str())) {
			for (const core::String &animation : animations) {
				const bool isSelected = currentAnimation == animation;
				if (ImGui::Selectable(animation.c_str(), isSelected)) {
					if (!_sceneMgr->setAnimation(animation)) {
						Log::error("Failed to activate animation %s", animation.c_str());
					}
					animationTimeline->resetFrames();
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::IconButton(ICON_LC_MINUS, _("Delete"))) {
			if (!_sceneMgr->removeAnimation(currentAnimation)) {
				Log::error("Failed to remove animation %s", currentAnimation.c_str());
			}
			animationTimeline->resetFrames();
		}
	}
	ImGui::End();
}

}
