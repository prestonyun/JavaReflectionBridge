#include "pch.h"
#include "ClientAPI.hpp"
#include <iostream>
#include <utility>
#include <type_traits>
#include <memory>
#include <functional>

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

    applet = nullptr;
    classLoader = nullptr;
    frame = nullptr;

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
    this->injector = env->NewGlobalRef(injector);
    this->cache->cacheObjectMethods(env, injector);
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

bool ClientAPI::IsDecendentOf(jobject object, const char* className) const noexcept
{
    auto cls = make_safe_local<jclass>(env->FindClass(className));
    return env->IsInstanceOf(object, cls.get());
}

bool ClientAPI::Initialize() noexcept
{
    this->frame = env->NewGlobalRef(env->FindClass("java/awt/Window"));
    if (!this->frame)
    {
		std::cout << "Failed to find frame" << std::endl;
		return false;
	}
    using Result = std::unique_ptr<typename std::remove_pointer<jobject>::type, std::function<void(jobject)>>;
    std::function<Result(jobject)> findApplet = [&](jobject component) -> Result {
        if (component && this->IsDecendentOf(component, "java/awt/Container"))
        {
            auto cls = make_safe_local<jclass>(env->GetObjectClass(component));
            jmethodID mid = env->GetMethodID(cls.get(), "getComponents", "()[Ljava/awt/Component;");

            if (mid)
            {
                auto components = make_safe_local<jobjectArray>(env->CallObjectMethod(component, mid));
                jint len = env->GetArrayLength(components.get());

                for (jint i = 0; i < len; ++i)
                {
                    auto component = make_safe_local<jobject>(env->GetObjectArrayElement(components.get(), i));
                    auto applet_class = make_safe_local<jclass>(env->FindClass("java/applet/Applet"));
                    if (env->IsInstanceOf(component.get(), applet_class.get()))
                    {
                        std::cout << "Found applet component" << std::endl;
                        return component;
                    }

                    auto result = findApplet(component.get());
                    if (result)
                    {
                        std::cout << "Found applet result" << std::endl;
                        return result;
                    }
                }
            }
            else {
				std::cout << "Failed to find components" << std::endl;
			}
        }

        return {};
    };

    std::function<Result(jobject)> findCanvas = [&](jobject component) -> Result {
        if (component && this->IsDecendentOf(component, "java/awt/Container"))
        {
            std::cout << "Found container" << std::endl;
            auto cls = make_safe_local<jclass>(env->GetObjectClass(component));
            jmethodID mid = env->GetMethodID(cls.get(), "getComponents", "()[Ljava/awt/Component;");

            if (mid)
            {
                auto components = make_safe_local<jobjectArray>(env->CallObjectMethod(component, mid));
                jint len = env->GetArrayLength(components.get());
                std::cout << "iterating components" << std::endl;
                for (jint i = 0; i < len; ++i)
                {
                    //Some java.awt.Panel.
                    auto component = make_safe_local<jobject>(env->GetObjectArrayElement(components.get(), i));
                    auto canvas_class = make_safe_local<jclass>(env->FindClass("java/awt/Canvas"));
                    if (env->IsInstanceOf(component.get(), canvas_class.get()))
                    {
                        return component;
                    }

                    auto result = findCanvas(component.get());
                    if (result)
                    {
                        return result;
                    }
                }
            }
            else {
                std::cout << "Failed to find components" << std::endl;
            }
        }
        std::cout << "component not descendent or is null" << std::endl;
        return {};
    };

    std::function<Result(jobject)> getParent = [&](jobject component) -> Result {
        if (component)
        {
            auto cls = make_safe_local<jclass>(env->GetObjectClass(component));
            if (cls)
            {
                jmethodID mid = env->GetMethodID(cls.get(), "getParent", "()Ljava/awt/Container;");
                if (mid)
                {
                    std::cout << "Found parent" << std::endl;
                    return make_safe_local<jobject>(env->CallObjectMethod(component, mid));
                }
            }
            else {
                std::cout << "Failed to find component class" << std::endl;
            }
        }
        std::cout << "Failed to find parent" << std::endl;
        return {};
    };

    //Find Canvas Class.
    this->applet = findApplet(frame).release();
    if (!this->applet)
    {
        this->applet = getParent(findCanvas(frame).get()).release();
    }

    if (this->applet)
    {
        this->frame = env->NewGlobalRef(frame);
        env->DeleteLocalRef(std::exchange(this->applet, env->NewGlobalRef(this->applet)));

        auto cls = make_safe_local<jclass>(env->GetObjectClass(this->applet));
        jmethodID mid = env->GetMethodID(cls.get(), "getClass", "()Ljava/lang/Class;");
        auto clsObj = make_safe_local<jobject>(env->CallObjectMethod(this->applet, mid));
        cls.reset(env->GetObjectClass(clsObj.get()));

        //Get Canvas's ClassLoader.
        mid = env->GetMethodID(cls.get(), "getClassLoader", "()Ljava/lang/ClassLoader;");
        this->classLoader = env->NewGlobalRef(make_safe_local<jobject>(env->CallObjectMethod(clsObj.get(), mid)).get());
        return true;
    }
    else {
        std::cout << "Failed to find applet" << std::endl;
    }

    return false;
}

std::string ClientAPI::GetClassName(jobject object) const noexcept
{
    auto getClassName = [&](jobject object) -> std::string {
        auto cls = make_safe_local<jclass>(env->GetObjectClass(object));
        jmethodID mid = env->GetMethodID(cls.get(), "getName", "()Ljava/lang/String;");
        auto strObj = make_safe_local<jstring>(env->CallObjectMethod(object, mid));

        if (strObj)
        {
            const char* str = env->GetStringUTFChars(strObj.get(), nullptr);
            std::string class_name = str;
            env->ReleaseStringUTFChars(strObj.get(), str);
            return class_name;
        }
        return std::string();
    };
    return getClassName(object);
}

void ClientAPI::PrintClasses() const noexcept
{
    if (this->classLoader)
    {
        auto clsLoaderClass = make_safe_local<jclass>(env->GetObjectClass(classLoader));
        jfieldID field = env->GetFieldID(clsLoaderClass.get(), "classes", "Ljava/util/Vector;");
        auto classes = make_safe_local<jobject>(env->GetObjectField(classLoader, field));
        jmethodID toArray = env->GetMethodID(make_safe_local<jclass>(env->GetObjectClass(classes.get())).get(), "toArray", "()[Ljava/lang/Object;");
        auto clses = make_safe_local<jobjectArray>(env->CallObjectMethod(classes.get(), toArray));

        std::cout << "LOADED CLASSES:\n" << std::endl;
        for (int i = 0; i < env->GetArrayLength(clses.get()); ++i)
        {
            auto clsObj = make_safe_local<jobject>(env->GetObjectArrayElement(clses.get(), i));
            std::string name = this->GetClassName(clsObj.get());
            std::cout << name << std::endl;
        }
        fflush(stdout);
    }
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
    if (Initialize()) {
    	std::cout << "Initialized" << std::endl;
    }
    PrintClasses();
    try {
        return this->cache->executeMethod(env, instruction);
        checkAndClearException(env);
    }
    catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "Exception caught in ClientAPI.cpp: " << e.what();
        throw std::runtime_error(oss.str());
    }

    return "failure";
}
