#include "pch.h"
#include "ClientAPI.hpp"

void DisplayErrorMessage(const std::wstring& message) {
    MessageBoxW(NULL, message.c_str(), L"Error", MB_OK | MB_ICONERROR);
}

bool checkAndClearException(JNIEnv* env) {
    // Check if an exception occurred
    if (env->ExceptionCheck()) {
        // Get the exception object
        jthrowable exception = env->ExceptionOccurred();

        // Clear the exception to be able to call further JNI methods
        env->ExceptionClear();

        // Retrieve the toString() representation of the exception
        jclass throwableClass = env->FindClass("java/lang/Throwable");
        jmethodID toStringMethod = env->GetMethodID(throwableClass, "toString", "()Ljava/lang/String;");
        jstring exceptionString = (jstring)env->CallObjectMethod(exception, toStringMethod);
        const char* exceptionCString = env->GetStringUTFChars(exceptionString, JNI_FALSE);

        // Display the exception message
        std::wstring message = L"JNI Exception: " + std::wstring(exceptionCString, exceptionCString + strlen(exceptionCString));
        MessageBox(NULL, message.c_str(), L"JNI Exception", MB_OK | MB_ICONERROR);

        // Clean up local references and release string memory
        env->ReleaseStringUTFChars(exceptionString, exceptionCString);
        env->DeleteLocalRef(exceptionString);
        env->DeleteLocalRef(throwableClass);
        return true;
    }
    return false;
}

std::string extractSignature(const std::string& str) {
    size_t start = str.find('(');
    size_t end = str.find(')');
    if (start != std::string::npos && end != std::string::npos && start < end) {
        return str.substr(start, end - start + 1);
    }
    return "";
}

static BOOL CALLBACK GetHWNDCurrentPID(HWND WindowHandle, LPARAM lParam)
{
    auto handles = reinterpret_cast<std::vector<HWND>*>(lParam);
    DWORD currentPID = GetCurrentProcessId();
    DWORD windowPID;
    GetWindowThreadProcessId(WindowHandle, &windowPID);

    if (currentPID == windowPID)
    {
        handles->push_back(WindowHandle);
    }


    return TRUE;
}

ClientAPI::ClientAPI() {
    jvm = nullptr;
    env = nullptr;

    injector = nullptr;
    client = nullptr;
    cache = new Cache();

    jsize nVMs;
    jint ret = JNI_GetCreatedJavaVMs(&jvm, 1, &nVMs);
    if (ret != JNI_OK || nVMs == 0) {
        DisplayErrorMessage(L"No JVM found");
        exit(1);
    }
    ret = jvm->AttachCurrentThread((void**)&env, NULL);
    if (ret != JNI_OK) {
        DisplayErrorMessage(L"Failed to attach to JVM");
        exit(1);
    }

}

bool ClientAPI::AttachToThread(JNIEnv** Thread)
{
    if (this->jvm)
        if (this->jvm->GetEnv((void**)Thread, JNI_VERSION_1_6) == JNI_EDETACHED)
            this->jvm->AttachCurrentThread((void**)Thread, nullptr);
    return (*Thread);
}

bool ClientAPI::DetachThread(JNIEnv** Thread)
{
    if (*Thread)
        if (this->jvm)
            if (this->jvm->DetachCurrentThread() == JNI_OK)
                *Thread = nullptr;
    return !(*Thread);
}

HWND ClientAPI::GetCanvasHWND() {
    std::vector<HWND> matchedWindows;
    EnumWindows(GetHWNDCurrentPID, reinterpret_cast<LPARAM>(&matchedWindows));

    //HWND frameHandle = FindWindowWithClassName(matchedWindows, L"SunAwtFrame");
    HWND frameHandle = FindWindowWithTitle(matchedWindows, L"RuneLite");

    if (!frameHandle) {
        DisplayErrorMessage(L"Failed to find frame");
        return nullptr; // No parent frame found.
    }
    HWND canvasHandle = GetWindow(frameHandle, GW_CHILD);
    if (!canvasHandle) {
        DisplayErrorMessage(L"Failed to find canvas");
        return nullptr;
    }
    clientHWND = frameHandle;
    return canvasHandle;
}

