#pragma once
#include "lua.h"
#include "lualib.h"

namespace CacheHelpFuncs
{
	static void IsInstance(lua_State* L, int idx) {
		std::string typeoff = luaL_typename(L, idx);
		if (typeoff != ("Instance"))
			luaL_typeerrorL(L, 1, ("Instance"));
	};
}

namespace Cache
{
	inline int iscached(lua_State* LS) {
		luaL_checktype(LS, 1, LUA_TUSERDATA);

		CacheHelpFuncs::IsInstance(LS, 1);
		const auto Instance = *static_cast<void**>(lua_touserdata(LS, 1));

		lua_pushlightuserdata(LS, (void*)Offsets::PushInstance1);
		lua_gettable(LS, LUA_REGISTRYINDEX);

		lua_pushlightuserdata(LS, Instance);
		lua_gettable(LS, -2);

		lua_pushboolean(LS, !lua_isnil(LS, -1));
		return 1;
	}
	inline int replace(lua_State* LS) {
		luaL_checktype(LS, 1, LUA_TUSERDATA);
		luaL_checktype(LS, 2, LUA_TUSERDATA);

		CacheHelpFuncs::IsInstance(LS, 1);
		CacheHelpFuncs::IsInstance(LS, 2);

		const auto Instance = *reinterpret_cast<uintptr_t*>(lua_touserdata(LS, 1));

		lua_pushlightuserdata(LS, (void*)Offsets::PushInstance1);
		lua_gettable(LS, LUA_REGISTRYINDEX);

		lua_pushlightuserdata(LS, (void*)Instance);
		lua_pushvalue(LS, 2);
		lua_settable(LS, -3);
		return 0;
	}
	inline int invalidate(lua_State* LS) {
		luaL_checktype(LS, 1, LUA_TUSERDATA);

		CacheHelpFuncs::IsInstance(LS, 1);

		const auto Instance = *static_cast<void**>(lua_touserdata(LS, 1));

		lua_pushlightuserdata(LS, (void*)Offsets::PushInstance1);
		lua_gettable(LS, LUA_REGISTRYINDEX);

		lua_pushlightuserdata(LS, reinterpret_cast<void*>(Instance));
		lua_pushnil(LS);
		lua_settable(LS, -3);

		return 0;
	};
}
