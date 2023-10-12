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

    // Use the ClientAPI instance to invoke the task on the client thread
    clientAPI->invokeOnClientThread(task);
}
