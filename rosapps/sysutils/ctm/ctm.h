/* Console Task Manager

   ctm.h - header file for main program

   Written by: Aleksey Bragin (aleksey@studiocerebral.com)
   
   Most of this file content is taken from
   ReactOS Task Manager written by Brian Palmet (brianp@reactos.org)
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef TMTM_H
#define TMTM_H

// keys
/*
#define VK_Q 0x51
#define VK_K 0x4B
#define VK_Y 0x59
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
*/

//typedef ULARGE_INTEGER	TIME, *PTIME;

/*

typedef struct _UNICODE_STRING {
    USHORT	Length;
    USHORT	MaximumLength;
    PWSTR	Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

*/

#define SystemBasicInformation			0
#define SystemPerformanceInformation	2
#define SystemTimeInformation			3
#define	SystemProcessInformation		5
#define SystemProcessorTimeInformation	8
#define SystemHandleInformation			16
#define SystemPageFileInformation		18
#define SystemCacheInformation			21

#define Li2Double(x) ((double)((x).HighPart) * 4.294967296E9 + (double)((x).LowPart))

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
	//IO_COUNTERS			IOCounters;

	TIME				UserTime;
	TIME				KernelTime;
} PERFDATA, *PPERFDATA;

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
	//SYSTEM_THREAD_INFORMATION	ThreadSysInfo [1];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

/*
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
*/


// SystemPerformanceInfo (2)
typedef struct SYSTEM_PERFORMANCE_INFORMATION
{
	LARGE_INTEGER	liIdleTime;  //TotalProcessorTime
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

#endif
