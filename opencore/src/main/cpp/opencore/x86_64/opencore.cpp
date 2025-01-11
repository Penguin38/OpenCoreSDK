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
#define LOG_TAG "x86_64-Opencore"
#endif

#include "eajnis/Log.h"
#include "opencore/x86_64/opencore.h"
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <errno.h>
#include <ucontext.h>

namespace x86_64 {

void Opencore::CreateCorePrStatus(int pid) {
    if (!pids.size()) return;

    prnum = pids.size();
    prstatus = (Elf64_prstatus *)malloc(prnum * sizeof(Elf64_prstatus));
    memset(prstatus, 0, prnum * sizeof(Elf64_prstatus));

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
                x86_64::pt_regs uc_regs;
                memset(&uc_regs, 0x0, sizeof(x86_64::pt_regs));
                uc_regs.r15 = context->uc_mcontext.gregs[7];
                uc_regs.r14 = context->uc_mcontext.gregs[6];
                uc_regs.r13 = context->uc_mcontext.gregs[5];
                uc_regs.r12 = context->uc_mcontext.gregs[4];
                uc_regs.rbp = context->uc_mcontext.gregs[10];
                uc_regs.rbx = context->uc_mcontext.gregs[11];
                uc_regs.r11 = context->uc_mcontext.gregs[3];
                uc_regs.r10 = context->uc_mcontext.gregs[2];
                uc_regs.r9 = context->uc_mcontext.gregs[1];
                uc_regs.r8 = context->uc_mcontext.gregs[0];
                uc_regs.rax = context->uc_mcontext.gregs[13];
                uc_regs.rcx = context->uc_mcontext.gregs[14];
                uc_regs.rdx = context->uc_mcontext.gregs[12];
                uc_regs.rsi = context->uc_mcontext.gregs[9];
                uc_regs.rdi = context->uc_mcontext.gregs[8];
                uc_regs.rip = context->uc_mcontext.gregs[16];
                uc_regs.cs = context->uc_mcontext.gregs[18];
                uc_regs.flags = context->uc_mcontext.gregs[17];
                uc_regs.rsp = context->uc_mcontext.gregs[15];
                uc_regs.ss = context->uc_mcontext.gregs[21];
                uc_regs.fs = context->uc_mcontext.gregs[20];
                uc_regs.gs = context->uc_mcontext.gregs[19];
                memcpy(&prstatus[idx].pr_reg, &uc_regs, sizeof(x86_64::pt_regs));
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
            sizeof(x86_64::pt_regs),
        };

        if (ptrace(PTRACE_GETREGSET, tid, NT_PRSTATUS, &ioVec) < 0) {
            JNI_LOGI("%s %d: %s", __func__ , tid, strerror(errno));
            continue;
        }
    }

    extra_note_filesz += (sizeof(Elf64_prstatus) + sizeof(Elf64_Nhdr) + 8) * prnum;
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
    }
}

void Opencore::Finish() {
    if (prstatus) free(prstatus);
    lp64::OpencoreImpl::Finish();
}

} // namespace x86_64
