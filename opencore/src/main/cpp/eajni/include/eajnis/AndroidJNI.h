/*
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef APENGUIN_ANDROIDJNI_H
#define APENGUIN_ANDROIDJNI_H

#include <eajnis/Thread.h>
#include <jni.h>
#include <string>

#define ANDROID_JNI_VERSION "1.1"

#ifdef __cplusplus
namespace android {

class AndroidJNI {
public:
    static std::string getVersion() { return ANDROID_JNI_VERSION; }

    static void init(JavaVM *vm) { mJavaVM = vm; }

    static JavaVM *getJavaVM() { return mJavaVM; }

    static JNIEnv *getJNIEnv();

    static android_thread_id_t
    createJavaThread(const char *name, void (*start)(void *), void *arg);
    static android_thread_id_t
    createJavaThread(const char *name, void (*start)(void *), void *arg, bool canwait);

private:
    static JavaVM *mJavaVM;

    static int javaCreateThreadEtc(
            android_thread_func_t entryFunction,
            void *userData,
            const char *threadName,
            int32_t threadPriority,
            size_t threadStackSize,
            int32_t threadCreate,
            android_thread_id_t *threadId);

    static int javaThreadShell(void *args);
};
}
#endif  // __cplusplus

#endif //APENGUIN_ANDROIDJNI_H
