#include "Environment.hpp"
#include "Console/Console.h"
#include "Scripts/Scripts.h"
#include "Closure/Closure.h"
#include "Metatable/metatable.h"
#include "Misc/Misc.h"
#include "gamehooks.hpp"
#include "Cache/Cache.h"
#include "Crypt/Crypt.h"
#include "Debug/Debug.h"
#include "Filesystem/Filesystem.h"
#include "Instances/Instances.h"
#include "Reflection/Reflection.h"
#include "Signals/Signals.h"

std::unordered_map<Closure*, Closure*> Environment::Newcclosures = {};
std::unordered_map<Closure*, Closure*> Environment::Newlclosures = {};
std::unordered_map<Closure*, Closure*> Environment::HookedFunctions = {};
std::map<Closure*, lua_CFunction> Environment::ExecutorClosures = {};
std::unordered_set<Closure*> Environment::ExecutorFunctions = {};
#define lua_normalisestack(L, maxSize) { if (lua_gettop(L) > maxSize) lua_settop(L, maxSize); }
#define lua_preparepush(L, pushCount) lua_rawcheckstack(L, pushCount)
#define lua_preparepushcollectable(L, pushCount) { lua_preparepush(L, pushCount); luaC_threadbarrier(L); }

// Register functions:
#pragma region RegisterFunctions
int Environment::ClosuresHandler(lua_State* L)
{
	auto found = ExecutorClosures.find(curr_func(L));
	if (found != ExecutorClosures.end())
	{
		return found->second(L);
	}
	return 0;
}

lua_CFunction Environment::GetClosure(Closure* Closure)
{
	return ExecutorClosures[Closure];
}

void Environment::SetClosure(Closure* Closure, lua_CFunction Function)
{
	ExecutorClosures[Closure] = Function;
}
void Environment::PushClosure(lua_State* L, lua_CFunction Function, const char* debugname, int nup)
{
	lua_pushcclosurek(L, ClosuresHandler, debugname, nup, 0);
	Closure* closure = *reinterpret_cast<Closure**>(index2addr(L, -1));
	ExecutorClosures[closure] = Function;
}
void Environment::PushWrappedClosure(lua_State* L, lua_CFunction Function, const char* debugname, int nup, lua_Continuation count)
{
	lua_pushcclosurek(L, ClosuresHandler, debugname, nup, count);
	Closure* closure = *reinterpret_cast<Closure**>(index2addr(L, -1));
	ExecutorClosures[closure] = Function;
	ExecutorFunctions.insert(closure);
	lua_ref(L, -1);
}
void Environment::AddFunction(lua_State* L, const char* globalname, lua_CFunction function)
{
	PushClosure(L, function, nullptr, 0);
	ExecutorFunctions.insert(*reinterpret_cast<Closure**>(index2addr(L, -1)));
	lua_setfield(L, LUA_GLOBALSINDEX, globalname);
}
void Environment::NewTableFunction(lua_State* L, const char* globalname, lua_CFunction function)
{
	PushClosure(L, function, nullptr, 0);
	ExecutorFunctions.insert(*reinterpret_cast<Closure**>(index2addr(L, -1)));
	lua_setfield(L, -2, globalname);
}
#pragma endregion



