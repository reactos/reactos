/*
 * File minidump.c - management of dumps (read & write)
 *
 * Copyright (C) 2004, Eric Pouech
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <time.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "dbghelp_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

#if 0
/* hard to see how we can generate this very easily (how to grab latest exception
 * in a process ?)
 */
static  void    DumpException(struct process* pcs, HANDLE hFile, RVA* rva)
{
    MINIDUMP_EXCEPTION_STREAM   mdExcpt;

    mdExcpt.ThreadId = DEBUG_CurrThread->tid;
    mdExcpt.__alignment = 0;
    mdExcpt.ExceptionRecord.

    ULONG                       ExceptionCode;
    ULONG                       ExceptionFlags;
    ULONGLONG                   ExceptionRecord;
    ULONGLONG                   ExceptionAddress;
    ULONG                       NumberParameters;
    ULONG                        __unusedAlignment;
    ULONGLONG                   ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
}
#endif

/******************************************************************
 *		dump_modules
 *
 * Write in File the modules from pcs
 */
static  void    dump_modules(struct process* pcs, HANDLE hFile, RVA* rva)
{
    MINIDUMP_MODULE             mdModule;
    MINIDUMP_MODULE_LIST        mdModuleList;
    DWORD                       written;
    struct module*              module = NULL;

    mdModuleList.NumberOfModules = 0;
    for (module = pcs->lmodules; module; module = module->next)
        mdModuleList.NumberOfModules++;
    WriteFile(hFile, &mdModuleList.NumberOfModules,
              sizeof(mdModuleList.NumberOfModules), &written, NULL);
    *rva += sizeof(mdModuleList.NumberOfModules) +
        sizeof(mdModule) * mdModuleList.NumberOfModules;
    for (module = pcs->lmodules; module; module = module->next)
    {
        mdModule.BaseOfImage = (DWORD)module->module.BaseOfImage;
        mdModule.SizeOfImage = module->module.ImageSize;
        mdModule.CheckSum = module->module.CheckSum;
        mdModule.TimeDateStamp = module->module.TimeDateStamp;
        mdModule.ModuleNameRva = *rva;
        *rva += strlen(module->module.ModuleName) + 1;
        memset(&mdModule.VersionInfo, 0, sizeof(mdModule.VersionInfo)); /* FIXME */
        mdModule.CvRecord.DataSize = 0; /* FIXME */
        mdModule.CvRecord.Rva = 0; /* FIXME */
        mdModule.MiscRecord.DataSize = 0; /* FIXME */
        mdModule.MiscRecord.Rva = 0; /* FIXME */
        mdModule.Reserved0 = 0;
        mdModule.Reserved1 = 0;
        WriteFile(hFile, &mdModule, sizeof(mdModule), &written, NULL);
    }
    for (module = pcs->lmodules; module; module = module->next)
    {
        WriteFile(hFile, module->module.ModuleName,
                  strlen(module->module.ModuleName) + 1, &written, NULL);
        FIXME("CV and misc records not written\n");
    }
}

/******************************************************************
 *		dump_system_info
 *
 * Dumps into File the information about the system
 */
static  void    dump_system_info(struct process* pcs, HANDLE hFile, RVA* rva)
{
    MINIDUMP_SYSTEM_INFO        mdSysInfo;
    SYSTEM_INFO                 sysInfo;
    OSVERSIONINFOA              osInfo;
    DWORD                       written;

    GetSystemInfo(&sysInfo);
    GetVersionExA(&osInfo);

    mdSysInfo.ProcessorArchitecture = sysInfo.u.s.wProcessorArchitecture;
    mdSysInfo.ProcessorLevel = sysInfo.wProcessorLevel;
    mdSysInfo.ProcessorRevision = sysInfo.wProcessorRevision;
    mdSysInfo.Reserved0 = 0;

    mdSysInfo.MajorVersion = osInfo.dwMajorVersion;
    mdSysInfo.MinorVersion = osInfo.dwMinorVersion;
    mdSysInfo.BuildNumber = osInfo.dwBuildNumber;
    mdSysInfo.PlatformId = osInfo.dwPlatformId;

    mdSysInfo.CSDVersionRva = *rva + sizeof(mdSysInfo);
    mdSysInfo.Reserved1 = 0;

    WriteFile(hFile, &mdSysInfo, sizeof(mdSysInfo), &written, NULL);
    *rva += sizeof(mdSysInfo);
    WriteFile(hFile, osInfo.szCSDVersion, strlen(osInfo.szCSDVersion) + 1,
              &written, NULL);
    *rva += strlen(osInfo.szCSDVersion) + 1;
}

/******************************************************************
 *		dump_threads
 *
 * Dumps into File the information about running threads
 */
