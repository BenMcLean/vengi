/**
 * @file
 */

#include "AnimationSettings.h"
#include "core/StringUtil.h"
#include "core/Assert.h"
#include "core/ArrayLength.h"
#include "core/Log.h"
#include "core/Common.h"
#include "SkeletonAttribute.h"
#include "BoneId.h"
#include "commonlua/LUAFunctions.h"

template<> struct clua_meta<animation::BoneIds> { static char const *name() {return "__meta_boneids";} };

namespace animation {

static const char *luaanim_settingsid() {
	return "__global_settings";
}

static AnimationSettings* luaanim_getsettings(lua_State *s) {
	lua_getglobal(s, luaanim_settingsid());
	AnimationSettings *provider = (AnimationSettings *)lua_touserdata(s, -1);
	lua_pop(s, 1);
	return provider;
}

static void luaanim_pushsettings(lua_State* s, AnimationSettings* provider) {
	lua_pushlightuserdata(s, provider);
	lua_setglobal(s, luaanim_settingsid());
}

static int luaanim_settingssetmeshtypes(lua_State * l) {
	AnimationSettings *settings = luaanim_getsettings(l);
	const int n = lua_gettop(l);
	core::DynamicArray<core::String> types;
	types.reserve(n);
	for (int i = 1; i <= n; ++i) {
		types.push_back(luaL_checkstring(l, i));
	}
	settings->setMeshTypes(types);
	return 0;
}

static int luaanim_settingsgetmeshtypes(lua_State * l) {
	AnimationSettings *settings = luaanim_getsettings(l);
	lua_newtable(l);
	const int top = lua_gettop(l);
	int i = 0;
	for (const core::String& type : settings->types()) {
		lua_pushinteger(l, ++i);
		lua_pushstring(l, type.c_str());
		lua_settable(l, top);
	}
	return 1;
}

static int luaanim_settingssetbasepath(lua_State * l) {
	AnimationSettings *settings = luaanim_getsettings(l);
	settings->basePath = luaL_checkstring(l, 1);
	return 0;
}

static int luaanim_settingssetpath(lua_State * l) {
	AnimationSettings *settings = luaanim_getsettings(l);
	const char *type = luaL_checkstring(l, 1);
	const char *value = luaL_checkstring(l, 2);
	const int idx = settings->getMeshTypeIdxForName(type);
	if (idx < 0) {
		return clua_error(l, "Could not find mesh type for %s", type);
	}
	settings->setPath(idx, value);
	return 0;
}

static int luaanim_settingssettype(lua_State * l) {
	AnimationSettings *settings = luaanim_getsettings(l);
	const char *type = luaL_checkstring(l, 1);
	const int n = lengthof(AnimationSettings::TypeStrings);
	for (int i = 0; i < n; ++i) {
		if (!SDL_strcmp(type, AnimationSettings::TypeStrings[i])) {
			settings->setType((AnimationSettings::Type)i);
			return 0;
		}
	}
	return clua_error(l, "Could not find entity type for %s", type);
}

static int luaanim_boneidstostring(lua_State* s) {
	BoneIds** a = clua_get<BoneIds*>(s, 1);
	BoneIds& boneIds = **a;
	if (boneIds.num == 0) {
		lua_pushstring(s, "empty");
		return 1;
	}
	if (boneIds.num == 1) {
		lua_pushfstring(s, "num bones: 1, bone[0]: %i", (int)boneIds.bones[0]);
		return 1;
	}
	if (boneIds.num == 2) {
		lua_pushfstring(s, "num bones: 2, bone[0]: %i, bone[1]: %i", (int)boneIds.bones[0], (int)boneIds.bones[1]);
		return 1;
	}
	lua_pushfstring(s, "error: num bones: %i", (*a)->num);
	return 1;
}

static int luaanim_boneidsadd(lua_State* s) {
	BoneIds** boneIdsPtrPtr = clua_get<BoneIds*>(s, 1);
	BoneIds* boneIds = *boneIdsPtrPtr;
	const char* boneName = luaL_checkstring(s, 2);
	BoneId id = toBoneId(boneName);
	if (id == BoneId::Max) {
		return clua_error(s, "Failed to resolve bone: '%s'", boneName);
	}
	if (boneIds->num >= lengthof(boneIds->bones)) {
		lua_pushboolean(s, 0);
		return 1;
	}
	boneIds->bones[boneIds->num] = (BoneId)id;
	boneIds->mirrored[boneIds->num] = clua_optboolean(s, 3, false);
	boneIds->num++;
	lua_pushboolean(s, 1);
	return 1;
}

static int luaanim_pushboneids(lua_State* s, BoneIds* b) {
	return clua_push(s, b);
}

static int luaanim_bonesetup(lua_State* l) {
	AnimationSettings* settings = luaanim_getsettings(l);
	const char* meshType = luaL_checkstring(l, 1);
	const int idx = settings->getMeshTypeIdxForName(meshType);
	if (idx < 0 || idx >= (int)AnimationSettings::MAX_ENTRIES) {
		return clua_error(l, "Could not find mesh type for %s", meshType);
	}
	BoneIds& b = settings->boneIds(idx);
	b = BoneIds {};
	if (luaanim_pushboneids(l, &b) != 1) {
		return clua_error(l, "Failed to push the bone ids");
	}
	return 1;
}

static int luaanim_boneregister(lua_State* l) {
	AnimationSettings* settings = luaanim_getsettings(l);
	const char* boneName = luaL_checkstring(l, 1);
	const BoneId boneId = toBoneId(boneName);
	if (boneId == BoneId::Max) {
		return clua_error(l, "Failed to resolve bone: '%s'", boneName);
	}
	if (settings->registerBoneId(boneId)) {
		lua_pushboolean(l, 1);
	} else {
		lua_pushboolean(l, 0);
	}
	return 1;
}

core::String luaFilename(const char *character) {
	return core::string::format("%s.lua", character);
}

bool loadAnimationSettings(const core::String& luaString, AnimationSettings& settings, SkeletonAttribute* skeletonAttr) {
	if (luaString.empty()) {
		Log::warn("empty animation settings can't get loaded");
		return false;
	}

	lua::LUA lua;

	static constexpr luaL_Reg settingsFuncs[] = {
		{"setBasePath", luaanim_settingssetbasepath},
		{"setPath", luaanim_settingssetpath},
		{"setType", luaanim_settingssettype},
		{"setMeshTypes", luaanim_settingssetmeshtypes},
		{"getMeshTypes", luaanim_settingsgetmeshtypes},
		{nullptr, nullptr}
	};

	static constexpr luaL_Reg globalBoneFuncs[] = {
		{"setup", luaanim_bonesetup},
		{"register", luaanim_boneregister},
		{nullptr, nullptr}
	};

	static constexpr luaL_Reg boneFuncs[] = {
		{"__tostring", luaanim_boneidstostring},
		{"add", luaanim_boneidsadd},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(lua, settingsFuncs, "__meta_settings", "settings");
	clua_registerfuncsglobal(lua, globalBoneFuncs, "__meta_bone", "bone");
	clua_registerfuncs(lua.state(), boneFuncs, clua_meta<BoneIds>::name());
	luaanim_pushsettings(lua, &settings);

	settings.reset();

	if (!lua.load(luaString)) {
		Log::error("%s", lua.error().c_str());
		return false;
	}

	if (!lua.execute("init", 1)) {
		Log::error("%s", lua.error().c_str());
		return false;
	}

	if (!lua_istable(lua, -1)) {
		Log::error("Expected to get a table as return value");
		return false;
	}

	const SkeletonAttributeMeta* metaIter = skeletonAttr != nullptr ? skeletonAttr->metaArray() : nullptr;
	for (; metaIter && metaIter->name; ++metaIter) {
		const SkeletonAttributeMeta& meta = *metaIter;
		float *saVal = (float*)(((uint8_t*)skeletonAttr) + meta.offset);

		lua_getfield(lua, -1, meta.name);
		if (lua_isnil(lua, -1)) {
			lua_pop(lua, 1);
			Log::debug("Skeleton attribute value for %s not given - use default: %f", meta.name, *saVal);
			continue;
		}
		*saVal = (float)lua_tonumber(lua, -1);
		lua_pop(lua, 1);
		Log::debug("Skeleton attribute value for %s: %f", meta.name, *saVal);
	}
	return settings.init();
}

void AnimationSettings::reset() {
	Log::debug("Reset bones");
	for (int i = 0; i < (int)BoneId::Max; ++i) {
		_boneIndices[i] = -1;
	}
	_currentBoneIdx = 0u;
}

bool AnimationSettings::init() {
	for (size_t i = 0u; i < MAX_ENTRIES; ++i) {
		if (boneIdsArray[i].num == 0) {
			continue;
		}
		for (uint8_t b = 0u; b < boneIdsArray[i].num; ++b) {
			const BoneId boneId = boneIdsArray[i].bones[b];
			if (boneId == BoneId::Max) {
				Log::error("Invalid bone mapping found for mesh type %i (bone num: %i)", (int)i, b);
				return false;
			}
			const int idx = core::enumVal(boneId);
			if (_boneIndices[idx] != -1) {
				continue;
			}

			Log::debug("Assign index %i to bone %s", _currentBoneIdx, toBoneId(boneId));
			_boneIndices[idx] = _currentBoneIdx++;
		}
	}
	Log::debug("Bones for animation: %i", (int)_currentBoneIdx);

	return true;
}

void AnimationSettings::setMeshTypes(const core::DynamicArray<core::String>& meshTypes) {
	_meshTypes = meshTypes;
}

const core::String& AnimationSettings::meshType(size_t meshTypeIdx) const {
	if (meshTypeIdx >= (int) MAX_ENTRIES) {
		static const core::String EMPTY;
		return EMPTY;
	}
	return _meshTypes[meshTypeIdx];
}

int AnimationSettings::getMeshTypeIdxForName(const char *name) const {
	for (size_t i = 0; i < _meshTypes.size(); ++i) {
		if (_meshTypes[i] == name) {
			return i;
		}
	}
	return -1;
}

core::String AnimationSettings::fullPath(int meshTypeIdx, const char *name) const {
	if (meshTypeIdx < 0 || meshTypeIdx >= (int) MAX_ENTRIES) {
		static const core::String EMPTY;
		return EMPTY;
	}
	if (name == nullptr) {
		name = paths[meshTypeIdx].c_str();
	}
	return core::string::path(basePath, _meshTypes[meshTypeIdx], name);
}

core::String AnimationSettings::path(int meshTypeIdx, const char *name) const {
	if (meshTypeIdx < 0 || meshTypeIdx >= (int) MAX_ENTRIES) {
		static const core::String EMPTY;
		return EMPTY;
	}
	if (name == nullptr) {
		name = paths[meshTypeIdx].c_str();
	}
	return core::string::path(_meshTypes[meshTypeIdx], name);
}

bool AnimationSettings::setPath(int meshTypeIdx, const char *str) {
	if (meshTypeIdx < 0 || meshTypeIdx >= (int) MAX_ENTRIES) {
		return false;
	}
	paths[meshTypeIdx] = str;
	return true;
}

const BoneIds& AnimationSettings::boneIds(int meshTypeIdx) const {
	core_assert(meshTypeIdx >= 0 && meshTypeIdx < (int) MAX_ENTRIES);
	return boneIdsArray[meshTypeIdx];
}

BoneIds& AnimationSettings::boneIds(int meshTypeIdx) {
	core_assert(meshTypeIdx >= 0 && meshTypeIdx < (int) MAX_ENTRIES);
	return boneIdsArray[meshTypeIdx];
}

bool AnimationSettings::registerBoneId(BoneId boneId) {
	core_assert(boneId != BoneId::Max);
	const int boneIdx = core::enumVal(boneId);
	if (_boneIndices[boneIdx] != -1) {
		return false;
	}
	Log::info("Register bone %s at index %i", toBoneId(boneId), _currentBoneIdx);
	_boneIndices[boneIdx] = _currentBoneIdx++;
	return true;
}

int8_t AnimationSettings::mapBoneIdToArrayIndex(BoneId boneId) const {
	core_assert(boneId != BoneId::Max);
	return _boneIndices[core::enumVal(boneId)];
}

}
