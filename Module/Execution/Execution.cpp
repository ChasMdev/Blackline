#include "Execution.hpp"
#include <Luau/Compiler.h>
#include <Luau/BytecodeBuilder.h>
#include <Luau/BytecodeUtils.h>
#include <Luau/Bytecode.h>

#include <lapi.h>
#include <lstate.h>
#include <lualib.h>
#include <Update/Offsets.hpp>

#include <Dependencies/zstd/include/zstd/xxhash.h>
#include <Dependencies/zstd/include/zstd/zstd.h>

#include "TaskScheduler/TaskScheduler.hpp"
using namespace Globals;

class LuauLoadByteCodeEncoderClass : public Luau::BytecodeEncoder {
	void encode(uint32_t* data, size_t count) override {
		for (auto i = 0; i < count;) {
			uint8_t op = LUAU_INSN_OP(data[i]);
			const auto oplen = Luau::getOpLength((LuauOpcode)op);
			BYTE* OpCodeLookUpTable = reinterpret_cast<BYTE*>(Offsets::OpCodeTableLookup);
			uint8_t new_op = op * 227;
			new_op = OpCodeLookUpTable[new_op];
			data[i] = (new_op) | (data[i] & ~0xff);
			i += oplen;
		}
	}
};

LuauLoadByteCodeEncoderClass LuauLoadEncoder;
uintptr_t capabilities = ~0ULL;

void BExecution::SetProtoCapabilities(Proto* proto) {
	proto->userdata = &capabilities;
	for (int i = 0; i < proto->sizep; i++)
	{
		SetProtoCapabilities(proto->p[i]);
	}
}

std::string BExecution::CompileScript(const std::string source) {
	Luau::CompileOptions options;
	options.debugLevel = 1;
	options.optimizationLevel = 1;
	const char* mutableGlobals[] = {
		"Game", "Workspace", "game", "plugin", "script", "shared", "workspace",
		"_G", "_ENV", nullptr
	};
	options.mutableGlobals = mutableGlobals;
	options.vectorLib = "Vector3";
	options.vectorCtor = "new";
	options.vectorType = "Vector3";

	return Luau::compile(source, options, {}, &LuauLoadEncoder);
}

void BExecution::Execute(std::string src) { 
	lua_State* l = Globals::ExploitThread;

	const int originalTop = lua_gettop(l);
	auto thread = lua_newthread(l);
	lua_pop(l, 1);
	Taskscheduler->SetThreadIdentity(thread, 8, MAXCAPABILITIES); // --> "CreateInstances" Permission in order to create new "LocalScript"
	auto Source = CompileScript(src + "\nscript = Instance.new('LocalScript');");
	if (luau_load(thread, "@Blackline", Source.c_str(), Source.length(), 0) != LUA_OK)
	{
		const char* err = lua_tostring(thread, -1);
		Roblox::Print(2, err);
		return;
	}
	Closure* closure = (Closure*)lua_topointer(thread, -1);
	if (closure && closure->l.p)
		Taskscheduler->SetProtoCapabilities(closure->l.p);

	lua_getglobal(l, "task");
	lua_getfield(l, -1, "defer");
	lua_remove(l, -2);
	lua_xmove(thread, l, 1);

	if (lua_pcall(l, 1, 0, 0) != LUA_OK) {
		const char* err = lua_tostring(l, -1);
		if (err) Roblox::Print(2, err);
		lua_pop(l, 1);
		return;
	}

	lua_settop(thread, 0);
	lua_settop(l, originalTop);
}