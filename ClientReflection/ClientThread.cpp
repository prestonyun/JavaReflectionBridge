#include "ClientThread.hpp"

void ClientThread::invokeOnClientThread(std::function<void()> task) {
    env->CallVoidMethod(clientThread, invokeMethod, env->NewObject(env->FindClass("java/lang/Runnable"), env->GetMethodID(env->FindClass("java/lang/Runnable"), "<init>", "()V"), [task] () {
        task();
    }));
}
