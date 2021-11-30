/**
 * @file
 */

#include "AnimationLuaSaver.h"
#include "animation/SkeletonAttribute.h"
#include "io/FileStream.h"
#include "core/GLM.h"
#include <glm/gtc/constants.hpp>
#include <glm/common.hpp>
#include "core/Assert.h"
#include "core/Log.h"

namespace voxedit {

#define wrapBool(expression) \
	if (!(expression)) { \
		Log::error("Error: writing animation entity lua at " SDL_FILE ":%i", SDL_LINE); \
		return false; \
	}

bool saveAnimationEntityLua(const animation::AnimationSettings& settings, const animation::SkeletonAttribute& sa, const char *name, const io::FilePtr& file) {
	if (!file || !file->validHandle()) {
		return false;
	}
	io::FileStream stream(file);
	wrapBool(stream.writeString("require 'chr.bones'\n", false))
	wrapBool(stream.writeString("require 'chr.shared'\n\n", false))
	wrapBool(stream.writeString("function init()\n", false))
	// TODO race and gender
	wrapBool(stream.writeString("  settings.setBasePath(\"human\", \"male\")\n", false))
	wrapBool(stream.writeString("  settings.setMeshTypes(", false))
	for (size_t i = 0; i < settings.types().size(); ++i) {
		const core::String& type = settings.types()[i];
		wrapBool(stream.writeStringFormat(false, "\"%s\"", type.c_str()))
		if (i != settings.types().size() - 1) {
			wrapBool(stream.writeString(", ", false))
		}
	}
	wrapBool(stream.writeString("  )\n", false))
	for (const core::String& t : settings.types()) {
		const int meshTypeIdx = settings.getMeshTypeIdxForName(t.c_str());
		const core::String& path = settings.path(meshTypeIdx, name);
		if (path.empty()) {
			continue;
		}
		wrapBool(stream.writeStringFormat(false, "  settings.setPath(\"%s\", \"%s\")\n", t.c_str(), path.c_str()))
	}

	wrapBool(stream.writeString("  local attributes = defaultSkeletonAttributes()\n", false))
	const animation::CharacterSkeletonAttribute dv {};
	for (const animation::SkeletonAttributeMeta* metaIter = sa.metaArray(); metaIter->name; ++metaIter) {
		const animation::SkeletonAttributeMeta& meta = *metaIter;
		const float *saVal = (const float*)(((const uint8_t*)&sa) + meta.offset);
		const float *dvVal = (const float*)(((const uint8_t*)&dv) + meta.offset);
		if (glm::abs(*saVal - *dvVal) > glm::epsilon<float>()) {
			wrapBool(stream.writeStringFormat(false, "  attributes[\"%s\"] = %f\n", meta.name, *saVal))
		}
	}
	wrapBool(stream.writeString("  return attributes\n", false))
	wrapBool(stream.writeString("end\n", false))
	return true;
}

#undef wrapBool

}