HWND ClientAPI::FindWindowWithClassName(const std::vector<HWND>&windows, const wchar_t* className)
{
    wchar_t nameBuffer[128];

    for (auto window : windows)
    {
        GetClassNameW(window, nameBuffer, sizeof(nameBuffer) / sizeof(wchar_t));  // Using the Unicode version
        if (wcscmp(nameBuffer, className) == 0)  // Wide string comparison
            return window;
    }
    DisplayErrorMessage(L"Failed to find window with class name");
    return nullptr;
}

HWND ClientAPI::FindWindowWithTitle(const std::vector<HWND>&windows, const wchar_t* windowTitle)
{
    wchar_t titleBuffer[256]; // Adjust the size as needed

    for (auto window : windows)
    {
        GetWindowTextW(window, titleBuffer, sizeof(titleBuffer) / sizeof(wchar_t));  // Using the Unicode version
        if (wcscmp(titleBuffer, windowTitle) == 0)  // Wide string comparison
            return window;
    }
    return nullptr;
}

jobject ClientAPI::GrabCanvas() {
    HMODULE jvmDLL = GetModuleHandle(L"jvm.dll");
    if (!jvmDLL) {
        MessageBoxW(NULL, L"get jvm.ll failure", L"", MB_OK | MB_ICONERROR);
        return nullptr;
    }

    ptr_GCJavaVMs getJVMs = (ptr_GCJavaVMs)GetProcAddress(jvmDLL, "JNI_GetCreatedJavaVMs");
    if (!getJVMs) {
        MessageBoxW(NULL, L"get jvm failure", L"", MB_OK | MB_ICONERROR);
        return nullptr;
    }
    JNIEnv* thread = nullptr;

    do {
        getJVMs(&(this->jvm), 1, nullptr);
        if (!this->jvm) {
            MessageBoxW(NULL, L"get jvm failure2", L"", MB_OK | MB_ICONERROR);
            break;
        }

        this->AttachToThread(&env);

        HMODULE awtDLL = GetModuleHandle(L"awt.dll");
        if (!awtDLL) {
            MessageBoxW(NULL, L"get awt dll failure", L"", MB_OK | MB_ICONERROR);
            break;
        }

        const char* awtFuncName = (sizeof(void*) == 8) ? "DSGetComponent" : "_DSGetComponent@8";
        this->GetComponent = (ptr_GetComponent)GetProcAddress(awtDLL, awtFuncName);
        if (!env || !this->GetComponent) {
            MessageBoxW(NULL, L"get component failure", L"", MB_OK | MB_ICONERROR);
            break;
        }

        HWND canvasHWND = GetCanvasHWND();
        if (!canvasHWND) {
            MessageBoxW(NULL, L"get handle failure", L"", MB_OK | MB_ICONERROR);
            break;
        }
        jobject tempCanvas = this->GetComponent(env, (void*)canvasHWND);
        if (!tempCanvas) {
            MessageBoxW(NULL, L"get component failure", L"", MB_OK | MB_ICONERROR);
            break;
        }

        jclass canvasClass = env->GetObjectClass(tempCanvas);
        if (!canvasClass) {
            MessageBoxW(NULL, L"canvas object class failure", L"", MB_OK | MB_ICONERROR);
            break;
        }

        jmethodID canvas_getParent = env->GetMethodID(canvasClass, "getParent", "()Ljava/awt/Container;");
        if (!canvas_getParent) {
            MessageBoxW(NULL, L"get parent failure", L"", MB_OK | MB_ICONERROR);
            break;
        }

        jobject tempClient = env->CallObjectMethod(tempCanvas, canvas_getParent);
        if (tempClient) {
            this->canvas = env->NewGlobalRef(tempClient);
            env->DeleteLocalRef(tempClient);
            return this->canvas;
        }
        else {
            MessageBoxW(NULL, L"get client failure", L"", MB_OK | MB_ICONERROR);
            break;
        }
        checkAndClearException(env);
        this->canvas = env->NewGlobalRef(tempClient);
        return this->canvas;
    } while (false);
    return nullptr;
}

