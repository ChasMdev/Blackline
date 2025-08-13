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
#include <Execution/Execution.hpp>
#include <memory>
#include <iostream>
#include <fstream>
#include <set>
#include <algorithm>

struct LuauUserdata {
	char pad_0000[48]; //0x0000
	uint64_t Identity; //0x0030
	char pad_0038[16]; //0x0038
	uint64_t Capabilities; //0x0048
	std::weak_ptr<uintptr_t> Script; //0x0050
};

namespace HelperClassClosure {
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

	std::string b64encode(const std::string& stringToEncode) {
		std::string base64EncodedString;
		CryptoPP::Base64Encoder encoder{ new CryptoPP::StringSink(base64EncodedString), false };
		encoder.Put((byte*)stringToEncode.c_str(), stringToEncode.length());
		encoder.MessageEnd();

		return base64EncodedString;
	}

	std::string b64decode(const std::string& stringToDecode) {
		std::string base64DecodedString;
		CryptoPP::Base64Decoder decoder{ new CryptoPP::StringSink(base64DecodedString) };
		decoder.Put((byte*)stringToDecode.c_str(), stringToDecode.length());
		decoder.MessageEnd();

		return base64DecodedString;
	}

	std::string RamdonString(int len) {
		static const char* chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
		std::string str;
		str.reserve(len);

		for (int i = 0; i < len; ++i) {
			str += chars[rand() % (strlen(chars) - 1)];
		}

		return str;
	}
	inline bool IsOurThread(lua_State* L) {
		if (L == nullptr)
			return false;

		auto threadUserdata = reinterpret_cast<LuauUserdata*>(L->userdata);

		return threadUserdata != nullptr &&
			threadUserdata->Script.lock() == nullptr &&
			(threadUserdata->Capabilities & ourCapability) == ourCapability;
	}

	std::string CompressBytecode(const std::string& bytecode) {
		const auto data_size = bytecode.size();
		const auto max_size = ZSTD_compressBound(data_size);
		auto buffer = std::vector<char>(max_size + 8);

		strcpy_s(&buffer[0], buffer.capacity(), ("RSB1"));
		memcpy_s(&buffer[4], buffer.capacity(), &data_size, sizeof(data_size));

		const auto compressed_size = ZSTD_compress(&buffer[8], max_size, bytecode.data(), data_size, ZSTD_maxCLevel());
		if (ZSTD_isError(compressed_size))
			throw std::runtime_error(("Failed to compress the bytecode."));

		const auto size = compressed_size + 8;
		const auto key = XXH32(buffer.data(), size, 42u);
		const auto bytes = reinterpret_cast<const uint8_t*>(&key);

		for (auto i = 0u; i < size; ++i)
			buffer[i] ^= bytes[i % 4] + i * 41u;

		return std::string(buffer.data(), size);
	}
}

namespace Closures {
	inline auto FindSavedCClosure(Closure* closure) {
		std::ofstream logFile("hookfunction_debug.txt", std::ios::app);

		std::string msg1 = "FindSavedCClosure: looking for closure=" + std::to_string((uintptr_t)closure);
		Roblox::Print(1, msg1.c_str());
		logFile << msg1 << std::endl;

		// Check Newcclosures
		const auto it = Environment::Newcclosures.find(closure);
		if (it != Environment::Newcclosures.end()) {
			std::string msg2 = "FindSavedCClosure: found in Newcclosures, returning " + std::to_string((uintptr_t)it->second);
			Roblox::Print(1, msg2.c_str());
			logFile << msg2 << std::endl;
			logFile.close();
			return it->second;
		}

		// Check Newlclosures  
		const auto lit = Environment::Newlclosures.find(closure);
		if (lit != Environment::Newlclosures.end()) {
			std::string msg2 = "FindSavedCClosure: found in Newlclosures, returning " + std::to_string((uintptr_t)lit->second);
			Roblox::Print(1, msg2.c_str());
			logFile << msg2 << std::endl;
			logFile.close();
			return lit->second;
		}

		for (const auto& pair : Environment::HookedFunctions) {
			if (pair.second == closure) {
			    std::string msg2 = "FindSavedCClosure: found as original in HookedFunctions, returning " + std::to_string((uintptr_t)closure);
			    Roblox::Print(1, msg2.c_str());
			    logFile << msg2 << std::endl;
			    logFile.close();
			    return closure;
			}
		}

		std::string msg3 = "FindSavedCClosure: not found, returning nullptr";
		Roblox::Print(1, msg3.c_str());
		logFile << msg3 << std::endl;
		logFile.close();
		return (Closure*)nullptr;
	}

