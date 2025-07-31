#include "Utils.hpp"
#include <Windows.h>
#include <functional>
#include <TaskScheduler/TaskScheduler.hpp>
#include <Update/Offsets.hpp>
#include <Execution/Execution.hpp>
#include <Hooks/Jobs/JobHooker.hpp>
#include <Environment/Environment.hpp>
#include <mutex>

using namespace Globals;

bool Utils::CheckMemory(uintptr_t addr) {
	if (addr < 0x10000 || addr > 0x7FFFFFFFFFFF) {
		return false;
	}
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery(reinterpret_cast<void*>(addr), &mbi, sizeof(mbi)) == 0) {
		return false;
	}
	if (mbi.Protect & PAGE_NOACCESS || mbi.State != MEM_COMMIT) {
		return false;
	}
	return true;
}

void Utils::Teleport_handler() {
	uintptr_t cachedDM;

	while (true) {
		cachedDM = Taskscheduler->GetDataModel();

		if (cachedDM != DataModel && cachedDM != 0x0) {
			DataModel = cachedDM;
			uintptr_t LuaState = Taskscheduler->GetLuaState(DataModel);
			ExploitThread = Taskscheduler->CreateThread(LuaState);
			Taskscheduler->SetThreadIdentity(ExploitThread, 8, MAXCAPABILITIES);
			Sleep(100);

			continue;
		}
		Sleep(100);
	}
}

void Utils::RegisterHook(std::function<void()> func) {
	std::lock_guard<std::mutex> lock(g_hooks_mutex);
	g_hooks.push_back(func);
}



