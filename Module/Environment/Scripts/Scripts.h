#pragma once
#include <lstate.h>
#include <lualib.h>
#include <lapi.h>
#include <lmem.h>
#include <lgc.h>
#include <Misc/Utils.hpp>
#include <TaskScheduler/TaskScheduler.hpp>
#include <zstd/include/zstd/zstd.h>
#include <zstd/include/zstd/xxhash.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <cryptopp/cryptlib.h>
#include <Windows.h>
#include <functional>
#include <Update/Offsets.hpp>
#include <Execution/Execution.hpp>
#include <Hooks/Jobs/JobHooker.hpp>
#include <Environment/Environment.hpp>
#include <mutex>

#include "ltable.h"

namespace HelperClass {
	template<typename T>
	static std::string hash_with_algo(const std::string& Input)
	{
		T Hash;
		std::string Digest;

		CryptoPP::StringSource SS(Input, true,
			new CryptoPP::HashFilter(Hash,
				new CryptoPP::HexEncoder(
					new CryptoPP::StringSink(Digest), false
				)));

		return Digest;
	}
	const auto DecompressBytecode = [&](std::string_view CompressedInput) -> std::string {
		const uint8_t BytecodeSignature[4] = { 'R', 'S', 'B', '1' };
		const int BytecodeHashMultiplier = 41;
		const int BytecodeHashSeed = 42;

		if (CompressedInput.size() < 8)
			return ("Compressed data too short");

		std::vector<uint8_t> CompressedData(CompressedInput.begin(), CompressedInput.end());
		std::vector<uint8_t> HeaderBuffer(4);

		for (size_t i = 0; i < 4; ++i)
		{
			HeaderBuffer[i] = CompressedData[i] ^ BytecodeSignature[i];
			HeaderBuffer[i] = (HeaderBuffer[i] - i * BytecodeHashMultiplier) % 256;
		}

		for (size_t i = 0; i < CompressedData.size(); ++i)
			CompressedData[i] ^= (HeaderBuffer[i % 4] + i * BytecodeHashMultiplier) % 256;

		uint32_t HashValue = 0;
		for (size_t i = 0; i < 4; ++i)
			HashValue |= HeaderBuffer[i] << (i * 8);

		uint32_t Rehash = XXH32(CompressedData.data(), CompressedData.size(), BytecodeHashSeed);
		if (Rehash != HashValue)
			return ("Hash mismatch during decompression");

		uint32_t DecompressedSize = 0;
		for (size_t i = 4; i < 8; ++i)
			DecompressedSize |= CompressedData[i] << ((i - 4) * 8);

		CompressedData = std::vector<uint8_t>(CompressedData.begin() + 8, CompressedData.end());
		std::vector<uint8_t> Result(DecompressedSize);

		size_t const ActualDecompressdSize = ZSTD_decompress(Result.data(), DecompressedSize, CompressedData.data(), CompressedData.size());
		if (ZSTD_isError(ActualDecompressdSize))
			return std::string(("ZSTD decompression error: ")) + std::string(ZSTD_getErrorName(ActualDecompressdSize));

		Result.resize(ActualDecompressdSize);

		return std::string(Result.begin(), Result.end());
		};


	std::string sha256(const std::string& input) {
		CryptoPP::SHA256 hash;
		std::string digest;

		CryptoPP::StringSource s(input, true,
			new CryptoPP::HashFilter(hash,
				new CryptoPP::HexEncoder(
					new CryptoPP::StringSink(digest), false
				)
			)
		);

		return digest;
	}
	std::string sha384(const std::string& input) {
		CryptoPP::SHA384 hash;
		std::string digest;

		CryptoPP::StringSource s(input, true,
			new CryptoPP::HashFilter(hash,
				new CryptoPP::HexEncoder(
					new CryptoPP::StringSink(digest), false
				)
			)
		);

		return digest;
	}
	static int GetEveryInstance(lua_State* L)
	{
		lua_pushvalue(L, LUA_REGISTRYINDEX);
		lua_pushlightuserdata(L, (void*)Roblox::PushInstance);
		lua_gettable(L, -2);
		return 1;
	};

	static std::string GetBytecode(std::uint64_t Address) {
		uintptr_t str = Address + 0x10;
		uintptr_t data;

		if (*reinterpret_cast<std::size_t*>(str + 0x18) > 0xf) {
			data = *reinterpret_cast<uintptr_t*>(str);
		}
		else {
			data = str;
		}

		std::string ee;
		std::size_t len = *reinterpret_cast<std::size_t*>(str + 0x10);
		ee.reserve(len);

		for (unsigned i = 0; i < len; i++) {
			ee += *reinterpret_cast<char*>(data + i);
		}

		return ee;
	}

