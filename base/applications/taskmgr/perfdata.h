/*
 *  ReactOS Task Manager
 *
 *  perfdata.h
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

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
     DWORD idx;
    LPWSTR str;
     ULONG len;
    struct _CMD_LINE_CACHE* pnext;
} CMD_LINE_CACHE, *PCMD_LINE_CACHE;

BOOL	PerfDataInitialize(void);
void	PerfDataUninitialize(void);
void	PerfDataRefresh(void);

BOOL	PerfDataGet(ULONG Index, PPERFDATA *lppData);
ULONG	PerfDataGetProcessIndex(ULONG pid);
ULONG	PerfDataGetProcessCount(void);
ULONG	PerfDataGetProcessorUsage(void);
ULONG	PerfDataGetProcessorSystemUsage(void);

BOOL	PerfDataGetImageName(ULONG Index, LPWSTR lpImageName, ULONG nMaxCount);
ULONG	PerfDataGetProcessId(ULONG Index);
BOOL	PerfDataGetUserName(ULONG Index, LPWSTR lpUserName, ULONG nMaxCount);

BOOL	PerfDataGetCommandLine(ULONG Index, LPWSTR lpCommandLine, ULONG nMaxCount);
void	PerfDataDeallocCommandLineCache(void);

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

ULONG	PerfDataGetCommitChargeTotalK(void);
ULONG	PerfDataGetCommitChargeLimitK(void);
ULONG	PerfDataGetCommitChargePeakK(void);

ULONG	PerfDataGetKernelMemoryTotalK(void);
ULONG	PerfDataGetKernelMemoryPagedK(void);
ULONG	PerfDataGetKernelMemoryNonPagedK(void);

ULONG	PerfDataGetPhysicalMemoryTotalK(void);
ULONG	PerfDataGetPhysicalMemoryAvailableK(void);
ULONG	PerfDataGetPhysicalMemorySystemCacheK(void);

ULONG	PerfDataGetSystemHandleCount(void);

ULONG	PerfDataGetTotalThreadCount(void);


#ifdef __cplusplus
};
#endif
