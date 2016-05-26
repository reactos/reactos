/*
 * File minidump.c - management of dumps (read & write)
 *
 * Copyright (C) 2004-2005, Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dbghelp_private.h"

#include <time.h>

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

/******************************************************************
 *		fetch_process_info
 *
 * reads system wide process information, and gather from it the threads information
 * for process of id 'pid'
 */
static BOOL fetch_process_info(struct dump_context* dc)
{
    ULONG       buf_size = 0x1000;
    NTSTATUS    nts;
    void*       pcs_buffer = NULL;

    if (!(pcs_buffer = HeapAlloc(GetProcessHeap(), 0, buf_size))) return FALSE;
    for (;;)
    {
        nts = NtQuerySystemInformation(SystemProcessInformation,
                                       pcs_buffer, buf_size, NULL);
        if (nts != STATUS_INFO_LENGTH_MISMATCH) break;
        pcs_buffer = HeapReAlloc(GetProcessHeap(), 0, pcs_buffer, buf_size *= 2);
        if (!pcs_buffer) return FALSE;
    }

    if (nts == STATUS_SUCCESS)
    {
        SYSTEM_PROCESS_INFORMATION*     spi = pcs_buffer;
        unsigned                        i;

        for (;;)
        {
            if (HandleToUlong(spi->UniqueProcessId) == dc->pid)
            {
                dc->num_threads = spi->dwThreadCount;
                dc->threads = HeapAlloc(GetProcessHeap(), 0,
                                        dc->num_threads * sizeof(dc->threads[0]));
                if (!dc->threads) goto failed;
                for (i = 0; i < dc->num_threads; i++)
                {
                    dc->threads[i].tid        = HandleToULong(spi->ti[i].ClientId.UniqueThread);
                    dc->threads[i].prio_class = spi->ti[i].dwBasePriority; /* FIXME */
                    dc->threads[i].curr_prio  = spi->ti[i].dwCurrentPriority;
                }
                HeapFree(GetProcessHeap(), 0, pcs_buffer);
                return TRUE;
            }
            if (!spi->NextEntryOffset) break;
            spi = (SYSTEM_PROCESS_INFORMATION*)((char*)spi + spi->NextEntryOffset);
        }
    }
failed:
    HeapFree(GetProcessHeap(), 0, pcs_buffer);
    return FALSE;
}

static void fetch_thread_stack(struct dump_context* dc, const void* teb_addr,
                               const CONTEXT* ctx, MINIDUMP_MEMORY_DESCRIPTOR* mmd)
{
    NT_TIB      tib;
    ADDRESS64   addr;

    if (ReadProcessMemory(dc->hProcess, teb_addr, &tib, sizeof(tib), NULL) &&
        dbghelp_current_cpu &&
        dbghelp_current_cpu->get_addr(NULL /* FIXME */, ctx, cpu_addr_stack, &addr) && addr.Mode == AddrModeFlat)
    {
        if (addr.Offset)
        {
            addr.Offset -= dbghelp_current_cpu->word_size;
            /* make sure stack pointer is within the established range of the stack.  It could have
               been clobbered by whatever caused the original exception. */
            if (addr.Offset < (ULONG_PTR)tib.StackLimit || addr.Offset > (ULONG_PTR)tib.StackBase)
                mmd->StartOfMemoryRange = (ULONG_PTR)tib.StackLimit;

            else
                mmd->StartOfMemoryRange = addr.Offset;
        }
        else
            mmd->StartOfMemoryRange = (ULONG_PTR)tib.StackLimit;
        mmd->Memory.DataSize = (ULONG_PTR)tib.StackBase - mmd->StartOfMemoryRange;
    }
}

/******************************************************************
 *		fetch_thread_info
 *
 * fetches some information about thread of id 'tid'
 */
static BOOL fetch_thread_info(struct dump_context* dc, int thd_idx,
                              const MINIDUMP_EXCEPTION_INFORMATION* except,
                              MINIDUMP_THREAD* mdThd, CONTEXT* ctx)
{
    DWORD                       tid = dc->threads[thd_idx].tid;
    HANDLE                      hThread;
    THREAD_BASIC_INFORMATION    tbi;

    memset(ctx, 0, sizeof(*ctx));

    mdThd->ThreadId = tid;
    mdThd->SuspendCount = 0;
    mdThd->Teb = 0;
    mdThd->Stack.StartOfMemoryRange = 0;
    mdThd->Stack.Memory.DataSize = 0;
    mdThd->Stack.Memory.Rva = 0;
    mdThd->ThreadContext.DataSize = 0;
    mdThd->ThreadContext.Rva = 0;
    mdThd->PriorityClass = dc->threads[thd_idx].prio_class;
    mdThd->Priority = dc->threads[thd_idx].curr_prio;

    if ((hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, tid)) == NULL)
    {
        FIXME("Couldn't open thread %u (%u)\n", tid, GetLastError());
        return FALSE;
    }
    
    if (NtQueryInformationThread(hThread, ThreadBasicInformation,
                                 &tbi, sizeof(tbi), NULL) == STATUS_SUCCESS)
    {
        mdThd->Teb = (ULONG_PTR)tbi.TebBaseAddress;
        if (tbi.ExitStatus == STILL_ACTIVE)
        {
            if (tid != GetCurrentThreadId() &&
                (mdThd->SuspendCount = SuspendThread(hThread)) != (DWORD)-1)
            {
                ctx->ContextFlags = CONTEXT_FULL;
                if (!GetThreadContext(hThread, ctx))
                    memset(ctx, 0, sizeof(*ctx));

                fetch_thread_stack(dc, tbi.TebBaseAddress, ctx, &mdThd->Stack);
                ResumeThread(hThread);
            }
            else if (tid == GetCurrentThreadId() && except)
            {
                CONTEXT lctx, *pctx;
                mdThd->SuspendCount = 1;
                if (except->ClientPointers)
                {
                    EXCEPTION_POINTERS      ep;

                    ReadProcessMemory(dc->hProcess, except->ExceptionPointers,
                                      &ep, sizeof(ep), NULL);
                    ReadProcessMemory(dc->hProcess, ep.ContextRecord,
                                      &lctx, sizeof(lctx), NULL);
                    pctx = &lctx;
                }
                else pctx = except->ExceptionPointers->ContextRecord;

                *ctx = *pctx;
                fetch_thread_stack(dc, tbi.TebBaseAddress, pctx, &mdThd->Stack);
            }
            else mdThd->SuspendCount = 0;
        }
    }
    CloseHandle(hThread);
    return TRUE;
}