	static std::string RequestBytecode(uintptr_t scriptPtr, bool Decompress) {
		uintptr_t temp[0x4];
		std::memset(temp, 0, sizeof(temp));

		Roblox::RequestCode((uintptr_t)temp, scriptPtr);

		uintptr_t bytecodePtr = temp[1];

		if (!bytecodePtr) {
			return "Nil";
		}

		std::string Compressed = GetBytecode(bytecodePtr);
		if (Compressed.size() <= 8) {
			return "Nil";
		}

		if (!Decompress)
		{
			return Compressed;
		}
		else
		{
			std::string Decompressed = DecompressBytecode(Compressed);
			if (Decompressed.size() <= 8) {
				return "Nil";
			}

			return Decompressed;
		}
	}

}
namespace Scripts {
	inline int getcallingscript(lua_State* LS) {
		std::uintptr_t scriptPtr = *(std::uintptr_t*)LS->userdata + Offsets::Scripts::ScriptInstance;
		if (!scriptPtr)
		{
			lua_pushnil(LS);
			return 1;
		}

		Roblox::PushInstance(LS, (void*)scriptPtr);
		return 1;
	};
	inline int getrunningscripts(lua_State* LS) {
		lua_newtable(LS);

		typedef struct {
			lua_State* State;
			int itemsFound;
			std::map< uintptr_t, bool > map;
		} GCOContext;

		auto gcCtx = GCOContext{ LS, 0 };

		const auto ullOldThreshold = LS->global->GCthreshold;
		LS->global->GCthreshold = SIZE_MAX;

		luaM_visitgco(LS, &gcCtx, [](void* ctx, lua_Page* pPage, GCObject* pGcObj) -> bool {
			const auto pCtx = static_cast<GCOContext*>(ctx);
			const auto ctxL = pCtx->State;

			if (isdead(ctxL->global, pGcObj))
				return false;

			if (const auto gcObjType = pGcObj->gch.tt;
				gcObjType == LUA_TFUNCTION) {
				ctxL->top->value.gc = pGcObj;
				ctxL->top->tt = gcObjType;
				ctxL->top++;

				lua_getfenv(ctxL, -1);

				if (!lua_isnil(ctxL, -1)) {
					lua_getfield(ctxL, -1, "script");

					if (!lua_isnil(ctxL, -1)) {
						uintptr_t Script = *(uintptr_t*)lua_touserdata(ctxL, -1);

						std::string ClassName = **(std::string**)(*(uintptr_t*)(Script + Offsets::Properties::ClassDescriptor) + Offsets::Properties::ClassName);

						if (pCtx->map.find(Script) == pCtx->map.end() && (ClassName == "LocalScript" || ClassName == "ModuleScript" || ClassName == "Script")) {
							pCtx->map.insert({ Script, true });
							lua_rawseti(ctxL, -4, ++pCtx->itemsFound);
						}
						else {
							lua_pop(ctxL, 1);
						}
					}
					else {
						lua_pop(ctxL, 1);
					}
				}

				lua_pop(ctxL, 2);
			}
			return false;
			});

		LS->global->GCthreshold = ullOldThreshold;

		return 1;
	};
	inline int getloadedmodules(lua_State* LS) {
		lua_pop(LS, lua_gettop(LS));

		getrunningscripts(LS);

		if (!lua_istable(LS, -1)) {
			lua_pop(LS, 1);
			lua_pushnil(LS);
			return 1;
		}

		lua_newtable(LS);
		int resultIndex = lua_gettop(LS);

		int index = 0;

		lua_pushnil(LS);
		while (lua_next(LS, -3) != 0) {
			if (!lua_isnil(LS, -1)) {
				lua_getfield(LS, -1, "ClassName");

				if (lua_isstring(LS, -1)) {
					std::string type = lua_tostring(LS, -1);
					if (type == "ModuleScript") {
						lua_pushinteger(LS, ++index);
						lua_pushvalue(LS, -3);
						lua_settable(LS, resultIndex);
					}
				}

				lua_pop(LS, 1);
			}

			lua_pop(LS, 1);
		}

		lua_remove(LS, -2);

		return 1;
	};
	inline int gettenv(lua_State* LS) {
		luaL_checktype(LS, 1, LUA_TTHREAD);
		lua_State* ls = (lua_State*)lua_topointer(LS, 1);
		LuaTable* tab = hvalue(luaA_toobject(ls, LUA_GLOBALSINDEX));
		#define isdead(g, obj) ((obj)->gch.marked & (g)->currentwhite) == 0
		sethvalue(LS, LS->top, tab);

		LS->top++;

		return 1;
	};
	int getgenv(lua_State* L) {
		lua_State* exec_lua_State = Globals::ExploitThread;
		lua_pushvalue(exec_lua_State, LUA_GLOBALSINDEX);
		lua_xmove(exec_lua_State, L, 1);
		return 1;
	}
	int getreg(lua_State* L) {
		lua_pushvalue(L, LUA_REGISTRYINDEX);

		return 1;
	}
	int getsenv(lua_State* L) {
		luaL_checktype(L, 1, LUA_TUSERDATA);

		uintptr_t script = *(uintptr_t*)lua_touserdata(L, 1);

		if (!Utils::CheckMemory(script) || !Utils::CheckMemory(*(uintptr_t*)(script + Offsets::Properties::ClassDescriptor))) {
			lua_pushnil(L);
			return 1;
		}

		const char* className = *(const char**)(*(uintptr_t*)(script + Offsets::Properties::ClassDescriptor) + Offsets::Properties::ClassName);

		bool decrypt = true;
		std::string bytecode = "";

		// Accept Script, LocalScript, AND ModuleScript (as per documentation)
		if (strcmp(className, "LocalScript") != 0 &&
			strcmp(className, "Script") != 0 &&
			strcmp(className, "ModuleScript") != 0) {
			luaL_argerror(L, 1, "script, localscript, or modulescript expected");
		}

		const auto script_node = *reinterpret_cast<uintptr_t*>(script + Offsets::Thread::weak_thread_node);

		// Check if script_node is valid before proceeding
		if (!script_node || !Utils::CheckMemory(script_node)) {
			lua_pushnil(L);
			return 1;
		}

		const auto node_weak_thread_ref = *reinterpret_cast<uintptr_t*>(script_node + Offsets::Thread::weak_thread_ref);

		// Check if node_weak_thread_ref is valid
		if (!node_weak_thread_ref || !Utils::CheckMemory(node_weak_thread_ref)) {
			lua_pushnil(L);
			return 1;
		}

		const auto live_thread_ref = *reinterpret_cast<uintptr_t*>(node_weak_thread_ref + Offsets::Thread::weak_thread_ref_live);

		// Check if live_thread_ref is valid
		if (!live_thread_ref || !Utils::CheckMemory(live_thread_ref)) {
			lua_pushnil(L);
			return 1;
		}

		const auto script_thread = *reinterpret_cast<lua_State**>(live_thread_ref + Offsets::Thread::weak_thread_ref_live_thread);

		// If script_thread is null or invalid, return nil (script not accessible)
		if (!script_thread || !Utils::CheckMemory(reinterpret_cast<uintptr_t>(script_thread))) {
			lua_pushnil(L);
			return 1;
		}

		lua_pushvalue(script_thread, LUA_GLOBALSINDEX);
		lua_xmove(script_thread, L, 1);

		return 1;
	}
	int getrenv(lua_State* L) {
		luaL_trimstack(L, 0);
		lua_State* RobloxState = L->global->mainthread;
		LuaTable* clone = luaH_clone(L, RobloxState->gt);

		lua_rawcheckstack(L, 1);
		luaC_threadbarrier(L);
		luaC_threadbarrier(RobloxState);

		L->top->value.p = clone;
		L->top->tt = LUA_TTABLE;
		L->top++;

		lua_rawgeti(L, LUA_REGISTRYINDEX, 2);
		lua_setfield(L, -2, "_G");
		lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
		lua_setfield(L, -2, "shared");
		return 1;
	}
	int getscriptbytecode(lua_State* LS) {
		luaL_checktype(LS, 1, LUA_TUSERDATA);

		std::string Bytecode = HelperClass::RequestBytecode(*(uintptr_t*)lua_topointer(LS, 1), true);

		if (Bytecode != "Nil")
		{
			lua_pushlstring(LS, Bytecode.data(), Bytecode.size());
		}
		else
			lua_pushnil(LS);

		return 1;
	}
	int getscriptclosure(lua_State* L) {
		luaL_checktype(L, 1, LUA_TUSERDATA);

		const auto scriptPtr = *(uintptr_t*)lua_topointer(L, 1);
		std::string scriptCode = Roblox::RequestBytecode(scriptPtr);

		lua_State* L2 = lua_newthread(L);
		luaL_sandboxthread(L2);

		uintptr_t Userdata = (uintptr_t)L2->userdata;
		if (!Userdata) {
			Roblox::Print(3, ("Invalid userdata in thread"));
			lua_gc(L, LUA_GCRESTART, 0);
			return 0;
		}

		uintptr_t identityPtr = Userdata + Offsets::ExtraSpace::Identity;
		uintptr_t capabilitiesPtr = Userdata + Offsets::ExtraSpace::Capabilities;

		if (identityPtr && capabilitiesPtr) {
			*(uintptr_t*)identityPtr = 8;
			*(int64_t*)capabilitiesPtr = ~0ULL;
		}
		else {
			Roblox::Print(3, ("Invalid memory addresses for identity or capabilities"));
			lua_gc(L, LUA_GCRESTART, 0);
			return 0;
		}

		lua_pushvalue(L, 1);
		lua_xmove(L, L2, 1);
		lua_setglobal(L2, ("script"));

		int result = Roblox::LuaVM__Load(L2, &scriptCode, (""), 0);

		if (result == LUA_OK) {
			Closure* cl = clvalue(luaA_toobject(L2, -1));

			if (cl) {
				Proto* p = cl->l.p;
				if (p) {
					Taskscheduler->SetProtoCapabilities(p);
				}
			}

			lua_pop(L2, lua_gettop(L2));
			lua_pop(L, lua_gettop(L));

			setclvalue(L, L->top, cl);
			incr_top(L);

			return 1;
		}

		lua_pushnil(L);
		return 1;
	};
	inline int getscripts(lua_State* L) {
		lua_pop(L, lua_gettop(L));

		HelperClass::GetEveryInstance(L);

		if (!lua_istable(L, -1)) {
			lua_pop(L, 1);
			lua_pushnil(L);
			return 1;
		}

		lua_newtable(L);
		int resultIndex = lua_gettop(L);

		int index = 0;

		lua_pushnil(L);
		while (lua_next(L, -3) != 0) {
			if (!lua_isnil(L, -1)) {
				lua_getfield(L, -1, "ClassName");

				if (lua_isstring(L, -1)) {
					std::string type = lua_tostring(L, -1);
					if (type == "LocalScript" || type == "ModuleScript" || type == "Script") {
						lua_pushinteger(L, ++index);
						lua_pushvalue(L, -3);
						lua_settable(L, resultIndex);
					}
				}

				lua_pop(L, 1);
			}

			lua_pop(L, 1);
		}

		lua_remove(L, -2);

		return 1;
	}
	inline int getfunctionhash(lua_State* LS) {
		luaL_checktype(LS, 1, LUA_TFUNCTION);
		Closure* closure = (Closure*)lua_topointer(LS, 1);
		Proto* PR = (Proto*)closure->l.p;

		std::string FuncData;

		FuncData += std::to_string(PR->numparams) + ",";

		FuncData += std::to_string(closure->nupvalues) + ",";

		for (int i = 0; i < PR->sizek; ++i)
		{
			const TValue& k = PR->k[i];

			switch (ttype(&k))
			{
			case LUA_TNIL:
				FuncData += "nil,";
				break;
			case LUA_TBOOLEAN:
				FuncData += (bvalue(&k) ? "true," : "false,");
				break;
			case LUA_TNUMBER:
			{
				FuncData += std::to_string(nvalue(&k)) + ",";
				break;
			}
			case LUA_TSTRING:
				FuncData += getstr(tsvalue(&k));
				FuncData += ",";
				break;
			default:
				FuncData += "unknwnn,";
				break;
			}
		}

		for (int i = 0; i < PR->sizecode; ++i)
		{
			Instruction instr = PR->code[i];
			FuncData += std::to_string((uint32_t)instr) + ",";
		}

		std::string hash = HelperClass::hash_with_algo<CryptoPP::SHA384>(FuncData);

		lua_pushstring(LS, hash.c_str());
		return 1;
	}
	int getscripthash(lua_State* L) {
		int type = lua_type(L, 1);
		if (type != LUA_TUSERDATA && type != LUA_TLIGHTUSERDATA) {
			lua_pushnil(L);
			return 1;
		}

		uintptr_t script = *(uintptr_t*)lua_touserdata(L, 1);

		if (!Utils::CheckMemory(script) || !Utils::CheckMemory(*(uintptr_t*)(script + Offsets::Properties::ClassDescriptor))) {
			lua_pushnil(L);
			return 1;
		}

		const char* className = *(const char**)(*(uintptr_t*)(script + Offsets::Properties::ClassDescriptor) + Offsets::Properties::ClassName);

		if (strcmp(className, ("LocalScript")) == 0) {
			uintptr_t bytecodePointer = *(uintptr_t*)(script + Offsets::Scripts::LocalScriptEmbedded);
			std::string bytecode = *(std::string*)(bytecodePointer + 0x10);

			std::string hashed = HelperClass::sha384(bytecode);

			lua_pushlstring(L, hashed.data(), hashed.size());
			return 1;
		}
		else if (strcmp(className, ("ModuleScript")) == 0) {
			uintptr_t bytecodePointer = *(uintptr_t*)(script + Offsets::Scripts::ModuleScriptEmbedded);
			std::string bytecode = *(std::string*)(bytecodePointer + 0x10);

			std::string hashed = HelperClass::sha384(bytecode);

			lua_pushlstring(L, hashed.data(), hashed.size());
			return 1;
		}
		else if (strcmp(className, ("Script")) == 0) {
			lua_pushstring(L, ("Scripts do not have bytecode"));
			return 1;
		}
		else {
			lua_pushstring(L, ("Invalid Script"));
			return 1;
		}
	}

