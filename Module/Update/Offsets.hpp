#pragma once
#include <functional>
#include <Windows.h>
#include <iostream>
#include <mutex>

#define REBASE(x) x + (uintptr_t)GetModuleHandleA(nullptr)
#define REBASEHYPERION(x) x + (uintptr_t)GetModuleHandleA("RobloxPlayerBeta.dll")
#define MAXCAPABILITIES (0x200000000000003FLL | 0xFFFFFFF00LL) | (1ull << 48ull)
inline std::vector<std::function<void()>> g_hooks;
inline std::mutex g_hooks_mutex;

struct lua_State;
struct WeakThreadRef {
	int _Refs;
	lua_State* thread;
	int32_t thread_ref;
	int32_t objectId;
};

// updated for version-2a06298afe3947ab

namespace Offsets
{
	namespace RawScheduler
	{
		const uintptr_t RawScheduler = REBASE(0x69A7320);
		const uintptr_t TaskSchedulerTargetFps = REBASE(0x609DEBC);
	}

	namespace Fires
	{
		const uintptr_t FireMouseClick = REBASE(0x1BD8150);
		const uintptr_t FireRightMouseClick = REBASE(0x1BD82F0);
		const uintptr_t FireMouseHoverEnter = REBASE(0x1BD96F0);
		const uintptr_t FireMouseHoverLeave = REBASE(0x1BD9890);
		const uintptr_t FireProximityPrompt = REBASE(0x1CB0A00);
		const uintptr_t FireTouchInterest = REBASE(0x1DFBAC0);
	}

	const uintptr_t KTable = REBASE(0x609DF40);
	const uintptr_t LockViolationInstanceCrash = REBASE(0x57CDBB0);
	const uintptr_t GetCurrentThreadId = REBASE(0x38E6B60);
	const uintptr_t SetProtoCapabilitiess = REBASE(0xCAFD40);
	const uintptr_t GetFFlag = REBASE(0x3906620);
	const uintptr_t GetIdentityStruct = REBASE(0x38E6D90);
	const uintptr_t GetModuleFromVMStateMap = REBASE(0xF4BA60);
	const uintptr_t GetProperty = REBASE(0xAA3C50);
	const uintptr_t GetValues = REBASE(0xC19A40);
	const uintptr_t Flag_SetValue = REBASE(0x3906620);

	namespace Luau
	{
		const uintptr_t LuaH_DummyNode = REBASE(0x47BED08);
		const uintptr_t LuaO_NilObject = REBASE(0x47BF5D8);
		const uintptr_t LuaT_Eventnames = REBASE(0x47BF530);
		const uintptr_t LuaT_Typenames = REBASE(0x47BF4D0);
		const uintptr_t Impersonator = REBASE(0x33E5630);
		const uintptr_t LuaA_toobject = REBASE(0x267B440);
		const uintptr_t LuaC_step = REBASE(0x268ACE0);
		const uintptr_t LuaD_throw = REBASE(0x268B3C0);
		const uintptr_t LuaG_runerrorL = REBASE(0x268E5F0);
		const uintptr_t LuaL_checkany = REBASE(0x269BC90);
		const uintptr_t LuaL_checklstring = REBASE(0x267ECF0);
		const uintptr_t LuaL_getmetafield = REBASE(0x267F190);
		const uintptr_t luaO_chunkid = REBASE(0x26E3470);
		const uintptr_t LuaL_register = REBASE(0x2680450);
		const uintptr_t LuaL_typeerrorL = REBASE(0x267CF90);
		const uintptr_t LuaM_visitgco = REBASE(0x26CEBB0);
		const uintptr_t LuaO_pushvfstring = REBASE(0x26E3440);
		const uintptr_t LuaV_gettable = REBASE(0x26D0760);
		const uintptr_t LuaV_settable = REBASE(0x26D0D80);
		const uintptr_t LuaVM_Load = REBASE(0xB91F90);
		const uintptr_t Luau_Execute = REBASE(0x26BAE40);

		namespace LuaState
		{
			const uintptr_t GetLuaState = REBASE(0xB8DA40);
			const uintptr_t GlobalState = 0x140;
			const uintptr_t EncryptedState = 0x88;
			const uintptr_t GlobalStatePointer = 0x140;
			const uintptr_t GlobalStateStart = 0x188;
			const uintptr_t DecryptState = 0x88;
		}
	}

	namespace ScriptContext {
		const uintptr_t ScriptContextStart = 0x1F8;
		const uintptr_t ScriptContext = 0x3C0;
	}

	namespace Properties {
		const uintptr_t ClassDescriptor = 0x18;
		const uintptr_t PropertyDescriptor = 0x3c0;
		const uintptr_t ClassName = 0x8;
		const uintptr_t Name = 0x78;
		const uintptr_t Children = 0x80;
	}

