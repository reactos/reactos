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

#include <time.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "dbghelp_private.h"
#include "winternl.h"
#include "psapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

struct dump_memory
{
    ULONG                               base;
    ULONG                               size;
    ULONG                               rva;
};

struct dump_module
{
    unsigned                            is_elf;
    ULONG                               base;
    ULONG                               size;
    DWORD                               timestamp;
    DWORD                               checksum;
    WCHAR                               name[MAX_PATH];
};

struct dump_context
{
    /* process & thread information */
    HANDLE                              hProcess;
    DWORD                               pid;
    void*                               pcs_buffer;
    SYSTEM_PROCESS_INFORMATION*         spi;
    /* module information */
    struct dump_module*                 modules;
    unsigned                            num_modules;
    /* exception information */
    /* output information */
    MINIDUMP_TYPE                       type;
    HANDLE                              hFile;
    RVA                                 rva;
    struct dump_memory*                 mem;
    unsigned                            num_mem;
    /* callback information */
    MINIDUMP_CALLBACK_INFORMATION*      cb;
};

/******************************************************************
 *		fetch_processes_info
 *
 * reads system wide process information, and make spi point to the record
 * for process of id 'pid'
 */
static BOOL fetch_processes_info(struct dump_context* dc)
{
    ULONG       buf_size = 0x1000;
    NTSTATUS    nts;

    dc->pcs_buffer = NULL;
    if (!(dc->pcs_buffer = HeapAlloc(GetProcessHeap(), 0, buf_size))) return FALSE;
    for (;;)
    {
        nts = NtQuerySystemInformation(SystemProcessInformation, 
                                       dc->pcs_buffer, buf_size, NULL);
        if (nts != STATUS_INFO_LENGTH_MISMATCH) break;
        dc->pcs_buffer = HeapReAlloc(GetProcessHeap(), 0, dc->pcs_buffer, 
                                     buf_size *= 2);
        if (!dc->pcs_buffer) return FALSE;
    }

    if (nts == STATUS_SUCCESS)
    {
        dc->spi = dc->pcs_buffer;
        for (;;)
        {
            if (dc->spi->dwProcessID == dc->pid) return TRUE;
            if (!dc->spi->dwOffset) break;
            dc->spi = (SYSTEM_PROCESS_INFORMATION*)     
                ((char*)dc->spi + dc->spi->dwOffset);
        }
    }
    HeapFree(GetProcessHeap(), 0, dc->pcs_buffer);
    dc->pcs_buffer = NULL;
    dc->spi = NULL;
    return FALSE;
}

