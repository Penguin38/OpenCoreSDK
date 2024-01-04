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

#ifndef OPENCORESDK_OPENCORE_H
#define OPENCORESDK_OPENCORE_H

#include <jni.h>
#include <string>

#define ELFCOREMAGIC "CORE"
#define NOTE_CORE_NAME_SZ 5
#define ELFLINUXMAGIC "LINUX"
#define NOTE_LINUX_NAME_SZ 6

/*
             ---------- <-
            |          |  \
          --| Program  |   \
         |  | 1 Header |   |
         |  |          |   |
         |   ----------    |
         |  |          |   |        ----------
       --|--| Program  |   |       |          |
      |  |  | 2 Header |   |       |ELF Header|
      |  |  |          |   \       |          |
      |  |   ----------     ------- ----------       ----> ----------
      |  |  |**********|           |          |     |     |          |
      |  |  |**********|           |          |     |     | Thread 1 |
      |  |  |**********|           |  Program |     |     | Registers|
      |  |  |**********|           |  Headers |     |     |          |
      |  |  |**********|           |          |     |      ----------
      |  |  |**********|           |          |    /      |          |
      |  |  |**********|           |          |   /       | Thread 2 |
      |  |   ----------            |          |  /        | Registers|
      |  |  |          |     ------ ---------- --         |          |
    --|--|--| Program  |    /      |          |            ----------
   |  |  |  | N Header |   |     ->|  Segment |           |**********|
   |  |  |  |          |   /    |  | (PT_NOTE)|           |**********|
   |  |  |   ---------- <--   --   |          |           |**********|
   |  |  |                   |      ---------- --         |**********|
   |  |   -------------------      |          |  \         ----------
   |  |                            |  Segment |   \       |          |
   |   --------------------------->| (PT_LOAD)|    |      | Thread N |
   |                               |          |    |      | Registers|
   |                                ----------     |      |          |
   |                               |          |    |       ----------
   |                               |  Segment |    \      |          |
   |                               | (PT_LOAD)|     \     |   AUXV   |
   |                               |          |      \    |          |
   |                                ----------        ---> ----------
   |                               |**********|
   |                               |**********|
   |                               |**********|
   |                               |**********|
   |                               |**********|
   |                                ----------
   |                               |          |
    ------------------------------>|  Segment |
                                   | (PT_LOAD)|
                                   |          |
                                    ----------
*/
#define OPENCORE_VERSION "OpenCoreSDK-1.4.3"
typedef void (*DumpCallback)(bool java, const char* path);

class Opencore {
public:
    static const int MODE_PTRACE = 1 << 0;
    static const int MODE_COPY = 1 << 1;
    static const int MODE_COPY2 = 1 << 2;
    static const int MODE_MAX = MODE_COPY2;

    static const int FLAG_CORE = 1 << 0;
    static const int FLAG_PROCESS_COMM = 1 << 1;
    static const int FLAG_PID = 1 << 2;
    static const int FLAG_THREAD_COMM = 1 << 3;
    static const int FLAG_TID = 1 << 4;
    static const int FLAG_TIMESTAMP = 1 << 5;
    static const int FLAG_ALL = FLAG_CORE | FLAG_PROCESS_COMM | FLAG_PID
                              | FLAG_THREAD_COMM | FLAG_TID | FLAG_TIMESTAMP;

    static const int STATE_ON = 1;
    static const int STATE_OFF = 0;

    static const int DEF_TIMEOUT = 30;

    static const int INVALID_TID = 0;

    static const int FILTER_NONE = 0x0;
    static const int FILTER_SPECIAL_VMA = 1 << 0;
    static const int FILTER_FILE_VMA = 1 << 1;
    static const int FILTER_SHARED_VMA = 1 << 2;
    static const int FILTER_SANITIZER_SHADOW_VMA = 1 << 3;

    static Opencore* GetInstance();
    static bool IsFilterSegment(char* flags, int inode, std::string segment, int offset);
    static void HandleSignal(int);
    static void dump(bool java, const char* filename);
    static void dump(bool java, int tid, const char* filename);
    static bool enable();
    static bool disable();
    static void setDir(const char* dir);
    static void setCallback(DumpCallback cb);
    static void setMode(int mode);
    static void setFlag(int flag);
    static void setTimeout(int sec);
    static void setFilter(int filter);
    static const char* getVersion() { return OPENCORE_VERSION; }
    static void TimeoutHandle(int);

    Opencore() {
        mode = MODE_COPY2;
        flag = FLAG_CORE | FLAG_TID;
        state = STATE_OFF;
        timeout = DEF_TIMEOUT;
        tid = INVALID_TID;
        filter = FILTER_NONE;
    }
    virtual bool DoCoreDump(const char* filename) = 0;
    virtual bool NeedFilterFile(const char* filename, int offset) = 0;
    std::string GetCoreDir() { return dir; }
    DumpCallback GetCallback() { return cb; }
    int GetMode() { return mode; }
    int GetFlag() { return flag; }
    int GetTimeout() { return timeout; }
    int GetFilter() { return filter; }
private:
    void SetCoreDir(std::string d) { dir = d; }
    void SetCallback(DumpCallback c) { cb = c; }
    void SetMode(int m) { mode = m; }
    void SetFlag(int f) { flag = f; }
    void SetTimeout(int t) { timeout = t; }
    void SetFilter(int f) { filter = f; }
    void SetState(int s) { state = s; }
    bool GetState() { return state == STATE_ON; }
    int GetTid() { return tid; }

    DumpCallback cb;
    std::string dir;
    int mode;
    int flag;
    int state;
    int timeout;
    int tid;
    int filter;
};

#endif //OPENCORESDK_OPENCORE_H
