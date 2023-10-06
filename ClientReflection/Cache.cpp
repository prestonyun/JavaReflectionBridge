#include "pch.h"
#include "Cache.hpp"
#include <cstdio>
#include <algorithm>
#include <iostream>

void Cache::addMethodToCache(jmethodID methodID, jobject methodObject, const std::string& name, const std::string& signature, const std::string& returnType, const std::string& className) {
    std::string key = className + "." + name;
    std::cout << "Key to be added: " << key << std::endl;
    Method method(methodID, methodObject, name, signature, returnType);
    methodCache[key] = method;
}

std::string Cache::replaceDotsWithSlashes(const std::string& input) {
    std::string output = input;
    std::replace(output.begin(), output.end(), '.', '/');
    return output;
}

std::string Cache::convertToSignature(JNIEnv* env, jobjectArray paramTypeArray) {
    std::ostringstream signature;
    signature << "(";
    jsize paramCount = env->GetArrayLength(paramTypeArray);
    for (jsize i = 0; i < paramCount; i++) {
        jobject paramTypeObject = env->GetObjectArrayElement(paramTypeArray, i);
        if (env->IsInstanceOf(paramTypeObject, env->FindClass("java/lang/Class"))) {
            jclass paramTypeClass = static_cast<jclass>(paramTypeObject);
            std::string paramTypeSignature = getClassSignature(env, paramTypeClass);
            paramTypeSignature = replaceDotsWithSlashes(paramTypeSignature);
            signature << paramTypeSignature;
        }
        env->DeleteLocalRef(paramTypeObject);  // Clean up the local reference
    }
    signature << ")";
    return signature.str();
}


void Cache::cacheObjectMethods(JNIEnv* env, jobject object) {
    jclass objectClass = env->GetObjectClass(object);
    if (objectClass == nullptr || env->ExceptionCheck()) {
        std::cout << "Failed to obtain object class" << std::endl;
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    
    jmethodID getNameMethod = env->GetMethodID(objectClass, "getName", "()Ljava/lang/String;");
    jstring classNameJava = (jstring)env->CallObjectMethod(object, getNameMethod);
    const char* classNameStr = env->GetStringUTFChars(classNameJava, 0);
    std::string className(classNameStr);
    std::cout << "Class name: " << className << std::endl;

    jclass classClass = env->FindClass("java/lang/Class");

    jmethodID getMethodsMethod = env->GetMethodID(classClass, "getMethods", "()[Ljava/lang/reflect/Method;");
    jobjectArray methodArray = (jobjectArray)env->CallObjectMethod(objectClass, getMethodsMethod);

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
        const char* nameStr = env->GetStringUTFChars(nameJavaStr, 0);
        std::string key = className + "." + nameStr;

        if (methodCache.find(key) != methodCache.end()) {
            // Clean up local references
            env->ReleaseStringUTFChars(nameJavaStr, nameStr);
            env->DeleteLocalRef(nameJavaStr);
            env->DeleteLocalRef(methodObject);
            env->DeleteLocalRef(methodClass);

            continue;  // Skip the rest of the processing for this method and move on to the next method
        }

        jmethodID getParameterTypesMethod = env->GetMethodID(methodClass, "getParameterTypes", "()[Ljava/lang/Class;");
        jobjectArray paramTypeArray = (jobjectArray)env->CallObjectMethod(methodObject, getParameterTypesMethod);

        jmethodID getReturnTypeMethod = env->GetMethodID(methodClass, "getReturnType", "()Ljava/lang/Class;");
        jobject returnTypeObject = env->CallObjectMethod(methodObject, getReturnTypeMethod);

        

        std::string signature = convertToSignature(env, paramTypeArray);
        std::string returnType = convertToReturnType(env, returnTypeObject);

        signature += returnType;

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
        std::cout << "Key: " << key << std::endl;

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
    std::cout << "Input: " << input << std::endl;
    size_t pos = 0, found;
    while ((found = input.find('.', pos)) != std::string::npos) {
        methods.push_back(input.substr(pos, found - pos));
        pos = found + 1;
    }
    methods.push_back(input.substr(pos));  // Push the last method

    std::string currentKey = methods[0]; 
    std::cout << "Current key: " << currentKey << std::endl;

    for (const auto& methodStr : methods) {
        if (methodStr == currentKey) continue;
        std::string methodName = methodStr.substr(0, methodStr.find('('));
        std::string key = currentKey + "." + methodName;
        std::cout << "Key: " << key << std::endl;
        auto it = methodCache.find(key);
        if (it == methodCache.end()) {
            printf("Method %s not found\n", key.c_str());
            return "";
        }
        Method& method = it->second;
        jobject currentObject = method.object;
        if (env->ExceptionOccurred()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            return "";
        }

        // Execute the Method
        try {
            if (method.return_type == "V") {  // void return type
                env->CallVoidMethod(currentObject, method.id);
                currentKey = "void";  // Since void methods don't return anything, you might want to set a default value
            }
            else if (method.return_type == "I") {  // int return type
                jint result = env->CallIntMethod(currentObject, method.id);
                currentKey = std::to_string(result);  // Convert jint to std::string and update currentKey
            }
            else if (method.return_type == "Ljava/lang/String;") {  // String return type
                jstring result = (jstring)env->CallObjectMethod(currentObject, method.id);
                const char* chars = env->GetStringUTFChars(result, nullptr);
                std::string result_str(chars);
                env->ReleaseStringUTFChars(result, chars);
                currentKey = result_str;  // Update currentKey with the result string
            }
            else if (method.return_type == "Z") {
                jstring result = (jstring)env->CallBooleanMethod(currentObject, method.id);
                const char* chars = env->GetStringUTFChars(result, nullptr);
                std::string result_str(chars);
                env->ReleaseStringUTFChars(result, chars);
                currentKey = result_str;  // Update currentKey with the result string
            }
            else {  // Other non-primitive types
                jobject result = env->CallObjectMethod(currentObject, method.id);
                if (env->ExceptionOccurred()) {
                    env->ExceptionDescribe();
                    env->ExceptionClear();
                    return "";
                }
                if (result != nullptr) {
                    jclass resultClass = env->GetObjectClass(result);
                    if (env->ExceptionOccurred()) {
                        env->ExceptionDescribe();
                        env->ExceptionClear();
                        return "";
                    }
                    jmethodID toStringMethod = env->GetMethodID(resultClass, "toString", "()Ljava/lang/String;");
                    if (env->ExceptionOccurred()) {
                        env->ExceptionDescribe();
                        env->ExceptionClear();
                        return "";
                    }
                    if (toStringMethod != nullptr) {
                        jstring resultStr = (jstring)env->CallObjectMethod(result, toStringMethod);
                        if (env->ExceptionOccurred()) {
                            env->ExceptionDescribe();
                            env->ExceptionClear();
                            return "";
                        }
                        const char* chars = env->GetStringUTFChars(resultStr, nullptr);
                        std::string result_str(chars);
                        try {
                            cacheObjectMethods(env, result);  // Cache the methods of the new object
                            }
                        catch (const std::exception& e) {
                            std::cerr << "Exception: " << e.what() << '\n';
                        }
                        env->ReleaseStringUTFChars(resultStr, chars);
                        return result_str;
                        
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << '\n';
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
