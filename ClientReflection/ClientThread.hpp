#ifndef CLIENT_THREAD_HPP
#define CLIENT_THREAD_HPP

#include <jni.h>
#include <functional>

class ClientThread {
    public:
        ClientThread(JNIEnv* env) : env(env) {
            clientThreadClass = env->FindClass("net/runelite/client/callback/ClientThread");
            jclass runeLiteClass = env->FindClass("net/runelite/client/RuneLite");
            jfieldID injectorField = env->GetStaticFieldID(runeLiteClass, "injector", "Lcom/google/inject/Injector;");
            jobject injector = env->GetStaticObjectField(runeLiteClass, injectorField);
            jclass injectorClass = env->GetObjectClass(injector);
            jmethodID getInstanceMethod = env->GetMethodID(injectorClass, "getInstance", "(Ljava/lang/Class;)Ljava/lang/Object;");
            clientThread = env->CallObjectMethod(injector, getInstanceMethod, clientThreadClass);
            invokeMethod = env->GetMethodID(clientThreadClass, "invoke", "(Ljava/lang/Runnable;)V");
        }

        ClientThread(JNIEnv* env, jobject client) : env(env) {
            clientThreadClass = env->FindClass("net/runelite/client/callback/ClientThread");
            jmethodID getClientThreadMethod = env->GetMethodID(clientThreadClass, "getClientThread", "()Lnet/runelite/client/callback/ClientThread;");
            clientThread = env->CallObjectMethod(client, getClientThreadMethod);
            invokeMethod = env->GetMethodID(clientThreadClass, "invoke", "(Ljava/lang/Runnable;)V");
        }
        ~ClientThread();
        void invokeOnClientThread(std::function<void()> task);

    private:
        JNIEnv* env;
        jclass clientThreadClass;
        jobject clientThread;
        jmethodID invokeMethod;
    };

    #endif // CLIENT_THREAD_HPP
