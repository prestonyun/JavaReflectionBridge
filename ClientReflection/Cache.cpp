#include "pch.h"
#include "Cache.hpp"
#include <cstdio>
#include <algorithm>

void Cache::cacheObjectMethods(JNIEnv* env, jobject object, const std::string& className) {
    jclass objectClass = env->GetObjectClass(object);
    if (objectClass == nullptr || env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jclass classClass = env->FindClass("java/lang/Class");
    jmethodID getDeclaredMethodsMethod = env->GetMethodID(classClass, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
    jobjectArray methodArray = (jobjectArray)env->CallObjectMethod(objectClass, getDeclaredMethodsMethod);

    jsize methodCount = env->GetArrayLength(methodArray);

    for (jsize i = 0; i < methodCount; i++) {
        jobject methodObject = env->GetObjectArrayElement(methodArray, i);
        if (methodObject == nullptr) {
            fprintf(stderr, "Failed to obtain method object at index %d\n", i);
            continue;
        }


        jclass methodClass = env->GetObjectClass(methodObject);
        if (methodClass == nullptr || env->ExceptionCheck()) {
            fprintf(stderr, "Failed to obtain method class at index %d\n", i);
            env->ExceptionClear();
            continue;
        }
        jmethodID getNameMethod = env->GetMethodID(methodClass, "getName", "()Ljava/lang/String;");
        jstring nameJavaStr = (jstring)env->CallObjectMethod(methodObject, getNameMethod);

        jmethodID getParameterTypesMethod = env->GetMethodID(methodClass, "getParameterTypes", "()[Ljava/lang/Class;");
        jobjectArray paramTypeArray = (jobjectArray)env->CallObjectMethod(methodObject, getParameterTypesMethod);

        jmethodID getReturnTypeMethod = env->GetMethodID(methodClass, "getReturnType", "()Ljava/lang/Class;");
        jobject returnTypeObject = env->CallObjectMethod(methodObject, getReturnTypeMethod);

        const char* nameStr = env->GetStringUTFChars(nameJavaStr, 0);

        std::string signature = convertToSignature(env, paramTypeArray);
        std::string returnType = convertToReturnType(env, returnTypeObject);

        if (returnType[0] == 'L') {  // It's an object type
            std::string objectClassName = returnType.substr(1, returnType.length() - 2);  // Remove 'L' and ';'
            std::replace(objectClassName.begin(), objectClassName.end(), '/', '.');  // Convert slashes to dots
            if (classCache.find(objectClassName) == classCache.end()) {  // If not already cached
                jclass objectClass = env->FindClass(objectClassName.c_str());
                cacheObjectMethods(env, env->NewGlobalRef(object), objectClassName);  // Recursive caching
            }
        }

        signature += returnType;

        std::string key = className + "." + nameStr;

        jmethodID methodExists = env->GetMethodID(objectClass, nameStr, signature.c_str());
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            methodExists = env->GetStaticMethodID(objectClass, nameStr, signature.c_str());
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                fprintf(stderr, "Method %s.%s with signature %s does not exist or is not accessible\n",
                    className.c_str(), nameStr, signature.c_str());
                continue;
            }
        }

        // If we reach here, the method exists and is accessible. Now get its ID.
        jmethodID methodID = methodExists;

        Method method(methodID, methodObject, nameStr, signature, returnType);
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

std::string replaceDotsWithSlashes(const std::string& input) {
    std::string output = input;
    for (size_t pos = 0; pos < output.length(); ++pos) {
        if (output[pos] == '.') {
            output[pos] = '/';
        }
    }
    return output;
}


std::string Cache::convertToSignature(JNIEnv* env, jobjectArray paramTypeArray) {
    std::ostringstream signature;
    signature << "(";
    jsize paramCount = env->GetArrayLength(paramTypeArray);
    for (jsize i = 0; i < paramCount; i++) {
        jobject paramTypeObject = env->GetObjectArrayElement(paramTypeArray, i);
        jclass paramTypeClass = static_cast<jclass>(paramTypeObject);
        std::string paramTypeSignature = getClassSignature(env, paramTypeClass);
        paramTypeSignature = replaceDotsWithSlashes(paramTypeSignature);
        signature << paramTypeSignature;
        env->DeleteLocalRef(paramTypeObject);  // Clean up the local reference
    }
    signature << ")";
    return signature.str();
}

std::string Cache::convertToReturnType(JNIEnv* env, jobject returnTypeObject) {
    jclass returnTypeClass = static_cast<jclass>(returnTypeObject);
    std::string returnTypeSignature = getClassSignature(env, returnTypeClass);
    returnTypeSignature = replaceDotsWithSlashes(returnTypeSignature);
    return returnTypeSignature;
}


std::string Cache::getClassSignature(JNIEnv* env, jclass clazz) {
    jclass classClass = env->GetObjectClass(clazz);
    jmethodID getNameMethod = env->GetMethodID(classClass, "getName", "()Ljava/lang/String;");
    jstring nameJavaStr = (jstring)env->CallObjectMethod(clazz, getNameMethod);
    const char* nameStr = env->GetStringUTFChars(nameJavaStr, 0);
    std::string nameStrCpp(nameStr);
    std::replace(nameStrCpp.begin(), nameStrCpp.end(), '.', '/');
    std::string signature;
    if (strcmp(nameStr, "int") == 0) {
        signature = "I";
    }
    else if (strcmp(nameStr, "long") == 0) {
        signature = "J";
    }
    else if (strcmp(nameStr, "boolean") == 0) {
        signature = "Z";
    }
    else if (strcmp(nameStr, "void") == 0) {
        signature = "V";
    }
    else if (strcmp(nameStr, "double") == 0) {
        signature = "D";
    }
    else if (strcmp(nameStr, "byte") == 0) {
        signature = "B";
    }
    else if (strcmp(nameStr, "short") == 0) {
        signature = "S";
    }
    else if (strcmp(nameStr, "char") == 0) {
		signature = "C";
	}
    else if (strcmp(nameStr, "float") == 0) {
		signature = "F";
	}
    else if (nameStr[0] == '[') {
        // Preserve the original array signature as it's already in JNI format
        signature = std::string(nameStr);
    }
    else {
        // Assume object type
        signature = "L" + nameStrCpp + ";";
    }
    env->ReleaseStringUTFChars(nameJavaStr, nameStr);
    env->DeleteLocalRef(nameJavaStr);
    env->DeleteLocalRef(classClass);
    return signature;
}

std::string Cache::executeSingleMethod(JNIEnv* env, const std::string& input) {
    // 1. Parse the input string to get the class and method name
    std::string::size_type dot_pos = input.find('.');
    if (dot_pos == std::string::npos) {
        throw std::runtime_error("Invalid input string format");
    }
    std::string class_name = input.substr(0, dot_pos);
    std::string method_name = input.substr(dot_pos + 1, input.find('(') - dot_pos - 1);

    // Construct the key
    std::string key = class_name + "." + method_name;

    // 2. Locate the Method
    auto it = methodCache.find(key);
    if (it == methodCache.end()) {
        printf("Method %s not found\n", key.c_str());
        return "";
    }
    Method& method = it->second;

    // 3. Execute the Method
    if (method.return_type == "V") {  // void return type
        env->CallVoidMethod(method.object, method.id);
    }
    else if (method.return_type == "I") {  // int return type
        jint result = env->CallIntMethod(method.object, method.id);
        return std::to_string(result);
    }
    else if (method.return_type == "Ljava/lang/String;") {  // String return type
        jstring result = (jstring)env->CallObjectMethod(method.object, method.id);
        const char* chars = env->GetStringUTFChars(result, nullptr);
        std::string result_str(chars);
        env->ReleaseStringUTFChars(result, chars);
        return result_str;
    }
    else {  // Other non-primitive types
        jobject result = env->CallObjectMethod(method.object, method.id);
        if (result != nullptr) {
            printf("Result is not null\n");
            jclass resultClass = env->GetObjectClass(result);
            jmethodID toStringMethod = env->GetMethodID(resultClass, "toString", "()Ljava/lang/String;");
            if (toStringMethod != nullptr) {
                jstring resultStr = (jstring)env->CallObjectMethod(result, toStringMethod);
                const char* chars = env->GetStringUTFChars(resultStr, nullptr);
                std::string result_str(chars);
                env->ReleaseStringUTFChars(resultStr, chars);
                return result_str;
            }
        }
    }
    // ... Handle other return types similarly

    return "";
}

std::string Cache::executeMethod(JNIEnv* env, const std::string& input) {
    // Split the method chain based on '.' to get individual method calls
    std::vector<std::string> methods;
    size_t pos = 0, found;
    while ((found = input.find('.', pos)) != std::string::npos) {
        methods.push_back(input.substr(pos, found - pos));
        pos = found + 1;
    }
    methods.push_back(input.substr(pos));  // Push the last method

    std::string currentKey = methods[0].substr(0, methods[0].find('('));

    for (const auto& methodStr : methods) {
        std::string methodName = methodStr.substr(0, methodStr.find('('));
        std::string key = currentKey + "." + methodName;

        auto it = methodCache.find(key);
        if (it == methodCache.end()) {
            printf("Method %s not found\n", key.c_str());
            return "";
        }
        Method& method = it->second;
        jobject currentObject = method.object;  // Get the object from the method

        // Execute the Method
        if (method.return_type == "V") {  // void return type
            env->CallVoidMethod(currentObject, method.id);
        }
        else if (method.return_type == "I") {  // int return type
            jint result = env->CallIntMethod(currentObject, method.id);
            // ... Handle integer result if necessary ...
        }
        else if (method.return_type == "Ljava/lang/String;") {  // String return type
            jstring result = (jstring)env->CallObjectMethod(currentObject, method.id);
            const char* chars = env->GetStringUTFChars(result, nullptr);
            std::string result_str(chars);
            env->ReleaseStringUTFChars(result, chars);
            // ... Handle string result if necessary ...
        }
        else {  // Other non-primitive types
            jobject result = env->CallObjectMethod(currentObject, method.id);
            if (result != nullptr) {
                jclass resultClass = env->GetObjectClass(result);
                jmethodID toStringMethod = env->GetMethodID(resultClass, "toString", "()Ljava/lang/String;");
                if (toStringMethod != nullptr) {
                    jstring resultStr = (jstring)env->CallObjectMethod(result, toStringMethod);
                    const char* chars = env->GetStringUTFChars(resultStr, nullptr);
                    std::string result_str(chars);
                    env->ReleaseStringUTFChars(resultStr, chars);
                    // Update currentKey for the next method in the chain
                    currentKey = result_str;
                    // Cache the new method details in methodCache
                    std::string newKey = currentKey + "." + methods.back().substr(0, methods.back().find('('));
                    Method newMethod(method.id, result, methodName, method.signature, method.return_type);
                    methodCache[newKey] = newMethod;
                }
            }
        }
    }

    return currentKey;  // The result of the last method in the chain
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
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
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
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    fieldCache[key] = fieldID;
    return fieldID;
}

void Cache::cleanup(JNIEnv* env) {
    // Iterate through each Method in the methodCache and delete the global reference to the jobject
    for (auto& entry : methodCache) {
        Method& method = entry.second;
        if (method.object != nullptr) {
            env->DeleteGlobalRef(method.object);
        }
    }

    for (auto& entry : classCache) {
        jclass clazz = entry.second;
        env->DeleteGlobalRef(clazz);
    }

    for (auto& entry : objectCache) {
        jobject object = entry.second;
        env->DeleteGlobalRef(object);
    }
}

Cache::~Cache() {
}
