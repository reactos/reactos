/* $Id: zwtypes.h,v 1.1.2.2 2004/10/25 21:52:24 weiden Exp $
 *
 *  ReactOS Headers
 *  Copyright (C) 1998-2004 ReactOS Team
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
/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/zwtypes.h
 * PURPOSE:         Defintions for Native Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _ZWTYPES_H
#define _ZWTYPES_H

/*FIXME: REOREGANIZE!! */

typedef unsigned short LANGID;
typedef LANGID *PLANGID;
typedef unsigned short RTL_ATOM;
typedef unsigned short *PRTL_ATOM;

#ifndef _WINDOWS_H
typedef struct _LDT_ENTRY {
	WORD LimitLow;
	WORD BaseLow;
	union {
		struct {
			BYTE BaseMid;
			BYTE Flags1;
			BYTE Flags2;
			BYTE BaseHi;
		} Bytes;
		struct {
			DWORD BaseMid:8;
			DWORD Type:5;
			DWORD Dpl:2;
			DWORD Pres:1;
			DWORD LimitHi:4;
			DWORD Sys:1;
			DWORD Reserved_0:1;
			DWORD Default_Big:1;
			DWORD Granularity:1;
			DWORD BaseHi:8;
		} Bits;
	} HighWord;
} LDT_ENTRY,*PLDT_ENTRY,*LPLDT_ENTRY;
#endif

typedef enum _THREAD_STATE {
	StateInitialized,
	StateReady,
	StateRunning,
	StateStandby,
	StateTerminated,
	StateWait,
	StateTransition,
	StateUnknown
} THREAD_STATE;

typedef enum _DEBUG_CONTROL_CODE {
	DebugGetTraceInformation = 1,
	DebugSetInternalBreakpoint,
	DebugSetSpecialCall,
	DebugClearSpecialCalls,
	DebugQuerySpecialCalls,
	DebugDbgBreakPoint,
	DebugDbgLoadSymbols
} DEBUG_CONTROL_CODE;

typedef enum _OBJECT_INFORMATION_CLASS {
	ObjectBasicInformation,
	ObjectNameInformation,
	ObjectTypeInformation,
	ObjectAllTypesInformation,
	ObjectHandleInformation
} OBJECT_INFORMATION_CLASS;

typedef struct _OBJECT_HANDLE_ATTRIBUTE_INFORMATION {
	BOOLEAN Inherit;
	BOOLEAN ProtectFromClose;
} OBJECT_HANDLE_ATTRIBUTE_INFORMATION, *POBJECT_HANDLE_ATTRIBUTE_INFORMATION;

// system information
// {Nt|Zw}{Query|Set}SystemInformation
// (GN means Gary Nebbet in "NT/W2K Native API Reference")

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemInformationClassMin		= 0,
	SystemBasicInformation			= 0,	/* Q */
	
	SystemProcessorInformation		= 1,	/* Q */
	
	SystemPerformanceInformation		= 2,	/* Q */
	
	SystemTimeOfDayInformation		= 3,	/* Q */
	
	SystemPathInformation			= 4,	/* Q (checked build only) */
	SystemNotImplemented1                   = 4,	/* Q (GN) */
	
	SystemProcessInformation		= 5,	/* Q */
	SystemProcessesAndThreadsInformation    = 5,	/* Q (GN) */
	
	SystemCallCountInfoInformation		= 6,	/* Q */
	SystemCallCounts			= 6,	/* Q (GN) */
	
	SystemDeviceInformation			= 7,	/* Q */
