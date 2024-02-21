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
#define LOG_TAG "Opencore-arm64"
#endif

#include "opencore_impl.h"
#include "eajnis/Log.h"

#include <unistd.h>
#include <dirent.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define NT_ARM_TAGGED_ADDR_CTRL 0x409
#define NT_ARM_PAC_ENABLED_KEYS 0x40A
#define GENMASK(h, l) (((1ULL<<(h+1))-1)&(~((1ULL<<l)-1)))

OpencoreImpl* impl = new OpencoreImpl;

OpencoreImpl* OpencoreImpl::GetInstance()
{
    return impl;
}

void OpencoreImpl::ParserPhdr(int index, void *start, void *end, char* flags, char* filename)
{
    phdr[index].p_type = PT_LOAD;

    phdr[index].p_vaddr = (Elf64_Addr)start;
    phdr[index].p_paddr = 0x0;
    phdr[index].p_memsz = (Elf64_Addr)end-(Elf64_Addr)start;

    if (flags[0] == 'r' || flags[0] == 'R')
        phdr[index].p_flags = phdr[index].p_flags | PF_R;

    if (flags[1] == 'w' || flags[1] == 'W')
        phdr[index].p_flags = phdr[index].p_flags | PF_W;

    if (flags[2] == 'x' || flags[2] == 'X')
        phdr[index].p_flags = phdr[index].p_flags | PF_X;

    if ((phdr[index].p_flags) & PF_R)
        phdr[index].p_filesz = phdr[index].p_memsz;

    phdr[index].p_align = sysconf(_SC_PAGE_SIZE);
}

void OpencoreImpl::ParserNtFile(int index, void *start, void *end, int fileofs, char* filename)
{
    ntfile[index].start = (Elf64_Addr)start;
    ntfile[index].end = (Elf64_Addr)end;
    ntfile[index].fileofs = fileofs >> 12;

    int len = strlen(filename);
    buffer.insert(buffer.end(), filename, filename + len);
    char empty[1] = {'\0'};
    buffer.insert(buffer.end(), empty, empty + 1);
    fileslen += len + 1;
    maps[ntfile[index].start] = filename;
}

void OpencoreImpl::AlignNtFile()
{
    int len = RoundUp(fileslen, 4);
    if (len > fileslen) {
        fileslen = len;
        int zero = fileslen - buffer.size();
        char empty[1] = {'\0'};
        for (int i = 0; i < zero; i++) {
            buffer.insert(buffer.end(), empty, empty + 1);
        }
    }
}

void OpencoreImpl::ParseSelfMapsVma()
{
    char line[1024];
    FILE *fp = fopen("/proc/self/maps", "r");
    if (fp) {
        int index = 0;
        while (fgets(line, sizeof(line), fp)) {
            int m;
            void *start;
            char filename[256];
            sscanf(line, "%p-%*p %*c%*c%*c%*c %*x %*x:%*x  %*u %[^\n] %n", &start, filename, &m);
            self_maps[(uint64_t)start] = filename;
            index++;
        }
        fclose(fp);
    }
}

bool OpencoreImpl::InSelfMaps(uint64_t load) {
    if (!self_maps.empty()) {
        std::map<uint64_t, std::string>::iterator iter;
        for(iter = self_maps.begin(); iter != self_maps.end(); iter++) {
            if (iter->first == load) {
                return true;
            }
        }
    }
    JNI_LOGI("[%p] %s Not in self.", (void *)load, maps[load].c_str());
    return false;
}

void OpencoreImpl::ParseProcessMapsVma(pid_t pid)
{
    char filename[32];
    char line[1024];

    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    FILE *fp = fopen(filename, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            phnum++;
        }
        fseek(fp, 0, SEEK_SET);

        phdr = (Elf64_Phdr *)malloc(phnum * sizeof(Elf64_Phdr));
        memset(phdr, 0, phnum * sizeof(Elf64_Phdr));
        ntfile = (Elf64_ntfile *)malloc(phnum * sizeof(Elf64_ntfile));
        memset(ntfile, 0, phnum * sizeof(Elf64_ntfile));

        int index = 0;
        while (fgets(line, sizeof(line), fp)) {
            int m, fileofs, inode;
            void *start, *end;
            char flags[4];
            char filename[256];

            sscanf(line, "%p-%p %c%c%c%c %x %*x:%*x  %u %[^\n] %n",
                   &start, &end,
                   &flags[0], &flags[1], &flags[2], &flags[3],
                   &fileofs, &inode, filename, &m);

            ParserPhdr(index, start, end, flags, filename);
            ParserNtFile(index, start, end, fileofs, filename);

            if (Opencore::IsFilterSegment(flags, inode, filename, fileofs)) {
                phdr[index].p_filesz = 0x0;
            }

            index++;
        }
        AlignNtFile();
        fclose(fp);
    }
}

