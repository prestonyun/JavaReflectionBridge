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
    // Initialize cache in JavaAPI constructor
    Cache* cache;

private:
    JavaVM* jvm;
    JNIEnv* env;

    jobject injector;
    jobject client;
};
