/**
 * @file
 */

#include "LUAFunctions.h"
#include "core/GLMConst.h"
#include "core/Log.h"
#include "command/CommandHandler.h"
#include "core/String.h"
#include "core/Var.h"
#include "app/App.h"
#include "core/collection/StringSet.h"
#include "io/Filesystem.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/ext/quaternion_trigonometric.hpp>

int clua_errorhandler(lua_State* s) {
	Log::error("Lua error handler invoked");
	return 0;
}

void clua_assert(lua_State* s, bool pass, const char *msg) {
	if (pass) {
		return;
	}
	lua_Debug ar;
	ar.name = nullptr;
	if (lua_getstack(s, 0, &ar)) {
		lua_getinfo(s, "n", &ar);
	}
	if (ar.name == nullptr) {
		ar.name = "?";
	}
	clua_error(s, msg, ar.name);
}

void clua_assert_argc(lua_State* s, bool pass) {
	clua_assert(s, pass, "wrong number of arguments to '%s'");
}

int clua_assignmetatable(lua_State* s, const char *name) {
	luaL_getmetatable(s, name);
	if (!lua_istable(s, -1)) {
		Log::error("LUA: metatable for %s doesn't exist", name);
		return 0;
	}
	lua_setmetatable(s, -2);
	return 1;
}

bool clua_registernew(lua_State* s, const char *name, lua_CFunction func) {
	if (luaL_getmetatable(s, name) == 0) {
		Log::error("Could not find metatable for %s", name);
		return false;
	}
	// Set a metatable for the metatable
	// This allows using Object(42) to make new objects
	lua_newtable(s);
	lua_pushcfunction(s, func);
	lua_setfield(s, -2, "__call");
	lua_setmetatable(s, -2);
	return true;
}

#ifdef DEBUG
static bool clua_validatefuncs(const luaL_Reg* funcs) {
	core::StringSet funcSet;
	for (; funcs->name != nullptr; ++funcs) {
		if (!funcSet.insert(funcs->name)) {
			Log::error("%s is already in the given funcs", funcs->name);
			return false;
		}
	}
	return true;
}
#endif

bool clua_registerfuncs(lua_State *s, const luaL_Reg *funcs, const char *name) {
	if (luaL_newmetatable(s, name) == 0) {
		Log::warn("Metatable %s already exists", name);
		return false;
	}
#ifdef DEBUG
	if (!clua_validatefuncs(funcs)) {
		return false;
	}
#endif
	luaL_setfuncs(s, funcs, 0);
	// assign the metatable to __index
	lua_pushvalue(s, -1);
	lua_setfield(s, -2, "__index");
	lua_pop(s, 1);
	return true;
}

bool clua_registerfuncsglobal(lua_State* s, const luaL_Reg* funcs, const char *meta, const char *name) {
	if (luaL_newmetatable(s, meta) == 0) {
		Log::warn("Metatable %s already exists", meta);
		return false;
	}
#ifdef DEBUG
	if (!clua_validatefuncs(funcs)) {
		return false;
	}
#endif
	luaL_setfuncs(s, funcs, 0);
	lua_pushvalue(s, -1);
	lua_setfield(s, -1, "__index");
	lua_setglobal(s, name);
	return true;
}

static core::String clua_stackdump(lua_State *L) {
	constexpr int depth = 64;
	core::String dump;
	dump.reserve(1024);
	dump.append("Stacktrace:\n");
	for (int cnt = 0; cnt < depth; cnt++) {
		lua_Debug dbg;
		if (lua_getstack(L, cnt + 1, &dbg) == 0) {
			break;
		}
		lua_getinfo(L, "Snl", &dbg);
		const char *func = dbg.name ? dbg.name : dbg.short_src;
		dump.append(cnt);
		dump.append(": ");
		dump.append(func);
		dump.append("\n");
	}
	dump.append("\n");
	const int top = lua_gettop(L);
	dump.append(core::string::format("%i values on stack%c", top, top > 0 ? '\n' : ' '));

	for (int i = 1; i <= top; i++) { /* repeat for each level */
		const int t = lua_type(L, i);
		switch (t) {
		case LUA_TSTRING:
			lua_pushfstring(L, "%d: %s (%s)\n", i, lua_tostring(L, i), luaL_typename(L, i));
			break;

		case LUA_TBOOLEAN:
			lua_pushfstring(L, "%d: %s (%s)\n", i, (lua_toboolean(L, i) ? "true" : "false"), luaL_typename(L, i));
			break;

		case LUA_TNUMBER:
			lua_pushfstring(L, "%d: %f (%s)\n", i, lua_tonumber(L, i), luaL_typename(L, i));
			break;

		case LUA_TUSERDATA:
		case LUA_TLIGHTUSERDATA:
			lua_pushfstring(L, "%d: %p (%s)\n", i, lua_touserdata(L, i), luaL_typename(L, i));
			break;

		case LUA_TNIL:
			lua_pushfstring(L, "%d: nil\n", i);
			break;

		default:
			lua_pushfstring(L, "%d: (%s)\n", i, luaL_typename(L, i));
			break;
		}
		const char* id = lua_tostring(L, -1);
		if (id != nullptr) {
			dump.append(id);
		}
		lua_pop(L, 1);
	}

	return dump;
}

