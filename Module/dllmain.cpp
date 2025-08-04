#include <thread>
#include <Windows.h>

#include "Communication/NamedPipe.hpp"
#include "Environment/Environment.hpp"
#include "Execution/Execution.hpp"
#include "Hooks/ControlFlowGuard/CFG.hpp"
#include "Hooks/Jobs/JobHooker.hpp"
#include "Misc/Utils.hpp"
#include "TaskScheduler/TaskScheduler.hpp"

std::string InitLuaScript = (R"(
	print("Blackline Loaded")
	for name, value in pairs(getgenv()) do
		if type(value) == "function" then
			print(">>LOADED FUNCTION: " .. name)
		end
	end

)");


void mainthread() {
	while (!Globals::ExploitThread) {
		Sleep(100);
		return mainthread();
	}

	JobHooker::Hook("WaitingHybridScriptsJob", &JobHooker::InsertThread);
	//InsertFunctionHook(MessageBoxA(0, 0, 0, 0));
	//Raknet::Hook();
	Environment::Init(Globals::ExploitThread);

	//Execution->Execute(InitLuaScript);
	Taskscheduler->SetFPS(1000);

	Communication::NamedPipe::InitializeNamePipe();
	while (true) {
		Sleep(1000);
	}

}

BOOL APIENTRY DllMain( HMODULE hModule,
					   DWORD  ul_reason_for_call,
					   LPVOID lpReserved
)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		ControlFlowGuard::PatchModule(hModule);

		std::thread(mainthread).detach();
		std::thread(Utils::Teleport_handler).detach();
	}
	return TRUE;
}