	inline void SplitString(std::string Str, std::string By, std::vector<std::string>& Tokens)
	{
		Tokens.push_back(Str);
		const auto splitLen = By.size();
		while (true)
		{
			auto frag = Tokens.back();
			const auto splitAt = frag.find(By);
			if (splitAt == std::string::npos)
				break;
			Tokens.back() = frag.substr(0, splitAt);
			Tokens.push_back(frag.substr(splitAt + splitLen, frag.size() - (splitAt + splitLen)));
		}
	};

	inline  std::string ErrorMessage(const std::string& message) {
		static auto callstack_regex = std::regex((R"(.*"\]:(\d)*: )"), std::regex::optimize | std::regex::icase);
		if (std::regex_search(message.begin(), message.end(), callstack_regex)) {
			const auto fixed = std::regex_replace(message, callstack_regex, "");
			return fixed;
		}

		return message;
	};

	inline static void handler_run(lua_State* L, void* ud) {
		luaD_call(L, (StkId)(ud), LUA_MULTRET);
	};

	inline int NewCClosureContinuation(lua_State* L, std::int32_t status) {
		if (status != LUA_OK) {
			std::string regexed_error = lua_tostring(L, -1);
			lua_pop(L, 1);

			lua_pushlstring(L, regexed_error.c_str(), regexed_error.size());
			lua_error(L);
		}

		return lua_gettop(L);
	};

	inline int NewCClosureStub(lua_State* L) {
		const auto nArgs = lua_gettop(L);
		L->ci->flags |= LUA_CALLINFO_HANDLE;

		const auto closure = Environment::Newcclosures.find(clvalue(L->ci->func));

		if (closure == Environment::Newcclosures.end())
		{
			Roblox::Print(3, "NewCClosureStub: Invalid closure");
			luaL_error(L, "Invalid closure");
		}

		setclvalue(L, L->top, closure->second);
		L->top++;

		lua_insert(L, 1);

		StkId func = L->base;
		L->ci->flags |= LUA_CALLINFO_HANDLE;

		L->baseCcalls++;
		int status = luaD_pcall(L, handler_run, func, savestack(L, func), 0);
		L->baseCcalls--;

		if (status == LUA_ERRRUN) {
			std::size_t error_len;
			const char* errmsg = luaL_checklstring(L, -1, &error_len);
			lua_pop(L, 1);
			std::string error(errmsg);

			if (error == std::string(("attempt to yield across metamethod/C-call boundary")))
				return lua_yield(L, 0);

			std::string fixedError = ErrorMessage(error);
			std::regex pattern(R"([^:]+:\d+:\s?)");

			std::smatch match;
			if (std::regex_search(fixedError, match, pattern)) {
				fixedError.erase(match.position(), match.length());
			}

			lua_pushlstring(L, fixedError.data(), fixedError.size());
			lua_error(L);
			return 0;
		}

		expandstacklimit(L, L->top);

		if (status == 0 && (L->status == LUA_YIELD || L->status == LUA_BREAK))
			return -1;

		return lua_gettop(L);
	};

