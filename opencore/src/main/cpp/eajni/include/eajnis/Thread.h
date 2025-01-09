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

#ifndef APENGUIN_THREAD_H
#define APENGUIN_THREAD_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* android_thread_id_t;
typedef int (*android_thread_func_t)(void*);

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus
enum {
    ANDROID_PRIORITY_BACKGROUND     =  10,
    ANDROID_PRIORITY_NORMAL         =   0,
    ANDROID_PRIORITY_DEFAULT        = ANDROID_PRIORITY_NORMAL,
};

class Thread {
public:
    static int androidCreateRawThreadEtc(
            android_thread_func_t entryFunction,
            void *userData,
            const char* threadName,
            int32_t threadPriority,
            size_t threadStackSize,
            android_thread_id_t *threadId);
};

#endif  // __cplusplus

#endif //APENGUIN_THREAD_H
