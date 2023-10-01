#include "pch.h"
#include "Cache.hpp"
#include <cstdio>

void Cache::cacheObjectMethods(JNIEnv* env, jobject object, const std::string& className) {
    jclass objectClass = env->GetObjectClass(object);

    jclass classClass = env->FindClass("java/lang/Class");
    jmethodID getDeclaredMethodsMethod = env->GetMethodID(classClass, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
    jobjectArray methodArray = (jobjectArray)env->CallObjectMethod(objectClass, getDeclaredMethodsMethod);

    jsize methodCount = env->GetArrayLength(methodArray);

    for (jsize i = 0; i < methodCount; i++) {
        jobject methodObject = env->GetObjectArrayElement(methodArray, i);

        jclass methodClass = env->GetObjectClass(methodObject);
        jmethodID getNameMethod = env->GetMethodID(methodClass, "getName", "()Ljava/lang/String;");
        jstring nameJavaStr = (jstring)env->CallObjectMethod(methodObject, getNameMethod);

        jmethodID getParameterTypesMethod = env->GetMethodID(methodClass, "getParameterTypes", "()[Ljava/lang/Class;");
        jobjectArray paramTypeArray = (jobjectArray)env->CallObjectMethod(methodObject, getParameterTypesMethod);

        jmethodID getReturnTypeMethod = env->GetMethodID(methodClass, "getReturnType", "()Ljava/lang/Class;");
        jobject returnTypeObject = env->CallObjectMethod(methodObject, getReturnTypeMethod);

        const char* nameStr = env->GetStringUTFChars(nameJavaStr, 0);

        // Here, you would need to convert paramTypeArray and returnTypeObject to the desired signature and return type string format
        std::string signature = convertToSignature(env, paramTypeArray);
        std::string returnType = convertToReturnType(env, returnTypeObject);

        std::string key = className + "::" + nameStr;

        jmethodID methodID = env->GetMethodID(objectClass, nameStr, signature.c_str());
        Method method(methodID, nameStr, signature, returnType);
        methodCache[key] = method;

        // Clean up local references
        env->ReleaseStringUTFChars(nameJavaStr, nameStr);
        env->DeleteLocalRef(nameJavaStr);
        env->DeleteLocalRef(methodObject);
        env->DeleteLocalRef(methodClass);
        env->DeleteLocalRef(paramTypeArray);
        env->DeleteLocalRef(returnTypeObject);
    }

    // Clean up
    env->DeleteLocalRef(classClass);
    env->DeleteLocalRef(objectClass);
    env->DeleteLocalRef(methodArray);
}

std::string Cache::convertToSignature(JNIEnv* env, jobjectArray paramTypeArray) {
    std::ostringstream signature;
    signature << "(";
    jsize paramCount = env->GetArrayLength(paramTypeArray);
    for (jsize i = 0; i < paramCount; i++) {
        jobject paramTypeObject = env->GetObjectArrayElement(paramTypeArray, i);
        jclass paramTypeClass = static_cast<jclass>(paramTypeObject);
        std::string paramTypeSignature = getClassSignature(env, paramTypeClass);
        signature << paramTypeSignature;
        env->DeleteLocalRef(paramTypeObject);  // Clean up the local reference
    }
    signature << ")";
    return signature.str();
}

std::string Cache::getClassSignature(JNIEnv* env, jclass clazz) {
    jclass classClass = env->GetObjectClass(clazz);
    jmethodID getNameMethod = env->GetMethodID(classClass, "getName", "()Ljava/lang/String;");
    jstring nameJavaStr = (jstring)env->CallObjectMethod(clazz, getNameMethod);
    const char* nameStr = env->GetStringUTFChars(nameJavaStr, 0);
    std::string signature;
    if (strcmp(nameStr, "int") == 0) {
        signature = "I";
    }
    else if (strcmp(nameStr, "long") == 0) {
        signature = "J";
    } // ... and so on for other primitive types
    else {
        signature = "L" + std::string(nameStr) + ";";
    }
    env->ReleaseStringUTFChars(nameJavaStr, nameStr);
    env->DeleteLocalRef(nameJavaStr);
    env->DeleteLocalRef(classClass);
    return signature;
}


std::string Cache::convertToReturnType(JNIEnv* env, jobject returnTypeObject) {
    jclass returnTypeClass = static_cast<jclass>(returnTypeObject);
    std::string returnTypeSignature = getClassSignature(env, returnTypeClass);
    return returnTypeSignature;
}



Cache::Method Cache::getMethod(JNIEnv* env, const std::string& key, jclass clazz, const char* name, const char* sig, const char* ret_type) {
    auto it = methodCache.find(key);
    if (it != methodCache.end()) {
        return it->second;
    }

    jmethodID methodID = env->GetMethodID(clazz, name, sig);
    if (env->ExceptionCheck()) {
        jthrowable exception = env->ExceptionOccurred();
        env->ExceptionClear();
        return { nullptr, "", "", ""};
    }

    // temp:
    return { nullptr, "", "", "" };

    //Method method(methodID, sig, ret_type);
    //methodCache[key] = method;

    //printf("Key: %s\n Signature: %s\n Return Type: %s", key.c_str(), sig ? sig : "null", ret_type ? ret_type : "null");
    //return method;
}

jclass Cache::getClass(JNIEnv* env, const std::string& name, jobject object) {
    auto it = classCache.find(name);
    if (it != classCache.end()) {
        return it->second;
    }

    jclass cls = env->GetObjectClass(object);
    classCache[name] = cls;
    return cls;
}

jobject Cache::getObject(JNIEnv* env, const std::string& key, jclass clazz, const char* name, const char* sig) {
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

jfieldID Cache::getFieldID(JNIEnv* env, const std::string& key, jclass clazz, const char* name, const char* sig) {
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

Cache::~Cache() {
}
