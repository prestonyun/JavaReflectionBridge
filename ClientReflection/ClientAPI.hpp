#pragma once
#include "pch.h"
#include <string>
#include <sstream>
#include <jni.h>
#include <vector>
#include <unordered_map>
#include "Cache.hpp"

typedef int (*ptr_GCJavaVMs)(JavaVM** vmBuf, jsize bufLen, jsize* nVMs);
typedef jobject(JNICALL* ptr_GetComponent)(JNIEnv* env, void* platformInfo);

struct AWTRectangle
{
    int x, y, width, height;
};

class ClientAPI {
public:
    ClientAPI();
    std::string ProcessInstruction(const std::string& instruction);
    bool AttachToThread(JNIEnv** Thread);
    bool DetachThread(JNIEnv** Thread);
    jobject getClient();
    jobject GrabCanvas();
    HWND GetCanvasHWND();
    HWND FindWindowWithClassName(const std::vector<HWND>& windows, const wchar_t* className);
    HWND FindWindowWithTitle(const std::vector<HWND>& windows, const wchar_t* windowTitle);
    // Initialize cache in JavaAPI constructor
    Cache* cache;
    ptr_GetComponent GetComponent;
    HWND clientHWND;
    jobject canvas;

private:
    JavaVM* jvm;
    JNIEnv* env;

    jobject injector;
    jobject client;
};
