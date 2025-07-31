#pragma once
#include <algorithm>

#include "lua.h"
#include "lualib.h"
#include <ranges>

namespace HelpFuncs
{
	static int GetEveryInstance(lua_State* L)
	{
		lua_pushvalue(L, LUA_REGISTRYINDEX);
		lua_pushlightuserdata(L, (void*)Roblox::PushInstance);
		lua_gettable(L, -2);
		return 1;
	};
}
static void IsInstance(lua_State* L, int idx) {
	std::string typeoff = luaL_typename(L, idx);
	if (typeoff != ("Instance"))
		luaL_typeerrorL(L, 1, ("Instance"));
};

namespace Instances
{
	inline int cloneref(lua_State* LS) {
		luaL_checktype(LS, 1, LUA_TUSERDATA);

		IsInstance(LS, 1);

		const auto OldUserdata = lua_touserdata(LS, 1);

		const auto NewUserdata = *reinterpret_cast<uintptr_t*>(OldUserdata);

		lua_pushlightuserdata(LS, (void*)Offsets::PushInstance1);

		lua_rawget(LS, -10000);
		lua_pushlightuserdata(LS, reinterpret_cast<void*>(NewUserdata));
		lua_rawget(LS, -2);

		lua_pushlightuserdata(LS, reinterpret_cast<void*>(NewUserdata));
		lua_pushnil(LS);
		lua_rawset(LS, -4);

		Roblox::PushInstance(LS, (void*)OldUserdata);

		lua_pushlightuserdata(LS, reinterpret_cast<void*>(NewUserdata));
		lua_pushvalue(LS, -3);
		lua_rawset(LS, -5);

		return 1;
	}
	inline int compareinstances(lua_State* LS) {
		luaL_checktype(LS, 1, LUA_TUSERDATA);
		luaL_checktype(LS, 2, LUA_TUSERDATA);

		IsInstance(LS, 1);
		IsInstance(LS, 2);

		uintptr_t First = *reinterpret_cast<uintptr_t*>(lua_touserdata(LS, 1));
		if (!First)
			luaL_argerrorL(LS, 1, "Invalid instance");

		uintptr_t Second = *reinterpret_cast<uintptr_t*>(lua_touserdata(LS, 2));
		if (!Second)
			luaL_argerrorL(LS, 2, "Invalid instance");

		if (First == Second)
			lua_pushboolean(LS, true);
		else
			lua_pushboolean(LS, false);

		return 1;
	}
	inline int fireclickdetector(lua_State* L) {
		luaL_checktype(L, 1, LUA_TUSERDATA);

		std::string clickOption = lua_isstring(L, 3) ? lua_tostring(L, 3) : "";

		if (strcmp(luaL_typename(L, 1), "Instance") != 0)
		{
			luaL_typeerror(L, 1, "Instance");
			return 0;
		}

		const auto clickDetector = *reinterpret_cast<uintptr_t*>(lua_touserdata(L, 1));

		float distance = 0.0;

		if (lua_isnumber(L, 2))
			distance = (float)lua_tonumber(L, 2);

		lua_getglobal(L, "game");
		lua_getfield(L, -1, "GetService");
		lua_insert(L, -2);
		lua_pushstring(L, "Players");
		lua_pcall(L, 2, 1, 0);

		lua_getfield(L, -1, "LocalPlayer");

		const auto localPlayer = *reinterpret_cast<uintptr_t*>(lua_touserdata(L, -1));

		std::transform(clickOption.begin(), clickOption.end(), clickOption.begin(), ::tolower);

		if (clickOption == "rightmouseclick")
			Roblox::FireRightMouseClick(clickDetector, distance, localPlayer);
		else if (clickOption == "mousehoverenter")
			Roblox::FireMouseHoverEnter(clickDetector, localPlayer);
		else if (clickOption == "mousehoverleave")
			Roblox::FireMouseHoverLeave(clickDetector, localPlayer);
		else
			Roblox::FireMouseClick(clickDetector, distance, localPlayer);
		return 0;
	}
	inline int fireproximityprompt(lua_State* LS) {
		luaL_checktype(LS, 1, LUA_TUSERDATA);

		uintptr_t ProximityPrompt = *(uintptr_t*)(lua_topointer(LS, 1));
		if (!ProximityPrompt)
			luaL_argerror(LS, 1, "Invalid proximity prompt!");

		Roblox::FireProximityPrompt(ProximityPrompt);
		return 0;
	};
	inline int firetouchinterest(lua_State* L) {
		luaL_checktype(L, 1, LUA_TUSERDATA);
		luaL_checktype(L, 2, LUA_TUSERDATA);
		luaL_checktype(L, 3, LUA_TNUMBER);

		int Toggle = lua_tonumber(L, 3);

		const auto Instance = *reinterpret_cast<uintptr_t*>(lua_touserdata(L, 1));
		const auto Instance2 = *reinterpret_cast<uintptr_t*>(lua_touserdata(L, 2));

		const auto Prim1 = *reinterpret_cast<std::uintptr_t*>(Instance + Offsets::Instance::Primitive);
		if (!Prim1)
			luaL_argerror(L, 1, "Failed to get Primitive 1");

		const auto Prim2 = *reinterpret_cast<std::uintptr_t*>(Instance2 + Offsets::Instance::Primitive);

		if (!Prim2)
			luaL_argerror(L, 2, "Failed to get Primitive 2");
		const auto Overlap = *reinterpret_cast<std::uintptr_t*>(Prim1 + Offsets::Instance::Overlap);

		Roblox::FireTouchInterest(Overlap, Prim1, Prim2, Toggle, true);
		return 0;
	}
	inline int getcallbackvalue(lua_State* LS) {
		IsInstance(LS, 1);
		luaL_checktype(LS, 2, LUA_TSTRING);

		const auto rawInstance = reinterpret_cast<uintptr_t>(lua_torawuserdata(LS, 1));
		int Atom;
		lua_tostringatom(LS, 2, &Atom);

		auto propertyName = reinterpret_cast<uintptr_t*>(Offsets::KTable)[Atom];
		if (propertyName == 0 || IsBadReadPtr(reinterpret_cast<void*>(propertyName), 0x10))
			luaL_argerrorL(LS, 2, "Invalid property!");

		const auto instanceClassDescriptor = *reinterpret_cast<uintptr_t*>(
			rawInstance + Offsets::Properties::ClassDescriptor);
		const auto Property = Roblox::GetProperty(
			instanceClassDescriptor + Offsets::Properties::PropertyDescriptor,
			&propertyName);
		if (Property == 0 || IsBadReadPtr(reinterpret_cast<void*>(Property), 0x10))
			luaL_argerrorL(LS, 2, "Invalid property!");

		const auto callbackStructureStart = rawInstance + *reinterpret_cast<uintptr_t*>(
			*reinterpret_cast<uintptr_t*>(Property) + 0x80);
		const auto hasCallback = *reinterpret_cast<uintptr_t*>(callbackStructureStart + 0x38);
		if (hasCallback == 0) {
			lua_pushnil(LS);
			return 1;
		}

		const auto callbackStructure = *reinterpret_cast<uintptr_t*>(callbackStructureStart + 0x18);
		if (callbackStructure == 0) {
			lua_pushnil(LS);
			return 1;
		}

		const auto ObjectRefs = *reinterpret_cast<uintptr_t*>(callbackStructure + 0x38);
		if (ObjectRefs == 0) {
			lua_pushnil(LS);
			return 1;
		}

		const auto ObjectRef = *reinterpret_cast<uintptr_t*>(ObjectRefs + 0x28);
		const auto RefId = *reinterpret_cast<int*>(ObjectRef + 0x14);

		lua_getref(LS, RefId);
		return 1;
		return 0;
	}
	inline int gethui(lua_State* LS)
	{
		lua_getglobal(LS, "__hiddeninterface");

		return 1;
	};
	inline int getinstances(lua_State* LS) {
		lua_pop(LS, lua_gettop(LS));

		HelpFuncs::GetEveryInstance(LS);

		if (!lua_istable(LS, -1)) { lua_pop(LS, 1); lua_pushnil(LS); return 1; };

		lua_newtable(LS);

		int index = 0;

		lua_pushnil(LS);
		while (lua_next(LS, -3) != 0) {

			if (!lua_isnil(LS, -1)) {
				lua_getglobal(LS, "typeof");
				lua_pushvalue(LS, -2);
				lua_pcall(LS, 1, 1, 0);

				std::string type = lua_tostring(LS, -1);
				lua_pop(LS, 1);

				if (type == "Instance") {
					lua_pushinteger(LS, ++index);

					lua_pushvalue(LS, -2);
					lua_settable(LS, -5);
				}
			}

			lua_pop(LS, 1);
		}

		lua_remove(LS, -2);

		return 1;
	};
	inline int getnilinstances(lua_State* LS)
	{
		lua_pop(LS, lua_gettop(LS));

		HelpFuncs::GetEveryInstance(LS);

		if (!lua_istable(LS, -1)) { lua_pop(LS, 1); lua_pushnil(LS); return 1; }

		lua_newtable(LS);

		int index = 0;

		lua_pushnil(LS);
		while (lua_next(LS, -3) != 0) {

			if (!lua_isnil(LS, -1)) {
				lua_getglobal(LS, "typeof");
				lua_pushvalue(LS, -2);
				lua_pcall(LS, 1, 1, 0);

				std::string type = lua_tostring(LS, -1);
				lua_pop(LS, 1);

				if (type == "Instance") {
					lua_getfield(LS, -1, "Parent");
					int parentType = lua_type(LS, -1);
					lua_pop(LS, 1);

					if (parentType == LUA_TNIL) {
						lua_pushinteger(LS, ++index);

						lua_pushvalue(LS, -2);
						lua_settable(LS, -5);
					}
				}
			}

			lua_pop(LS, 1);
		}

		lua_remove(LS, -2);

		return 1;
	};
}
