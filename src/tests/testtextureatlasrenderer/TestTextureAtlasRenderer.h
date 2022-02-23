/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "voxelformat/MeshCache.h"
#include "voxelrender/CachedMeshRenderer.h"
#include "video/TextureAtlasRenderer.h"
#include "video/Buffer.h"
#include "RenderShaders.h"

class TestTextureAtlasRenderer: public TestApp {
private:
	struct Vertex {
		union {
			glm::vec2 pos;
			struct {
				float x;
				float y;
			};
		};
		union {
			glm::vec2 uv;
			struct {
				float u;
				float v;
			};
		};
		union {
			uint32_t color;
			struct {
				uint8_t r;
				uint8_t g;
				uint8_t b;
				uint8_t a;
			};
		};
	};
	alignas(16) Vertex _vertices[6];
	int _bufIdx = -1;

	using Super = TestApp;
	voxelrender::CachedMeshRenderer _meshRenderer;
	video::TextureAtlasRenderer _atlasRenderer;
	shader::DefaultShader _textureShader;
	video::Buffer _vbo;
	glm::mat4 _modelMatrix;
	float _scale = 5.0f;
	int _modelIndex = -1;
	float _omegaY = 0.0f;

	void updateModelMatrix();
	void doRender() override;
public:
	TestTextureAtlasRenderer(const metric::MetricPtr& metric,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider);

	virtual void onRenderUI() override;
	virtual app::AppState onInit() override;
	virtual app::AppState onCleanup() override;
};