/******************************************************************
 *		add_module
 *
 * Add a module to a dump context
 */
static BOOL add_module(struct dump_context* dc, const WCHAR* name,
                       DWORD64 base, DWORD size, DWORD timestamp, DWORD checksum,
                       BOOL is_elf)
{
    if (!dc->modules)
    {
        dc->alloc_modules = 32;
        dc->modules = HeapAlloc(GetProcessHeap(), 0,
                                dc->alloc_modules * sizeof(*dc->modules));
    }
    else if(dc->num_modules >= dc->alloc_modules)
    {
        dc->alloc_modules *= 2;
        dc->modules = HeapReAlloc(GetProcessHeap(), 0, dc->modules,
                                  dc->alloc_modules * sizeof(*dc->modules));
    }
    if (!dc->modules)
    {
        dc->alloc_modules = dc->num_modules = 0;
        return FALSE;
    }
    if (is_elf ||
        !GetModuleFileNameExW(dc->hProcess, (HMODULE)(DWORD_PTR)base,
                              dc->modules[dc->num_modules].name,
                              sizeof(dc->modules[dc->num_modules].name) / sizeof(WCHAR)))
        lstrcpynW(dc->modules[dc->num_modules].name, name,
                  sizeof(dc->modules[dc->num_modules].name) / sizeof(WCHAR));
    dc->modules[dc->num_modules].base = base;
    dc->modules[dc->num_modules].size = size;
    dc->modules[dc->num_modules].timestamp = timestamp;
    dc->modules[dc->num_modules].checksum = checksum;
    dc->modules[dc->num_modules].is_elf = is_elf;
    dc->num_modules++;

    return TRUE;
}

/******************************************************************
 *		fetch_pe_module_info_cb
 *
 * Callback for accumulating in dump_context a PE modules set
 */
static BOOL WINAPI fetch_pe_module_info_cb(PCWSTR name, DWORD64 base, ULONG size,
                                           PVOID user)
{
    struct dump_context*        dc = user;
    IMAGE_NT_HEADERS            nth;

    if (!validate_addr64(base)) return FALSE;

    if (pe_load_nt_header(dc->hProcess, base, &nth))
        add_module(user, name, base, size,
                   nth.FileHeader.TimeDateStamp, nth.OptionalHeader.CheckSum,
                   FALSE);
    return TRUE;
}

/******************************************************************
 *		fetch_elf_module_info_cb
 *
 * Callback for accumulating in dump_context an ELF modules set
 */
static BOOL fetch_elf_module_info_cb(const WCHAR* name, unsigned long base,
                                     void* user)
{
    struct dump_context*        dc = user;
    DWORD_PTR                   rbase;
    DWORD                       size, checksum;

    /* FIXME: there's no relevant timestamp on ELF modules */
    /* NB: if we have a non-null base from the live-target use it (whenever
     * the ELF module is relocatable or not). If we have a null base (ELF
     * module isn't relocatable) then grab its base address from ELF file
     */
    if (!elf_fetch_file_info(name, &rbase, &size, &checksum))
        size = checksum = 0;
    add_module(dc, name, base ? base : rbase, size, 0 /* FIXME */, checksum, TRUE);
    return TRUE;
}

/******************************************************************
 *		fetch_macho_module_info_cb
 *
 * Callback for accumulating in dump_context a Mach-O modules set
 */
static BOOL fetch_macho_module_info_cb(const WCHAR* name, unsigned long base,
                                       void* user)
{
    struct dump_context*        dc = (struct dump_context*)user;
    DWORD_PTR                   rbase;
    DWORD                       size, checksum;

    /* FIXME: there's no relevant timestamp on Mach-O modules */
    /* NB: if we have a non-null base from the live-target use it.  If we have
     * a null base, then grab its base address from Mach-O file.
     */
    if (!macho_fetch_file_info(dc->hProcess, name, base, &rbase, &size, &checksum))
        size = checksum = 0;
    add_module(dc, name, base ? base : rbase, size, 0 /* FIXME */, checksum, TRUE);
    return TRUE;
}

