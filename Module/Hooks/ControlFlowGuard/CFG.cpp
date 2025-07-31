#include "CFG.hpp"
#include <iostream>
#include <Update/Offsets.hpp>
#include <TlHelp32.h>
#include <Psapi.h>



void ControlFlowGuard::PatchCFG(uintptr_t address) {
	uintptr_t bitmap = *(uintptr_t*)(Offsets::Hyperion::BitMap);
	uintptr_t byteOffset = (address >> 0x13);
	uintptr_t bitOffset = (address >> 0x10) & 7;

	uint8_t* Cache = (uint8_t*)(bitmap + byteOffset);

	DWORD oldProtect;
	VirtualProtect(Cache, 1, PAGE_READWRITE, &oldProtect);

	*Cache |= (1 << bitOffset);

	VirtualProtect(Cache, 1, oldProtect, &oldProtect);
}

void ControlFlowGuard::PatchModule(HMODULE dll) {
	uintptr_t dll_base = (uintptr_t)(dll);

	MODULEINFO modinfo{};
	GetModuleInformation(GetCurrentProcess(), dll, &modinfo, sizeof(modinfo));
	SIZE_T dll_size = modinfo.SizeOfImage;

	uintptr_t bitmap = *(uintptr_t*)(Offsets::Hyperion::BitMap);

	for (uintptr_t page = dll_base; page < dll_base + dll_size; page += 0x1000) {
		uintptr_t byte_offset = (page >> 0x13);
		uintptr_t bit_offset = (page >> 0x10) & 7;
		uint8_t* cfg_byte = (uint8_t*)(bitmap + byte_offset);
		*cfg_byte |= (1 << bit_offset);
	}
}

void ControlFlowGuard::HookDeliberateCrash() {

	
}