#include "JobHooker.hpp"
#include <TaskScheduler/TaskScheduler.hpp>
#include <Misc/Utils.hpp>
#include <Update/Offsets.hpp>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <Hooks/ControlFlowGuard/CFG.hpp>
#include <thread>


typedef uintptr_t(__stdcall* t_job)(uintptr_t, uintptr_t, uintptr_t);
t_job original_job_fn = nullptr;
uintptr_t Job;
uintptr_t o_vtable;


uintptr_t JobHooker::GetJob(const std::string& name) {
    uintptr_t joblist = Taskscheduler->GetJobList();
    if (!Utils::CheckMemory(joblist)) {
        return 0x0;
    }
    uintptr_t JobBase = Taskscheduler->GetJobOffset(name.c_str(), joblist, 256);
    uintptr_t heartbeatJob = *(uintptr_t*)(joblist + JobBase);

    if (!Utils::CheckMemory(heartbeatJob)) {
        return 0x0;
    }

    return heartbeatJob;
}


uintptr_t JobHooker::InsertThread() {

    uintptr_t name = *(uintptr_t*)(Job + Offsets::Job::JobName);
    Roblox::Print(3, "[%s] 0x%llx -> 0x%llx", *(std::string*)name, o_vtable, &InsertThread); // do not use char cuz its a ptr to a ptr
    std::lock_guard<std::mutex> lock(g_hooks_mutex);
    for (auto& hookFunc : g_hooks) {
        hookFunc();
    }
}

void JobHooker::Hook(const std::string& name, void* HookedFunction) {
    Job = (uintptr_t)GetJob(name.c_str());
    if (!Utils::CheckMemory(Job)) {
        return;
    }

    void** vtable = *(void***)(Job);
    if (!vtable) {
        return;
    }
    o_vtable = (uintptr_t)vtable;
    constexpr int idx = 1, size = 768;
    auto new_vtable = new void* [size];
    memcpy(new_vtable, vtable, sizeof(void*) * size);
    original_job_fn = (t_job)new_vtable[idx];
    new_vtable[idx] = HookedFunction;
    *(void***)(Job) = new_vtable;
}