static void fetch_modules_info(struct dump_context* dc)
{
    EnumerateLoadedModulesW64(dc->hProcess, fetch_pe_module_info_cb, dc);
    /* Since we include ELF modules in a separate stream from the regular PE ones,
     * we can always include those ELF modules (they don't eat lots of space)
     * And it's always a good idea to have a trace of the loaded ELF modules for
     * a given application in a post mortem debugging condition.
     */
    elf_enum_modules(dc->hProcess, fetch_elf_module_info_cb, dc);
    macho_enum_modules(dc->hProcess, fetch_macho_module_info_cb, dc);
}

static void fetch_module_versioninfo(LPCWSTR filename, VS_FIXEDFILEINFO* ffi)
{
    DWORD       handle;
    DWORD       sz;
    static const WCHAR backslashW[] = {'\\', '\0'};

    memset(ffi, 0, sizeof(*ffi));
    if ((sz = GetFileVersionInfoSizeW(filename, &handle)))
    {
        void*   info = HeapAlloc(GetProcessHeap(), 0, sz);
        if (info && GetFileVersionInfoW(filename, handle, sz, info))
        {
            VS_FIXEDFILEINFO*   ptr;
            UINT    len;

            if (VerQueryValueW(info, backslashW, (void*)&ptr, &len))
                memcpy(ffi, ptr, min(len, sizeof(*ffi)));
        }
        HeapFree(GetProcessHeap(), 0, info);
    }
}

/******************************************************************
 *		minidump_add_memory_block
 *
 * Add a memory block to be dumped in a minidump
 * If rva is non 0, it's the rva in the minidump where has to be stored
 * also the rva of the memory block when written (this allows us to reference
 * a memory block from outside the list of memory blocks).
 */
void minidump_add_memory_block(struct dump_context* dc, ULONG64 base, ULONG size, ULONG rva)
{
    if (!dc->mem)
    {
        dc->alloc_mem = 32;
        dc->mem = HeapAlloc(GetProcessHeap(), 0, dc->alloc_mem * sizeof(*dc->mem));
    }
    else if (dc->num_mem >= dc->alloc_mem)
    {
        dc->alloc_mem *= 2;
        dc->mem = HeapReAlloc(GetProcessHeap(), 0, dc->mem,
                              dc->alloc_mem * sizeof(*dc->mem));
    }
    if (dc->mem)
    {
        dc->mem[dc->num_mem].base = base;
        dc->mem[dc->num_mem].size = size;
        dc->mem[dc->num_mem].rva  = rva;
        dc->num_mem++;
    }
    else dc->num_mem = dc->alloc_mem = 0;
}

/******************************************************************
 *		writeat
 *
 * Writes a chunk of data at a given position in the minidump
 */
static void writeat(struct dump_context* dc, RVA rva, const void* data, unsigned size)
{
    DWORD       written;

    SetFilePointer(dc->hFile, rva, NULL, FILE_BEGIN);
    WriteFile(dc->hFile, data, size, &written, NULL);
}

/******************************************************************
 *		append
 *
 * writes a new chunk of data to the minidump, increasing the current
 * rva in dc
 */
static void append(struct dump_context* dc, const void* data, unsigned size)
{
    writeat(dc, dc->rva, data, size);
    dc->rva += size;
}

/******************************************************************
 *		dump_exception_info
 *
 * Write in File the exception information from pcs
 */
static  unsigned        dump_exception_info(struct dump_context* dc,
                                            const MINIDUMP_EXCEPTION_INFORMATION* except)
{
    MINIDUMP_EXCEPTION_STREAM   mdExcpt;
    EXCEPTION_RECORD            rec, *prec;
    CONTEXT                     ctx, *pctx;
    DWORD                       i;

    mdExcpt.ThreadId = except->ThreadId;
    mdExcpt.__alignment = 0;
    if (except->ClientPointers)
    {
        EXCEPTION_POINTERS      ep;

        ReadProcessMemory(dc->hProcess, 
                          except->ExceptionPointers, &ep, sizeof(ep), NULL);
        ReadProcessMemory(dc->hProcess, 
                          ep.ExceptionRecord, &rec, sizeof(rec), NULL);
        ReadProcessMemory(dc->hProcess, 
                          ep.ContextRecord, &ctx, sizeof(ctx), NULL);
        prec = &rec;
        pctx = &ctx;
    }
    else
    {
        prec = except->ExceptionPointers->ExceptionRecord;
        pctx = except->ExceptionPointers->ContextRecord;
    }
    mdExcpt.ExceptionRecord.ExceptionCode = prec->ExceptionCode;
    mdExcpt.ExceptionRecord.ExceptionFlags = prec->ExceptionFlags;
    mdExcpt.ExceptionRecord.ExceptionRecord = (DWORD_PTR)prec->ExceptionRecord;
    mdExcpt.ExceptionRecord.ExceptionAddress = (DWORD_PTR)prec->ExceptionAddress;
    mdExcpt.ExceptionRecord.NumberParameters = prec->NumberParameters;
    mdExcpt.ExceptionRecord.__unusedAlignment = 0;
    for (i = 0; i < mdExcpt.ExceptionRecord.NumberParameters; i++)
        mdExcpt.ExceptionRecord.ExceptionInformation[i] = prec->ExceptionInformation[i];
    mdExcpt.ThreadContext.DataSize = sizeof(*pctx);
    mdExcpt.ThreadContext.Rva = dc->rva + sizeof(mdExcpt);

    append(dc, &mdExcpt, sizeof(mdExcpt));
    append(dc, pctx, sizeof(*pctx));
    return sizeof(mdExcpt);
}

