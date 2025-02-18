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
#define LOG_TAG "Opencore"
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

static void penguin_opencore_sdk_coredump_docallback(void *arg) {
    if (gUser.gCoredump && gUser.gCallbackEvent) {
        const char* path = reinterpret_cast<const char *>(arg);
        JNIEnv *env = android::AndroidJNI::getJNIEnv();
        jstring filepath = env->NewStringUTF(path);
        env->CallStaticVoidMethod(gUser.gCoredump, gUser.gCallbackEvent, filepath);
    } else {
        JNI_LOGE("coredump callback err!");
    }
}

static void penguin_opencore_sdk_coredump_callback(const char* filepath) {
    pthread_t thread = (pthread_t) android::AndroidJNI::createJavaThread("opencore-cb",
                                                            penguin_opencore_sdk_coredump_docallback,
                                                            (void *)filepath, true);
    if (thread) pthread_join(thread, NULL);
}

static jstring penguin_opencore_sdk_Coredump_nativeVersion(JNIEnv* env, jclass /*clazz*/) {
    return env->NewStringUTF(Opencore::GetVersion());
}

static jboolean penguin_opencore_sdk_Coredump_nativeEnable(JNIEnv* /*env*/, jclass /*clazz*/) {
    return Opencore::Enable();
}

static jboolean penguin_opencore_sdk_Coredump_nativeDisable(JNIEnv* /*env*/, jclass /*clazz*/) {
    return Opencore::Disable();
}

static jboolean penguin_opencore_sdk_Coredump_nativeCoredump(JNIEnv* env, jclass /*clazz*/, jint tid, jstring filename) {
    jboolean isCopy;
    if (filename != NULL) {
        const char *cstr = env->GetStringUTFChars(filename, &isCopy);
        Opencore::Dump(cstr, tid);
        env->ReleaseStringUTFChars(filename, cstr);
    } else {
        Opencore::Dump(nullptr, tid);
    }
    return true;
}

static void penguin_opencore_sdk_Coredump_nativeSetDir(JNIEnv* env, jclass /*clazz*/, jstring dir) {
    jboolean isCopy;
    if (dir != NULL) {
        const char *cstr = env->GetStringUTFChars(dir, &isCopy);
        Opencore::SetDir(cstr);
        env->ReleaseStringUTFChars(dir, cstr);
    }
}

static void penguin_opencore_sdk_Coredump_nativeSetFlag(JNIEnv* /*env*/, jclass /*clazz*/, jint flag) {
    Opencore::SetFlag(flag);
}

static void penguin_opencore_sdk_Coredump_nativeSetTimeout(JNIEnv* /*env*/, jclass /*clazz*/, jint sec) {
    Opencore::SetTimeout(sec);
}

static void penguin_opencore_sdk_Coredump_nativeSetFilter(JNIEnv* /*env*/, jclass /*clazz*/, jint filter) {
    Opencore::SetFilter(filter);
}

static jboolean penguin_opencore_sdk_Coredump_nativeIsEnabled(JNIEnv* /*env*/, jclass /*clazz*/) {
    return Opencore::IsEnabled();
}

static jstring penguin_opencore_sdk_Coredump_nativeGetDir(JNIEnv* env, jclass /*clazz*/) {
    const char* dir = Opencore::GetDir();
    return env->NewStringUTF(dir);
}

static jint penguin_opencore_sdk_Coredump_nativeGetFlag(JNIEnv* /*env*/, jclass /*clazz*/) {
    return Opencore::GetFlag();
}

static jint penguin_opencore_sdk_Coredump_nativeGetTimeout(JNIEnv* /*env*/, jclass /*clazz*/) {
    return Opencore::GetTimeout();
}

static jint penguin_opencore_sdk_Coredump_nativeGetFilter(JNIEnv* /*env*/, jclass /*clazz*/) {
    return Opencore::GetFilter();
}

