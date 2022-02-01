/**
 * @file
 */

#include "ScopedPolygonMode.h"
#include "core/Common.h"
#include "Renderer.h"
#include <glm/vec2.hpp>

namespace video {

ScopedPolygonMode::ScopedPolygonMode(video::PolygonMode mode) :
		_mode(mode), _oldMode(video::polygonMode(video::Face::FrontAndBack, mode)) {
}

ScopedPolygonMode::ScopedPolygonMode(video::PolygonMode mode, const glm::vec2& offset) :
		ScopedPolygonMode(mode) {
	_offset = true;
	if (mode == video::PolygonMode::Points) {
		_alreadyActive = video::enable(State::PolygonOffsetPoint);
		video::polygonOffset(offset);
	} else if (mode == video::PolygonMode::WireFrame) {
		_alreadyActive = video::enable(State::PolygonOffsetLine);
		video::polygonOffset(offset);
	} else if (mode == video::PolygonMode::Solid) {
		_alreadyActive = video::enable(State::PolygonOffsetFill);
		video::polygonOffset(offset);
	}
}

ScopedPolygonMode::~ScopedPolygonMode() {
	if (_offset && !_alreadyActive) {
		if (_mode == video::PolygonMode::Points) {
			video::disable(State::PolygonOffsetPoint);
		} else if (_mode == video::PolygonMode::WireFrame) {
			video::disable(State::PolygonOffsetLine);
		} else if (_mode == video::PolygonMode::Solid) {
			video::disable(State::PolygonOffsetFill);
		}
	}

	video::polygonMode(video::Face::FrontAndBack, _oldMode);
}

}
