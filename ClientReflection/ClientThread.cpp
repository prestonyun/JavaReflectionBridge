JNIEXPORT void JNICALL Java_SomeEntryPoint_nativeTask(JNIEnv* env, jobject) {
    // Obtain the ClientThread singleton instance
    jclass clientThreadClass = env->FindClass("net/runelite/client/callback/ClientThread"); // Replace 'path/to' with the actual package path
    
    // Use injector instance from ClientAPI to get the ClientThread instance
    // ...
    
    jobject clientThreadInstance = env->CallStaticObjectMethod(clientThreadClass, getInstanceMethod);

    // Create a task using a lambda function or any other way
    std::function<void()> task = []() {
        // Define your task here
    };

    // Get the ClientThread class from the JVM
    jclass clientThreadClass = env->FindClass("net/runelite/client/callback/ClientThread");
    
    // Get the getInstance method of the ClientThread class
    jmethodID getInstanceMethod = env->GetStaticMethodID(clientThreadClass, "getInstance", "()Lnet/runelite/client/callback/ClientThread;");
    
    // Call the getInstance method to get the ClientThread instance
    jobject clientThreadInstance = env->CallStaticObjectMethod(clientThreadClass, getInstanceMethod);
    
    // Create a Runnable from the std::function
    // This is a complex task that requires advanced JNI programming
    // Here, I'm assuming you have a function createRunnable that does this
    jobject runnable = createRunnable(task);
    
    // Get the invoke method of the ClientThread class
    jmethodID invokeMethod = env->GetMethodID(clientThreadClass, "invoke", "(Ljava/lang/Runnable;)V");
    
    // Call the invoke method with the Runnable
    env->CallVoidMethod(clientThreadInstance, invokeMethod, runnable);
}
