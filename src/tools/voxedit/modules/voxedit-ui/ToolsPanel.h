/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "core/Var.h"
#include "ui/Panel.h"
#include "command/CommandHandler.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class ToolsPanel : public ui::Panel {
private:
	using Super = ui::Panel;

	SceneManagerPtr _sceneMgr;
	core::VarPtr _gizmoOperations;
	core::VarPtr _showGizmoScene;
	core::VarPtr _showGizmoModel;
	void updateSceneMode(command::CommandExecutionListener &listener);
	void updateEditMode(command::CommandExecutionListener &listener);

public:
	ToolsPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "tools"), _sceneMgr(sceneMgr) {
	}

	bool init();
	void shutdown();
	void update(const char *title, bool sceneMode, command::CommandExecutionListener &listener);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *title) override;
#endif
};

} // namespace voxedit