static  void    dump_threads(struct process* pcs, HANDLE hFile, RVA* rva)
{
#if 0
    MINIDUMP_THREAD             mdThd;
    MINIDUMP_THREAD_LIST        mdThdList;
    DWORD                       written;
    DBG_THREAD*                 thd;

    mdThdList.NumberOfThreads = pcs->num_threads;
    WriteFile(hFile, &mdThdList.NumberOfThreads, sizeof(mdThdList.NumberOfThreads),
              &written, NULL);
    *rva += sizeof(mdThdList.NumberOfThreads) +
        mdThdList.NumberOfThreads * sizeof(mdThd);
    for (thd = pcs->threads; thd; thd = thd->next)
    {
        mdThd.ThreadId = thd->tid;
        mdThd.SuspendCount = 0; /* FIXME */
        mdThd.PriorityClass = 0; /* FIXME */
        mdThd.Priority = 0; /* FIXME */
        mdThd.Teb = 0; /* FIXME */
        mdThd.Stack.StartOfMemoryRange = 0; /* FIXME */
        mdThd.Stack.Memory.DataSize = 0; /* FIXME */
        mdThd.Stack.Memory.Rva = 0; /* FIXME */
        mdThd.ThreadContext.DataSize = 0;/* FIXME */
        mdThd.ThreadContext.Rva = 0; /* FIXME */

        WriteFile(hFile, &mdThd, sizeof(mdThd), &written, NULL);
        FIXME("Stack & thread context not written\n");
    }
#endif
}

/******************************************************************
 *		MiniDumpWriteDump (DEBUGHLP.@)
 *
 *
 */
BOOL WINAPI MiniDumpWriteDump(HANDLE hProcess, DWORD ProcessId, HANDLE hFile,
                              MINIDUMP_TYPE DumpType,
                              PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                              PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                              PMINIDUMP_CALLBACK_INFORMATION CallbackParam)
{
    struct process*     pcs;
    MINIDUMP_HEADER     mdHead;
    MINIDUMP_DIRECTORY  mdDir;
    DWORD               currRva, written;
    DWORD               i, nStream, addStream;
    RVA                 rva;

    pcs = process_find_by_handle(hProcess);
    if (!pcs) return FALSE; /* FIXME: should try to load it ??? */

    /* 1) init */

    nStream = UserStreamParam ? UserStreamParam->UserStreamCount : 0;
    addStream = 0;
    if (DumpType & MiniDumpNormal)
        addStream += 3; /* sure ? thread stack back trace */

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
    rva = sizeof(mdHead);
    mdHead.Signature = MINIDUMP_SIGNATURE;
    mdHead.Version = MINIDUMP_VERSION;
    mdHead.NumberOfStreams = nStream + addStream;
    mdHead.StreamDirectoryRva = rva;
    mdHead.u.TimeDateStamp = time(NULL);
    mdHead.Flags = DumpType;
    WriteFile(hFile, &mdHead, sizeof(mdHead), &written, NULL);

    /* 3) write stream directories */
    rva += (nStream + addStream) * sizeof(mdDir);
    /* 3.1) write data stream directories */
    currRva = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
    SetFilePointer(hFile, rva, NULL, FILE_BEGIN);
    mdDir.StreamType = ModuleListStream;
    mdDir.Location.Rva = rva;
    dump_modules(pcs, hFile, &rva);
    mdDir.Location.DataSize = SetFilePointer(hFile, 0, NULL, FILE_CURRENT) - mdDir.Location.Rva;
    SetFilePointer(hFile, currRva, NULL, FILE_BEGIN);
    WriteFile(hFile, &mdDir, sizeof(mdDir), &written, NULL);

    currRva = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
    SetFilePointer(hFile, rva, NULL, FILE_BEGIN);
    mdDir.StreamType = ThreadListStream;
    mdDir.Location.Rva = rva;
    dump_threads(pcs, hFile, &rva);
    mdDir.Location.DataSize = SetFilePointer(hFile, 0, NULL, FILE_CURRENT) - mdDir.Location.Rva;
    SetFilePointer(hFile, currRva, NULL, FILE_BEGIN);
    WriteFile(hFile, &mdDir, sizeof(mdDir), &written, NULL);

    currRva = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
    SetFilePointer(hFile, rva, NULL, FILE_BEGIN);
    mdDir.StreamType = SystemInfoStream;
    mdDir.Location.Rva = rva;
    dump_system_info(pcs, hFile, &rva);
    mdDir.Location.DataSize = SetFilePointer(hFile, 0, NULL, FILE_CURRENT) - mdDir.Location.Rva;
    SetFilePointer(hFile, currRva, NULL, FILE_BEGIN);
    WriteFile(hFile, &mdDir, sizeof(mdDir), &written, NULL);

    /* 3.2) write user define stream */
    for (i = 0; i < nStream; i++)
    {
        mdDir.StreamType = UserStreamParam->UserStreamArray[i].Type;
        mdDir.Location.DataSize = UserStreamParam->UserStreamArray[i].BufferSize;
        mdDir.Location.Rva = rva;
        WriteFile(hFile, &mdDir, sizeof(mdDir), &written, NULL);
        currRva = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
        SetFilePointer(hFile, rva, NULL, FILE_BEGIN);
        WriteFile(hFile,
                  UserStreamParam->UserStreamArray[i].Buffer,
                  UserStreamParam->UserStreamArray[i].BufferSize,
                  &written, NULL);
        rva += UserStreamParam->UserStreamArray[i].BufferSize;
        SetFilePointer(hFile, currRva, NULL, FILE_BEGIN);
    }

    return TRUE;
}
