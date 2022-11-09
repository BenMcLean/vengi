/**
 * @file
 */
#include "TestTemplate.h"
#include "testcore/TestAppMain.h"

TestTemplate::TestTemplate(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, timeProvider) {
	init(ORGANISATION, "testtemplate");
}

app::AppState TestTemplate::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	return state;
}

app::AppState TestTemplate::onCleanup() {
	// your cleanup here
	return Super::onCleanup();
}

void TestTemplate::doRender() {
}

TEST_APP(TestTemplate)
