/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Performance Counters
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2014 Ismael Ferreras Morezuelas <swyterzone+ros@gmail.com>
 */

#include "precomp.h"

#define WIN32_LEAN_AND_MEAN
#include <aclapi.h>

#define NTOS_MODE_USER
#include <ndk/psfuncs.h>
#include <ndk/exfuncs.h>

CRITICAL_SECTION PerfDataCriticalSection;
PPERFDATA        pPerfDataOld = NULL;    /* Older perf data (saved to establish delta values) */
PPERFDATA        pPerfData = NULL;       /* Most recent copy of perf data */
ULONG            ProcessCountOld = 0;
ULONG            ProcessCount = 0;

typedef struct _CPU_TIMES
{
    double dbIdleTime;
    double dbKernelTime;

    LARGE_INTEGER OldIdleTime;
    LARGE_INTEGER OldKernelTime;
} CPU_TIMES, *PCPU_TIMES;

static PCPU_TIMES ProcessorTimes = NULL;
static CPU_TIMES  ProcessorMeanTimes = {0};

double        dbSystemTime;
LARGE_INTEGER OldSystemTime = {{0,0}};

SYSTEM_BASIC_INFORMATION                   SystemBasicInfo;
PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION  SystemProcessorTimeInfo = NULL;
SYSTEM_PERFORMANCE_INFORMATION             SystemPerfInfo = {0};
SYSTEM_FILECACHE_INFORMATION               SystemCacheInfo = {0};
ULONG                                      SystemNumberOfHandles = 0;
PSID                                       SystemUserSid = NULL;

PCMD_LINE_CACHE global_cache = NULL;

#define CMD_LINE_MIN(a, b) (a < b ? a - sizeof(WCHAR) : b)

typedef struct _SIDTOUSERNAME
{
    LIST_ENTRY List;
    LPWSTR pszName;
    BYTE Data[0];
} SIDTOUSERNAME, *PSIDTOUSERNAME;

static LIST_ENTRY SidToUserNameHead = {&SidToUserNameHead, &SidToUserNameHead};

/*static*/ VOID
__cdecl
_DiagError(
    _In_ PCWSTR Format,
    ...)
{
    va_list args;
    WCHAR Buffer[1024];

    va_start(args, Format);
    StringCbVPrintfW(Buffer, sizeof(Buffer), Format, args);
    va_end(args);

    MessageBoxW(NULL, Buffer, L"Error", MB_ICONERROR | MB_OK);
}

BOOL PerfDataInitialize(VOID)
{
    SID_IDENTIFIER_AUTHORITY NtSidAuthority = {SECURITY_NT_AUTHORITY};
    NTSTATUS status;
    ULONG BufferSize;

    InitializeCriticalSection(&PerfDataCriticalSection);

    /* Retrieve basic system information, including the number of processors in the system */
    status = NtQuerySystemInformation(SystemBasicInformation, &SystemBasicInfo, sizeof(SystemBasicInfo), NULL);
    if (!NT_SUCCESS(status))
    {
        // ZeroMemory(&SystemBasicInfo, sizeof(SystemBasicInfo));
        return FALSE;
    }

    /* Allocate the array for per-processor time information */
    // TODO: For supporting more CPUs, see e.g.
    // https://github.com/processhacker/processhacker/commit/8b3a09f99d06e45df64bfee2ce4c91d7c02b8b3d
    // but this should be Win7+.
    BufferSize = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * SystemBasicInfo.NumberOfProcessors;
    SystemProcessorTimeInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);

    /* Allocate an array for per-CPU times. If allocation fails (e.g.
     * low memory resources), use the mean times instead. */
    ProcessorTimes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                               sizeof(CPU_TIMES) * SystemBasicInfo.NumberOfProcessors);
    if (!ProcessorTimes)
        ProcessorTimes = &ProcessorMeanTimes;

    /* Create the SYSTEM Sid */
    AllocateAndInitializeSid(&NtSidAuthority, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &SystemUserSid);
    return TRUE;
}

VOID PerfDataUninitialize(VOID)
{
    PLIST_ENTRY pCur;
    PSIDTOUSERNAME pEntry;

    if (pPerfData)
    {
        HeapFree(GetProcessHeap(), 0, pPerfData);
        pPerfData = NULL;
    }

    DeleteCriticalSection(&PerfDataCriticalSection);

    if (SystemUserSid)
    {
        FreeSid(SystemUserSid);
        SystemUserSid = NULL;
    }

    /* Free user names cache list */
    pCur = SidToUserNameHead.Flink;
    while (pCur != &SidToUserNameHead)
    {
        pEntry = CONTAINING_RECORD(pCur, SIDTOUSERNAME, List);
        pCur = pCur->Flink;
        HeapFree(GetProcessHeap(), 0, pEntry);
    }

    if (ProcessorTimes && (ProcessorTimes != &ProcessorMeanTimes))
    {
        HeapFree(GetProcessHeap(), 0, ProcessorTimes);
        ProcessorTimes = NULL;
    }

    if (SystemProcessorTimeInfo)
    {
        HeapFree(GetProcessHeap(), 0, SystemProcessorTimeInfo);
        SystemProcessorTimeInfo = NULL;
    }
}

