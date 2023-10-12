#ifndef CLIENT_THREAD_HPP
#define CLIENT_THREAD_HPP

#include <jni.h>
#include <functional>

class ClientThread {
public:
    ClientThread(JNIEnv* env) {
        // Implementation goes here
    }
    void invokeOnClientThread(std::function<void()> task) {
        // Implementation goes here
    }
    jobject createRunnable(std::function<void()> task) {
        // Implementation goes here
    }

private:
    JNIEnv* env;
    jclass clientThreadClass;
    jmethodID getInstanceMethod;
    jmethodID invokeMethod;
};

#endif // CLIENT_THREAD_HPP
