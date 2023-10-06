#include "pch.h"
#include "ClientAPI.hpp"
#include <iostream>

void DisplayErrorMessage(const std::wstring& message) {
    MessageBoxW(NULL, message.c_str(), L"Error", MB_OK | MB_ICONERROR);
}

bool checkAndClearException(JNIEnv* env) {
    // Check if an exception occurred
    if (env->ExceptionCheck()) {
        // Get the exception object
        jthrowable exception = env->ExceptionOccurred();

        // Clear the exception to be able to call further JNI methods
        env->ExceptionClear();

        // Retrieve the toString() representation of the exception
        jclass throwableClass = env->FindClass("java/lang/Throwable");
        jmethodID toStringMethod = env->GetMethodID(throwableClass, "toString", "()Ljava/lang/String;");
        jstring exceptionString = (jstring)env->CallObjectMethod(exception, toStringMethod);
        const char* exceptionCString = env->GetStringUTFChars(exceptionString, JNI_FALSE);

        // Display the exception message
        std::wstring message = L"JNI Exception: " + std::wstring(exceptionCString, exceptionCString + strlen(exceptionCString));
        MessageBox(NULL, message.c_str(), L"JNI Exception", MB_OK | MB_ICONERROR);

        // Clean up local references and release string memory
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
    if (checkAndClearException(env)) {
        MessageBoxW(NULL, L"Failed to find injector field", L"Error", MB_OK | MB_ICONERROR);
        return nullptr;
    }

    jobject injector = env->GetStaticObjectField(runeLiteClass, injectorField);
    if (checkAndClearException(env)) {
        MessageBoxW(NULL, L"Failed to find injector object field", L"Error", MB_OK | MB_ICONERROR);
        return nullptr;
    }

    this->injector = injector;
    jclass injectorClass = this->cache->getClass(env, "InjectorClass", injector);
    if (env->ExceptionCheck()) {
        MessageBoxW(NULL, L"Failed to find injector class", L"Error", MB_OK | MB_ICONERROR);
        env->ExceptionClear();
        return nullptr;
    }

    jmethodID getInstanceMethod = env->GetMethodID(injectorClass, "getInstance", "(Ljava/lang/Class;)Ljava/lang/Object;");
    if (env->ExceptionCheck()) {
        MessageBoxW(NULL, L"Failed to find injector instance", L"Error", MB_OK | MB_ICONERROR);
        env->ExceptionClear();
        return nullptr;
    }

    jobject runeLiteClient = env->CallObjectMethod(injector, getInstanceMethod, runeLiteClass);
    if (env->ExceptionCheck()) {
        MessageBoxW(NULL, L"Failed to call injector method", L"Error", MB_OK | MB_ICONERROR);
        env->ExceptionClear();
        return nullptr;
    }
    jclass runeLiteClientClass = env->GetObjectClass(runeLiteClient);
    if (env->ExceptionCheck()) {
        MessageBoxW(NULL, L"Failed to find client class", L"Error", MB_OK | MB_ICONERROR);
        env->ExceptionClear();
        return nullptr;
    }

    jfieldID clientField = env->GetFieldID(runeLiteClientClass, "client", "Lnet/runelite/api/Client;");
    if (env->ExceptionCheck()) {
        MessageBoxW(NULL, L"Failed to find client field", L"Error", MB_OK | MB_ICONERROR);
        env->ExceptionClear();
        return nullptr;
    }

    jobject client = env->GetObjectField(runeLiteClient, clientField);
    if (env->ExceptionCheck()) {
        MessageBoxW(NULL, L"Failed to find client object field", L"Error", MB_OK | MB_ICONERROR);
        env->ExceptionClear();
        return nullptr;
    }

    jclass clientClass = this->cache->getClass(env, "ClientClass", client);
    if (env->ExceptionCheck()) {
        MessageBoxW(NULL, L"Failed to find client object class", L"Error", MB_OK | MB_ICONERROR);
        env->ExceptionClear();
        return nullptr;
    }
    checkAndClearException(env);
    this->client = env->NewGlobalRef(client);
    this->cache->cacheObjectMethods(env, client);
    return client;
}

std::string ClientAPI::ProcessInstruction(const std::string& instruction) {
    if (this->client == nullptr) {
        getClient();
    }
    if (!this->env || !this->client) {
        MessageBoxW(NULL, L"Invalid state: no env or client", L"Error", MB_OK | MB_ICONERROR);
        return "";
    }
    

    std::cout << "Total number of methods in methodCache: " << this->cache->methodCache.size() << std::endl;

    
    try {
        std::string response = this->cache->executeMethod(env, instruction);
        return response;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
    }

    //std::cout << "Response: " << response << std::endl;
    return "";
}