static VOID
SidToUserName(
    _In_ PSID Sid,
    _Out_ LPWSTR szBuffer,
    _In_ DWORD BufferSize)
{
    static WCHAR szDomainNameUnused[255];
    DWORD DomainNameLen = _countof(szDomainNameUnused);
    SID_NAME_USE Use;

    if (Sid != NULL)
        LookupAccountSidW(NULL, Sid, szBuffer, &BufferSize, szDomainNameUnused, &DomainNameLen, &Use);
}

/* Should be from utildll.dll */
VOID
WINAPI
CachedGetUserFromSid(
    _In_ PSID pSid,
    _Out_ LPWSTR pUserName,
    _Inout_ PULONG pcwcUserName)
{
    PLIST_ENTRY pCur;
    PSIDTOUSERNAME pEntry;
    ULONG cbSid, cwcUserName;

    cwcUserName = *pcwcUserName;

    /* Walk through the list */
    for (pCur = SidToUserNameHead.Flink;
         pCur != &SidToUserNameHead;
         pCur = pCur->Flink)
    {
        pEntry = CONTAINING_RECORD(pCur, SIDTOUSERNAME, List);
        if (EqualSid((PSID)&pEntry->Data, pSid))
        {
            wcsncpy(pUserName, pEntry->pszName, cwcUserName);
            *pcwcUserName = wcslen(pUserName);
            return;
        }
    }

    /* We didn't find the SID in the list, get the name conventional */
    SidToUserName(pSid, pUserName, cwcUserName);
    *pcwcUserName = wcslen(pUserName);

    /* Allocate a new entry */
    cwcUserName = *pcwcUserName + 1;
    cbSid = GetLengthSid(pSid);
    pEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(SIDTOUSERNAME) + cbSid + cwcUserName * sizeof(WCHAR));

    /* Copy the Sid and name to our entry */
    CopySid(cbSid, (PSID)&pEntry->Data, pSid);
    pEntry->pszName = (LPWSTR)(pEntry->Data + cbSid);
    wcsncpy(pEntry->pszName, pUserName, cwcUserName);

    /* Insert the new entry */
    // InsertHeadList();
    pEntry->List.Flink = &SidToUserNameHead;
    pEntry->List.Blink = SidToUserNameHead.Blink;
    SidToUserNameHead.Blink->Flink = &pEntry->List;
    SidToUserNameHead.Blink = &pEntry->List;
}