	int getthreadidentity(lua_State* L) {
		uintptr_t userData = (uintptr_t)L->userdata;
		lua_pushnumber(L, *reinterpret_cast<__int64*>(userData + Offsets::ExtraSpace::Identity));

		return 1;
	}
	int setthreadidentity(lua_State* L) {
		Taskscheduler->SetThreadIdentity(L, lua_tointeger(L, 1), MAXCAPABILITIES);
		return 1;
	}

	int messagebox(lua_State* l) {
		std::string content = lua_tostring(l, 1);
		std::string title = lua_tostring(l, 2);

		std::thread([content, title] {
			MessageBoxA(nullptr, content.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
			}).detach();

		return 0;
	}
	inline int loadstring(lua_State* L) {
		luaL_checktype(L, 1, LUA_TSTRING);

		const std::string source = lua_tostring(L, 1);
		const std::string chunkname = luaL_optstring(L, 2, "=");

		std::string script = Execution->CompileScript(source);

		if (script[0] == '\0' || script.empty()) {
			lua_pushnil(L);
			lua_pushstring(L, "Failed to compile script");
			return 2;
		}

		int result = Roblox::LuaVM__Load(L, &script, chunkname.data(), 0);
		if (result != LUA_OK) {
			std::string Error = luaL_checklstring(L, -1, nullptr);
			lua_pop(L, 1);

			lua_pushnil(L);
			lua_pushstring(L, Error.data());

			return 2;
		}

		Closure* closure = clvalue(luaA_toobject(L, -1));

		Taskscheduler->SetProtoCapabilities(closure->l.p);

		return 1;
	}
	inline int getgc(lua_State* L) {
		luaL_checkstack(L, 3, "getgc");
		const bool includeTables = luaL_optboolean(L, 1, false);

		// Create the result table
		lua_newtable(L);

		typedef struct {
			lua_State* luaThread;
			bool includeTables;
			int itemsFound;
		} GCOContext;

		auto GCContext = GCOContext{ L, includeTables, 1 };

		const auto oldGCThreshold = L->global->GCthreshold;
		L->global->GCthreshold = SIZE_MAX;

		luaM_visitgco(L, &GCContext, [](void* ctx, lua_Page* page, GCObject* gcObj) -> bool {
			const auto context = static_cast<GCOContext*>(ctx);
			const auto luaThread = context->luaThread;

			// Skip dead objects
			if (isdead(luaThread->global, gcObj))
				return false;

			const auto gcObjectType = gcObj->gch.tt;

			bool shouldInclude = false;

			// Include all function types - this is what sUNC is looking for
			if (gcObjectType == LUA_TFUNCTION) {
				shouldInclude = true;
			}
			else if (gcObjectType == LUA_TTHREAD || gcObjectType == LUA_TUSERDATA ||
				gcObjectType == LUA_TLIGHTUSERDATA || gcObjectType == LUA_TBUFFER) {
				shouldInclude = true;
			}
			// Include tables only if requested
			else if (gcObjectType == LUA_TTABLE && context->includeTables) {
				shouldInclude = true;
			}

			if (shouldInclude) {
				// Ensure we have stack space
				if (luaThread->top >= luaThread->stack_last) {
					return false; // Stack overflow protection
				}

				// Push the object onto the stack properly
				luaThread->top->value.gc = gcObj;
				luaThread->top->tt = gcObjectType;
				incr_top(luaThread);

				// Store in the result table using 1-based indexing
				lua_rawseti(luaThread, -2, context->itemsFound++);
			}

			return false; // Continue traversal
			});

		L->global->GCthreshold = oldGCThreshold;

		return 1;
	}
}
