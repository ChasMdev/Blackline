#pragma once
#include <Windows.h>
#include <thread>
#include <vector>
#include <Execution/Execution.hpp>
#include <Update/Offsets.hpp>

namespace Communication {

    class NamedPipe {
    private:
        static constexpr const char* PIPE_NAME = "\\\\.\\pipe\\BlacklineExecution";
        static constexpr DWORD BUFFER_SIZE = 65536;
        HANDLE hPipe;
        bool running;
        std::thread PipeThread;

    public:
        NamedPipe();
        ~NamedPipe();

        bool Start();
        void Stop();

    private:
        void HandleClient();

    public:
        static int InitializeNamePipe();
    };

}
