/*
 *  ReactOS Task Manager
 *
 *  perfdata.h
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
	
#ifndef __PERFDATA_H
#define __PERFDATA_H

#if 0
typedef struct _TIME {
	DWORD LowPart;
	LONG HighPart;
} TIME, *PTIME;
#endif

typedef ULARGE_INTEGER	TIME, *PTIME;

//typedef WCHAR			UNICODE_STRING;
typedef struct _UNICODE_STRING {
    USHORT	Length;
    USHORT	MaximumLength;
    PWSTR	Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _PERFDATA
{
	WCHAR				ImageName[MAX_PATH];
	ULONG				ProcessId;
	WCHAR				UserName[MAX_PATH];
	ULONG				SessionId;
	ULONG				CPUUsage;
	TIME				CPUTime;
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

	TIME				UserTime;
	TIME				KernelTime;
} PERFDATA, *PPERFDATA;

typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef enum _KWAIT_REASON
{
   Executive,
   FreePage,
   PageIn,
   PoolAllocation,
   DelayExecution,
   Suspended,
   UserRequest,
   WrExecutive,
   WrFreePage,
   WrPageIn,
   WrDelayExecution,
   WrSuspended,
   WrUserRequest,
   WrQueue,
   WrLpcReceive,
   WrLpcReply,
   WrVirtualMemory,
   WrPageOut,
   WrRendezvous,
   Spare2,
   Spare3,
   Spare4,
   Spare5,
   Spare6,
   WrKernel,
   MaximumWaitReason,
} KWAIT_REASON;

// SystemProcessThreadInfo (5)
typedef struct _SYSTEM_THREAD_INFORMATION
{
	TIME		KernelTime;
	TIME		UserTime;
	TIME		CreateTime;
	ULONG		TickCount;
	ULONG		StartEIP;
	CLIENT_ID	ClientId;
	ULONG		DynamicPriority;
	ULONG		BasePriority;
	ULONG		nSwitches;
	DWORD		State;
	KWAIT_REASON	WaitReason;
	
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

typedef struct SYSTEM_PROCESS_INFORMATION
{
	ULONG				RelativeOffset;
	ULONG				ThreadCount;
	ULONG				Unused1 [6];
	TIME				CreateTime;
	TIME				UserTime;
	TIME				KernelTime;
	UNICODE_STRING		Name;
	ULONG				BasePriority;
	ULONG				ProcessId;
	ULONG				ParentProcessId;
	ULONG				HandleCount;
	ULONG				SessionId;
	ULONG				Unused2;
	ULONG				PeakVirtualSizeBytes;
	ULONG				TotalVirtualSizeBytes;
	ULONG				PageFaultCount;
	ULONG				PeakWorkingSetSizeBytes;
	ULONG				TotalWorkingSetSizeBytes;
	ULONG				PeakPagedPoolUsagePages;
	ULONG				TotalPagedPoolUsagePages;
	ULONG				PeakNonPagedPoolUsagePages;
	ULONG				TotalNonPagedPoolUsagePages;
	ULONG				TotalPageFileUsageBytes;
	ULONG				PeakPageFileUsageBytes;
	ULONG				TotalPrivateBytes;
	SYSTEM_THREAD_INFORMATION	ThreadSysInfo [1];
	
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef struct
{
	DWORD	dwUnknown1;
	ULONG	uKeMaximumIncrement;
	ULONG	uPageSize;
	ULONG	uMmNumberOfPhysicalPages;
	ULONG	uMmLowestPhysicalPage;
	ULONG	uMmHighestPhysicalPage;
	ULONG	uAllocationGranularity;
	PVOID	pLowestUserAddress;
	PVOID	pMmHighestUserAddress;
	ULONG	uKeActiveProcessors;
	BYTE	bKeNumberProcessors;
	BYTE	bUnknown2;
	WORD	wUnknown3;
} SYSTEM_BASIC_INFORMATION;

// SystemPerformanceInfo (2)
typedef struct _SYSTEM_PERFORMANCE_INFORMATION
{
	LARGE_INTEGER	/*TotalProcessorTime*/liIdleTime;
	LARGE_INTEGER	IoReadTransferCount;
	LARGE_INTEGER	IoWriteTransferCount;
	LARGE_INTEGER	IoOtherTransferCount;
	ULONG		IoReadOperationCount;
	ULONG		IoWriteOperationCount;
	ULONG		IoOtherOperationCount;
	ULONG		MmAvailablePages;
	ULONG		MmTotalCommitedPages;
	ULONG		MmTotalCommitLimit;
	ULONG		MmPeakLimit;
	ULONG		PageFaults;
	ULONG		WriteCopies;
	ULONG		TransitionFaults;
	ULONG		Unknown1;
	ULONG		DemandZeroFaults;
	ULONG		PagesInput;
	ULONG		PagesRead;
	ULONG		Unknown2;
	ULONG		Unknown3;
	ULONG		PagesOutput;
	ULONG		PageWrites;
	ULONG		Unknown4;
	ULONG		Unknown5;
	ULONG		PoolPagedBytes;
	ULONG		PoolNonPagedBytes;
	ULONG		Unknown6;
	ULONG		Unknown7;
	ULONG		Unknown8;
	ULONG		Unknown9;
	ULONG		MmTotalSystemFreePtes;
	ULONG		MmSystemCodepage;
	ULONG		MmTotalSystemDriverPages;
	ULONG		MmTotalSystemCodePages;
	ULONG		Unknown10;
	ULONG		Unknown11;
	ULONG		Unknown12;
	ULONG		MmSystemCachePage;
	ULONG		MmPagedPoolPage;
	ULONG		MmSystemDriverPage;
	ULONG		CcFastReadNoWait;
	ULONG		CcFastReadWait;
	ULONG		CcFastReadResourceMiss;
	ULONG		CcFastReadNotPossible;
	ULONG		CcFastMdlReadNoWait;
	ULONG		CcFastMdlReadWait;
	ULONG		CcFastMdlReadResourceMiss;
	ULONG		CcFastMdlReadNotPossible;
	ULONG		CcMapDataNoWait;
	ULONG		CcMapDataWait;
	ULONG		CcMapDataNoWaitMiss;
	ULONG		CcMapDataWaitMiss;
	ULONG		CcPinMappedDataCount;
	ULONG		CcPinReadNoWait;
	ULONG		CcPinReadWait;
	ULONG		CcPinReadNoWaitMiss;
	ULONG		CcPinReadWaitMiss;
	ULONG		CcCopyReadNoWait;
	ULONG		CcCopyReadWait;
	ULONG		CcCopyReadNoWaitMiss;
	ULONG		CcCopyReadWaitMiss;
	ULONG		CcMdlReadNoWait;
	ULONG		CcMdlReadWait;
	ULONG		CcMdlReadNoWaitMiss;
	ULONG		CcMdlReadWaitMiss;
	ULONG		CcReadaheadIos;
	ULONG		CcLazyWriteIos;
	ULONG		CcLazyWritePages;
	ULONG		CcDataFlushes;
	ULONG		CcDataPages;
	ULONG		ContextSwitches;
	ULONG		Unknown13;
	ULONG		Unknown14;
	ULONG		SystemCalls;

} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

