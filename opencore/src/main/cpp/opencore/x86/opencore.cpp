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
#define LOG_TAG "x86-opencore"
#endif

#include "eajnis/Log.h"
#include "opencore/x86/opencore.h"
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <errno.h>
#include <ucontext.h>
#include <linux/auxvec.h>

namespace x86 {

void Opencore::CreateCorePrStatus(int pid) {
    if (!threads.size()) return;

    prnum = threads.size();
    prstatus = (Elf32_prstatus *)malloc(prnum * sizeof(Elf32_prstatus));
    memset(prstatus, 0, prnum * sizeof(Elf32_prstatus));

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
                x86::pt_regs uc_regs;
                memset(&uc_regs, 0x0, sizeof(x86::pt_regs));
                uc_regs.ebx = context->uc_mcontext.gregs[8];
                uc_regs.ecx = context->uc_mcontext.gregs[10];
                uc_regs.edx = context->uc_mcontext.gregs[9];
                uc_regs.esi = context->uc_mcontext.gregs[5];
                uc_regs.edi = context->uc_mcontext.gregs[4];
                uc_regs.ebp = context->uc_mcontext.gregs[6];
                uc_regs.eax = context->uc_mcontext.gregs[11];
                uc_regs.ds = context->uc_mcontext.gregs[3];
                uc_regs.es = context->uc_mcontext.gregs[2];
                uc_regs.fs = context->uc_mcontext.gregs[1];
                uc_regs.gs = context->uc_mcontext.gregs[0];
                uc_regs.orig_eax = context->uc_mcontext.gregs[11];
                uc_regs.eip = context->uc_mcontext.gregs[14];
                uc_regs.eflags = context->uc_mcontext.gregs[16];
                uc_regs.esp = context->uc_mcontext.gregs[7];
                uc_regs.ss = context->uc_mcontext.gregs[15];
                memcpy(&prstatus[idx].pr_reg, &uc_regs, sizeof(x86::pt_regs));
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
            sizeof(x86::pt_regs),
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

int Opencore::IsSpecialFilterSegment(Opencore::VirtualMemoryArea& vma) {
    int filter = getFilter();
    if (filter & FILTER_MINIDUMP) {
        if (!prnum)
            return VMA_NULL;

        x86::pt_regs *regs = &prstatus[0].pr_reg;
        if (regs->ebx >= vma.begin && regs->ebx < vma.end
                || regs->edx >= vma.begin && regs->edx < vma.end
                || regs->esi >= vma.begin && regs->esi < vma.end
                || regs->edi >= vma.begin && regs->edi < vma.end
                || regs->ebp >= vma.begin && regs->ebp < vma.end
                || regs->eax >= vma.begin && regs->eax < vma.end
                || regs->eip >= vma.begin && regs->eip < vma.end
                || regs->esp >= vma.begin && regs->esp < vma.end) {
            return VMA_INCLUDE;
        }

        return VMA_NULL;
    }
    return VMA_NORMAL;
}

void Opencore::Finish() {
    if (prstatus) free(prstatus);
    lp32::OpencoreImpl::Finish();
}

} // namespace x86
