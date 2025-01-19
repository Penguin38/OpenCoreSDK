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
#define LOG_TAG "arm-Opencore"
#endif

#include "eajnis/Log.h"
#include "opencore/arm/opencore.h"
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <errno.h>
#include <ucontext.h>
#include <linux/auxvec.h>

namespace arm {

void Opencore::CreateCorePrStatus(int pid) {
    if (!pids.size()) return;

    prnum = pids.size();
    prstatus = (Elf32_prstatus *)malloc(prnum * sizeof(Elf32_prstatus));
    memset(prstatus, 0, prnum * sizeof(Elf32_prstatus));

    int cur = 1;
    for (int index = 0; index < prnum; index++) {
        pid_t tid = pids[index];
        int idx;
        if (tid == getTid()) {
            idx = 0;
            prstatus[idx].pr_pid = tid;
            // top thread maybe use ucontext prs
            if (getContext()) {
                struct ucontext *context = (struct ucontext *) getContext();
                memcpy(&prstatus[idx].pr_reg, &context->uc_mcontext.arm_r0, sizeof(arm::pt_regs));
                continue;
            }
        } else {
            // 0 top thread was truncated
            idx = (cur >= prnum) ? 0 : cur;
            ++cur;
            prstatus[idx].pr_pid = tid;
        }

        struct iovec ioVec = {
            &prstatus[idx].pr_reg,
            sizeof(arm::pt_regs),
        };

        if (ptrace(PTRACE_GETREGSET, tid, NT_PRSTATUS, &ioVec) < 0)
            continue;
    }

    extra_note_filesz += (sizeof(Elf32_prstatus) + sizeof(Elf32_Nhdr) + 8) * prnum;
    extra_note_filesz += sizeof(siginfo_t) + sizeof(Elf32_Nhdr) + 8;      // NT_SIGINFO
}

void Opencore::WriteCorePrStatus(FILE* fp) {
    Elf32_Nhdr elf_nhdr;
    elf_nhdr.n_namesz = NOTE_CORE_NAME_SZ;
    elf_nhdr.n_descsz = sizeof(Elf32_prstatus);
    elf_nhdr.n_type = NT_PRSTATUS;

    char magic[8];
    memset(magic, 0, sizeof(magic));
    snprintf(magic, NOTE_CORE_NAME_SZ, ELFCOREMAGIC);

    for (int index = 0; index < prnum; index++) {
        fwrite(&elf_nhdr, sizeof(Elf32_Nhdr), 1, fp);
        fwrite(magic, sizeof(magic), 1, fp);
        fwrite(&prstatus[index], sizeof(Elf32_prstatus), 1, fp);
        if (!index) WriteCoreSignalInfo(fp);
    }
}

bool Opencore::IsSpecialFilterSegment(Opencore::VirtualMemoryArea& vma, int idx) {
    int filter = getFilter();
    if (filter & FILTER_MINIDUMP) {
        if (!prnum)
            return true;

        arm::pt_regs *regs = &prstatus[0].pr_reg;
        if (regs->pc >= vma.begin && regs->pc < vma.end
                || regs->regs[0] >= vma.begin && regs->regs[0] < vma.end
                || regs->regs[1] >= vma.begin && regs->regs[1] < vma.end
                || regs->regs[2] >= vma.begin && regs->regs[2] < vma.end
                || regs->regs[3] >= vma.begin && regs->regs[3] < vma.end
                || regs->regs[4] >= vma.begin && regs->regs[4] < vma.end
                || regs->regs[5] >= vma.begin && regs->regs[5] < vma.end
                || regs->regs[6] >= vma.begin && regs->regs[6] < vma.end
                || regs->regs[7] >= vma.begin && regs->regs[7] < vma.end
                || regs->regs[8] >= vma.begin && regs->regs[8] < vma.end
                || regs->regs[9] >= vma.begin && regs->regs[9] < vma.end
                || regs->regs[10] >= vma.begin && regs->regs[10] < vma.end
                || regs->regs[11] >= vma.begin && regs->regs[11] < vma.end
                || regs->regs[12] >= vma.begin && regs->regs[12] < vma.end
                || regs->sp >= vma.begin && regs->sp < vma.end
                || regs->lr >= vma.begin && regs->lr < vma.end) {
            phdr[idx].p_filesz = phdr[idx].p_memsz;
            return false;
        }

        return true;
    }
    return false;
}

void Opencore::Finish() {
    if (prstatus) free(prstatus);
    lp32::OpencoreImpl::Finish();
}

} // namespace arm
