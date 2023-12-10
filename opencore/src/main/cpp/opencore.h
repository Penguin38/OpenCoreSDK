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

typedef void (*DumpCallback)(void* user, bool java);

static pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;

class Opencore {
public:
    static const int MODE_PTRACE = 1 << 0;
    static const int MODE_COPY = 1 << 1;
    static const int MODE_COPY2 = 1 << 2;
    static const int MODE_MAX = MODE_COPY2;

    static Opencore* GetInstance();
    static bool IsFilterSegment(std::string segment);
    static void HandleSignal(int);
    static void dump(bool java);
    static bool enable();
    static bool disable();
    static void setDir(const char* dir);
    static void setDir(std::string dir);
    static void setUserData(void *u);
    static void setCallback(DumpCallback cb);
    static void setMode(int mode);

    Opencore() { mode = MODE_COPY2; }
    virtual bool DoCoreDump() = 0;
    std::string GetCoreDir() { return dir; }
    void* GetUser() { return user; }
    DumpCallback GetCallback() { return cb; }
    int GetMode() { return mode; }
private:
    void SetCoreDir(std::string d) { dir = d; }
    void SetUserData(void *u) { user = u; }
    void SetCallback(DumpCallback c) { cb = c; }
    void SetMode(int m) { mode = m; }

    void* user;
    DumpCallback cb;
    std::string dir;
    int mode;
};

#endif //OPENCORESDK_OPENCORE_H
