/**
 * @file
 */
#include "TestIMGUI.h"
#include "testcore/TestAppMain.h"
#include "core/Color.h"
#include "core/Log.h"

TestIMGUI::TestIMGUI(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, timeProvider) {
	init(ORGANISATION, "testimgui");
}

void TestIMGUI::onRenderUI() {
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Separator();
	ImGui::ShowUserGuide();
	//ImGui::Separator();
	//ImGui::ShowStyleEditor(&ImGui::GetStyle());
	ImGui::Separator();
	if (ImGui::Button("Test Window")) {
		_showTestWindow ^= true;
	}
	if (ImGui::Button("Metrics Window")) {
		_showMetricsWindow ^= true;
	}
	if (ImGui::Button("Quit")) {
		requestQuit();
	}

	if (_showMetricsWindow) {
		ImGui::ShowMetricsWindow(&_showMetricsWindow);
	}

	if (_showTestWindow) {
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(650, 20), ImGuiCond_FirstUseEver);
		ImGui::ShowDemoWindow(&_showTestWindow);
	}
}

app::AppState TestIMGUI::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}
	_logLevelVar->setVal(core::string::toString(SDL_LOG_PRIORITY_DEBUG));
	Log::init();

	video::clearColor(::core::Color::Black);
	//video::enableDebug(video::DebugSeverity::Medium);
	return state;
}

TEST_APP(TestIMGUI)