/******************************************************************
 *		dump_modules
 *
 * Write in File the modules from pcs
 */
static  unsigned        dump_modules(struct dump_context* dc, BOOL dump_elf)
{
    MINIDUMP_MODULE             mdModule;
    MINIDUMP_MODULE_LIST        mdModuleList;
    char                        tmp[1024];
    MINIDUMP_STRING*            ms = (MINIDUMP_STRING*)tmp;
    ULONG                       i, nmod;
    RVA                         rva_base;
    DWORD                       flags_out;
    unsigned                    sz;

    for (i = nmod = 0; i < dc->num_modules; i++)
    {
        if ((dc->modules[i].is_elf && dump_elf) ||
            (!dc->modules[i].is_elf && !dump_elf))
            nmod++;
    }

    mdModuleList.NumberOfModules = 0;
    /* reserve space for mdModuleList
     * FIXME: since we don't support 0 length arrays, we cannot use the
     * size of mdModuleList
     * FIXME: if we don't ask for all modules in cb, we'll get a hole in the file
     */

    /* the stream size is just the size of the module index.  It does not include the data for the
       names of each module.  *Technically* the names are supposed to go into the common string table
       in the minidump file.  Since each string is referenced by RVA they can all safely be located
       anywhere between streams in the file, so the end of this stream is sufficient. */
    rva_base = dc->rva;
    dc->rva += sz = sizeof(mdModuleList.NumberOfModules) + sizeof(mdModule) * nmod;
    for (i = 0; i < dc->num_modules; i++)
    {
        if ((dc->modules[i].is_elf && !dump_elf) ||
            (!dc->modules[i].is_elf && dump_elf))
            continue;

        flags_out = ModuleWriteModule | ModuleWriteMiscRecord | ModuleWriteCvRecord;
        if (dc->type & MiniDumpWithDataSegs)
            flags_out |= ModuleWriteDataSeg;
        if (dc->type & MiniDumpWithProcessThreadData)
            flags_out |= ModuleWriteTlsData;
        if (dc->type & MiniDumpWithCodeSegs)
            flags_out |= ModuleWriteCodeSegs;
        ms->Length = (lstrlenW(dc->modules[i].name) + 1) * sizeof(WCHAR);
        if (sizeof(ULONG) + ms->Length > sizeof(tmp))
            FIXME("Buffer overflow!!!\n");
        lstrcpyW(ms->Buffer, dc->modules[i].name);

        if (dc->cb)
        {
            MINIDUMP_CALLBACK_INPUT     cbin;
            MINIDUMP_CALLBACK_OUTPUT    cbout;

            cbin.ProcessId = dc->pid;
            cbin.ProcessHandle = dc->hProcess;
            cbin.CallbackType = ModuleCallback;

            cbin.u.Module.FullPath = ms->Buffer;
            cbin.u.Module.BaseOfImage = dc->modules[i].base;
            cbin.u.Module.SizeOfImage = dc->modules[i].size;
            cbin.u.Module.CheckSum = dc->modules[i].checksum;
            cbin.u.Module.TimeDateStamp = dc->modules[i].timestamp;
            memset(&cbin.u.Module.VersionInfo, 0, sizeof(cbin.u.Module.VersionInfo));
            cbin.u.Module.CvRecord = NULL;
            cbin.u.Module.SizeOfCvRecord = 0;
            cbin.u.Module.MiscRecord = NULL;
            cbin.u.Module.SizeOfMiscRecord = 0;

            cbout.u.ModuleWriteFlags = flags_out;
            if (!dc->cb->CallbackRoutine(dc->cb->CallbackParam, &cbin, &cbout))
                continue;
            flags_out &= cbout.u.ModuleWriteFlags;
        }
        if (flags_out & ModuleWriteModule)
        {
            /* fetch CPU dependent module info (like UNWIND_INFO) */
            dbghelp_current_cpu->fetch_minidump_module(dc, i, flags_out);

            mdModule.BaseOfImage = dc->modules[i].base;
            mdModule.SizeOfImage = dc->modules[i].size;
            mdModule.CheckSum = dc->modules[i].checksum;
            mdModule.TimeDateStamp = dc->modules[i].timestamp;
            mdModule.ModuleNameRva = dc->rva;
            ms->Length -= sizeof(WCHAR);
            append(dc, ms, sizeof(ULONG) + ms->Length + sizeof(WCHAR));
            fetch_module_versioninfo(ms->Buffer, &mdModule.VersionInfo);
            mdModule.CvRecord.DataSize = 0; /* FIXME */
            mdModule.CvRecord.Rva = 0; /* FIXME */
            mdModule.MiscRecord.DataSize = 0; /* FIXME */
            mdModule.MiscRecord.Rva = 0; /* FIXME */
            mdModule.Reserved0 = 0; /* FIXME */
            mdModule.Reserved1 = 0; /* FIXME */
            writeat(dc,
                    rva_base + sizeof(mdModuleList.NumberOfModules) + 
                        mdModuleList.NumberOfModules++ * sizeof(mdModule), 
                    &mdModule, sizeof(mdModule));
        }
    }
    writeat(dc, rva_base, &mdModuleList.NumberOfModules, 
            sizeof(mdModuleList.NumberOfModules));

    return sz;
}

