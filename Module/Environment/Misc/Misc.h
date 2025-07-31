#pragma once
#include <lstate.h>
#include <lualib.h>
#include <lapi.h>
#include <lmem.h>
#include <lgc.h>
#include <Misc/Utils.hpp>
#include <TaskScheduler/TaskScheduler.hpp>
#include <Update/Offsets.hpp>
#include <zstd/include/zstd/zstd.h>
#include <zstd/include/zstd/xxhash.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <cryptopp/cryptlib.h>



namespace Misc {

	inline int setfpscap(lua_State* l) {
		luaL_checknumber(l, 1);

		Taskscheduler->SetFPS(lua_tointeger(l, 1));

		return 1;
	}
	inline int getfpscap(lua_State* l) {
		lua_pushinteger(l, Taskscheduler->GetFPS());
		return 1;
	}

	inline int identifyexecutor(lua_State* l) {
		lua_pushstring(l, "Blackline"); // name
		lua_pushstring(l, "INDEV 1.1.0"); // version
		return 2;
	}
	inline int setclipboard(lua_State* LS) {
		luaL_checktype(LS, 1, LUA_TSTRING);

		std::string content = lua_tostring(LS, 1);

		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, content.size() + 1);
		memcpy(GlobalLock(hMem), content.data(), content.size());
		GlobalUnlock(hMem);
		OpenClipboard(0);
		EmptyClipboard();
		SetClipboardData(CF_TEXT, hMem);
		CloseClipboard();
		return 0;
	};


}