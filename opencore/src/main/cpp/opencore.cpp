#ifndef LOG_TAG
#define LOG_TAG "Opencore"
#endif

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

void Opencore::dump(bool java)
{
    Opencore* impl = GetInstance();
    if (impl) {
        impl->DoCoreDump();
    } else {
        JNI_LOGI("Not support coredump!!");
    }

    DumpCallback callback = impl->GetCallback();
    if (callback) {
        callback(impl->GetUser(), java);
    }
}

void Opencore::HandleSignal(int)
{
    disable();
    dump(false);
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

void Opencore::setDir(std::string dir)
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
        impl->SetMode(mode);
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