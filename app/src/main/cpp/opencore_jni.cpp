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

static userdata user;

extern "C"
JNIEXPORT jstring JNICALL
Java_penguin_opencore_sdk_Coredump_getVersion(JNIEnv *env, jobject /*thiz*/)
{
    return env->NewStringUTF("Opencore-sdk-1.0");
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
    std::string coredir = std::string(cstr);
    env->ReleaseStringUTFChars(dir, cstr);
    Opencore::setDir(coredir);
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
    return JNI_VERSION_1_4;
}