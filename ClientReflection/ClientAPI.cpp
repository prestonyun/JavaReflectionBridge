#include "pch.h"
#include "ClientAPI.hpp"
#include <iostream>

void DisplayErrorMessage(const std::wstring& message) {
    MessageBoxW(NULL, message.c_str(), L"Error", MB_OK | MB_ICONERROR);
}

bool checkAndClearException(JNIEnv* env) {
    if (env->ExceptionCheck()) {
        jthrowable exception = env->ExceptionOccurred();
        env->ExceptionClear();

        jclass throwableClass = env->FindClass("java/lang/Throwable");
        jmethodID toStringMethod = env->GetMethodID(throwableClass, "toString", "()Ljava/lang/String;");
        jstring exceptionString = (jstring)env->CallObjectMethod(exception, toStringMethod);
        const char* exceptionCString = env->GetStringUTFChars(exceptionString, JNI_FALSE);
        std::wstring message = L"JNI Exception: " + std::wstring(exceptionCString, exceptionCString + strlen(exceptionCString));
        DisplayErrorMessage(message);

        env->ReleaseStringUTFChars(exceptionString, exceptionCString);
        env->DeleteLocalRef(exceptionString);
        env->DeleteLocalRef(throwableClass);
        return true;
    }
    return false;
}

ClientAPI::ClientAPI() {
    jvm = nullptr;
    env = nullptr;

    injector = nullptr;
    client = nullptr;
    cache = new Cache();

    jsize nVMs;
    jint ret = JNI_GetCreatedJavaVMs(&jvm, 1, &nVMs);
    if (ret != JNI_OK || nVMs == 0) {
        DisplayErrorMessage(L"No JVM found");
        exit(1);
    }
    ret = jvm->AttachCurrentThread((void**)&env, NULL);
    if (ret != JNI_OK) {
        DisplayErrorMessage(L"Failed to attach to JVM");
        exit(1);
    }

}

bool ClientAPI::AttachToThread(JNIEnv** Thread)
{
    if (this->jvm)
        if (this->jvm->GetEnv((void**)Thread, JNI_VERSION_1_6) == JNI_EDETACHED)
            this->jvm->AttachCurrentThread((void**)Thread, nullptr);
    return (*Thread);
}

bool ClientAPI::DetachThread(JNIEnv** Thread)
{
    if (*Thread)
        if (this->jvm)
            if (this->jvm->DetachCurrentThread() == JNI_OK)
                *Thread = nullptr;
    return !(*Thread);
}


jobject ClientAPI::getClient() {
    jclass runeLiteClass = env->FindClass("net/runelite/client/RuneLite");
    if (checkAndClearException(env)) {
        MessageBoxW(NULL, L"Failed to find RuneLite class", L"Error", MB_OK | MB_ICONERROR);
		return nullptr;
	}

    jfieldID injectorField = env->GetStaticFieldID(runeLiteClass, "injector", "Lcom/google/inject/Injector;");
    checkAndClearException(env);
    jobject injector = env->GetStaticObjectField(runeLiteClass, injectorField);
    checkAndClearException(env);
    this->injector = injector;
    //this->cache->cacheObjectMethods(env, injector);
    jclass injectorClass = this->cache->getClass(env, "InjectorClass", injector);
    checkAndClearException(env);
    jmethodID getInstanceMethod = env->GetMethodID(injectorClass, "getInstance", "(Ljava/lang/Class;)Ljava/lang/Object;");
    checkAndClearException(env);
    jobject runeLiteClient = env->CallObjectMethod(injector, getInstanceMethod, runeLiteClass);
    checkAndClearException(env);
    jclass runeLiteClientClass = env->GetObjectClass(runeLiteClient);
    checkAndClearException(env);
    jfieldID clientField = env->GetFieldID(runeLiteClientClass, "client", "Lnet/runelite/api/Client;");
    checkAndClearException(env);
    jobject client = env->GetObjectField(runeLiteClient, clientField);
    checkAndClearException(env);
    jclass clientClass = this->cache->getClass(env, "ClientClass", client);
    checkAndClearException(env);
    this->client = env->NewGlobalRef(client);
    this->cache->cacheObjectMethods(env, client);
    return client;
}

std::string ClientAPI::ProcessInstruction(const std::string& instruction) {
    if (this->client == nullptr && !getClient()) {
        DisplayErrorMessage(L"Invalid state: no client");
        return "";
    }
    if (!this->env) {
        DisplayErrorMessage(L"Invalid state: no env");
        return "";
    }

    std::cout << "Total number of methods in methodCache: " << this->cache->methodCache.size() << std::endl;

    try {
        return this->cache->executeMethod(env, instruction);
        checkAndClearException(env);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
    }
    if (this->injector != nullptr) {
        std::cout << "caching injector methods" << std::endl;
        this->cache->cacheObjectMethods(env, this->injector);
    }
    

    return "";
}
