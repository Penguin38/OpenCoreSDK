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

#ifndef OPENCORE_X86_OPENCORE_IMPL_H_
#define OPENCORE_X86_OPENCORE_IMPL_H_

#include "opencore/lp32/opencore.h"

namespace x86 {

struct pt_regs {
    uint32_t ebx, ecx, edx, esi, edi, ebp, eax;
    uint16_t ds, __ds, es, __es;
    uint16_t fs, __fs, gs, __gs;
    uint32_t orig_eax, eip;
    uint16_t cs, __cs;
    uint32_t eflags, esp;
    uint16_t ss, __ss;
};

typedef struct elf32_prstatus {
    uint32_t             pr_si_signo;
    uint32_t             pr_si_code;
    uint32_t             pr_si_errno;
    uint16_t             pr_cursig;
    uint16_t             __padding1;
    uint32_t             pr_sigpend;
    uint32_t             pr_sighold;
    uint32_t             pr_pid;
    uint32_t             pr_ppid;
    uint32_t             pr_pgrp;
    uint32_t             pd_sid;
    uint64_t             pr_utime;
    uint64_t             pr_stime;
    uint64_t             pr_cutime;
    uint64_t             pr_cstime;
    struct pt_regs       pr_reg;
    uint32_t             pr_fpvalid;
} Elf32_prstatus;

class Opencore : public lp32::OpencoreImpl {
public:
    Opencore() : lp32::OpencoreImpl(),
                 prnum(0), prstatus(nullptr) {}
    void Finish();
    void CreateCorePrStatus(int pid);
    void WriteCorePrStatus(FILE* fp);
    int getMachine() { return EM_386; }
private:
    int prnum;
    Elf32_prstatus *prstatus;
};

} // namespace x86

#endif // OPENCORE_X86_OPENCORE_IMPL_H_