typedef struct
{
	LARGE_INTEGER	liKeBootTime;
	LARGE_INTEGER	liKeSystemTime;
	LARGE_INTEGER	liExpTimeZoneBias;
	ULONG			uCurrentTimeZoneId;
	DWORD			dwReserved;
} SYSTEM_TIME_INFORMATION;

// SystemCacheInformation (21)
typedef struct _SYSTEM_CACHE_INFORMATION
{
	ULONG	CurrentSize;
	ULONG	PeakSize;
	ULONG	PageFaultCount;
	ULONG	MinimumWorkingSet;
	ULONG	MaximumWorkingSet;
	ULONG	Unused[4];

} SYSTEM_CACHE_INFORMATION;

// SystemPageFileInformation (18)
typedef
struct _SYSTEM_PAGEFILE_INFORMATION
{
	ULONG		RelativeOffset;
	ULONG		CurrentSizePages;
	ULONG		TotalUsedPages;
	ULONG		PeakUsedPages;
	UNICODE_STRING	PagefileFileName;
	
} SYSTEM_PAGEFILE_INFORMATION, *PSYSTEM_PAGEFILE_INFORMATION;

// SystemHandleInformation (16)
// (see ontypes.h)
typedef
struct _SYSTEM_HANDLE_ENTRY
{
	ULONG	OwnerPid;
	BYTE	ObjectType;
	BYTE	HandleFlags;
	USHORT	HandleValue;
	PVOID	ObjectPointer;
	ULONG	AccessMask;
	
} SYSTEM_HANDLE_ENTRY, *PSYSTEM_HANDLE_ENTRY;

