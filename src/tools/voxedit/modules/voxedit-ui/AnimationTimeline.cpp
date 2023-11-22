/**
 * @file
 */

#include "AnimationTimeline.h"
#include "IconsLucide.h"
#include "core/ArrayLength.h"
#include "core/collection/DynamicArray.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/dearimgui/imgui_neo_sequencer.h"
#include "voxedit-util/SceneManager.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxedit {

void AnimationTimeline::header(scenegraph::FrameIndex currentFrame, scenegraph::FrameIndex maxFrame) {
	if (ImGui::DisabledButton(ICON_LC_PLUS " Add", _play)) {
		sceneMgr().nodeAddKeyFrame(InvalidNodeId, currentFrame);
	}
	ImGui::TooltipText("Add a new keyframe to the current active node");
	ImGui::SameLine();
	if (ImGui::DisabledButton(ICON_LC_PLUS_SQUARE " Add all", _play)) {
		sceneMgr().nodeAllAddKeyFrames(currentFrame);
	}
	ImGui::TooltipText("Add a new keyframe to all model nodes");
	ImGui::SameLine();
	if (ImGui::DisabledButton(ICON_LC_MINUS_SQUARE " Remove", _play)) {
		sceneMgr().nodeRemoveKeyFrame(InvalidNodeId, currentFrame);
	}
	ImGui::TooltipText("Delete the current keyframe of the active nodes");
	ImGui::SameLine();
	if (ImGui::Button(ICON_LC_ARROW_RIGHT_LEFT)) {
		_startFrame = 0;
		_endFrame = core_max(64, maxFrame + 1);
	}
	ImGui::TooltipText("Crop frames");
	ImGui::SameLine();

	if (_play) {
		if (ImGui::Button(ICON_LC_STOP_CIRCLE)) {
			_play = false;
		}
	} else {
		if (ImGui::DisabledButton(ICON_LC_PLAY, maxFrame <= 0)) {
			_play = true;
		}
		ImGui::TooltipText("Max frames for this animation: %i", maxFrame);
	}
}

void AnimationTimeline::timelineEntry(scenegraph::FrameIndex currentFrame, core::Buffer<Selection> &selectionBuffer,
								  core::Buffer<scenegraph::FrameIndex> &selectedFrames,
								  const scenegraph::SceneGraphNode &modelNode) {
	const core::String &label = core::String::format("%s###node-%i", modelNode.name().c_str(), modelNode.id());
	scenegraph::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	if (ImGui::BeginNeoTimelineEx(label.c_str(), nullptr, ImGuiNeoTimelineFlags_AllowFrameChanging)) {
		for (scenegraph::SceneGraphKeyFrame &kf : modelNode.keyFrames()) {
			int32_t oldFrameIdx = kf.frameIdx;
			ImGui::NeoKeyframe(&kf.frameIdx);
			if (kf.frameIdx < 0) {
				kf.frameIdx = 0;
			}
			if (oldFrameIdx != kf.frameIdx) {
				sceneGraph.markMaxFramesDirty();
			}

			if (ImGui::IsNeoKeyframeHovered()) {
				ImGui::BeginTooltip();
				const char *interpolation = scenegraph::InterpolationTypeStr[(int)kf.interpolation];
				ImGui::Text("Keyframe %i, Interpolation: %s", kf.frameIdx, interpolation);
				ImGui::EndTooltip();
			}
		}

		sceneMgr().setCurrentFrame(currentFrame);
		if (ImGui::IsNeoTimelineSelected(ImGuiNeoTimelineIsSelectedFlags_NewlySelected)) {
			sceneMgr().nodeActivate(modelNode.id());
		} else if (sceneMgr().sceneGraph().activeNode() == modelNode.id()) {
			ImGui::SetSelectedTimeline(label.c_str());
		}
		uint32_t selectionCount = ImGui::GetNeoKeyframeSelectionSize();
		if (selectionCount > 0) {
			selectedFrames.clear();
			selectedFrames.resizeIfNeeded(selectionCount);
			ImGui::GetNeoKeyframeSelection(selectedFrames.data());
			for (uint32_t i = 0; i < selectionCount; ++i) {
				selectionBuffer.push_back(Selection{selectedFrames[i], modelNode.id()});
			}
		}
		ImGui::EndNeoTimeLine();
	}
}

