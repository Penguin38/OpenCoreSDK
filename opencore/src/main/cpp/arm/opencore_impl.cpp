#ifndef LOG_TAG
#define LOG_TAG "Opencore-arm"
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

OpencoreImpl* impl = new OpencoreImpl;

OpencoreImpl* OpencoreImpl::GetInstance()
{
    return impl;
}

void OpencoreImpl::ParserPhdr(int index, void *start, void *end, char* flags, char* filename)
{
    phdr[index].p_type = PT_LOAD;

    phdr[index].p_vaddr = (Elf32_Addr)start;
    phdr[index].p_paddr = 0x0;
    phdr[index].p_memsz = (Elf32_Addr)end-(Elf32_Addr)start;

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
    ntfile[index].start = (Elf32_Addr)start;
    ntfile[index].end = (Elf32_Addr)end;
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

bool OpencoreImpl::InSelfMaps(uint32_t load) {
    if (!self_maps.empty()) {
        std::map<uint32_t, std::string>::iterator iter;
        for(iter = self_maps.begin(); iter != self_maps.end(); iter++) {
            if (iter->first == load) {
                return true;
            }
        }
    }
    JNI_LOGI("[0x%x] %s Not in self.", load, maps[load].c_str());
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

        phdr = (Elf32_Phdr *)malloc(phnum * sizeof(Elf32_Phdr));
        memset(phdr, 0, phnum * sizeof(Elf32_Phdr));
        ntfile = (Elf32_ntfile *)malloc(phnum * sizeof(Elf32_ntfile));
        memset(ntfile, 0, phnum * sizeof(Elf32_ntfile));

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
    ehdr.e_ident[EI_CLASS] = ELFCLASS32;
    ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
    ehdr.e_ident[EI_VERSION] = EV_CURRENT;

    ehdr.e_type = ET_CORE;
    ehdr.e_machine = EM_ARM;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_entry = 0x0;
    ehdr.e_phoff = sizeof(Elf32_Ehdr);
    ehdr.e_shoff = 0x0;
    ehdr.e_flags = 0x0;
    ehdr.e_ehsize = sizeof(Elf32_Ehdr);
    ehdr.e_phentsize = sizeof(Elf32_Phdr);
    ehdr.e_phnum = phnum + 1;
    ehdr.e_shentsize = 0x0;
    ehdr.e_shnum = 0x0;
    ehdr.e_shstrndx = 0x0;
}

void OpencoreImpl::CreateCoreNoteHeader()
{
    note.p_type = PT_NOTE;
    note.p_offset = sizeof(Elf32_Ehdr) + ehdr.e_phnum * sizeof(Elf32_Phdr);
}

void OpencoreImpl::CreateCorePrStatus(pid_t pid)
{
    if (pids.size()) {
        prnum = pids.size();
        prstatus = (Elf32_prstatus *)malloc(prnum * sizeof(Elf32_prstatus));
        memset(prstatus, 0, prnum * sizeof(Elf32_prstatus));

        for (int index = 0; index < prnum; index++) {
            pid_t tid = pids[index];
            prstatus[index].pr_pid = tid;

            uintptr_t regset = 1;
            struct iovec ioVec;

            ioVec.iov_base = &prstatus[index].pr_reg;
            ioVec.iov_len = sizeof(core_arm_pt_regs);

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
    Elf32_auxv vec;
    snprintf(filename, sizeof(filename), "/proc/%d/auxv", pid);

    FILE *fp = fopen(filename, "rb");
    if (fp != nullptr) {
        while (fread(&vec, sizeof(vec), 1, fp)) {
            auxvnum++;
        }
        fseek(fp, 0, SEEK_SET);

        auxv = (Elf32_auxv *)malloc(auxvnum * sizeof(Elf32_auxv));
        memset(auxv, 0, auxvnum * sizeof(Elf32_auxv));

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
    fwrite((void *)&ehdr, sizeof(Elf32_Ehdr), 1, fp);
}

void OpencoreImpl::WriteCoreNoteHeader(FILE* fp)
{
    note.p_filesz = (sizeof(Elf32_prstatus) + sizeof(Elf32_Nhdr) + 8) * prnum;
    note.p_filesz += sizeof(Elf32_auxv) * auxvnum + sizeof(Elf32_Nhdr) + 8;
    note.p_filesz += sizeof(Elf32_ntfile) * phnum + sizeof(Elf32_Nhdr) + 8 + 8 + fileslen;
    fwrite((void *)&note, sizeof(Elf32_Phdr), 1, fp);
}

void OpencoreImpl::WriteCoreProgramHeaders(FILE* fp)
{
    uint32_t offset = note.p_offset + note.p_filesz;
    int page_size = sysconf(_SC_PAGE_SIZE);
    if (offset % page_size) {
        offset = ((offset + page_size)/ page_size) * page_size;
    }

    phdr[0].p_offset = offset;
    fwrite(&phdr[0], sizeof(Elf32_Phdr), 1, fp);

    int index = 1;
    while (index < ehdr.e_phnum - 1) {
        phdr[index].p_offset = phdr[index - 1].p_offset + phdr[index-1].p_filesz;
        fwrite(&phdr[index], sizeof(Elf32_Phdr), 1, fp);
        index++;
    }
}

void OpencoreImpl::WriteCorePrStatus(FILE* fp)
{
    Elf32_Nhdr elf_nhdr;
    elf_nhdr.n_namesz = NOTE_CORE_NAME_SZ;
    elf_nhdr.n_descsz = sizeof(Elf32_prstatus);
    elf_nhdr.n_type = NT_PRSTATUS;

    char magic[8];
    memset(magic, 0, sizeof(magic));
    snprintf(magic, NOTE_CORE_NAME_SZ, ELFCOREMAGIC);

    int index = 0;
    while (index < prnum) {
        fwrite(&elf_nhdr, sizeof(Elf32_Nhdr), 1, fp);
        fwrite(magic, sizeof(magic), 1, fp);
        fwrite(&prstatus[index], sizeof(Elf32_prstatus), 1, fp);
        index++;
    }
}

void OpencoreImpl::WriteCoreAUXV(FILE* fp)
{
    Elf32_Nhdr elf_nhdr;
    elf_nhdr.n_namesz = NOTE_CORE_NAME_SZ;
    elf_nhdr.n_descsz = sizeof(Elf32_auxv) * auxvnum;
    elf_nhdr.n_type = NT_AUXV;

    char magic[8];
    memset(magic, 0, sizeof(magic));
    snprintf(magic, NOTE_CORE_NAME_SZ, ELFCOREMAGIC);

    fwrite(&elf_nhdr, sizeof(Elf32_Nhdr), 1, fp);
    fwrite(magic, sizeof(magic), 1, fp);

    int index = 0;
    while (index < auxvnum) {
        fwrite(&auxv[index], sizeof(Elf32_auxv), 1, fp);
        index++;
    }
}

void OpencoreImpl::WriteNtFile(FILE* fp)
{
    Elf32_Nhdr elf_nhdr;
    elf_nhdr.n_namesz = NOTE_CORE_NAME_SZ;
    elf_nhdr.n_descsz = sizeof(Elf32_ntfile) * phnum + 2 * 4 + fileslen;
    elf_nhdr.n_type = NT_FILE;

    char magic[8];
    memset(magic, 0, sizeof(magic));
    snprintf(magic, NOTE_CORE_NAME_SZ, ELFCOREMAGIC);

    fwrite(&elf_nhdr, sizeof(Elf32_Nhdr), 1, fp);
    fwrite(magic, sizeof(magic), 1, fp);

    uint32_t number = phnum;
    fwrite(&number, 4, 1, fp);
    uint32_t page_size = sysconf(_SC_PAGE_SIZE);
    fwrite(&page_size, 4, 1, fp);

    int index = 0;
    while(index < phnum){
        fwrite(&ntfile[index], sizeof(Elf32_ntfile), 1, fp);
        index++;
    }

    fwrite(buffer.data(), buffer.size(), 1, fp);
}

void OpencoreImpl::AlignNoteSegment(FILE* fp)
{
    uint32_t offset = RoundUp(note.p_offset + note.p_filesz, sysconf(_SC_PAGE_SIZE));
    uint32_t size = offset - (note.p_offset + note.p_filesz);
    char *ptr = (char *)malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
        fwrite(ptr, size, 1, fp);
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
    bool is_compat = false;
    struct stat statbuf;

    if (mode == MODE_COPY2) {
        snprintf(filename, sizeof(filename), "/proc/%d/mem", pid);
        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            JNI_LOGE("Failed to open %s, mode switch MODE_COPY.\n", filename);
            mode = MODE_COPY;
        } else {
            if (stat("/system/lib64", &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
                JNI_LOGI("Compat mode exec arm application.");
                is_compat = true;
            }
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
                    Elf32_Addr target = phdr[index].p_vaddr;
                    while (target < phdr[index].p_vaddr + phdr[index].p_memsz) {
                        int mem = ptrace(PTRACE_PEEKTEXT, pid, target, 0x0);
                        fwrite(&mem, sizeof(mem), 1, fp);
                        target = target + sizeof(Elf32_Addr);
                    }
                } break;
                case MODE_COPY: {
                    if (InSelfMaps(phdr[index].p_vaddr)) {
                        uint32_t ret = fwrite((void *)phdr[index].p_vaddr, phdr[index].p_memsz, 1, fp);
                        if (ret != 1) {
                            need_padd_zero = true;
                            JNI_LOGE("[0x%x] write load segment fail. %s %s",
                                    phdr[index].p_vaddr, strerror(errno), maps[ntfile[index].start].c_str());
                        }
                    } else {
                        need_padd_zero = true;
                    }
                } break;
                case MODE_PTRACE | MODE_COPY: {
                    if (!InSelfMaps(phdr[index].p_vaddr)) {
                        Elf32_Addr target = phdr[index].p_vaddr;
                        while (target < phdr[index].p_vaddr + phdr[index].p_memsz) {
                            int mem = ptrace(PTRACE_PEEKTEXT, pid, target, 0x0);
                            fwrite(&mem, sizeof(mem), 1, fp);
                            target = target + sizeof(Elf32_Addr);
                        }
                    } else {
                        uint32_t ret = fwrite((void *)phdr[index].p_vaddr, phdr[index].p_memsz, 1, fp);
                        if (ret != 1) {
                            need_padd_zero = true;
                            JNI_LOGE("[0x%x] write load segment fail. %s %s",
                                    phdr[index].p_vaddr, strerror(errno), maps[ntfile[index].start].c_str());
                        }
                    }
                } break;
                case MODE_COPY2: {
                    int count = phdr[index].p_memsz / sizeof(zero);
                    for (int i = 0; i < count; i++) {
                        memset(&zero, 0x0, sizeof(zero));
                        if (is_compat) {
                            pread64(fd, &zero, sizeof(zero), phdr[index].p_vaddr + (i * sizeof(zero)));
                        } else {
                            pread(fd, &zero, sizeof(zero), phdr[index].p_vaddr + (i * sizeof(zero)));
                        }
                        uint32_t ret = fwrite(zero, sizeof(zero), 1, fp);
                        if (ret != 1) {
                            need_padd_zero = true;
                            JNI_LOGE("[0x%x] write load segment fail. %s %s",
                                    phdr[index].p_vaddr, strerror(errno), maps[ntfile[index].start].c_str());
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
                    uint32_t ret = fwrite(zero, sizeof(zero), 1, fp);
                    if (ret != 1) {
                        JNI_LOGE("[0x%x] padding load segment fail. %s %s",
                                phdr[index].p_vaddr, strerror(errno), maps[ntfile[index].start].c_str());
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
    memset(&ehdr, 0, sizeof(Elf32_Ehdr));
    memset(&note, 0, sizeof(Elf32_Phdr));
    phnum = 0;
    prnum = 0;
    auxvnum = 0;
    fileslen = 0;
    pids.clear();
    buffer.clear();
    maps.clear();
    self_maps.clear();
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

    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)mem;
    if (strncmp(mem, ELFMAG, 4) || ehdr->e_machine != EM_ARM) {
        munmap(mem, sb.st_size);
        return true;
    }

    bool ret = true;
    Elf32_Phdr* phdr = (Elf32_Phdr *)(mem + ehdr->e_phoff);
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
