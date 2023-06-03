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

#include <eajnis/AndroidJNI.h>
#include <eajnis/Log.h>
#include <eajnis/Thread.h>

#include <string>
#include <assert.h>
#include <android/binder_ibinder.h>

namespace android {
    JavaVM *AndroidJNI::mJavaVM = NULL;

    static int javaAttachThread(const char *threadName, JNIEnv **pEnv) {
        JavaVMAttachArgs args;
        JavaVM *vm;
        jint result;

        vm = AndroidJNI::getJavaVM();
        assert(vm != NULL);

        args.version = JNI_VERSION_1_4;
        args.name = (char *) threadName;
        args.group = NULL;

        result = vm->AttachCurrentThread(pEnv, (void *) &args);

        if (result != JNI_OK)
            JNI_LOGE("attach of thread '%s' failed\n", threadName);

        return result;
    }

    static int javaDetachThread(void) {
        JavaVM *vm;
        jint result;

        vm = AndroidJNI::getJavaVM();
        assert(vm != NULL);

        result = vm->DetachCurrentThread();
        if (result != JNI_OK)
            JNI_LOGE("thread detach failed\n");

        return result;
    }

    JNIEnv *AndroidJNI::getJNIEnv() {
        JNIEnv *env;
        JavaVM *vm = AndroidJNI::getJavaVM();
        assert(vm != NULL);

        if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK)
            return NULL;

        return env;
    }

    int AndroidJNI::javaThreadShell(void *args) {
        void *start = ((void **) args)[0];
        void *userData = ((void **) args)[1];
        char *name = (char *) ((void **) args)[2];
        free(args);

        JNIEnv *env;
        int result;

        if (javaAttachThread(name, &env) != JNI_OK)
            return -1;

        result = (*(android_thread_func_t) start)(userData);

        javaDetachThread();
        free(name);

        return result;
    }

    int AndroidJNI::javaCreateThreadEtc(
            android_thread_func_t entryFunction,
            void *userData,
            const char *threadName,
            int32_t threadPriority,
            size_t threadStackSize,
            android_thread_id_t *threadId) {

        void **args = (void **) malloc(3 * sizeof(void *));
        int result;

        if (!threadName)
            threadName = "unnamed thread";

        args[0] = (void *) entryFunction;
        args[1] = userData;
        args[2] = (void *) strdup(threadName);

        result = Thread::androidCreateRawThreadEtc(AndroidJNI::javaThreadShell, args,
                                                   threadName, threadPriority, threadStackSize,
                                                   threadId);

        return result;
    }

    android_thread_id_t
    AndroidJNI::createJavaThread(const char *name, void (*start)(void *), void *arg) {
        android_thread_id_t thread_id = 0;
        javaCreateThreadEtc((android_thread_func_t) start, arg, name, ANDROID_PRIORITY_DEFAULT, 0,
                            &thread_id);
        return thread_id;
    }
}


