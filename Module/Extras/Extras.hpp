#pragma once
#include <Windows.h>

#include <string>


class Extras {
private:


public:
	static void HPrint(int, const char*);
	static void Execute(const std::string& luasrc);
};
