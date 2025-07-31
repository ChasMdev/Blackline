// NamedPipe.cpp - Chunked, length-prefixed pipe receiver
#include "NamedPipe.hpp"

namespace Communication {

    NamedPipe::NamedPipe() : hPipe(INVALID_HANDLE_VALUE), running(false) {}

    NamedPipe::~NamedPipe() {
        Stop();
    }

    bool NamedPipe::Start() {
        running = true;

        while (running) {
            hPipe = CreateNamedPipeA(
                PIPE_NAME,
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                PIPE_UNLIMITED_INSTANCES,
                BUFFER_SIZE,
                BUFFER_SIZE,
                0,
                nullptr
            );

            if (hPipe == INVALID_HANDLE_VALUE) {
                return false;
            }

            BOOL connected = ConnectNamedPipe(hPipe, nullptr) ?
                TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

            if (connected) {
                HandleClient();
            }

            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }

        return true;
    }

    void NamedPipe::Stop() {
        running = false;
        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
        if (PipeThread.joinable()) {
            PipeThread.join();
        }
    }

    void NamedPipe::HandleClient() {
        DWORD bytesRead = 0;
        DWORD totalSize = 0;

        // Step 1: Read 4-byte length prefix
        if (!ReadFile(hPipe, &totalSize, sizeof(DWORD), &bytesRead, nullptr) || bytesRead != sizeof(DWORD)) {
            return;
        }

        std::vector<char> buffer(totalSize);
        size_t bytesReceived = 0;

        // Step 2: Read full message in chunks
        while (bytesReceived < totalSize) {
            DWORD chunk = 0;
            BOOL success = ReadFile(
                hPipe,
                buffer.data() + bytesReceived,
                static_cast<DWORD>(totalSize - bytesReceived),
                &chunk,
                nullptr
            );

            if (!success || chunk == 0) {
                return;
            }

            bytesReceived += chunk;
        }

        std::string luaScript(buffer.begin(), buffer.end());

        DWORD bytesWritten = 0;

        try {
            Execution->Execute(luaScript);
            const char* response = "Script executed successfully";
            WriteFile(hPipe, response, strlen(response), &bytesWritten, nullptr);
        }
        catch (const std::exception& e) {
            std::string errorMsg = "Execution error: " + std::string(e.what());
            WriteFile(hPipe, errorMsg.c_str(), static_cast<DWORD>(errorMsg.size()), &bytesWritten, nullptr);
        }

        FlushFileBuffers(hPipe);
    }

    int NamedPipe::InitializeNamePipe() {
        Roblox::Print(1, "Loaded pipe");

        NamedPipe server;

        std::thread serverThread([&server]() {
            server.Start();
            });

        if (serverThread.joinable()) {
            serverThread.join();
        }

        return 0;
    }
}