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

#ifndef LOG_TAG
#define LOG_TAG "opencore"
#endif

#include "eajnis/Log.h"
#include "opencore/opencore.h"
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#if defined(__aarch64__) || defined(__arm64__)
#include "opencore/arm64/opencore.h"
Opencore* opencore = new arm64::Opencore();
#elif defined(__arm__)
#include "opencore/arm/opencore.h"
Opencore* opencore = new arm::Opencore();
#elif defined(__x86_64__)
#include "opencore/x86_64/opencore.h"
Opencore* opencore = new x86_64::Opencore();
#elif defined(__i386__) || defined(__x86__)
#include "opencore/x86/opencore.h"
Opencore* opencore = new x86::Opencore();
#elif defined(__riscv64__)
#include "opencore/riscv64/opencore.h"
Opencore* opencore = new riscv64::Opencore();
#else
Opencore* opencore = new Opencore();
#endif

Opencore* Opencore::GetInstance() {
    return opencore;
}

static pthread_mutex_t g_handle_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_switch_lock = PTHREAD_MUTEX_INITIALIZER;
static constexpr int kExceptionSignals[] = {
    SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS, SIGTRAP
};
static constexpr int kNumHandledSignals = sizeof(kExceptionSignals) / sizeof(kExceptionSignals[0]);
static struct sigaction g_restore_action[kNumHandledSignals] = {0};
static bool handlers_installed = false;

void Opencore::HandleSignal(int signal, siginfo_t* siginfo, void* ucontext_raw) {
    pthread_mutex_lock(&g_handle_lock);
    Disable();
    Dump(siginfo, ucontext_raw);
    raise(signal);
    pthread_mutex_unlock(&g_handle_lock);
}

bool Opencore::Enable() {
    pthread_mutex_lock(&g_switch_lock);
    if (handlers_installed) {
        JNI_LOGW("opencore native handlers already installed.");
        pthread_mutex_unlock(&g_switch_lock);
        return false;
    }

    for (int i = 0; i < kNumHandledSignals; ++i) {
        if (sigaction(kExceptionSignals[i], NULL, &g_restore_action[i]) == -1) {
            JNI_LOGE("signal %d unable to store old handler", kExceptionSignals[i]);
            pthread_mutex_unlock(&g_switch_lock);
            return false;
        }
    }

    struct sigaction stact;
    memset(&stact, 0, sizeof(stact));
    sigemptyset(&stact.sa_mask);

    for (int i = 0; i < kNumHandledSignals; ++i)
        sigaddset(&stact.sa_mask, kExceptionSignals[i]);

    stact.sa_sigaction = Opencore::HandleSignal;
    stact.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;

    for (int i = 0; i < kNumHandledSignals; i++)
        sigaction(kExceptionSignals[i], &stact, NULL);

    handlers_installed = true;
    pthread_mutex_unlock(&g_switch_lock);
    return true;
}

bool Opencore::Disable() {
    pthread_mutex_lock(&g_switch_lock);
    if (!handlers_installed) {
        JNI_LOGW("opencore native handlers already uninstalled.");
        pthread_mutex_unlock(&g_switch_lock);
        return false;
    }

    for (int i = 0; i < kNumHandledSignals; i++)
        sigaction(kExceptionSignals[i], &g_restore_action[i], NULL);

    memset(&g_restore_action, 0, sizeof(g_restore_action));
    handlers_installed = false;
    pthread_mutex_unlock(&g_switch_lock);
    return true;
}

bool Opencore::IsEnabled() {
    bool installed = false;
    pthread_mutex_lock(&g_switch_lock);
    installed = handlers_installed;
    pthread_mutex_unlock(&g_switch_lock);
    return installed;
}

void Opencore::IgnoreHandler() {
    // only child process use
    struct sigaction stact;
    memset(&stact, 0, sizeof(stact));
    stact.sa_handler = SIG_IGN;
    stact.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &stact, NULL);

    // ignore opencore signal handler
    for (int i = 0; i < kNumHandledSignals; i++)
        sigaction(kExceptionSignals[i], &stact, NULL);
}

void Opencore::SetDir(const char *dir) {
    Opencore* impl = GetInstance();
    if (impl) impl->setDir(dir);
}

void Opencore::SetCallback(DumpCallback cb) {
    Opencore* impl = GetInstance();
    if (impl) impl->setCallback(cb);
}

void Opencore::SetFlag(int flag) {
    Opencore* impl = GetInstance();
    if (impl) impl->setFlag(flag);
}

void Opencore::SetTimeout(int sec) {
    Opencore* impl = GetInstance();
    if (impl && sec > 0)
        impl->setTimeout(sec);
}

void Opencore::SetFilter(int filter) {
    Opencore* impl = GetInstance();
    if (impl) impl->setFilter(filter);
}

void Opencore::TimeoutHandle(int) {
    JNI_LOGI("Coredump timeout.");
    Opencore* impl = GetInstance();
    if (impl) impl->Finish();
    _exit(0);
}

