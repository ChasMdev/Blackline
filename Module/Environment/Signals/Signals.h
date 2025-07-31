#pragma once
#include <unordered_map>

#include "lgc.h"
#include "lua.h"
#include "lualib.h"
#include "lstate.h"
#include <Luau/BytecodeBuilder.h>

#include "lapi.h"
#include "Environment/Reflection/Reflection.h"


struct weak_thread_ref_t2 {
    std::atomic< std::int32_t > _refs;
    lua_State* thread;
    std::int32_t thread_ref;
    std::int32_t object_id;
    std::int32_t unk1;
    std::int32_t unk2;

    weak_thread_ref_t2(lua_State* L)
        : thread(L), thread_ref(NULL), object_id(NULL), unk1(NULL), unk2(NULL) {
    }
};
struct live_thread_ref
{
    int __atomic_refs; // 0
    lua_State* th; // 8
    int thread_id; // 16
    int ref_id; // 20
};
struct functionscriptslot_t {
    void* vftable;
    std::uint8_t pad_0[104];
    live_thread_ref* func_ref; // 112

};
struct slot_t {
    int64_t unk_0; // 0
    void* func_0; // 8

    slot_t* next; // 16
    void* __atomic; // 24

    int64_t sig; // 32

    void* func_1; // 40

    functionscriptslot_t* storage; // 48
};

struct SignalConnectionBridge {
    slot_t* islot; // 0
    int64_t __shared_reference_islot; // 8

    int64_t unk0; // 16
    int64_t unk1; // 24
};


namespace Lua_Objects {
    inline std::unordered_map<slot_t*, int64_t> NodeMap;

    class RobloxConnection_t {
    private:
        slot_t* node;
        int64_t sig;
        live_thread_ref* func_ref;
        bool bIsLua;

    public:
        RobloxConnection_t(slot_t* node, bool isLua) : node(node), bIsLua(isLua)
        {
            if (isLua)
            {
                if (node->storage && (node->storage->func_ref != nullptr))
                {
                    func_ref = node->storage->func_ref;
                }
                else
                    func_ref = nullptr;
            }
            else
            {
                func_ref = nullptr;
            }
        }
        bool isLuaConn() { return bIsLua; }
        auto get_function_ref() -> int { return func_ref->ref_id; }
        auto get_thread_ref() -> int { return func_ref->thread_id; }
        auto get_luathread() -> lua_State* { return func_ref->th; }
        auto get_node() -> slot_t* { return this->node; }

        auto is_enabled() -> bool { return node->sig != NULL; }

        auto disable() -> void
        {
            if (!NodeMap[node])
            {
                NodeMap[node] = node->sig;
            }
            node->sig = NULL;
        }

        auto enable() -> void
        {
            if (!node->sig)
            {
                node->sig = NodeMap[node];
            }
        }
    };
}

struct signal_t;

struct signal_connection_t {
    char padding[16];
    int thread_idx; // 0x10
    int func_idx; //0x14
};

struct signal_data_t {
    uint64_t padding1;
    signal_t* root; //0x8
    uint64_t padding2[12];
    signal_connection_t* connection_data; //0x70
};

struct signal_t {
    uint64_t padding1[2];
    signal_t* next; //0x10
    uint64_t padding2;
    uint64_t state;
    uint64_t padding3;
    signal_data_t* signal_data; //0x30
};

struct connection_object {
    signal_t* signal;
    uint64_t state;
    uint64_t metatable;
    uint64_t root;
};

inline int TRobloxScriptConnection = 0;
inline std::unordered_map<std::uintptr_t, LuaTable*> connections;

struct connection_struct
{
    std::uintptr_t connection;
    std::uintptr_t oldEnabled;
    bool disconnected = false;
    bool ForeignState = false;
};

inline auto findSavedMetatable(uintptr_t signalPtr) {
    const auto it = connections.find(signalPtr);
    return it != connections.end() ? it->second : nullptr;
}

