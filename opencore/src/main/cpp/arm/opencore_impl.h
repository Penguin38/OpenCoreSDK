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

struct core_arm_pt_regs {
    uint32_t  regs[13];
    uint32_t  sp;
    uint32_t  lr;
    uint32_t  pc;
    uint32_t  cpsr;
} __attribute__((aligned(1)));

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
    struct core_arm_pt_regs  pr_reg;
    uint32_t             pr_fpvalid;
    uint32_t             __padding2;
} __attribute__((packed, aligned(1))) Elf32_prstatus;

typedef struct elf32_auxv {
    uint32_t a_type;
    uint32_t a_val;
} Elf32_auxv;

typedef struct elf32_ntfile{
    uint32_t start;
    uint32_t end;
    uint32_t fileofs;
} Elf32_ntfile;

constexpr uint32_t RoundDown(uint32_t x, uint32_t n) {
    return (x & -n);
}

constexpr uint32_t RoundUp(uint32_t x, uint32_t n) {
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
    bool InSelfMaps(uint32_t load);
    void ParseProcessMapsVma(pid_t pid);
    void ParserPhdr(int index, void *start, void *end, char* flags, char* filename);
    void ParserNtFile(int index, void *start, void *end, int fileofs, char* filename);
    void AlignNtFile();
    void CreateCoreHeader();
    void CreateCoreNoteHeader();
    void CreateCorePrStatus(pid_t pid);
    void CreateCoreAUXV(pid_t pid);
    int WriteAndRecord(const void *buf, size_t size, size_t count, FILE *stream);
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
    Elf32_Ehdr ehdr;
    Elf32_Phdr *phdr;
    int phnum;

    Elf32_Phdr note;
    Elf32_prstatus *prstatus;
    int prnum;
    Elf32_auxv *auxv;
    int auxvnum;
    Elf32_ntfile *ntfile;
    int fileslen;
    std::vector<pid_t> pids;
    std::vector<uint8_t> buffer;
    std::map<uint32_t, std::string> maps;
    std::map<uint32_t, std::string> self_maps;
};

#endif //OPENCORESDK_OPENCORE_IMPL_H
