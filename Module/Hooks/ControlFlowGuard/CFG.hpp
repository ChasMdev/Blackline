#pragma once
#include <Windows.h>


class ControlFlowGuard {
private:

public:
	static void PatchCFG(uintptr_t address);
	static void PatchModule(HMODULE dll);
	static void HookDeliberateCrash();
};