int clua_error(lua_State *s, const char *fmt, ...) {
	core::String stackdump = clua_stackdump(s);
	Log::error("%s", stackdump.c_str());
	stackdump = core::String();
	va_list argp;
	va_start(argp, fmt);
	luaL_where(s, 1);
	lua_pushvfstring(s, fmt, argp);
	va_end(argp);
	lua_concat(s, 2);
	return lua_error(s);
}

bool clua_optboolean(lua_State* s, int index, bool defaultVal) {
	if (lua_isboolean(s, index)) {
		return lua_toboolean(s, index);
	}
	return defaultVal;
}

int clua_typerror(lua_State *s, int narg, const char *tname) {
	const char *msg = lua_pushfstring(s, "%s expected, got %s", tname, luaL_typename(s, narg));
	return luaL_argerror(s, narg, msg);
}

int clua_checkboolean(lua_State *s, int index) {
	if (index < 0) {
		index += lua_gettop(s) + 1;
	}
	luaL_checktype(s, index, LUA_TBOOLEAN);
	return lua_toboolean(s, index);
}

static int clua_cmdexecute(lua_State *s) {
	const char *cmds = luaL_checkstring(s, 1);
	command::executeCommands(cmds);
	return 0;
}

void clua_cmdregister(lua_State* s) {
	const luaL_Reg funcs[] = {
		{"execute", clua_cmdexecute},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, "_metacmd", "g_cmd");
}

static int clua_vargetstr(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	lua_pushstring(s, v->strVal().c_str());
	return 1;
}

static int clua_vargetint(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	lua_pushinteger(s, v->intVal());
	return 1;
}

static int clua_vargetbool(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	lua_pushboolean(s, v->boolVal());
	return 1;
}

static int clua_vargetfloat(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	lua_pushnumber(s, v->floatVal());
	return 1;
}

static int clua_varsetstr(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	v->setVal(luaL_checkstring(s, 2));
	return 0;
}

static int clua_varsetbool(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	v->setVal(clua_checkboolean(s, 2));
	return 0;
}

static int clua_varsetint(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	v->setVal((int)luaL_checkinteger(s, 2));
	return 0;
}

static int clua_varsetfloat(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	v->setVal((float)luaL_checknumber(s, 2));
	return 0;
}

void clua_varregister(lua_State* s) {
	const luaL_Reg funcs[] = {
		{"str", clua_vargetstr},
		{"bool", clua_vargetbool},
		{"int", clua_vargetint},
		{"float", clua_vargetfloat},
		{"setstr", clua_varsetstr},
		{"setbool", clua_varsetbool},
		{"setint", clua_varsetint},
		{"setfloat", clua_varsetfloat},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, "_metavar", "g_var");
}

static int clua_loginfo(lua_State *s) {
	Log::info("%s", luaL_checkstring(s, 1));
	return 0;
}

static int clua_logerror(lua_State *s) {
	Log::error("%s", luaL_checkstring(s, 1));
	return 0;
}

static int clua_logwarn(lua_State *s) {
	Log::warn("%s", luaL_checkstring(s, 1));
	return 0;
}

static int clua_logdebug(lua_State *s) {
	Log::debug("%s", luaL_checkstring(s, 1));
	return 0;
}

static int clua_logtrace(lua_State *s) {
	Log::trace("%s", luaL_checkstring(s, 1));
	return 0;
}

void clua_logregister(lua_State* s) {
	const luaL_Reg funcs[] = {
		{"info", clua_loginfo},
		{"error", clua_logerror},
		{"warn", clua_logwarn},
		{"debug", clua_logdebug},
		{"trace", clua_logtrace},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, "_metalog", "log");
}

int clua_ioloader(lua_State *s) {
	core::String name = luaL_checkstring(s, 1);
	name.replaceAllChars('.', '/');
	name.append(".lua");
	io::FilePtr file = io::filesystem()->open(name);
	if (!file->exists()) {
		file.release();
		return clua_error(s, "Could not open required file %s", name.c_str());
	}
	const core::String& content = file->load();
	Log::debug("Loading lua module %s with %i bytes", name.c_str(), (int)content.size());
	if (luaL_loadbuffer(s, content.c_str(), content.size(), name.c_str())) {
		Log::error("%s", lua_tostring(s, -1));
		lua_pop(s, 1);
	}
	return 1;
}