void OpencoreImpl::CreateCoreHeader()
{
    snprintf((char *)ehdr.e_ident, 5, ELFMAG);
    ehdr.e_ident[EI_CLASS] = ELFCLASS64;
    ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
    ehdr.e_ident[EI_VERSION] = EV_CURRENT;

    ehdr.e_type = ET_CORE;
    ehdr.e_machine = EM_AARCH64;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_entry = 0x0;
    ehdr.e_phoff = sizeof(Elf64_Ehdr);
    ehdr.e_shoff = 0x0;
    ehdr.e_flags = 0x0;
    ehdr.e_ehsize = sizeof(Elf64_Ehdr);
    ehdr.e_phentsize = sizeof(Elf64_Phdr);
    ehdr.e_phnum = phnum + 1;
    ehdr.e_shentsize = 0x0;
    ehdr.e_shnum = 0x0;
    ehdr.e_shstrndx = 0x0;
}

void OpencoreImpl::CreateCoreNoteHeader()
{
    note.p_type = PT_NOTE;
    note.p_offset = sizeof(Elf64_Ehdr) + ehdr.e_phnum * sizeof(Elf64_Phdr);
}

void OpencoreImpl::CreateCorePrStatus(pid_t pid)
{
    if (pids.size()) {
        prnum = pids.size();
        prstatus = (Elf64_prstatus *)malloc(prnum * sizeof(Elf64_prstatus));
        memset(prstatus, 0, prnum * sizeof(Elf64_prstatus));

        for (int index = 0; index < prnum; index++) {
            pid_t tid = pids[index];
            prstatus[index].pr_pid = tid;

            uintptr_t regset = 1;
            struct iovec ioVec;

            ioVec.iov_base = &prstatus[index].pr_reg;
            ioVec.iov_len = sizeof(core_arm64_pt_regs);

            if (ptrace(PTRACE_GETREGSET, tid, regset, &ioVec) < 0) {
                JNI_LOGI("%s %d: %s\n", __func__ , tid, strerror(errno));
                continue;
            }
        }
    }
}

void OpencoreImpl::CreateCoreAUXV(pid_t pid)
{
    char filename[32];
    Elf64_auxv vec;
    snprintf(filename, sizeof(filename), "/proc/%d/auxv", pid);

    FILE *fp = fopen(filename, "rb");
    if (fp != nullptr) {
        while (fread(&vec, sizeof(vec), 1, fp)) {
            auxvnum++;
        }
        fseek(fp, 0, SEEK_SET);

        auxv = (Elf64_auxv *)malloc(auxvnum * sizeof(Elf64_auxv));
        memset(auxv, 0, auxvnum * sizeof(Elf64_auxv));

        int index =0;
        while (fread(&vec, sizeof(vec), 1, fp)) {
            auxv[index].a_type = vec.a_type;
            auxv[index].a_val = vec.a_val;
            index++;
        }

        fclose(fp);
    }
}

void OpencoreImpl::WriteCoreHeader(FILE* fp)
{
    WriteAndRecord((void *)&ehdr, sizeof(Elf64_Ehdr), 1, fp);
}

void OpencoreImpl::WriteCoreNoteHeader(FILE* fp)
{
    note.p_filesz = (sizeof(Elf64_prstatus) + sizeof(Elf64_Nhdr) + 8) * prnum;
    note.p_filesz += sizeof(Elf64_auxv) * auxvnum + sizeof(Elf64_Nhdr) + 8;
    note.p_filesz += ((sizeof(user_pac_mask) + sizeof(Elf64_Nhdr) + 8)  // NT_ARM_PAC_MASK
                     +(sizeof(uint64_t) + sizeof(Elf64_Nhdr) + 8)       // NT_ARM_PAC_ENABLED_KEYS
                     ) * prnum;
    note.p_filesz += (sizeof(uint64_t) + sizeof(Elf64_Nhdr) + 8) * prnum;  // NT_ARM_TAGGED_ADDR_CTRL
    note.p_filesz += sizeof(Elf64_ntfile) * phnum + sizeof(Elf64_Nhdr) + 8 + 2 * sizeof(unsigned long) + fileslen;
    WriteAndRecord((void *)&note, sizeof(Elf64_Phdr), 1, fp);
}

