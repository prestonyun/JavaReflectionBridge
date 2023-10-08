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

    void PrintClasses() const noexcept;
    bool Initialize() noexcept;
    bool IsDecendentOf(jobject object, const char* className) const noexcept;
    std::string GetClassName(jobject object) const noexcept;

    bool AttachToThread(JNIEnv** Thread);
    bool DetachThread(JNIEnv** Thread);
    jobject getClient();
    Cache* cache;

private:
    JavaVM* jvm;
    JNIEnv* env;

    jobject injector;
    jobject client;
    jobject frame;
    jobject applet;
    jobject classLoader;

    template<typename T>
    auto make_safe_local(auto object) const noexcept
    {
        auto deleter = [&](T ptr) {
            if (jvm && ptr)
            {
                env->DeleteLocalRef(static_cast<jobject>(ptr));
            }
        };

        return std::unique_ptr<typename std::remove_pointer<T>::type, decltype(deleter)>{static_cast<T>(object), deleter};
    }
};
