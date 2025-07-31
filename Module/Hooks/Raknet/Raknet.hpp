#include <Windows.h>
#include <iostream>

class Raknet {
private:
	static uintptr_t GetPacketJob();
	static void Integrate();
	static int64_t __fastcall hookfunction(int64_t* this_ptr, int64_t* bitStream, int priority, int reliability, char orderingChannel, int64_t* systemIdentifier, char broadcast, int shift);
public:
	static void Hook();
};