jobject ClientAPI::getClient() {
    jclass runeLiteClass = env->FindClass("net/runelite/client/RuneLite");
    if (checkAndClearException(env)) {
        MessageBoxW(NULL, L"Failed to find RuneLite class", L"Error", MB_OK | MB_ICONERROR);
		return nullptr; // or handle the error as appropriate
	}

    jfieldID injectorField = env->GetStaticFieldID(runeLiteClass, "injector", "Lcom/google/inject/Injector;");
    if (checkAndClearException(env)) {
        MessageBoxW(NULL, L"Failed to find injector field", L"Error", MB_OK | MB_ICONERROR);
        return nullptr; // or handle the error as appropriate
    }

    jobject injector = env->GetStaticObjectField(runeLiteClass, injectorField);
    if (checkAndClearException(env)) {
        MessageBoxW(NULL, L"Failed to find injector object field", L"Error", MB_OK | MB_ICONERROR);
        return nullptr; // or handle the error as appropriate
    }

    this->injector = injector;
    jclass injectorClass = this->cache->getClass(env, "InjectorClass", injector);
    if (env->ExceptionCheck()) {
        MessageBoxW(NULL, L"Failed to find injector class", L"Error", MB_OK | MB_ICONERROR);
        env->ExceptionClear();
        return nullptr; // or handle the error as appropriate
    }

    jmethodID getInstanceMethod = env->GetMethodID(injectorClass, "getInstance", "(Ljava/lang/Class;)Ljava/lang/Object;");
    if (env->ExceptionCheck()) {
        MessageBoxW(NULL, L"Failed to find injector instance", L"Error", MB_OK | MB_ICONERROR);
        env->ExceptionClear();
        return nullptr; // or handle the error as appropriate
    }

    jobject runeLiteClient = env->CallObjectMethod(injector, getInstanceMethod, runeLiteClass);
    if (env->ExceptionCheck()) {
        MessageBoxW(NULL, L"Failed to call injector method", L"Error", MB_OK | MB_ICONERROR);
        env->ExceptionClear();
        return nullptr; // or handle the error as appropriate
    }
    jclass runeLiteClientClass = env->GetObjectClass(runeLiteClient);
    if (env->ExceptionCheck()) {
        MessageBoxW(NULL, L"Failed to find client class", L"Error", MB_OK | MB_ICONERROR);
        env->ExceptionClear();
        return nullptr; // or handle the error as appropriate
    }

    jfieldID clientField = env->GetFieldID(runeLiteClientClass, "client", "Lnet/runelite/api/Client;");
    if (env->ExceptionCheck()) {
        MessageBoxW(NULL, L"Failed to find client field", L"Error", MB_OK | MB_ICONERROR);
        env->ExceptionClear();
        return nullptr; // or handle the error as appropriate
    }

    jobject client = env->GetObjectField(runeLiteClient, clientField);
    if (env->ExceptionCheck()) {
        MessageBoxW(NULL, L"Failed to find client object field", L"Error", MB_OK | MB_ICONERROR);
        env->ExceptionClear();
        return nullptr; // or handle the error as appropriate
    }

    jclass clientClass = this->cache->getClass(env, "ClientClass", client);
    if (env->ExceptionCheck()) {
        MessageBoxW(NULL, L"Failed to find client object class", L"Error", MB_OK | MB_ICONERROR);
        env->ExceptionClear();
        return nullptr; // or handle the error as appropriate
    }
    checkAndClearException(env);
    this->client = env->NewGlobalRef(client);
    return client;
}

DWORD WINAPI ShowMessageBox(LPVOID lpParam) {
    const char* msg = (const char*)lpParam;
    int len = MultiByteToWideChar(CP_UTF8, 0, msg, -1, NULL, 0);
    if (len > 0) {
        wchar_t* wmsg = new wchar_t[len];
        MultiByteToWideChar(CP_UTF8, 0, msg, -1, wmsg, len);
        MessageBox(NULL, wmsg, L"Non-blocking Message Box", MB_OK);
        delete[] wmsg;
    }
    return 0;
}