	inline int NonYieldNewCClosureStub(lua_State* L) {
		const auto nArgs = lua_gettop(L);

		Closure* cl = clvalue(L->ci->func);
		if (!cl) {
			Roblox::Print(3, "NonYieldNewCClosureStub: Invalid closure");
			luaL_error(L, ("Invalid closure"));
		}

		Closure* originalClosure = FindSavedCClosure(cl);

		if (originalClosure)
			Roblox::Print(1, !originalClosure->isC ? "Lua closure" : "C closure");

		if (!originalClosure) {
			Roblox::Print(3, "NonYieldNewCClosureStub: Invalid closure - not found in saved closures");
			luaL_error(L, ("Invalid closure"));
			return 0;
		}

		setclvalue(L, L->top, originalClosure);
		L->top++;

		lua_insert(L, 1);

		StkId func = L->base;
		L->ci->flags |= LUA_CALLINFO_HANDLE;

		L->baseCcalls++;
		int status = luaD_pcall(L, handler_run, func, savestack(L, func), 0);
		L->baseCcalls--;

		if (status == LUA_ERRRUN) {
			std::size_t error_len;
			const char* errmsg = luaL_checklstring(L, -1, &error_len);
			lua_pop(L, 1);
			std::string error(errmsg);

			Roblox::Print(3, ("NonYieldNewCClosureStub ERROR: " + error).c_str());

			if (error == std::string(("attempt to yield across metamethod/C-call boundary")))
				return lua_yield(L, 0);

			std::string fixedError = ErrorMessage(error);
			std::regex pattern(R"([^:]+:\d+:\s?)");

			std::smatch match;
			if (std::regex_search(fixedError, match, pattern)) {
				fixedError.erase(match.position(), match.length());
			}

			Roblox::Print(3, ("NonYieldNewCClosureStub FIXED ERROR: " + fixedError).c_str());

			lua_pushlstring(L, fixedError.data(), fixedError.size());
			lua_error(L);
			return 0;
		}

		expandstacklimit(L, L->top);

		if (status == 0 && (L->status == LUA_YIELD || L->status == LUA_BREAK))
			return -1;

		return lua_gettop(L);
	};

	enum type
	{
		None,
		RobloxClosure,
		LuauClosure,
		ExecutorFunction,
		NewCClosure,
	};

	inline static type GetClosureType(Closure* closure)
	{
		auto cl_type = None;

		if (!closure->isC)
		{
			cl_type = LuauClosure;
		}
		else
		{
			if (closure->c.f.get() == Environment::ClosuresHandler)
			{
				if (Environment::GetClosure(closure) == NewCClosureStub || Environment::GetClosure(closure) == NonYieldNewCClosureStub)
					cl_type = NewCClosure;
				else
					cl_type = ExecutorFunction;
			}
			else
				cl_type = RobloxClosure;
		}

		return cl_type;
	};

	inline void WrapClosure(lua_State* L, int idx) {
		Closure* OldClosure = clvalue(luaA_toobject(L, idx));
		if (GetClosureType(OldClosure) == NewCClosure)
		{
			lua_pushvalue(L, idx);
			return;
		}

		lua_ref(L, idx);
		Environment::PushWrappedClosure(L, NewCClosureStub, 0, 0, NewCClosureContinuation);

		Closure* HookClosure = clvalue(luaA_toobject(L, -1));

		HookClosure->isC = 1;
		HookClosure->env = OldClosure->env;

		lua_ref(L, -1);

		Environment::Newcclosures[HookClosure] = OldClosure;
	};

