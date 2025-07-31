#pragma once
#include <Windows.h>
#include <functional>
#include <TaskScheduler/TaskScheduler.hpp>
#include <Update/Offsets.hpp>
#include <Execution/Execution.hpp>
#include <Hooks/Jobs/JobHooker.hpp>
#include <Environment/Environment.hpp>
#include <mutex>

class Utils {
private:

public:
	static bool CheckMemory(uintptr_t address);
	static void Teleport_handler();
	static void RegisterHook(std::function<void()> func);

};

#define InsertFunctionHook(x) Utils::RegisterHook([&]() { (x); })

