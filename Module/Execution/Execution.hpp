#pragma once
#include <string>
#include <memory>
#include "lobject.h"

struct lua_State;

class BExecution {
public:
	void Execute(std::string);
	void SetProtoCapabilities(Proto*);
	std::string CompileScript(const std::string);
};

inline auto Execution = std::make_unique<BExecution>();