// It conflicts with symbol in ntoskrnl/io/resource.c
//	SystemConfigurationInformation		= 7,	/* Q (GN) */
	
	SystemProcessorPerformanceInformation	= 8,	/* Q */
	SystemProcessorTimes			= 8,	/* Q (GN) */
	
	SystemFlagsInformation			= 9,	/* QS */
	SystemGlobalFlag			= 9,	/* QS (GN) */
	
	SystemCallTimeInformation		= 10,
	SystemNotImplemented2			= 10,	/* (GN) */
	
	SystemModuleInformation			= 11,	/* Q */
	
	SystemLocksInformation			= 12,	/* Q */
	SystemLockInformation			= 12,	/* Q (GN) */
	
	SystemStackTraceInformation		= 13,
	SystemNotImplemented3			= 13,	/* Q (GN) */
	
	SystemPagedPoolInformation		= 14,
	SystemNotImplemented4			= 14,	/* Q (GN) */
	
	SystemNonPagedPoolInformation		= 15,
	SystemNotImplemented5			= 15,	/* Q (GN) */
	
	SystemHandleInformation			= 16,	/* Q */
	
	SystemObjectInformation			= 17,	/* Q */
	
	SystemPageFileInformation		= 18,	/* Q */
	SystemPagefileInformation		= 18,	/* Q (GN) */
	
	SystemVdmInstemulInformation		= 19,	/* Q */
	SystemInstructionEmulationCounts	= 19,	/* Q (GN) */
	
	SystemVdmBopInformation			= 20,
	SystemInvalidInfoClass1			= 20,	/* (GN) */
	
	SystemFileCacheInformation		= 21,	/* QS */
	SystemCacheInformation			= 21,	/* QS (GN) */
	
	SystemPoolTagInformation		= 22,	/* Q (checked build only) */
	
	SystemInterruptInformation		= 23,	/* Q */
	SystemProcessorStatistics		= 23,	/* Q (GN) */
	
	SystemDpcBehaviourInformation		= 24,	/* QS */
	SystemDpcInformation			= 24,	/* QS (GN) */
	
	SystemFullMemoryInformation		= 25,
	SystemNotImplemented6			= 25,	/* (GN) */
	
	SystemLoadImage				= 26,	/* S (callable) (GN) */
	
	SystemUnloadImage			= 27,	/* S (callable) (GN) */
	
	SystemTimeAdjustmentInformation		= 28,	/* QS */
	SystemTimeAdjustment			= 28,	/* QS (GN) */
	
	SystemSummaryMemoryInformation		= 29,
	SystemNotImplemented7			= 29,	/* (GN) */
	
	SystemNextEventIdInformation		= 30,
	SystemNotImplemented8			= 30,	/* (GN) */
	
	SystemEventIdsInformation		= 31,
	SystemNotImplemented9			= 31,	/* (GN) */
	
	SystemCrashDumpInformation		= 32,	/* Q */
	
	SystemExceptionInformation		= 33,	/* Q */
	
	SystemCrashDumpStateInformation		= 34,	/* Q */
	
	SystemKernelDebuggerInformation		= 35,	/* Q */
	
	SystemContextSwitchInformation		= 36,	/* Q */
	
	SystemRegistryQuotaInformation		= 37,	/* QS */
	
	SystemLoadAndCallImage			= 38,	/* S (GN) */
	
	SystemPrioritySeparation		= 39,	/* S */
	
	SystemPlugPlayBusInformation		= 40,
	SystemNotImplemented10			= 40,	/* Q (GN) */
	
	SystemDockInformation			= 41,
	SystemNotImplemented11			= 41,	/* Q (GN) */
	
	SystemPowerInfo				= 42,
	SystemInvalidInfoClass2			= 42,	/* (GN) */
	
	SystemProcessorSpeedInformation		= 43,
	SystemInvalidInfoClass3			= 43,	/* (GN) */
	
	SystemCurrentTimeZoneInformation	= 44,	/* QS */
	SystemTimeZoneInformation		= 44,	/* QS (GN) */
	
	SystemLookasideInformation		= 45,	/* Q */
	
	SystemSetTimeSlipEvent			= 46,	/* S (GN) */
	
	SystemCreateSession			= 47,	/* S (GN) */
	
	SystemDeleteSession			= 48,	/* S (GN) */
	
	SystemInvalidInfoClass4			= 49,	/* (GN) */
	
	SystemRangeStartInformation		= 50,	/* Q (GN) */
	
	SystemVerifierInformation		= 51,	/* QS (GN) */
	
	SystemAddVerifier			= 52,	/* S (GN) */
	
	SystemSessionProcessesInformation	= 53,	/* Q (GN) */
	SystemInformationClassMax

} SYSTEM_INFORMATION_CLASS;

