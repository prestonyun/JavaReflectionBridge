#include "pch.h"
#include "Pipeline.hpp"
#include "ClientAPI.hpp"
#include <sstream>

Pipeline::Pipeline(const std::wstring& pipeName, size_t bufferSize)
    : pipeName(pipeName), bufferSize(bufferSize), hPipe(INVALID_HANDLE_VALUE) {}

Pipeline::~Pipeline() {
    DisconnectAndClose();
}

void Pipeline::StartServer() {
    hPipe = CreateNamedPipe(
        pipeName.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        static_cast<DWORD>(bufferSize),
        static_cast<DWORD>(bufferSize),
        0,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        LPVOID lpMsgBuf = nullptr;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            error,
            0, // Default language
            (LPWSTR)&lpMsgBuf,
            0,
            NULL
        );

        // Display the error message in a message box
        MessageBox(NULL, (LPCTSTR)lpMsgBuf, L"Error", MB_OK | MB_ICONERROR);

        LocalFree(lpMsgBuf);
        exit(1);
    }
}

bool Pipeline::ReadFromPipe(std::vector<char>& buffer, DWORD& bytesRead) {
    buffer.resize(bufferSize - 1);
    if (!::ReadFile(hPipe, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, NULL)) {
        return false;
    }
    return true;
}


bool Pipeline::WriteResponse(const std::string& response) {
    DWORD bytesWritten;
    if (!::WriteFile(hPipe, response.c_str(), static_cast<DWORD>(response.size()), &bytesWritten, NULL)) {
        return false;
    }
    return true;
}

DWORD WINAPI Pipeline::RunServer(LPVOID lpParam) {
    Pipeline* pipeline = static_cast<Pipeline*>(lpParam);
    ClientAPI clientAPI;

    pipeline->StartServer();
    std::vector<char> buffer;
    DWORD bytesRead;

    while (true) {
        BOOL connected = ConnectNamedPipe(pipeline->hPipe, NULL);
        if (!connected) {
            if (GetLastError() == ERROR_PIPE_CONNECTED) {
                connected = TRUE;
            }
        }

        if (connected) {
            while (true) {
                if (pipeline->ReadFromPipe(buffer, bytesRead)) {
                    buffer[bytesRead] = '\0';
                    std::string instruction(buffer.data());
                    std::string response = clientAPI.ProcessInstruction(instruction);
                    pipeline->WriteResponse(response);
                }
                else {
                    break;
                }
            }
            DisconnectNamedPipe(pipeline->hPipe);
        }
    }

    return 0;
}


void Pipeline::DisconnectAndClose() {
    if (hPipe != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }
}
