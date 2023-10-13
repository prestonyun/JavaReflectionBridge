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
    ClientThread(JNIEnv* env) {
        clientThreadClass = env->FindClass("net/runelite/client/callback/ClientThread");
        jclass runeLiteClass = env->FindClass("net/runelite/client/RuneLite");
        jfieldID injectorField = env->GetStaticFieldID(runeLiteClass, "injector", "Lcom/google/inject/Injector;");
        injector = env->GetStaticObjectField(runeLiteClass, injectorField);
        jclass injectorClass = env->GetObjectClass(injector)
        jmethodID getInstanceMethod = env->GetMethodID(injectorClass, "getInstance", "(Ljava/lang/Class;)Ljava/lang/Object;");
        clientThread = env->CallObjectMethod(injector, getInstanceMethod, clientThreadClass);
    }
    ~ClientThread() {
        env->DeleteLocalRef(clientThreadClass);
        env->DeleteLocalRef(clientThread);
    }
    void invokeOnClientThread(std::function<void()> task) {
        jobject runnable = createRunnable(task);
        
        if (!invokeMethod) {
            invokeMethod = env->GetMethodID(clientThreadClass, "invoke", "(Ljava/lang/Runnable;)V");
        }
        env->CallVoidMethod(clientThread, invokeMethod, runnable);

        env->DeleteLocalRef(runnable);
    }

    jobject createRunnable(std::function<void()> task) {
        currentTask = task;
        
        // Create a new Runnable using an anonymous subclass
        std::string runnableCode = "(Ljava/lang/Object;)Ljava/lang/Runnable;";
        jmethodID ctor = env->GetMethodID(clientThreadClass, "<init>", "()V");
        jobject newRunnable = env->NewObject(clientThreadClass, ctor);
        return newRunnable;
    }



private:
    JNIEnv* env;
    jclass clientThreadClass;
    jobject clientThread;

    jobject injector;
    jmethodID invokeMethod;
};

#endif // CLIENT_THREAD_HPP