glm::quat clua_toquat(lua_State *s, int n) {
	luaL_checktype(s, n, LUA_TTABLE);
	glm::quat v;
	for (int i = 0; i < 4; ++i) {
		lua_getfield(s, n, VEC_MEMBERS[i]);
		v[i] = LuaNumberFuncs<typename glm::quat::value_type>::check(s, -1);
		lua_pop(s, 1);
	}
	return v;
}

static glm::quat rotateX(float angle) {
	return glm::angleAxis(angle, glm::right());
}

static glm::quat rotateZ(float angle) {
	return glm::angleAxis(angle, glm::backward());
}

static glm::quat rotateY(float angle) {
	return glm::angleAxis(angle, glm::up());
}

static constexpr glm::quat rotateXZ(float angleX, float angleZ) {
	return glm::quat(glm::vec3(angleX, 0.0f, angleZ));
}

static constexpr glm::quat rotateXY(float angleX, float angleY) {
	return glm::quat(glm::vec3(angleX, angleY, 0.0f));
}

static int clua_quat_rotate_xyz(lua_State* s) {
	const float x = lua_tonumber(s, 1);
	const float z = lua_tonumber(s, 2);
	return clua_push(s, rotateXZ(x, z));
}

static int clua_quat_rotate_xy(lua_State* s) {
	const float x = lua_tonumber(s, 1);
	const float y = lua_tonumber(s, 2);
	return clua_push(s, rotateXY(x, y));
}

static int clua_quat_rotate_yz(lua_State* s) {
	const float y = lua_tonumber(s, 1);
	const float z = lua_tonumber(s, 2);
	return clua_push(s, rotateXY(y, z));
}

static int clua_quat_rotate_xz(lua_State* s) {
	const float x = lua_tonumber(s, 1);
	const float z = lua_tonumber(s, 2);
	return clua_push(s, rotateXY(x, z));
}

static int clua_quat_rotate_x(lua_State* s) {
	const float x = lua_tonumber(s, 1);
	return clua_push(s, rotateX(x));
}

static int clua_quat_rotate_y(lua_State* s) {
	const float y = lua_tonumber(s, 1);
	return clua_push(s, rotateY(y));
}

static int clua_quat_rotate_z(lua_State* s) {
	const float z = lua_tonumber(s, 1);
	return clua_push(s, rotateZ(z));
}

void clua_quatregister(lua_State* s) {
	const luaL_Reg funcs[] = {
		{"__add", clua_vecadd<glm::quat>},
		{"__sub", clua_vecsub<glm::quat>},
		{"__mul", clua_vecmul<glm::quat>},
		{"__unm", clua_vecnegate<glm::quat>},
		{"__index", clua_vecindex<glm::quat>},
		{"__newindex", clua_vecnewindex<glm::quat>},
		{nullptr, nullptr}
	};
	Log::debug("Register %s lua functions", clua_meta<glm::quat>::name());
	clua_registerfuncs(s, funcs, clua_meta<glm::quat>::name());
	const luaL_Reg globalFuncs[] = {
		{"new",          clua_vecnew<glm::quat>::vecnew},
		{"rotateXYZ",    clua_quat_rotate_xyz},
		{"rotateXY",     clua_quat_rotate_xy},
		{"rotateYZ",     clua_quat_rotate_yz},
		{"rotateXZ",     clua_quat_rotate_xz},
		{"rotateY",      clua_quat_rotate_x},
		{"rotateX",      clua_quat_rotate_y},
		{"rotateZ",      clua_quat_rotate_z},
		{nullptr, nullptr}
	};
	const core::String& globalMeta = core::string::format("%s_global", clua_meta<glm::quat>::name());
	clua_registerfuncsglobal(s, globalFuncs, globalMeta.c_str(), clua_name<glm::quat>::name());
}

void clua_register(lua_State *s) {
	clua_cmdregister(s);
	clua_varregister(s);
	clua_logregister(s);
}

void clua_mathregister(lua_State *s) {
	clua_vecregister<glm::vec2>(s);
	clua_vecregister<glm::vec3>(s);
	clua_vecregister<glm::vec4>(s);
	clua_vecregister<glm::ivec2>(s);
	clua_vecregister<glm::ivec3>(s);
	clua_vecregister<glm::ivec4>(s);
	clua_quatregister(s);
}