// SystemBasicInformation (0)
// Modified by Andrew Greenwood (15th July 2003) to match Win 32 API headers
typedef
struct _SYSTEM_BASIC_INFORMATION
{
	ULONG		Unknown;
	ULONG		MaximumIncrement;
	ULONG		PhysicalPageSize;
	ULONG		NumberOfPhysicalPages;
	ULONG		LowestPhysicalPage;
	ULONG		HighestPhysicalPage;
	ULONG		AllocationGranularity;
	ULONG		LowestUserAddress;
	ULONG		HighestUserAddress;
	KAFFINITY	ActiveProcessors;
	CCHAR		NumberProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

// SystemProcessorInformation (1)
// Modified by Andrew Greenwood (15th July 2003) to match Win 32 API headers
typedef struct _SYSTEM_PROCESSOR_INFORMATION {
	USHORT  ProcessorArchitecture;
	USHORT  ProcessorLevel;
	USHORT  ProcessorRevision;
	USHORT  Unknown;
	ULONG  FeatureBits;
} SYSTEM_PROCESSOR_INFORMATION, *PSYSTEM_PROCESSOR_INFORMATION;

// SystemPerformanceInfo (2)
// Modified by Andrew Greenwood (15th July 2003) to match Win 32 API headers
typedef struct _SYSTEM_PERFORMANCE_INFORMATION {
	LARGE_INTEGER  IdleTime;
	LARGE_INTEGER  ReadTransferCount;
	LARGE_INTEGER  WriteTransferCount;
	LARGE_INTEGER  OtherTransferCount;
	ULONG  ReadOperationCount;
	ULONG  WriteOperationCount;
	ULONG  OtherOperationCount;
	ULONG  AvailablePages;
	ULONG  TotalCommittedPages;
	ULONG  TotalCommitLimit;
	ULONG  PeakCommitment;
	ULONG  PageFaults;
	ULONG  WriteCopyFaults;
	ULONG  TransitionFaults;
	ULONG  CacheTransitionFaults;
	ULONG  DemandZeroFaults;
	ULONG  PagesRead;
	ULONG  PageReadIos;
	ULONG	 CacheReads;
	ULONG	 CacheIos;
	ULONG  PagefilePagesWritten;
	ULONG  PagefilePageWriteIos;
	ULONG  MappedFilePagesWritten;
	ULONG  MappedFilePageWriteIos;
	ULONG  PagedPoolUsage;
	ULONG  NonPagedPoolUsage;
	ULONG  PagedPoolAllocs;
	ULONG  PagedPoolFrees;
	ULONG  NonPagedPoolAllocs;
	ULONG  NonPagedPoolFrees;
	ULONG  TotalFreeSystemPtes;
	ULONG  SystemCodePage;
	ULONG  TotalSystemDriverPages;
	ULONG  TotalSystemCodePages;
	ULONG  SmallNonPagedLookasideListAllocateHits;
	ULONG  SmallPagedLookasideListAllocateHits;
	ULONG  Reserved3;
	ULONG  MmSystemCachePage;
	ULONG  PagedPoolPage;
	ULONG  SystemDriverPage;
	ULONG  FastReadNoWait;
	ULONG  FastReadWait;
	ULONG  FastReadResourceMiss;
	ULONG  FastReadNotPossible;
	ULONG  FastMdlReadNoWait;
	ULONG  FastMdlReadWait;
	ULONG  FastMdlReadResourceMiss;
	ULONG  FastMdlReadNotPossible;
	ULONG  MapDataNoWait;
	ULONG  MapDataWait;
	ULONG  MapDataNoWaitMiss;
	ULONG  MapDataWaitMiss;
	ULONG  PinMappedDataCount;
	ULONG  PinReadNoWait;
	ULONG  PinReadWait;
	ULONG  PinReadNoWaitMiss;
	ULONG  PinReadWaitMiss;
	ULONG  CopyReadNoWait;
	ULONG  CopyReadWait;
	ULONG  CopyReadNoWaitMiss;
	ULONG  CopyReadWaitMiss;
	ULONG  MdlReadNoWait;
	ULONG  MdlReadWait;
	ULONG  MdlReadNoWaitMiss;
	ULONG  MdlReadWaitMiss;
	ULONG  ReadAheadIos;
	ULONG  LazyWriteIos;
	ULONG  LazyWritePages;
	ULONG  DataFlushes;
	ULONG  DataPages;
	ULONG  ContextSwitches;
	ULONG  FirstLevelTbFills;
	ULONG  SecondLevelTbFills;
	ULONG  SystemCalls;
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

// SystemModuleInformation (11)
typedef struct _SYSTEM_MODULE_INFORMATION_ENTRY {
	ULONG	 Unknown1;
	ULONG	 Unknown2;
	PVOID  Base;
	ULONG  Size;
	ULONG  Flags;
	USHORT  Index;
  /* Length of module name not including the path, this
     field contains valid value only for NTOSKRNL module */
	USHORT	NameLength;
	USHORT  LoadCount;
	USHORT  PathLength;
	CHAR  ImageName[256];
} SYSTEM_MODULE_INFORMATION_ENTRY, *PSYSTEM_MODULE_INFORMATION_ENTRY;

typedef struct _SYSTEM_MODULE_INFORMATION {
	ULONG  Count;
	SYSTEM_MODULE_INFORMATION_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

// SystemHandleInformation (16)
// (see ontypes.h)
typedef struct _SYSTEM_HANDLE_ENTRY {
	ULONG	OwnerPid;
	BYTE	ObjectType;
	BYTE	HandleFlags;
	USHORT	HandleValue;
	PVOID	ObjectPointer;
	ULONG	AccessMask;
} SYSTEM_HANDLE_ENTRY, *PSYSTEM_HANDLE_ENTRY;

typedef struct _SYSTEM_HANDLE_INFORMATION {
	ULONG  ProcessId;
	UCHAR  ObjectTypeNumber;
	UCHAR  Flags;
	USHORT  Handle;
	PVOID  Object;
	ACCESS_MASK  GrantedAccess;
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

// SystemObjectInformation (17)
typedef struct _SYSTEM_OBJECT_TYPE_INFORMATION {
	ULONG		NextEntryOffset;
	ULONG		ObjectCount;
	ULONG		HandleCount;
	ULONG		TypeNumber;
	ULONG		InvalidAttributes;
	GENERIC_MAPPING	GenericMapping;
	ACCESS_MASK	ValidAccessMask;
	POOL_TYPE	PoolType;
	UCHAR		Unknown;
	UNICODE_STRING	Name;
} SYSTEM_OBJECT_TYPE_INFORMATION, *PSYSTEM_OBJECT_TYPE_INFORMATION;

typedef struct _SYSTEM_OBJECT_INFORMATION {
	ULONG			NextEntryOffset;
	PVOID			Object;
	ULONG			CreatorProcessId;
	USHORT			Unknown;
	USHORT			Flags;
	ULONG			PointerCount;
	ULONG			HandleCount;
	ULONG			PagedPoolUsage;
	ULONG			NonPagedPoolUsage;
	ULONG			ExclusiveProcessId;
	PSECURITY_DESCRIPTOR	SecurityDescriptor;
	UNICODE_STRING		Name;
} SYSTEM_OBJECT_INFORMATION, *PSYSTEM_OBJECT_INFORMATION;

// SystemPageFileInformation (18)
typedef struct _SYSTEM_PAGEFILE_INFORMATION {
	ULONG		RelativeOffset;
	ULONG		CurrentSizePages;
	ULONG		TotalUsedPages;
	ULONG		PeakUsedPages;
	UNICODE_STRING	PagefileFileName;
} SYSTEM_PAGEFILE_INFORMATION, *PSYSTEM_PAGEFILE_INFORMATION;

// SystemDpcInformation (24)
typedef struct _SYSTEM_DPC_INFORMATION {
	ULONG	Unused;
	ULONG	KiMaximumDpcQueueDepth;
	ULONG	KiMinimumDpcRate;
	ULONG	KiAdjustDpcThreshold;
	ULONG	KiIdealDpcRate;
} SYSTEM_DPC_INFORMATION, *PSYSTEM_DPC_INFORMATION;

// SystemLoadImage (26)
typedef struct _SYSTEM_LOAD_IMAGE {
	UNICODE_STRING ModuleName;
	PVOID ModuleBase;
	PVOID SectionPointer;
	PVOID EntryPoint;
	PVOID ExportDirectory;
} SYSTEM_LOAD_IMAGE, *PSYSTEM_LOAD_IMAGE;

// SystemUnloadImage (27)
typedef struct _SYSTEM_UNLOAD_IMAGE {
	PVOID ModuleBase;
} SYSTEM_UNLOAD_IMAGE, *PSYSTEM_UNLOAD_IMAGE;

// SystemTimeAdjustmentInformation (28)
typedef struct _SYSTEM_QUERY_TIME_ADJUSTMENT {
	ULONG	TimeAdjustment;
	ULONG	MaximumIncrement;
	BOOLEAN	TimeSynchronization;
} SYSTEM_QUERY_TIME_ADJUSTMENT, *PSYSTEM_QUERY_TIME_ADJUSTMENT;

typedef struct _SYSTEM_SET_TIME_ADJUSTMENT {
	ULONG	TimeAdjustment;
	BOOLEAN	TimeSynchronization;
} SYSTEM_SET_TIME_ADJUSTMENT, *PSYSTEM_SET_TIME_ADJUSTMENT;

// atom information
typedef enum _ATOM_INFORMATION_CLASS
{
	AtomBasicInformation		= 0,
	AtomTableInformation		= 1,
} ATOM_INFORMATION_CLASS;

typedef struct _ATOM_BASIC_INFORMATION {
	USHORT UsageCount;
	USHORT Flags;
	USHORT NameLength;
	WCHAR Name[1];
} ATOM_BASIC_INFORMATION, *PATOM_BASIC_INFORMATION;

// SystemLoadAndCallImage(38)
typedef struct _SYSTEM_LOAD_AND_CALL_IMAGE {
	UNICODE_STRING ModuleName;
} SYSTEM_LOAD_AND_CALL_IMAGE, *PSYSTEM_LOAD_AND_CALL_IMAGE;

// SystemTimeZoneInformation (44)
typedef struct _SYSTEM_TIME_ZONE_INFORMATION {
	LONG	Bias;
	WCHAR	StandardName [32];
	LARGE_INTEGER	StandardDate;
	LONG	StandardBias;
	WCHAR	DaylightName [32];
	LARGE_INTEGER	DaylightDate;
	LONG	DaylightBias;
} SYSTEM_TIME_ZONE_INFORMATION, * PSYSTEM_TIME_ZONE_INFORMATION;

// SystemLookasideInformation (45)
typedef struct _SYSTEM_LOOKASIDE_INFORMATION {
	USHORT		Depth;
	USHORT		MaximumDepth;
	ULONG		TotalAllocates;
	ULONG		AllocatesMisses;
	ULONG		TotalFrees;
	ULONG		FreeMisses;
	POOL_TYPE	Type;
	ULONG		Tag;
	ULONG		Size;
} SYSTEM_LOOKASIDE_INFORMATION, * PSYSTEM_LOOKASIDE_INFORMATION;

// SystemSetTimeSlipEvent (46)
typedef struct _SYSTEM_SET_TIME_SLIP_EVENT {
	HANDLE	TimeSlipEvent; /* IN */
} SYSTEM_SET_TIME_SLIP_EVENT, * PSYSTEM_SET_TIME_SLIP_EVENT;

// SystemCreateSession (47)
// (available only on TSE/NT5+)
typedef struct _SYSTEM_CREATE_SESSION {
	ULONG	SessionId; /* OUT */
} SYSTEM_CREATE_SESSION, * PSYSTEM_CREATE_SESSION;

// SystemDeleteSession (48)
// (available only on TSE/NT5+)
typedef struct _SYSTEM_DELETE_SESSION {
	ULONG	SessionId; /* IN */
} SYSTEM_DELETE_SESSION, * PSYSTEM_DELETE_SESSION;

// SystemRangeStartInformation (50)
typedef struct _SYSTEM_RANGE_START_INFORMATION {
	PVOID	SystemRangeStart;
} SYSTEM_RANGE_START_INFORMATION, * PSYSTEM_RANGE_START_INFORMATION;

// SystemSessionProcessesInformation (53)
// (available only on TSE/NT5+)
typedef struct _SYSTEM_SESSION_PROCESSES_INFORMATION {
	ULONG	SessionId;
	ULONG	BufferSize;
	PVOID	Buffer; /* same format as in SystemProcessInformation */
} SYSTEM_SESSION_PROCESSES_INFORMATION, * PSYSTEM_SESSION_PROCESSES_INFORMATION;

typedef struct _SYSTEM_THREADS {
	LARGE_INTEGER  KernelTime;
	LARGE_INTEGER  UserTime;
	LARGE_INTEGER  CreateTime;
	ULONG  WaitTime;
	PVOID  StartAddress;
	CLIENT_ID  ClientId;
	KPRIORITY  Priority;
	KPRIORITY  BasePriority;
	ULONG  ContextSwitchCount;
	THREAD_STATE  State;
	KWAIT_REASON  WaitReason;
} SYSTEM_THREADS, *PSYSTEM_THREADS;

typedef struct _SYSTEM_PROCESSES {
	ULONG  NextEntryDelta;
	ULONG  ThreadCount;
	ULONG  Reserved1[6];
	LARGE_INTEGER  CreateTime;
	LARGE_INTEGER  UserTime;
	LARGE_INTEGER  KernelTime;
	UNICODE_STRING  ProcessName;
	KPRIORITY  BasePriority;
	ULONG  ProcessId;
	ULONG  InheritedFromProcessId;
	ULONG  HandleCount;
	ULONG  Reserved2[2];
	VM_COUNTERS  VmCounters;
	IO_COUNTERS  IoCounters;
	SYSTEM_THREADS  Threads[1];
} SYSTEM_PROCESSES, *PSYSTEM_PROCESSES;

// memory information
typedef enum _MEMORY_INFORMATION_CLASS {
	MemoryBasicInformation,
	MemoryWorkingSetList,
	MemorySectionName,
	MemoryBasicVlmInformation
} MEMORY_INFORMATION_CLASS;

// Information Class 1
typedef struct _MEMORY_WORKING_SET_LIST {
	ULONG NumberOfPages;
	ULONG WorkingSetList[1];
} MEMORY_WORKING_SET_LIST, *PMEMORY_WORKING_SET_LIST;

typedef struct {
	UNICODE_STRING SectionFileName;
	WCHAR          NameBuffer[ANYSIZE_ARRAY];
} MEMORY_SECTION_NAME, *PMEMORY_SECTION_NAME;

// thread information
// FIXME: incompatible with MS NT
typedef struct _THREAD_BASIC_INFORMATION {
	NTSTATUS  ExitStatus;
	PVOID     TebBaseAddress;	// PNT_TIB (GN)
	CLIENT_ID ClientId;
	KAFFINITY AffinityMask;
	KPRIORITY Priority;
	KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef enum _SECTION_INFORMATION_CLASS {
	SectionBasicInformation,
	SectionImageInformation,
} SECTION_INFORMATION_CLASS;

// shutdown action
typedef enum SHUTDOWN_ACTION_TAG {
	ShutdownNoReboot,
	ShutdownReboot,
	ShutdownPowerOff
} SHUTDOWN_ACTION;

#define DebugDbgLoadSymbols ((DEBUG_CONTROL_CODE)0xffffffff)

// event access mask
#define EVENT_READ_ACCESS			1
#define EVENT_WRITE_ACCESS			2

//process query / set information class

#define ProcessBasicInformation			0
#define ProcessQuotaLimits			1
#define ProcessIoCounters			2
#define ProcessVmCounters			3
#define ProcessTimes				4
#define ProcessBasePriority			5
#define ProcessRaisePriority			6
#define ProcessDebugPort			7
#define ProcessExceptionPort			8
#define ProcessAccessToken			9
#define ProcessLdtInformation			10
#define ProcessLdtSize				11
#define ProcessDefaultHardErrorMode		12
#define ProcessIoPortHandlers			13
#define ProcessPooledUsageAndLimits		14
#define ProcessWorkingSetWatch			15
#define ProcessUserModeIOPL			16
#define ProcessEnableAlignmentFaultFixup	17
#define ProcessPriorityClass			18
#define ProcessWx86Information			19
#define ProcessHandleCount			20
#define ProcessAffinityMask			21
#define ProcessPriorityBoost			22
#define ProcessDeviceMap			23
#define ProcessSessionInformation		24
#define ProcessForegroundInformation		25
#define ProcessWow64Information			26
/* ReactOS private. */
#define ProcessImageFileName			27
#define ProcessDesktop                          28
#define MaxProcessInfoClass			29

/*
 * thread query / set information class
 */
#define ThreadBasicInformation			0
#define ThreadTimes				1
#define ThreadPriority				2
#define ThreadBasePriority			3
#define ThreadAffinityMask			4
#define ThreadImpersonationToken		5
#define ThreadDescriptorTableEntry		6
#define ThreadEnableAlignmentFaultFixup		7
#define ThreadEventPair				8
#define ThreadQuerySetWin32StartAddress		9
#define ThreadZeroTlsCell			10
#define ThreadPerformanceCount			11
#define ThreadAmILastThread			12
#define ThreadIdealProcessor			13
#define ThreadPriorityBoost			14
#define ThreadSetTlsArrayAddress		15
#define ThreadIsIoPending			16
#define ThreadHideFromDebugger			17
#define MaxThreadInfoClass			17

typedef struct _ATOM_TABLE_INFORMATION {
	ULONG NumberOfAtoms;
	RTL_ATOM Atoms[1];
} ATOM_TABLE_INFORMATION, *PATOM_TABLE_INFORMATION;

// mutant information
typedef enum _MUTANT_INFORMATION_CLASS {
	MutantBasicInformation = 0
} MUTANT_INFORMATION_CLASS;

typedef struct _MUTANT_BASIC_INFORMATION {
	LONG Count;
	BOOLEAN Owned;
	BOOLEAN Abandoned;
} MUTANT_BASIC_INFORMATION, *PMUTANT_BASIC_INFORMATION;

// SystemTimeOfDayInformation (3)
typedef struct _SYSTEM_TIMEOFDAY_INFORMATION {
	LARGE_INTEGER	BootTime;
	LARGE_INTEGER	CurrentTime;
	LARGE_INTEGER	TimeZoneBias;
	ULONG		TimeZoneId;
	ULONG		Reserved;
} SYSTEM_TIMEOFDAY_INFORMATION, *PSYSTEM_TIMEOFDAY_INFORMATION;

// SystemPathInformation (4)
// IT DOES NOT WORK
typedef struct _SYSTEM_PATH_INFORMATION {
	PVOID	Dummy;
} SYSTEM_PATH_INFORMATION, * PSYSTEM_PATH_INFORMATION;

// SystemCallCountInformation (6)
typedef struct _SYSTEM_SDT_INFORMATION {
	ULONG	BufferLength;
	ULONG	NumberOfSystemServiceTables;
	ULONG	NumberOfServices [1];
	ULONG	ServiceCounters [1];
} SYSTEM_SDT_INFORMATION, *PSYSTEM_SDT_INFORMATION;

// SystemDeviceInformation (7)
typedef struct _SYSTEM_DEVICE_INFORMATION {
	ULONG	NumberOfDisks;
	ULONG	NumberOfFloppies;
	ULONG	NumberOfCdRoms;
	ULONG	NumberOfTapes;
	ULONG	NumberOfSerialPorts;
	ULONG	NumberOfParallelPorts;
} SYSTEM_DEVICE_INFORMATION, *PSYSTEM_DEVICE_INFORMATION;

// SystemProcessorPerformanceInformation (8)
// (one per processor in the system)
typedef struct _SYSTEM_PROCESSORTIME_INFO {
	LARGE_INTEGER	TotalProcessorRunTime;
	LARGE_INTEGER	TotalProcessorTime;
	LARGE_INTEGER	TotalProcessorUserTime;
	LARGE_INTEGER	TotalDPCTime;
	LARGE_INTEGER	TotalInterruptTime;
	ULONG		TotalInterrupts;
	ULONG		Unused;
} SYSTEM_PROCESSORTIME_INFO, *PSYSTEM_PROCESSORTIME_INFO;

// SystemFlagsInformation (9)
typedef struct _SYSTEM_FLAGS_INFORMATION {
	ULONG	Flags;
} SYSTEM_FLAGS_INFORMATION, * PSYSTEM_FLAGS_INFORMATION;

#define FLG_STOP_ON_EXCEPTION		0x00000001
#define FLG_SHOW_LDR_SNAPS		0x00000002
#define FLG_DEBUG_INITIAL_COMMAND	0x00000004
#define FLG_STOP_ON_HANG_GUI		0x00000008
#define FLG_HEAP_ENABLE_TAIL_CHECK	0x00000010
#define FLG_HEAP_ENABLE_FREE_CHECK	0x00000020
#define FLG_HEAP_VALIDATE_PARAMETERS	0x00000040
#define FLG_HEAP_VALIDATE_ALL		0x00000080
#define FLG_POOL_ENABLE_TAIL_CHECK	0x00000100
#define FLG_POOL_ENABLE_FREE_CHECK	0x00000200
#define FLG_POOL_ENABLE_TAGGING		0x00000400
#define FLG_HEAP_ENABLE_TAGGING		0x00000800
#define FLG_USER_STACK_TRACE_DB		0x00001000
#define FLG_KERNEL_STACK_TRACE_DB	0x00002000
#define FLG_MAINTAIN_OBJECT_TYPELIST	0x00004000
#define FLG_HEAP_ENABLE_TAG_BY_DLL	0x00008000
#define FLG_IGNORE_DEBUG_PRIV		0x00010000
#define FLG_ENABLE_CSRDEBUG		0x00020000
#define FLG_ENABLE_KDEBUG_SYMBOL_LOAD	0x00040000
#define FLG_DISABLE_PAGE_KERNEL_STACKS	0x00080000
#define FLG_HEAP_ENABLE_CALL_TRACING	0x00100000
#define FLG_HEAP_DISABLE_COALESCING	0x00200000
#define FLG_ENABLE_CLOSE_EXCEPTION	0x00400000
#define FLG_ENABLE_EXCEPTION_LOGGING	0x00800000
#define FLG_UNKNOWN_01000000		0x01000000
#define FLG_UNKNOWN_02000000		0x02000000
#define FLG_UNKNOWN_04000000		0x04000000
#define FLG_ENABLE_DBGPRINT_BUFFERING	0x08000000
#define FLG_UNKNOWN_10000000		0x10000000
#define FLG_UNKNOWN_20000000		0x20000000
#define FLG_UNKNOWN_40000000		0x40000000
#define FLG_UNKNOWN_80000000		0x80000000

// SystemCallTimeInformation (10)
// UNKNOWN

// SystemLocksInformation (12)
typedef struct _SYSTEM_RESOURCE_LOCK_ENTRY {
	ULONG	ResourceAddress;
	ULONG	Always1;
	ULONG	Unknown;
	ULONG	ActiveCount;
	ULONG	ContentionCount;
	ULONG	Unused[2];
	ULONG	NumberOfSharedWaiters;
	ULONG	NumberOfExclusiveWaiters;
} SYSTEM_RESOURCE_LOCK_ENTRY, *PSYSTEM_RESOURCE_LOCK_ENTRY;

typedef struct _SYSTEM_RESOURCE_LOCK_INFO {
	ULONG				Count;
	SYSTEM_RESOURCE_LOCK_ENTRY	Lock [1];
} SYSTEM_RESOURCE_LOCK_INFO, *PSYSTEM_RESOURCE_LOCK_INFO;

// SystemInformation13 (13)
// UNKNOWN

// SystemInformation14 (14)
// UNKNOWN

// SystemInformation15 (15)
// UNKNOWN

// SystemInstructionEmulationInfo (19)
typedef struct _SYSTEM_VDM_INFORMATION {
	ULONG VdmSegmentNotPresentCount;
	ULONG VdmINSWCount;
	ULONG VdmESPREFIXCount;
	ULONG VdmCSPREFIXCount;
	ULONG VdmSSPREFIXCount;
	ULONG VdmDSPREFIXCount;
	ULONG VdmFSPREFIXCount;
	ULONG VdmGSPREFIXCount;
	ULONG VdmOPER32PREFIXCount;
	ULONG VdmADDR32PREFIXCount;
	ULONG VdmINSBCount;
	ULONG VdmINSWV86Count;
	ULONG VdmOUTSBCount;
	ULONG VdmOUTSWCount;
	ULONG VdmPUSHFCount;
	ULONG VdmPOPFCount;
	ULONG VdmINTNNCount;
	ULONG VdmINTOCount;
	ULONG VdmIRETCount;
	ULONG VdmINBIMMCount;
	ULONG VdmINWIMMCount;
	ULONG VdmOUTBIMMCount;
	ULONG VdmOUTWIMMCount;
	ULONG VdmINBCount;
	ULONG VdmINWCount;
	ULONG VdmOUTBCount;
	ULONG VdmOUTWCount;
	ULONG VdmLOCKPREFIXCount;
	ULONG VdmREPNEPREFIXCount;
	ULONG VdmREPPREFIXCount;
	ULONG VdmHLTCount;
	ULONG VdmCLICount;
	ULONG VdmSTICount;
	ULONG VdmBopCount;
} SYSTEM_VDM_INFORMATION, *PSYSTEM_VDM_INFORMATION;

// SystemInformation20 (20)
// UNKNOWN

// SystemPoolTagInformation (22)
// found by Klaus P. Gerlicher
// (implemented only in checked builds)
typedef struct _POOL_TAG_STATS {
	ULONG AllocationCount;
	ULONG FreeCount;
	ULONG SizeBytes;
} POOL_TAG_STATS;

typedef struct _SYSTEM_POOL_TAG_ENTRY {
	ULONG		Tag;
	POOL_TAG_STATS	Paged;
	POOL_TAG_STATS	NonPaged;
} SYSTEM_POOL_TAG_ENTRY, * PSYSTEM_POOL_TAG_ENTRY;

typedef struct _SYSTEM_POOL_TAG_INFO {
	ULONG			Count;
	SYSTEM_POOL_TAG_ENTRY	PoolEntry [1];
} SYSTEM_POOL_TAG_INFO, *PSYSTEM_POOL_TAG_INFO;

// SystemProcessorScheduleInfo (23)
typedef struct _SYSTEM_PROCESSOR_SCHEDULE_INFO {
	ULONG nContextSwitches;
	ULONG nDPCQueued;
	ULONG nDPCRate;
	ULONG TimerResolution;
	ULONG nDPCBypasses;
	ULONG nAPCBypasses;
} SYSTEM_PROCESSOR_SCHEDULE_INFO, *PSYSTEM_PROCESSOR_SCHEDULE_INFO;

// SystemInformation25 (25)
// UNKNOWN

// SystemProcessorFaultCountInfo (33)
typedef struct _SYSTEM_PROCESSOR_FAULT_INFO {
	ULONG	nAlignmentFixup;
	ULONG	nExceptionDispatches;
	ULONG	nFloatingEmulation;
	ULONG	Unknown;
} SYSTEM_PROCESSOR_FAULT_INFO, *PSYSTEM_PROCESSOR_FAULT_INFO;

// SystemCrashDumpStateInfo (34)
//

// SystemDebuggerInformation (35)
typedef struct _SYSTEM_DEBUGGER_INFO {
	BOOLEAN	KdDebuggerEnabled;
	BOOLEAN	KdDebuggerPresent;
} SYSTEM_DEBUGGER_INFO, *PSYSTEM_DEBUGGER_INFO;

// SystemInformation36 (36)
// UNKNOWN

// SystemQuotaInformation (37)
typedef struct _SYSTEM_QUOTA_INFORMATION {
	ULONG	CmpGlobalQuota;
	ULONG	CmpGlobalQuotaUsed;
	ULONG	MmSizeofPagedPoolInBytes;
} SYSTEM_QUOTA_INFORMATION, *PSYSTEM_QUOTA_INFORMATION;


// (49)
// UNKNOWN

// SystemVerifierInformation (51)
// UNKNOWN

// SystemAddVerifier (52)
// UNKNOWN

// wait type

#define WaitAll					0
#define WaitAny					1

// number of wait objects

#define THREAD_WAIT_OBJECTS			3
//#define MAXIMUM_WAIT_OBJECTS			64

// object type access rights

#define OBJECT_TYPE_CREATE		0x0001
#define OBJECT_TYPE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)

#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

// symbolic link access rights

#define SYMBOLIC_LINK_QUERY			0x0001
#define SYMBOLIC_LINK_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)


// File System Control commands ( related to defragging )

#define	FSCTL_READ_MFT_RECORD			0x90068 // NTFS only

typedef struct _BITMAP_DESCRIPTOR {
	ULONGLONG	StartLcn;
	ULONGLONG	ClustersToEndOfVol;
	BYTE		Map[0]; // variable size
} BITMAP_DESCRIPTOR, *PBITMAP_DESCRIPTOR;

typedef struct _TIMER_BASIC_INFORMATION {
	LARGE_INTEGER TimeRemaining;
	BOOLEAN SignalState;
} TIMER_BASIC_INFORMATION, *PTIMER_BASIC_INFORMATION;

typedef enum _TIMER_INFORMATION_CLASS {
	TimerBasicInformation
} TIMER_INFORMATION_CLASS;

// semaphore information
typedef enum _SEMAPHORE_INFORMATION_CLASS {
	SemaphoreBasicInformation		= 0
} SEMAPHORE_INFORMATION_CLASS;

typedef struct _SEMAPHORE_BASIC_INFORMATION {
	LONG CurrentCount;
	LONG MaximumCount;
} SEMAPHORE_BASIC_INFORMATION, *PSEMAPHORE_BASIC_INFORMATION;

// event information
typedef enum _EVENT_INFORMATION_CLASS {
	EventBasicInformation			= 0
} EVENT_INFORMATION_CLASS;

typedef struct _EVENT_BASIC_INFORMATION {
	EVENT_TYPE EventType;
	LONG EventState;
} EVENT_BASIC_INFORMATION, *PEVENT_BASIC_INFORMATION;

typedef enum _IO_COMPLETION_INFORMATION_CLASS {
	IoCompletionBasicInformation
} IO_COMPLETION_INFORMATION_CLASS;

// wmi trace event data
typedef struct _EVENT_TRACE_HEADER {
	USHORT           Size;
	union {
		USHORT FieldTypeFlags;
		struct {
			UCHAR HeaderType;
			UCHAR            MarkerFlags;
			};
		};
	union {
		ULONG         Version;
		struct {
			UCHAR     Type;
			UCHAR     Level;
			USHORT    Version;
		} Class;
	};
	ULONG ThreadId;
	ULONG ProcessId;
	LARGE_INTEGER    TimeStamp;
	union {
		GUID      Guid;
		ULONGLONG GuidPtr;
	};
	union {
		struct {
			ULONG ClientContext;
			ULONG Flags;
		};
		struct {
			ULONG KernelTime;
			ULONG UserTime;
		};
		ULONG64 ProcessorTime;
	};
} EVENT_TRACE_HEADER, *PEVENT_TRACE_HEADER;

typedef struct _FILE_USER_QUOTA_INFORMATION {
	ULONG NextEntryOffset;
	ULONG SidLength;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER QuotaUsed;
	LARGE_INTEGER QuotaThreshold;
	LARGE_INTEGER QuotaLimit;
	SID Sid[1];
} FILE_USER_QUOTA_INFORMATION, *PFILE_USER_QUOTA_INFORMATION;

typedef struct _INITIAL_TEB {
	PVOID FixedStackBase;
	PVOID FixedStackLimit;
	PVOID ExpandableStackBase;
	PVOID ExpandableStackLimit;
	PVOID ExpandableStackBottom;
} INITIAL_TEB, *PINITIAL_TEB; 

#endif
