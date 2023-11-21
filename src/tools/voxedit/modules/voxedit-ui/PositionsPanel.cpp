/**
 * @file
 */

#include "PositionsPanel.h"
#include "IconsForkAwesome.h"
#include "Util.h"
#include "core/ArrayLength.h"
#include "core/Color.h"
#include "core/Log.h"
#include "Gizmo.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsFontAwesome6.h"
#include "ui/IconsLucide.h"
#include "ui/ScopedStyle.h"
#include "ui/Toolbar.h"
#include "ui/dearimgui/ImGuizmo.h"
#include "ui/dearimgui/implot.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"

#include <glm/gtc/type_ptr.hpp>

namespace voxedit {

static bool xyzValues(const char *title, glm::ivec3 &v) {
	bool retVal = false;
	const float width = ImGui::CalcTextSize("10000").x + ImGui::GetStyle().FramePadding.x * 2.0f;

	char buf[64];
	core::String id = "##";
	id.append(title);
	id.append("0");

	id.c_str()[id.size() - 1] = '0';
	core::string::formatBuf(buf, sizeof(buf), "%i", v.x);
	{
		ui::ScopedStyle style;
		style.setColor(ImGuiCol_Text, core::Color::Red());
		ImGui::PushItemWidth(width);
		if (ImGui::InputText(id.c_str(), buf, sizeof(buf),
							 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
			retVal = true;
			v.x = core::string::toInt(buf);
		}
		ImGui::SameLine(0.0f, 2.0f);

		id.c_str()[id.size() - 1] = '1';
		core::string::formatBuf(buf, sizeof(buf), "%i", v.y);
		style.setColor(ImGuiCol_Text, core::Color::Green());
		ImGui::PushItemWidth(width);
		if (ImGui::InputText(id.c_str(), buf, sizeof(buf),
							 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
			retVal = true;
			v.y = core::string::toInt(buf);
		}
		ImGui::SameLine(0.0f, 2.0f);

		id.c_str()[id.size() - 1] = '2';
		core::string::formatBuf(buf, sizeof(buf), "%i", v.z);
		style.setColor(ImGuiCol_Text, core::Color::Blue());
		ImGui::PushItemWidth(width);
		if (ImGui::InputText(id.c_str(), buf, sizeof(buf),
							 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
			retVal = true;
			v.z = core::string::toInt(buf);
		}
	}
	ImGui::SameLine();
	ImGui::TextUnformatted(title);

	return retVal;
}

bool PositionsPanel::init() {
	_gizmoOperations = core::Var::getSafe(cfg::VoxEditGizmoOperations);
	_regionSizes = core::Var::getSafe(cfg::VoxEditRegionSizes);
	_showGizmo = core::Var::getSafe(cfg::VoxEditShowaxis);
	return true;
}

void PositionsPanel::shutdown() {
}

void PositionsPanel::modelView(command::CommandExecutionListener &listener) {
	if (ImGui::CollapsingHeader(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Region", ImGuiTreeNodeFlags_DefaultOpen)) {
		const int nodeId = sceneMgr().sceneGraph().activeNode();
		const core::String &sizes = _regionSizes->strVal();
		if (!sizes.empty()) {
			static const char *max = "888x888x888";
			const ImVec2 buttonSize(ImGui::CalcTextSize(max).x, ImGui::GetFrameHeight());
			ui::Toolbar toolbar(buttonSize, &listener);

			core::DynamicArray<core::String> regionSizes;
			core::string::splitString(sizes, regionSizes, ",");
			for (const core::String &s : regionSizes) {
				glm::ivec3 maxs;
				core::string::parseIVec3(s, &maxs[0]);
				for (int i = 0; i < 3; ++i) {
					if (maxs[i] <= 0 || maxs[i] > 256) {
						return;
					}
				}
				const core::String &title = core::string::format("%ix%ix%i##regionsize", maxs.x, maxs.y, maxs.z);
				toolbar.customNoStyle([&]() {
					if (ImGui::Button(title.c_str())) {
						voxel::Region newRegion(glm::ivec3(0), maxs);
						sceneMgr().resize(nodeId, newRegion);
					}
				});
			}
		} else {
			const voxel::RawVolume *v = sceneMgr().volume(nodeId);
			if (v != nullptr) {
				const voxel::Region &region = v->region();
				glm::ivec3 mins = region.getLowerCorner();
				glm::ivec3 maxs = region.getDimensionsInVoxels();
				if (xyzValues("pos", mins)) {
					const glm::ivec3 &f = mins - region.getLowerCorner();
					sceneMgr().shift(nodeId, f);
				}
				if (xyzValues("size", maxs)) {
					voxel::Region newRegion(region.getLowerCorner(), region.getLowerCorner() + maxs - 1);
					sceneMgr().resize(nodeId, newRegion);
				}

				if (ImGui::CollapsingHeader(ICON_FA_CUBE " Gizmo settings", ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::CheckboxVar("Show gizmo", cfg::VoxEditModelGizmo);
					ImGui::CheckboxVar("Flip Axis", cfg::VoxEditGizmoAllowAxisFlip);
					ImGui::CheckboxVar("Snap", cfg::VoxEditGizmoSnap);
				}
			}
		}

		ImGui::SliderVarInt("Cursor details", cfg::VoxEditCursorDetails, 0, 2);
	}

	ImGui::NewLine();

	if (ImGui::CollapsingHeader(ICON_FA_ARROW_UP " Translate", ImGuiTreeNodeFlags_DefaultOpen)) {
		static glm::ivec3 translate{0};
		veui::InputAxisInt(math::Axis::X, "X##translate", &translate.x, 1);
		veui::InputAxisInt(math::Axis::X, "Y##translate", &translate.y, 1);
		veui::InputAxisInt(math::Axis::X, "Z##translate", &translate.z, 1);
		const core::String &shiftCmd = core::string::format("shift %i %i %i", translate.x, translate.y, translate.z);
		ImGui::CommandButton(ICON_FA_BORDER_ALL " Volumes", shiftCmd.c_str(), listener);
		ImGui::SameLine();
		const core::String &moveCmd = core::string::format("move %i %i %i", translate.x, translate.y, translate.z);
		ImGui::CommandButton(ICON_FA_CUBES " Voxels", moveCmd.c_str(), listener);
	}

	ImGui::NewLine();

	if (ImGui::CollapsingHeader(ICON_FA_CUBE " Cursor", ImGuiTreeNodeFlags_DefaultOpen)) {
		glm::ivec3 cursorPosition = sceneMgr().modifier().cursorPosition();
		math::Axis lockedAxis = sceneMgr().modifier().lockedAxis();
		if (veui::CheckboxAxisFlags(math::Axis::X, "X##cursorlock", &lockedAxis)) {
			command::executeCommands("lockx", &listener);
		}
		ImGui::TooltipCommand("lockx");
		ImGui::SameLine();
		const int step = core::Var::getSafe(cfg::VoxEditGridsize)->intVal();
		if (veui::InputAxisInt(math::Axis::X, "##cursorx", &cursorPosition.x, step)) {
			sceneMgr().setCursorPosition(cursorPosition, true);
		}

		if (veui::CheckboxAxisFlags(math::Axis::Y, "Y##cursorlock", &lockedAxis)) {
			command::executeCommands("locky", &listener);
		}
		ImGui::TooltipCommand("locky");
		ImGui::SameLine();
		if (veui::InputAxisInt(math::Axis::Y, "##cursory", &cursorPosition.y, step)) {
			sceneMgr().setCursorPosition(cursorPosition, true);
		}

		if (veui::CheckboxAxisFlags(math::Axis::Z, "Z##cursorlock", &lockedAxis)) {
			command::executeCommands("lockz", &listener);
		}
		ImGui::TooltipCommand("lockz");
		ImGui::SameLine();
		if (veui::InputAxisInt(math::Axis::Z, "##cursorz", &cursorPosition.z, step)) {
			sceneMgr().setCursorPosition(cursorPosition, true);
		}
	}
}

void PositionsPanel::keyFrameInterpolationSettings(scenegraph::SceneGraphNode &node,
												   scenegraph::KeyFrameIndex keyFrameIdx) {
	ui::ScopedStyle style;
	if (node.type() == scenegraph::SceneGraphNodeType::Camera) {
		style.disableItem();
	}
	const scenegraph::SceneGraphKeyFrame &keyFrame = node.keyFrame(keyFrameIdx);
	const int currentInterpolation = (int)keyFrame.interpolation;
	if (ImGui::BeginCombo("Interpolation##interpolationstrings",
						  scenegraph::InterpolationTypeStr[currentInterpolation])) {
		for (int n = 0; n < lengthof(scenegraph::InterpolationTypeStr); n++) {
			const bool isSelected = (currentInterpolation == n);
			if (ImGui::Selectable(scenegraph::InterpolationTypeStr[n], isSelected)) {
				// TODO: undo missing
				node.keyFrame(keyFrameIdx).interpolation = (scenegraph::InterpolationType)n;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::CollapsingHeader(ICON_FK_LINE_CHART " Interpolation details")) {
		core::Array<glm::dvec2, 20> data;
		for (size_t i = 0; i < data.size(); ++i) {
			const double t = (double)i / (double)data.size();
			const double v = scenegraph::interpolate(keyFrame.interpolation, t, 0.0, 1.0);
			data[i] = glm::dvec2(t, v);
		}
		ImPlotFlags flags = ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoInputs;
		if (ImPlot::BeginPlot("##plotintertype", ImVec2(-1, 0), flags)) {
			ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);
			ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);
			ImPlot::SetupAxisLimits(ImAxis_X1, 0.0f, 1.0f, ImGuiCond_Once);
			ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0f, 1.0f, ImGuiCond_Once);
			const char *lineTitle = scenegraph::InterpolationTypeStr[currentInterpolation];
			const ImPlotLineFlags lineFlag = ImPlotLineFlags_None;
			ImPlot::PlotLine(lineTitle, &data[0].x, &data[0].y, data.size(), lineFlag, 0, sizeof(glm::dvec2));
			ImPlot::EndPlot();
		}
	}
}

void PositionsPanel::keyFrameActionsAndOptions(const scenegraph::SceneGraph &sceneGraph,
											   scenegraph::SceneGraphNode &node, scenegraph::FrameIndex frameIdx,
											   scenegraph::KeyFrameIndex keyFrameIdx) {
	if (ImGui::Button("Reset all")) {
		scenegraph::SceneGraphTransform &transform = node.keyFrame(keyFrameIdx).transform();
		if (_localSpace) {
			transform.setLocalMatrix(glm::mat4(1.0f));
		} else {
			transform.setWorldMatrix(glm::mat4(1.0f));
		}
		node.setPivot({0.0f, 0.0f, 0.0f});
		const bool updateChildren = core::Var::getSafe(cfg::VoxEditTransformUpdateChildren)->boolVal();
		transform.update(sceneGraph, node, frameIdx, updateChildren);
		sceneMgr().mementoHandler().markNodeTransform(node, keyFrameIdx);
	}
	ImGui::SameLine();
	ImGui::CheckboxVar("Auto Keyframe", cfg::VoxEditAutoKeyFrame);
	ImGui::TooltipText("Automatically create keyframes when changing transforms");
}

void PositionsPanel::sceneView(command::CommandExecutionListener &listener) {
	if (ImGui::CollapsingHeader(ICON_FA_ARROW_UP " Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		const scenegraph::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
		const int activeNode = sceneGraph.activeNode();
		if (activeNode != InvalidNodeId) {
			scenegraph::SceneGraphNode &node = sceneGraph.node(activeNode);

			const scenegraph::FrameIndex frameIdx = sceneMgr().currentFrame();
			scenegraph::KeyFrameIndex keyFrameIdx = node.keyFrameForFrame(frameIdx);
			const scenegraph::SceneGraphTransform &transform = node.keyFrame(keyFrameIdx).transform();
			float matrixTranslation[3], matrixRotation[3], matrixScale[3];
			const glm::mat4 &matrix = _localSpace ? transform.localMatrix() : transform.worldMatrix();
			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix), matrixTranslation, matrixRotation,
												  matrixScale);
			bool change = false;
			ImGui::Checkbox("Local transforms", &_localSpace);
			ImGui::CheckboxVar("Update children", cfg::VoxEditTransformUpdateChildren);
			change |= ImGui::InputFloat3("Tr", matrixTranslation, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_X "##resettr")) {
				matrixTranslation[0] = matrixTranslation[1] = matrixTranslation[2] = 0.0f;
				change = true;
			}
			ImGui::TooltipText("Reset");

			change |= ImGui::InputFloat3("Rt", matrixRotation, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_X "##resetrt")) {
				matrixRotation[0] = matrixRotation[1] = matrixRotation[2] = 0.0f;
				change = true;
			}
			ImGui::TooltipText("Reset");

			change |= ImGui::InputFloat3("Sc", matrixScale, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_X "##resetsc")) {
				matrixScale[0] = matrixScale[1] = matrixScale[2] = 1.0f;
				change = true;
			}
			ImGui::TooltipText("Reset");

			glm::vec3 pivot = node.pivot();
			bool pivotChanged =
				ImGui::InputFloat3("Pv", glm::value_ptr(pivot), "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
			change |= pivotChanged;
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_X "##resetpv")) {
				pivot[0] = pivot[1] = pivot[2] = 0.0f;
				pivotChanged = change = true;
			}
			ImGui::TooltipText("Reset");

			keyFrameActionsAndOptions(sceneGraph, node, frameIdx, keyFrameIdx);
			keyFrameInterpolationSettings(node, keyFrameIdx);

			if (change) {
				const bool autoKeyFrame = core::Var::getSafe(cfg::VoxEditAutoKeyFrame)->boolVal();
				// check if a new keyframe should get generated automatically
				if (autoKeyFrame && node.keyFrame(keyFrameIdx).frameIdx != frameIdx) {
					if (sceneMgr().nodeAddKeyFrame(node.id(), frameIdx)) {
						const scenegraph::KeyFrameIndex newKeyFrameIdx = node.keyFrameForFrame(frameIdx);
						core_assert(newKeyFrameIdx != keyFrameIdx);
						core_assert(newKeyFrameIdx != InvalidKeyFrame);
						keyFrameIdx = newKeyFrameIdx;
					}
				}
				glm::mat4 matrix;
				_lastChanged = true;

				if (pivotChanged) {
					sceneMgr().nodeUpdatePivot(node.id(), pivot);
				} else {
					ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale,
															glm::value_ptr(matrix));
					scenegraph::SceneGraphTransform &transform = node.keyFrame(keyFrameIdx).transform();
					if (_localSpace) {
						transform.setLocalMatrix(matrix);
					} else {
						transform.setWorldMatrix(matrix);
					}
					const bool updateChildren = core::Var::getSafe(cfg::VoxEditTransformUpdateChildren)->boolVal();
					transform.update(sceneGraph, node, frameIdx, updateChildren);
				}
			}
			if (!change && _lastChanged) {
				_lastChanged = false;
				sceneMgr().mementoHandler().markNodeTransform(node, keyFrameIdx);
			}
		}
	}

	ImGui::NewLine();

	if (ImGui::CollapsingHeader(ICON_FA_CUBE " Gizmo settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::CheckboxVar(ICON_LC_AXIS_3D " Show gizmo", _showGizmo);

		ImGui::Indent();
		if (!_showGizmo->boolVal())
			ImGui::BeginDisabled();

		int operations = _gizmoOperations->intVal();
		bool dirty = false;

		dirty |= ImGui::CheckboxFlags(ICON_LC_ROTATE_3D " Rotate", &operations, GizmoOperation_Rotate);
		ImGui::TooltipText("Activate the rotate operation");

		dirty |= ImGui::CheckboxFlags(ICON_LC_MOVE_3D " Translate", &operations, GizmoOperation_Translate);
		ImGui::TooltipText("Activate the translate operation");

		// dirty |= ImGui::CheckboxFlags(ICON_LC_BOX " Bounds", &operations, GizmoOperation_Bounds);
		// ImGui::TooltipText("Activate the bounds operation");

		// dirty |= ImGui::CheckboxFlags(ICON_LC_SCALE_3D " Scale", &operations, GizmoOperation_Scale);
		// ImGui::TooltipText("Activate the uniform scale operation");

		if (dirty) {
			_gizmoOperations->setVal(operations);
		}
		ImGui::CheckboxVar(ICON_LC_MAGNET " Snap to grid", cfg::VoxEditGizmoSnap);
		ImGui::CheckboxVar(ICON_LC_REFRESH_CCW_DOT " Pivot", cfg::VoxEditGizmoPivot);
		ImGui::CheckboxVar(ICON_LC_FLIP_HORIZONTAL_2 " Flip axis", cfg::VoxEditGizmoAllowAxisFlip);

		if (!_showGizmo->boolVal())
			ImGui::EndDisabled();

		ImGui::Unindent();
	}
}

void PositionsPanel::update(const char *title, bool sceneMode, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		if (sceneMode) {
			sceneView(listener);
		} else {
			modelView(listener);
		}
	}
	ImGui::End();
}

} // namespace voxedit