	inline int newcclosure(lua_State* LS)
	{
		luaL_checktype(LS, 1, LUA_TFUNCTION);

		auto Closure = clvalue(luaA_toobject(LS, 1));
		if (!Closure)
		{
			luaL_error(LS, "Invalid closure provided in newcclosure");
		};

		if (Closure->isC)
		{
			lua_pushvalue(LS, 1);
			return 1;
		}

		WrapClosure(LS, 1);

		return 1;
	};
	inline int checkcaller(lua_State* L) {
		lua_pushboolean(L, HelperClassClosure::IsOurThread(L));
		return 1;
	}
	inline int iscclosure(lua_State* LS)
	{
		luaL_checktype(LS, 1, LUA_TFUNCTION);

		auto Closure = clvalue(luaA_toobject(LS, 1));
		if (!Closure)
		{
			luaL_error(LS, "Invalid closure provided in iscclosure");
		};

		lua_pushboolean(LS, Closure->isC);

		return 1;
	};
	inline int islclosure(lua_State* LS)
	{
		luaL_checktype(LS, 1, LUA_TFUNCTION);

		auto Closure = clvalue(luaA_toobject(LS, 1));
		if (!Closure)
		{
			luaL_error(LS, "Invalid closure provided in islclosure");
		};

		lua_pushboolean(LS, !Closure->isC);

		return 1;
	};
	inline int clonefunction(lua_State* LS)
	{
		luaL_checktype(LS, 1, LUA_TFUNCTION);

		auto Closure = clvalue(luaA_toobject(LS, 1));
		if (!Closure)
		{
			luaL_error(LS, "Invalid closure provided in clonefunction");
		};

		luaL_checktype(LS, 1, LUA_TFUNCTION);

		switch (GetClosureType(clvalue(luaA_toobject(LS, 1))))
		{
		case ExecutorFunction:
			lua_clonecfunction(LS, 1);
			Environment::SetClosure(clvalue(luaA_toobject(LS, -1)), Environment::GetClosure(clvalue(luaA_toobject(LS, 1))));
			break;
		case RobloxClosure:
			lua_clonecfunction(LS, 1);
			break;
		case LuauClosure:
			lua_clonefunction(LS, 1);
			break;
		case None:
			luaL_argerror(LS, 1, "Type of closure unsupported");
			return 0;
		};

		return 1;
	};
	inline int isexecutorclosure(lua_State* LS) {
		if (lua_type(LS, 1) != LUA_TFUNCTION) { lua_pushboolean(LS, false); return 1; }

		auto Closure = clvalue(luaA_toobject(LS, 1));
		if (!Closure)
		{
			luaL_error(LS, "Invalid closure provided in isexecutorclosure");
		};

		bool value = false;

		if (lua_isLfunction(LS, 1))
		{
			value = Taskscheduler->CheckProto(Closure->l.p);
		}
		else
		{
			value = Environment::ExecutorFunctions.contains(Closure);
		}

		lua_pushboolean(LS, value);
		return 1;
	}
	inline int getfunctionhash(lua_State* LS)
	{
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

		std::string hash = HelperClassClosure::hash_with_algo<CryptoPP::SHA384>(FuncData);

		lua_pushstring(LS, hash.c_str());
		return 1;
	};
	inline int newlclosure(lua_State* L) {
		luaL_checktype(L, 1, LUA_TFUNCTION);

		lua_newtable(L);
		lua_newtable(L);

		lua_pushvalue(L, LUA_GLOBALSINDEX);
		lua_setfield(L, -2, "__index");
		lua_setreadonly(L, -1, true);
		lua_setmetatable(L, -2);

		lua_pushvalue(L, 1);
		lua_setfield(L, -2, "NEW_L_CLOSURE");
		const auto code = "return NEW_L_CLOSURE(...)";
		auto compiledBytecode = Execution->CompileScript(code);
		luau_load(L, std::format("{}_newlclosurewrapper", "=").c_str(), compiledBytecode.c_str(),
			compiledBytecode.length(), -1);
		lua_getfenv(L, 1);
		lua_setfenv(L, -3);

		const auto loadedClosure = lua_toclosure(L, -1);
		Taskscheduler->ElevateProto(loadedClosure);

		return 1;
	}