typedef
struct _SYSTEM_HANDLE_INFORMATION
{
	ULONG			Count;
	SYSTEM_HANDLE_ENTRY	Handle [1];
	
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

// SystemProcessorPerformanceInformation (8)
typedef
struct _SYSTEM_PROCESSORTIME_INFO
{
	LARGE_INTEGER	IdleTime;
	LARGE_INTEGER	KernelTime;
	LARGE_INTEGER	UserTime;
	LARGE_INTEGER	DpcTime;
	LARGE_INTEGER	InterruptTime;
	ULONG			InterruptCount;
	ULONG			Unused;
	
} SYSTEM_PROCESSORTIME_INFO, *PSYSTEM_PROCESSORTIME_INFO;

#define SystemBasicInformation			0
#define SystemPerformanceInformation	2
#define SystemTimeInformation			3
#define	SystemProcessInformation		5
#define SystemProcessorTimeInformation	8
#define SystemHandleInformation			16
#define SystemPageFileInformation		18
#define SystemCacheInformation			21

#define Li2Double(x) ((double)((x).HighPart) * 4.294967296E9 + (double)((x).LowPart))

#define BELOW_NORMAL_PRIORITY_CLASS 0x00004000
#define ABOVE_NORMAL_PRIORITY_CLASS 0x00008000

#define GR_GDIOBJECTS     0       /* Count of GDI objects */
#define GR_USEROBJECTS    1       /* Count of USER objects */

// ntdll!NtQuerySystemInformation (NT specific!) 
// 
// The function copies the system information of the 
// specified type into a buffer 
// 
// NTSYSAPI 
// NTSTATUS 
// NTAPI 
// NtQuerySystemInformation( 
// IN UINT SystemInformationClass, // information type 
// OUT PVOID SystemInformation, // pointer to buffer 
// IN ULONG SystemInformationLength, // buffer size in bytes 
// OUT PULONG ReturnLength OPTIONAL // pointer to a 32-bit 
// // variable that receives 
// // the number of bytes 
// // written to the buffer 
// ); 
typedef LONG (WINAPI *PROCNTQSI)(UINT,PVOID,ULONG,PULONG); 

//DWORD GetGuiResources (
//  HANDLE hProcess,  // handle to process
//  DWORD uiFlags     // GUI object type
//);
typedef DWORD (WINAPI *PROCGGR)(HANDLE,DWORD);

//BOOL GetProcessIoCounters(
//  HANDLE hProcess,           // handle to process
//  PIO_COUNTERS lpIoCounters  // I/O accouting information
//);
typedef BOOL (WINAPI *PROCGPIC)(HANDLE,PIO_COUNTERS);

BOOL	PerfDataInitialize(void);
void	PerfDataUninitialize(void);
void	PerfDataRefresh(void);

ULONG	PerfDataGetProcessCount(void);
ULONG	PerfDataGetProcessorUsage(void);
ULONG	PerfDataGetProcessorSystemUsage(void);

BOOL	PerfDataGetImageName(ULONG Index, LPTSTR lpImageName, int nMaxCount);
ULONG	PerfDataGetProcessId(ULONG Index);
BOOL	PerfDataGetUserName(ULONG Index, LPTSTR lpUserName, int nMaxCount);
ULONG	PerfDataGetSessionId(ULONG Index);
ULONG	PerfDataGetCPUUsage(ULONG Index);
TIME	PerfDataGetCPUTime(ULONG Index);
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

#endif // __PERFDATA_H