void Environment::Init(lua_State* L) {

	// Console Library:
	AddFunction(L, ("consolecreate"), ConsoleLib::rconsolecreate);
	AddFunction(L, ("rconsolecreate"), ConsoleLib::rconsolecreate);
	AddFunction(L, ("consoleprint"), ConsoleLib::rconsoleprint);
	AddFunction(L, ("rconsoleprint"), ConsoleLib::rconsoleprint);
	AddFunction(L, ("consoleclear"), ConsoleLib::rconsoleclear);
	AddFunction(L, ("rconsoleclear"), ConsoleLib::rconsoleclear);
	AddFunction(L, ("consolesettitle"), ConsoleLib::rconsolesettitle);
	AddFunction(L, ("rconsolesettitle"), ConsoleLib::rconsolesettitle);
	AddFunction(L, ("consolename"), ConsoleLib::rconsolesettitle);
	AddFunction(L, ("rconsolename"), ConsoleLib::rconsolesettitle);
	AddFunction(L, ("consoledestroy"), ConsoleLib::rconsoledestroy);
	AddFunction(L, ("rconsoledestroy"), ConsoleLib::rconsoledestroy);
	AddFunction(L, ("rconsoleinput"), ConsoleLib::rconsoleinput);
	AddFunction(L, ("consoleinput"), ConsoleLib::rconsoleinput);
	AddFunction(L, ("rconsoletopmost"), ConsoleLib::rconsoletopmost);
	AddFunction(L, ("consoletopmost"), ConsoleLib::rconsoletopmost);
	AddFunction(L, ("rconsoleinfo"), ConsoleLib::rconsoleinfo);
	AddFunction(L, ("consoleinfo"), ConsoleLib::rconsoleinfo);
	AddFunction(L, ("rconsolewarn"), ConsoleLib::rconsolewarn);
	AddFunction(L, ("consolewarn"), ConsoleLib::rconsolewarn);
	AddFunction(L, ("rconsoleerr"), ConsoleLib::rconsoleerr);
	AddFunction(L, ("consoleerr"), ConsoleLib::rconsoleerr);
	AddFunction(L, ("rconsolehidden"), ConsoleLib::rconsolehidden);
	AddFunction(L, ("consoleerr"), ConsoleLib::rconsolehidden);
	AddFunction(L, ("rconsolehide"), ConsoleLib::rconsolehide);
	AddFunction(L, ("consolehide"), ConsoleLib::rconsolehide);
	AddFunction(L, ("rconsoleshow"), ConsoleLib::rconsoleshow);
	AddFunction(L, ("consoleshow"), ConsoleLib::rconsoleshow);
	AddFunction(L, ("rconsoletoggle"), ConsoleLib::rconsoletoggle);
	AddFunction(L, ("consoletoggle"), ConsoleLib::rconsoletoggle);

	// Script Library:
	AddFunction(L, ("getcallingscript"), Scripts::getcallingscript);
	AddFunction(L, ("getrunningscripts"), Scripts::getrunningscripts);
	AddFunction(L, ("getloadedmodules"), Scripts::getloadedmodules);
	AddFunction(L, ("getgenv"), Scripts::getgenv);
	AddFunction(L, ("gettenv"), Scripts::gettenv);
	AddFunction(L, ("getreg"), Scripts::getreg);
	AddFunction(L, ("getsenv"), Scripts::getsenv);
	AddFunction(L, ("getrenv"), Scripts::getrenv);
	AddFunction(L, ("getscriptbytecode"), Scripts::getscriptbytecode);
	AddFunction(L, ("getscriptclosure"), Scripts::getscriptclosure);
	AddFunction(L, ("getscripts"), Scripts::getscripts);
	AddFunction(L, ("getfunctionhash"), Scripts::getfunctionhash);
	AddFunction(L, ("getscripthash"), Scripts::getscripthash);
	AddFunction(L, ("getthreadidentity"), Scripts::getthreadidentity);
	AddFunction(L, ("setthreadidentity"), Scripts::setthreadidentity);
	AddFunction(L, ("messagebox"), Scripts::messagebox);
	AddFunction(L, ("loadstring"), Scripts::loadstring);
	AddFunction(L, "getgc", Scripts::getgc);

	// Reflection Library
	AddFunction(L, ("isscriptable"), Reflection::isscriptable);
	//AddFunction(L, "setscriptable", Reflection::setscriptable);
	//AddFunction(L, "gethiddenproperty", Reflection::gethiddenproperty); // crashes due to RBX::shared_string_t being incorrect?
	AddFunction(L, "sethiddenproperty", Reflection::sethiddenproperty);

	// Closure Library
	AddFunction(L, ("newcclosure"), Closures::newcclosure);
	AddFunction(L, ("checkcaller"), Closures::checkcaller);
	AddFunction(L, ("iscclosure"), Closures::iscclosure);
	AddFunction(L, ("islclosure"), Closures::islclosure);
	AddFunction(L, ("clonefunction"), Closures::clonefunction);
	AddFunction(L, ("isexecutorclosure"), Closures::isexecutorclosure);
	AddFunction(L, ("newlclosure"), Closures::newlclosure);
	AddFunction(L, ("getfunctionhash"), Closures::getfunctionhash);
	AddFunction(L, ("comparefunctions"), Closures::comparefunctions);
	AddFunction(L, ("hookfunction"), Closures::hookfunction);
	//AddFunction(L, "restorefunction", Closures::restorefunction);

	// Http Library
	AddFunction(L, ("httpget"), Http::httpget);
	AddFunction(L, ("HttpGet"), Http::httpget);
	AddFunction(L, ("HttpGetAsync"), Http::httpget);
	lua_newtable(L);
	NewTableFunction(L, ("request"), Http::request);
	lua_setfield(L, LUA_GLOBALSINDEX, ("http"));
	AddFunction(L, ("request"), Http::request);
	AddFunction(L, ("http_request"), Http::request);
	AddFunction(L, ("decompile"), Http::decompile);
	AddFunction(L, ("Decompile"), Http::decompile);
	AddFunction(L, ("gethwid"), Http::gethwid);
	AddFunction(L, ("GetHwid"), Http::gethwid);
	AddFunction(L, ("Gethwid"), Http::gethwid);
	AddFunction(L, ("get_hwid"), Http::gethwid);
	AddFunction(L, ("gethardwareid"), Http::gethwid);
	AddFunction(L, ("GetHardwareId"), Http::gethwid);
	AddFunction(L, ("GETHWID"), Http::gethwid);

	// Metatable Library
	AddFunction(L, ("getrawmetatable"), Metatable::getrawmetatable);
	AddFunction(L, ("setrawmetatable"), Metatable::setrawmetatable);
	AddFunction(L, ("isreadonly"), Metatable::isreadonly);
	AddFunction(L, ("setreadonly"), Metatable::setreadonly);
	AddFunction(L, ("getnamecallmethod"), Metatable::getnamecallmethod);
	AddFunction(L, ("hookmetamethod"), Metatable::hookmetamethod);

	// Miscellaneous Functions
	AddFunction(L, ("setfpscap"), Misc::setfpscap);
	AddFunction(L, ("getfpscap"), Misc::getfpscap);
	AddFunction(L, ("identifyexecutor"), Misc::identifyexecutor);
	AddFunction(L, ("setclipboard"), Misc::setclipboard);
	AddFunction(L, ("toclipboard"), Misc::setclipboard);
	//AddFunction(L, ("queueonteleport"), Misc::queueonteleport); // --> Injection has to handle this!

	// Crypt Library
	AddFunction(L, "lz4compress", Crypt::lz4compress);
	AddFunction(L, "lz4decompress", Crypt::lz4decompress);
	AddFunction(L, "base64_encode", Crypt::base64_encode);
	AddFunction(L, "base64_decode", Crypt::base64_decode);
	lua_newtable(L);
	NewTableFunction(L, "encode", Crypt::base64_encode);
	NewTableFunction(L, "decode", Crypt::base64_decode);
	lua_setfield(L, LUA_GLOBALSINDEX, ("base64"));
	lua_newtable(L);
	NewTableFunction(L, "base64encode", Crypt::base64_encode);
	NewTableFunction(L, "base64decode", Crypt::base64_decode);
	NewTableFunction(L, "base64_encode", Crypt::base64_encode);
	NewTableFunction(L, "base64_decode", Crypt::base64_decode);
	lua_newtable(L);
	NewTableFunction(L, "encode", Crypt::base64_encode);
	NewTableFunction(L, "decode", Crypt::base64_decode);
	lua_setfield(L, -2, ("base64"));
	NewTableFunction(L, "encrypt", Crypt::encrypt);
	NewTableFunction(L, "decrypt", Crypt::decrypt);
	NewTableFunction(L, "generatebytes", Crypt::generatebytes);
	NewTableFunction(L, "generatekey", Crypt::generatekey);
	NewTableFunction(L, "hash", Crypt::hash);
	lua_setfield(L, LUA_GLOBALSINDEX, ("crypt"));

	// Filesystem Library
	AddFunction(L, "dofile", Filesystem::dofile);
	AddFunction(L, "appendfile", Filesystem::appendfile);
	AddFunction(L, "delfile", Filesystem::delfile);
	AddFunction(L, "delfolder", Filesystem::delfolder);
	AddFunction(L, "getcustomasset", Filesystem::getcustomasset);
	AddFunction(L, "isfile", Filesystem::isfile);
	AddFunction(L, "isfolder", Filesystem::isfolder);
	AddFunction(L, "listfiles", Filesystem::listfiles);
	AddFunction(L, "loadfile", Filesystem::loadfile);
	AddFunction(L, "makefolder", Filesystem::makefolder);
	AddFunction(L, "readfile", Filesystem::readfile);
	AddFunction(L, "writefile", Filesystem::writefile);

	// Debug Library
	lua_getglobal(L, "debug");
	lua_getglobal(L, "setreadonly");
	lua_pushvalue(L, -2);
	lua_pushboolean(L, false);
	lua_pcall(L, 2, 0, 0);
	NewTableFunction(L, "getconstant", Debug::getconstant); // why crash?
	NewTableFunction(L, "getconstants", Debug::getconstants);
	NewTableFunction(L, "getproto", Debug::getproto);
	NewTableFunction(L, "getprotos", Debug::getprotos);
	NewTableFunction(L, "getstack", Debug::getstack);
	NewTableFunction(L, "getupvalue", Debug::getupvalue);
	NewTableFunction(L, "getupvalues", Debug::getupvalues);
	NewTableFunction(L, "setconstant", Debug::setconstant);
	NewTableFunction(L, "setstack", Debug::setstack);
	NewTableFunction(L, "setupvalue", Debug::debug_setupvalue);
	NewTableFunction(L, "getinfo", Debug::getinfo);
	lua_pop(L, 1);

	// Cache Library
	lua_newtable(L);
	NewTableFunction(L, "iscached", Cache::iscached);
	NewTableFunction(L, "replace", Cache::replace);
	NewTableFunction(L, "invalidate", Cache::invalidate);
	lua_setfield(L, LUA_GLOBALSINDEX, ("cache"));

	// Instances Library
	AddFunction(L, "cloneref", Instances::cloneref);
	AddFunction(L, "compareinstances", Instances::compareinstances);
	AddFunction(L, "fireclickdetector", Instances::fireclickdetector);
	AddFunction(L, "fireproximityprompt", Instances::fireproximityprompt);
	AddFunction(L, "firetouchinterest", Instances::firetouchinterest);
	AddFunction(L, "gethui", Instances::gethui);
	AddFunction(L, "getinstances", Instances::getinstances);
	AddFunction(L, "getnilinstances", Instances::getnilinstances);

	// Signals Library
	//AddFunction(L, "firesignal", Signals::firesignal);
	//AddFunction(L, "getconnections", Signals::getconnections);

	GameHooks::InitializeHooks(L);

	lua_newtable(L);
	lua_setglobal(L, "_G");

	lua_newtable(L);
	lua_setglobal(L, "shared");

	lua_getglobal(L, "game");
	lua_getfield(L, -1, "GetService");
	lua_pushvalue(L, -2);
	lua_pushstring(L, "CoreGui");
	lua_call(L, 2, 1);

	lua_setglobal(L, "__hiddeninterface");
}