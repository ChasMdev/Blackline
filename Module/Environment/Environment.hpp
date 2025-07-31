#pragma once
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <Luau/Compiler.h>
#include <Luau/BytecodeBuilder.h>
#include <Luau/BytecodeUtils.h>
#include <Luau/Bytecode.h>

#include <lapi.h>
#include <lstate.h>
#include <lualib.h>
#include <lobject.h>
#include <TaskScheduler/TaskScheduler.hpp>
 

class Environment {
private:
	

	// functions
	//static void InitGameHooks();
	//static void NamecallHook();

public:
	static void Init(lua_State* L);
	// defs
	static std::unordered_map<Closure*, Closure*> HookedFunctions;
	static std::map<Closure*, lua_CFunction> ExecutorClosures;
	static std::unordered_map<Closure*, Closure*> Newcclosures;
	static std::unordered_map<Closure*, Closure*> Newlclosures;
	static std::unordered_set<Closure*> ExecutorFunctions;

	// register functions
	static int ClosuresHandler(lua_State* L);
	static lua_CFunction GetClosure(Closure* Closure);
	static void SetClosure(Closure* Closure, lua_CFunction Function);
	static void PushClosure(lua_State* L, lua_CFunction Function, const char* debugname, int nup);
	static void PushWrappedClosure(lua_State* L, lua_CFunction Function, const char* debugname, int nup, lua_Continuation count);
	static void AddFunction(lua_State* L, const char* globalname, lua_CFunction function);
	static void NewTableFunction(lua_State* L, const char* globalname, lua_CFunction function);
};

#define basicCapabilities 0x3ffffff00ull
#define ourCapability (1ull << 48ull)

