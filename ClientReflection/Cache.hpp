#pragma once
#include "pch.h"
#include <jni.h>
#include <unordered_map>
#include <string>
#include <sstream>

class Cache {
public:
    // Struct to hold method information.
    struct Method {
        jmethodID id;
        jobject object;
        std::string name;
        std::string signature;
        std::string return_type;

        Method() : id(nullptr), object(nullptr) {}
        Method(jmethodID id, jobject object, const std::string& name, const std::string& signature, const std::string& return_type)
            : id(id), object(object), name(name), signature(signature), return_type(return_type) {}
    };

    jclass getClass(JNIEnv* env, const std::string& name, jobject object);
    jobject getObject(JNIEnv* env, const std::string& key, jclass clazz, const char* name, const char* sig);
    jfieldID getFieldID(JNIEnv* env, const std::string& key, jclass clazz, const char* name, const char* sig);

    void cacheObjectMethods(JNIEnv* env, jobject object, const std::string& className);
    std::string convertToSignature(JNIEnv* env, jobjectArray paramTypeArray);
    std::string getClassSignature(JNIEnv* env, jclass clazz);
    std::string convertToReturnType(JNIEnv* env, jobject returnTypeObject);
    std::string executeSingleMethod(JNIEnv* env, const std::string& input);
    std::string executeMethod(JNIEnv* env, const std::string& input);

    void cleanup(JNIEnv* env);

    Cache() = default;
    ~Cache();

    std::unordered_map<std::string, Method> methodCache;
    std::unordered_map<std::string, jclass> classCache;
    std::unordered_map<std::string, jobject> objectCache;
    std::unordered_map<std::string, jfieldID> fieldCache;

};