VOID PerfDataRefresh(VOID)
{
    ULONG                       ulSize;
    NTSTATUS                    status;
    LPBYTE                      pBuffer;
    ULONG                       BufferSize;
    PSYSTEM_PROCESS_INFORMATION pSPI;
    PPERFDATA                   pPDOld;
    ULONG                       Idx, Idx2;
    HANDLE                      hProcess;
    HANDLE                      hProcessToken;

    SYSTEM_TIMEOFDAY_INFORMATION SysTimeInfo = {0};
#if (NTDDI_VERSION < NTDDI_WIN7)
    /* Larger on Win2k8/Win7 */
    DECLSPEC_ALIGN(8) BYTE SysPerfInfoData[sizeof(SYSTEM_PERFORMANCE_INFORMATION) + 16] = {0};
#else
    SYSTEM_PERFORMANCE_INFORMATION SysPerfInfoData = {0};
#endif
    PSYSTEM_PERFORMANCE_INFORMATION pSysPerfInfo = (PSYSTEM_PERFORMANCE_INFORMATION)SysPerfInfoData;
    SYSTEM_FILECACHE_INFORMATION SysCacheInfo  = {0};
    SYSTEM_HANDLE_INFORMATION    SysHandleInfo = {0};

    LARGE_INTEGER CurrentIdleTime, MeanIdleTime;
    LARGE_INTEGER CurrentKernelTime, MeanKernelTime;
    LARGE_INTEGER liCurrentTime;

    PSECURITY_DESCRIPTOR ProcessSD;
    PSID                 ProcessUser;
    ULONG                Buffer[64]; /* Must be 4 bytes aligned! */
    ULONG                cwcUserName;

    /*
     * Retrieve several pieces of system information.
     *
     * Retrieval may sometimes fail, either because we are actually interested
     * only in partial information (e.g. SystemHandleInformation), or because
     * we are low in memory resources, or because of (Windows 7 WOW64-only) bugs
     * (see below). In any of these cases, we try to fallback to sane values,
     * and continue retrieving other information.
     */

    /* Get new system time */
    status = NtQuerySystemInformation(SystemTimeOfDayInformation, &SysTimeInfo, sizeof(SysTimeInfo), NULL);
    if (!NT_SUCCESS(status))
        SysTimeInfo.CurrentTime = OldSystemTime;

    /*
     * Get general system performance info.
     *
     * The SYSTEM_PERFORMANCE_INFORMATION structure's size increased in Win7+.
     * NtQuerySystemInformation(SystemPerformanceInformation) also supports its
     * older version by filling in as much of the whole structure as possible,
     * and returns STATUS_SUCCESS. However, Windows 7 x64 WOW64 has a bug, in
     * wow64.dll!whNtQuerySystemInformation_SystemPerformanceInformation(),
     * that fails to handle the different structures sizes, only supporting
     * the largest possible one, and fails if the structure uses its legacy
     * size (no data is copied, and STATUS_INFO_LENGTH_MISMATCH is returned).
     * This problem thus happens only for 32-bit compiled code, but does not
     * happen with native 64-bit compiled code. This problems does not happen
     * on 32-bit Windows 7.
     */
    // ZeroMemory(&SysPerfInfoData, sizeof(SysPerfInfoData));
    status = NtQuerySystemInformation(SystemPerformanceInformation, &SysPerfInfoData, sizeof(SysPerfInfoData), NULL);
    if (!NT_SUCCESS(status))
        *pSysPerfInfo = SystemPerfInfo;

    /* Get system cache information */
    status = NtQuerySystemInformation(SystemFileCacheInformation, &SysCacheInfo, sizeof(SysCacheInfo), NULL);
    if (!NT_SUCCESS(status))
        SysCacheInfo = SystemCacheInfo;

    /* Get handle information. Number of handles is enough, no need for data array. */
    status = NtQuerySystemInformation(SystemHandleInformation, &SysHandleInfo, sizeof(SysHandleInfo), NULL);
    if (!NT_SUCCESS(status) && (status != STATUS_INFO_LENGTH_MISMATCH))
        SysHandleInfo.NumberOfHandles = SystemNumberOfHandles;

    /* Get process information.
     * We don't know how much data there is so just keep
     * increasing the buffer size until the call succeeds.
     */
    BufferSize = 0;
    do
    {
        BufferSize += 0x10000;
        pBuffer = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, BufferSize);

        status = NtQuerySystemInformation(SystemProcessInformation, pBuffer, BufferSize, &ulSize);
        if (status == STATUS_INFO_LENGTH_MISMATCH)
            HeapFree(GetProcessHeap(), 0, pBuffer);

    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    EnterCriticalSection(&PerfDataCriticalSection);

    /* Save system performance, cache, and handle information */
    SystemPerfInfo = *pSysPerfInfo;
    SystemCacheInfo = SysCacheInfo;
    SystemNumberOfHandles = SysHandleInfo.NumberOfHandles;

    /* If it's a first call, skip idle time calcs */
    if (OldSystemTime.QuadPart != 0)
    {
        liCurrentTime.QuadPart = SysTimeInfo.CurrentTime.QuadPart - OldSystemTime.QuadPart;
        dbSystemTime = Li2Double(liCurrentTime);
    }
    /* Store the system time */
    OldSystemTime = SysTimeInfo.CurrentTime;

    /* Get system processor time information. In case of failure, keep the old data. */
    ulSize = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * SystemBasicInfo.NumberOfProcessors;
    status = NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                                      SystemProcessorTimeInfo,
                                      ulSize,
                                      NULL);
    if (!NT_SUCCESS(status))
    {
        /* Keep the same data as before */
        // ZeroMemory(SystemProcessorTimeInfo, ulSize);
    }

    MeanIdleTime.QuadPart = 0;
    MeanKernelTime.QuadPart = 0;
    for (Idx = 0; Idx < SystemBasicInfo.NumberOfProcessors; Idx++)
    {
        /* IDLE Time */
        CurrentIdleTime = SystemProcessorTimeInfo[Idx].IdleTime;

        /* Kernel Time */
        CurrentKernelTime = SystemProcessorTimeInfo[Idx].KernelTime;
        CurrentKernelTime.QuadPart += SystemProcessorTimeInfo[Idx].DpcTime.QuadPart;
        CurrentKernelTime.QuadPart += SystemProcessorTimeInfo[Idx].InterruptTime.QuadPart;

        /* Save the per-CPU times */
        if (ProcessorTimes != &ProcessorMeanTimes)
        {
            /* If it's a first call, skip idle time calcs */
            if (ProcessorMeanTimes.OldIdleTime.QuadPart != 0)
            {
                /* CurrentValue = NewValue - OldValue */
                liCurrentTime.QuadPart  = CurrentIdleTime.QuadPart;
                liCurrentTime.QuadPart -= ProcessorTimes[Idx].OldIdleTime.QuadPart;
                ProcessorTimes[Idx].dbIdleTime = Li2Double(liCurrentTime);

                liCurrentTime.QuadPart  = CurrentKernelTime.QuadPart;
                liCurrentTime.QuadPart -= ProcessorTimes[Idx].OldKernelTime.QuadPart;
                ProcessorTimes[Idx].dbKernelTime = Li2Double(liCurrentTime);

                /* CurrentCpuIdle = IdleTime / SystemTime */
                ProcessorTimes[Idx].dbIdleTime   /= dbSystemTime;
                ProcessorTimes[Idx].dbKernelTime /= dbSystemTime;

                /* CurrentCpuUsage% = 100 - (CurrentCpuIdle * 100) */
                ProcessorTimes[Idx].dbIdleTime =
                    100.0 - ProcessorTimes[Idx].dbIdleTime * 100.0; /* + 0.5; */
                ProcessorTimes[Idx].dbKernelTime =
                    100.0 - ProcessorTimes[Idx].dbKernelTime * 100.0; /* + 0.5; */

                /* Sanitize the values (percentages between 0 and 100) */
                ProcessorTimes[Idx].dbIdleTime   = min(max(ProcessorTimes[Idx].dbIdleTime, 0.), 100.);
                ProcessorTimes[Idx].dbKernelTime = min(max(ProcessorTimes[Idx].dbKernelTime, 0.), 100.);
            }

            /* Store new CPU's idle and kernel times */
            ProcessorTimes[Idx].OldIdleTime   = CurrentIdleTime;
            ProcessorTimes[Idx].OldKernelTime = CurrentKernelTime;
        }

        /* Calculate the mean values as well */
        MeanIdleTime.QuadPart   += CurrentIdleTime.QuadPart;
        MeanKernelTime.QuadPart += CurrentKernelTime.QuadPart;
    }

    /* If it's a first call, skip idle time calcs */
    if (ProcessorMeanTimes.OldIdleTime.QuadPart != 0)
    {
        /* CurrentValue = NewValue - OldValue */
        liCurrentTime.QuadPart = MeanIdleTime.QuadPart - ProcessorMeanTimes.OldIdleTime.QuadPart;
        ProcessorMeanTimes.dbIdleTime = Li2Double(liCurrentTime);

        liCurrentTime.QuadPart = MeanKernelTime.QuadPart - ProcessorMeanTimes.OldKernelTime.QuadPart;
        ProcessorMeanTimes.dbKernelTime = Li2Double(liCurrentTime);

        /* CurrentCpuIdle = IdleTime / SystemTime */
        ProcessorMeanTimes.dbIdleTime   /= dbSystemTime;
        ProcessorMeanTimes.dbKernelTime /= dbSystemTime;

        /* CurrentCpuUsage% = 100 - (CurrentCpuIdle * 100) / NumberOfProcessors */
        ProcessorMeanTimes.dbIdleTime =
            100.0 - ProcessorMeanTimes.dbIdleTime * 100.0 / (double)SystemBasicInfo.NumberOfProcessors; /* + 0.5; */
        ProcessorMeanTimes.dbKernelTime =
            100.0 - ProcessorMeanTimes.dbKernelTime * 100.0 / (double)SystemBasicInfo.NumberOfProcessors; /* + 0.5; */

        /* Sanitize the values (percentages between 0 and 100) */
        ProcessorMeanTimes.dbIdleTime   = min(max(ProcessorMeanTimes.dbIdleTime, 0.), 100.);
        ProcessorMeanTimes.dbKernelTime = min(max(ProcessorMeanTimes.dbKernelTime, 0.), 100.);
    }

    /* Store the mean CPU times */
    ProcessorMeanTimes.OldIdleTime   = MeanIdleTime;
    ProcessorMeanTimes.OldKernelTime = MeanKernelTime;

    /* Determine the process count.
     * We loop through the data we got from NtQuerySystemInformation
     * and count how many structures there are (until NextEntryOffset is 0)
     */
    ProcessCountOld = ProcessCount;
    ProcessCount = 0;
    for (pSPI = (PSYSTEM_PROCESS_INFORMATION)pBuffer;
         pSPI && (pSPI->NextEntryOffset != 0);
         pSPI = (PSYSTEM_PROCESS_INFORMATION)((ULONG_PTR)pSPI + pSPI->NextEntryOffset))
    {
        ProcessCount++;
    }

    /* Now alloc a new PERFDATA array and fill in the data */
    pPerfData = (PPERFDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PERFDATA) * ProcessCount);

    pSPI = (PSYSTEM_PROCESS_INFORMATION)pBuffer;
    for (Idx=0; Idx<ProcessCount; Idx++)
    {
        /* Get the old perf data for this process (if any)
         * so that we can establish delta values */
        pPDOld = NULL;
        if (pPerfDataOld)
        {
            for (Idx2=0; Idx2<ProcessCountOld; Idx2++)
            {
                if (pPerfDataOld[Idx2].ProcessId == pSPI->UniqueProcessId)
                {
                    pPDOld = &pPerfDataOld[Idx2];
                    break;
                }
            }
        }

        if (pSPI->ImageName.Buffer)
        {
            StringCbCopyNW(pPerfData[Idx].ImageName, sizeof(pPerfData[Idx].ImageName),
                           pSPI->ImageName.Buffer, pSPI->ImageName.Length);
        }
        else
        {
            LoadStringW(hInst, IDS_IDLE_PROCESS,
                        pPerfData[Idx].ImageName,
                        _countof(pPerfData[Idx].ImageName));
        }

        pPerfData[Idx].ProcessId = pSPI->UniqueProcessId;

        if (pPDOld)
        {
            double    CurTime = Li2Double(pSPI->KernelTime) + Li2Double(pSPI->UserTime);
            double    OldTime = Li2Double(pPDOld->KernelTime) + Li2Double(pPDOld->UserTime);
            double    CpuTime = (CurTime - OldTime) / dbSystemTime;
            CpuTime = CpuTime * 100.0 / (double)SystemBasicInfo.NumberOfProcessors; /* + 0.5; */
            pPerfData[Idx].CPUUsage = (ULONG)CpuTime;
        }
        pPerfData[Idx].CPUTime.QuadPart = pSPI->UserTime.QuadPart + pSPI->KernelTime.QuadPart;
        pPerfData[Idx].WorkingSetSizeBytes = pSPI->WorkingSetSize;
        pPerfData[Idx].PeakWorkingSetSizeBytes = pSPI->PeakWorkingSetSize;

        if (pPDOld)
            pPerfData[Idx].WorkingSetSizeDelta = labs((LONG)pSPI->WorkingSetSize - (LONG)pPDOld->WorkingSetSizeBytes);
        else
            pPerfData[Idx].WorkingSetSizeDelta = 0;

        pPerfData[Idx].PageFaultCount = pSPI->PageFaultCount;

        if (pPDOld)
            pPerfData[Idx].PageFaultCountDelta = labs((LONG)pSPI->PageFaultCount - (LONG)pPDOld->PageFaultCount);
        else
            pPerfData[Idx].PageFaultCountDelta = 0;

        pPerfData[Idx].VirtualMemorySizeBytes = pSPI->VirtualSize;
        pPerfData[Idx].PagedPoolUsagePages = pSPI->QuotaPeakPagedPoolUsage;
        pPerfData[Idx].NonPagedPoolUsagePages = pSPI->QuotaPeakNonPagedPoolUsage;
        pPerfData[Idx].BasePriority = pSPI->BasePriority;
        pPerfData[Idx].HandleCount = pSPI->HandleCount;
        pPerfData[Idx].ThreadCount = pSPI->NumberOfThreads;
        pPerfData[Idx].SessionId = pSPI->SessionId;
        pPerfData[Idx].UserName[0] = UNICODE_NULL;
        pPerfData[Idx].USERObjectCount = 0;
        pPerfData[Idx].GDIObjectCount = 0;
        ProcessUser = SystemUserSid;
        ProcessSD = NULL;

        if (pSPI->UniqueProcessId != NULL)
        {
            hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | READ_CONTROL, FALSE, PtrToUlong(pSPI->UniqueProcessId));
            if (hProcess)
            {
                /* Don't query the information of the system process. It's possible but
                 * returns Administrators as the owner of the process instead of SYSTEM. */
                if (pSPI->UniqueProcessId != (HANDLE)0x4)
                {
                    if (OpenProcessToken(hProcess, TOKEN_QUERY, &hProcessToken))
                    {
                        DWORD RetLen = 0;
                        BOOL Ret;

                        Ret = GetTokenInformation(hProcessToken, TokenUser, (LPVOID)Buffer, sizeof(Buffer), &RetLen);
                        CloseHandle(hProcessToken);

                        if (Ret)
                            ProcessUser = ((PTOKEN_USER)Buffer)->User.Sid;
                        else
                            goto ReadProcOwner;
                    }
                    else
                    {
ReadProcOwner:
                        GetSecurityInfo(hProcess, SE_KERNEL_OBJECT, OWNER_SECURITY_INFORMATION, &ProcessUser, NULL, NULL, NULL, &ProcessSD);
                    }

                    pPerfData[Idx].USERObjectCount = GetGuiResources(hProcess, GR_USEROBJECTS);
                    pPerfData[Idx].GDIObjectCount = GetGuiResources(hProcess, GR_GDIOBJECTS);
                }

                GetProcessIoCounters(hProcess, &pPerfData[Idx].IOCounters);
                CloseHandle(hProcess);
            }
            else
            {
                goto ClearInfo;
            }
        }
        else
        {
ClearInfo:
            /* Clear information we were unable to fetch */
            ZeroMemory(&pPerfData[Idx].IOCounters, sizeof(IO_COUNTERS));
        }

        cwcUserName = _countof(pPerfData[0].UserName);
        CachedGetUserFromSid(ProcessUser, pPerfData[Idx].UserName, &cwcUserName);

        if (ProcessSD != NULL)
            LocalFree((HLOCAL)ProcessSD);

        pPerfData[Idx].UserTime.QuadPart = pSPI->UserTime.QuadPart;
        pPerfData[Idx].KernelTime.QuadPart = pSPI->KernelTime.QuadPart;
        pSPI = (PSYSTEM_PROCESS_INFORMATION)((ULONG_PTR)pSPI + pSPI->NextEntryOffset);
    }
    HeapFree(GetProcessHeap(), 0, pBuffer);
    if (pPerfDataOld)
        HeapFree(GetProcessHeap(), 0, pPerfDataOld);
    pPerfDataOld = pPerfData;

    LeaveCriticalSection(&PerfDataCriticalSection);
}