void AnimationTimeline::sequencer(scenegraph::FrameIndex &currentFrame) {
	ImGuiNeoSequencerFlags flags = ImGuiNeoSequencerFlags_AlwaysShowHeader;
	flags |= ImGuiNeoSequencerFlags_EnableSelection;
	flags |= ImGuiNeoSequencerFlags_AllowLengthChanging;
	flags |= ImGuiNeoSequencerFlags_Selection_EnableDragging;
	flags |= ImGuiNeoSequencerFlags_Selection_EnableDeletion;

	if (ImGui::BeginNeoSequencer("##neo-sequencer", &currentFrame, &_startFrame, &_endFrame, {0, 0}, flags)) {
		core::Buffer<Selection> selectionBuffer;
		if (_clearSelection) {
			ImGui::NeoClearSelection();
			_clearSelection = false;
		}
		core::Buffer<scenegraph::FrameIndex> selectedFrames;
		const scenegraph::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
		for (auto entry : sceneGraph.nodes()) {
			const scenegraph::SceneGraphNode &node = entry->second;
			if (!node.isAnyModelNode()) {
				continue;
			}
			timelineEntry(currentFrame, selectionBuffer, selectedFrames, node);
		}
		bool selectionRightClicked = ImGui::IsNeoKeyframeSelectionRightClicked();

		ImGui::EndNeoSequencer();

		if (selectionRightClicked) {
			_selectionBuffer = selectionBuffer;
			ImGui::OpenPopup("keyframe-context-menu");
		}
		ImGuiWindowFlags popupFlags = 0;
		popupFlags |= ImGuiWindowFlags_AlwaysAutoResize;
		popupFlags |= ImGuiWindowFlags_NoTitleBar;
		popupFlags |= ImGuiWindowFlags_NoSavedSettings;
		if (ImGui::BeginPopup("keyframe-context-menu", popupFlags)) {
			if (ImGui::Selectable(ICON_LC_PLUS_SQUARE " Add")) {
				sceneMgr().nodeAddKeyFrame(InvalidNodeId, currentFrame);
				_clearSelection = true;
				ImGui::CloseCurrentPopup();
			}
			if (!_selectionBuffer.empty()) {
#if 0
				if (ImGui::Selectable(ICON_LC_COPY " Copy")) {
					// TODO: implement copy
				}
				if (ImGui::Selectable(ICON_LC_PASTE " Paste")) {
					// TODO: implement paste
				}
#endif
				if (ImGui::Selectable(ICON_LC_COPY " Duplicate keyframe")) {
					for (const Selection &sel : _selectionBuffer) {
						sceneMgr().nodeAddKeyFrame(sel.nodeId, sel.frameIdx + 1);
					}
					_clearSelection = true;
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::Selectable(ICON_LC_TRASH " Delete keyframes")) {
					for (const Selection &sel : _selectionBuffer) {
						sceneMgr().nodeRemoveKeyFrame(sel.nodeId, sel.frameIdx);
					}
					_clearSelection = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::TooltipText("Delete %i keyframes", (int)_selectionBuffer.size());
			}
			ImGui::EndPopup();
		}
	}
}

bool AnimationTimeline::update(const char *sequencerTitle, double deltaFrameSeconds) {
	scenegraph::FrameIndex currentFrame = sceneMgr().currentFrame();
	const scenegraph::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	const scenegraph::FrameIndex maxFrame = sceneGraph.maxFrames(sceneGraph.activeAnimation());
	if (_endFrame == -1) {
		_endFrame = core_max(64, maxFrame + 1);
	}
	_seconds += deltaFrameSeconds;

	if (_play) {
		if (maxFrame <= 0) {
			_play = false;
		} else {
			// TODO: anim fps
			currentFrame = (currentFrame + 1) % maxFrame;
			sceneMgr().setCurrentFrame(currentFrame);
		}
	}
	if (ImGui::Begin(sequencerTitle)) {
		header(currentFrame, maxFrame);
		if (ImGui::BeginChild("##neo-sequencer-child")) {
			sequencer(currentFrame);
		}
		ImGui::EndChild();
	}
	ImGui::End();
	return true;
}

} // namespace voxedit