static int disable_connection(lua_State* ls)
{
    auto pInfo = reinterpret_cast<Lua_Objects::RobloxConnection_t*>(lua_touserdata(ls, lua_upvalueindex(1)));
    if (!pInfo)
        luaL_error(ls, "Invalid connection userdata in disable_connection");
    pInfo->disable();
    return 0;
}

static int enable_connection(lua_State* ls)
{
    auto pInfo = reinterpret_cast<Lua_Objects::RobloxConnection_t*>(lua_touserdata(ls, lua_upvalueindex(1)));
    if (!pInfo)
        luaL_error(ls, "Invalid connection userdata in enable_connection");
    pInfo->enable();
    return 0;
}

inline int connection_blank(lua_State* rl) {
    return 0;
}

static int fire_connection(lua_State* ls)
{
    const auto nargs = lua_gettop(ls);

    lua_pushvalue(ls, lua_upvalueindex(1));
    if (lua_isfunction(ls, -1))
    {
        lua_insert(ls, 1);
        lua_call(ls, nargs, 0);
    }

    return 0;
}

static int defer_connection(lua_State* ls)
{
    const auto nargs = lua_gettop(ls);

    lua_getglobal(ls, "task");
    lua_getfield(ls, -1, "defer");
    lua_remove(ls, -2);

    lua_pushvalue(ls, lua_upvalueindex(1));
    if (!lua_isfunction(ls, -1))
        return 0;

    lua_insert(ls, 1);
    lua_insert(ls, 1);

    if (lua_pcall(ls, nargs + 1, 1, 0) != LUA_OK)
        luaL_error(ls, "Error in defer_connection: %s", lua_tostring(ls, -1));

    return 1;
}