const char* Opencore::GetDir() {
    Opencore* impl = GetInstance();
    if (impl) {
        std::string& dir = impl->getDir();
        return dir.c_str();
    }
    return "";
}

int Opencore::GetFlag() {
    Opencore* impl = GetInstance();
    if (impl)
        return impl->getFlag();
    return 0;
}

int Opencore::GetTimeout() {
    Opencore* impl = GetInstance();
    if (impl)
        return impl->getTimeout();
    return DEF_TIMEOUT;
}

int Opencore::GetFilter() {
    Opencore* impl = GetInstance();
    if (impl)
        return impl->getFilter();
    return FILTER_NONE;
}

void Opencore::Dump() {
    Opencore::DumpOption option;
    option.pid = getpid();
    option.tid = gettid();
    Opencore::Dump(&option);
}

void Opencore::Dump(const char* filename) {
    Opencore::DumpOption option;
    option.filename = const_cast<char *>(filename);
    option.pid = getpid();
    option.tid = gettid();
    Opencore::Dump(&option);
}

void Opencore::Dump(const char* filename, int tid) {
    Opencore::DumpOption option;
    option.filename = const_cast<char *>(filename);
    option.pid = getpid();
    option.tid = tid;
    Opencore::Dump(&option);
}

void Opencore::Dump(siginfo_t* siginfo, void* ucontext_raw) {
    Opencore::DumpOption option;
    option.pid = getpid();
    option.tid = gettid();
    option.siginfo = siginfo;
    option.context = ucontext_raw;
    Opencore::Dump(&option);
}

void Opencore::Dump(Opencore::DumpOption* option) {
    if (!option || !option->pid) {
        JNI_LOGE("No any dump!");
        return;
    }

    int ori_dumpable;
    int flag, pid, tid;
    char comm[16];
    bool need_split = false;
    bool need_restore_dumpable = false;
    bool need_restore_ptrace = false;
    std::string output;

    Opencore* impl = GetInstance();
    if (impl) {
        impl->setPid(option->pid);
        impl->setTid(option->tid);
        impl->setSignalInfo(option->siginfo);

        if (impl->getFilter() & FILTER_SIGNAL_CONTEXT)
            impl->setContext(option->context);

        pid = impl->getPid();
        tid = impl->getTid();
        flag = impl->getFlag();

        if (impl->getDir().length() > 0) {
            output.append(impl->getDir()).append("/");
        }
        if (!option->filename) {
            if (!(flag & FLAG_ALL)) {
                flag |= FLAG_CORE;
                flag |= FLAG_TID;
            }

            if (flag & FLAG_CORE)
                output.append("core.");

            if (flag & FLAG_PROCESS_COMM) {
                char comm_path[32];
                snprintf(comm_path, sizeof(comm_path), "/proc/%d/comm", pid);
                int fd = open(comm_path, O_RDONLY);
                if (fd > 0) {
                    memset(&comm, 0x0, sizeof(comm));
                    int rc = read(fd, &comm, sizeof(comm) - 1);
                    if (rc > 0) {
                        for (int i = 0; i < rc; i++) {
                            if (comm[i] == '\n') {
                                comm[i] = 0;
                                break;
                            }
                        }
                        comm[rc] = 0;
                        output.append(comm);
                    } else {
                        output.append("unknown");
                    }
                    close(fd);
                }
                need_split = true;
            }

            if (flag & FLAG_PID) {
                if (need_split)
                    output.append("_");
                output.append(std::to_string(pid));
                need_split = true;
            }

            if (flag & FLAG_THREAD_COMM) {
                if (need_split)
                    output.append("_");

                char thread_comm_path[64];
                snprintf(thread_comm_path, sizeof(thread_comm_path), "/proc/%d/task/%d/comm", pid, tid);
                int fd = open(thread_comm_path, O_RDONLY);
                if (fd > 0) {
                    memset(&comm, 0x0, sizeof(comm));
                    int rc = read(fd, &comm, sizeof(comm) - 1);
                    if (rc > 0) {
                        for (int i = 0; i < rc; i++) {
                            if (comm[i] == '\n') {
                                comm[i] = 0;
                                break;
                            }
                        }
                        comm[rc] = 0;
                        output.append(comm);
                    } else {
                        output.append("unknown");
                    }
                    close(fd);
                }
                need_split = true;
            }

            if (flag & FLAG_TID) {
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
            output.append(option->filename);
        }

        ori_dumpable = prctl(PR_GET_DUMPABLE);
        if (!prctl(PR_SET_DUMPABLE, 1))
            need_restore_dumpable = true;

        if (!prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY))
            need_restore_ptrace = true;

        impl->Coredump(output.c_str());

        if (need_restore_dumpable) prctl(PR_SET_DUMPABLE, ori_dumpable);
        if (need_restore_ptrace) prctl(PR_SET_PTRACER, 0);

        DumpCallback callback = impl->getCallback();
        if (callback) callback(output.c_str());
    } else {
        JNI_LOGI("Not support coredump!!");
    }
}

