/*
 *  ReactOS Task Manager
 *
 *  perfdata.cpp
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
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

#include <precomp.h>

CRITICAL_SECTION                 PerfDataCriticalSection;
PPERFDATA                        pPerfDataOld = NULL;    /* Older perf data (saved to establish delta values) */
PPERFDATA                        pPerfData = NULL;    /* Most recent copy of perf data */
ULONG                            ProcessCountOld = 0;
ULONG                            ProcessCount = 0;
double                            dbIdleTime;
double                            dbKernelTime;
double                            dbSystemTime;
LARGE_INTEGER                    liOldIdleTime = {{0,0}};
double                            OldKernelTime = 0;
LARGE_INTEGER                    liOldSystemTime = {{0,0}};
SYSTEM_PERFORMANCE_INFORMATION    SystemPerfInfo;
SYSTEM_BASIC_INFORMATION        SystemBasicInfo;
SYSTEM_CACHE_INFORMATION        SystemCacheInfo;
SYSTEM_HANDLE_INFORMATION        SystemHandleInfo;
PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION SystemProcessorTimeInfo = NULL;

BOOL PerfDataInitialize(void)
{
    NTSTATUS    status;

    InitializeCriticalSection(&PerfDataCriticalSection);

    /*
     * Get number of processors in the system
     */
    status = NtQuerySystemInformation(SystemBasicInformation, &SystemBasicInfo, sizeof(SystemBasicInfo), NULL);
    if (status != NO_ERROR)
        return FALSE;

    return TRUE;
}

void PerfDataUninitialize(void)
{
    DeleteCriticalSection(&PerfDataCriticalSection);
}

