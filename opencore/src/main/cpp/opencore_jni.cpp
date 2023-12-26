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

static void penguin_opencore_sdk_coredump_docallback(void *arg)
{
    if (gUser.gCoredump && gUser.gCallbackEvent) {
        const char* path = reinterpret_cast<const char *>(arg);
        JNIEnv *env = android::AndroidJNI::getJNIEnv();
        jstring filepath = env->NewStringUTF(path);
        env->CallStaticVoidMethod(gUser.gCoredump, gUser.gCallbackEvent, filepath);
    } else {
        JNI_LOGE("coredump callback err!");
    }
}

static void penguin_opencore_sdk_coredump_callback(bool java, const char* filepath)
{
    if (!java) {
        pthread_t thread = (pthread_t) android::AndroidJNI::createJavaThread("opencore-cb",
                                                                penguin_opencore_sdk_coredump_docallback,
                                                                (void *)filepath);
        if (thread > 0)
            pthread_join(thread, NULL);
    } else {
        penguin_opencore_sdk_coredump_docallback((void *)filepath);
    }
}

extern "C"
JNIEXPORT jstring JNICALL
Java_penguin_opencore_sdk_Coredump_getVersion(JNIEnv *env, jobject /*thiz*/)
{
    return env->NewStringUTF(Opencore::getVersion());
}

extern "C"
JNIEXPORT void JNICALL
Java_penguin_opencore_sdk_Coredump_native_1init(JNIEnv *env, jobject /*thiz*/, jclass clazz)
{
    gUser.gCoredump = (jclass)env->NewGlobalRef(clazz);
    gUser.gCallbackEvent = env->GetStaticMethodID(gUser.gCoredump, "callbackEvent", "(Ljava/lang/String;)V");
    Opencore::setCallback(penguin_opencore_sdk_coredump_callback);
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
JNIEXPORT void JNICALL
Java_penguin_opencore_sdk_Coredump_native_1setCoreTimeout(JNIEnv *env, jobject /*thiz*/, jint sec) {
    Opencore::setTimeout(sec);
}

extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void * /*reserved*/)
{
    JNI_LOGI("Init %s environment..", Opencore::getVersion());
    android::AndroidJNI::init(vm);
    return JNI_VERSION_1_4;
}