VOID PerfDataAcquireLock(VOID)
{
    EnterCriticalSection(&PerfDataCriticalSection);
}

VOID PerfDataReleaseLock(VOID)
{
    LeaveCriticalSection(&PerfDataCriticalSection);
}

ULONG PerfDataGetProcessIndex(ULONG pid)
{
    ULONG idx;

    EnterCriticalSection(&PerfDataCriticalSection);

    for (idx = 0; idx < ProcessCount; idx++)
    {
        if (PtrToUlong(pPerfData[idx].ProcessId) == pid)
        {
            break;
        }
    }

    LeaveCriticalSection(&PerfDataCriticalSection);

    if (idx == ProcessCount)
    {
        return -1;
    }
    return idx;
}

ULONG PerfDataGetProcessCount(void)
{
    ULONG Result;
    EnterCriticalSection(&PerfDataCriticalSection);
    Result = ProcessCount;
    LeaveCriticalSection(&PerfDataCriticalSection);
    return Result;
}

ULONG PerfDataGetProcessorCount(VOID)
{
    /* Note: No need for locking since this is a one-time initialized data */
    // TODO: Investigate how this should be fixed with NUMA support.
    return SystemBasicInfo.NumberOfProcessors;
}

ULONG PerfDataGetProcessorUsage(VOID)
{
    ULONG Result;
    EnterCriticalSection(&PerfDataCriticalSection);
    Result = (ULONG)ProcessorMeanTimes.dbIdleTime;
    LeaveCriticalSection(&PerfDataCriticalSection);
    return Result;
}