void PerfDataRefresh(void)
{
    ULONG                            ulSize;
    NTSTATUS                          status;
    LPBYTE                            pBuffer;
    ULONG                            BufferSize;
    PSYSTEM_PROCESS_INFORMATION        pSPI;
    PPERFDATA                        pPDOld;
    ULONG                            Idx, Idx2;
    HANDLE                            hProcess;
    HANDLE                            hProcessToken;
    TCHAR                            szTemp[MAX_PATH];
    DWORD                            dwSize;
    SYSTEM_PERFORMANCE_INFORMATION    SysPerfInfo;
    SYSTEM_TIMEOFDAY_INFORMATION      SysTimeInfo;
    SYSTEM_CACHE_INFORMATION        SysCacheInfo;
    LPBYTE                            SysHandleInfoData;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION SysProcessorTimeInfo;
    double                            CurrentKernelTime;

    /* Get new system time */
    status = NtQuerySystemInformation(SystemTimeOfDayInformation, &SysTimeInfo, sizeof(SysTimeInfo), 0);
    if (status != NO_ERROR)
        return;

    /* Get new CPU's idle time */
    status = NtQuerySystemInformation(SystemPerformanceInformation, &SysPerfInfo, sizeof(SysPerfInfo), NULL);
    if (status != NO_ERROR)
        return;

    /* Get system cache information */
    status = NtQuerySystemInformation(SystemFileCacheInformation, &SysCacheInfo, sizeof(SysCacheInfo), NULL);
    if (status != NO_ERROR)
        return;

    /* Get processor time information */
    SysProcessorTimeInfo = (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)HeapAlloc(GetProcessHeap(), 0, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * SystemBasicInfo.NumberOfProcessors);
    status = NtQuerySystemInformation(SystemProcessorPerformanceInformation, SysProcessorTimeInfo, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * SystemBasicInfo.NumberOfProcessors, &ulSize);
    if (status != NO_ERROR)
        return;

    /* Get handle information
     * We don't know how much data there is so just keep
     * increasing the buffer size until the call succeeds
     */
    BufferSize = 0;
    do
    {
        BufferSize += 0x10000;
        SysHandleInfoData = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, BufferSize);

        status = NtQuerySystemInformation(SystemHandleInformation, SysHandleInfoData, BufferSize, &ulSize);

        if (status == STATUS_INFO_LENGTH_MISMATCH) {
            HeapFree(GetProcessHeap(), 0, SysHandleInfoData);
        }

    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    /* Get process information
     * We don't know how much data there is so just keep
     * increasing the buffer size until the call succeeds
     */
    BufferSize = 0;
    do
    {
        BufferSize += 0x10000;
        pBuffer = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, BufferSize);

        status = NtQuerySystemInformation(SystemProcessInformation, pBuffer, BufferSize, &ulSize);

        if (status == STATUS_INFO_LENGTH_MISMATCH) {
            HeapFree(GetProcessHeap(), 0, pBuffer);
        }

    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    EnterCriticalSection(&PerfDataCriticalSection);

    /*
     * Save system performance info
     */
    memcpy(&SystemPerfInfo, &SysPerfInfo, sizeof(SYSTEM_PERFORMANCE_INFORMATION));

    /*
     * Save system cache info
     */
    memcpy(&SystemCacheInfo, &SysCacheInfo, sizeof(SYSTEM_CACHE_INFORMATION));

    /*
     * Save system processor time info
     */
    if (SystemProcessorTimeInfo) {
        HeapFree(GetProcessHeap(), 0, SystemProcessorTimeInfo);
    }
    SystemProcessorTimeInfo = SysProcessorTimeInfo;

    /*
     * Save system handle info
     */
    memcpy(&SystemHandleInfo, SysHandleInfoData, sizeof(SYSTEM_HANDLE_INFORMATION));
    HeapFree(GetProcessHeap(), 0, SysHandleInfoData);

    for (CurrentKernelTime=0, Idx=0; Idx<(ULONG)SystemBasicInfo.NumberOfProcessors; Idx++) {
        CurrentKernelTime += Li2Double(SystemProcessorTimeInfo[Idx].KernelTime);
        CurrentKernelTime += Li2Double(SystemProcessorTimeInfo[Idx].DpcTime);
        CurrentKernelTime += Li2Double(SystemProcessorTimeInfo[Idx].InterruptTime);
    }

    /* If it's a first call - skip idle time calcs */
    if (liOldIdleTime.QuadPart != 0) {
        /*  CurrentValue = NewValue - OldValue */
        dbIdleTime = Li2Double(SysPerfInfo.IdleProcessTime) - Li2Double(liOldIdleTime);
        dbKernelTime = CurrentKernelTime - OldKernelTime;
        dbSystemTime = Li2Double(SysTimeInfo.CurrentTime) - Li2Double(liOldSystemTime);

        /*  CurrentCpuIdle = IdleTime / SystemTime */
        dbIdleTime = dbIdleTime / dbSystemTime;
        dbKernelTime = dbKernelTime / dbSystemTime;

        /*  CurrentCpuUsage% = 100 - (CurrentCpuIdle * 100) / NumberOfProcessors */
        dbIdleTime = 100.0 - dbIdleTime * 100.0 / (double)SystemBasicInfo.NumberOfProcessors; /* + 0.5; */
        dbKernelTime = 100.0 - dbKernelTime * 100.0 / (double)SystemBasicInfo.NumberOfProcessors; /* + 0.5; */
    }

    /* Store new CPU's idle and system time */
    liOldIdleTime = SysPerfInfo.IdleProcessTime;
    liOldSystemTime = SysTimeInfo.CurrentTime;
    OldKernelTime = CurrentKernelTime;

    /* Determine the process count
     * We loop through the data we got from NtQuerySystemInformation
     * and count how many structures there are (until RelativeOffset is 0)
     */
    ProcessCountOld = ProcessCount;
    ProcessCount = 0;
    pSPI = (PSYSTEM_PROCESS_INFORMATION)pBuffer;
    while (pSPI) {
        ProcessCount++;
        if (pSPI->NextEntryOffset == 0)
            break;
        pSPI = (PSYSTEM_PROCESS_INFORMATION)((LPBYTE)pSPI + pSPI->NextEntryOffset);
    }

    /* Now alloc a new PERFDATA array and fill in the data */
    if (pPerfDataOld) {
        HeapFree(GetProcessHeap(), 0, pPerfDataOld);
    }
    pPerfDataOld = pPerfData;
    pPerfData = (PPERFDATA)HeapAlloc(GetProcessHeap(), 0, sizeof(PERFDATA) * ProcessCount);
    pSPI = (PSYSTEM_PROCESS_INFORMATION)pBuffer;
    for (Idx=0; Idx<ProcessCount; Idx++) {
        /* Get the old perf data for this process (if any) */
        /* so that we can establish delta values */
        pPDOld = NULL;
        for (Idx2=0; Idx2<ProcessCountOld; Idx2++) {
            if (pPerfDataOld[Idx2].ProcessId == pSPI->UniqueProcessId) {
                pPDOld = &pPerfDataOld[Idx2];
                break;
            }
        }

        /* Clear out process perf data structure */
        memset(&pPerfData[Idx], 0, sizeof(PERFDATA));

        if (pSPI->ImageName.Buffer)
            wcscpy(pPerfData[Idx].ImageName, pSPI->ImageName.Buffer);
        else
            LoadStringW(hInst, IDS_IDLE_PROCESS, pPerfData[Idx].ImageName,
                        sizeof(pPerfData[Idx].ImageName) / sizeof(pPerfData[Idx].ImageName[0]));

        pPerfData[Idx].ProcessId = pSPI->UniqueProcessId;

        if (pPDOld)    {
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

        if (pSPI->UniqueProcessId != NULL) {
            hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, (DWORD)pSPI->UniqueProcessId);
            if (hProcess) {
                if (OpenProcessToken(hProcess, TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_IMPERSONATE, &hProcessToken)) {
                    ImpersonateLoggedOnUser(hProcessToken);
                    memset(szTemp, 0, sizeof(TCHAR[MAX_PATH]));
                    dwSize = MAX_PATH;
                    GetUserName(szTemp, &dwSize);
#ifndef UNICODE
                    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szTemp, -1, pPerfData[Idx].UserName, MAX_PATH);
/*
int MultiByteToWideChar(
  UINT CodePage,         // code page
  DWORD dwFlags,         //  character-type options
  LPCSTR lpMultiByteStr, //  string to map
  int cbMultiByte,       //  number of bytes in string
  LPWSTR lpWideCharStr,  //  wide-character buffer
  int cchWideChar        //  size of buffer
);
 */
#endif
                    RevertToSelf();
                    CloseHandle(hProcessToken);
                } else {
                    pPerfData[Idx].UserName[0] = _T('\0');
                }
                pPerfData[Idx].USERObjectCount = GetGuiResources(hProcess, GR_USEROBJECTS);
                pPerfData[Idx].GDIObjectCount = GetGuiResources(hProcess, GR_GDIOBJECTS);
                GetProcessIoCounters(hProcess, &pPerfData[Idx].IOCounters);
                CloseHandle(hProcess);
            } else {
                goto ClearInfo;
            }
        } else {
ClearInfo:
            /* clear information we were unable to fetch */
            pPerfData[Idx].UserName[0] = _T('\0');
            pPerfData[Idx].USERObjectCount = 0;
            pPerfData[Idx].GDIObjectCount = 0;
            ZeroMemory(&pPerfData[Idx].IOCounters, sizeof(IO_COUNTERS));
        }
        pPerfData[Idx].UserTime.QuadPart = pSPI->UserTime.QuadPart;
        pPerfData[Idx].KernelTime.QuadPart = pSPI->KernelTime.QuadPart;
        pSPI = (PSYSTEM_PROCESS_INFORMATION)((LPBYTE)pSPI + pSPI->NextEntryOffset);
    }
    HeapFree(GetProcessHeap(), 0, pBuffer);
    LeaveCriticalSection(&PerfDataCriticalSection);
}