void OpencoreImpl::WriteCoreProgramHeaders(FILE* fp)
{
    uint64_t offset = note.p_offset + note.p_filesz;
    int page_size = sysconf(_SC_PAGE_SIZE);
    if (offset % page_size) {
        offset = ((offset + page_size)/ page_size) * page_size;
    }

    phdr[0].p_offset = offset;
    WriteAndRecord(&phdr[0], sizeof(Elf64_Phdr), 1, fp);

    int index = 1;
    while (index < ehdr.e_phnum - 1) {
        phdr[index].p_offset = phdr[index - 1].p_offset + phdr[index-1].p_filesz;
        WriteAndRecord(&phdr[index], sizeof(Elf64_Phdr), 1, fp);
        index++;
    }
}

void OpencoreImpl::WriteCorePrStatus(FILE* fp)
{
    Elf64_Nhdr elf_nhdr;
    elf_nhdr.n_namesz = NOTE_CORE_NAME_SZ;
    elf_nhdr.n_descsz = sizeof(Elf64_prstatus);
    elf_nhdr.n_type = NT_PRSTATUS;

    char magic[8];
    memset(magic, 0, sizeof(magic));
    snprintf(magic, NOTE_CORE_NAME_SZ, ELFCOREMAGIC);

    int index = 0;
    while (index < prnum) {
        WriteAndRecord(&elf_nhdr, sizeof(Elf64_Nhdr), 1, fp);
        WriteAndRecord(magic, sizeof(magic), 1, fp);
        WriteAndRecord(&prstatus[index], sizeof(Elf64_prstatus), 1, fp);
        WriteCorePAC(prstatus[index].pr_pid ,fp);
        WriteCoreMTE(prstatus[index].pr_pid ,fp);
        index++;
    }
}

void OpencoreImpl::WriteCoreAUXV(FILE* fp)
{
    Elf64_Nhdr elf_nhdr;
    elf_nhdr.n_namesz = NOTE_CORE_NAME_SZ;
    elf_nhdr.n_descsz = sizeof(Elf64_auxv) * auxvnum;
    elf_nhdr.n_type = NT_AUXV;

    char magic[8];
    memset(magic, 0, sizeof(magic));
    snprintf(magic, NOTE_CORE_NAME_SZ, ELFCOREMAGIC);

    WriteAndRecord(&elf_nhdr, sizeof(Elf64_Nhdr), 1, fp);
    WriteAndRecord(magic, sizeof(magic), 1, fp);

    int index = 0;
    while (index < auxvnum) {
        WriteAndRecord(&auxv[index], sizeof(Elf64_auxv), 1, fp);
        index++;
    }
}

void OpencoreImpl::WriteCorePAC(pid_t tid, FILE* fp)
{
    // NT_ARM_PAC_MASK
    Elf64_Nhdr elf_nhdr;
    elf_nhdr.n_namesz = NOTE_LINUX_NAME_SZ;
    elf_nhdr.n_descsz = sizeof(user_pac_mask);
    elf_nhdr.n_type = NT_ARM_PAC_MASK;

    char magic[8];
    memset(magic, 0, sizeof(magic));
    snprintf(magic, NOTE_LINUX_NAME_SZ, ELFLINUXMAGIC);

    WriteAndRecord(&elf_nhdr, sizeof(Elf64_Nhdr), 1, fp);
    WriteAndRecord(magic, sizeof(magic), 1, fp);

    user_pac_mask uregs;
    struct iovec pac_mask_iov = {
        &uregs,
        sizeof(user_pac_mask),
    };
    if (ptrace(PTRACE_GETREGSET, tid, NT_ARM_PAC_MASK,
                reinterpret_cast<void*>(&pac_mask_iov)) == -1) {
        uint64_t mask = GENMASK(54, DEF_VA_BITS);
        uregs.data_mask = mask;
        uregs.insn_mask = mask;
    }
    WriteAndRecord(&uregs, sizeof(user_pac_mask), 1, fp);

    // NT_ARM_PAC_ENABLED_KEYS
    elf_nhdr.n_descsz = sizeof(uint64_t);
    elf_nhdr.n_type = NT_ARM_PAC_ENABLED_KEYS;

    WriteAndRecord(&elf_nhdr, sizeof(Elf64_Nhdr), 1, fp);
    WriteAndRecord(magic, sizeof(magic), 1, fp);

    uint64_t pac_enabled_keys;
    struct iovec pac_enabled_keys_iov = {
        &pac_enabled_keys,
        sizeof(pac_enabled_keys),
    };
    if (ptrace(PTRACE_GETREGSET, tid, NT_ARM_PAC_ENABLED_KEYS,
                reinterpret_cast<void*>(&pac_enabled_keys_iov)) == -1) {
        pac_enabled_keys = 0;
    }
    WriteAndRecord(&pac_enabled_keys, sizeof(uint64_t), 1, fp);
}

