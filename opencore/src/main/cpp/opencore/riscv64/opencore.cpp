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
#define LOG_TAG "riscv64-opencore"
#endif

#include "eajnis/Log.h"
#include "opencore/riscv64/opencore.h"
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <errno.h>
#include <ucontext.h>
#include <linux/auxvec.h>

namespace riscv64 {

void Opencore::CreateCorePrStatus(int pid) {
    if (!threads.size()) return;

    prnum = threads.size();
    prstatus = (Elf64_prstatus *)malloc(prnum * sizeof(Elf64_prstatus));
    memset(prstatus, 0, prnum * sizeof(Elf64_prstatus));

    int cur = 1;
    for (int index = 0; index < prnum; index++) {
        pid_t tid = threads[index].pid;
        int idx;
        if (tid == getTid()) {
            idx = 0;
            prstatus[idx].pr_pid = tid;
            // top thread maybe use ucontext prs
            if (getContext()) {
                struct ucontext *context = (struct ucontext *) getContext();
                memcpy(&prstatus[idx].pr_reg, &context->uc_mcontext.sc_regs, sizeof(riscv64::pt_regs));
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
            sizeof(riscv64::pt_regs),
        };

        if (ptrace(PTRACE_GETREGSET, tid, NT_PRSTATUS, &ioVec) < 0)
            continue;
    }

    extra_note_filesz += (sizeof(Elf64_prstatus) + sizeof(Elf64_Nhdr) + 8) * prnum;
    extra_note_filesz += sizeof(siginfo_t) + sizeof(Elf64_Nhdr) + 8;      // NT_SIGINFO
}

void Opencore::WriteCorePrStatus(FILE* fp) {
    Elf64_Nhdr elf_nhdr;
    elf_nhdr.n_namesz = NOTE_CORE_NAME_SZ;
    elf_nhdr.n_descsz = sizeof(Elf64_prstatus);
    elf_nhdr.n_type = NT_PRSTATUS;

    char magic[8];
    memset(magic, 0, sizeof(magic));
    snprintf(magic, NOTE_CORE_NAME_SZ, ELFCOREMAGIC);

    for (int index = 0; index < prnum; index++) {
        fwrite(&elf_nhdr, sizeof(Elf64_Nhdr), 1, fp);
        fwrite(magic, sizeof(magic), 1, fp);
        fwrite(&prstatus[index], sizeof(Elf64_prstatus), 1, fp);
        if (!index) WriteCoreSignalInfo(fp);
    }
}

int Opencore::IsSpecialFilterSegment(Opencore::VirtualMemoryArea& vma) {
    int filter = getFilter();
    if (filter & FILTER_MINIDUMP) {
        if (!prnum)
            return VMA_NULL;

        riscv64::pt_regs *regs = &prstatus[0].pr_reg;
        if (regs->pc >= vma.begin && regs->pc < vma.end
                || regs->ra >= vma.begin && regs->ra < vma.end
                || regs->sp >= vma.begin && regs->sp < vma.end
                || regs->gp >= vma.begin && regs->gp < vma.end
                || regs->tp >= vma.begin && regs->tp < vma.end
                || regs->t0 >= vma.begin && regs->t0 < vma.end
                || regs->t1 >= vma.begin && regs->t1 < vma.end
                || regs->t2 >= vma.begin && regs->t2 < vma.end
                || regs->s0 >= vma.begin && regs->s0 < vma.end
                || regs->s1 >= vma.begin && regs->s1 < vma.end
                || regs->a0 >= vma.begin && regs->a0 < vma.end
                || regs->a1 >= vma.begin && regs->a1 < vma.end
                || regs->a2 >= vma.begin && regs->a2 < vma.end
                || regs->a3 >= vma.begin && regs->a3 < vma.end
                || regs->a4 >= vma.begin && regs->a4 < vma.end
                || regs->a5 >= vma.begin && regs->a5 < vma.end
                || regs->a6 >= vma.begin && regs->a6 < vma.end
                || regs->a7 >= vma.begin && regs->a7 < vma.end
                || regs->s2 >= vma.begin && regs->s2 < vma.end
                || regs->s3 >= vma.begin && regs->s3 < vma.end
                || regs->s4 >= vma.begin && regs->s4 < vma.end
                || regs->s5 >= vma.begin && regs->s5 < vma.end
                || regs->s6 >= vma.begin && regs->s6 < vma.end
                || regs->s7 >= vma.begin && regs->s7 < vma.end
                || regs->s8 >= vma.begin && regs->s8 < vma.end
                || regs->s9 >= vma.begin && regs->s9 < vma.end
                || regs->s10 >= vma.begin && regs->s10 < vma.end
                || regs->s11 >= vma.begin && regs->s11 < vma.end
                || regs->t3 >= vma.begin && regs->t3 < vma.end
                || regs->t4 >= vma.begin && regs->t4 < vma.end
                || regs->t5 >= vma.begin && regs->t5 < vma.end
                || regs->t6 >= vma.begin && regs->t6 < vma.end) {
            return VMA_INCLUDE;
        }

        return VMA_NULL;
    }
    return VMA_NORMAL;
}

void Opencore::Finish() {
    if (prstatus) free(prstatus);
    lp64::OpencoreImpl::Finish();
}

} // namespace riscv64