	const uintptr_t Print = REBASE(0x14D2A10);
	const uintptr_t Pseudo2Addr = REBASE(0x267B380);
	const uintptr_t PushInstance1 = REBASE(0xEDB9D0);
	const uintptr_t PushInstance2 = REBASE(0xEDBA20);
	const uintptr_t OpCodeTableLookup = REBASE(0x4DBE9A0);
	const uintptr_t RaiseEventInvocation = REBASE(0x147ED10);
	const uintptr_t RequestCode = REBASE(0x946B00);
	const uintptr_t ScriptContextResume = REBASE(0xE0F940);
	const uintptr_t SetFFlag = REBASE(0x39070B0);
	const uintptr_t TaskSynchronize = REBASE(0x1024C30);
	const uintptr_t TaskDesynchronize = REBASE(0x1025160);
	const uintptr_t TaskDefer = REBASE(0x1025D60);
	const uintptr_t TaskSpawn = REBASE(0x1026090);
	const uintptr_t TaskDelay = REBASE(0x1026430);
	const uintptr_t TaskWait = REBASE(0x10267E0);
	const uintptr_t TaskCancel = REBASE(0x1026A10);
	const uintptr_t cast_to_variant = REBASE(0xC19A40);
	const uintptr_t VariantCastInt = REBASE(0x1462070);
	const uintptr_t VariantCastInt64 = REBASE(0x1462380);
	const uintptr_t VariantCastFloat = REBASE(0x14629C0);

	namespace Hyperion
	{
		const uintptr_t BitMap = REBASEHYPERION(0x2B74D0);
		const uintptr_t SetInsert = REBASEHYPERION(0xBFD2C0);
		const uintptr_t WhitelistedPages = REBASEHYPERION(0x29A710);
		const uintptr_t PageHash = 0xF7455279;
		const uintptr_t ByteHash = 0xA9;
	}

	namespace ExtraSpace
	{
		const uintptr_t Identity = 0x30;
		const uintptr_t Capabilities = 0x48;
	}

	namespace DataModel
	{
		const uintptr_t FakeDataModelPointer = REBASE(0x68D7308);
		const uintptr_t FakeDataModelToReal = 0x1C0;
		const uintptr_t HeartBeatDataModelInstance = 0x550;
		const uintptr_t GameLoaded = 0x420;
	}

	namespace Instance {
		const uintptr_t InstanceClassDescriptor = 0x18;
		const uintptr_t Primitive = 0x178LL;
		const uintptr_t Overlap = 0x288;
		const uintptr_t Flags = 0x50;
		namespace ClassDescriptor {
			const uintptr_t PropertiesStart = 0x30;
			const uintptr_t PropertiesEnd = 0x38;
			const uintptr_t PropertyDescriptor = 0x3C0;
			const uintptr_t Type = 0x60;
			const uintptr_t TypeGetSetDescriptor = 0x98;
			namespace GetSetDescriptor {
				const uintptr_t getVFtableFunc = 0x18;
			}
			namespace TypeStructure {
				enum PropertyType : uint32_t {
					BinaryString = 0x1E,
					SystemAddress = 0x1D,
					SharedString = 0x3D,
					Enum = 0x20,
					String = 0x6,
					Instance = 0x8
				};

				const uintptr_t typeId = 0x38;
			}
			namespace Property {
				const uintptr_t Name = 0x8;
				const uintptr_t Capabilities = 0x48;
			}
		}
	}

	namespace LocalScript {
		const uintptr_t ProtectedBytecode = 0x1B0;
		const uintptr_t ThreadNodeList = 0x188;
		namespace ThreadNode {
			const uintptr_t next = 0x18;
			const uintptr_t liveLuaRef = 0x20;
		}
	}

	namespace ModuleScript {
		const uintptr_t ProtectedBytecode = 0x158;
		const uintptr_t VmStateMap = 0x1A8;
		namespace VmState {
			const uintptr_t LoadedStatus = 0x18;
			const uintptr_t LuaState = 0x28;
		}
	}

	namespace Scripts {
		const uintptr_t ScriptInstance = 0x50;
		const uintptr_t LocalScriptEmbedded = 0x1B0;
		const uintptr_t LocalScriptHash = 0x1C0;
		const uintptr_t ModuleScriptEmbedded = 0x158;
		const uintptr_t ModuleScriptHash = 0x180;
	}

	namespace Thread {
		const uintptr_t weak_thread_node = 0x188;
		const uintptr_t weak_thread_ref = 0x8;
		const uintptr_t weak_thread_ref_live = 0x20;
		const uintptr_t weak_thread_ref_live_thread = 0x8;
	}


	namespace Job {
		const uintptr_t FpsCap = 0x1B0;
		const uintptr_t JobStart = 0x1D0;
		const uintptr_t JobName = 0x18;
	}

	inline uintptr_t RakNetBase = 0x218;

	inline uintptr_t MessageBoxA = (uintptr_t)GetProcAddress(GetModuleHandleA("user32.dll"), "MessageBoxA");

	namespace Security {
		const uintptr_t Require_bypass = 0x7E8;
		const uintptr_t PlaceId = 0x1A0;
	}
}

namespace Roblox
{
	using TPrint = void(__cdecl*)(int, const char*, ...);
	inline auto Print = reinterpret_cast<TPrint>(Offsets::Print);

	using TLuaVM__Load = int(__fastcall*)(lua_State*, void*, const char*, int);
	inline auto LuaVM__Load = reinterpret_cast<TLuaVM__Load>(Offsets::Luau::LuaVM_Load);

