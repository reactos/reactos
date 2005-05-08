/*
 * ntapi.h
 *
 * Windows NT Native API
 *
 * Most structures in this file is obtained from Windows NT/2000 Native API
 * Reference by Gary Nebbett, ISBN 1578701996.
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __NTAPI_H
#define __NTAPI_H

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push,4)

#include <stdarg.h>
#include <winbase.h>
#include "ntddk.h"
#include "ntpoapi.h"

typedef struct _PEB *PPEB;

/* FIXME: Unknown definitions */
typedef PVOID POBJECT_TYPE_LIST;
typedef PVOID PEXECUTION_STATE;
typedef PVOID PLANGID;


/* System information and control */

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemInformationClassMin = 0,
	SystemBasicInformation = 0,
	SystemProcessorInformation = 1,
	SystemPerformanceInformation = 2,
	SystemTimeOfDayInformation = 3,
	SystemPathInformation = 4,
	SystemNotImplemented1 = 4,
	SystemProcessInformation = 5,
	SystemProcessesAndThreadsInformation = 5,
	SystemCallCountInfoInformation = 6,
	SystemCallCounts = 6,
	SystemDeviceInformation = 7,
	SystemConfigurationInformation = 7,
	SystemProcessorPerformanceInformation = 8,
	SystemProcessorTimes = 8,
	SystemFlagsInformation = 9,
	SystemGlobalFlag = 9,
	SystemCallTimeInformation = 10,
	SystemNotImplemented2 = 10,
	SystemModuleInformation = 11,
	SystemLocksInformation = 12,
	SystemLockInformation = 12,
	SystemStackTraceInformation = 13,
	SystemNotImplemented3 = 13,
	SystemPagedPoolInformation = 14,
	SystemNotImplemented4 = 14,
	SystemNonPagedPoolInformation = 15,
	SystemNotImplemented5 = 15,
	SystemHandleInformation = 16,
	SystemObjectInformation = 17,
	SystemPageFileInformation = 18,
	SystemPagefileInformation = 18,
	SystemVdmInstemulInformation = 19,
	SystemInstructionEmulationCounts = 19,
	SystemVdmBopInformation = 20,
	SystemInvalidInfoClass1 = 20,	
	SystemFileCacheInformation = 21,
	SystemCacheInformation = 21,
	SystemPoolTagInformation = 22,
	SystemInterruptInformation = 23,
	SystemProcessorStatistics = 23,
	SystemDpcBehaviourInformation = 24,
	SystemDpcInformation = 24,
	SystemFullMemoryInformation = 25,
	SystemNotImplemented6 = 25,
	SystemLoadImage = 26,
	SystemUnloadImage = 27,
	SystemTimeAdjustmentInformation = 28,
	SystemTimeAdjustment = 28,
	SystemSummaryMemoryInformation = 29,
	SystemNotImplemented7 = 29,
	SystemNextEventIdInformation = 30,
	SystemNotImplemented8 = 30,
	SystemEventIdsInformation = 31,
	SystemNotImplemented9 = 31,
	SystemCrashDumpInformation = 32,
	SystemExceptionInformation = 33,
	SystemCrashDumpStateInformation = 34,
	SystemKernelDebuggerInformation = 35,
	SystemContextSwitchInformation = 36,
	SystemRegistryQuotaInformation = 37,
	SystemLoadAndCallImage = 38,
	SystemPrioritySeparation = 39,
	SystemPlugPlayBusInformation = 40,
	SystemNotImplemented10 = 40,
	SystemDockInformation = 41,
	SystemNotImplemented11 = 41,
	/* SystemPowerInformation = 42, Conflicts with POWER_INFORMATION_LEVEL 1 */
	SystemInvalidInfoClass2 = 42,
	SystemProcessorSpeedInformation = 43,
	SystemInvalidInfoClass3 = 43,
	SystemCurrentTimeZoneInformation = 44,
	SystemTimeZoneInformation = 44,
	SystemLookasideInformation = 45,
	SystemSetTimeSlipEvent = 46,
	SystemCreateSession = 47,
	SystemDeleteSession = 48,
	SystemInvalidInfoClass4 = 49,
	SystemRangeStartInformation = 50,
	SystemVerifierInformation = 51,
	SystemAddVerifier = 52,
	SystemSessionProcessesInformation	= 53,
	SystemInformationClassMax
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_BASIC_INFORMATION {
	ULONG  Unknown;
	ULONG  MaximumIncrement;
	ULONG  PhysicalPageSize;
	ULONG  NumberOfPhysicalPages;
	ULONG  LowestPhysicalPage;
	ULONG  HighestPhysicalPage;
	ULONG  AllocationGranularity;
	ULONG  LowestUserAddress;
	ULONG  HighestUserAddress;
	ULONG  ActiveProcessors;
	UCHAR  NumberProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_INFORMATION {
	USHORT  ProcessorArchitecture;
	USHORT  ProcessorLevel;
	USHORT  ProcessorRevision;
	USHORT  Unknown;
	ULONG  FeatureBits;
} SYSTEM_PROCESSOR_INFORMATION, *PSYSTEM_PROCESSOR_INFORMATION;

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

typedef struct _SYSTEM_TIME_OF_DAY_INFORMATION {
	LARGE_INTEGER  BootTime;
	LARGE_INTEGER  CurrentTime;
	LARGE_INTEGER  TimeZoneBias;
	ULONG  CurrentTimeZoneId;
} SYSTEM_TIME_OF_DAY_INFORMATION, *PSYSTEM_TIME_OF_DAY_INFORMATION;

typedef struct _VM_COUNTERS {
	ULONG  PeakVirtualSize;
	ULONG  VirtualSize;
	ULONG  PageFaultCount;
	ULONG  PeakWorkingSetSize;
	ULONG  WorkingSetSize;
	ULONG  QuotaPeakPagedPoolUsage;
	ULONG  QuotaPagedPoolUsage;
	ULONG  QuotaPeakNonPagedPoolUsage;
	ULONG  QuotaNonPagedPoolUsage;
	ULONG  PagefileUsage;
	ULONG  PeakPagefileUsage;
} VM_COUNTERS;

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

typedef struct _SYSTEM_CALLS_INFORMATION {
	ULONG  Size;
	ULONG  NumberOfDescriptorTables;
	ULONG  NumberOfRoutinesInTable[1];
	ULONG  CallCounts[ANYSIZE_ARRAY];
} SYSTEM_CALLS_INFORMATION, *PSYSTEM_CALLS_INFORMATION;

typedef struct _SYSTEM_CONFIGURATION_INFORMATION {
	ULONG  DiskCount;
	ULONG  FloppyCount;
	ULONG  CdRomCount;
	ULONG  TapeCount;
	ULONG  SerialCount;
	ULONG  ParallelCount;
} SYSTEM_CONFIGURATION_INFORMATION, *PSYSTEM_CONFIGURATION_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_TIMES {
	LARGE_INTEGER  IdleTime;
	LARGE_INTEGER  KernelTime;
	LARGE_INTEGER  UserTime;
	LARGE_INTEGER  DpcTime;
	LARGE_INTEGER  InterruptTime;
	ULONG  InterruptCount;
} SYSTEM_PROCESSOR_TIMES, *PSYSTEM_PROCESSOR_TIMES;

/* SYSTEM_GLOBAL_FLAG.GlobalFlag constants */
#define FLG_STOP_ON_EXCEPTION             0x00000001
#define FLG_SHOW_LDR_SNAPS                0x00000002
#define FLG_DEBUG_INITIAL_COMMAND         0x00000004
#define FLG_STOP_ON_HUNG_GUI              0x00000008
#define FLG_HEAP_ENABLE_TAIL_CHECK        0x00000010
#define FLG_HEAP_ENABLE_FREE_CHECK        0x00000020
#define FLG_HEAP_VALIDATE_PARAMETERS      0x00000040
#define FLG_HEAP_VALIDATE_ALL             0x00000080
#define FLG_POOL_ENABLE_TAIL_CHECK        0x00000100
#define FLG_POOL_ENABLE_FREE_CHECK        0x00000200
#define FLG_POOL_ENABLE_TAGGING           0x00000400
#define FLG_HEAP_ENABLE_TAGGING           0x00000800
#define FLG_USER_STACK_TRACE_DB           0x00001000
#define FLG_KERNEL_STACK_TRACE_DB         0x00002000
#define FLG_MAINTAIN_OBJECT_TYPELIST      0x00004000
#define FLG_HEAP_ENABLE_TAG_BY_DLL        0x00008000
#define FLG_IGNORE_DEBUG_PRIV             0x00010000
#define FLG_ENABLE_CSRDEBUG               0x00020000
#define FLG_ENABLE_KDEBUG_SYMBOL_LOAD     0x00040000
#define FLG_DISABLE_PAGE_KERNEL_STACKS    0x00080000
#define FLG_HEAP_ENABLE_CALL_TRACING      0x00100000
#define FLG_HEAP_DISABLE_COALESCING       0x00200000
#define FLG_ENABLE_CLOSE_EXCEPTIONS       0x00400000
#define FLG_ENABLE_EXCEPTION_LOGGING      0x00800000
#define FLG_ENABLE_DBGPRINT_BUFFERING     0x08000000

typedef struct _SYSTEM_GLOBAL_FLAG {
  ULONG  GlobalFlag;
} SYSTEM_GLOBAL_FLAG, *PSYSTEM_GLOBAL_FLAG;

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

typedef struct _SYSTEM_LOCK_INFORMATION {
	PVOID  Address;
	USHORT  Type;
	USHORT  Reserved1;
	ULONG  ExclusiveOwnerThreadId;
	ULONG  ActiveCount;
	ULONG  ContentionCount;
	ULONG  Reserved2[2];
	ULONG  NumberOfSharedWaiters;
	ULONG  NumberOfExclusiveWaiters;
} SYSTEM_LOCK_INFORMATION, *PSYSTEM_LOCK_INFORMATION;

/*SYSTEM_HANDLE_INFORMATION.Flags cosntants */
#define PROTECT_FROM_CLOSE                0x01
#define INHERIT                           0x02

typedef struct _SYSTEM_HANDLE_INFORMATION {
	ULONG  ProcessId;
	UCHAR  ObjectTypeNumber;
	UCHAR  Flags;
	USHORT  Handle;
	PVOID  Object;
	ACCESS_MASK  GrantedAccess;
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef struct _SYSTEM_OBJECT_TYPE_INFORMATION {
	ULONG  NextEntryOffset;
	ULONG  ObjectCount;
	ULONG  HandleCount;
	ULONG  TypeNumber;
	ULONG  InvalidAttributes;
	GENERIC_MAPPING  GenericMapping;
	ACCESS_MASK  ValidAccessMask;
	POOL_TYPE  PoolType;
	UCHAR  Unknown;
	UNICODE_STRING  Name;
} SYSTEM_OBJECT_TYPE_INFORMATION, *PSYSTEM_OBJECT_TYPE_INFORMATION;

/* SYSTEM_OBJECT_INFORMATION.Flags constants */
#define FLG_SYSOBJINFO_SINGLE_HANDLE_ENTRY    0x40
#define FLG_SYSOBJINFO_DEFAULT_SECURITY_QUOTA 0x20
#define FLG_SYSOBJINFO_PERMANENT              0x10
#define FLG_SYSOBJINFO_EXCLUSIVE              0x08
#define FLG_SYSOBJINFO_CREATOR_INFO           0x04
#define FLG_SYSOBJINFO_KERNEL_MODE            0x02

typedef struct _SYSTEM_OBJECT_INFORMATION {
	ULONG  NextEntryOffset;
	PVOID  Object;
	ULONG  CreatorProcessId;
	USHORT  Unknown;
	USHORT  Flags;
	ULONG  PointerCount;
	ULONG  HandleCount;
	ULONG  PagedPoolUsage;
	ULONG  NonPagedPoolUsage;
	ULONG  ExclusiveProcessId;
	PSECURITY_DESCRIPTOR  SecurityDescriptor;
	UNICODE_STRING  Name;
} SYSTEM_OBJECT_INFORMATION, *PSYSTEM_OBJECT_INFORMATION;

typedef struct _SYSTEM_PAGEFILE_INFORMATION {
	ULONG  NextEntryOffset;
	ULONG  CurrentSize;
	ULONG  TotalUsed;
	ULONG  PeakUsed;
	UNICODE_STRING  FileName;
} SYSTEM_PAGEFILE_INFORMATION, *PSYSTEM_PAGEFILE_INFORMATION;

typedef struct _SYSTEM_INSTRUCTION_EMULATION_INFORMATION {
	ULONG  SegmentNotPresent;
	ULONG  TwoByteOpcode;
	ULONG  ESprefix;
	ULONG  CSprefix;
	ULONG  SSprefix;
	ULONG  DSprefix;
	ULONG  FSPrefix;
	ULONG  GSprefix;
	ULONG  OPER32prefix;
	ULONG  ADDR32prefix;
	ULONG  INSB;
	ULONG  INSW;
	ULONG  OUTSB;
	ULONG  OUTSW;
	ULONG  PUSHFD;
	ULONG  POPFD;
	ULONG  INTnn;
	ULONG  INTO;
	ULONG  IRETD;
	ULONG  INBimm;
	ULONG  INWimm;
	ULONG  OUTBimm;
	ULONG  OUTWimm;
	ULONG  INB;
	ULONG  INW;
	ULONG  OUTB;
	ULONG  OUTW;
	ULONG  LOCKprefix;
	ULONG  REPNEprefix;
	ULONG  REPprefix;
	ULONG  HLT;
	ULONG  CLI;
	ULONG  STI;
	ULONG  GenericInvalidOpcode;
} SYSTEM_INSTRUCTION_EMULATION_INFORMATION, *PSYSTEM_INSTRUCTION_EMULATION_INFORMATION;

typedef struct _SYSTEM_POOL_TAG_INFORMATION {
	CHAR  Tag[4];
	ULONG  PagedPoolAllocs;
	ULONG  PagedPoolFrees;
	ULONG  PagedPoolUsage;
	ULONG  NonPagedPoolAllocs;
	ULONG  NonPagedPoolFrees;
	ULONG  NonPagedPoolUsage;
} SYSTEM_POOL_TAG_INFORMATION, *PSYSTEM_POOL_TAG_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_STATISTICS {
	ULONG  ContextSwitches;
	ULONG  DpcCount;
	ULONG  DpcRequestRate;
	ULONG  TimeIncrement;
	ULONG  DpcBypassCount;
	ULONG  ApcBypassCount;
} SYSTEM_PROCESSOR_STATISTICS, *PSYSTEM_PROCESSOR_STATISTICS;

typedef struct _SYSTEM_DPC_INFORMATION {
	ULONG  Reserved;
	ULONG  MaximumDpcQueueDepth;
	ULONG  MinimumDpcRate;
	ULONG  AdjustDpcThreshold;
	ULONG  IdealDpcRate;
} SYSTEM_DPC_INFORMATION, *PSYSTEM_DPC_INFORMATION;

typedef struct _SYSTEM_LOAD_IMAGE {
	UNICODE_STRING  ModuleName;
	PVOID  ModuleBase;
	PVOID  SectionPointer;
	PVOID  EntryPoint;
	PVOID  ExportDirectory;
} SYSTEM_LOAD_IMAGE, *PSYSTEM_LOAD_IMAGE;

typedef struct _SYSTEM_UNLOAD_IMAGE {
  PVOID  ModuleBase;
} SYSTEM_UNLOAD_IMAGE, *PSYSTEM_UNLOAD_IMAGE;

typedef struct _SYSTEM_QUERY_TIME_ADJUSTMENT {
	ULONG  TimeAdjustment;
	ULONG  MaximumIncrement;
	BOOLEAN  TimeSynchronization;
} SYSTEM_QUERY_TIME_ADJUSTMENT, *PSYSTEM_QUERY_TIME_ADJUSTMENT;

typedef struct _SYSTEM_SET_TIME_ADJUSTMENT {
	ULONG  TimeAdjustment;
	BOOLEAN  TimeSynchronization;
} SYSTEM_SET_TIME_ADJUSTMENT, *PSYSTEM_SET_TIME_ADJUSTMENT;

typedef struct _SYSTEM_CRASH_DUMP_INFORMATION {
	HANDLE  CrashDumpSectionHandle;
	HANDLE  Unknown;
} SYSTEM_CRASH_DUMP_INFORMATION, *PSYSTEM_CRASH_DUMP_INFORMATION;

typedef struct _SYSTEM_EXCEPTION_INFORMATION {
	ULONG  AlignmentFixupCount;
	ULONG  ExceptionDispatchCount;
	ULONG  FloatingEmulationCount;
	ULONG  Reserved;
} SYSTEM_EXCEPTION_INFORMATION, *PSYSTEM_EXCEPTION_INFORMATION;

typedef struct _SYSTEM_CRASH_DUMP_STATE_INFORMATION {
	ULONG  CrashDumpSectionExists;
	ULONG  Unknown;
} SYSTEM_CRASH_DUMP_STATE_INFORMATION, *PSYSTEM_CRASH_DUMP_STATE_INFORMATION;

typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION {
	BOOLEAN  DebuggerEnabled;
	BOOLEAN  DebuggerNotPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION, *PSYSTEM_KERNEL_DEBUGGER_INFORMATION;

typedef struct _SYSTEM_CONTEXT_SWITCH_INFORMATION {
	ULONG  ContextSwitches;
	ULONG  ContextSwitchCounters[11];
} SYSTEM_CONTEXT_SWITCH_INFORMATION, *PSYSTEM_CONTEXT_SWITCH_INFORMATION;

typedef struct _SYSTEM_REGISTRY_QUOTA_INFORMATION {
	ULONG  RegistryQuota;
	ULONG  RegistryQuotaInUse;
	ULONG  PagedPoolSize;
} SYSTEM_REGISTRY_QUOTA_INFORMATION, *PSYSTEM_REGISTRY_QUOTA_INFORMATION;

typedef struct _SYSTEM_LOAD_AND_CALL_IMAGE {
  UNICODE_STRING  ModuleName;
} SYSTEM_LOAD_AND_CALL_IMAGE, *PSYSTEM_LOAD_AND_CALL_IMAGE;

typedef struct _SYSTEM_PRIORITY_SEPARATION {
  ULONG  PrioritySeparation;
} SYSTEM_PRIORITY_SEPARATION, *PSYSTEM_PRIORITY_SEPARATION;

typedef struct _SYSTEM_TIME_ZONE_INFORMATION {
	LONG  Bias;
	WCHAR  StandardName[32];
	LARGE_INTEGER  StandardDate;
	LONG  StandardBias;
	WCHAR  DaylightName[32];
	LARGE_INTEGER  DaylightDate;
	LONG  DaylightBias;
} SYSTEM_TIME_ZONE_INFORMATION, *PSYSTEM_TIME_ZONE_INFORMATION;

typedef struct _SYSTEM_LOOKASIDE_INFORMATION {
	USHORT  Depth;
	USHORT  MaximumDepth;
	ULONG  TotalAllocates;
	ULONG  AllocateMisses;
	ULONG  TotalFrees;
	ULONG  FreeMisses;
	POOL_TYPE  Type;
	ULONG  Tag;
	ULONG  Size;
} SYSTEM_LOOKASIDE_INFORMATION, *PSYSTEM_LOOKASIDE_INFORMATION;

typedef struct _SYSTEM_SET_TIME_SLIP_EVENT {
  HANDLE  TimeSlipEvent;
} SYSTEM_SET_TIME_SLIP_EVENT, *PSYSTEM_SET_TIME_SLIP_EVENT;

typedef struct _SYSTEM_CREATE_SESSION {
  ULONG  SessionId;
} SYSTEM_CREATE_SESSION, *PSYSTEM_CREATE_SESSION;

typedef struct _SYSTEM_DELETE_SESSION {
  ULONG  SessionId;
} SYSTEM_DELETE_SESSION, *PSYSTEM_DELETE_SESSION;

typedef struct _SYSTEM_RANGE_START_INFORMATION {
  PVOID  SystemRangeStart;
} SYSTEM_RANGE_START_INFORMATION, *PSYSTEM_RANGE_START_INFORMATION;

typedef struct _SYSTEM_SESSION_PROCESSES_INFORMATION {
	ULONG  SessionId;
	ULONG  BufferSize;
	PVOID  Buffer;
} SYSTEM_SESSION_PROCESSES_INFORMATION, *PSYSTEM_SESSION_PROCESSES_INFORMATION;

typedef struct _SYSTEM_POOL_BLOCK {
	BOOLEAN  Allocated;
	USHORT  Unknown;
	ULONG  Size;
	CHAR  Tag[4];
} SYSTEM_POOL_BLOCK, *PSYSTEM_POOL_BLOCK;

typedef struct _SYSTEM_POOL_BLOCKS_INFORMATION {
	ULONG  PoolSize;
	PVOID  PoolBase;
	USHORT  Unknown;
	ULONG  NumberOfBlocks;
	SYSTEM_POOL_BLOCK  PoolBlocks[1];
} SYSTEM_POOL_BLOCKS_INFORMATION, *PSYSTEM_POOL_BLOCKS_INFORMATION;

typedef struct _SYSTEM_MEMORY_USAGE {
	PVOID  Name;
	USHORT  Valid;
	USHORT  Standby;
	USHORT  Modified;
	USHORT  PageTables;
} SYSTEM_MEMORY_USAGE, *PSYSTEM_MEMORY_USAGE;

typedef struct _SYSTEM_MEMORY_USAGE_INFORMATION {
  ULONG  Reserved;
	PVOID  EndOfData;
	SYSTEM_MEMORY_USAGE  MemoryUsage[1];
} SYSTEM_MEMORY_USAGE_INFORMATION, *PSYSTEM_MEMORY_USAGE_INFORMATION;

NTOSAPI
NTSTATUS
NTAPI
NtQuerySystemInformation(
  IN SYSTEM_INFORMATION_CLASS  SystemInformationClass,
  IN OUT PVOID  SystemInformation,
  IN ULONG  SystemInformationLength,
  OUT PULONG  ReturnLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
  IN SYSTEM_INFORMATION_CLASS  SystemInformationClass,
  IN OUT PVOID  SystemInformation,
  IN ULONG  SystemInformationLength,
  OUT PULONG  ReturnLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwSetSystemInformation(
	IN SYSTEM_INFORMATION_CLASS  SystemInformationClass,
	IN OUT PVOID  SystemInformation,
	IN ULONG  SystemInformationLength);

NTOSAPI
NTSTATUS
NTAPI
ZwQuerySystemEnvironmentValue(
	IN PUNICODE_STRING  Name,
	OUT PVOID  Value,
	IN ULONG  ValueLength,
	OUT PULONG  ReturnLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwSetSystemEnvironmentValue(
	IN PUNICODE_STRING  Name,
	IN PUNICODE_STRING  Value);

typedef enum _SHUTDOWN_ACTION {
	ShutdownNoReboot,
	ShutdownReboot,
	ShutdownPowerOff
} SHUTDOWN_ACTION;

NTOSAPI
NTSTATUS
NTAPI
NtShutdownSystem(
  IN SHUTDOWN_ACTION  Action);

typedef enum _DEBUG_CONTROL_CODE {
  DebugGetTraceInformation = 1,
	DebugSetInternalBreakpoint,
	DebugSetSpecialCall,
	DebugClearSpecialCalls,
	DebugQuerySpecialCalls,
	DebugDbgBreakPoint,
	DebugMaximum
} DEBUG_CONTROL_CODE;


NTOSAPI
NTSTATUS
NTAPI
ZwSystemDebugControl(
	IN DEBUG_CONTROL_CODE  ControlCode,
	IN PVOID  InputBuffer  OPTIONAL,
	IN ULONG  InputBufferLength,
	OUT PVOID  OutputBuffer  OPTIONAL,
	IN ULONG  OutputBufferLength,
	OUT PULONG  ReturnLength  OPTIONAL);



/* Objects, Object directories, and symbolic links */

typedef enum _OBJECT_INFORMATION_CLASS {
	ObjectBasicInformation,
	ObjectNameInformation,
	ObjectTypeInformation,
	ObjectAllTypesInformation,
	ObjectHandleInformation
} OBJECT_INFORMATION_CLASS;

NTOSAPI
NTSTATUS
NTAPI
ZwQueryObject(
	IN HANDLE  ObjectHandle,
	IN OBJECT_INFORMATION_CLASS  ObjectInformationClass,
	OUT PVOID  ObjectInformation,
	IN ULONG  ObjectInformationLength,
	OUT PULONG  ReturnLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwSetInformationObject(
	IN HANDLE  ObjectHandle,
	IN OBJECT_INFORMATION_CLASS  ObjectInformationClass,
	IN PVOID  ObjectInformation,
	IN ULONG  ObjectInformationLength);

/* OBJECT_BASIC_INFORMATION.Attributes constants */
/* also in winbase.h */
#define HANDLE_FLAG_INHERIT               0x01
#define HANDLE_FLAG_PROTECT_FROM_CLOSE    0x02
/* end winbase.h */
#define PERMANENT                         0x10
#define EXCLUSIVE                         0x20

typedef struct _OBJECT_BASIC_INFORMATION {
	ULONG  Attributes;
	ACCESS_MASK  GrantedAccess;
	ULONG  HandleCount;
	ULONG  PointerCount;
	ULONG  PagedPoolUsage;
	ULONG  NonPagedPoolUsage;
	ULONG  Reserved[3];
	ULONG  NameInformationLength;
	ULONG  TypeInformationLength;
	ULONG  SecurityDescriptorLength;
	LARGE_INTEGER  CreateTime;
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;
#if 0
/* FIXME: Enable later */
typedef struct _OBJECT_TYPE_INFORMATION {
	UNICODE_STRING  Name;
	ULONG  ObjectCount;
	ULONG  HandleCount;
	ULONG  Reserved1[4];
	ULONG  PeakObjectCount;
	ULONG  PeakHandleCount;
	ULONG  Reserved2[4];
	ULONG  InvalidAttributes;
	GENERIC_MAPPING  GenericMapping;
	ULONG  ValidAccess;
	UCHAR  Unknown;
	BOOLEAN  MaintainHandleDatabase;
	POOL_TYPE  PoolType;
	ULONG  PagedPoolUsage;
	ULONG  NonPagedPoolUsage;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

typedef struct _OBJECT_ALL_TYPES_INFORMATION {
  ULONG  NumberOfTypes;
  OBJECT_TYPE_INFORMATION  TypeInformation;
} OBJECT_ALL_TYPES_INFORMATION, *POBJECT_ALL_TYPES_INFORMATION;
#endif
typedef struct _OBJECT_HANDLE_ATTRIBUTE_INFORMATION {
  BOOLEAN  Inherit;
  BOOLEAN  ProtectFromClose;
} OBJECT_HANDLE_ATTRIBUTE_INFORMATION, *POBJECT_HANDLE_ATTRIBUTE_INFORMATION;

NTOSAPI
NTSTATUS
NTAPI
NtDuplicateObject(
  IN HANDLE  SourceProcessHandle,
  IN HANDLE  SourceHandle,
  IN HANDLE  TargetProcessHandle,
  OUT PHANDLE  TargetHandle  OPTIONAL,
  IN ACCESS_MASK  DesiredAccess,
  IN ULONG  Attributes,
  IN ULONG  Options);

NTOSAPI
NTSTATUS
NTAPI
ZwDuplicateObject(
  IN HANDLE  SourceProcessHandle,
  IN HANDLE  SourceHandle,
  IN HANDLE  TargetProcessHandle,
  OUT PHANDLE  TargetHandle  OPTIONAL,
  IN ACCESS_MASK  DesiredAccess,
  IN ULONG  Attributes,
  IN ULONG  Options);

NTOSAPI
NTSTATUS
NTAPI
NtQuerySecurityObject(
  IN HANDLE Handle,
  IN SECURITY_INFORMATION  SecurityInformation,
  OUT PSECURITY_DESCRIPTOR  SecurityDescriptor,
  IN ULONG  SecurityDescriptorLength,
  OUT PULONG  ReturnLength);

NTOSAPI
NTSTATUS
NTAPI
ZwQuerySecurityObject(
  IN HANDLE Handle,
  IN SECURITY_INFORMATION  SecurityInformation,
  OUT PSECURITY_DESCRIPTOR  SecurityDescriptor,
  IN ULONG  SecurityDescriptorLength,
  OUT PULONG  ReturnLength);

NTOSAPI
NTSTATUS
NTAPI
NtSetSecurityObject(
  IN HANDLE  Handle,
  IN SECURITY_INFORMATION  SecurityInformation,
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor);

NTOSAPI
NTSTATUS
NTAPI
ZwSetSecurityObject(
  IN HANDLE  Handle,
  IN SECURITY_INFORMATION  SecurityInformation,
  IN PSECURITY_DESCRIPTOR  SecurityDescriptor);

NTOSAPI
NTSTATUS
NTAPI
ZwOpenDirectoryObject(
  OUT PHANDLE  DirectoryHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTOSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryObject(
  IN HANDLE  DirectoryHandle,
  OUT PVOID  Buffer,
  IN ULONG  BufferLength,
  IN BOOLEAN  ReturnSingleEntry,
  IN BOOLEAN  RestartScan,
  IN OUT PULONG  Context,
  OUT PULONG  ReturnLength  OPTIONAL);

typedef struct _DIRECTORY_BASIC_INFORMATION {
  UNICODE_STRING  ObjectName;
  UNICODE_STRING  ObjectTypeName;
} DIRECTORY_BASIC_INFORMATION, *PDIRECTORY_BASIC_INFORMATION;

NTOSAPI
NTSTATUS
NTAPI
ZwCreateSymbolicLinkObject(
  OUT PHANDLE  SymbolicLinkHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN PUNICODE_STRING  TargetName);




/* Virtual memory */

typedef enum _MEMORY_INFORMATION_CLASS {
MemoryBasicInformation,
MemoryWorkingSetList,
MemorySectionName,
MemoryBasicVlmInformation
} MEMORY_INFORMATION_CLASS;

NTOSAPI
NTSTATUS
NTAPI
NtAllocateVirtualMemory(
  IN HANDLE  ProcessHandle,
  IN OUT PVOID  *BaseAddress,
  IN ULONG  ZeroBits,
  IN OUT PULONG  AllocationSize,
  IN ULONG  AllocationType,
  IN ULONG  Protect);

NTOSAPI
NTSTATUS
NTAPI
ZwAllocateVirtualMemory(
  IN HANDLE  ProcessHandle,
  IN OUT PVOID  *BaseAddress,
  IN ULONG  ZeroBits,
  IN OUT PULONG  AllocationSize,
  IN ULONG  AllocationType,
  IN ULONG  Protect);

NTOSAPI
NTSTATUS
NTAPI
NtFreeVirtualMemory(
  IN HANDLE  ProcessHandle,
  IN OUT PVOID  *BaseAddress,
  IN OUT PULONG  FreeSize,
  IN ULONG  FreeType);

NTOSAPI
NTSTATUS
NTAPI
ZwFreeVirtualMemory(
  IN HANDLE  ProcessHandle,
  IN OUT PVOID  *BaseAddress,
  IN OUT PULONG  FreeSize,
  IN ULONG  FreeType);

NTOSAPI
NTSTATUS
NTAPI
ZwQueryVirtualMemory(
	IN HANDLE  ProcessHandle,
	IN PVOID  BaseAddress,
	IN MEMORY_INFORMATION_CLASS  MemoryInformationClass,
	OUT PVOID  MemoryInformation,
	IN ULONG  MemoryInformationLength,
	OUT PULONG  ReturnLength  OPTIONAL);

/* MEMORY_WORKING_SET_LIST.WorkingSetList constants */
#define WSLE_PAGE_READONLY                0x001
#define WSLE_PAGE_EXECUTE                 0x002
#define WSLE_PAGE_READWRITE               0x004
#define WSLE_PAGE_EXECUTE_READ            0x003
#define WSLE_PAGE_WRITECOPY               0x005
#define WSLE_PAGE_EXECUTE_READWRITE       0x006
#define WSLE_PAGE_EXECUTE_WRITECOPY       0x007
#define WSLE_PAGE_SHARE_COUNT_MASK        0x0E0
#define WSLE_PAGE_SHAREABLE               0x100

typedef struct _MEMORY_WORKING_SET_LIST {
  ULONG  NumberOfPages;
  ULONG  WorkingSetList[1];
} MEMORY_WORKING_SET_LIST, *PMEMORY_WORKING_SET_LIST;

typedef struct _MEMORY_SECTION_NAME {
  UNICODE_STRING  SectionFileName;
} MEMORY_SECTION_NAME, *PMEMORY_SECTION_NAME;

/* Zw[Lock|Unlock]VirtualMemory.LockType constants */
#define LOCK_VM_IN_WSL                    0x01
#define LOCK_VM_IN_RAM                    0x02

NTOSAPI
NTSTATUS
NTAPI
ZwLockVirtualMemory(
	IN HANDLE  ProcessHandle,
	IN OUT PVOID  *BaseAddress,
	IN OUT PULONG  LockSize,
	IN ULONG  LockType);

NTOSAPI
NTSTATUS
NTAPI
ZwUnlockVirtualMemory(
	IN HANDLE  ProcessHandle,
	IN OUT PVOID  *BaseAddress,
	IN OUT PULONG  LockSize,
	IN ULONG  LockType);

NTOSAPI
NTSTATUS
NTAPI
ZwReadVirtualMemory(
	IN HANDLE  ProcessHandle,
	IN PVOID  BaseAddress,
	OUT PVOID  Buffer,
	IN ULONG  BufferLength,
	OUT PULONG  ReturnLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwWriteVirtualMemory(
	IN HANDLE  ProcessHandle,
	IN PVOID  BaseAddress,
	IN PVOID  Buffer,
	IN ULONG  BufferLength,
	OUT PULONG  ReturnLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwProtectVirtualMemory(
	IN HANDLE  ProcessHandle,
	IN OUT PVOID  *BaseAddress,
	IN OUT PULONG  ProtectSize,
	IN ULONG  NewProtect,
	OUT PULONG  OldProtect);

NTOSAPI
NTSTATUS
NTAPI
ZwFlushVirtualMemory(
	IN HANDLE  ProcessHandle,
	IN OUT PVOID  *BaseAddress,
	IN OUT PULONG  FlushSize,
	OUT PIO_STATUS_BLOCK  IoStatusBlock);

NTOSAPI
NTSTATUS
NTAPI
ZwAllocateUserPhysicalPages(
	IN HANDLE  ProcessHandle,
	IN PULONG  NumberOfPages,
	OUT PULONG  PageFrameNumbers);

NTOSAPI
NTSTATUS
NTAPI
ZwFreeUserPhysicalPages(
	IN HANDLE  ProcessHandle,
	IN OUT PULONG  NumberOfPages,
	IN PULONG  PageFrameNumbers);

NTOSAPI
NTSTATUS
NTAPI
ZwMapUserPhysicalPages(
	IN PVOID  BaseAddress,
	IN PULONG  NumberOfPages,
	IN PULONG  PageFrameNumbers);

NTOSAPI
NTSTATUS
NTAPI
ZwMapUserPhysicalPagesScatter(
	IN PVOID  *BaseAddresses,
	IN PULONG  NumberOfPages,
	IN PULONG  PageFrameNumbers);

NTOSAPI
NTSTATUS
NTAPI
ZwGetWriteWatch(
	IN HANDLE  ProcessHandle,
	IN ULONG  Flags,
	IN PVOID  BaseAddress,
	IN ULONG  RegionSize,
	OUT PULONG  Buffer,
	IN OUT PULONG  BufferEntries,
	OUT PULONG  Granularity);

NTOSAPI
NTSTATUS
NTAPI
ZwResetWriteWatch(
	IN HANDLE  ProcessHandle,
	IN PVOID  BaseAddress,
	IN ULONG  RegionSize);




/* Sections */

typedef enum _SECTION_INFORMATION_CLASS {
  SectionBasicInformation,
  SectionImageInformation
} SECTION_INFORMATION_CLASS;

NTOSAPI
NTSTATUS
NTAPI
NtCreateSection(
  OUT PHANDLE  SectionHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN PLARGE_INTEGER  SectionSize  OPTIONAL,
  IN ULONG  Protect,
  IN ULONG  Attributes,
  IN HANDLE  FileHandle);

NTOSAPI
NTSTATUS
NTAPI
ZwCreateSection(
  OUT PHANDLE  SectionHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN PLARGE_INTEGER  SectionSize  OPTIONAL,
  IN ULONG  Protect,
  IN ULONG  Attributes,
  IN HANDLE  FileHandle);

NTOSAPI
NTSTATUS
NTAPI
ZwQuerySection(
	IN HANDLE  SectionHandle,
	IN SECTION_INFORMATION_CLASS  SectionInformationClass,
	OUT PVOID  SectionInformation,
	IN ULONG  SectionInformationLength,
  OUT PULONG  ResultLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwExtendSection(
	IN HANDLE  SectionHandle,
	IN PLARGE_INTEGER  SectionSize);

NTOSAPI
NTSTATUS
NTAPI
ZwAreMappedFilesTheSame(
	IN PVOID  Address1,
	IN PVOID  Address2);




/* Threads */

typedef struct _USER_STACK {
	PVOID  FixedStackBase;
	PVOID  FixedStackLimit;
	PVOID  ExpandableStackBase;
	PVOID  ExpandableStackLimit;
	PVOID  ExpandableStackBottom;
} USER_STACK, *PUSER_STACK;

NTOSAPI
NTSTATUS
NTAPI
ZwCreateThread(
	OUT PHANDLE  ThreadHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN POBJECT_ATTRIBUTES  ObjectAttributes,
	IN HANDLE  ProcessHandle,
	OUT PCLIENT_ID  ClientId,
	IN PCONTEXT  ThreadContext,
	IN PUSER_STACK  UserStack,
	IN BOOLEAN  CreateSuspended);

NTOSAPI
NTSTATUS
NTAPI
NtOpenThread(
  OUT PHANDLE  ThreadHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN PCLIENT_ID  ClientId);

NTOSAPI
NTSTATUS
NTAPI
ZwOpenThread(
  OUT PHANDLE  ThreadHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN PCLIENT_ID  ClientId);

NTOSAPI
NTSTATUS
NTAPI
ZwTerminateThread(
	IN HANDLE  ThreadHandle  OPTIONAL,
	IN NTSTATUS  ExitStatus);

NTOSAPI
NTSTATUS
NTAPI
NtQueryInformationThread(
  IN HANDLE  ThreadHandle,
  IN THREADINFOCLASS  ThreadInformationClass,
  OUT PVOID  ThreadInformation,
  IN ULONG  ThreadInformationLength,
  OUT PULONG  ReturnLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwQueryInformationThread(
  IN HANDLE  ThreadHandle,
  IN THREADINFOCLASS  ThreadInformationClass,
  OUT PVOID  ThreadInformation,
  IN ULONG  ThreadInformationLength,
  OUT PULONG  ReturnLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
NtSetInformationThread(
  IN HANDLE  ThreadHandle,
  IN THREADINFOCLASS  ThreadInformationClass,
  IN PVOID  ThreadInformation,
  IN ULONG  ThreadInformationLength);

typedef struct _THREAD_BASIC_INFORMATION {
	NTSTATUS  ExitStatus;
	PNT_TIB  TebBaseAddress;
	CLIENT_ID  ClientId;
	KAFFINITY  AffinityMask;
	KPRIORITY  Priority;
	KPRIORITY  BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef struct _KERNEL_USER_TIMES {
	LARGE_INTEGER  CreateTime;
	LARGE_INTEGER  ExitTime;
	LARGE_INTEGER  KernelTime;
	LARGE_INTEGER  UserTime;
} KERNEL_USER_TIMES, *PKERNEL_USER_TIMES;

NTOSAPI
NTSTATUS
NTAPI
ZwSuspendThread(
  IN HANDLE  ThreadHandle,
  OUT PULONG  PreviousSuspendCount  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwResumeThread(
  IN HANDLE  ThreadHandle,
  OUT PULONG  PreviousSuspendCount  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwGetContextThread(
  IN HANDLE  ThreadHandle,
  OUT PCONTEXT  Context);

NTOSAPI
NTSTATUS
NTAPI
ZwSetContextThread(
	IN HANDLE  ThreadHandle,
	IN PCONTEXT  Context);

NTOSAPI
NTSTATUS
NTAPI
ZwQueueApcThread(
	IN HANDLE  ThreadHandle,
	IN PKNORMAL_ROUTINE  ApcRoutine,
	IN PVOID  ApcContext  OPTIONAL,
	IN PVOID  Argument1  OPTIONAL,
	IN PVOID  Argument2  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwTestAlert(
  VOID);

NTOSAPI
NTSTATUS
NTAPI
ZwAlertThread(
  IN HANDLE  ThreadHandle);

NTOSAPI
NTSTATUS
NTAPI
ZwAlertResumeThread(
  IN HANDLE  ThreadHandle,
  OUT PULONG  PreviousSuspendCount  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwRegisterThreadTerminatePort(
  IN HANDLE  PortHandle);

NTOSAPI
NTSTATUS
NTAPI
ZwImpersonateThread(
	IN HANDLE  ThreadHandle,
	IN HANDLE  TargetThreadHandle,
	IN PSECURITY_QUALITY_OF_SERVICE  SecurityQos);

NTOSAPI
NTSTATUS
NTAPI
ZwImpersonateAnonymousToken(
  IN HANDLE  ThreadHandle);




/* Processes */

NTOSAPI
NTSTATUS
NTAPI
ZwCreateProcess(
	OUT PHANDLE  ProcessHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN POBJECT_ATTRIBUTES  ObjectAttributes,
	IN HANDLE  InheritFromProcessHandle,
	IN BOOLEAN  InheritHandles,
	IN HANDLE  SectionHandle  OPTIONAL,
	IN HANDLE  DebugPort  OPTIONAL,
	IN HANDLE  ExceptionPort  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwCreateProcess(
	OUT PHANDLE  ProcessHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN POBJECT_ATTRIBUTES  ObjectAttributes,
	IN HANDLE  InheritFromProcessHandle,
	IN BOOLEAN  InheritHandles,
	IN HANDLE  SectionHandle  OPTIONAL,
	IN HANDLE  DebugPort  OPTIONAL,
	IN HANDLE  ExceptionPort  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwTerminateProcess(
	IN HANDLE  ProcessHandle  OPTIONAL,
	IN NTSTATUS  ExitStatus);

NTOSAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess(
  IN HANDLE  ProcessHandle,
  IN PROCESSINFOCLASS  ProcessInformationClass,
  OUT PVOID  ProcessInformation,
  IN ULONG  ProcessInformationLength,
  OUT PULONG  ReturnLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
NtSetInformationProcess(
  IN HANDLE  ProcessHandle,
  IN PROCESSINFOCLASS  ProcessInformationClass,
  IN PVOID  ProcessInformation,
  IN ULONG  ProcessInformationLength);

NTOSAPI
NTSTATUS
NTAPI
ZwSetInformationProcess(
  IN HANDLE  ProcessHandle,
  IN PROCESSINFOCLASS  ProcessInformationClass,
  IN PVOID  ProcessInformation,
  IN ULONG  ProcessInformationLength);

typedef struct _PROCESS_BASIC_INFORMATION {
	NTSTATUS  ExitStatus;
	PPEB  PebBaseAddress;
	KAFFINITY  AffinityMask;
	KPRIORITY  BasePriority;
	ULONG  UniqueProcessId;
	ULONG  InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef struct _PROCESS_ACCESS_TOKEN {
  HANDLE  Token;
  HANDLE  Thread;
} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

/* DefaultHardErrorMode constants */
/* also in winbase.h */
#define SEM_FAILCRITICALERRORS            0x0001
#define SEM_NOGPFAULTERRORBOX             0x0002
#define SEM_NOALIGNMENTFAULTEXCEPT        0x0004
#define SEM_NOOPENFILEERRORBOX            0x8000
/* end winbase.h */
typedef struct _POOLED_USAGE_AND_LIMITS {
	ULONG  PeakPagedPoolUsage;
	ULONG  PagedPoolUsage;
	ULONG  PagedPoolLimit;
	ULONG  PeakNonPagedPoolUsage;
	ULONG  NonPagedPoolUsage;
	ULONG  NonPagedPoolLimit;
	ULONG  PeakPagefileUsage;
	ULONG  PagefileUsage;
	ULONG  PagefileLimit;
} POOLED_USAGE_AND_LIMITS, *PPOOLED_USAGE_AND_LIMITS;

typedef struct _PROCESS_WS_WATCH_INFORMATION {
  PVOID  FaultingPc;
  PVOID  FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

/* PROCESS_PRIORITY_CLASS.PriorityClass constants */
#define PC_IDLE                           1
#define PC_NORMAL                         2
#define PC_HIGH                           3
#define PC_REALTIME                       4
#define PC_BELOW_NORMAL                   5
#define PC_ABOVE_NORMAL                   6

typedef struct _PROCESS_PRIORITY_CLASS {
  BOOLEAN  Foreground;
  UCHAR  PriorityClass;
} PROCESS_PRIORITY_CLASS, *PPROCESS_PRIORITY_CLASS;

/* PROCESS_DEVICEMAP_INFORMATION.DriveType constants */
#define DRIVE_UNKNOWN                     0
#define DRIVE_NO_ROOT_DIR                 1
#define DRIVE_REMOVABLE                   2
#define DRIVE_FIXED                       3
#define DRIVE_REMOTE                      4
#define DRIVE_CDROM                       5
#define DRIVE_RAMDISK                     6

typedef struct _PROCESS_DEVICEMAP_INFORMATION {
	_ANONYMOUS_UNION union {
		struct {
		  HANDLE  DirectoryHandle;
		} Set;
		struct {
		  ULONG  DriveMap;
		  UCHAR  DriveType[32];
		} Query;
	} DUMMYUNIONNAME;
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

typedef struct _PROCESS_SESSION_INFORMATION {
  ULONG  SessionId;
} PROCESS_SESSION_INFORMATION, *PPROCESS_SESSION_INFORMATION;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
	ULONG  AllocationSize;
	ULONG  Size;
	ULONG  Flags;
	ULONG  DebugFlags;
	HANDLE  hConsole;
	ULONG  ProcessGroup;
	HANDLE  hStdInput;
	HANDLE  hStdOutput;
	HANDLE  hStdError;
	UNICODE_STRING  CurrentDirectoryName;
	HANDLE  CurrentDirectoryHandle;
	UNICODE_STRING  DllPath;
	UNICODE_STRING  ImagePathName;
	UNICODE_STRING  CommandLine;
	PWSTR  Environment;
	ULONG  dwX;
	ULONG  dwY;
	ULONG  dwXSize;
	ULONG  dwYSize;
	ULONG  dwXCountChars;
	ULONG  dwYCountChars;
	ULONG  dwFillAttribute;
	ULONG  dwFlags;
	ULONG  wShowWindow;
	UNICODE_STRING  WindowTitle;
	UNICODE_STRING  DesktopInfo;
	UNICODE_STRING  ShellInfo;
	UNICODE_STRING  RuntimeInfo;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

NTSTATUS
NTAPI
RtlCreateProcessParameters(
	OUT PRTL_USER_PROCESS_PARAMETERS  *ProcessParameters,
	IN PUNICODE_STRING  ImageFile,
	IN PUNICODE_STRING  DllPath  OPTIONAL,
	IN PUNICODE_STRING  CurrentDirectory  OPTIONAL,
	IN PUNICODE_STRING  CommandLine  OPTIONAL,
	IN PWSTR  Environment OPTIONAL,
	IN PUNICODE_STRING  WindowTitle  OPTIONAL,
	IN PUNICODE_STRING  DesktopInfo  OPTIONAL,
	IN PUNICODE_STRING  ShellInfo  OPTIONAL,
	IN PUNICODE_STRING  RuntimeInfo  OPTIONAL);

NTSTATUS
NTAPI
RtlDestroyProcessParameters(
  IN PRTL_USER_PROCESS_PARAMETERS  ProcessParameters);

typedef struct _DEBUG_BUFFER {
	HANDLE  SectionHandle;
	PVOID  SectionBase;
	PVOID  RemoteSectionBase;
	ULONG  SectionBaseDelta;
	HANDLE  EventPairHandle;
	ULONG  Unknown[2];
	HANDLE  RemoteThreadHandle;
	ULONG  InfoClassMask;
	ULONG  SizeOfInfo;
	ULONG  AllocatedSize;
	ULONG  SectionSize;
	PVOID  ModuleInformation;
	PVOID  BackTraceInformation;
	PVOID  HeapInformation;
	PVOID  LockInformation;
	PVOID  Reserved[8];
} DEBUG_BUFFER, *PDEBUG_BUFFER;

PDEBUG_BUFFER
NTAPI
RtlCreateQueryDebugBuffer(
	IN ULONG  Size,
	IN BOOLEAN  EventPair);

/* RtlQueryProcessDebugInformation.DebugInfoClassMask constants */
#define PDI_MODULES                       0x01
#define PDI_BACKTRACE                     0x02
#define PDI_HEAPS                         0x04
#define PDI_HEAP_TAGS                     0x08
#define PDI_HEAP_BLOCKS                   0x10
#define PDI_LOCKS                         0x20

NTSTATUS
NTAPI
RtlQueryProcessDebugInformation(
	IN ULONG  ProcessId,
	IN ULONG  DebugInfoClassMask,
	IN OUT PDEBUG_BUFFER  DebugBuffer);

NTSTATUS
NTAPI
RtlDestroyQueryDebugBuffer(
  IN PDEBUG_BUFFER  DebugBuffer);

/* DEBUG_MODULE_INFORMATION.Flags constants */
#define LDRP_STATIC_LINK                  0x00000002
#define LDRP_IMAGE_DLL                    0x00000004
#define LDRP_LOAD_IN_PROGRESS             0x00001000
#define LDRP_UNLOAD_IN_PROGRESS           0x00002000
#define LDRP_ENTRY_PROCESSED              0x00004000
#define LDRP_ENTRY_INSERTED               0x00008000
#define LDRP_CURRENT_LOAD                 0x00010000
#define LDRP_FAILED_BUILTIN_LOAD          0x00020000
#define LDRP_DONT_CALL_FOR_THREADS        0x00040000
#define LDRP_PROCESS_ATTACH_CALLED        0x00080000
#define LDRP_DEBUG_SYMBOLS_LOADED         0x00100000
#define LDRP_IMAGE_NOT_AT_BASE            0x00200000
#define LDRP_WX86_IGNORE_MACHINETYPE      0x00400000

typedef struct _DEBUG_MODULE_INFORMATION {
	ULONG  Reserved[2];
	ULONG  Base;
	ULONG  Size;
	ULONG  Flags;
	USHORT  Index;
	USHORT  Unknown;
	USHORT  LoadCount;
	USHORT  ModuleNameOffset;
	CHAR  ImageName[256];
} DEBUG_MODULE_INFORMATION, *PDEBUG_MODULE_INFORMATION;

typedef struct _DEBUG_HEAP_INFORMATION {
	ULONG  Base;
	ULONG  Flags;
	USHORT  Granularity;
	USHORT  Unknown;
	ULONG  Allocated;
	ULONG  Committed;
	ULONG  TagCount;
	ULONG  BlockCount;
	ULONG  Reserved[7];
	PVOID  Tags;
	PVOID  Blocks;
} DEBUG_HEAP_INFORMATION, *PDEBUG_HEAP_INFORMATION;

typedef struct _DEBUG_LOCK_INFORMATION {
	PVOID  Address;
	USHORT  Type;
	USHORT  CreatorBackTraceIndex;
	ULONG  OwnerThreadId;
	ULONG  ActiveCount;
	ULONG  ContentionCount;
	ULONG  EntryCount;
	ULONG  RecursionCount;
	ULONG  NumberOfSharedWaiters;
	ULONG  NumberOfExclusiveWaiters;
} DEBUG_LOCK_INFORMATION, *PDEBUG_LOCK_INFORMATION;



/* Jobs */

NTOSAPI
NTSTATUS
NTAPI
ZwCreateJobObject(
	OUT PHANDLE  JobHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTOSAPI
NTSTATUS
NTAPI
ZwOpenJobObject(
	OUT PHANDLE  JobHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTOSAPI
NTSTATUS
NTAPI
ZwTerminateJobObject(
	IN HANDLE  JobHandle,
	IN NTSTATUS  ExitStatus);

NTOSAPI
NTSTATUS
NTAPI
ZwAssignProcessToJobObject(
	IN HANDLE  JobHandle,
	IN HANDLE  ProcessHandle);

NTOSAPI
NTSTATUS
NTAPI
ZwQueryInformationJobObject(
	IN HANDLE  JobHandle,
	IN JOBOBJECTINFOCLASS  JobInformationClass,
	OUT PVOID  JobInformation,
	IN ULONG  JobInformationLength,
	OUT PULONG  ReturnLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwSetInformationJobObject(
	IN HANDLE  JobHandle,
	IN JOBOBJECTINFOCLASS  JobInformationClass,
	IN PVOID  JobInformation,
	IN ULONG  JobInformationLength);


/* Tokens */

NTOSAPI
NTSTATUS
NTAPI
ZwCreateToken(
OUT PHANDLE TokenHandle,
IN ACCESS_MASK DesiredAccess,
IN POBJECT_ATTRIBUTES ObjectAttributes,
IN TOKEN_TYPE Type,
IN PLUID AuthenticationId,
IN PLARGE_INTEGER ExpirationTime,
IN PTOKEN_USER User,
IN PTOKEN_GROUPS Groups,
IN PTOKEN_PRIVILEGES Privileges,
IN PTOKEN_OWNER Owner,
IN PTOKEN_PRIMARY_GROUP PrimaryGroup,
IN PTOKEN_DEFAULT_DACL DefaultDacl,
IN PTOKEN_SOURCE Source
);

NTOSAPI
NTSTATUS
NTAPI
NtOpenProcessToken(
  IN HANDLE  ProcessHandle,
  IN ACCESS_MASK  DesiredAccess,
  OUT PHANDLE  TokenHandle);

NTOSAPI
NTSTATUS
NTAPI
ZwOpenProcessToken(
  IN HANDLE  ProcessHandle,
  IN ACCESS_MASK  DesiredAccess,
  OUT PHANDLE  TokenHandle);

NTOSAPI
NTSTATUS
NTAPI
NtOpenThreadToken(
  IN HANDLE  ThreadHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN BOOLEAN  OpenAsSelf,
  OUT PHANDLE  TokenHandle);

NTOSAPI
NTSTATUS
NTAPI
ZwOpenThreadToken(
  IN HANDLE  ThreadHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN BOOLEAN  OpenAsSelf,
  OUT PHANDLE  TokenHandle);

NTOSAPI
NTSTATUS
NTAPI
NtDuplicateToken(
  IN HANDLE  ExistingTokenHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN BOOLEAN  EffectiveOnly,
  IN TOKEN_TYPE  TokenType,
  OUT PHANDLE  NewTokenHandle);

NTOSAPI
NTSTATUS
NTAPI
ZwDuplicateToken(
  IN HANDLE  ExistingTokenHandle,
  IN ACCESS_MASK  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes,
  IN BOOLEAN  EffectiveOnly,
  IN TOKEN_TYPE  TokenType,
  OUT PHANDLE  NewTokenHandle);

NTOSAPI
NTSTATUS
NTAPI
ZwFilterToken(
	IN HANDLE  ExistingTokenHandle,
	IN ULONG  Flags,
	IN PTOKEN_GROUPS  SidsToDisable,
	IN PTOKEN_PRIVILEGES  PrivilegesToDelete,
	IN PTOKEN_GROUPS  SidsToRestricted,
	OUT PHANDLE  NewTokenHandle);

NTOSAPI
NTSTATUS
NTAPI
NtAdjustPrivilegesToken(
  IN HANDLE  TokenHandle,
  IN BOOLEAN  DisableAllPrivileges,
  IN PTOKEN_PRIVILEGES  NewState,
  IN ULONG  BufferLength,
  OUT PTOKEN_PRIVILEGES  PreviousState  OPTIONAL,
  OUT PULONG  ReturnLength);

NTOSAPI
NTSTATUS
NTAPI
ZwAdjustPrivilegesToken(
  IN HANDLE  TokenHandle,
  IN BOOLEAN  DisableAllPrivileges,
  IN PTOKEN_PRIVILEGES  NewState,
  IN ULONG  BufferLength,
  OUT PTOKEN_PRIVILEGES  PreviousState  OPTIONAL,
  OUT PULONG  ReturnLength);

NTOSAPI
NTSTATUS
NTAPI
ZwAdjustGroupsToken(
	IN HANDLE  TokenHandle,
	IN BOOLEAN  ResetToDefault,
	IN PTOKEN_GROUPS  NewState,
	IN ULONG  BufferLength,
	OUT PTOKEN_GROUPS  PreviousState  OPTIONAL,
	OUT PULONG  ReturnLength);

NTOSAPI
NTSTATUS
NTAPI
NtQueryInformationToken(
  IN HANDLE  TokenHandle,
  IN TOKEN_INFORMATION_CLASS  TokenInformationClass,
  OUT PVOID  TokenInformation,
  IN ULONG  TokenInformationLength,
  OUT PULONG  ReturnLength);

NTOSAPI
NTSTATUS
NTAPI
ZwQueryInformationToken(
  IN HANDLE  TokenHandle,
  IN TOKEN_INFORMATION_CLASS  TokenInformationClass,
  OUT PVOID  TokenInformation,
  IN ULONG  TokenInformationLength,
  OUT PULONG  ReturnLength);

NTOSAPI
NTSTATUS
NTAPI
ZwSetInformationToken(
	IN HANDLE  TokenHandle,
	IN TOKEN_INFORMATION_CLASS  TokenInformationClass,
	IN PVOID  TokenInformation,
  IN ULONG  TokenInformationLength);




/* Time */

NTOSAPI
NTSTATUS
NTAPI
ZwQuerySystemTime(
  OUT PLARGE_INTEGER  CurrentTime);

NTOSAPI
NTSTATUS
NTAPI
ZwSetSystemTime(
  IN PLARGE_INTEGER  NewTime,
  OUT PLARGE_INTEGER  OldTime  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwQueryPerformanceCounter(
  OUT PLARGE_INTEGER  PerformanceCount,
  OUT PLARGE_INTEGER  PerformanceFrequency  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwQueryPerformanceCounter(
  OUT PLARGE_INTEGER  PerformanceCount,
  OUT PLARGE_INTEGER  PerformanceFrequency  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwQueryTimerResolution(
	OUT PULONG  CoarsestResolution,
	OUT PULONG  FinestResolution,
	OUT PULONG  ActualResolution);

NTOSAPI
NTSTATUS
NTAPI
ZwDelayExecution(
  IN BOOLEAN  Alertable,
  IN PLARGE_INTEGER  Interval);

NTOSAPI
NTSTATUS
NTAPI
ZwYieldExecution(
  VOID);

NTOSAPI
ULONG
NTAPI
ZwGetTickCount(
  VOID);




/* Execution profiling */

NTOSAPI
NTSTATUS
NTAPI
ZwCreateProfile(
	OUT PHANDLE  ProfileHandle,
	IN HANDLE  ProcessHandle,
	IN PVOID  Base,
	IN ULONG  Size,
	IN ULONG  BucketShift,
	IN PULONG  Buffer,
	IN ULONG  BufferLength,
	IN KPROFILE_SOURCE  Source,
	IN ULONG  ProcessorMask);

NTOSAPI
NTSTATUS
NTAPI
ZwSetIntervalProfile(
  IN ULONG  Interval,
  IN KPROFILE_SOURCE  Source);

NTOSAPI
NTSTATUS
NTAPI
ZwQueryIntervalProfile(
	IN KPROFILE_SOURCE  Source,
	OUT PULONG  Interval);

NTOSAPI
NTSTATUS
NTAPI
ZwStartProfile(
  IN HANDLE  ProfileHandle);

NTOSAPI
NTSTATUS
NTAPI
ZwStopProfile(
  IN HANDLE  ProfileHandle);




/* Local Procedure Call (LPC) */

typedef struct _LPC_MESSAGE {
	USHORT  DataSize;
	USHORT  MessageSize;
	USHORT  MessageType;
	USHORT  VirtualRangesOffset;
	CLIENT_ID  ClientId;
	ULONG  MessageId;
	ULONG  SectionSize;
	UCHAR  Data[ANYSIZE_ARRAY];
} LPC_MESSAGE, *PLPC_MESSAGE;

typedef enum _LPC_TYPE {
	LPC_NEW_MESSAGE,
	LPC_REQUEST,
	LPC_REPLY,
	LPC_DATAGRAM,
	LPC_LOST_REPLY,
	LPC_PORT_CLOSED,
	LPC_CLIENT_DIED,
	LPC_EXCEPTION,
	LPC_DEBUG_EVENT,
	LPC_ERROR_EVENT,
	LPC_CONNECTION_REQUEST,
	LPC_CONNECTION_REFUSED,
  LPC_MAXIMUM
} LPC_TYPE;

typedef struct _LPC_SECTION_WRITE {
	ULONG  Length;
	HANDLE  SectionHandle;
	ULONG  SectionOffset;
	ULONG  ViewSize;
	PVOID  ViewBase;
	PVOID  TargetViewBase;
} LPC_SECTION_WRITE, *PLPC_SECTION_WRITE;

typedef struct _LPC_SECTION_READ {
	ULONG  Length;
	ULONG  ViewSize;
	PVOID  ViewBase;
} LPC_SECTION_READ, *PLPC_SECTION_READ;

NTOSAPI
NTSTATUS
NTAPI
ZwCreatePort(
	OUT PHANDLE  PortHandle,
	IN POBJECT_ATTRIBUTES  ObjectAttributes,
	IN ULONG  MaxDataSize,
	IN ULONG  MaxMessageSize,
	IN ULONG  Reserved);

NTOSAPI
NTSTATUS
NTAPI
ZwCreateWaitablePort(
	OUT PHANDLE  PortHandle,
	IN POBJECT_ATTRIBUTES  ObjectAttributes,
	IN ULONG  MaxDataSize,
	IN ULONG  MaxMessageSize,
	IN ULONG  Reserved);

NTOSAPI
NTSTATUS
NTAPI
NtConnectPort(
  OUT PHANDLE  PortHandle,
  IN PUNICODE_STRING  PortName,
  IN PSECURITY_QUALITY_OF_SERVICE  SecurityQos,
  IN OUT PLPC_SECTION_WRITE  WriteSection  OPTIONAL,
  IN OUT PLPC_SECTION_READ  ReadSection  OPTIONAL,
  OUT PULONG  MaxMessageSize  OPTIONAL,
  IN OUT PVOID  ConnectData  OPTIONAL,
  IN OUT PULONG  ConnectDataLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwConnectPort(
  OUT PHANDLE  PortHandle,
  IN PUNICODE_STRING  PortName,
  IN PSECURITY_QUALITY_OF_SERVICE  SecurityQos,
  IN OUT PLPC_SECTION_WRITE  WriteSection  OPTIONAL,
  IN OUT PLPC_SECTION_READ  ReadSection  OPTIONAL,
  OUT PULONG  MaxMessageSize  OPTIONAL,
  IN OUT PVOID  ConnectData  OPTIONAL,
  IN OUT PULONG  ConnectDataLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwConnectPort(
	OUT PHANDLE  PortHandle,
	IN PUNICODE_STRING  PortName,
	IN PSECURITY_QUALITY_OF_SERVICE  SecurityQos,
	IN OUT PLPC_SECTION_WRITE  WriteSection  OPTIONAL,
	IN OUT PLPC_SECTION_READ  ReadSection  OPTIONAL,
	OUT PULONG  MaxMessageSize  OPTIONAL,
	IN OUT PVOID  ConnectData  OPTIONAL,
	IN OUT PULONG  ConnectDataLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwListenPort(
  IN HANDLE  PortHandle,
  OUT PLPC_MESSAGE  Message);

NTOSAPI
NTSTATUS
NTAPI
ZwAcceptConnectPort(
	OUT PHANDLE  PortHandle,
	IN ULONG  PortIdentifier,
	IN PLPC_MESSAGE  Message,
	IN BOOLEAN  Accept,
	IN OUT PLPC_SECTION_WRITE  WriteSection  OPTIONAL,
	IN OUT PLPC_SECTION_READ  ReadSection  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwCompleteConnectPort(
  IN HANDLE  PortHandle);

NTOSAPI
NTSTATUS
NTAPI
NtRequestPort(
  IN HANDLE  PortHandle,
  IN PLPC_MESSAGE  RequestMessage);

NTOSAPI
NTSTATUS
NTAPI
NtRequestWaitReplyPort(
  IN HANDLE  PortHandle,
  IN PLPC_MESSAGE  RequestMessage,
  OUT PLPC_MESSAGE  ReplyMessage);

NTOSAPI
NTSTATUS
NTAPI
ZwRequestWaitReplyPort(
  IN HANDLE  PortHandle,
  IN PLPC_MESSAGE  RequestMessage,
  OUT PLPC_MESSAGE  ReplyMessage);

NTOSAPI
NTSTATUS
NTAPI
ZwReplyPort(
	IN HANDLE  PortHandle,
	IN PLPC_MESSAGE  ReplyMessage);

NTOSAPI
NTSTATUS
NTAPI
ZwReplyWaitReplyPort(
	IN HANDLE  PortHandle,
	IN OUT PLPC_MESSAGE  ReplyMessage);

NTOSAPI
NTSTATUS
NTAPI
ZwReplyWaitReceivePort(
	IN HANDLE  PortHandle,
	OUT PULONG  PortIdentifier  OPTIONAL,
	IN PLPC_MESSAGE  ReplyMessage  OPTIONAL,
	OUT PLPC_MESSAGE  Message);

NTOSAPI
NTSTATUS
NTAPI
ZwReplyWaitReceivePortEx(
	IN HANDLE  PortHandle,
	OUT PULONG  PortIdentifier  OPTIONAL,
	IN PLPC_MESSAGE  ReplyMessage  OPTIONAL,
	OUT PLPC_MESSAGE  Message,
	IN PLARGE_INTEGER  Timeout);

NTOSAPI
NTSTATUS
NTAPI
ZwReadRequestData(
	IN HANDLE  PortHandle,
	IN PLPC_MESSAGE  Message,
	IN ULONG  Index,
	OUT PVOID  Buffer,
	IN ULONG  BufferLength,
	OUT PULONG  ReturnLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwWriteRequestData(
	IN HANDLE  PortHandle,
	IN PLPC_MESSAGE  Message,
	IN ULONG  Index,
	IN PVOID  Buffer,
	IN ULONG  BufferLength,
	OUT PULONG  ReturnLength  OPTIONAL);

typedef enum _PORT_INFORMATION_CLASS {
  PortBasicInformation
} PORT_INFORMATION_CLASS;

NTOSAPI
NTSTATUS
NTAPI
ZwQueryInformationPort(
	IN HANDLE  PortHandle,
	IN PORT_INFORMATION_CLASS  PortInformationClass,
	OUT PVOID  PortInformation,
	IN ULONG  PortInformationLength,
  OUT PULONG  ReturnLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwImpersonateClientOfPort(
  IN HANDLE  PortHandle,
  IN PLPC_MESSAGE  Message);




/* Files */

NTOSAPI
NTSTATUS
NTAPI
NtDeleteFile(
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTOSAPI
NTSTATUS
NTAPI
ZwDeleteFile(
  IN POBJECT_ATTRIBUTES  ObjectAttributes);

NTOSAPI
NTSTATUS
NTAPI
ZwFlushBuffersFile(
	IN HANDLE  FileHandle,
	OUT PIO_STATUS_BLOCK  IoStatusBlock);

NTOSAPI
NTSTATUS
NTAPI
ZwCancelIoFile(
	IN HANDLE  FileHandle,
	OUT PIO_STATUS_BLOCK  IoStatusBlock);

NTOSAPI
NTSTATUS
NTAPI
ZwReadFileScatter(
	IN HANDLE  FileHandle,
	IN HANDLE  Event OPTIONAL,
	IN PIO_APC_ROUTINE  ApcRoutine  OPTIONAL,
	IN PVOID  ApcContext  OPTIONAL,
	OUT PIO_STATUS_BLOCK  IoStatusBlock,
	IN PFILE_SEGMENT_ELEMENT  Buffer,
	IN ULONG  Length,
	IN PLARGE_INTEGER  ByteOffset  OPTIONAL,
	IN PULONG  Key  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwWriteFileGather(
	IN HANDLE  FileHandle,
	IN HANDLE  Event  OPTIONAL,
	IN PIO_APC_ROUTINE  ApcRoutine  OPTIONAL,
	IN PVOID  ApcContext  OPTIONAL,
	OUT PIO_STATUS_BLOCK  IoStatusBlock,
	IN PFILE_SEGMENT_ELEMENT  Buffer,
	IN ULONG  Length,
	IN PLARGE_INTEGER  ByteOffset  OPTIONAL,
	IN PULONG  Key  OPTIONAL);




/* Registry keys */

NTOSAPI
NTSTATUS
NTAPI
ZwSaveKey(
	IN HANDLE  KeyHandle,
	IN HANDLE  FileHandle);

NTOSAPI
NTSTATUS
NTAPI
ZwSaveMergedKeys(
	IN HANDLE  KeyHandle1,
	IN HANDLE  KeyHandle2,
	IN HANDLE  FileHandle);

NTOSAPI
NTSTATUS
NTAPI
ZwRestoreKey(
  IN HANDLE  KeyHandle,
  IN HANDLE  FileHandle,
  IN ULONG  Flags);

NTOSAPI
NTSTATUS
NTAPI
ZwLoadKey(
  IN POBJECT_ATTRIBUTES  KeyObjectAttributes,
  IN POBJECT_ATTRIBUTES  FileObjectAttributes);

NTOSAPI
NTSTATUS
NTAPI
ZwLoadKey2(
	IN POBJECT_ATTRIBUTES  KeyObjectAttributes,
	IN POBJECT_ATTRIBUTES  FileObjectAttributes,
	IN ULONG  Flags);

NTOSAPI
NTSTATUS
NTAPI
ZwUnloadKey(
  IN POBJECT_ATTRIBUTES  KeyObjectAttributes);

NTOSAPI
NTSTATUS
NTAPI
ZwQueryOpenSubKeys(
	IN POBJECT_ATTRIBUTES  KeyObjectAttributes,
	OUT PULONG  NumberOfKeys);

NTOSAPI
NTSTATUS
NTAPI
ZwReplaceKey(
	IN POBJECT_ATTRIBUTES  NewFileObjectAttributes,
	IN HANDLE  KeyHandle,
	IN POBJECT_ATTRIBUTES  OldFileObjectAttributes);

typedef enum _KEY_SET_INFORMATION_CLASS {
  KeyLastWriteTimeInformation
} KEY_SET_INFORMATION_CLASS;

NTOSAPI
NTSTATUS
NTAPI
ZwSetInformationKey(
	IN HANDLE  KeyHandle,
	IN KEY_SET_INFORMATION_CLASS  KeyInformationClass,
	IN PVOID  KeyInformation,
	IN ULONG  KeyInformationLength);

typedef struct _KEY_LAST_WRITE_TIME_INFORMATION {
  LARGE_INTEGER LastWriteTime;
} KEY_LAST_WRITE_TIME_INFORMATION, *PKEY_LAST_WRITE_TIME_INFORMATION;

typedef struct _KEY_NAME_INFORMATION {
	ULONG NameLength;
	WCHAR Name[1];
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

NTOSAPI
NTSTATUS
NTAPI
ZwNotifyChangeKey(
	IN HANDLE  KeyHandle,
	IN HANDLE  EventHandle  OPTIONAL,
	IN PIO_APC_ROUTINE  ApcRoutine  OPTIONAL,
	IN PVOID  ApcContext  OPTIONAL,
	OUT PIO_STATUS_BLOCK  IoStatusBlock,
	IN ULONG  NotifyFilter,
	IN BOOLEAN  WatchSubtree,
	IN PVOID  Buffer,
	IN ULONG  BufferLength,
	IN BOOLEAN  Asynchronous);

/* ZwNotifyChangeMultipleKeys.Flags constants */
#define REG_MONITOR_SINGLE_KEY            0x00
#define REG_MONITOR_SECOND_KEY            0x01

NTOSAPI
NTSTATUS
NTAPI
ZwNotifyChangeMultipleKeys(
	IN HANDLE  KeyHandle,
	IN ULONG  Flags,
	IN POBJECT_ATTRIBUTES  KeyObjectAttributes,
	IN HANDLE  EventHandle  OPTIONAL,
	IN PIO_APC_ROUTINE  ApcRoutine  OPTIONAL,
	IN PVOID  ApcContext  OPTIONAL,
	OUT PIO_STATUS_BLOCK  IoStatusBlock,
	IN ULONG  NotifyFilter,
	IN BOOLEAN  WatchSubtree,
	IN PVOID  Buffer,
	IN ULONG  BufferLength,
	IN BOOLEAN  Asynchronous);

NTOSAPI
NTSTATUS
NTAPI
ZwQueryMultipleValueKey(
	IN HANDLE  KeyHandle,
	IN OUT  PKEY_VALUE_ENTRY  ValueList,
	IN ULONG  NumberOfValues,
	OUT PVOID  Buffer,
	IN OUT PULONG  Length,
	OUT PULONG  ReturnLength);

NTOSAPI
NTSTATUS
NTAPI
ZwInitializeRegistry(
  IN BOOLEAN  Setup);




/* Security and auditing */

NTOSAPI
NTSTATUS
NTAPI
ZwPrivilegeCheck(
	IN HANDLE  TokenHandle,
	IN PPRIVILEGE_SET  RequiredPrivileges,
	OUT PBOOLEAN  Result);

NTOSAPI
NTSTATUS
NTAPI
ZwPrivilegeObjectAuditAlarm(
	IN PUNICODE_STRING  SubsystemName,
	IN PVOID  HandleId,
	IN HANDLE  TokenHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN PPRIVILEGE_SET  Privileges,
	IN BOOLEAN  AccessGranted);

NTOSAPI
NTSTATUS
NTAPI
ZwPrivilegeObjectAuditAlarm(
	IN PUNICODE_STRING  SubsystemName,
	IN PVOID  HandleId,
	IN HANDLE  TokenHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN PPRIVILEGE_SET  Privileges,
	IN BOOLEAN  AccessGranted);

NTOSAPI
NTSTATUS
NTAPI
ZwAccessCheck(
	IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
	IN HANDLE  TokenHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN PGENERIC_MAPPING  GenericMapping,
	IN PPRIVILEGE_SET  PrivilegeSet,
	IN PULONG  PrivilegeSetLength,
	OUT PACCESS_MASK  GrantedAccess,
	OUT PBOOLEAN  AccessStatus);

NTOSAPI
NTSTATUS
NTAPI
ZwAccessCheckAndAuditAlarm(
	IN PUNICODE_STRING  SubsystemName,
	IN PVOID  HandleId,
	IN PUNICODE_STRING  ObjectTypeName,
	IN PUNICODE_STRING  ObjectName,
	IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
	IN ACCESS_MASK  DesiredAccess,
	IN PGENERIC_MAPPING  GenericMapping,
	IN BOOLEAN  ObjectCreation,
	OUT PACCESS_MASK  GrantedAccess,
	OUT PBOOLEAN  AccessStatus,
	OUT PBOOLEAN  GenerateOnClose);

NTOSAPI
NTSTATUS
NTAPI
ZwAccessCheckByType(
	IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
	IN PSID  PrincipalSelfSid,
	IN HANDLE  TokenHandle,
	IN ULONG  DesiredAccess,
	IN POBJECT_TYPE_LIST  ObjectTypeList,
	IN ULONG  ObjectTypeListLength,
	IN PGENERIC_MAPPING  GenericMapping,
	IN PPRIVILEGE_SET  PrivilegeSet,
	IN PULONG  PrivilegeSetLength,
	OUT PACCESS_MASK  GrantedAccess,
	OUT PULONG  AccessStatus);

typedef enum _AUDIT_EVENT_TYPE {
	AuditEventObjectAccess,
	AuditEventDirectoryServiceAccess
} AUDIT_EVENT_TYPE, *PAUDIT_EVENT_TYPE;

NTOSAPI
NTSTATUS
NTAPI
ZwAccessCheckByTypeAndAuditAlarm(
	IN PUNICODE_STRING  SubsystemName,
	IN PVOID  HandleId,
	IN PUNICODE_STRING  ObjectTypeName,
	IN PUNICODE_STRING  ObjectName,
	IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
	IN PSID  PrincipalSelfSid,
	IN ACCESS_MASK  DesiredAccess,
	IN AUDIT_EVENT_TYPE  AuditType,
	IN ULONG  Flags,
	IN POBJECT_TYPE_LIST  ObjectTypeList,
	IN ULONG  ObjectTypeListLength,
	IN PGENERIC_MAPPING  GenericMapping,
	IN BOOLEAN  ObjectCreation,
	OUT PACCESS_MASK  GrantedAccess,
	OUT PULONG  AccessStatus,
	OUT PBOOLEAN  GenerateOnClose);

NTOSAPI
NTSTATUS
NTAPI
ZwAccessCheckByTypeResultList(
	IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
	IN PSID  PrincipalSelfSid,
	IN HANDLE  TokenHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN POBJECT_TYPE_LIST  ObjectTypeList,
	IN ULONG  ObjectTypeListLength,
	IN PGENERIC_MAPPING  GenericMapping,
	IN PPRIVILEGE_SET  PrivilegeSet,
	IN PULONG  PrivilegeSetLength,
	OUT PACCESS_MASK  GrantedAccessList,
	OUT PULONG  AccessStatusList);

NTOSAPI
NTSTATUS
NTAPI
ZwAccessCheckByTypeResultListAndAuditAlarm(
	IN PUNICODE_STRING  SubsystemName,
	IN PVOID  HandleId,
	IN PUNICODE_STRING  ObjectTypeName,
	IN PUNICODE_STRING  ObjectName,
	IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
	IN PSID  PrincipalSelfSid,
	IN ACCESS_MASK  DesiredAccess,
	IN AUDIT_EVENT_TYPE  AuditType,
	IN ULONG  Flags,
	IN POBJECT_TYPE_LIST  ObjectTypeList,
	IN ULONG  ObjectTypeListLength,
	IN PGENERIC_MAPPING  GenericMapping,
	IN BOOLEAN  ObjectCreation,
	OUT PACCESS_MASK  GrantedAccessList,
	OUT PULONG  AccessStatusList,
	OUT PULONG  GenerateOnClose);

NTOSAPI
NTSTATUS
NTAPI
ZwAccessCheckByTypeResultListAndAuditAlarmByHandle(
	IN PUNICODE_STRING  SubsystemName,
	IN PVOID  HandleId,
	IN HANDLE  TokenHandle,
	IN PUNICODE_STRING  ObjectTypeName,
	IN PUNICODE_STRING  ObjectName,
	IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
	IN PSID  PrincipalSelfSid,
	IN ACCESS_MASK  DesiredAccess,
	IN AUDIT_EVENT_TYPE  AuditType,
	IN ULONG  Flags,
	IN POBJECT_TYPE_LIST  ObjectTypeList,
	IN ULONG  ObjectTypeListLength,
	IN PGENERIC_MAPPING  GenericMapping,
	IN BOOLEAN  ObjectCreation,
	OUT PACCESS_MASK  GrantedAccessList,
	OUT PULONG  AccessStatusList,
	OUT PULONG  GenerateOnClose);

NTOSAPI
NTSTATUS
NTAPI
ZwOpenObjectAuditAlarm(
	IN PUNICODE_STRING  SubsystemName,
	IN PVOID  *HandleId,
	IN PUNICODE_STRING  ObjectTypeName,
	IN PUNICODE_STRING  ObjectName,
	IN PSECURITY_DESCRIPTOR  SecurityDescriptor,
	IN HANDLE  TokenHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN ACCESS_MASK  GrantedAccess,
	IN PPRIVILEGE_SET  Privileges  OPTIONAL,
	IN BOOLEAN  ObjectCreation,
	IN BOOLEAN  AccessGranted,
	OUT PBOOLEAN  GenerateOnClose);

NTOSAPI
NTSTATUS
NTAPI
ZwCloseObjectAuditAlarm(
  IN PUNICODE_STRING  SubsystemName,
  IN PVOID  HandleId,
  IN BOOLEAN  GenerateOnClose);

NTOSAPI
NTSTATUS
NTAPI
ZwDeleteObjectAuditAlarm(
  IN PUNICODE_STRING  SubsystemName,
  IN PVOID  HandleId,
  IN BOOLEAN  GenerateOnClose);




/* Plug and play and power management */

NTOSAPI
NTSTATUS
NTAPI
ZwRequestWakeupLatency(
  IN LATENCY_TIME  Latency);

NTOSAPI
NTSTATUS
NTAPI
ZwRequestDeviceWakeup(
  IN HANDLE  DeviceHandle);

NTOSAPI
NTSTATUS
NTAPI
ZwCancelDeviceWakeupRequest(
  IN HANDLE  DeviceHandle);

NTOSAPI
BOOLEAN
NTAPI
ZwIsSystemResumeAutomatic(
  VOID);

NTOSAPI
NTSTATUS
NTAPI
ZwSetThreadExecutionState(
	IN EXECUTION_STATE  ExecutionState,
	OUT PEXECUTION_STATE  PreviousExecutionState);

NTOSAPI
NTSTATUS
NTAPI
ZwGetDevicePowerState(
  IN HANDLE  DeviceHandle,
  OUT PDEVICE_POWER_STATE  DevicePowerState);

NTOSAPI
NTSTATUS
NTAPI
ZwSetSystemPowerState(
	IN POWER_ACTION  SystemAction,
	IN SYSTEM_POWER_STATE  MinSystemState,
	IN ULONG  Flags);

NTOSAPI
NTSTATUS
NTAPI
ZwInitiatePowerAction(
	IN POWER_ACTION  SystemAction,
	IN SYSTEM_POWER_STATE  MinSystemState,
	IN ULONG  Flags,
	IN BOOLEAN  Asynchronous);

NTOSAPI
NTSTATUS
NTAPI
ZwPowerInformation(
	IN POWER_INFORMATION_LEVEL  PowerInformationLevel,
	IN PVOID  InputBuffer  OPTIONAL,
	IN ULONG  InputBufferLength,
	OUT PVOID  OutputBuffer  OPTIONAL,
	IN ULONG  OutputBufferLength);

NTOSAPI
NTSTATUS
NTAPI
ZwPlugPlayControl(
  IN ULONG  ControlCode,
  IN OUT PVOID  Buffer,
  IN ULONG  BufferLength);

NTOSAPI
NTSTATUS
NTAPI
ZwGetPlugPlayEvent(
	IN ULONG  Reserved1,
	IN ULONG  Reserved2,
	OUT PVOID  Buffer,
	IN ULONG  BufferLength);




/* Miscellany */

NTOSAPI
NTSTATUS
NTAPI
ZwRaiseException(
  IN PEXCEPTION_RECORD  ExceptionRecord,
  IN PCONTEXT  Context,
  IN BOOLEAN  SearchFrames);

NTOSAPI
NTSTATUS
NTAPI
ZwContinue(
  IN PCONTEXT  Context,
  IN BOOLEAN  TestAlert);

NTOSAPI
NTSTATUS
NTAPI
ZwW32Call(
	IN ULONG  RoutineIndex,
	IN PVOID  Argument,
	IN ULONG  ArgumentLength,
	OUT PVOID  *Result  OPTIONAL,
	OUT PULONG  ResultLength  OPTIONAL);

NTOSAPI
NTSTATUS
NTAPI
ZwSetLowWaitHighThread(
  VOID);

NTOSAPI
NTSTATUS
NTAPI
ZwSetHighWaitLowThread(
  VOID);

NTOSAPI
NTSTATUS
NTAPI
ZwLoadDriver(
  IN PUNICODE_STRING  DriverServiceName);

NTOSAPI
NTSTATUS
NTAPI
ZwUnloadDriver(
  IN PUNICODE_STRING  DriverServiceName);

NTOSAPI
NTSTATUS
NTAPI
ZwFlushInstructionCache(
	IN HANDLE  ProcessHandle,
	IN PVOID  BaseAddress  OPTIONAL,
	IN ULONG  FlushSize);

NTOSAPI
NTSTATUS
NTAPI
ZwFlushWriteBuffer(
  VOID);

NTOSAPI
NTSTATUS
NTAPI
ZwQueryDefaultLocale(
	IN BOOLEAN  ThreadOrSystem,
	OUT PLCID  Locale);

NTOSAPI
NTSTATUS
NTAPI
ZwSetDefaultLocale(
  IN BOOLEAN  ThreadOrSystem,
  IN LCID  Locale);

NTOSAPI
NTSTATUS
NTAPI
ZwQueryDefaultUILanguage(
  OUT PLANGID  LanguageId);

NTOSAPI
NTSTATUS
NTAPI
ZwSetDefaultUILanguage(
  IN LANGID  LanguageId);

NTOSAPI
NTSTATUS
NTAPI
ZwQueryInstallUILanguage(
  OUT PLANGID  LanguageId);

NTOSAPI
NTSTATUS
NTAPI
NtAllocateLocallyUniqueId(
  OUT PLUID  Luid);

NTOSAPI
NTSTATUS
NTAPI
NtAllocateUuids(
  OUT PLARGE_INTEGER  UuidLastTimeAllocated,
  OUT PULONG  UuidDeltaTime,
  OUT PULONG  UuidSequenceNumber,
  OUT PUCHAR  UuidSeed);

NTOSAPI
NTSTATUS
NTAPI
ZwSetUuidSeed(
  IN PUCHAR  UuidSeed);

typedef enum _HARDERROR_RESPONSE_OPTION {
	OptionAbortRetryIgnore,
	OptionOk,
	OptionOkCancel,
	OptionRetryCancel,
	OptionYesNo,
	OptionYesNoCancel,
	OptionShutdownSystem
} HARDERROR_RESPONSE_OPTION, *PHARDERROR_RESPONSE_OPTION;

typedef enum _HARDERROR_RESPONSE {
	ResponseReturnToCaller,
	ResponseNotHandled,
	ResponseAbort,
	ResponseCancel,
	ResponseIgnore,
	ResponseNo,
	ResponseOk,
	ResponseRetry,
	ResponseYes
} HARDERROR_RESPONSE, *PHARDERROR_RESPONSE;

NTOSAPI
NTSTATUS
NTAPI
ZwRaiseHardError(
	IN NTSTATUS  Status,
	IN ULONG  NumberOfArguments,
	IN ULONG  StringArgumentsMask,
	IN PULONG  Arguments,
	IN HARDERROR_RESPONSE_OPTION  ResponseOption,
	OUT PHARDERROR_RESPONSE  Response);

NTOSAPI
NTSTATUS
NTAPI
ZwSetDefaultHardErrorPort(
  IN HANDLE  PortHandle);

NTOSAPI
NTSTATUS
NTAPI
ZwDisplayString(
  IN PUNICODE_STRING  String);

NTOSAPI
NTSTATUS
NTAPI
ZwCreatePagingFile(
  IN PUNICODE_STRING  FileName,
  IN PULARGE_INTEGER  InitialSize,
  IN PULARGE_INTEGER  MaximumSize,
  IN ULONG  Reserved);

typedef USHORT RTL_ATOM, *PRTL_ATOM;

NTOSAPI
NTSTATUS
NTAPI
NtAddAtom(
  IN PWSTR  AtomName,
  IN ULONG  AtomNameLength,
  OUT PRTL_ATOM  Atom);

NTOSAPI
NTSTATUS
NTAPI
NtFindAtom(
  IN PWSTR  AtomName,
  IN ULONG  AtomNameLength,
  OUT PRTL_ATOM  Atom);

NTOSAPI
NTSTATUS
NTAPI
NtDeleteAtom(
  IN RTL_ATOM  Atom);

typedef enum _ATOM_INFORMATION_CLASS {
	AtomBasicInformation,
	AtomListInformation
} ATOM_INFORMATION_CLASS;

NTOSAPI
NTSTATUS
NTAPI
NtQueryInformationAtom(
  IN RTL_ATOM  Atom,
  IN ATOM_INFORMATION_CLASS  AtomInformationClass,
  OUT PVOID  AtomInformation,
  IN ULONG  AtomInformationLength,
  OUT PULONG  ReturnLength  OPTIONAL);

typedef struct _ATOM_BASIC_INFORMATION {
	USHORT  ReferenceCount;
	USHORT  Pinned;
	USHORT  NameLength;
	WCHAR  Name[1];
} ATOM_BASIC_INFORMATION, *PATOM_BASIC_INFORMATION;

typedef struct _ATOM_LIST_INFORMATION {
  ULONG  NumberOfAtoms;
  ATOM  Atoms[1];
} ATOM_LIST_INFORMATION, *PATOM_LIST_INFORMATION;

NTOSAPI
NTSTATUS
NTAPI
ZwSetLdtEntries(
	IN ULONG  Selector1,
	IN LDT_ENTRY  LdtEntry1,
	IN ULONG  Selector2,
	IN LDT_ENTRY  LdtEntry2);

NTOSAPI
NTSTATUS
NTAPI
NtVdmControl(
  IN ULONG  ControlCode,
  IN PVOID  ControlData);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* __NTAPI_H */