void OpencoreImpl::WriteCoreMTE(pid_t tid, FILE* fp)
{
    // NT_ARM_TAGGED_ADDR_CTRL
    Elf64_Nhdr elf_nhdr;
    elf_nhdr.n_namesz = NOTE_LINUX_NAME_SZ;
    elf_nhdr.n_descsz = sizeof(uint64_t);
    elf_nhdr.n_type = NT_ARM_TAGGED_ADDR_CTRL;

    char magic[8];
    memset(magic, 0, sizeof(magic));
    snprintf(magic, NOTE_LINUX_NAME_SZ, ELFLINUXMAGIC);

    WriteAndRecord(&elf_nhdr, sizeof(Elf64_Nhdr), 1, fp);
    WriteAndRecord(magic, sizeof(magic), 1, fp);

    uint64_t tagged_addr_ctrl;
    struct iovec tagged_addr_ctrl_iov = {
        &tagged_addr_ctrl,
        sizeof(tagged_addr_ctrl),
    };
    if (ptrace(PTRACE_GETREGSET, tid, NT_ARM_TAGGED_ADDR_CTRL,
                reinterpret_cast<void*>(&tagged_addr_ctrl_iov)) == -1) {
    }
    WriteAndRecord(&tagged_addr_ctrl, sizeof(uint64_t), 1, fp);
}

void OpencoreImpl::WriteNtFile(FILE* fp)
{
    Elf64_Nhdr elf_nhdr;
    elf_nhdr.n_namesz = NOTE_CORE_NAME_SZ;
    elf_nhdr.n_descsz = sizeof(Elf64_ntfile) * phnum + 2 * sizeof(unsigned long) + fileslen;
    elf_nhdr.n_type = NT_FILE;

    char magic[8];
    memset(magic, 0, sizeof(magic));
    snprintf(magic, NOTE_CORE_NAME_SZ, ELFCOREMAGIC);

    WriteAndRecord(&elf_nhdr, sizeof(Elf64_Nhdr), 1, fp);
    WriteAndRecord(magic, sizeof(magic), 1, fp);

    uint64_t number = phnum;
    WriteAndRecord(&number, sizeof(unsigned long), 1, fp);
    uint64_t page_size = sysconf(_SC_PAGE_SIZE);
    WriteAndRecord(&page_size, sizeof(unsigned long), 1, fp);
    int index = 0;
    while(index < phnum){
        WriteAndRecord(&ntfile[index], sizeof(Elf64_ntfile), 1, fp);
        index++;
    }

    WriteAndRecord(buffer.data(), buffer.size(), 1, fp);
}

void OpencoreImpl::AlignNoteSegment(FILE* fp)
{
    uint64_t offset = RoundUp(note.p_offset + note.p_filesz, sysconf(_SC_PAGE_SIZE));
    uint64_t size = offset - (note.p_offset + note.p_filesz);
    char *ptr = (char *)malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
        WriteAndRecord(ptr, size, 1, fp);
        free(ptr);
    }
}