ULONG PerfDataGetProcessorSystemUsage(VOID)
{
    ULONG Result;
    EnterCriticalSection(&PerfDataCriticalSection);
    Result = (ULONG)ProcessorMeanTimes.dbKernelTime;
    LeaveCriticalSection(&PerfDataCriticalSection);
    return Result;
}

ULONG PerfDataGetProcessorUsagePerCPU(ULONG CPUIndex)
{
    if (CPUIndex >= SystemBasicInfo.NumberOfProcessors)
        return 0;
    return (ULONG)ProcessorTimes[CPUIndex].dbIdleTime;
}

ULONG PerfDataGetProcessorSystemUsagePerCPU(ULONG CPUIndex)
{
    if (CPUIndex >= SystemBasicInfo.NumberOfProcessors)
        return 0;
    return (ULONG)ProcessorTimes[CPUIndex].dbKernelTime;
}

BOOL PerfDataGetImageName(ULONG Index, LPWSTR lpImageName, ULONG nMaxCount)
{
    BOOL  bSuccessful;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount) {
        wcsncpy(lpImageName, pPerfData[Index].ImageName, nMaxCount);
        bSuccessful = TRUE;
    } else {
        bSuccessful = FALSE;
    }
    LeaveCriticalSection(&PerfDataCriticalSection);
    return bSuccessful;
}

