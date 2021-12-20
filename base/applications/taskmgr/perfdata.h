/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Performance Counters
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2014 Ismael Ferreras Morezuelas <swyterzone+ros@gmail.com>
 */

#pragma once

#define Li2Double(x) ((double)((x).HighPart) * 4.294967296E9 + (double)((x).LowPart))

typedef struct _PERFDATA
{
	WCHAR				ImageName[MAX_PATH];
	HANDLE				ProcessId;
	WCHAR				UserName[MAX_PATH];
	ULONG				SessionId;
	ULONG				CPUUsage;
	LARGE_INTEGER		CPUTime;
	ULONG				WorkingSetSizeBytes;
	ULONG				PeakWorkingSetSizeBytes;
	ULONG				WorkingSetSizeDelta;
	ULONG				PageFaultCount;
	ULONG				PageFaultCountDelta;
	ULONG				VirtualMemorySizeBytes;
	ULONG				PagedPoolUsagePages;
	ULONG				NonPagedPoolUsagePages;
	ULONG				BasePriority;
	ULONG				HandleCount;
	ULONG				ThreadCount;
	ULONG				USERObjectCount;
	ULONG				GDIObjectCount;
	IO_COUNTERS			IOCounters;

	LARGE_INTEGER		UserTime;
	LARGE_INTEGER		KernelTime;
} PERFDATA, *PPERFDATA;

typedef struct _CMD_LINE_CACHE
{
    DWORD  idx;
    LPWSTR str;
    ULONG  len;
    struct _CMD_LINE_CACHE* pnext;
} CMD_LINE_CACHE, *PCMD_LINE_CACHE;

BOOL PerfDataInitialize(VOID);
VOID PerfDataUninitialize(VOID);
VOID PerfDataRefresh(VOID);

VOID PerfDataAcquireLock(VOID);
VOID PerfDataReleaseLock(VOID);

BOOL	PerfDataGet(ULONG Index, PPERFDATA *lppData);
ULONG	PerfDataGetProcessIndex(ULONG pid);
ULONG	PerfDataGetProcessCount(void);

ULONG PerfDataGetProcessorCount(VOID);
ULONG PerfDataGetProcessorUsage(VOID);
ULONG PerfDataGetProcessorSystemUsage(VOID);
/****/
ULONG PerfDataGetProcessorUsagePerCPU(ULONG CPUIndex);
ULONG PerfDataGetProcessorSystemUsagePerCPU(ULONG CPUIndex);
/****/

BOOL	PerfDataGetImageName(ULONG Index, LPWSTR lpImageName, ULONG nMaxCount);
ULONG	PerfDataGetProcessId(ULONG Index);
BOOL	PerfDataGetUserName(ULONG Index, LPWSTR lpUserName, ULONG nMaxCount);

BOOL	PerfDataGetCommandLine(ULONG Index, LPWSTR lpCommandLine, ULONG nMaxCount);
void	PerfDataDeallocCommandLineCache();

ULONG	PerfDataGetSessionId(ULONG Index);
ULONG	PerfDataGetCPUUsage(ULONG Index);
LARGE_INTEGER	PerfDataGetCPUTime(ULONG Index);
ULONG	PerfDataGetWorkingSetSizeBytes(ULONG Index);
ULONG	PerfDataGetPeakWorkingSetSizeBytes(ULONG Index);
ULONG	PerfDataGetWorkingSetSizeDelta(ULONG Index);
ULONG	PerfDataGetPageFaultCount(ULONG Index);
ULONG	PerfDataGetPageFaultCountDelta(ULONG Index);
ULONG	PerfDataGetVirtualMemorySizeBytes(ULONG Index);
ULONG	PerfDataGetPagedPoolUsagePages(ULONG Index);
ULONG	PerfDataGetNonPagedPoolUsagePages(ULONG Index);
ULONG	PerfDataGetBasePriority(ULONG Index);
ULONG	PerfDataGetHandleCount(ULONG Index);
ULONG	PerfDataGetThreadCount(ULONG Index);
ULONG	PerfDataGetUSERObjectCount(ULONG Index);
ULONG	PerfDataGetGDIObjectCount(ULONG Index);
BOOL	PerfDataGetIOCounters(ULONG Index, PIO_COUNTERS pIoCounters);

VOID
PerfDataGetCommitChargeK(
    _Out_opt_ PULONGLONG Total,
    _Out_opt_ PULONGLONG Limit,
    _Out_opt_ PULONGLONG Peak);

VOID
PerfDataGetKernelMemoryK(
    _Out_opt_ PULONGLONG MemTotal,
    _Out_opt_ PULONGLONG MemPaged,
    _Out_opt_ PULONGLONG MemNonPaged);

VOID
PerfDataGetPhysicalMemoryK(
    _Out_opt_ PULONGLONG MemTotal,
    _Out_opt_ PULONGLONG MemAvailable,
    _Out_opt_ PULONGLONG MemSysCache);

ULONG	PerfDataGetSystemHandleCount(void);

ULONG	PerfDataGetTotalThreadCount(void);
