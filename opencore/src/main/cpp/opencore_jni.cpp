/*
 * Copyright (C) 2024-present, Guanyou.Chen. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <jni.h>
#include <string>

#ifndef LOG_TAG
#define LOG_TAG "Opencore-SDK"
#endif

#include "opencore/opencore.h"
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

static jstring penguin_opencore_sdk_Coredump_getVersion(JNIEnv *env, jobject /*thiz*/)
{
    return env->NewStringUTF(Opencore::GetVersion());
}

static jboolean penguin_opencore_sdk_Coredump_native_enable(JNIEnv *env, jobject /*thiz*/)
{
    return Opencore::Enable();
}

static jboolean penguin_opencore_sdk_Coredump_native_disable(JNIEnv *env, jobject /*thiz*/)
{
    return Opencore::Disable();
}

static jboolean penguin_opencore_sdk_Coredump_native_doCoredump(JNIEnv *env, jobject /*thiz*/, jint tid, jstring filename)
{
    jboolean isCopy;
    if (filename != NULL) {
        const char *cstr = env->GetStringUTFChars(filename, &isCopy);
        Opencore::Dump(true, cstr, getpid(), tid);
        env->ReleaseStringUTFChars(filename, cstr);
    } else {
        Opencore::Dump(true, nullptr, getpid(), tid);
    }
    return true;
}

static void penguin_opencore_sdk_Coredump_native_setCoreDir(JNIEnv *env, jobject /*thiz*/, jstring dir)
{
    jboolean isCopy;
    if (dir != NULL) {
        const char *cstr = env->GetStringUTFChars(dir, &isCopy);
        Opencore::SetDir(cstr);
        env->ReleaseStringUTFChars(dir, cstr);
    }
}

static void penguin_opencore_sdk_Coredump_native_setCoreFlag(JNIEnv *env, jobject /*thiz*/, jint flag)
{
    Opencore::SetFlag(flag);
}

static void penguin_opencore_sdk_Coredump_native_setCoreTimeout(JNIEnv *env, jobject /*thiz*/, jint sec)
{
    Opencore::SetTimeout(sec);
}

static void penguin_opencore_sdk_Coredump_native_setCoreFilter(JNIEnv *env, jobject /*thiz*/, jint filter)
{
    Opencore::SetFilter(filter);
}

static JNINativeMethod gMethods[] =
{
    {
        "getVersion",
        "()Ljava/lang/String;",
        (void *)penguin_opencore_sdk_Coredump_getVersion
    },
    {
        "native_enable",
        "()Z",
        (void *)penguin_opencore_sdk_Coredump_native_enable
    },
    {
        "native_disable",
        "()Z",
        (void *)penguin_opencore_sdk_Coredump_native_disable
    },
    {
        "native_doCoredump",
        "(ILjava/lang/String;)Z",
        (void *)penguin_opencore_sdk_Coredump_native_doCoredump
    },
    {
        "native_setCoreDir",
        "(Ljava/lang/String;)V",
        (void *)penguin_opencore_sdk_Coredump_native_setCoreDir
    },
    {
        "native_setCoreFlag",
        "(I)V",
        (void *)penguin_opencore_sdk_Coredump_native_setCoreFlag
    },
    {
        "native_setCoreTimeout",
        "(I)V",
        (void *)penguin_opencore_sdk_Coredump_native_setCoreTimeout
    },
    {
        "native_setCoreFilter",
        "(I)V",
        (void *)penguin_opencore_sdk_Coredump_native_setCoreFilter
    },
};

extern "C"
JNIEXPORT jboolean JNICALL
Java_penguin_opencore_sdk_Coredump_native_1init(JNIEnv *env, jclass clazz, jobject object)
{
    gUser.gCoredump = (jclass)env->NewGlobalRef(clazz);
    if (env->RegisterNatives(gUser.gCoredump, gMethods, sizeof(gMethods) / sizeof(gMethods[0])) < 0) {
        JNI_LOGE("Init native environment fail.");
        return false;
    }
    gUser.gCallbackEvent = env->GetStaticMethodID(gUser.gCoredump, "callbackEvent", "(Ljava/lang/String;)V");
    Opencore::SetCallback(penguin_opencore_sdk_coredump_callback);
    return true;
}

extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void * /*reserved*/)
{
    JNI_LOGI("Init %s environment..", Opencore::GetVersion());
    android::AndroidJNI::init(vm);
    return JNI_VERSION_1_4;
}