bool Opencore::Coredump(const char* filename) {
    pid_t child = fork();
    if (child == 0) {
        IgnoreHandler();
        signal(SIGALRM, Opencore::TimeoutHandle);
        alarm(getTimeout());
        DoCoredump(filename);
        Finish();
        _exit(0);
    } else {
        JNI_LOGI("Wait (%d) coredump", child);
        int status = 0;
        wait(&status);
    }
    return true;
}

void Opencore::Finish() {
    Continue();
    maps.clear();
    setContext(nullptr);
    setSignalInfo(nullptr);
}

int Opencore::IsFilterSegment(Opencore::VirtualMemoryArea& vma) {
    int filter = getFilter();
    if (filter & FILTER_SPECIAL_VMA) {
        if (vma.file == "/dev/binderfs/hwbinder"
                || vma.file == "/dev/binderfs/binder"
                || vma.file == "[vvar]"
                || vma.file == "/dev/mali0"
           ) {
            return VMA_NULL;
        }
    }

    if (filter & FILTER_FILE_VMA) {
        if (vma.inode > 0 && vma.flags[1] == '-')
            return NeedFilterFile(vma);
    }

    if (filter & FILTER_SHARED_VMA) {
        if (vma.flags[3] == 's' || vma.flags[3] == 'S')
            return VMA_NULL;
    }

    if (filter & FILTER_SANITIZER_SHADOW_VMA) {
        if (vma.file == "[anon:low shadow]"
                || vma.file == "[anon:high shadow]"
                || (vma.file.compare(0, 12, "[anon:hwasan") == 0))
            return VMA_NULL;
    }

    if (filter & FILTER_NON_READ_VMA) {
        if (vma.flags[0] == '-' && vma.flags[1] == '-' && vma.flags[2] == '-')
            return VMA_NULL;
    }

    if (filter & FILTER_JAVAHEAP_VMA) {
        if (vma.file.compare(0, 12, "[anon:dalvik") == 0)
            return VMA_NULL;
    }

    if (filter & FILTER_JIT_CACHE_VMA) {
        if (vma.file.compare(0, 10, "/memfd:jit") == 0)
            return VMA_NULL;
    }
    return VMA_NORMAL;
}

void Opencore::StopTheWorld(int pid) {
    char task_dir[32];
    struct dirent *entry;
    snprintf(task_dir, sizeof(task_dir), "/proc/%d/task", pid);
    DIR *dp = opendir(task_dir);
    if (dp) {
        while ((entry=readdir(dp)) != NULL) {
            if (!strncmp(entry->d_name, ".", 1)) {
                continue;
            }

            pid_t tid = std::atoi(entry->d_name);
            Opencore::StopTheThread(tid);
        }
        closedir(dp);
    }
}

bool Opencore::StopTheThread(int tid) {
    ThreadRecord ts = {
        .pid = tid,
        .attached = false,
    };
    if (ptrace(PTRACE_ATTACH, tid, NULL, 0) < 0) {
        threads.push_back(ts);
        return false;
    }

    ts.attached = true;
    threads.push_back(ts);

    int status = 0;
    int result = waitpid(tid, &status, WUNTRACED | __WALL);
    if (result != tid) {
        JNI_LOGW("waitpid failed on %d while detaching", tid);
        return false;
    }
    if (WIFSTOPPED(status)) {
        int received_signal = 0;
        if (status >> 16 == PTRACE_EVENT_STOP) {
            received_signal = 0;
        } else {
            received_signal = WSTOPSIG(status);
        }
        return true;
    }
    JNI_LOGW("waitpid failed on %d while non-stop, status 0x%x", tid, status);
    return false;
}

void Opencore::Continue() {
    for (int index = 0; index < threads.size(); index++) {
        ThreadRecord& ts = threads[index];
        if (!ts.attached)
            continue;

        pid_t tid = ts.pid;
        if (ptrace(PTRACE_DETACH, tid, NULL, 0) < 0)
            continue;
    }
    threads.clear();
}

void Opencore::ParseMaps(int pid, std::vector<VirtualMemoryArea>& maps) {
    char filename[32];
    char line[1024];

    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    FILE *fp = fopen(filename, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            int m;
            VirtualMemoryArea vma;
            char filename[256] = {'\0'};


            sscanf(line, "%" PRIx64 "-%" PRIx64 " %c%c%c%c %x %x:%x  %" PRIu64 "  %[^\n] %n",
                   &vma.begin, &vma.end,
                   &vma.flags[0], &vma.flags[1], &vma.flags[2], &vma.flags[3],
                   &vma.offset, &vma.major, &vma.minor, &vma.inode, filename, &m);

            vma.file = filename;
            maps.push_back(vma);
        }
        fclose(fp);
    }
}