	int comparefunctions(lua_State* LS) {
		luaL_checktype(LS, 1, LUA_TFUNCTION);
		luaL_checktype(LS, 2, LUA_TFUNCTION);

		const auto FirstClosure = lua_toclosure(LS, 1);
		const auto SecondClosure = lua_toclosure(LS, 2);

		if (FirstClosure->isC != SecondClosure->isC) {
			lua_pushboolean(LS, false);
			return 1;
		}

		if (FirstClosure->isC) {
			lua_pushboolean(LS, FirstClosure->c.f == SecondClosure->c.f);
		}
		else {
			lua_pushboolean(LS, FirstClosure->l.p == SecondClosure->l.p);
		}

		return 1;
	}

	inline int hookfunction(lua_State* rl)
	{
		if (!lua_isfunction(rl, 1) || !lua_isfunction(rl, 2)) {
			luaL_error(rl, "Both arguments must be functions");
		}

		lua_ref(rl, 1);
		lua_ref(rl, 2);

		const auto fn1 = clvalue(index2addr(rl, 1));
		const auto fn2 = clvalue(index2addr(rl, 2));

		if (!fn1 || !fn2) {
			luaL_error(rl, "Failed to extract closures from arguments");
		}

		const auto fn1_nups = fn1->nupvalues;
		const auto fn2_nups = fn2->nupvalues;
		const auto fn1_t = GetClosureType(fn1);
		const auto fn2_t = GetClosureType(fn2);

		if (Environment::HookedFunctions.find(fn1) == Environment::HookedFunctions.end()) {
			lua_clonefunction(rl, 1);
			Environment::HookedFunctions[fn1] = lua_toclosure(rl, -1);
			lua_pop(rl, 1);
		}

		//Roblox::Print(1, ("Hooking: " + std::to_string(fn1_t) + " -> " + std::to_string(fn2_t)).c_str());
		lua_pushvalue(rl, 1);

		// LuauClosure -> LuauClosure
		if (fn1_t == LuauClosure && fn2_t == LuauClosure) {
			lua_clonefunction(rl, 1);
			lua_ref(rl, -1);

			LuaTable* originalEnv = fn1->env;
			fn1->l.p = fn2->l.p;
			fn1->stacksize = fn2->stacksize;
			fn1->env = originalEnv ? originalEnv : fn2->env;

			for (int i = 0; i < fn1_nups; i++)
				setobj2n(rl, &fn1->l.uprefs[i], luaO_nilobject);
			for (int i = 0; i < fn2_nups; i++)
				setobj2n(rl, &fn1->l.uprefs[i], &fn2->l.uprefs[i]);

			return 1;
		}

		// LuauClosure -> Any C-type
		if (fn1_t == LuauClosure && fn2_t != LuauClosure) {
			if (fn2->isC || fn2->nupvalues > fn1->nupvalues) {
				lua_getglobal(rl, "newlclosure");
				lua_pushvalue(rl, 2);
				lua_call(rl, 1, 1);
			}
			else {
				lua_pushvalue(rl, 2);
			}
			lua_ref(rl, -1);

			const auto finalHookWith = lua_toclosure(rl, -1);
			fn1->env = finalHookWith->env;
			fn1->stacksize = finalHookWith->stacksize;
			fn1->preload = finalHookWith->preload;
			fn1->nupvalues = finalHookWith->nupvalues;
			fn1->l.p = finalHookWith->l.p;

			for (int i = 0; i < finalHookWith->nupvalues; i++)
				setobj2n(rl, &fn1->l.uprefs[i], &finalHookWith->l.uprefs[i]);

			lua_pushrawclosure(rl, Environment::HookedFunctions.find(fn1)->second);
			lua_ref(rl, -2);
			return 1;
		}

		// All remaining cases are C-type -> C-type, so clone once
		if (lua_iscfunction(rl, 1)) {
			lua_clonecfunction(rl, 1);
		}
		else {
			lua_clonefunction(rl, 1);
		}
		lua_ref(rl, -1);

		// NewCClosure -> NewCClosure (special case)
		if (fn1_t == NewCClosure && fn2_t == NewCClosure) {
			const auto it = Environment::Newcclosures.find(fn2);
			if (!it->second)
				luaL_error(rl, "Hookfunction: newcclosure has no binded lclosure");

			Environment::Newcclosures[fn1] = it->second;
			fn1->c.cont = fn2->c.cont;
			fn1->env = fn2->env;
			fn1->nupvalues = fn2_nups;
			return 1;
		}

		// Cases that need NewCClosureStub: Any C-type -> LuauClosure OR RobloxClosure/ExecutorFunction -> NewCClosure
		if ((fn1_t != LuauClosure && fn2_t == LuauClosure) ||
			((fn1_t == RobloxClosure || fn1_t == ExecutorFunction) && fn2_t == NewCClosure)) {

			fn1->c.f = NewCClosureStub;

			if (fn2_t == NewCClosure) {
				const auto it = Environment::Newcclosures.find(fn2);
				if (!it->second)
					luaL_error(rl, "Hookfunction: newcclosure has no binded lclosure");
				Environment::Newcclosures[fn1] = it->second;
			}
			else {
				Environment::Newcclosures[fn1] = fn2;
			}
			return 1;
		}

		// Direct C-function copying: Any C-type -> RobloxClosure OR RobloxClosure -> ExecutorFunction
		if ((fn1_t != LuauClosure && fn2_t == RobloxClosure) ||
			(fn1_t == RobloxClosure && fn2_t == ExecutorFunction)) {

			// Check upvalue count for Any C-type -> RobloxClosure
			if (fn1_t != LuauClosure && fn2_t == RobloxClosure && fn1_nups < fn2_nups) {
				luaL_error(rl, "Destination has an upvalue count of %i source has an upvalue count of %i", fn1_nups, fn2_nups);
			}

			fn1->c.f = fn2->c.f;
			fn1->c.cont = fn2->c.cont;
			fn1->env = fn2->env;

			for (int i = 0; i < fn1_nups; i++)
				setobj2n(rl, &fn1->c.upvals[i], luaO_nilobject);
			for (int i = 0; i < fn2_nups && i < fn1_nups; i++)
				setobj2n(rl, &fn1->c.upvals[i], &fn2->c.upvals[i]);

			return 1;
		}

		luaL_error(rl, "Invalid hookfunction arguments: destination %i source %i", fn1_t, fn2_t);
	}

