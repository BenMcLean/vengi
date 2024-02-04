/**
 * @file
 * @ingroup UI
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/Common.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "dearimgui/imgui.h"
#include "video/Types.h"
#include <glm/fwd.hpp>

namespace io {
struct FormatDescription;
}

namespace video {
class Camera;
}

namespace ImGui {

IMGUI_API void Image(video::Id handle, const glm::ivec2 &size, const glm::vec2 &uv0 = glm::vec2(0),
					 const glm::vec2 &uv1 = glm::vec2(1), const glm::vec4 &tintColor = glm::vec4(1),
					 const glm::vec4 &borderColor = glm::vec4(0));
IMGUI_API void Image(video::Id handle, const ImVec2 &size, const ImVec2 &uv0 = ImVec2(),
					 const ImVec2 &uv1 = ImVec2(1, 1), const ImVec4 &tintColor = ImVec4(1, 1, 1, 1),
					 const ImVec4 &borderColor = ImVec4());
IMGUI_API bool ImageButton(video::Id handle, const ImVec2 &size, const ImVec2 &uv0 = ImVec2(),
						   const ImVec2 &uv1 = ImVec2(1, 1), int frame_padding = -1,
						   const ImVec4 &borderColor = ImVec4(), const ImVec4 &tintColor = ImVec4(1, 1, 1, 1));
IMGUI_API bool InputVec2(const char *label, glm::ivec2 &vec, ImGuiInputTextFlags flags = 0);
IMGUI_API bool InputVec2(const char *label, glm::vec2 &vec, const char *format = "%.2f", ImGuiInputTextFlags flags = 0);
IMGUI_API bool InputVec3(const char *label, glm::ivec3 &vec, ImGuiInputTextFlags flags = 0);
IMGUI_API bool InputVec3(const char *label, glm::vec3 &vec, const char *format = "%.3f", ImGuiInputTextFlags flags = 0);
IMGUI_API bool InputVarString(const char *label, const core::VarPtr &var, ImGuiInputTextFlags flags = 0);
IMGUI_API bool InputVarFloat(const char *label, const core::VarPtr &var, float step = 0.0f, float step_fast = 0.0f,
							 ImGuiInputTextFlags extra_flags = 0);
IMGUI_API bool InputVarFloat(const char *label, const char *varName, float step = 0.0f, float step_fast = 0.0f,
							 ImGuiInputTextFlags extra_flags = 0);
IMGUI_API bool InputVarInt(const char *label, const core::VarPtr &var, int step = 1, int step_fast = 100,
						   ImGuiInputTextFlags extra_flags = 0);
IMGUI_API bool InputVarInt(const char *label, const char *varName, int step = 1, int step_fast = 100,
						   ImGuiInputTextFlags extra_flags = 0);
IMGUI_API bool CheckboxVar(const char *label, const core::VarPtr &var);
IMGUI_API bool CheckboxVar(const char *label, const char *varName);
IMGUI_API bool SliderVarInt(const char *label, const core::VarPtr &var, int v_min, int v_max, const char *format = "%d",
							ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderVarInt(const char *label, const char *varName, int v_min, int v_max, const char *format = "%d",
							ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderVarFloat(const char* label, const core::VarPtr& var, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderVarFloat(const char* label, const char* varName, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
IMGUI_API bool ColorEdit3Var(const char* label, const char* varName);
IMGUI_API bool MenuItemCmd(const char *label, const char *command);
IMGUI_API bool Fullscreen(const char *title = "##main", ImGuiWindowFlags additionalFlags = ImGuiWindowFlags_None);
IMGUI_API void LoadingIndicatorCircle(const char *label, const float indicator_radius = 200,
									  const ImVec4 &main_color = ImVec4(0.0f, 0.0f, 1.0f, 1.0f),
									  const ImVec4 &backdrop_color = ImVec4(0.0f, 0.0f, 0.5f, 1.0f),
									  const int circle_count = 13, const float speed = 1.0f);
IMGUI_API bool InputFile(const char *label, core::String *file, const io::FormatDescription *descriptions, ImGuiInputTextFlags flags = 0u);
IMGUI_API float CalcTextWidth(const char *text, bool withPadding = true);
IMGUI_API float CalcComboWidth(const char *previewLabel, bool withPadding = true);

template <class Collection>
bool ComboItems(const char *label, int *currentItem, const Collection &items) {
	const char *previewValue = nullptr;
	const int itemCount = (int)items.size();
	if (*currentItem >= 0 && *currentItem < itemCount) {
		previewValue = items[*currentItem].c_str();
	}

	if (ImGui::BeginCombo(label, previewValue, ImGuiComboFlags_None)) {
		for (int i = 0; i < itemCount; ++i) {
			const bool selected = i == *currentItem;
			const char *text = items[i].c_str();
			if (ImGui::Selectable(text, selected)) {
				*currentItem = i;
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
		return true;
	}
	return false;
}

template <class Collection>
bool ComboVar(const char* label, const char* varName, const Collection &items) {
	const core::VarPtr &var = core::Var::getSafe(varName);
	int currentItem = var->intVal();
	const bool retVal = ComboItems(label, &currentItem, items);
	if (retVal) {
		var->setVal(currentItem);
		return true;
	}
	return false;
}

IMGUI_API bool TooltipText(CORE_FORMAT_STRING const char *msg, ...) CORE_PRINTF_VARARG_FUNC(1);
IMGUI_API void TextCentered(const char *text);
IMGUI_API void Headline(const char *text);
IMGUI_API bool ToggleButton(const char *text, bool state);
IMGUI_API void TableKeyValue(const char *key, CORE_FORMAT_STRING const char *msg, ...) CORE_PRINTF_VARARG_FUNC(2);
IMGUI_API void TableKeyValue(const char *key, const core::String &value);
IMGUI_API bool DisabledButton(const char *text, bool disabled, const ImVec2& size = ImVec2(0, 0));

IMGUI_API bool InputText(const char *label, core::String *str, ImGuiInputTextFlags flags = 0,
						 ImGuiInputTextCallback callback = nullptr, void *userData = nullptr);
IMGUI_API bool InputTextMultiline(const char *label, core::String *str, const ImVec2 &size = ImVec2(0, 0),
								  ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr,
								  void *userData = nullptr);
IMGUI_API bool InputTextWithHint(const char *label, const char *hint, core::String *str, ImGuiInputTextFlags flags = 0,
								 ImGuiInputTextCallback callback = nullptr, void *userData = nullptr);
IMGUI_API const char *CommandButton(const char *title, const char *command, const char *tooltip = nullptr,
									const ImVec2 &size = ImVec2(0.0f, 0.0f), command::CommandExecutionListener *listener = nullptr);
IMGUI_API const char *CommandButton(const char *title, const char *command, command::CommandExecutionListener &listener);

IMGUI_API bool URLButton(const char *title, const char *url);
IMGUI_API bool CommandRadioButton(const char *title, const core::String &command, bool enabled, command::CommandExecutionListener* listener = nullptr);
IMGUI_API const char *CommandMenuItem(const char *title, const char *command, bool enabled = true,
									  command::CommandExecutionListener *listener = nullptr);
IMGUI_API void URLItem(const char *title, const char *url, float width = 0.0f);
IMGUI_API bool ButtonFullWidth(const char *title);

IMGUI_API void TooltipCommand(const char *command);
IMGUI_API void DrawGrid(ImDrawList *drawList, const video::Camera &camera, const glm::mat4 &matrix, const float gridSize);

} // namespace ImGui
