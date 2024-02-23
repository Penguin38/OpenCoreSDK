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

#ifndef OPENCORESDK_OPENCORE_IMPL_H
#define OPENCORESDK_OPENCORE_IMPL_H

#include "../opencore.h"
#include <sys/types.h>
#include <linux/elf.h>
#include <vector>
#include <map>

struct core_x86_64_pt_regs {
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
	uint64_t cs;
	uint64_t flags;
	uint64_t rsp;
	uint64_t ss;
	uint64_t fs_base;
	uint64_t gs_base;
	uint64_t ds;
	uint64_t es;
	uint64_t fs;
	uint64_t gs;
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
    struct core_x86_64_pt_regs  pr_reg;
    uint32_t             pr_fpvalid;
} Elf64_prstatus;

typedef struct elf64_auxv {
    uint64_t a_type;
    uint64_t a_val;
} Elf64_auxv;

typedef struct elf64_ntfile{
    uint64_t start;
    uint64_t end;
    uint64_t fileofs;
} Elf64_ntfile;

constexpr uint64_t RoundDown(uint64_t x, uint64_t n) {
    return (x & -n);
}

constexpr uint64_t RoundUp(uint64_t x, uint64_t n) {
    return RoundDown(x + n - 1, n);
}

class OpencoreImpl : public Opencore {
public:
    static OpencoreImpl* GetInstance();
    bool DoCoreDump(const char* filename);
    bool NeedFilterFile(const char* filename, int offset);
    void StopAllThread(pid_t pid);
    void ContinueAllThread(pid_t pid);
    void Prepare(const char* filename);
    void ParseSelfMapsVma();
    bool InSelfMaps(uint64_t load);
    void ParseProcessMapsVma(pid_t pid);
    void ParserPhdr(int index, void *start, void *end, char* flags, char* filename);
    void ParserNtFile(int index, void *start, void *end, int fileofs, char* filename);
    void AlignNtFile();
    void CreateCoreHeader();
    void CreateCoreNoteHeader();
    void CreateCorePrStatus(pid_t pid);
    void CreateCoreAUXV(pid_t pid);

    // ELF Header
    void WriteCoreHeader(FILE* fp);

    // Program Headers
    void WriteCoreNoteHeader(FILE* fp);
    void WriteCoreProgramHeaders(FILE* fp);

    // Segments
    void WriteCorePrStatus(FILE* fp);
    void WriteCoreAUXV(FILE* fp);
    void WriteNtFile(FILE* fp);
    void AlignNoteSegment(FILE* fp);
    void WriteCoreLoadSegment(pid_t pid, FILE* fp);

    void Finish();
private:
    Elf64_Ehdr ehdr;
    Elf64_Phdr *phdr;
    int phnum;

    Elf64_Phdr note;
    Elf64_prstatus *prstatus;
    int prnum;
    Elf64_auxv *auxv;
    int auxvnum;
    Elf64_ntfile *ntfile;
    int fileslen;
    std::vector<pid_t> pids;
    std::vector<uint8_t> buffer;
    std::map<uint64_t, std::string> maps;
    std::map<uint64_t, std::string> self_maps;
};

#endif //OPENCORESDK_OPENCORE_IMPL_H
