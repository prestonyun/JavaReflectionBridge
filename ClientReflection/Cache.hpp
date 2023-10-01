// JniCache.hpp
#pragma once
#include "pch.h"
#include <jni.h>
#include <unordered_map>
#include <string>

class Cache {
public:
    // Struct to hold method information.
    struct Method {
        jmethodID id;
        std::string signature;

        Method() : id(nullptr), signature("") {} // default constructor
        Method(jmethodID id, const std::string& sig) : id(id), signature(sig) {} // parameterized constructor
    };

    // Cache for method IDs, using className::methodName as the key.
    std::unordered_map<std::string, Method> methodCache;
    // Cache for class objects, using className as the key.
    std::unordered_map<std::string, jclass> classCache;
    std::unordered_map<std::string, jobject> objectCache;
    std::unordered_map<std::string, jfieldID> fieldCache;

    // Method to get a Method struct from the cache, or find and add it to the cache.
    Method getMethod(JNIEnv* env, const std::string& key, jclass clazz, const std::string name, const std::string sig) {
        auto it = methodCache.find(key);
        if (it != methodCache.end()) {
            return it->second;
        }

        jmethodID methodID = env->GetMethodID(clazz, name.c_str(), sig.c_str());
        if (env->ExceptionCheck()) {
            // Get the exception object
            jthrowable exception = env->ExceptionOccurred();

            // Clear the exception to be able to call further JNI methods
            env->ExceptionClear();
            return { nullptr, "" };
        }

        Method method(methodID, sig);
        methodCache[key] = method;
        return method;
    }

    // Method to get a class object from the cache, or find and add it to the cache.
    jclass getClass(JNIEnv* env, const std::string& name, jobject object) {
        auto it = classCache.find(name);
        if (it != classCache.end()) {
            return it->second;
        }

        jclass cls = env->GetObjectClass(object);
        classCache[name] = cls;
        return cls;
    }

    // Method to get a jobject from the cache, or find and add it to the cache.
    jobject getObject(JNIEnv* env, const std::string& key, jclass clazz, const char* name, const char* sig) {
        auto it = objectCache.find(key);
        if (it != objectCache.end()) {
            return it->second;
        }

        jobject object = env->GetStaticObjectField(clazz, env->GetStaticFieldID(clazz, name, sig));
        if (env->ExceptionCheck()) {
            // Get the exception object
            jthrowable exception = env->ExceptionOccurred();

            // Clear the exception to be able to call further JNI methods
            env->ExceptionClear();
            return nullptr; // or handle the error as appropriate
        }

        objectCache[key] = object;
        return object;
    }

    // Method to get a jfieldID from the cache, or find and add it to the cache.
    jfieldID getFieldID(JNIEnv* env, const std::string& key, jclass clazz, const char* name, const char* sig) {
        auto it = fieldCache.find(key);
        if (it != fieldCache.end()) {
            return it->second;
        }

        jfieldID fieldID = env->GetFieldID(clazz, name, sig);
        if (env->ExceptionCheck()) {
            // Get the exception object
            jthrowable exception = env->ExceptionOccurred();

            // Clear the exception to be able to call further JNI methods
            env->ExceptionClear();
            return nullptr; // or handle the error as appropriate
        }

        fieldCache[key] = fieldID;
        return fieldID;
    }

    // Constructor, can be used to preload some classes into the cache.
    Cache() = default;

    // Destructor for cleanup.
    ~Cache() {
        // Handle the cleanup, if needed.
        // Remember to delete global references, if any.
    }
};