ULONG PerfDataGetProcessCount(void)
{
    return ProcessCount;
}

ULONG PerfDataGetProcessorUsage(void)
{
    return (ULONG)dbIdleTime;
}

ULONG PerfDataGetProcessorSystemUsage(void)
{
    return (ULONG)dbKernelTime;
}

BOOL PerfDataGetImageName(ULONG Index, LPTSTR lpImageName, int nMaxCount)
{
    BOOL    bSuccessful;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount) {
        #ifdef _UNICODE
            wcsncpy(lpImageName, pPerfData[Index].ImageName, nMaxCount);
        #else
            WideCharToMultiByte(CP_ACP, 0, pPerfData[Index].ImageName, -1, lpImageName, nMaxCount, NULL, NULL);
        #endif

        bSuccessful = TRUE;
    } else {
        bSuccessful = FALSE;
    }
    LeaveCriticalSection(&PerfDataCriticalSection);
    return bSuccessful;
}

ULONG PerfDataGetProcessId(ULONG Index)
{
    ULONG    ProcessId;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        ProcessId = (ULONG)pPerfData[Index].ProcessId;
    else
        ProcessId = 0;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return ProcessId;
}

BOOL PerfDataGetUserName(ULONG Index, LPTSTR lpUserName, int nMaxCount)
{
    BOOL    bSuccessful;

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount) {
        #ifdef _UNICODE
            wcsncpy(lpUserName, pPerfData[Index].UserName, nMaxCount);
        #else
            WideCharToMultiByte(CP_ACP, 0, pPerfData[Index].UserName, -1, lpUserName, nMaxCount, NULL, NULL);
        #endif

        bSuccessful = TRUE;
    } else {
        bSuccessful = FALSE;
    }

    LeaveCriticalSection(&PerfDataCriticalSection);

    return bSuccessful;
}

