#include "ClientThread.hpp"

ClientThread::ClientThread(JNIEnv* env) : env(env) {
    clientThreadClass = env->FindClass("net/runelite/client/callback/ClientThread");
    jclass runeLiteClass = env->FindClass("net/runelite/client/RuneLite");
    jfieldID injectorField = env->GetStaticFieldID(runeLiteClass, "injector", "Lcom/google/inject/Injector;");
    jobject injector = env->GetStaticObjectField(runeLiteClass, injectorField);
    jclass injectorClass = env->GetObjectClass(injector);
    jmethodID getInstanceMethod = env->GetMethodID(injectorClass, "getInstance", "(Ljava/lang/Class;)Ljava/lang/Object;");
    clientThread = env->CallObjectMethod(injector, getInstanceMethod, clientThreadClass);
    invokeMethod = env->GetMethodID(clientThreadClass, "invoke", "(Ljava/lang/Runnable;)V");
}

ClientThread::ClientThread(JNIEnv* env, jobject client) : env(env) {
    clientThreadClass = env->FindClass("net/runelite/client/callback/ClientThread");
    jmethodID getClientThreadMethod = env->GetMethodID(clientThreadClass, "getClientThread", "()Lnet/runelite/client/callback/ClientThread;");
    clientThread = env->CallObjectMethod(client, getClientThreadMethod);
    invokeMethod = env->GetMethodID(clientThreadClass, "invoke", "(Ljava/lang/Runnable;)V");
}

void ClientThread::invokeOnClientThread(std::function<void()> task) {
    // The implementation of this method depends on how you want to create the Runnable object
    // and how you want to set its run method to call the task.
    // For now, I'll leave it empty.
}
