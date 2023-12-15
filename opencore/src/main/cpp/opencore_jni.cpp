#include <jni.h>
#include <string>

#ifndef LOG_TAG
#define LOG_TAG "Opencore-SDK"
#endif

#include "opencore.h"
#include "eajnis/AndroidJNI.h"
#include "eajnis/Log.h"

#include <pthread.h>
#include <unistd.h>

struct userdata {
    jclass gCoredump;
    jmethodID gCallbackEvent;
};

static userdata gUser;

struct callback_data {
    std::string path;
};

static void penguin_opencore_sdk_coredump_docallback(void *arg)
{
    callback_data *cbp = reinterpret_cast<callback_data *>(arg);
    JNIEnv *env = android::AndroidJNI::getJNIEnv();
    jstring filepath = env->NewStringUTF(cbp->path.c_str());
    env->CallStaticVoidMethod(gUser.gCoredump, gUser.gCallbackEvent, filepath);
}

static void penguin_opencore_sdk_coredump_callback(bool java, std::string& filepath)
{
    callback_data cb;
    cb.path = filepath;
    if (!java) {
        android::AndroidJNI::createJavaThread("opencore-cb", penguin_opencore_sdk_coredump_docallback, &cb);
    } else {
        penguin_opencore_sdk_coredump_docallback(&cb);
    }
}

extern "C"
JNIEXPORT jstring JNICALL
Java_penguin_opencore_sdk_Coredump_getVersion(JNIEnv *env, jobject /*thiz*/)
{
    return env->NewStringUTF("Opencore-sdk-1.3.4");
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_penguin_opencore_sdk_Coredump_native_1enable(JNIEnv *env, jobject /*thiz*/)
{
    return Opencore::enable();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_penguin_opencore_sdk_Coredump_native_1diable(JNIEnv *env, jobject /*thiz*/)
{
    return Opencore::disable();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_penguin_opencore_sdk_Coredump_native_1doCoredump(JNIEnv *env, jobject /*thiz*/, jstring filename)
{
    jboolean isCopy;
    if (filename != NULL) {
        const char *cstr = env->GetStringUTFChars(filename, &isCopy);
        Opencore::dump(true, cstr);
        env->ReleaseStringUTFChars(filename, cstr);
    } else {
        Opencore::dump(true, nullptr);
    }
    return true;
}

extern "C"
JNIEXPORT void JNICALL
Java_penguin_opencore_sdk_Coredump_native_1setCoreDir(JNIEnv *env, jobject /*thiz*/, jstring dir)
{
    jboolean isCopy;
    if (dir != NULL) {
        const char *cstr = env->GetStringUTFChars(dir, &isCopy);
        Opencore::setDir(cstr);
        env->ReleaseStringUTFChars(dir, cstr);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_penguin_opencore_sdk_Coredump_native_1setCoreMode(JNIEnv *env, jobject /*thiz*/, jint mode) {
    Opencore::setMode(mode);
}

extern "C"
JNIEXPORT void JNICALL
Java_penguin_opencore_sdk_Coredump_native_1setCoreFlag(JNIEnv *env, jobject /*thiz*/, jint flag) {
    Opencore::setFlag(flag);
}

extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void * /*reserved*/)
{
    JNI_LOGI("Init opencore environment..");
    android::AndroidJNI::init(vm);
    JNIEnv *env = android::AndroidJNI::getJNIEnv();
    jclass clazz= env->FindClass("penguin/opencore/sdk/Coredump");
    gUser.gCoredump = (jclass)env->NewGlobalRef(clazz);
    gUser.gCallbackEvent = env->GetStaticMethodID(gUser.gCoredump, "callbackEvent", "(Ljava/lang/String;)V");
    Opencore::setCallback(penguin_opencore_sdk_coredump_callback);
    return JNI_VERSION_1_4;
}