static void fetch_thread_stack(struct dump_context* dc, const void* teb_addr,
                               const CONTEXT* ctx, MINIDUMP_MEMORY_DESCRIPTOR* mmd)
{
    NT_TIB      tib;

    if (ReadProcessMemory(dc->hProcess, teb_addr, &tib, sizeof(tib), NULL))
    {
#ifdef __i386__
        /* limiting the stack dumping to the size actually used */
        if (ctx->Esp){

            /* make sure ESP is within the established range of the stack.  It could have
               been clobbered by whatever caused the original exception. */
            if (ctx->Esp - 4 < (ULONG_PTR)tib.StackLimit || ctx->Esp - 4 > (ULONG_PTR)tib.StackBase)
                mmd->StartOfMemoryRange = (ULONG_PTR)tib.StackLimit;

            else
                mmd->StartOfMemoryRange = (ctx->Esp - 4);
        }

        else
            mmd->StartOfMemoryRange = (ULONG_PTR)tib.StackLimit;

#elif defined(__powerpc__)
        if (ctx->Iar){

            /* make sure IAR is within the established range of the stack.  It could have
               been clobbered by whatever caused the original exception. */
            if (ctx->Iar - 4 < (ULONG_PTR)tib.StackLimit || ctx->Iar - 4 > (ULONG_PTR)tib.StackBase)
                mmd->StartOfMemoryRange = (ULONG_PTR)tib.StackLimit;

            else
                mmd->StartOfMemoryRange = (ctx->Iar - 4);
        }

        else
            mmd->StartOfMemoryRange = (ULONG_PTR)tib.StackLimit;

#elif defined(__x86_64__)
        if (ctx->Rsp){

            /* make sure RSP is within the established range of the stack.  It could have
               been clobbered by whatever caused the original exception. */
            if (ctx->Rsp - 8 < (ULONG_PTR)tib.StackLimit || ctx->Rsp - 8 > (ULONG_PTR)tib.StackBase)
                mmd->StartOfMemoryRange = (ULONG_PTR)tib.StackLimit;

            else
                mmd->StartOfMemoryRange = (ctx->Rsp - 8);
        }

        else
            mmd->StartOfMemoryRange = (ULONG_PTR)tib.StackLimit;

#else
#error unsupported CPU
#endif
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
    DWORD                       tid = dc->spi->ti[thd_idx].dwThreadID;
    HANDLE                      hThread;
    THREAD_BASIC_INFORMATION    tbi;

    memset(ctx, 0, sizeof(*ctx));

    mdThd->ThreadId = dc->spi->ti[thd_idx].dwThreadID;
    mdThd->SuspendCount = 0;
    mdThd->Teb = 0;
    mdThd->Stack.StartOfMemoryRange = 0;
    mdThd->Stack.Memory.DataSize = 0;
    mdThd->Stack.Memory.Rva = 0;
    mdThd->ThreadContext.DataSize = 0;
    mdThd->ThreadContext.Rva = 0;
    mdThd->PriorityClass = dc->spi->ti[thd_idx].dwBasePriority; /* FIXME */
    mdThd->Priority = dc->spi->ti[thd_idx].dwCurrentPriority;

    if ((hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, tid)) == NULL)
    {
        FIXME("Couldn't open thread %u (%u)\n",
              dc->spi->ti[thd_idx].dwThreadID, GetLastError());
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
                                      &ctx, sizeof(ctx), NULL);
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
                       DWORD base, DWORD size, DWORD timestamp, DWORD checksum,
                       BOOL is_elf)
{
    if (!dc->modules)
        dc->modules = HeapAlloc(GetProcessHeap(), 0,
                                ++dc->num_modules * sizeof(*dc->modules));
    else
        dc->modules = HeapReAlloc(GetProcessHeap(), 0, dc->modules,
                                  ++dc->num_modules * sizeof(*dc->modules));
    if (!dc->modules) return FALSE;
    if (is_elf ||
        !GetModuleFileNameExW(dc->hProcess, (HMODULE)base,
                              dc->modules[dc->num_modules - 1].name,
                              sizeof(dc->modules[dc->num_modules - 1].name) / sizeof(WCHAR)))
        lstrcpynW(dc->modules[dc->num_modules - 1].name, name,
                  sizeof(dc->modules[dc->num_modules - 1].name) / sizeof(WCHAR));
    dc->modules[dc->num_modules - 1].base = base;
    dc->modules[dc->num_modules - 1].size = size;
    dc->modules[dc->num_modules - 1].timestamp = timestamp;
    dc->modules[dc->num_modules - 1].checksum = checksum;
    dc->modules[dc->num_modules - 1].is_elf = is_elf;

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
    struct dump_context*        dc = (struct dump_context*)user;
    IMAGE_NT_HEADERS            nth;

    if (!validate_addr64(base)) return FALSE;

    if (pe_load_nt_header(dc->hProcess, base, &nth))
        add_module((struct dump_context*)user, name, base, size,
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
    struct dump_context*        dc = (struct dump_context*)user;
    DWORD                       rbase, size, checksum;

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

static void fetch_modules_info(struct dump_context* dc)
{
    EnumerateLoadedModulesW64(dc->hProcess, fetch_pe_module_info_cb, dc);
    /* Since we include ELF modules in a separate stream from the regular PE ones,
     * we can always include those ELF modules (they don't eat lots of space)
     * And it's always a good idea to have a trace of the loaded ELF modules for
     * a given application in a post mortem debugging condition.
     */
    elf_enum_modules(dc->hProcess, fetch_elf_module_info_cb, dc);
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
 *		add_memory_block
 *
 * Add a memory block to be dumped in a minidump
 * If rva is non 0, it's the rva in the minidump where has to be stored
 * also the rva of the memory block when written (this allows to reference
 * a memory block from outside the list of memory blocks).
 */
static void add_memory_block(struct dump_context* dc, ULONG64 base, ULONG size, ULONG rva)
{
    if (dc->mem)
        dc->mem = HeapReAlloc(GetProcessHeap(), 0, dc->mem, 
                              ++dc->num_mem * sizeof(*dc->mem));
    else
        dc->mem = HeapAlloc(GetProcessHeap(), 0, ++dc->num_mem * sizeof(*dc->mem));
    if (dc->mem)
    {
        dc->mem[dc->num_mem - 1].base = base;
        dc->mem[dc->num_mem - 1].size = size;
        dc->mem[dc->num_mem - 1].rva  = rva;
    }
    else dc->num_mem = 0;
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
static void append(struct dump_context* dc, void* data, unsigned size)
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
    int                         i;

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

    GetSystemInfo(&sysInfo);
    osInfo.dwOSVersionInfoSize = sizeof(osInfo);
    GetVersionExW(&osInfo);

    mdSysInfo.ProcessorArchitecture = sysInfo.u.s.wProcessorArchitecture;
    mdSysInfo.ProcessorLevel = sysInfo.wProcessorLevel;
    mdSysInfo.ProcessorRevision = sysInfo.wProcessorRevision;
    mdSysInfo.u.s.NumberOfProcessors = sysInfo.dwNumberOfProcessors;
    mdSysInfo.u.s.ProductType = VER_NT_WORKSTATION; /* FIXME */
    mdSysInfo.MajorVersion = osInfo.dwMajorVersion;
    mdSysInfo.MinorVersion = osInfo.dwMinorVersion;
    mdSysInfo.BuildNumber = osInfo.dwBuildNumber;
    mdSysInfo.PlatformId = osInfo.dwPlatformId;

    mdSysInfo.CSDVersionRva = dc->rva + sizeof(mdSysInfo);
    mdSysInfo.u1.Reserved1 = 0;
    mdSysInfo.u1.s.SuiteMask = VER_SUITE_TERMINAL;

    FIXME("fill in CPU vendorID and feature set\n");
    memset(&mdSysInfo.Cpu, 0, sizeof(mdSysInfo.Cpu));

    append(dc, &mdSysInfo, sizeof(mdSysInfo));

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
    unsigned                    i;
    RVA                         rva_base;
    DWORD                       flags_out;
    CONTEXT                     ctx;

    mdThdList.NumberOfThreads = 0;

    rva_base = dc->rva;
    dc->rva += sizeof(mdThdList.NumberOfThreads) +
        dc->spi->dwThreadCount * sizeof(mdThd);

    for (i = 0; i < dc->spi->dwThreadCount; i++)
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
            cbin.u.Thread.ThreadId = dc->spi->ti[i].dwThreadID;
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
                add_memory_block(dc, mdThd.Stack.StartOfMemoryRange,
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
        if (ctx.ContextFlags && (flags_out & ThreadWriteInstructionWindow))
        {
            /* FIXME: - Native dbghelp also dumps 0x80 bytes around EIP
             *        - also crop values across module boundaries, 
             *        - and don't make it i386 dependent 
             */
            /* add_memory_block(dc, ctx.Eip - 0x80, ctx.Eip + 0x80, 0); */
        }
    }
    writeat(dc, rva_base,
            &mdThdList.NumberOfThreads, sizeof(mdThdList.NumberOfThreads));

    return dc->rva - rva_base;
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
                                  (void*)(dc->mem[i].base + pos),
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
    dc.cb = CallbackParam;
    dc.type = DumpType;
    dc.mem = NULL;
    dc.num_mem = 0;
    dc.rva = 0;

    if (!fetch_processes_info(&dc)) return FALSE;
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

    HeapFree(GetProcessHeap(), 0, dc.pcs_buffer);
    HeapFree(GetProcessHeap(), 0, dc.mem);
    HeapFree(GetProcessHeap(), 0, dc.modules);

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
    MINIDUMP_HEADER*    mdHead = (MINIDUMP_HEADER*)base;

    if (mdHead->Signature == MINIDUMP_SIGNATURE)
    {
        MINIDUMP_DIRECTORY* dir;
        int                 i;

        dir = (MINIDUMP_DIRECTORY*)((char*)base + mdHead->StreamDirectoryRva);
        for (i = 0; i < mdHead->NumberOfStreams; i++, dir++)
        {
            if (dir->StreamType == str_idx)
            {
                *pdir = dir;
                *stream = (char*)base + dir->Location.Rva;
                *size = dir->Location.DataSize;
                return TRUE;
            }
        }
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}
