#ifndef LOG_TAG
#define LOG_TAG "Opencore"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <eajnis/AndroidJNI.h>
#include "opencore.h"
#if defined(__aarch64__) || defined(__arm64__)
#include "arm64/opencore_impl.h"
#endif
#if defined(__arm__)
#include "arm/opencore_impl.h"
#endif
#include "eajnis/Log.h"

Opencore* Opencore::GetInstance()
{
#if defined(__aarch64__) || defined(__arm64__) || defined(__arm__)
    return OpencoreImpl::GetInstance();
#endif
    return nullptr;
}

void Opencore::dump(bool java, const char* filename)
{
    int ori_dumpable;
    int flag;
    int pid = 0;
    int tid = 0;
    char comm[16];
    bool need_split = false;
    bool need_restore_dumpable = false;
    bool need_restore_ptrace = false;
    std::string output;

    Opencore* impl = GetInstance();
    if (impl) {
        output.append(impl->GetCoreDir()).append("/");
        if (!filename) {
            flag = impl->GetFlag();
            if (!(flag & FLAG_ALL)) {
                flag |= FLAG_CORE;
                flag |= FLAG_TID;
            }

            if (flag & FLAG_CORE)
                output.append("core.");

            if (flag & FLAG_PROCESS_COMM) {
                pid = getpid();
                char comm_path[32];
                snprintf(comm_path, sizeof(comm_path), "/proc/%d/comm", pid);
                int fd = open(comm_path, O_RDONLY);
                if (fd > 0) {
                    memset(&comm, 0x0, sizeof(comm));
                    int rc = read(fd, &comm, sizeof(comm) - 1);
                    if (rc > 0) {
                        if (rc < sizeof(comm) - 1)
                            comm[rc - 1] = 0;
                        output.append(comm);
                    } else {
                        output.append("unknown");
                    }
                    close(fd);
                }
                need_split = true;
            }

            if (flag & FLAG_PID) {
                if (!pid)
                    pid = getpid();
                if (need_split)
                    output.append("_");
                output.append(std::to_string(pid));
                need_split = true;
            }

            if (flag & FLAG_THREAD_COMM) {
                if (!pid)
                    pid = getpid();
                if (need_split)
                    output.append("_");

                tid = gettid();
                char thread_comm_path[64];
                snprintf(thread_comm_path, sizeof(thread_comm_path), "/proc/%d/task/%d/comm", pid, tid);
                int fd = open(thread_comm_path, O_RDONLY);
                if (fd > 0) {
                    memset(&comm, 0x0, sizeof(comm));
                    int rc = read(fd, &comm, sizeof(comm) - 1);
                    if (rc > 0) {
                        if (rc < sizeof(comm) - 1)
                            comm[rc - 1] = 0;
                        output.append(comm);
                    } else {
                        output.append("unknown");
                    }
                    close(fd);
                }
                need_split = true;
            }

            if (flag & FLAG_TID) {
                if (!tid)
                    tid = gettid();
                if (need_split)
                    output.append("_");
                output.append(std::to_string(tid));
                need_split = true;
            }

            if (flag & FLAG_TIMESTAMP) {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                if (need_split)
                    output.append("_");
                output.append(std::to_string(tv.tv_sec));
            }
        } else {
            output.append(filename);
        }

        ori_dumpable = prctl(PR_GET_DUMPABLE);
        if (!prctl(PR_SET_DUMPABLE, 1))
            need_restore_dumpable = true;

        if (!prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY))
            need_restore_ptrace = true;

        impl->DoCoreDump(output);

        if (need_restore_dumpable) prctl(PR_SET_DUMPABLE, ori_dumpable);
        if (need_restore_ptrace) prctl(PR_SET_PTRACER, 0);
    } else {
        JNI_LOGI("Not support coredump!!");
    }

    DumpCallback callback = impl->GetCallback();
    if (callback) {
        callback(impl->GetUser(), java, output);
    }
}

void Opencore::HandleSignal(int)
{
    pthread_mutex_lock(&gLock);
    disable();
    dump(false, nullptr);
    pthread_mutex_unlock(&gLock);
}

bool Opencore::enable()
{
    Opencore* impl = GetInstance();
    if (impl) {
        struct sigaction stact;
        memset(&stact, 0, sizeof(stact));
        stact.sa_handler = Opencore::HandleSignal;
        sigaction(SIGSEGV, &stact, NULL);
        sigaction(SIGABRT, &stact, NULL);
        sigaction(SIGBUS, &stact, NULL);
        sigaction(SIGILL, &stact, NULL);
        sigaction(SIGFPE, &stact, NULL);
        return true;
    }
    return false;
}

bool Opencore::disable()
{
    Opencore* impl = GetInstance();
    if (impl) {
        signal(SIGSEGV, SIG_DFL);
        signal(SIGABRT, SIG_DFL);
        signal(SIGBUS, SIG_DFL);
        signal(SIGILL, SIG_DFL);
        signal(SIGFPE, SIG_DFL);
        return true;
    }
    return false;
}

void Opencore::setDir(const char *dir)
{
    Opencore* impl = GetInstance();
    if (impl) {
        impl->SetCoreDir(dir);
    }
}

void Opencore::setUserData(void *user)
{
    Opencore* impl = GetInstance();
    if (impl) {
        impl->SetUserData(user);
    }
}

void Opencore::setCallback(DumpCallback cb)
{
    Opencore* impl = GetInstance();
    if (impl) {
        impl->SetCallback(cb);
    }
}

void Opencore::setMode(int mode)
{
    Opencore* impl = GetInstance();
    if (impl) {
        if (mode > MODE_MAX) {
            impl->SetMode(MODE_MAX);
        } else {
            impl->SetMode(mode);
        }
    }
}

void Opencore::setFlag(int flag)
{
    Opencore* impl = GetInstance();
    if (impl) {
        impl->SetFlag(flag);
    }
}

bool Opencore::IsFilterSegment(std::string segment)
{
    if (segment == "/dev/binderfs/hwbinder"
            || segment == "/dev/binderfs/binder"
            || segment == "[vvar]"
            || segment == "/dev/mali0"
            //|| segment == "/memfd:jit-cache (deleted)"
            ) {
        return true;
    }
    return false;
}
