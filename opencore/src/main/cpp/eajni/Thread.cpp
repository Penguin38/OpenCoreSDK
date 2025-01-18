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

#include <eajnis/Thread.h>
#include <eajnis/Log.h>

#include <pthread.h>
#include <string>
#include <errno.h>

typedef void* (*android_pthread_entry)(void*);

int Thread::androidCreateRawThreadEtc(android_thread_func_t entryFunction, void *userData,
                                      const char *threadName, int32_t threadPriority, size_t threadStackSize,
                                      int32_t threadCreate, android_thread_id_t *threadId) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, threadCreate);

    if (threadStackSize) {
        pthread_attr_setstacksize(&attr, threadStackSize);
    }

    errno = 0;
    pthread_t thread;
    int result = pthread_create(&thread, &attr, (android_pthread_entry)entryFunction, userData);

    pthread_attr_destroy(&attr);
    if (result != 0) {
        JNI_LOGE("androidCreateRawThreadEtc failed (entry=%p, res=%d, errno=%d, threadPriority=%d)",
             entryFunction, result, errno, threadPriority);
        return 0;
    }

    if (threadId != NULL) {
        *threadId = (android_thread_id_t)thread;
    }
    return 1;
}
