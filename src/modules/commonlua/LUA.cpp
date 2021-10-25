/**
 * @file
 */

#include "LUA.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "LUAFunctions.h"
#include "Trace.h"
#include "engine-config.h"

namespace lua {

class StackChecker {
private:
	lua_State *_state;
	const int _startStackDepth;
public:
	StackChecker(lua_State *state) :
			_state(state), _startStackDepth(lua_gettop(_state)) {
	}
	~StackChecker() {
		core_assert(_startStackDepth == lua_gettop(_state));
	}
};

namespace {
int panicCB(lua_State *L) {
	const char *error = lua_isnil(L, -1) ? "" : lua_tostring(L, -1);
	Log::error("Lua panic. Error message: %s", error);
	return 0;
}

void debugHook(lua_State *L, lua_Debug *ar) {
	if (!lua_getinfo(L, "Sn", ar)) {
		return;
	}

	Log::info("LUADBG: %s %s %s %i",
			(ar->namewhat != nullptr) ? ar->namewhat : "",
			(ar->name != nullptr) ? ar->name : "",
			ar->short_src,
			ar->currentline);
}
}

#ifdef DEBUG
#define checkStack() StackChecker(this->_state)
#else
#define checkStack()
#endif

LUA::LUA(lua_State *state) :
		_state(state), _destroy(false), _debug(false) {
}

LUA::LUA(bool debug) :
		_destroy(true), _debug(debug) {
	openState();
}

LUA::~LUA() {
	closeState();
}

static int clua_print(lua_State *L) {
	int nargs = lua_gettop(L);
	for (int i = 1; i <= nargs; i++) {
		if (lua_isstring(L, i)) {
			const char* str = lua_tostring(L, i);
			Log::info("%s", str);
		}
	}

	return 0;
}

void LUA::openState() {
	_error.clear();

	_state = luaL_newstate();

	luaL_openlibs(_state);

	clua_register(_state);

	static const struct luaL_Reg printlib[] = {
		{"print", clua_print},
		{nullptr, nullptr}
	};
	lua_getglobal(_state, "_G");
	luaL_setfuncs(_state, printlib, 0);
	lua_pop(_state, 1);

	lua_register(_state, "ioloader", clua_ioloader);
	const char* str = "table.insert(package.searchers, 2, ioloader) \n";
	luaL_dostring(_state, str);

	// Register panic callback function
	lua_atpanic(_state, panicCB);

	int mask = 0;
	if (_debug) {
		mask |= LUA_MASKCALL;
		mask |= LUA_MASKRET;
		mask |= LUA_MASKLINE;
	}

	// Register debug callback function
	lua_sethook(_state, debugHook, mask, 0);

	clua_registertrace(_state);
}

void LUA::closeState() {
	if (_destroy) {
		lua_close(_state);
		_state = nullptr;
	}
}

bool LUA::resetState() {
	// externally managed
	if (!_destroy) {
		return false;
	}

	closeState();
	openState();
	return true;
}

void LUA::reg(const core::String& prefix, const luaL_Reg* funcs) {
	const core::String metaTableName = META_PREFIX + prefix;
	luaL_newmetatable(_state, metaTableName.c_str());
	luaL_setfuncs(_state, funcs, 0);
	lua_pushvalue(_state, -1);
	lua_setfield(_state, -1, "__index");
	lua_setglobal(_state, prefix.c_str());
}

bool LUA::load(const core::String& luaString, int returnValues) {
	if (luaL_loadbufferx(_state, luaString.c_str(), luaString.size(), "", nullptr) || lua_pcall(_state, 0, returnValues, 0)) {
		setError(lua_tostring(_state, -1));
		pop(1);
		return false;
	}

	return true;
}

bool LUA::execute(const core::String &function, int returnValues) {
	lua_getglobal(_state, function.c_str());
	if (lua_isnil(_state, -1)) {
		setError("Function " + function + " wasn't found");
		return false;
	}
	const int nargs = 0;
	/* calculate stack position for message handler */
	const int hpos = lua_gettop(_state) - nargs;
	/* push custom error message handler */
	lua_pushcfunction(_state, clua_errorhandler);
	/* move it before function and arguments */
	lua_insert(_state, hpos);
	/* call lua_pcall function with custom handler */
	const int ret = lua_pcall(_state, nargs, returnValues, hpos);
	/* remove custom error message handler from stack */
	lua_remove(_state, hpos);
	/* pass return value of lua_pcall */
	if (ret != LUA_OK) {
		setError(lua_tostring(_state, -1));
		return false;
	}

	return true;
}

core::String LUA::string(const core::String& expr, const core::String& defaultValue) {
	checkStack();
	const char* r = defaultValue.c_str();
	/* Assign the Lua expression to a Lua global variable. */
	const core::String buf("evalExpr=" + expr);
	if (!luaL_dostring(_state, buf.c_str())) {
		/* Get the value of the global variable */
		lua_getglobal(_state, "evalExpr");
		if (lua_isstring(_state, -1)) {
			r = lua_tostring(_state, -1);
		} else if (lua_isboolean(_state, -1)) {
			r = lua_toboolean(_state, -1) ? "true" : "false";
		}
		/* remove lua_getglobal value */
		lua_pop(_state, 1);
	}
	return r;
}

int LUA::intValue(const core::String& path, int defaultValue) {
	const core::String& str = string(path);
	if (str.empty()) {
		return defaultValue;
	}
	return core::string::toInt(str);
}

float LUA::floatValue(const core::String& path, float defaultValue) {
	const core::String& str = string(path);
	if (str.empty()) {
		return defaultValue;
	}
	return core::string::toFloat(str);
}

void LUA::pop(int amount) {
	lua_pop(_state, amount);
}

}
