#ifndef CLIENT_THREAD_H
#define CLIENT_THREAD_H

#include <jni.h>
#include <functional>

class ClientThread {
public:
    ClientThread(JNIEnv* env);
    void invokeOnClientThread(std::function<void()> task);
    jobject createRunnable(std::function<void()> task);

private:
    JNIEnv* env;
    jclass clientThreadClass;
    jmethodID getInstanceMethod;
    jmethodID invokeMethod;
};

#endif // CLIENT_THREAD_H
