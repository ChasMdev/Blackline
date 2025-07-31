#pragma once
#include <Windows.h>
#include <string>


class JobHooker {

private:
	static uintptr_t GetJob(const std::string& name);
public:
	static void Hook(const std::string& name, void* function);
	static uintptr_t InsertThread();

};