ULONG PerfDataGetProcessId(ULONG Index)
{
    ULONG  ProcessId;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        ProcessId = PtrToUlong(pPerfData[Index].ProcessId);
    else
        ProcessId = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return ProcessId;
}

BOOL PerfDataGetUserName(ULONG Index, LPWSTR lpUserName, ULONG nMaxCount)
{
    BOOL  bSuccessful;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount) {
        wcsncpy(lpUserName, pPerfData[Index].UserName, nMaxCount);
        bSuccessful = TRUE;
    } else {
        bSuccessful = FALSE;
    }

    LeaveCriticalSection(&PerfDataCriticalSection);

    return bSuccessful;
}

BOOL PerfDataGetCommandLine(ULONG Index, LPWSTR lpCommandLine, ULONG nMaxCount)
{
    static const LPWSTR ellipsis = L"...";

    PROCESS_BASIC_INFORMATION pbi = {0};
    UNICODE_STRING CommandLineStr = {0};

    PVOID ProcessParams = NULL;
    HANDLE hProcess;
    ULONG ProcessId;

    NTSTATUS Status;
    BOOL result;

    PCMD_LINE_CACHE new_entry;
    LPWSTR new_string;

    PCMD_LINE_CACHE cache = global_cache;

    /* [A] Search for a string already in cache? If so, use it */
    while (cache && cache->pnext != NULL)
    {
        if (cache->idx == Index && cache->str != NULL)
        {
            /* Found it. Use it, and add some ellipsis at the very end to make it cute */
            wcsncpy(lpCommandLine, cache->str, CMD_LINE_MIN(nMaxCount, cache->len));
            wcscpy(lpCommandLine + CMD_LINE_MIN(nMaxCount, cache->len) - wcslen(ellipsis), ellipsis);
            return TRUE;
        }

        cache = cache->pnext;
    }

    /* [B] We don't; let's allocate and load a value from the process mem... and cache it */
    ProcessId = PerfDataGetProcessId(Index);

    /* Default blank command line in case things don't work out */
    wcsncpy(lpCommandLine, L"", nMaxCount);

    /* Ask for a handle to the target process so that we can read its memory and query stuff */
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
    if (!hProcess)
        goto cleanup;

    /* First off, get the ProcessEnvironmentBlock location in that process' address space */
    Status = NtQueryInformationProcess(hProcess, 0, &pbi, sizeof(pbi), NULL);
    if (!NT_SUCCESS(Status))
        goto cleanup;

    /* Then get the PEB.ProcessParameters member pointer */
    result = ReadProcessMemory(hProcess,
                               (PVOID)((ULONG_PTR)pbi.PebBaseAddress + FIELD_OFFSET(PEB, ProcessParameters)),
                               &ProcessParams,
                               sizeof(ProcessParams),
                               NULL);
    if (!result)
        goto cleanup;

    /* Then copy the PEB->ProcessParameters.CommandLine member
       to get the pointer to the string buffer and its size */
    result = ReadProcessMemory(hProcess,
                               (PVOID)((ULONG_PTR)ProcessParams + FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, CommandLine)),
                               &CommandLineStr,
                               sizeof(CommandLineStr),
                               NULL);
    if (!result)
        goto cleanup;

    /* Allocate the next cache entry and its accompanying string in one go */
    new_entry = HeapAlloc(GetProcessHeap(),
                          HEAP_ZERO_MEMORY,
                          sizeof(CMD_LINE_CACHE) + CommandLineStr.Length + sizeof(UNICODE_NULL));
    if (!new_entry)
        goto cleanup;

    new_string = (LPWSTR)((ULONG_PTR)new_entry + sizeof(CMD_LINE_CACHE));

    /* Bingo, the command line should be stored there,
       copy the string from the other process */
    result = ReadProcessMemory(hProcess,
                               CommandLineStr.Buffer,
                               new_string,
                               CommandLineStr.Length,
                               NULL);
    if (!result)
    {
        /* Weird, after successfully reading the mem of that process
           various times it fails now, forget it and bail out */
        HeapFree(GetProcessHeap(), 0, new_entry);
        goto cleanup;
    }

    /* Add our pointer to the cache... */
    new_entry->idx = Index;
    new_entry->str = new_string;
    new_entry->len = CommandLineStr.Length;

    if (!global_cache)
        global_cache = new_entry;
    else
        cache->pnext = new_entry;

    /* ... and print the buffer for the first time */
    wcsncpy(lpCommandLine, new_string, CMD_LINE_MIN(nMaxCount, CommandLineStr.Length));

