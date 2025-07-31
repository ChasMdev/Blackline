#pragma once
#include <lstate.h>
#include <lualib.h>
#include <lapi.h>
#include <lmem.h>
#include <lgc.h>
#include <Update/Offsets.hpp>
#include <zstd/include/zstd/zstd.h>
#include <zstd/include/zstd/xxhash.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <cryptopp/cryptlib.h>
#include <Environment/Environment.hpp>
#include <regex>
#include <cryptopp/config_ns.h>
#include <cryptopp/base64.h>
#include <TaskScheduler/TaskScheduler.hpp>


static bool IsMetamethod(const char* Metamethod)
{
    if (std::string(Metamethod).empty())
        return false;

    const std::unordered_set<std::string> Allowed = {
        "__namecall",
        "__newindex",
        "__index"
    };
    return Allowed.find(Metamethod) != Allowed.end();
};


namespace Metatable {
    int getrawmetatable(lua_State* LS) {
        luaL_checkany(LS, 1);

        if (!lua_getmetatable(LS, 1))
            lua_pushnil(LS);

        return 1;
    };

    int setrawmetatable(lua_State* LS) {
        luaL_argexpected(LS, lua_istable(LS, 1) || lua_islightuserdata(LS, 1) || lua_isuserdata(LS, 1) || lua_isbuffer(LS, 1) || lua_isvector(LS, 1), 1, "Expected a table or an userdata or a buffer or a vector");

        luaL_argexpected(LS, lua_istable(LS, 2) || lua_isnil(LS, 2), 2, "Expected table or nil");

        const bool OldState = lua_getreadonly(LS, 1);

        lua_setreadonly(LS, 1, false);

        lua_setmetatable(LS, 1);

        lua_setreadonly(LS, 1, OldState);

        lua_ref(LS, 1);

        return 1;
    };

    int isreadonly(lua_State* LS) {
        lua_pushboolean(LS, lua_getreadonly(LS, 1));
        return 1;
    };

    int setreadonly(lua_State* LS) {
        luaL_checktype(LS, 1, LUA_TTABLE);
        luaL_checktype(LS, 2, LUA_TBOOLEAN);

        lua_setreadonly(LS, 1, lua_toboolean(LS, 2));

        return 0;
    };

    int getnamecallmethod(lua_State* LS) {
        auto Namecall = lua_namecallatom(LS, nullptr);
        if (Namecall == nullptr)
            lua_pushnil(LS);
        else
            lua_pushstring(LS, Namecall);

        return 1;
    };

    int hookmetamethod(lua_State* LS) {
        luaL_checkany(LS, 1);
        luaL_checkstring(LS, 2);
        luaL_checkany(LS, 3);

        if (!lua_getmetatable(LS, 1)) {
            lua_pushnil(LS);
            return 1;
        }

        int Table = lua_gettop(LS);
        const char* Method = lua_tostring(LS, 2);
        if (!IsMetamethod(Method))
            return 0;

        auto OldReadOnly = lua_getreadonly(LS, 1);

        lua_getfield(LS, Table, Method);
        lua_pushvalue(LS, -1);

        lua_setreadonly(LS, Table, false);

        lua_pushvalue(LS, 3);
        lua_setfield(LS, Table, Method);

        lua_setreadonly(LS, Table, OldReadOnly);

        lua_remove(LS, Table);

        return 1;
    };
};