void ClientAPI::CacheClientMethods(JNIEnv* env, jobject object, const std::string& className) {
    jclass objectClass = env->GetObjectClass(object);

    jclass classClass = env->FindClass("java/lang/Class");
    if (checkAndClearException(env)) {
        MessageBoxW(NULL, L"Failed to find Class class", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    jmethodID getDeclaredMethodsMethod = env->GetMethodID(classClass, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
    jobjectArray methodArray = (jobjectArray)env->CallObjectMethod(objectClass, getDeclaredMethodsMethod);

    if (checkAndClearException(env)) {
        MessageBoxW(NULL, L"Failed to find method array", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    jsize methodCount = env->GetArrayLength(methodArray);

    for (jsize i = 0; i < methodCount; i++) {
        jobject methodObject = env->GetObjectArrayElement(methodArray, i);
        if (checkAndClearException(env)) {
            MessageBoxW(NULL, L"Failed to find array element", L"Error", MB_OK | MB_ICONERROR);
            return;
        }

        jclass methodClass = env->GetObjectClass(methodObject);
        jmethodID getNameMethod = env->GetMethodID(methodClass, "getName", "()Ljava/lang/String;");
        jstring nameJavaStr = (jstring)env->CallObjectMethod(methodObject, getNameMethod);

        jmethodID toStringMethod = env->GetMethodID(methodClass, "toString", "()Ljava/lang/String;");
        jstring signatureJavaStr = (jstring)env->CallObjectMethod(methodObject, toStringMethod);
        if (checkAndClearException(env)) {
            MessageBoxW(NULL, L"Failed to find find sig", L"Error", MB_OK | MB_ICONERROR);
            return;
        }

        const char* nameStr = env->GetStringUTFChars(nameJavaStr, 0);
        //ShowMessageBox((LPVOID)nameStr);
        const char* signatureStr = env->GetStringUTFChars(signatureJavaStr, 0);
        ShowMessageBox((LPVOID)signatureStr);

        std::string methodName(nameStr);
        std::string fullSignature(signatureStr);

        std::string actualSignature = extractSignature(fullSignature);

        std::string key = className + "::" + methodName;
        std::string comb = key + " " + actualSignature;

        //CreateThread(NULL, 0, ShowMessageBox, (LPVOID)key.c_str(), NULL, 0);
        this->cache->getMethod(env, key, methodClass, methodName, actualSignature);
        // Clean up local references
        env->ReleaseStringUTFChars(nameJavaStr, nameStr);
        env->DeleteLocalRef(nameJavaStr);
        env->DeleteLocalRef(methodObject);
        env->DeleteLocalRef(methodClass);
    }

    // Clean up
    env->DeleteLocalRef(classClass);
    env->DeleteLocalRef(objectClass);
    env->DeleteLocalRef(methodArray);
}

std::string ClientAPI::ProcessInstruction(const std::string& instruction) {
    if (!this->env) {
        MessageBoxW(NULL, L"no env", L"Error", MB_OK | MB_ICONERROR);
        GrabCanvas();
    }
    if (this->client == nullptr) {
        getClient();
    }
    if (!this->env || !this->client) {
        MessageBoxW(NULL, L"Invalid state: no env or client", L"Error", MB_OK | MB_ICONERROR);
        return "";  // Return early to avoid undefined behavior
    }
    CacheClientMethods(env, client, "ClientClass");
    DisplayErrorMessage(L"methods cached");

    std::stringstream ss;
    for (const auto& pair : this->cache->methodCache) {
        ss.str("");  // Clear the stringstream
        ss << "Key: " << pair.first << "\n";
        ss << "Method ID: " << pair.second.id << "\n";
        ss << "Signature: " << pair.second.signature << "\n\n";

        int slength = static_cast<int>(ss.str().length()) + 1;
        int len = MultiByteToWideChar(CP_UTF8, 0, ss.str().c_str(), slength, NULL, 0);
        if (len > 0) {
            wchar_t* buf = new wchar_t[len];
            MultiByteToWideChar(CP_UTF8, 0, ss.str().c_str(), slength, buf, len);
            std::wstring r(buf);
            DisplayErrorMessage(r);
            delete[] buf;  // Free memory at the end of each iteration
        }
    }
    return ss.str();
}

