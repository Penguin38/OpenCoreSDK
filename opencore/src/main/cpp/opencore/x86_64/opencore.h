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

#ifndef OPENCORE_X86_64_OPENCORE_IMPL_H_
#define OPENCORE_X86_64_OPENCORE_IMPL_H_

#include "opencore/lp64/opencore.h"

namespace x86_64 {

struct pt_regs {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rax;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t orig_rax;
    uint64_t rip;
    uint32_t cs;
    uint32_t __cs;
    uint64_t flags;
    uint64_t rsp;
    uint32_t ss;
    uint32_t __ss;
    uint64_t fs_base;
    uint64_t gs_base;
    uint32_t ds;
    uint32_t __ds;
    uint32_t es;
    uint32_t __es;
    uint32_t fs;
    uint32_t __fs;
    uint32_t gs;
    uint32_t __gs;
};

typedef struct elf64_prstatus {
    uint32_t             pr_si_signo;
    uint32_t             pr_si_code;
    uint32_t             pr_si_errno;
    uint16_t             pr_cursig;
    uint64_t             pr_sigpend;
    uint64_t             pr_sighold;
    uint32_t             pr_pid;
    uint32_t             pr_ppid;
    uint32_t             pr_pgrp;
    uint32_t             pd_sid;
    uint64_t             pr_utime[2];
    uint64_t             pr_stime[2];
    uint64_t             pr_cutime[2];
    uint64_t             pr_cstime[2];
    struct pt_regs       pr_reg;
    uint32_t             pr_fpvalid;
} Elf64_prstatus;

class Opencore : public lp64::OpencoreImpl {
public:
    Opencore() : lp64::OpencoreImpl(),
                 prnum(0), prstatus(nullptr) {}
    void Finish();
    void CreateCorePrStatus(int pid);
    void WriteCorePrStatus(FILE* fp);
    int IsSpecialFilterSegment(Opencore::VirtualMemoryArea& vma);
    int getMachine() { return EM_X86_64; }
private:
    int prnum;
    Elf64_prstatus *prstatus;
};

} // namespace x86_64

#endif // OPENCORE_X86_64_OPENCORE_IMPL_H_
