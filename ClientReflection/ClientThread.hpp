#ifndef CLIENT_THREAD_HPP
#define CLIENT_THREAD_HPP

#include <jni.h>
#include <functional>

std::function<void()> currentTask;  // Global

extern "C" JNIEXPORT void JNICALL Java_Path_To_Callback_RunTask(JNIEnv*, jclass) {
    if (currentTask) {
        currentTask();
        currentTask = nullptr;  // Reset after using
    }
}


class ClientThread {
public:
    ClientThread(JNIEnv* env)
    ClientThread(JNIEnv* env, jobject client)
    ~ClientThread() {
        env->DeleteLocalRef(clientThreadClass);
        env->DeleteLocalRef(clientThread);
    }
    void invokeOnClientThread(std::function<void()> task)

    jobject createRunnable(std::function<void()> task)



private:
    JNIEnv* env;
    jclass clientThreadClass;
    jobject clientThread;

    jobject injector;
    jmethodID invokeMethod;
};

#endif // CLIENT_THREAD_HPP