/* Calls cpuid with an eax of 'ax' and returns the 16 bytes in *p
 * We are compiled with -fPIC, so we can't clobber ebx.
 */
static inline void do_x86cpuid(unsigned int ax, unsigned int *p)
{
#if defined(__GNUC__) && defined(__i386__)
    __asm__("pushl %%ebx\n\t"
            "cpuid\n\t"
            "movl %%ebx, %%esi\n\t"
            "popl %%ebx"
            : "=a" (p[0]), "=S" (p[1]), "=c" (p[2]), "=d" (p[3])
            :  "0" (ax));
#endif
}

/* From xf86info havecpuid.c 1.11 */
static inline int have_x86cpuid(void)
{
#if defined(__GNUC__) && defined(__i386__)
    unsigned int f1, f2;
    __asm__("pushfl\n\t"
            "pushfl\n\t"
            "popl %0\n\t"
            "movl %0,%1\n\t"
            "xorl %2,%0\n\t"
            "pushl %0\n\t"
            "popfl\n\t"
            "pushfl\n\t"
            "popl %0\n\t"
            "popfl"
            : "=&r" (f1), "=&r" (f2)
            : "ir" (0x00200000));
    return ((f1^f2) & 0x00200000) != 0;
#else
    return 0;
#endif
}

/******************************************************************
 *		dump_system_info
 *
 * Dumps into File the information about the system
 */
static  unsigned        dump_system_info(struct dump_context* dc)
{
    MINIDUMP_SYSTEM_INFO        mdSysInfo;
    SYSTEM_INFO                 sysInfo;
    OSVERSIONINFOW              osInfo;
    DWORD                       written;
    ULONG                       slen;
    DWORD                       wine_extra = 0;

    const char *(CDECL *wine_get_build_id)(void);
    void (CDECL *wine_get_host_version)(const char **sysname, const char **release);
    const char* build_id = NULL;
    const char* sys_name = NULL;
    const char* release_name = NULL;

    GetSystemInfo(&sysInfo);
    osInfo.dwOSVersionInfoSize = sizeof(osInfo);
    GetVersionExW(&osInfo);

    wine_get_build_id = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_build_id");
    wine_get_host_version = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_host_version");
    if (wine_get_build_id && wine_get_host_version)
    {
        /* cheat minidump system information by adding specific wine information */
        wine_extra = 4 + 4 * sizeof(slen);
        build_id = wine_get_build_id();
        wine_get_host_version(&sys_name, &release_name);
        wine_extra += strlen(build_id) + 1 + strlen(sys_name) + 1 + strlen(release_name) + 1;
    }

    mdSysInfo.ProcessorArchitecture = sysInfo.u.s.wProcessorArchitecture;
    mdSysInfo.ProcessorLevel = sysInfo.wProcessorLevel;
    mdSysInfo.ProcessorRevision = sysInfo.wProcessorRevision;
    mdSysInfo.u.s.NumberOfProcessors = sysInfo.dwNumberOfProcessors;
    mdSysInfo.u.s.ProductType = VER_NT_WORKSTATION; /* FIXME */
    mdSysInfo.MajorVersion = osInfo.dwMajorVersion;
    mdSysInfo.MinorVersion = osInfo.dwMinorVersion;
    mdSysInfo.BuildNumber = osInfo.dwBuildNumber;
    mdSysInfo.PlatformId = osInfo.dwPlatformId;

    mdSysInfo.CSDVersionRva = dc->rva + sizeof(mdSysInfo) + wine_extra;
    mdSysInfo.u1.Reserved1 = 0;
    mdSysInfo.u1.s.SuiteMask = VER_SUITE_TERMINAL;

    if (have_x86cpuid())
    {
        unsigned        regs0[4], regs1[4];

        do_x86cpuid(0, regs0);
        mdSysInfo.Cpu.X86CpuInfo.VendorId[0] = regs0[1];
        mdSysInfo.Cpu.X86CpuInfo.VendorId[1] = regs0[3];
        mdSysInfo.Cpu.X86CpuInfo.VendorId[2] = regs0[2];
        do_x86cpuid(1, regs1);
        mdSysInfo.Cpu.X86CpuInfo.VersionInformation = regs1[0];
        mdSysInfo.Cpu.X86CpuInfo.FeatureInformation = regs1[3];
        mdSysInfo.Cpu.X86CpuInfo.AMDExtendedCpuFeatures = 0;
        if (regs0[1] == 0x68747541 /* "Auth" */ &&
            regs0[3] == 0x69746e65 /* "enti" */ &&
            regs0[2] == 0x444d4163 /* "cAMD" */)
        {
            do_x86cpuid(0x80000000, regs1);  /* get vendor cpuid level */
            if (regs1[0] >= 0x80000001)
            {
                do_x86cpuid(0x80000001, regs1);  /* get vendor features */
                mdSysInfo.Cpu.X86CpuInfo.AMDExtendedCpuFeatures = regs1[3];
            }
        }
    }
    else
    {
        unsigned        i;
        ULONG64         one = 1;

        mdSysInfo.Cpu.OtherCpuInfo.ProcessorFeatures[0] = 0;
        mdSysInfo.Cpu.OtherCpuInfo.ProcessorFeatures[1] = 0;

        for (i = 0; i < sizeof(mdSysInfo.Cpu.OtherCpuInfo.ProcessorFeatures[0]) * 8; i++)
            if (IsProcessorFeaturePresent(i))
                mdSysInfo.Cpu.OtherCpuInfo.ProcessorFeatures[0] |= one << i;
    }
    append(dc, &mdSysInfo, sizeof(mdSysInfo));

    /* write Wine specific system information just behind the structure, and before any string */
    if (wine_extra)
    {
        char code[] = {'W','I','N','E'};

        WriteFile(dc->hFile, code, 4, &written, NULL);
        /* number of sub-info, so that we can extend structure if needed */
        slen = 3;
        WriteFile(dc->hFile, &slen, sizeof(slen), &written, NULL);
        /* we store offsets from just after the WINE marker */
        slen = 4 * sizeof(DWORD);
        WriteFile(dc->hFile, &slen, sizeof(slen), &written, NULL);
        slen += strlen(build_id) + 1;
        WriteFile(dc->hFile, &slen, sizeof(slen), &written, NULL);
        slen += strlen(sys_name) + 1;
        WriteFile(dc->hFile, &slen, sizeof(slen), &written, NULL);
        WriteFile(dc->hFile, build_id, strlen(build_id) + 1, &written, NULL);
        WriteFile(dc->hFile, sys_name, strlen(sys_name) + 1, &written, NULL);
        WriteFile(dc->hFile, release_name, strlen(release_name) + 1, &written, NULL);
        dc->rva += wine_extra;
    }

    /* write the service pack version string after this stream.  It is referenced within the
       stream by its RVA in the file. */
    slen = lstrlenW(osInfo.szCSDVersion) * sizeof(WCHAR);
    WriteFile(dc->hFile, &slen, sizeof(slen), &written, NULL);
    WriteFile(dc->hFile, osInfo.szCSDVersion, slen, &written, NULL);
    dc->rva += sizeof(ULONG) + slen;

    return sizeof(mdSysInfo);
}

