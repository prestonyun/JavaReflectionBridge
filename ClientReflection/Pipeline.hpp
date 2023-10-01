#include "pch.h"
#include <string>
#include <vector>
#include <Windows.h>
#include "ClientAPI.hpp"

class Pipeline {
public:
    Pipeline(const std::wstring& pipeName, size_t bufferSize);
    ~Pipeline();
    void StartServer();
    static DWORD WINAPI RunServer(LPVOID lpParam);
    bool ReadFromPipe(std::vector<char>& buffer, DWORD& bytesRead);
    bool WriteResponse(const std::string& response);
    void DisconnectAndClose();

private:
    HANDLE hPipe;
    std::wstring pipeName;
    size_t bufferSize;
    ClientAPI clientAPI;
};
