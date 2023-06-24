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

static userdata user;

static void penguin_opencore_sdk_coredump_docallback(void *arg)
{
    userdata *user = reinterpret_cast<userdata*>(arg);
    JNIEnv *env = android::AndroidJNI::getJNIEnv();
    env->CallStaticVoidMethod(user->gCoredump, user->gCallbackEvent);
}

static void penguin_opencore_sdk_coredump_callback(void* user, bool java)
{
    if (!java) {
        android::AndroidJNI::createJavaThread("opencore-cb", penguin_opencore_sdk_coredump_docallback, user);
    } else {
        penguin_opencore_sdk_coredump_docallback(user);
    }
}

extern "C"
JNIEXPORT jstring JNICALL
Java_penguin_opencore_sdk_Coredump_getVersion(JNIEnv *env, jobject /*thiz*/)
{
    return env->NewStringUTF("Opencore-sdk-1.2.7");
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
Java_penguin_opencore_sdk_Coredump_native_1doCoredump(JNIEnv *env, jobject /*thiz*/)
{
    Opencore::dump(true);
    return true;
}

extern "C"
JNIEXPORT void JNICALL
Java_penguin_opencore_sdk_Coredump_native_1setCoreDir(JNIEnv *env, jobject /*thiz*/, jstring dir)
{
    jboolean isCopy;
    const char *cstr = env->GetStringUTFChars(dir, &isCopy);
    Opencore::setDir(cstr);
    env->ReleaseStringUTFChars(dir, cstr);
}

extern "C"
JNIEXPORT void JNICALL
Java_penguin_opencore_sdk_Coredump_native_1setCoreMode(JNIEnv *env, jobject /*thiz*/, jint mode) {
    Opencore::setMode(mode);
}

extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void * /*reserved*/)
{
    JNI_LOGI("Init opencore environment..");
    android::AndroidJNI::init(vm);
    JNIEnv *env = android::AndroidJNI::getJNIEnv();
    jclass clazz= env->FindClass("penguin/opencore/sdk/Coredump");
    user.gCoredump = (jclass)env->NewGlobalRef(clazz);
    user.gCallbackEvent = env->GetStaticMethodID(user.gCoredump, "callbackEvent", "()V");
    Opencore::setUserData(&user);
    Opencore::setCallback(penguin_opencore_sdk_coredump_callback);
    return JNI_VERSION_1_4;
}
