/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "video/Texture.h"
#include "video/Buffer.h"
#include "TestglslcompShaders.h"
#include "testcore/TextureRenderer.h"

/**
 * @brief Visual test for GLSL compute shaders
 *
 * This test application is using a compute shader to fill
 * a texture that is rendered later on.
 */
class TestGLSLComp: public TestApp {
private:
	using Super = TestApp;
	shader::TestShader _testShader;
	render::TextureRenderer _renderer;
	video::TexturePtr _texture;

	void doRender() override;
public:
	TestGLSLComp(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider);

	virtual app::AppState onInit() override;
	virtual app::AppState onCleanup() override;
};