/******************************************************************
 *		dump_threads
 *
 * Dumps into File the information about running threads
 */
static  unsigned        dump_threads(struct dump_context* dc,
                                     const MINIDUMP_EXCEPTION_INFORMATION* except)
{
    MINIDUMP_THREAD             mdThd;
    MINIDUMP_THREAD_LIST        mdThdList;
    unsigned                    i, sz;
    RVA                         rva_base;
    DWORD                       flags_out;
    CONTEXT                     ctx;

    mdThdList.NumberOfThreads = 0;

    rva_base = dc->rva;
    dc->rva += sz = sizeof(mdThdList.NumberOfThreads) + dc->num_threads * sizeof(mdThd);

    for (i = 0; i < dc->num_threads; i++)
    {
        fetch_thread_info(dc, i, except, &mdThd, &ctx);

        flags_out = ThreadWriteThread | ThreadWriteStack | ThreadWriteContext |
            ThreadWriteInstructionWindow;
        if (dc->type & MiniDumpWithProcessThreadData)
            flags_out |= ThreadWriteThreadData;
        if (dc->type & MiniDumpWithThreadInfo)
            flags_out |= ThreadWriteThreadInfo;

        if (dc->cb)
        {
            MINIDUMP_CALLBACK_INPUT     cbin;
            MINIDUMP_CALLBACK_OUTPUT    cbout;

            cbin.ProcessId = dc->pid;
            cbin.ProcessHandle = dc->hProcess;
            cbin.CallbackType = ThreadCallback;
            cbin.u.Thread.ThreadId = dc->threads[i].tid;
            cbin.u.Thread.ThreadHandle = 0; /* FIXME */
            cbin.u.Thread.Context = ctx;
            cbin.u.Thread.SizeOfContext = sizeof(CONTEXT);
            cbin.u.Thread.StackBase = mdThd.Stack.StartOfMemoryRange;
            cbin.u.Thread.StackEnd = mdThd.Stack.StartOfMemoryRange +
                mdThd.Stack.Memory.DataSize;

            cbout.u.ThreadWriteFlags = flags_out;
            if (!dc->cb->CallbackRoutine(dc->cb->CallbackParam, &cbin, &cbout))
                continue;
            flags_out &= cbout.u.ThreadWriteFlags;
        }
        if (flags_out & ThreadWriteThread)
        {
            if (ctx.ContextFlags && (flags_out & ThreadWriteContext))
            {
                mdThd.ThreadContext.Rva = dc->rva;
                mdThd.ThreadContext.DataSize = sizeof(CONTEXT);
                append(dc, &ctx, sizeof(CONTEXT));
            }
            if (mdThd.Stack.Memory.DataSize && (flags_out & ThreadWriteStack))
            {
                minidump_add_memory_block(dc, mdThd.Stack.StartOfMemoryRange,
                                          mdThd.Stack.Memory.DataSize,
                                          rva_base + sizeof(mdThdList.NumberOfThreads) +
                                          mdThdList.NumberOfThreads * sizeof(mdThd) +
                                          FIELD_OFFSET(MINIDUMP_THREAD, Stack.Memory.Rva));
            }
            writeat(dc, 
                    rva_base + sizeof(mdThdList.NumberOfThreads) +
                        mdThdList.NumberOfThreads * sizeof(mdThd),
                    &mdThd, sizeof(mdThd));
            mdThdList.NumberOfThreads++;
        }
        /* fetch CPU dependent thread info (like 256 bytes around program counter */
        dbghelp_current_cpu->fetch_minidump_thread(dc, i, flags_out, &ctx);
    }
    writeat(dc, rva_base,
            &mdThdList.NumberOfThreads, sizeof(mdThdList.NumberOfThreads));

    return sz;
}