cleanup:
    if (hProcess) CloseHandle(hProcess);
    return TRUE;
}

void PerfDataDeallocCommandLineCache()
{
    PCMD_LINE_CACHE cache, pnext;

    for (cache = global_cache; cache; cache = pnext)
    {
        pnext = cache->pnext;
        HeapFree(GetProcessHeap(), 0, cache);
    }

    global_cache = NULL;
}

ULONG PerfDataGetSessionId(ULONG Index)
{
    ULONG  SessionId;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        SessionId = pPerfData[Index].SessionId;
    else
        SessionId = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return SessionId;
}

ULONG PerfDataGetCPUUsage(ULONG Index)
{
    ULONG  CpuUsage;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        CpuUsage = pPerfData[Index].CPUUsage;
    else
        CpuUsage = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return CpuUsage;
}

LARGE_INTEGER PerfDataGetCPUTime(ULONG Index)
{
    LARGE_INTEGER  CpuTime = {{0,0}};

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        CpuTime = pPerfData[Index].CPUTime;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return CpuTime;
}

ULONG PerfDataGetWorkingSetSizeBytes(ULONG Index)
{
    ULONG  WorkingSetSizeBytes;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        WorkingSetSizeBytes = pPerfData[Index].WorkingSetSizeBytes;
    else
        WorkingSetSizeBytes = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return WorkingSetSizeBytes;
}

ULONG PerfDataGetPeakWorkingSetSizeBytes(ULONG Index)
{
    ULONG  PeakWorkingSetSizeBytes;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        PeakWorkingSetSizeBytes = pPerfData[Index].PeakWorkingSetSizeBytes;
    else
        PeakWorkingSetSizeBytes = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return PeakWorkingSetSizeBytes;
}

ULONG PerfDataGetWorkingSetSizeDelta(ULONG Index)
{
    ULONG  WorkingSetSizeDelta;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        WorkingSetSizeDelta = pPerfData[Index].WorkingSetSizeDelta;
    else
        WorkingSetSizeDelta = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return WorkingSetSizeDelta;
}

ULONG PerfDataGetPageFaultCount(ULONG Index)
{
    ULONG  PageFaultCount;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        PageFaultCount = pPerfData[Index].PageFaultCount;
    else
        PageFaultCount = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return PageFaultCount;
}

ULONG PerfDataGetPageFaultCountDelta(ULONG Index)
{
    ULONG  PageFaultCountDelta;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        PageFaultCountDelta = pPerfData[Index].PageFaultCountDelta;
    else
        PageFaultCountDelta = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return PageFaultCountDelta;
}

ULONG PerfDataGetVirtualMemorySizeBytes(ULONG Index)
{
    ULONG  VirtualMemorySizeBytes;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        VirtualMemorySizeBytes = pPerfData[Index].VirtualMemorySizeBytes;
    else
        VirtualMemorySizeBytes = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return VirtualMemorySizeBytes;
}

ULONG PerfDataGetPagedPoolUsagePages(ULONG Index)
{
    ULONG  PagedPoolUsage;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        PagedPoolUsage = pPerfData[Index].PagedPoolUsagePages;
    else
        PagedPoolUsage = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return PagedPoolUsage;
}

ULONG PerfDataGetNonPagedPoolUsagePages(ULONG Index)
{
    ULONG  NonPagedPoolUsage;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        NonPagedPoolUsage = pPerfData[Index].NonPagedPoolUsagePages;
    else
        NonPagedPoolUsage = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return NonPagedPoolUsage;
}

ULONG PerfDataGetBasePriority(ULONG Index)
{
    ULONG  BasePriority;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        BasePriority = pPerfData[Index].BasePriority;
    else
        BasePriority = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return BasePriority;
}

ULONG PerfDataGetHandleCount(ULONG Index)
{
    ULONG  HandleCount;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        HandleCount = pPerfData[Index].HandleCount;
    else
        HandleCount = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return HandleCount;
}

