#include "TaskScheduler.hpp"

uintptr_t BTaskscheduler::GetJobList() {
	uintptr_t jobList = *reinterpret_cast<uintptr_t*>(Offsets::RawScheduler::RawScheduler + Offsets::Job::JobStart);
	return jobList;
}

uintptr_t BTaskscheduler::GetJobOffset(const char* jobname, uintptr_t jobListBase, int maxJobs) {
	for (int i = 0; i < maxJobs; i++) {
		uintptr_t jobPtrAddr = jobListBase + i * sizeof(uintptr_t);
		uintptr_t jobPtr = *reinterpret_cast<uintptr_t*>(jobPtrAddr);
		if (jobPtr == 0)
			continue;

		char nameBuffer[64] = {};
		__try {
			const char* namePtr = reinterpret_cast<const char*>(jobPtr + 0x18);
			strncpy_s(nameBuffer, namePtr, sizeof(nameBuffer) - 1);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			continue;
		}
		if (strlen(nameBuffer) == 0)
			continue;
		if (strstr(nameBuffer, jobname)) {
			return i * sizeof(uintptr_t);
		}
	}
	return 0;
}

uintptr_t BTaskscheduler::GetDataModel() {
	//uintptr_t JobList = GetJobList();
	//uintptr_t LuaGc = GetJobOffset("LuaGc", JobList);
	//if (LuaGc == 0x0)
	//	return 0x0;
	//
	//uintptr_t LuaGcR = *(uintptr_t*)(JobList + LuaGc);
	uintptr_t FakeDataModelPointer = *(uintptr_t*)(Offsets::DataModel::FakeDataModelPointer);
	uintptr_t DataModel = *(uintptr_t*)(FakeDataModelPointer + Offsets::DataModel::FakeDataModelToReal);

	uintptr_t DataModelNamePointer = *(uintptr_t*)(DataModel + Offsets::Properties::Name);
	std::string DataModelName = *(std::string*)(DataModelNamePointer);

	if (DataModelName == "LuaApp") {
		return 0x0; // Homescreen
	}

	return DataModel;
}
uintptr_t BTaskscheduler::GetScriptContext(uintptr_t datamodel) {
	uintptr_t DataModelChildrenContainer = *(uintptr_t*)(datamodel + Offsets::Properties::Children);
	uintptr_t DataModelChildrens = *(uintptr_t*)(DataModelChildrenContainer);

	uintptr_t ScriptContext = *(uintptr_t*)(DataModelChildrens + Offsets::ScriptContext::ScriptContext);

	return ScriptContext;
}

uintptr_t BTaskscheduler::GetLuaState(uintptr_t datamodel) {
	if (datamodel == 0x0)
		return 0x0;

	//uintptr_t encryptedState = GetScriptContext(datamodel) + Offsets::GlobalState + Offsets::GlobalStateStart + Offsets::EncryptedState;
	//uint32_t low = *(uint32_t*)encryptedState ^ (uint32_t)encryptedState;
	//uint32_t high = *(uint32_t*)(encryptedState + 0x4) ^ (uint32_t)encryptedState;
	//uintptr_t luaState = ((uint64_t)high << 32) | low;
	//return luaState;

	uint64_t a2 = 0;
	uint64_t a3 = 0;

	uintptr_t luaState = Roblox::GetLuaState(GetScriptContext(datamodel), &a2, &a3);

	return luaState;
}

bool BTaskscheduler::SetFPS(int val) {
	*(uintptr_t*)Offsets::RawScheduler::TaskSchedulerTargetFps = val;

	return true;
}

int BTaskscheduler::GetFPS() {
	return *(uintptr_t*)Offsets::RawScheduler::TaskSchedulerTargetFps;
}

void BTaskscheduler::SetProtoCapabilities(Proto* proto) {
	proto->userdata = &MaxCapabilities;
	for (int i = 0; i < proto->sizep; i++) {
		SetProtoCapabilities(proto->p[i]);
	}
}
void BTaskscheduler::ElevateProtoInternal(Proto* proto, uint64_t* capabilityToSet) {
	proto->userdata = capabilityToSet;

	Proto** protoProtos = proto->p;
	for (int i = 0; i < proto->sizep; i++) {
		ElevateProtoInternal(protoProtos[i], capabilityToSet);
	}
}
void BTaskscheduler::ElevateProto(Closure* closure) {
	if (closure && closure->l.p)
	{
		auto capabilitiesInMemory = new uint64_t(MAXCAPABILITIES);
		ElevateProtoInternal(closure->l.p, capabilitiesInMemory);
	}
}

void BTaskscheduler::SetThreadIdentity(lua_State* l, int identity, uintptr_t capabilities) {
	auto extraspace = (uintptr_t)(l->userdata);
	*(uintptr_t*)(extraspace + Offsets::ExtraSpace::Identity) = identity;
	*(uintptr_t*)(extraspace + Offsets::ExtraSpace::Capabilities) = capabilities;

	int64_t a1[128];
	Roblox::Impersonator(a1, &identity, (__int64)((uintptr_t)l->userdata + Offsets::ExtraSpace::Capabilities));
}

lua_State* BTaskscheduler::CreateThread(uintptr_t lstate) {
	Globals::ExploitThread = lua_newthread((lua_State*)lstate);
	luaL_sandboxthread(Globals::ExploitThread);

	return Globals::ExploitThread;
}

bool BTaskscheduler::CheckThread(lua_State* l) {
	if (l == nullptr)
		return false;
	const auto threadUserdata = l->userdata;
	return threadUserdata != nullptr &&
		threadUserdata->Script.lock() == nullptr && l->userdata->Script.expired() && (
			threadUserdata->Capabilities & MAXCAPABILITIES) == MAXCAPABILITIES;
}
bool BTaskscheduler::IsProtoInternal(const Proto* proto, std::unordered_set<const Proto*>& visited) {
	if (!proto || !proto->userdata || visited.contains(proto)) {
		return false;
	}
	visited.insert(proto);

	const auto* capsPtr = static_cast<const uint64_t*>(proto->userdata);
	if ((*capsPtr & MAXCAPABILITIES) == MAXCAPABILITIES) {
		return true;
	}

	if (proto->p && proto->sizep > 0) {
		for (int i = 0; i < proto->sizep; i++) {
			const auto currentProto = proto->p[i];
			if (IsProtoInternal(currentProto, visited)) {
				return true;
			}
		}
	}

	return false;
}
bool BTaskscheduler::CheckProto(const Proto* proto) {
	std::unordered_set<const Proto*> visited;
	return IsProtoInternal(proto, visited);
}

std::string BTaskscheduler::GetPlaceID(lua_State* LS) {
	lua_getglobal(LS, "game");
	lua_getfield(LS, -1, "PlaceId");
	std::string PlaceId = lua_tostring(LS, -1);

	return PlaceId;
}

std::string BTaskscheduler::GetGameID(lua_State* LS) {
	lua_getglobal(LS, "game");
	lua_getfield(LS, -1, "GameId");
	std::string GameId = lua_tostring(LS, -1);

	return GameId;
}

std::string BTaskscheduler::GetJobID(lua_State* LS) {
	lua_getglobal(LS, "game");
	lua_getfield(LS, -1, "JobId");
	std::string GameId = lua_tostring(LS, -1);

	return GameId;
}