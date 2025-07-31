#include "Raknet.hpp"
#include <TaskScheduler/TaskScheduler.hpp>
#include <Misc/Utils.hpp>
#include <Update/Offsets.hpp>
#include <Extras/Extras.hpp>

uintptr_t Raknet::GetPacketJob() {
	uintptr_t joblist = Taskscheduler->GetJobList();
	if (!Utils::CheckMemory(joblist)) {
		return 0x0;
	}
	uintptr_t JobBase = Taskscheduler->GetJobOffset("Net Peer Send", joblist, 256);
	uintptr_t PacketJob = *(uintptr_t*)(joblist + JobBase);

	if (!Utils::CheckMemory(PacketJob)) {
		return 0x0;
	}

	return PacketJob;
}


using send_fn_t = int64_t(__fastcall*)(int64_t*, int64_t*, int, int, char, int64_t*, char, int);
inline send_fn_t original_raknet_send = nullptr;

using receive_fn_t = int64_t(__fastcall*)(void*);
inline receive_fn_t original_raknet_receive;

int64_t __fastcall Raknet::hookfunction(int64_t* this_ptr, int64_t* bitStream, int priority, int reliability, char orderingChannel, int64_t* systemIdentifier, char broadcast, int shift) {
	uintptr_t BitStreamDereference = bitStream[0];
	uint8_t* packetBytes = *reinterpret_cast<uint8_t**>(BitStreamDereference + 0x10);

	uint8_t packetId = packetBytes[0];



	if (broadcast == 1) {
		// chatmessage
	}
	if (packetId == 0x1B) {
		// physicspackets

	}


	return original_raknet_send(
		this_ptr, bitStream, priority, reliability,
		orderingChannel, systemIdentifier, broadcast, shift
	);
}

int64_t __fastcall hookfunctionrec(void* this_ptr) {
	



	return 0; //original_raknet_receive(this_ptr);
}



void Raknet::Integrate() {
	uintptr_t PacketJob = GetPacketJob();
	uintptr_t RakNetBase = *reinterpret_cast<uintptr_t*>(PacketJob + Offsets::RakNetBase);
	if (!Utils::CheckMemory(RakNetBase)) {
		Roblox::Print(3, "RakNetBase is corrupted or invalid.");
		return;
	}
	uintptr_t RakPeer = RakNetBase;

	const int vtable_index_send = 20;
	const int vtable_index_receive = 22;
	const int vtable_size = 150;

	void** original_vtable = *(void***)(RakPeer);
	void** new_vtable = new void* [vtable_size];

	memcpy(new_vtable, original_vtable, sizeof(void*) * vtable_size);

	original_raknet_send = reinterpret_cast<send_fn_t>(new_vtable[vtable_index_send]);
	new_vtable[vtable_index_send] = reinterpret_cast<void*>(&hookfunction);

	*(void***)(RakPeer) = new_vtable;


	void** original_vtable1 = *(void***)(RakPeer);
	void** new_vtable1 = new void* [vtable_size];

	memcpy(new_vtable1, original_vtable1, sizeof(void*) * vtable_size);

	original_raknet_receive = reinterpret_cast<receive_fn_t>(new_vtable1[vtable_index_receive]);
	new_vtable1[vtable_index_receive] = reinterpret_cast<void*>(&hookfunctionrec);

	*(void***)(RakPeer) = new_vtable1;


	Globals::ishooked = true;
}

void Raknet::Hook() {
	if (!Globals::ishooked)
		Integrate();
}