static int disconnect_connection(lua_State* ls)
{
    auto pInfo = reinterpret_cast<Lua_Objects::RobloxConnection_t*>(lua_touserdatatagged(ls, lua_upvalueindex(1), 72));
    if (!pInfo)
        luaL_error(ls, "Invalid connection userdata in disconnect_connection");

    if (!pInfo->is_enabled())
        luaL_error(ls, ("Cannot Disconnect a Disabled connection! ( Enable it first before Disconnecting. )"));

    auto pUd = reinterpret_cast<SignalConnectionBridge*>(lua_newuserdatatagged(ls, sizeof(SignalConnectionBridge), TRobloxScriptConnection));
    if (!pUd)
        luaL_error(ls, "Failed to create SignalConnectionBridge userdata");

    pUd->islot = pInfo->get_node();
    pUd->unk0 = 0;
    pUd->unk1 = 0;

    lua_getfield(ls, LUA_REGISTRYINDEX, ("RobloxScriptConnection"));
    if (!lua_istable(ls, -1))
        luaL_error(ls, "RobloxScriptConnection metatable not found in registry");
    lua_setmetatable(ls, -2);

    lua_getfield(ls, -1, ("Disconnect"));
    if (!lua_isfunction(ls, -1))
        luaL_error(ls, "Disconnect function not found in RobloxScriptConnection metatable");

    lua_pushvalue(ls, -2);
    if (lua_pcall(ls, 1, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(ls, -1);
        luaL_error(ls, ("Error while disconnecting connection (%s)"), err);
    }

    return 0;
}

inline int connection_index(lua_State* rl) {
    std::string key = std::string(lua_tolstring(rl, 2, nullptr));
    auto connection = (connection_object*)lua_touserdata(rl, 1);
    uintptr_t connection2 = *(uintptr_t*)lua_touserdata(rl, 1);

    if (key == "Enabled" || key == "enabled") {
        lua_pushboolean(rl, !(connection->signal->state == 0));
        return 1;
    }

    if (key == "Function" || key == "function" || key == "Fire" || key == "fire" || key == "Defer" || key == "defer") {
        int signal_data = *(int*)&connection->signal->signal_data;
        if (signal_data && *(int*)&connection->signal->signal_data->connection_data) {
            int index = connection->signal->signal_data->connection_data->func_idx;
            lua_rawgeti(rl, LUA_REGISTRYINDEX, index);

            if (lua_type(rl, -1) != LUA_TFUNCTION)
                lua_pushcclosure(rl, connection_blank, 0, 0, 0);

            return 1;
        }

        lua_pushcclosure(rl, connection_blank, 0, 0, 0);
        return 1;
    }

    if (key == "LuaConnection") {
        int signal_data = *(int*)&connection->signal->signal_data;
        if (signal_data && *(int*)&connection->signal->signal_data->connection_data) {
            int index = connection->signal->signal_data->connection_data->func_idx;

            lua_rawgeti(rl, LUA_REGISTRYINDEX, index);
            auto func_tval = (TValue*)luaA_toobject(rl, -1);
            auto cl = (Closure*)func_tval->value.gc;
            bool lua = !cl->isC;

            lua_pop(rl, 1);
            lua_pushboolean(rl, lua);
            return 1;
        }

        lua_pushboolean(rl, false);
        return 1;
    }

    if (key == "ForeignState") {
        int signal_data = *(int*)&connection->signal->signal_data;
        if (signal_data && *(int*)&connection->signal->signal_data->connection_data) {
            int index = connection->signal->signal_data->connection_data->func_idx;

            lua_rawgeti(rl, LUA_REGISTRYINDEX, index);
            auto func_tval = (TValue*)luaA_toobject(rl, -1);
            auto cl = (Closure*)func_tval->value.gc;
            bool c = cl->isC;

            lua_pop(rl, 1);
            lua_pushboolean(rl, c);
            return 1;
        }

        lua_pushboolean(rl, false);
        return 1;
    }

    if (key == "Disconnect" || key == "disconnect") {
        lua_pushcclosure(rl, disconnect_connection, 0, 0, 0);
        return 1;
    }

    if (key == "Disable" || key == "disable") {
        lua_pushcclosure(rl, disable_connection, 0, 0, 0);
        return 1;
    }

    if (key == "Enable" || key == "enable") {
        lua_pushcclosure(rl, enable_connection, 0, 0, 0);
        return 1;
    }

    if (key == "Thread") {
        int signal_data = *(int*)&connection->signal->signal_data;
        if (signal_data && *(int*)&connection->signal->signal_data->connection_data) {
            int index = connection->signal->signal_data->connection_data->thread_idx;
            lua_rawgeti(rl, LUA_REGISTRYINDEX, index);

            if (lua_type(rl, -1) != LUA_TTHREAD)
                lua_pushthread(rl);

            return 1;
        }
    }

    luaL_error(rl, "Invalid idx");
    return 0;
}


static int mt_newindex(lua_State* ls)
{
    auto pInfo = reinterpret_cast<Lua_Objects::RobloxConnection_t*>(lua_touserdatatagged(ls, 1, 72));
    if (!pInfo)
        luaL_error(ls, "Invalid connection userdata in __newindex");

    const char* key = luaL_checkstring(ls, 2);

    if (strcmp(key, "Enabled") == 0)
    {
        bool enabled = luaL_checkboolean(ls, 3);
        if (enabled)
            pInfo->enable();
        else
            pInfo->disable();
    }

    return 0;
}

static int mt_index(lua_State* ls)
{
    const auto pInfo = reinterpret_cast<Lua_Objects::RobloxConnection_t*>(lua_touserdatatagged(ls, 1, 72));
    const char* key = luaL_checkstring(ls, 2);

    bool is_l_connection = true;

    if (strcmp(key, ("Enabled")) == 0 || strcmp(key, ("Connected")) == 0)
    {
        lua_pushboolean(ls, pInfo->is_enabled());
        return 1;
    }
    else if (strcmp(key, ("Disable")) == 0)
    {
        lua_pushvalue(ls, 1);
        lua_pushcclosure(ls, disable_connection, nullptr, 1);
        return 1;
    }
    else if (strcmp(key, ("Enable")) == 0)
    {
        lua_pushvalue(ls, 1);
        lua_pushcclosure(ls, enable_connection, nullptr, 1);
        return 1;
    }
    else if (strcmp(key, ("LuaConnection")) == 0)
    {
        lua_pushboolean(ls, pInfo->isLuaConn());
        return 1;
    }
    else if (strcmp(key, ("ForeignState")) == 0)
    {
        if (pInfo->isLuaConn() == false) {
            lua_pushboolean(ls, true);
            return 1;
        }

        auto th = pInfo->get_luathread();
        if (!th)
        {
            const auto ref = pInfo->get_thread_ref();
            if (ref) {
                lua_getref(ls, ref);
                if (!lua_isthread(ls, -1))
                {
                    lua_pushboolean(ls, true);
                    return 1;
                }
                else
                {
                    th = lua_tothread(ls, -1);
                    lua_pop(ls, 1);
                }
            }
            else {
                lua_pushboolean(ls, true);
                return 1;
            }
        }

        lua_pushboolean(ls, (th->global != ls->global));
        return 1;
    }
    else if (strcmp(key, ("Function")) == 0)
    {
        if (pInfo->isLuaConn() == false) {
            lua_pushnil(ls);
            return 1;
        }

        auto th = pInfo->get_luathread();
        if (!th)
        {
            const auto ref = pInfo->get_thread_ref();
            if (ref) {
                lua_getref(ls, ref);
                if (!lua_isthread(ls, -1))
                {
                    lua_pushnil(ls);
                    return 1;
                }
                else
                {
                    th = lua_tothread(ls, -1);
                    lua_pop(ls, 1);
                }
            }
            else
            {
                lua_pushnil(ls);
                return 1;
            }
        }

        if (th->global != ls->global)
        {
            lua_pushnil(ls);
            return 1;
        }

        const auto ref = pInfo->get_function_ref();
        if (ref) {
            lua_getref(ls, ref);
            if (!lua_isfunction(ls, -1))
            {
                lua_pushnil(ls);
            }
        }
        else
            lua_pushnil(ls);
        return 1;
    }
    else if (strcmp(key, ("Thread")) == 0)
    {
        if (pInfo->isLuaConn() == false) {
            lua_pushnil(ls);
            return 1;
        }

        auto th = pInfo->get_luathread();
        if (!th)
        {
            const auto ref = pInfo->get_thread_ref();
            if (ref) {
                lua_getref(ls, ref);
                if (!lua_isthread(ls, -1))
                {
                    lua_pushnil(ls);
                    return 1;
                }
                else
                {
                    return 1;
                }
            }
            else {
                lua_pushnil(ls);
                return 1;
            }
        }

        luaC_threadbarrier(ls) setthvalue(ls, ls->top, th) ls->top++;
        return 1;
    }
    else if (strcmp(key, ("Fire")) == 0)
    {
        lua_getfield(ls, 1, ("Function"));
        lua_pushcclosure(ls, fire_connection, nullptr, 1);
        return 1;
    }
    else if (strcmp(key, ("Defer")) == 0)
    {
        lua_getfield(ls, 1, ("Function"));
        lua_pushcclosure(ls, defer_connection, nullptr, 1);
        return 1;
    }
    else if (strcmp(key, ("Disconnect")) == 0)
    {
        lua_pushvalue(ls, 1);
        lua_pushcclosure(ls, disconnect_connection, nullptr, 1);
        return 1;
    }

    return 0;
}
inline int allcons(lua_State* ls)
{
    luaL_checktype(ls, 1, LUA_TUSERDATA);

    if (strcmp(luaL_typename(ls, 1), ("RBXScriptSignal")) != 0)
        luaL_typeerror(ls, 1, ("RBXScriptSignal"));

    const auto stub = ([](lua_State*) -> int { return 0; });

    static void* funcScrSlotvft = nullptr;
    static void* waitvft = nullptr;
    static void* oncevft = nullptr;
    static void* connectparalellvft = nullptr;

    lua_getfield(ls, 1, "Connect");
    {
        lua_pushvalue(ls, 1);
        lua_pushcfunction(ls, stub, nullptr);
    }
    if (lua_pcall(ls, 2, 1, 0) != 0)
        luaL_error(ls, "Error calling Connect stub: %s", lua_tostring(ls, -1));

    auto sigconbr = reinterpret_cast<SignalConnectionBridge*>(lua_touserdata(ls, -1));
    if (!sigconbr)
        luaL_error(ls, "Failed to retrieve connection stub");

    if (!funcScrSlotvft)
        funcScrSlotvft = sigconbr->islot->storage->vftable;

    auto Node = sigconbr->islot->next;

    if (!TRobloxScriptConnection)
        TRobloxScriptConnection = lua_userdatatag(ls, -1);

    lua_getfield(ls, -1, "Disconnect");
    {
        lua_insert(ls, -2);
    }
    if (lua_pcall(ls, 1, 0, 0) != 0)
        luaL_error(ls, "Error calling Disconnect on stub: %s", lua_tostring(ls, -1));

    if (!waitvft && !oncevft && !connectparalellvft) {
        lua_getfield(ls, 1, "Once");
        {
            lua_pushvalue(ls, 1);
            lua_pushcfunction(ls, stub, nullptr);
        }
        lua_call(ls, 2, 1);
        auto sigconbr_once = reinterpret_cast<SignalConnectionBridge*>(lua_touserdata(ls, -1));
        if (!sigconbr_once)
            luaL_error(ls, "Failed to retrieve Once stub");
        oncevft = sigconbr_once->islot->storage->vftable;
        lua_getfield(ls, -1, "Disconnect");
        {
            lua_insert(ls, -2);
        }
        lua_call(ls, 1, 0);
    }

    int idx = 1;
    lua_newtable(ls);

    while (Node)
    {
        auto conn = reinterpret_cast<Lua_Objects::RobloxConnection_t*>(
            lua_newuserdatatagged(ls, sizeof(Lua_Objects::RobloxConnection_t), 72)
            );
        if (Node->storage && Node->storage->vftable == funcScrSlotvft ||
            Node->storage->vftable == waitvft ||
            Node->storage->vftable == oncevft ||
            Node->storage->vftable == connectparalellvft)
        {
            *conn = Lua_Objects::RobloxConnection_t(Node, true);
        }
        else
        {
            *conn = Lua_Objects::RobloxConnection_t(Node, false);
        }

        lua_newtable(ls);
        lua_pushcfunction(ls, mt_index, nullptr);
        lua_setfield(ls, -2, ("__index"));
        lua_pushcfunction(ls, mt_newindex, nullptr);
        lua_setfield(ls, -2, ("__newindex"));
        lua_pushstring(ls, "Event");
        lua_setfield(ls, -2, ("__type"));
        lua_setmetatable(ls, -2);

        lua_rawseti(ls, -2, idx++);
        Node = Node->next;
    }

    return 1;
}

namespace Signals
{
    inline int firesignal(lua_State* L)
    {
        int nargs = lua_gettop(L);
        lua_pushcfunction(L, allcons, "");
        lua_pushvalue(L, 1);
        lua_pcall(L, 1, 1, 0);
        lua_pushnil(L);

        while (lua_next(L, -2)) {
            lua_getfield(L, -1, "Function");
            for (int i = 0; i < nargs - 1; i++)
                lua_pushvalue(L, i + 2);
            lua_pcall(L, nargs - 1, 0, 0);
            lua_pop(L, 1);
        }
        return 0;
    }
    int push_connection_mt(lua_State* L, uintptr_t signalPtr) {
        connection_struct* connection = (connection_struct*)lua_newuserdata(L, sizeof(connection_struct));
        lua_ref(L, -1);
        lua_pop(L, 1);

        connection->connection = signalPtr;
        connection->oldEnabled = *reinterpret_cast<uintptr_t*>(signalPtr + 0x20);

        lua_newtable(L);

        lua_pushlightuserdata(L, connection);
        lua_setfield(L, -2, "Object");

        lua_getfield(L, LUA_REGISTRYINDEX, "connection_signal_object");
        lua_setmetatable(L, -2);
        return 1;
    };
    inline int getconnections(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TUSERDATA);

        std::string Check = luaL_typename(L, 1);
        if (Check != "RBXScriptSignal")
            luaL_argerror(L, 1, ("Expected RBXScriptSignal got " + Check).c_str());

        lua_getfield(L, 1, "Connect");
        lua_pushvalue(L, 1);
        lua_pushcclosure(L, [](lua_State*) -> int { return 0; }, 0, 0);
        lua_pcall(L, 2, 1, 0);

        const auto UserData = *reinterpret_cast<std::uintptr_t*>(lua_touserdata(L, -1));
        auto signal = *reinterpret_cast<std::uintptr_t*>(UserData + 0x10);

        lua_getfield(L, -1, "Disconnect");
        lua_insert(L, -2);
        lua_pcall(L, 1, 0, 0);
        lua_remove(L, 1);

        lua_newtable(L);

        if (!signal) {
            return 1;
        }

        auto count = 1;

        do {
            auto cachedCon = findSavedMetatable(signal);
            if (!cachedCon) {
                if (push_connection_mt(L, signal)) {
                    LuaTable* newTBL = hvalue(luaA_toobject(L, -1));
                    lua_ref(L, -1);

                    connections[signal] = newTBL;

                    lua_rawseti(L, -2, count++);
                }
            }
            else {
                sethvalue(L, L->top, cachedCon);
                incr_top(L);
                lua_ref(L, -1);

                lua_rawseti(L, -2, count++);
            }

            signal = *reinterpret_cast<std::uintptr_t*>(signal + 0x10);
        } while (signal != 0);

        return 1;
    }
    using TRaiseEventInvocation = void(__fastcall*)(uintptr_t*, uintptr_t*, std::vector<RBX::Reflection::Variant>*, RBX::Reflection::EventSource::RemoteEventInvocationTargetOptions*);
    inline auto RaiseEventInvocation = (TRaiseEventInvocation)Offsets::RaiseEventInvocation;
    using TGetValues = std::shared_ptr<RBX::Reflection::Tuple>(__fastcall*)(std::shared_ptr<RBX::Reflection::Tuple>*, lua_State*);
    inline auto GetValues = (TGetValues)Offsets::GetValues;
    inline int replicatesignal(lua_State* L) {
        if (strcmp(luaL_typename(L, 1), "RBXScriptSignal") != 0)
        {
            luaL_typeerror(L, 1, "RBXScriptSignal");
            return 0;
        }

        uintptr_t UserData = *reinterpret_cast<uintptr_t*>(lua_touserdata(L, 1));
        uintptr_t Descriptor = *(uintptr_t*)(UserData);
        uintptr_t Instance = *(uintptr_t*)(UserData + 0x18);

        *(BYTE*)(Offsets::LockViolationInstanceCrash) = FALSE;

        RBX::Reflection::EventSource::RemoteEventInvocationTargetOptions Options;
        Options.target = 0;
        Options.isExcludeTarget = RBX::Reflection::EventSource::OnlyTarget;

        std::vector<RBX::Reflection::Variant> Arguments;

        std::shared_ptr<RBX::Reflection::Tuple> Tuple;
        GetValues(&Tuple, L);

        int c = 1;
        for (auto it = Tuple->values.begin() + 1; it != Tuple->values.end(); ++it) {
            const auto& variant = *it;
            if (!variant._type) continue;
            if (!variant._type->reflectionType) continue;

            Arguments.push_back(variant);
            c++;
        }

        RaiseEventInvocation(reinterpret_cast<uintptr_t*>(Instance), reinterpret_cast<uintptr_t*>(Descriptor), &Arguments, &Options);

        return 0;
    }

}