#include "Extras.hpp"
#include <Misc/Utils.hpp>
#include <Update/Offsets.hpp>
#include <Execution/Execution.hpp>

void Extras::HPrint(int mode, const char* fmt) {
    InsertFunctionHook(Roblox::Print(mode, fmt));
}


void Extras::Execute(const std::string& content) {
    InsertFunctionHook(Execution->Execute(content));
}