ULONG PerfDataGetThreadCount(ULONG Index)
{
    ULONG  ThreadCount;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        ThreadCount = pPerfData[Index].ThreadCount;
    else
        ThreadCount = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return ThreadCount;
}

ULONG PerfDataGetUSERObjectCount(ULONG Index)
{
    ULONG  USERObjectCount;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        USERObjectCount = pPerfData[Index].USERObjectCount;
    else
        USERObjectCount = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return USERObjectCount;
}

ULONG PerfDataGetGDIObjectCount(ULONG Index)
{
    ULONG  GDIObjectCount;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        GDIObjectCount = pPerfData[Index].GDIObjectCount;
    else
        GDIObjectCount = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return GDIObjectCount;
}

BOOL PerfDataGetIOCounters(ULONG Index, PIO_COUNTERS pIoCounters)
{
    BOOL  bSuccessful;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
    {
        memcpy(pIoCounters, &pPerfData[Index].IOCounters, sizeof(IO_COUNTERS));
        bSuccessful = TRUE;
    }
    else
        bSuccessful = FALSE;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return bSuccessful;
}

VOID
PerfDataGetCommitChargeK(
    _Out_opt_ PULONGLONG ChargeTotal,
    _Out_opt_ PULONGLONG ChargeLimit,
    _Out_opt_ PULONGLONG ChargePeak)
{
    ULONG Total;
    ULONG Limit;
    ULONG Peak;
    ULONG PageSize;

    EnterCriticalSection(&PerfDataCriticalSection);

    Total = SystemPerfInfo.CommittedPages;
    Limit = SystemPerfInfo.CommitLimit;
    Peak  = SystemPerfInfo.PeakCommitment;
    PageSize = SystemBasicInfo.PageSize;

    LeaveCriticalSection(&PerfDataCriticalSection);

    if (ChargeTotal)
        *ChargeTotal = (ULONGLONG)Total * (PageSize / 1024);
    if (ChargeLimit)
        *ChargeLimit = (ULONGLONG)Limit * (PageSize / 1024);
    if (ChargePeak)
        *ChargePeak = (ULONGLONG)Peak * (PageSize / 1024);
}

VOID
PerfDataGetKernelMemoryK(
    _Out_opt_ PULONGLONG MemTotal,
    _Out_opt_ PULONGLONG MemPaged,
    _Out_opt_ PULONGLONG MemNonPaged)
{
    ULONG Paged;
    ULONG NonPaged;
    ULONG PageSize;

    EnterCriticalSection(&PerfDataCriticalSection);

    Paged    = SystemPerfInfo.PagedPoolPages;
    NonPaged = SystemPerfInfo.NonPagedPoolPages;
    PageSize = SystemBasicInfo.PageSize;

    LeaveCriticalSection(&PerfDataCriticalSection);

    if (MemTotal)
        *MemTotal = (ULONGLONG)(Paged + NonPaged) * (PageSize / 1024);
    if (MemPaged)
        *MemPaged = (ULONGLONG)Paged * (PageSize / 1024);
    if (MemNonPaged)
        *MemNonPaged = (ULONGLONG)NonPaged * (PageSize / 1024);
}

VOID
PerfDataGetPhysicalMemoryK(
    _Out_opt_ PULONGLONG MemTotal,
    _Out_opt_ PULONGLONG MemAvailable,
    _Out_opt_ PULONGLONG MemSysCache)
{
    ULONG Total;
    ULONG Available;
    ULONG SystemCache;
    ULONG PageSize;

    EnterCriticalSection(&PerfDataCriticalSection);

    Total = SystemBasicInfo.NumberOfPhysicalPages;
    Available   = SystemPerfInfo.AvailablePages;
    SystemCache = SystemCacheInfo.CurrentSizeIncludingTransitionInPages;
    PageSize = SystemBasicInfo.PageSize;

    LeaveCriticalSection(&PerfDataCriticalSection);

    if (MemTotal)
        *MemTotal = (ULONGLONG)Total * (PageSize / 1024);
    if (MemAvailable)
        *MemAvailable = (ULONGLONG)Available * (PageSize / 1024);
    if (MemSysCache)
        *MemSysCache = (ULONGLONG)SystemCache * (PageSize / 1024);
}

ULONG PerfDataGetSystemHandleCount(void)
{
    ULONG  HandleCount;

    EnterCriticalSection(&PerfDataCriticalSection);

    HandleCount = SystemNumberOfHandles;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return HandleCount;
}

ULONG PerfDataGetTotalThreadCount(void)
{
    ULONG  ThreadCount = 0;
    ULONG  i;

    EnterCriticalSection(&PerfDataCriticalSection);

    for (i=0; i<ProcessCount; i++)
    {
        ThreadCount += pPerfData[i].ThreadCount;
    }

    LeaveCriticalSection(&PerfDataCriticalSection);

    return ThreadCount;
}

BOOL PerfDataGet(ULONG Index, PPERFDATA *lppData)
{
    BOOL  bSuccessful = FALSE;

    EnterCriticalSection(&PerfDataCriticalSection);
    if (Index < ProcessCount)
    {
        *lppData = pPerfData + Index;
        bSuccessful = TRUE;
    }
    LeaveCriticalSection(&PerfDataCriticalSection);
    return bSuccessful;
}

