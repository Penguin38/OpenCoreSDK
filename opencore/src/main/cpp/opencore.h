#ifndef OPENCORESDK_OPENCORE_H
#define OPENCORESDK_OPENCORE_H

#include <jni.h>
#include <string>
#include <pthread.h>

#define ELFCOREMAGIC "CORE"
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
#define NT_GNU_PROPERTY_TYPE_0 5

typedef void (*DumpCallback)(void* user, bool java, std::string& filepath);

static pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;

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

    static Opencore* GetInstance();
    static bool IsFilterSegment(std::string segment);
    static void HandleSignal(int);
    static void dump(bool java, const char* filename);
    static bool enable();
    static bool disable();
    static void setDir(const char* dir);
    static void setUserData(void *u);
    static void setCallback(DumpCallback cb);
    static void setMode(int mode);
    static void setFlag(int flag);

    Opencore() {
        mode = MODE_COPY2;
        flag = FLAG_CORE | FLAG_TID;
    }
    virtual bool DoCoreDump(std::string& filename) = 0;
    std::string GetCoreDir() { return dir; }
    void* GetUser() { return user; }
    DumpCallback GetCallback() { return cb; }
    int GetMode() { return mode; }
    int GetFlag() { return flag; }
private:
    void SetCoreDir(std::string d) { dir = d; }
    void SetUserData(void *u) { user = u; }
    void SetCallback(DumpCallback c) { cb = c; }
    void SetMode(int m) { mode = m; }
    void SetFlag(int f) { flag = f; }

    void* user;
    DumpCallback cb;
    std::string dir;
    int mode;
    int flag;
};

#endif //OPENCORESDK_OPENCORE_H
