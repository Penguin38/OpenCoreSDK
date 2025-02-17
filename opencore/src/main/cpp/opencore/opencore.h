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

#ifndef OPENCORE_OPENCORE_H_
#define OPENCORE_OPENCORE_H_

#include <inttypes.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <vector>
#include <type_traits>

#define EM_NONE     0
#define EM_386      3
#define EM_ARM      40
#define EM_X86_64   62
#define EM_AARCH64  183
#define EM_RISCV    243

#define ELF_PAGE_SIZE 0x1000

#define ELFCOREMAGIC "CORE"
#define NOTE_CORE_NAME_SZ 5
#define ELFLINUXMAGIC "LINUX"
#define NOTE_LINUX_NAME_SZ 6

#define GENMASK_UL(h, l) (((~0ULL) << (l)) & (~0ULL >> (64 - 1 - (h))))

typedef void (*DumpCallback)(const char* path);

template<typename T>
constexpr T RoundDown(T x, std::remove_reference_t<T> n) {
    return (x & -n);
}

template<typename T>
constexpr T RoundUp(T x, std::remove_reference_t<T> n) {
    return RoundDown(x + n - 1, n);
}

class Opencore {
public:
    static constexpr int FLAG_CORE = 1 << 0;
    static constexpr int FLAG_PROCESS_COMM = 1 << 1;
    static constexpr int FLAG_PID = 1 << 2;
    static constexpr int FLAG_THREAD_COMM = 1 << 3;
    static constexpr int FLAG_TID = 1 << 4;
    static constexpr int FLAG_TIMESTAMP = 1 << 5;
    static constexpr int FLAG_ALL = FLAG_CORE | FLAG_PROCESS_COMM | FLAG_PID
                              | FLAG_THREAD_COMM | FLAG_TID | FLAG_TIMESTAMP;

    static constexpr int INVALID_TID = 0;

    static constexpr int FILTER_NONE = 0x0;
    static constexpr int FILTER_SPECIAL_VMA = 1 << 0;
    static constexpr int FILTER_FILE_VMA = 1 << 1;
    static constexpr int FILTER_SHARED_VMA = 1 << 2;
    static constexpr int FILTER_SANITIZER_SHADOW_VMA = 1 << 3;
    static constexpr int FILTER_NON_READ_VMA = 1 << 4;
    static constexpr int FILTER_SIGNAL_CONTEXT = 1 << 5;
    static constexpr int FILTER_MINIDUMP = 1 << 6;
    static constexpr int FILTER_JAVAHEAP_VMA = 1 << 7;
    static constexpr int FILTER_JIT_CACHE_VMA = 1 << 8;

    static constexpr int VMA_NORMAL = 0;
    static constexpr int VMA_NULL = 1 << 0;
    static constexpr int VMA_INCLUDE = 1 << 1;

    /** only opencore-sdk append **/
    static constexpr int DEF_TIMEOUT = 120;

    Opencore() {
        flag = FLAG_CORE
             | FLAG_PID
             | FLAG_PROCESS_COMM
             | FLAG_TIMESTAMP;
        pid = INVALID_TID;
        tid = INVALID_TID;
        filter = FILTER_NONE;
        extra_note_filesz = 0;
        page_size = sysconf(_SC_PAGE_SIZE);
        align_size = ELF_PAGE_SIZE;
        zero = nullptr;

        /** only opencore-sdk append **/
        ucontext_raw = nullptr;
        siginfo = nullptr;
        timeout = DEF_TIMEOUT;
    }

    struct VirtualMemoryArea {
        uint64_t begin;
        uint64_t end;
        char     flags[4];
        uint32_t offset;
        uint32_t major;
        uint32_t minor;
        uint64_t inode;
        std::string file;
    };

    struct ThreadRecord {
        int pid;
        bool attached;
    };

    void setDir(const char* d) { dir = d; }
    void setPid(int p) { pid = p; }
    void setTid(int t) { tid = t; }
    void setFilter(int f) { filter = f; }
    std::string& getDir() { return dir; }
    int getFlag() { return flag; }
    int getPid() { return pid; }
    int getTid() { return tid; }
    int getFilter() { return filter; }
    int getExtraNoteFilesz() { return extra_note_filesz; }
    bool Coredump(const char* filename);
    virtual void Finish();
    virtual bool DoCoredump(const char* filename) { return false; }
    virtual int NeedFilterFile(Opencore::VirtualMemoryArea& vma) { return VMA_NORMAL; }
    virtual int getMachine() { return EM_NONE; }
    int IsFilterSegment(Opencore::VirtualMemoryArea& vma);
    void StopTheWorld(int pid);
    bool StopTheThread(int tid);
    void Continue();
    static void ParseMaps(int pid, std::vector<VirtualMemoryArea>& maps);

    /** only opencore-sdk append **/
    void setFlag(int f) { flag = f; }
    void setTimeout(int sec) { timeout = sec; }
    void setContext(void *raw) { ucontext_raw = raw; }
    void setSignalInfo(void* info) { siginfo = info; }
    void setCallback(DumpCallback callback) { cb = callback; }
    int getTimeout() { return timeout; }
    void* getContext() { return ucontext_raw; }
    void* getSignalInfo() { return siginfo; }
    DumpCallback getCallback() { return cb; }

    static Opencore* GetInstance();
    static const char* GetVersion() { return __OPENCORE_VERSION__; }
    static void HandleSignal(int signal, siginfo_t* siginfo, void* ucontext_raw);

    class DumpOption {
    public:
        DumpOption()
            : filename(nullptr),
              pid(INVALID_TID), tid(INVALID_TID),
              siginfo(nullptr), context(nullptr) {}
        char* filename;
        int pid;
        int tid;
        void* siginfo;
        void* context;
    };

    static void Dump(const char* filename);
    static void Dump(const char* filename, int tid);
    static void Dump(siginfo_t* siginfo, void* ucontext_raw);
    static void Dump(DumpOption* option);
    static bool Enable();
    static bool Disable();
    static bool IsEnabled();
    static void IgnoreHandler();
    static void SetDir(const char* dir);
    static void SetCallback(DumpCallback cb);
    static void SetFlag(int flag);
    static void SetTimeout(int sec);
    static void SetFilter(int filter);
    static void TimeoutHandle(int);
    static const char* GetDir();
    static int GetFlag();
    static int GetTimeout();
    static int GetFilter();
protected:
    int extra_note_filesz;
    std::vector<ThreadRecord> threads;
    std::vector<VirtualMemoryArea> maps;
    uint8_t* zero;
    uint32_t align_size;
    uint32_t page_size;

    /** only opencore-sdk append **/
    void* ucontext_raw;
    void* siginfo;
private:
    std::string dir;
    int flag;
    int pid;
    int tid;
    int filter;

    /** only opencore-sdk append **/
    DumpCallback cb;
    int timeout;
};

#endif // OPENCORE_OPENCORE_H_