static JNINativeMethod gMethods[] = {
    {
        "nativeVersion",
        "()Ljava/lang/String;",
        (void *)penguin_opencore_sdk_Coredump_nativeVersion
    },
    {
        "nativeEnable",
        "()Z",
        (void *)penguin_opencore_sdk_Coredump_nativeEnable
    },
    {
        "nativeDisable",
        "()Z",
        (void *)penguin_opencore_sdk_Coredump_nativeDisable
    },
    {
        "nativeCoredump",
        "(ILjava/lang/String;)Z",
        (void *)penguin_opencore_sdk_Coredump_nativeCoredump
    },
    {
        "nativeSetDir",
        "(Ljava/lang/String;)V",
        (void *)penguin_opencore_sdk_Coredump_nativeSetDir
    },
    {
        "nativeSetFlag",
        "(I)V",
        (void *)penguin_opencore_sdk_Coredump_nativeSetFlag
    },
    {
        "nativeSetTimeout",
        "(I)V",
        (void *)penguin_opencore_sdk_Coredump_nativeSetTimeout
    },
    {
        "nativeSetFilter",
        "(I)V",
        (void *)penguin_opencore_sdk_Coredump_nativeSetFilter
    },
    {
        "nativeIsEnabled",
        "()Z",
        (void *)penguin_opencore_sdk_Coredump_nativeIsEnabled
    },
    {
        "nativeGetDir",
        "()Ljava/lang/String;",
        (void *)penguin_opencore_sdk_Coredump_nativeGetDir
    },
    {
        "nativeGetFlag",
        "()I",
        (void *)penguin_opencore_sdk_Coredump_nativeGetFlag
    },
    {
        "nativeGetTimeout",
        "()I",
        (void *)penguin_opencore_sdk_Coredump_nativeGetTimeout
    },
    {
        "nativeGetFilter",
        "()I",
        (void *)penguin_opencore_sdk_Coredump_nativeGetFilter
    },
};

extern "C"
JNIEXPORT jboolean JNICALL
Java_penguin_opencore_sdk_Coredump_nativeInit(JNIEnv* env, jclass clazz) {
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
JNI_OnLoad(JavaVM *vm, void * /*reserved*/) {
    JNI_LOGI("Init %s environment..", Opencore::GetVersion());
    android::AndroidJNI::init(vm);
    return JNI_VERSION_1_4;
}

// #define INJECT_OPENCORE_DIR "/sdcard/Android/data/penguin.opencore.sdk/files"
/*
 * build inject version if define macro "INJECT_OPENCORE_DIR" topath save dir.
 * exp:
 *     #define INJECT_OPENCORE_DIR "/sdcard/Android/data/penguin.opencore.sdk/files"
 * core-parser> remote hook --inject -l libopencore.so
 * it will inject target process to enable opencore native crash handler.
 */
#if defined(INJECT_OPENCORE_DIR)
extern "C"
void __attribute__((constructor)) opencore_ctor_init() {
    Opencore::SetDir(INJECT_OPENCORE_DIR);
    Opencore::SetTimeout(180);
    Opencore::SetFlag(Opencore::FLAG_CORE
                    | Opencore::FLAG_PROCESS_COMM
                    | Opencore::FLAG_PID
                    | Opencore::FLAG_TIMESTAMP);
    Opencore::SetFilter(Opencore::FILTER_SPECIAL_VMA
                     // | Opencore::FILTER_FILE_VMA
                     // | Opencore::FILTER_SHARED_VMA
                      | Opencore::FILTER_SANITIZER_SHADOW_VMA
                      | Opencore::FILTER_NON_READ_VMA
                      | Opencore::FILTER_SIGNAL_CONTEXT
                     // | Opencore::FILTER_JAVAHEAP_VMA
                     // | Opencore::FILTER_JIT_CACHE_VMA
                     /* | Opencore::FILTER_MINIDUMP */);
    Opencore::Enable();
    JNI_LOGI("Init inject %s environment..", Opencore::GetVersion());
}
#endif