	inline int restorefunction(lua_State* L) {
		luaL_checktype(L, 1, LUA_TFUNCTION);
		auto current = lua_toclosure(L, 1);
		auto it = Environment::HookedFunctions.find(current);

		if (it == Environment::HookedFunctions.end()) {
			luaL_error(L, "Function was not previously hooked");
			return 0;
		}

		auto original = it->second;

		if (current->isC) {
			current->c.f = original->c.f;
			current->c.cont = original->c.cont;
			current->nupvalues = original->nupvalues;

			for (int i = 0; i < original->nupvalues; i++) {
				current->c.upvals[i] = original->c.upvals[i];
			}

			auto closureOpt = Environment::Newcclosures.find(current);
			if (closureOpt != Environment::Newcclosures.end()) {
				Environment::Newcclosures.erase(current);
			}
			auto lclosureOpt = Environment::Newlclosures.find(current);
			if (lclosureOpt != Environment::Newlclosures.end()) {
				Environment::Newlclosures.erase(current);
			}
		}
		else {
			current->l.p = original->l.p;
			current->env = original->env;
			current->stacksize = original->stacksize;
			current->preload = original->preload;
			current->nupvalues = original->nupvalues;

			for (int i = 0; i < original->nupvalues; i++) {
				setobj2n(L, &current->l.uprefs[i], &original->l.uprefs[i]);
			}
		}

		Environment::HookedFunctions.erase(it);
		lua_pushboolean(L, 1);
		return 1;
	}
}