ULONG PerfDataGetSessionId(ULONG Index)
{
    ULONG    SessionId;

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
    ULONG    CpuUsage;

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
    LARGE_INTEGER    CpuTime = {{0,0}};

    EnterCriticalSection(&PerfDataCriticalSection);

    if (Index < ProcessCount)
        CpuTime = pPerfData[Index].CPUTime;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return CpuTime;
}

ULONG PerfDataGetWorkingSetSizeBytes(ULONG Index)
{
    ULONG    WorkingSetSizeBytes;

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
    ULONG    PeakWorkingSetSizeBytes;

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
    ULONG    WorkingSetSizeDelta;

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
    ULONG    PageFaultCount;

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
    ULONG    PageFaultCountDelta;

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
    ULONG    VirtualMemorySizeBytes;

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
    ULONG    PagedPoolUsage;

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
    ULONG    NonPagedPoolUsage;

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
    ULONG    BasePriority;

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
    ULONG    HandleCount;

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
    ULONG    ThreadCount;

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
    ULONG    USERObjectCount;

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
    ULONG    GDIObjectCount;

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
    BOOL    bSuccessful;

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

ULONG PerfDataGetCommitChargeTotalK(void)
{
    ULONG    Total;
    ULONG    PageSize;

    EnterCriticalSection(&PerfDataCriticalSection);

    Total = SystemPerfInfo.CommittedPages;
    PageSize = SystemBasicInfo.PageSize;

    LeaveCriticalSection(&PerfDataCriticalSection);

    Total = Total * (PageSize / 1024);

    return Total;
}

ULONG PerfDataGetCommitChargeLimitK(void)
{
    ULONG    Limit;
    ULONG    PageSize;

    EnterCriticalSection(&PerfDataCriticalSection);

    Limit = SystemPerfInfo.CommitLimit;
    PageSize = SystemBasicInfo.PageSize;

    LeaveCriticalSection(&PerfDataCriticalSection);

    Limit = Limit * (PageSize / 1024);

    return Limit;
}

ULONG PerfDataGetCommitChargePeakK(void)
{
    ULONG    Peak;
    ULONG    PageSize;

    EnterCriticalSection(&PerfDataCriticalSection);

    Peak = SystemPerfInfo.PeakCommitment;
    PageSize = SystemBasicInfo.PageSize;

    LeaveCriticalSection(&PerfDataCriticalSection);

    Peak = Peak * (PageSize / 1024);

    return Peak;
}

ULONG PerfDataGetKernelMemoryTotalK(void)
{
    ULONG    Total;
    ULONG    Paged;
    ULONG    NonPaged;
    ULONG    PageSize;

    EnterCriticalSection(&PerfDataCriticalSection);

    Paged = SystemPerfInfo.PagedPoolPages;
    NonPaged = SystemPerfInfo.NonPagedPoolPages;
    PageSize = SystemBasicInfo.PageSize;

    LeaveCriticalSection(&PerfDataCriticalSection);

    Paged = Paged * (PageSize / 1024);
    NonPaged = NonPaged * (PageSize / 1024);

    Total = Paged + NonPaged;

    return Total;
}

ULONG PerfDataGetKernelMemoryPagedK(void)
{
    ULONG    Paged;
    ULONG    PageSize;

    EnterCriticalSection(&PerfDataCriticalSection);

    Paged = SystemPerfInfo.PagedPoolPages;
    PageSize = SystemBasicInfo.PageSize;

    LeaveCriticalSection(&PerfDataCriticalSection);

    Paged = Paged * (PageSize / 1024);

    return Paged;
}

ULONG PerfDataGetKernelMemoryNonPagedK(void)
{
    ULONG    NonPaged;
    ULONG    PageSize;

    EnterCriticalSection(&PerfDataCriticalSection);

    NonPaged = SystemPerfInfo.NonPagedPoolPages;
    PageSize = SystemBasicInfo.PageSize;

    LeaveCriticalSection(&PerfDataCriticalSection);

    NonPaged = NonPaged * (PageSize / 1024);

    return NonPaged;
}

ULONG PerfDataGetPhysicalMemoryTotalK(void)
{
    ULONG    Total;
    ULONG    PageSize;

    EnterCriticalSection(&PerfDataCriticalSection);

    Total = SystemBasicInfo.NumberOfPhysicalPages;
    PageSize = SystemBasicInfo.PageSize;

    LeaveCriticalSection(&PerfDataCriticalSection);

    Total = Total * (PageSize / 1024);

    return Total;
}

ULONG PerfDataGetPhysicalMemoryAvailableK(void)
{
    ULONG    Available;
    ULONG    PageSize;

    EnterCriticalSection(&PerfDataCriticalSection);

    Available = SystemPerfInfo.AvailablePages;
    PageSize = SystemBasicInfo.PageSize;

    LeaveCriticalSection(&PerfDataCriticalSection);

    Available = Available * (PageSize / 1024);

    return Available;
}

ULONG PerfDataGetPhysicalMemorySystemCacheK(void)
{
    ULONG    SystemCache;
    ULONG    PageSize;

    EnterCriticalSection(&PerfDataCriticalSection);

    SystemCache = SystemCacheInfo.CurrentSize;
    PageSize = SystemBasicInfo.PageSize;

    LeaveCriticalSection(&PerfDataCriticalSection);

    /* SystemCache = SystemCache * (PageSize / 1024); */
    SystemCache = SystemCache / 1024;

    return SystemCache;
}

ULONG PerfDataGetSystemHandleCount(void)
{
    ULONG    HandleCount;

    EnterCriticalSection(&PerfDataCriticalSection);

    HandleCount = SystemHandleInfo.NumberOfHandles;

    LeaveCriticalSection(&PerfDataCriticalSection);

    return HandleCount;
}

ULONG PerfDataGetTotalThreadCount(void)
{
    ULONG    ThreadCount = 0;
    ULONG    i;

    EnterCriticalSection(&PerfDataCriticalSection);

    for (i=0; i<ProcessCount; i++)
    {
        ThreadCount += pPerfData[i].ThreadCount;
    }

    LeaveCriticalSection(&PerfDataCriticalSection);

    return ThreadCount;
}
