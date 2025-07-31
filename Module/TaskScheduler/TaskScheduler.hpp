#pragma once
#include <iostream>
#include <memory>
#include <unordered_set>
#include <Luau/BytecodeBuilder.h>
#include <lstate.h>
#include <lua.h>
#include "lualib.h"


static struct lua_State;
static struct Proto;
static struct Closure;

inline uintptr_t MaxCapabilities = ~0ULL;

class BTaskscheduler {
private:

	static uintptr_t GetScriptContext(uintptr_t DataModel);
	static bool IsProtoInternal(const Proto* proto, std::unordered_set<const Proto*>& visited);
	static void ElevateProtoInternal(Proto* proto, uint64_t* capabilityToSet);
public:
	static uintptr_t GetJobOffset(const char* jobname, uintptr_t jobListBase, int maxJobs = 256);
	static uintptr_t GetJobList();
	static uintptr_t GetDataModel();
	static uintptr_t GetLuaState(uintptr_t DataModel);
	static lua_State* CreateThread(uintptr_t Thread);

	static bool SetFPS(int fps);
	static int GetFPS();

	static void SetProtoCapabilities(Proto* proto);
	static void ElevateProto(Closure* cl);
	static void SetThreadIdentity(lua_State* l, int identity, uintptr_t capabilities);
	static bool CheckThread(lua_State* l);
	static bool CheckProto(const Proto* proto);

	static std::string GetPlaceID(lua_State* L);
	static std::string GetGameID(lua_State* L);
	static std::string GetJobID(lua_State* L);

};

inline auto Taskscheduler = std::make_unique<BTaskscheduler>();