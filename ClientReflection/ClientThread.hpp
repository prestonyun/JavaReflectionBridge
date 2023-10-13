#ifndef CLIENT_THREAD_HPP
#define CLIENT_THREAD_HPP

#include <jni.h>
#include <functional>

class ClientThread {
public:
    ClientThread(JNIEnv* env);
    ClientThread(JNIEnv* env, jobject client);
    ~ClientThread();
    void invokeOnClientThread(std::function<void()> task);

private:
    JNIEnv* env;
    jclass clientThreadClass;
    jobject clientThread;
    jmethodID invokeMethod;
};

#endif // CLIENT_THREAD_HPP