	inline auto Impersonator = (void(__fastcall*)(std::int64_t*, std::int32_t*, std::int64_t))Offsets::Luau::Impersonator;

	inline auto KTable = reinterpret_cast<uintptr_t*>(Offsets::KTable);

	using TTask__Defer = int(__fastcall*)(lua_State*);
	inline auto Task__Defer = reinterpret_cast<TTask__Defer>(Offsets::TaskDefer);

	using TGetLuaState = uintptr_t(__fastcall*)(int64_t, uint64_t*, uint64_t*);
	inline auto GetLuaState = reinterpret_cast<TGetLuaState>(Offsets::Luau::LuaState::GetLuaState);

	using TPushInstance = void(__fastcall*)(lua_State* state, void* instance);
	inline auto PushInstance = reinterpret_cast<TPushInstance>(Offsets::PushInstance1);

	using TLuaD_throw = void(__fastcall*)(lua_State*, int);
	inline auto LuaD_throw = reinterpret_cast<TLuaD_throw>(Offsets::Luau::LuaD_throw);

	using TGetProperty = uintptr_t * (__thiscall*)(uintptr_t, uintptr_t*);
	inline auto GetProperty = reinterpret_cast<TGetProperty>(Offsets::GetProperty);

	inline auto FireTouchInterest = (void(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, bool, bool))Offsets::Fires::FireTouchInterest;

	using TFireProxmityPrompt = std::uintptr_t(__fastcall*)(std::uintptr_t prompt);
	inline auto FireProximityPrompt = reinterpret_cast<TFireProxmityPrompt>(Offsets::Fires::FireProximityPrompt);

	using TRequestCode = uintptr_t(__fastcall*)(uintptr_t protected_string_ref, uintptr_t script);
	inline auto RequestCode = reinterpret_cast<TRequestCode>(Offsets::RequestCode);

	using TFireMouseClick = void(__fastcall*)(__int64 a1, float a2, __int64 a3);
	inline auto FireMouseClick = reinterpret_cast<TFireMouseClick>(Offsets::Fires::FireMouseClick);

	using TFireRightMouseClick = void(__fastcall*)(__int64 a1, float a2, __int64 a3);
	inline auto FireRightMouseClick = reinterpret_cast<TFireRightMouseClick>(Offsets::Fires::FireRightMouseClick);

	using TFireMouseHoverEnter = void(__fastcall*)(__int64 a1, __int64 a2);
	inline auto FireMouseHoverEnter = reinterpret_cast<TFireMouseHoverEnter>(Offsets::Fires::FireMouseHoverEnter);

	using TFireMouseHoverLeave = void(__fastcall*)(__int64 a1, __int64 a2);
	inline auto FireMouseHoverLeave = reinterpret_cast<TFireMouseHoverLeave>(Offsets::Fires::FireMouseHoverLeave);

	inline auto TaskDefer = (int(__fastcall*)(lua_State*))Offsets::TaskDefer;

	using TGetModuleVmState = uintptr_t(__fastcall*)(uintptr_t* vmStateMap, uintptr_t* receivedVmState, lua_State** mainThread);
	inline auto GetModuleVmState = reinterpret_cast<TGetModuleVmState>(Offsets::GetModuleFromVMStateMap);

	using TGetIdentityStruct = void* (__fastcall*)(uintptr_t identityData);
	inline auto GetIdentityStruct = reinterpret_cast<TGetIdentityStruct>(Offsets::GetIdentityStruct);

	using TMessageBoxA = int(__stdcall*)(HWND, LPCSTR, LPCSTR, UINT);
	inline void MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
		auto fn = (TMessageBoxA)(Offsets::MessageBoxA);
		std::thread([=]() {
			fn(hWnd, lpText, lpCaption, uType);
			}).detach();
	}


	inline std::string RequestBytecode(uintptr_t scriptPtr) {
		uintptr_t code[0x4];
		std::memset(code, 0, sizeof(code));

		RequestCode((std::uintptr_t)code, scriptPtr);

		std::uintptr_t bytecodePtr = code[1];

		if (!bytecodePtr) { return "Failed to get bytecode"; }

		std::uintptr_t str = bytecodePtr + 0x10;
		std::uintptr_t data;

		if (*reinterpret_cast<std::size_t*>(str + 0x18) > 0xf) {
			data = *reinterpret_cast<std::uintptr_t*>(str);
		}
		else {
			data = str;
		}

		std::string BOOOHOOOOOOOO;
		std::size_t len = *reinterpret_cast<std::size_t*>(str + 0x10);
		BOOOHOOOOOOOO.reserve(len);

		for (unsigned i = 0; i < len; i++) {
			BOOOHOOOOOOOO += *reinterpret_cast<char*>(data + i);
		}

		if (BOOOHOOOOOOOO.size() <= 8) { "Failed to get bytecode"; }

		return BOOOHOOOOOOOO;
	}
}

namespace Globals {
	inline lua_State* ExploitThread;
	inline uintptr_t DataModel;

	inline bool raksniff;
	inline bool ishooked;
}