void OpencoreImpl::WriteCoreLoadSegment(pid_t pid, FILE* fp)
{
    char filename[32];
    int fd;
    int mode = GetMode();
    JNI_LOGI("%s Mode(%d)", __func__, mode);
    int index = 0;
    uint8_t zero[4096];
    memset(&zero, 0x0, sizeof(zero));

    if (mode == MODE_COPY2) {
        snprintf(filename, sizeof(filename), "/proc/%d/mem", pid);
        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            JNI_LOGE("Failed to open %s, mode switch MODE_COPY.\n", filename);
            mode = MODE_COPY;
        }
    }

    if (mode != MODE_COPY2)
        ParseSelfMapsVma();

    while(index < ehdr.e_phnum - 1) {
        if (phdr[index].p_filesz > 0) {
            long current_pos = ftell(fp);
            bool need_padd_zero = false;
            switch (mode) {
                case MODE_PTRACE: {
                    Elf64_Addr target = phdr[index].p_vaddr;
                    while (target < phdr[index].p_vaddr + phdr[index].p_memsz) {
                        long mem = ptrace(PTRACE_PEEKTEXT, pid, target, 0x0);
                        WriteAndRecord(&mem, sizeof(mem), 1, fp);
                        target = target + sizeof(Elf64_Addr);
                    }
                } break;
                case MODE_COPY: {
                    if (InSelfMaps(phdr[index].p_vaddr)) {
                        uint64_t ret = WriteAndRecord((void *)phdr[index].p_vaddr, phdr[index].p_memsz, 1, fp);
                        if (ret != 1) {
                            need_padd_zero = true;
                            JNI_LOGE("[%p] write load segment fail. %s %s",
                                    (void *)phdr[index].p_vaddr, strerror(errno), maps[ntfile[index].start].c_str());
                        }
                    } else {
                        need_padd_zero = true;
                    }
                } break;
                case MODE_PTRACE | MODE_COPY: {
                    if (!InSelfMaps(phdr[index].p_vaddr)) {
                        Elf64_Addr target = phdr[index].p_vaddr;
                        while (target < phdr[index].p_vaddr + phdr[index].p_memsz) {
                            long mem = ptrace(PTRACE_PEEKTEXT, pid, target, 0x0);
                            WriteAndRecord(&mem, sizeof(mem), 1, fp);
                            target = target + sizeof(Elf64_Addr);
                        }
                    } else {
                        uint64_t ret = WriteAndRecord((void *)phdr[index].p_vaddr, phdr[index].p_memsz, 1, fp);
                        if (ret != 1) {
                            need_padd_zero = true;
                            JNI_LOGE("[%p] write load segment fail. %s %s",
                                    (void *)phdr[index].p_vaddr, strerror(errno), maps[ntfile[index].start].c_str());
                        }
                    }
                } break;
                case MODE_COPY2: {
                    int count = phdr[index].p_memsz / sizeof(zero);
                    for (int i = 0; i < count; i++) {
                        memset(&zero, 0x0, sizeof(zero));
                        pread(fd, &zero, sizeof(zero), phdr[index].p_vaddr + (i * sizeof(zero)));
                        uint64_t ret = WriteAndRecord(zero, sizeof(zero), 1, fp);
                        if (ret != 1) {
                            need_padd_zero = true;
                            JNI_LOGE("[%p] write load segment fail. %s %s",
                                    (void *)phdr[index].p_vaddr, strerror(errno), maps[ntfile[index].start].c_str());
                            break;
                        }
                    }
                } break;
                default: {
                    // do nothing
                } break;
            }
            if (need_padd_zero && current_pos > 0) {
                memset(&zero, 0x0, sizeof(zero));
                fseek(fp, current_pos, SEEK_SET);
                int count = phdr[index].p_memsz / sizeof(zero);
                for (int i = 0; i < count; i++) {
                    uint64_t ret = WriteAndRecord(zero, sizeof(zero), 1, fp);
                    if (ret != 1) {
                        JNI_LOGE("[%p] padding load segment fail. %s %s",
                                (void *)phdr[index].p_vaddr, strerror(errno), maps[ntfile[index].start].c_str());
                    }
                }
            }
        }
        index++;
    }
    if (fd > 0)
        close(fd);
}

void OpencoreImpl::StopAllThread(pid_t pid)
{
    char task_dir[32];
    struct dirent *entry;
    snprintf(task_dir, sizeof(task_dir), "/proc/%d/task", pid);
    DIR *dp = opendir(task_dir);
    if (dp) {
        while ((entry=readdir(dp)) != NULL) {
            if (!strncmp(entry->d_name, ".", 1)) {
                continue;
            }

            pid_t tid = atoi(entry->d_name);
            if (ptrace(PTRACE_ATTACH, tid, NULL, 0) < 0) {
                JNI_LOGI("%s %d: %s\n", __func__ , tid, strerror(errno));
                continue;
            }
            pids.push_back(tid);
            int status = 0;
            waitpid(tid, &status, WUNTRACED);
        }
        closedir(dp);
    }
}

