#include "pch.h"
#include "ClientAPI.hpp"
#include <vector>
#include "Pipeline.hpp"

// Global handle for the server thread (if needed)
HANDLE serverThread = NULL;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    static Pipeline pipeline(L"\\\\.\\pipe\\clientpipe", 32768); // Static initialization will persist

    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        serverThread = CreateThread(NULL, 0, Pipeline::RunServer, &pipeline, 0, NULL);
        if (!serverThread) {
        }
        break;

    case DLL_PROCESS_DETACH:
        // Handle any necessary cleanup
        if (serverThread) {
            // Close the handle:
            CloseHandle(serverThread);
        }
        break;
    }

    return TRUE;
}
