JNIEXPORT void JNICALL Java_SomeEntryPoint_nativeTask(JNIEnv* env, jobject) {
    // Obtain the ClientThread singleton instance
    jclass clientThreadClass = env->FindClass("net/runelite/client/callback/ClientThread"); // Replace 'path/to' with the actual package path
    
    // Use injector instance from ClientAPI to get the ClientThread instance
    // ...
    
    jobject clientThreadInstance = env->CallStaticObjectMethod(clientThreadClass, getInstanceMethod);

    // Create a Runnable for the task you want to execute
    jclass runnableClass = env->FindClass("java/lang/Runnable");
    jmethodID runnableInit = env->GetMethodID(runnableClass, "<init>", "()V");
    jobject runnableObj = env->NewObject(runnableClass, runnableInit);

    // TODO: Set up the actual task you want the Runnable to perform
    // ...

    // Invoke the 'invoke' method on the ClientThread instance
    jmethodID invokeMethod = env->GetMethodID(clientThreadClass, "invoke", "(Ljava/lang/Runnable;)V");
    env->CallVoidMethod(clientThreadInstance, invokeMethod, runnableObj);
}