void OpencoreImpl::ContinueAllThread(pid_t pid)
{
    for (int index = 0; index < pids.size(); index++) {
        pid_t tid = pids[index];
        if (ptrace(PTRACE_DETACH, tid, NULL, 0) < 0) {
            JNI_LOGI("%s %d: %s\n", __func__ , tid, strerror(errno));
            continue;
        }
    }
}

void OpencoreImpl::Prepare(const char* filename)
{
    JNI_LOGI("Coredump %s ...", filename);
    memset(&ehdr, 0, sizeof(Elf64_Ehdr));
    memset(&note, 0, sizeof(Elf64_Phdr));
    phnum = 0;
    prnum = 0;
    auxvnum = 0;
    fileslen = 0;
    pids.clear();
    buffer.clear();
    maps.clear();
    self_maps.clear();
    SetCoreFilePath(filename);
}

void OpencoreImpl::Finish()
{
    if (auxv) {
        free(auxv);
        auxv = nullptr;
    }

    if (phdr) {
        free(phdr);
        phdr = nullptr;
    }

    if (prstatus) {
        free(prstatus);
        prstatus = nullptr;
    }

    if (ntfile) {
        free(ntfile);
        ntfile = nullptr;
    }
    RemoveCoreFileSuffix();
    JNI_LOGI("Coredump Done.");
}

bool OpencoreImpl::DoCoreDump(const char* filename)
{
    pid_t child = fork();
    if (child == 0) {
        signal(SIGALRM, Opencore::TimeoutHandle);
        alarm(GetTimeout());

        disable();
        Prepare(filename);
        StopAllThread(getppid());

        FILE* fp = fopen(filename, "wb");
        if (fp) {
            ParseProcessMapsVma(getppid());
            CreateCoreHeader();
            CreateCoreNoteHeader();
            CreateCorePrStatus(getppid());
            CreateCoreAUXV(getppid());

            // ELF Header
            WriteCoreHeader(fp);

            // Program Headers
            WriteCoreNoteHeader(fp);
            WriteCoreProgramHeaders(fp);

            // Segments
            WriteCorePrStatus(fp);
            WriteCoreAUXV(fp);
            WriteNtFile(fp);
            AlignNoteSegment(fp);
            WriteCoreLoadSegment(getppid(), fp);

            Finish();
            fclose(fp);
        } else {
            JNI_LOGE("%s %s: %s\n", __func__ , filename, strerror(errno));
        }

        // ContinueAllThread(getppid());
        _exit(0);
    } else if (child > 0) {
        JNI_LOGI("Wait (%d) coredump", child);
        int status = 0;
        wait(&status);
    }
    return true;
}

bool OpencoreImpl::NeedFilterFile(const char* filename, int offset)
{
    struct stat sb;
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
        return true;

    if (fstat(fd, &sb) < 0) {
        close(fd);
        return true;
    }

    char* mem = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0x0);
    close(fd);
    if (mem == MAP_FAILED)
        return true;

    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)mem;
    if (strncmp(mem, ELFMAG, 4) || ehdr->e_machine != EM_AARCH64) {
        munmap(mem, sb.st_size);
        return true;
    }

    bool ret = true;
    Elf64_Phdr* phdr = (Elf64_Phdr *)(mem + ehdr->e_phoff);
    for (int index = 0; index < ehdr->e_phnum; index++) {
        if (phdr[index].p_type != PT_LOAD)
            continue;

        int pos = RoundDown(phdr[index].p_offset, 0x1000);
        int end = RoundUp(phdr[index].p_offset + phdr[index].p_memsz, 0x1000);
        if (pos <= offset && offset < end) {
            if ((phdr[index].p_flags & PF_W))
                ret = false;
        }
    }

    munmap(mem, sb.st_size);
    return ret;
}

int OpencoreImpl::WriteAndRecord(const void *buf, size_t size, size_t count, FILE *stream) {
    if (GetFileLimit() >= 0 && file_size + size * count > GetFileLimit()) {
        auto remaining = GetFileLimit() - file_size;
        JNI_LOGE("Core file size exceeds limit: write %zu bytes and %ld bytes until limit, "
                 "core limit %d, truncated", size * count, remaining,  GetFileLimit());
        size = remaining > 0 ? remaining / count : 0;

        fwrite(buf, size, count, stream);
        Finish();

        AddCoreFileSuffix("truncated");
        _exit(0);
    }
    int ret = fwrite(buf, size, count, stream);
    file_size += size * count;
    return ret;
}