/******************************************************************
 *		dump_memory_info
 *
 * dumps information about the memory of the process (stack of the threads)
 */
static unsigned         dump_memory_info(struct dump_context* dc)
{
    MINIDUMP_MEMORY_LIST        mdMemList;
    MINIDUMP_MEMORY_DESCRIPTOR  mdMem;
    DWORD                       written;
    unsigned                    i, pos, len, sz;
    RVA                         rva_base;
    char                        tmp[1024];

    mdMemList.NumberOfMemoryRanges = dc->num_mem;
    append(dc, &mdMemList.NumberOfMemoryRanges,
           sizeof(mdMemList.NumberOfMemoryRanges));
    rva_base = dc->rva;
    sz = mdMemList.NumberOfMemoryRanges * sizeof(mdMem);
    dc->rva += sz;
    sz += sizeof(mdMemList.NumberOfMemoryRanges);

    for (i = 0; i < dc->num_mem; i++)
    {
        mdMem.StartOfMemoryRange = dc->mem[i].base;
        mdMem.Memory.Rva = dc->rva;
        mdMem.Memory.DataSize = dc->mem[i].size;
        SetFilePointer(dc->hFile, dc->rva, NULL, FILE_BEGIN);
        for (pos = 0; pos < dc->mem[i].size; pos += sizeof(tmp))
        {
            len = min(dc->mem[i].size - pos, sizeof(tmp));
            if (ReadProcessMemory(dc->hProcess, 
                                  (void*)(DWORD_PTR)(dc->mem[i].base + pos),
                                  tmp, len, NULL))
                WriteFile(dc->hFile, tmp, len, &written, NULL);
        }
        dc->rva += mdMem.Memory.DataSize;
        writeat(dc, rva_base + i * sizeof(mdMem), &mdMem, sizeof(mdMem));
        if (dc->mem[i].rva)
        {
            writeat(dc, dc->mem[i].rva, &mdMem.Memory.Rva, sizeof(mdMem.Memory.Rva));
        }
    }

    return sz;
}

static unsigned         dump_misc_info(struct dump_context* dc)
{
    MINIDUMP_MISC_INFO  mmi;

    mmi.SizeOfInfo = sizeof(mmi);
    mmi.Flags1 = MINIDUMP_MISC1_PROCESS_ID;
    mmi.ProcessId = dc->pid;
    /* FIXME: create/user/kernel time */
    mmi.ProcessCreateTime = 0;
    mmi.ProcessKernelTime = 0;
    mmi.ProcessUserTime = 0;

    append(dc, &mmi, sizeof(mmi));
    return sizeof(mmi);
}

/******************************************************************
 *		MiniDumpWriteDump (DEBUGHLP.@)
 *
 */
