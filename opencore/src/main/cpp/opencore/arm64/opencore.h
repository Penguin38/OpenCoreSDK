/*
 * Copyright (C) 2024-present, Guanyou.Chen. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file ercept in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either erpress or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OPENCORE_ARM64_OPENCORE_IMPL_H_
#define OPENCORE_ARM64_OPENCORE_IMPL_H_

#include "opencore/lp64/opencore.h"

namespace arm64 {

struct pt_regs {
    uint64_t  regs[31];
    uint64_t  sp;
    uint64_t  pc;
    uint64_t  pstate;
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
    void WriteCorePAC(int tid, FILE* fp);
    void WriteCoreMTE(int tid, FILE* fp);
    int getMachine() { return EM_AARCH64; }
private:
    int prnum;
    Elf64_prstatus *prstatus;
};

} // namespace arm64

#endif // OPENCORE_ARM64_OPENCORE_IMPL_H_