BOOL WINAPI MiniDumpWriteDump(HANDLE hProcess, DWORD pid, HANDLE hFile,
                              MINIDUMP_TYPE DumpType,
                              PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                              PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                              PMINIDUMP_CALLBACK_INFORMATION CallbackParam)
{
    static const MINIDUMP_DIRECTORY emptyDir = {UnusedStream, {0, 0}};
    MINIDUMP_HEADER     mdHead;
    MINIDUMP_DIRECTORY  mdDir;
    DWORD               i, nStreams, idx_stream;
    struct dump_context dc;

    dc.hProcess = hProcess;
    dc.hFile = hFile;
    dc.pid = pid;
    dc.modules = NULL;
    dc.num_modules = 0;
    dc.alloc_modules = 0;
    dc.threads = NULL;
    dc.num_threads = 0;
    dc.cb = CallbackParam;
    dc.type = DumpType;
    dc.mem = NULL;
    dc.num_mem = 0;
    dc.alloc_mem = 0;
    dc.rva = 0;

    if (!fetch_process_info(&dc)) return FALSE;
    fetch_modules_info(&dc);

    /* 1) init */
    nStreams = 6 + (ExceptionParam ? 1 : 0) +
        (UserStreamParam ? UserStreamParam->UserStreamCount : 0);

    /* pad the directory size to a multiple of 4 for alignment purposes */
    nStreams = (nStreams + 3) & ~3;

    if (DumpType & MiniDumpWithDataSegs)
        FIXME("NIY MiniDumpWithDataSegs\n");
    if (DumpType & MiniDumpWithFullMemory)
        FIXME("NIY MiniDumpWithFullMemory\n");
    if (DumpType & MiniDumpWithHandleData)
        FIXME("NIY MiniDumpWithHandleData\n");
    if (DumpType & MiniDumpFilterMemory)
        FIXME("NIY MiniDumpFilterMemory\n");
    if (DumpType & MiniDumpScanMemory)
        FIXME("NIY MiniDumpScanMemory\n");

    /* 2) write header */
    mdHead.Signature = MINIDUMP_SIGNATURE;
    mdHead.Version = MINIDUMP_VERSION;  /* NOTE: native puts in an 'implementation specific' value in the high order word of this member */
    mdHead.NumberOfStreams = nStreams;
    mdHead.CheckSum = 0;                /* native sets a 0 checksum in its files */
    mdHead.StreamDirectoryRva = sizeof(mdHead);
    mdHead.u.TimeDateStamp = time(NULL);
    mdHead.Flags = DumpType;
    append(&dc, &mdHead, sizeof(mdHead));

    /* 3) write stream directories */
    dc.rva += nStreams * sizeof(mdDir);
    idx_stream = 0;

    /* 3.1) write data stream directories */

    /* must be first in minidump */
    mdDir.StreamType = SystemInfoStream;
    mdDir.Location.Rva = dc.rva;
    mdDir.Location.DataSize = dump_system_info(&dc);
    writeat(&dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
            &mdDir, sizeof(mdDir));

    mdDir.StreamType = ThreadListStream;
    mdDir.Location.Rva = dc.rva;
    mdDir.Location.DataSize = dump_threads(&dc, ExceptionParam);
    writeat(&dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir), 
            &mdDir, sizeof(mdDir));

    mdDir.StreamType = ModuleListStream;
    mdDir.Location.Rva = dc.rva;
    mdDir.Location.DataSize = dump_modules(&dc, FALSE);
    writeat(&dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
            &mdDir, sizeof(mdDir));

    mdDir.StreamType = 0xfff0; /* FIXME: this is part of MS reserved streams */
    mdDir.Location.Rva = dc.rva;
    mdDir.Location.DataSize = dump_modules(&dc, TRUE);
    writeat(&dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
            &mdDir, sizeof(mdDir));

    mdDir.StreamType = MemoryListStream;
    mdDir.Location.Rva = dc.rva;
    mdDir.Location.DataSize = dump_memory_info(&dc);
    writeat(&dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
            &mdDir, sizeof(mdDir));

    mdDir.StreamType = MiscInfoStream;
    mdDir.Location.Rva = dc.rva;
    mdDir.Location.DataSize = dump_misc_info(&dc);
    writeat(&dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
            &mdDir, sizeof(mdDir));

    /* 3.2) write exception information (if any) */
    if (ExceptionParam)
    {
        mdDir.StreamType = ExceptionStream;
        mdDir.Location.Rva = dc.rva;
        mdDir.Location.DataSize = dump_exception_info(&dc, ExceptionParam);
        writeat(&dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
                &mdDir, sizeof(mdDir));
    }

    /* 3.3) write user defined streams (if any) */
    if (UserStreamParam)
    {
        for (i = 0; i < UserStreamParam->UserStreamCount; i++)
        {
            mdDir.StreamType = UserStreamParam->UserStreamArray[i].Type;
            mdDir.Location.DataSize = UserStreamParam->UserStreamArray[i].BufferSize;
            mdDir.Location.Rva = dc.rva;
            writeat(&dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
                    &mdDir, sizeof(mdDir));
            append(&dc, UserStreamParam->UserStreamArray[i].Buffer, 
                   UserStreamParam->UserStreamArray[i].BufferSize);
        }
    }

    /* fill the remaining directory entries with 0's (unused stream types) */
    /* NOTE: this should always come last in the dump! */
    for (i = idx_stream; i < nStreams; i++)
        writeat(&dc, mdHead.StreamDirectoryRva + i * sizeof(emptyDir), &emptyDir, sizeof(emptyDir));

    HeapFree(GetProcessHeap(), 0, dc.mem);
    HeapFree(GetProcessHeap(), 0, dc.modules);
    HeapFree(GetProcessHeap(), 0, dc.threads);

    return TRUE;
}

/******************************************************************
 *		MiniDumpReadDumpStream (DEBUGHLP.@)
 *
 *
 */
BOOL WINAPI MiniDumpReadDumpStream(PVOID base, ULONG str_idx,
                                   PMINIDUMP_DIRECTORY* pdir,
                                   PVOID* stream, ULONG* size)
{
    MINIDUMP_HEADER*    mdHead = base;

    if (mdHead->Signature == MINIDUMP_SIGNATURE)
    {
        MINIDUMP_DIRECTORY* dir;
        DWORD               i;

        dir = (MINIDUMP_DIRECTORY*)((char*)base + mdHead->StreamDirectoryRva);
        for (i = 0; i < mdHead->NumberOfStreams; i++, dir++)
        {
            if (dir->StreamType == str_idx)
            {
                if (pdir) *pdir = dir;
                if (stream) *stream = (char*)base + dir->Location.Rva;
                if (size) *size = dir->Location.DataSize;
                return TRUE;
            }
        }
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}
