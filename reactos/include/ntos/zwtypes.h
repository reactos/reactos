#ifndef __INCLUDE_DDK_ZWTYPES_H
#define __INCLUDE_DDK_ZWTYPES_H

#ifndef __USE_W32API

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

typedef enum _DEBUG_CONTROL_CODE
{
  DebugGetTraceInformation = 1,
  DebugSetInternalBreakpoint,
  DebugSetSpecialCalls,
  DebugClearSpecialCalls,
  DebugQuerySpecialCalls,
  DebugDbgBreakPoint,
  DebugDbgLoadSymbols
} DEBUG_CONTROL_CODE;

typedef enum _KPROFILE_SOURCE
{
  ProfileTime
} KPROFILE_SOURCE;

// file disposition values

#define FILE_SUPERSEDE                  0x0000
#define FILE_OPEN                       0x0001
#define FILE_CREATE                     0x0002
#define FILE_OPEN_IF                    0x0003
#define FILE_OVERWRITE                  0x0004
#define FILE_OVERWRITE_IF               0x0005
#define FILE_MAXIMUM_DISPOSITION        0x0005

// job query / set information class

typedef enum _JOBOBJECTINFOCLASS {               // Q S
    JobObjectBasicAccountingInformation = 1,     // Y N
    JobObjectBasicLimitInformation,              // Y Y
    JobObjectBasicProcessIdList,                 // Y N
    JobObjectBasicUIRestrictions,                // Y Y
    JobObjectSecurityLimitInformation,           // Y Y
    JobObjectEndOfJobTimeInformation,            // N Y
    JobObjectAssociateCompletionPortInformation, // N Y
    JobObjectBasicAndIoAccountingInformation,    // Y N
    JobObjectExtendedLimitInformation,           // Y Y
} JOBOBJECTINFOCLASS;

// system information
// {Nt|Zw}{Query|Set}SystemInformation
// (GN means Gary Nebbet in "NT/W2K Native API Reference")

typedef
enum _SYSTEM_INFORMATION_CLASS
{
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
	
	SystemPowerInformation			= 42,
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
typedef
struct _SYSTEM_BASIC_INFORMATION
{
	ULONG		Reserved;
	ULONG		TimerResolution;
	ULONG		PageSize;
	ULONG		NumberOfPhysicalPages;
	ULONG		LowestPhysicalPageNumber;
	ULONG		HighestPhysicalPageNumber;
	ULONG		AllocationGranularity;
	ULONG		MinimumUserModeAddress;
	ULONG		MaximumUserModeAddress;
	KAFFINITY	ActiveProcessorsAffinityMask;
	CCHAR		NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

// SystemProcessorInformation (1)
typedef
struct _SYSTEM_PROCESSOR_INFORMATION
{
	USHORT	ProcessorArchitecture;
	USHORT	ProcessorLevel;
	USHORT	ProcessorRevision;
	USHORT	Reserved;
	ULONG	ProcessorFeatureBits;
} SYSTEM_PROCESSOR_INFORMATION, *PSYSTEM_PROCESSOR_INFORMATION;

// SystemPerformanceInfo (2)
typedef
struct _SYSTEM_PERFORMANCE_INFORMATION
{
	LARGE_INTEGER	IdleProcessorTime;
	LARGE_INTEGER	IoReadTransferCount;
	LARGE_INTEGER	IoWriteTransferCount;
	LARGE_INTEGER	IoOtherTransferCount;
	ULONG		IoReadOperationCount;
	ULONG		IoWriteOperationCount;
	ULONG		IoOtherOperationCount;
	ULONG		AvailablePages;
	ULONG		CommitedPages;
	ULONG		CommitLimit;
	ULONG		PeakCommitment;
	ULONG		PageFaultCount;
	ULONG		CopyOnWriteCount;
	ULONG		TransitionCount;
	ULONG		CacheTransitionCount;
	ULONG		DemandZeroCount;
	ULONG		PageReadCount;
	ULONG		PageReadIoCount;
	ULONG		CacheReadCount;
	ULONG		CacheIoCount;
	ULONG		DirtyPagesWriteCount;
	ULONG		DirtyWriteIoCount;
	ULONG		MappedPagesWriteCount;
	ULONG		MappedWriteIoCount;
	ULONG		PagedPoolPages;
	ULONG		NonPagedPoolPages;
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

} SYSTEM_PERFORMANCE_INFO, *PSYSTEM_PERFORMANCE_INFO;

// SystemModuleInformation (11)
typedef
struct _SYSTEM_MODULE_INFORMATION
{
 ULONG Reserved[2];
 PVOID Base;
 SIZE_T Size;
 ULONG Flags;
 USHORT Index;
 USHORT Unknown;
 USHORT LoadCount;
 USHORT ModuleNameOffset;
 CHAR ImageName[256];
#if 0
	ULONG	Unknown1;
	ULONG	Unknown2;
	PVOID	BaseAddress;
	ULONG	Size;
	ULONG	Flags;
	ULONG	EntryIndex;
	USHORT	NameLength; /* Length of module name not including the path, this field contains valid value only for NTOSKRNL module*/
	USHORT	PathLength; /* Length of 'directory path' part of modulename*/
	CHAR	Name [256];
#endif
} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;

typedef
struct _SYSTEM_MODULES
{
 SIZE_T Count;
 SYSTEM_MODULE_INFORMATION Modules[ANYSIZE_ARRAY];
} SYSTEM_MODULES, *PSYSTEM_MODULES;

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

// SystemObjectInformation (17)
typedef
struct _SYSTEM_OBJECT_TYPE_INFORMATION
{
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

typedef
struct _SYSTEM_OBJECT_INFORMATION
{
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
typedef
struct _SYSTEM_PAGEFILE_INFORMATION
{
	ULONG		RelativeOffset;
	ULONG		CurrentSizePages;
	ULONG		TotalUsedPages;
	ULONG		PeakUsedPages;
	UNICODE_STRING	PagefileFileName;
	
} SYSTEM_PAGEFILE_INFORMATION, *PSYSTEM_PAGEFILE_INFORMATION;

// SystemCacheInformation (21)
typedef
struct _SYSTEM_CACHE_INFORMATION
{
	ULONG	CurrentSize;
	ULONG	PeakSize;
	ULONG	PageFaultCount;
	ULONG	MinimumWorkingSet;
	ULONG	MaximumWorkingSet;
	ULONG	Unused[4];

} SYSTEM_CACHE_INFORMATION;

// SystemDpcInformation (24)
typedef
struct _SYSTEM_DPC_INFORMATION
{
	ULONG	Unused;
	ULONG	KiMaximumDpcQueueDepth;
	ULONG	KiMinimumDpcRate;
	ULONG	KiAdjustDpcThreshold;
	ULONG	KiIdealDpcRate;

} SYSTEM_DPC_INFORMATION, *PSYSTEM_DPC_INFORMATION;

// SystemLoadImage (26)
typedef struct _SYSTEM_LOAD_IMAGE
{
  UNICODE_STRING ModuleName;
  PVOID ModuleBase;
  PVOID SectionPointer;
  PVOID EntryPoint;
  PVOID ExportDirectory;
} SYSTEM_LOAD_IMAGE, *PSYSTEM_LOAD_IMAGE;

// SystemUnloadImage (27)
typedef struct _SYSTEM_UNLOAD_IMAGE
{
  PVOID ModuleBase;
} SYSTEM_UNLOAD_IMAGE, *PSYSTEM_UNLOAD_IMAGE;

// SystemTimeAdjustmentInformation (28)
typedef
struct _SYSTEM_QUERY_TIME_ADJUSTMENT
{
	ULONG	TimeAdjustment;
	ULONG	MaximumIncrement;
	BOOLEAN	TimeSynchronization;

} SYSTEM_QUERY_TIME_ADJUSTMENT, *PSYSTEM_QUERY_TIME_ADJUSTMENT;

typedef
struct _SYSTEM_SET_TIME_ADJUSTMENT
{
	ULONG	TimeAdjustment;
	BOOLEAN	TimeSynchronization;
	
} SYSTEM_TIME_ADJUSTMENT_INFO, *PSYSTEM_TIME_ADJUSTMENT_INFO;

// atom information

typedef enum _ATOM_INFORMATION_CLASS
{
   AtomBasicInformation		= 0,
   AtomTableInformation		= 1,
} ATOM_INFORMATION_CLASS;

typedef struct _ATOM_BASIC_INFORMATION
{
   USHORT UsageCount;
   USHORT Flags;
   USHORT NameLength;
   WCHAR Name[1];
} ATOM_BASIC_INFORMATION, *PATOM_BASIC_INFORMATION;

// SystemLoadAndCallImage(38)
typedef struct _SYSTEM_LOAD_AND_CALL_IMAGE
{
  UNICODE_STRING ModuleName;
} SYSTEM_LOAD_AND_CALL_IMAGE, *PSYSTEM_LOAD_AND_CALL_IMAGE;

// SystemTimeZoneInformation (44)
typedef
struct _SYSTEM_TIME_ZONE_INFORMATION
{
	LONG	Bias;
	WCHAR	StandardName [32];
	TIME	StandardDate;
	LONG	StandardBias;
	WCHAR	DaylightName [32];
	TIME	DaylightDate;
	LONG	DaylightBias;

} SYSTEM_TIME_ZONE_INFORMATION, * PSYSTEM_TIME_ZONE_INFORMATION;

// SystemLookasideInformation (45)
typedef
struct _SYSTEM_LOOKASIDE_INFORMATION
{
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
typedef
struct _SYSTEM_SET_TIME_SLIP_EVENT
{
	HANDLE	TimeSlipEvent; /* IN */

} SYSTEM_SET_TIME_SLIP_EVENT, * PSYSTEM_SET_TIME_SLIP_EVENT;

// SystemCreateSession (47)
// (available only on TSE/NT5+)
typedef
struct _SYSTEM_CREATE_SESSION
{
	ULONG	SessionId; /* OUT */

} SYSTEM_CREATE_SESSION, * PSYSTEM_CREATE_SESSION;

// SystemDeleteSession (48)
// (available only on TSE/NT5+)
typedef
struct _SYSTEM_DELETE_SESSION
{
	ULONG	SessionId; /* IN */

} SYSTEM_DELETE_SESSION, * PSYSTEM_DELETE_SESSION;

// SystemRangeStartInformation (50)
typedef
struct _SYSTEM_RANGE_START_INFORMATION
{
	PVOID	SystemRangeStart;

} SYSTEM_RANGE_START_INFORMATION, * PSYSTEM_RANGE_START_INFORMATION;

// SystemSessionProcessesInformation (53)
// (available only on TSE/NT5+)
typedef
struct _SYSTEM_SESSION_PROCESSES_INFORMATION
{
	ULONG	SessionId;
	ULONG	BufferSize;
	PVOID	Buffer; /* same format as in SystemProcessInformation */

} SYSTEM_SESSION_PROCESSES_INFORMATION, * PSYSTEM_SESSION_PROCESSES_INFORMATION;

// memory information

typedef enum _MEMORY_INFORMATION_CLASS {
 MemoryBasicInformation,
 MemoryWorkingSetList,
 MemorySectionName //,
 //MemoryBasicVlmInformation //???
} MEMORY_INFORMATION_CLASS;

typedef struct _MEMORY_BASIC_INFORMATION { // Information Class 0
 PVOID BaseAddress;
 PVOID AllocationBase;
 ULONG AllocationProtect;
 ULONG RegionSize;
 ULONG State;
 ULONG Protect;
 ULONG Type;
} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;

typedef struct _MEMORY_WORKING_SET_LIST { // Information Class 1
 ULONG NumberOfPages;
 ULONG WorkingSetList[1];
} MEMORY_WORKING_SET_LIST, *PMEMORY_WORKING_SET_LIST;

// Information Class 2
#define _MEMORY_SECTION_NAME_STATIC(__bufsize__) \
 { \
 UNICODE_STRING SectionFileName; \
 WCHAR          NameBuffer[(__bufsize__)]; \
} 

#define MEMORY_SECTION_NAME_STATIC(__bufsize__) \
 struct _MEMORY_SECTION_NAME_STATIC((__bufsize__) 

typedef struct _MEMORY_SECTION_NAME_STATIC(ANYSIZE_ARRAY)
 MEMORY_SECTION_NAME, *PMEMORY_SECTION_NAME;

// Information class 0
typedef struct _PROCESS_BASIC_INFORMATION
{
	NTSTATUS ExitStatus;
	PPEB PebBaseAddress;
	KAFFINITY AffinityMask;
	KPRIORITY BasePriority;
	ULONG UniqueProcessId;
	ULONG InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

// Information class 1
typedef struct _QUOTA_LIMITS
{
	ULONG PagedPoolLimit;
	ULONG NonPagedPoolLimit;
	ULONG MinimumWorkingSetSize;
	ULONG MaximumWorkingSetSize;
	ULONG PagefileLimit;
	TIME TimeLimit;
} QUOTA_LIMITS, *PQUOTA_LIMITS;

// Information class 2
typedef struct _IO_COUNTERS
{
	LARGE_INTEGER ReadOperationCount;
	LARGE_INTEGER WriteOperationCount;
	LARGE_INTEGER OtherOperationCount;
	LARGE_INTEGER ReadTransferCount;
	LARGE_INTEGER WriteTransferCount;
	LARGE_INTEGER OtherTransferCount;
} IO_COUNTERS, *PIO_COUNTERS;

// Information class 3
typedef struct _VM_COUNTERS_
{
	ULONG PeakVirtualSize;
	ULONG VirtualSize;
	ULONG PageFaultCount;
	ULONG PeakWorkingSetSize;
	ULONG WorkingSetSize;
	ULONG QuotaPeakPagedPoolUsage;
	ULONG QuotaPagedPoolUsage;
	ULONG QuotaPeakNonPagedPoolUsage;
	ULONG QuotaNonPagedPoolUsage;
	ULONG PagefileUsage;
	ULONG PeakPagefileUsage;
} VM_COUNTERS, *PVM_COUNTERS;

// Information class 4
typedef struct _KERNEL_USER_TIMES
{
	TIME CreateTime;
	TIME ExitTime;
	TIME KernelTime;
	TIME UserTime;
} KERNEL_USER_TIMES, *PKERNEL_USER_TIMES;

// Information class 9
typedef struct _PROCESS_ACCESS_TOKEN
{
	HANDLE Token;
	HANDLE Thread;
} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

// Information class 14 
typedef struct _POOLED_USAGE_AND_LIMITS_
{
	ULONG PeakPagedPoolUsage;
	ULONG PagedPoolUsage;
	ULONG PagedPoolLimit;
	ULONG PeakNonPagedPoolUsage;
	ULONG NonPagedPoolUsage;
	ULONG NonPagedPoolLimit;
	ULONG PeakPagefileUsage;
	ULONG PagefileUsage;
	ULONG PagefileLimit;
} POOLED_USAGE_AND_LIMITS, *PPOOLED_USAGE_AND_LIMITS;

// Information class 15
typedef struct _PROCESS_WS_WATCH_INFORMATION
{
	PVOID FaultingPc;
	PVOID FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

// Information class 18
typedef struct _PROCESS_PRIORITY_CLASS
{
	BOOLEAN Foreground;
	UCHAR   PriorityClass;
} PROCESS_PRIORITY_CLASS, *PPROCESS_PRIORITY_CLASS;

// Information class 23
typedef struct _PROCESS_DEVICEMAP_INFORMATION
{
	union {
		struct {
			HANDLE DirectoryHandle;
		} Set;
		struct {
			ULONG DriveMap;
			UCHAR DriveType[32];
		} Query;
	};
} PROCESS_DEVICEMAP_INFORMATION, *pPROCESS_DEVICEMAP_INFORMATION;

// Information class 24
typedef struct _PROCESS_SESSION_INFORMATION
{
	ULONG SessionId;
} PROCESS_SESSION_INFORMATION, *PPROCESS_SESSION_INFORMATION;

// thread information

// incompatible with MS NT

typedef struct _THREAD_BASIC_INFORMATION
{
  NTSTATUS  ExitStatus;
  PVOID     TebBaseAddress;	// PNT_TIB (GN)
  CLIENT_ID ClientId;
  KAFFINITY AffinityMask;
  KPRIORITY Priority;
  KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

// object information

typedef struct _OBJECT_NAME_INFORMATION
{
	UNICODE_STRING	Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

// file information

typedef struct _FILE_BASIC_INFORMATION
{
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION
{
	LARGE_INTEGER AllocationSize;
	LARGE_INTEGER EndOfFile;
	ULONG NumberOfLinks;
	BOOLEAN DeletePending;
	BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_POSITION_INFORMATION
{
	LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_ALIGNMENT_INFORMATION
{
	ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION
{
	BOOLEAN DoDeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef struct _FILE_END_OF_FILE_INFORMATION
{
	LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;

typedef struct _FILE_NETWORK_OPEN_INFORMATION
{
	TIME CreationTime;
	TIME LastAccessTime;
	TIME LastWriteTime;
	TIME ChangeTime;
	LARGE_INTEGER AllocationSize;
	LARGE_INTEGER EndOfFile;
	ULONG FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

typedef struct _FILE_FULL_EA_INFORMATION
{
	ULONG NextEntryOffset;
	UCHAR Flags;
	UCHAR EaNameLength;
	USHORT EaValueLength;
	CHAR  EaName[0];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;


typedef struct _FILE_EA_INFORMATION {
	ULONG EaSize;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;


typedef struct _FILE_GET_EA_INFORMATION {
	ULONG NextEntryOffset;
	UCHAR EaNameLength;
	CHAR EaName[0];
} FILE_GET_EA_INFORMATION, *PFILE_GET_EA_INFORMATION;

typedef struct _FILE_STREAM_INFORMATION {
	ULONG NextEntryOffset;
	ULONG StreamNameLength;
	LARGE_INTEGER StreamSize;
	LARGE_INTEGER StreamAllocationSize;
	WCHAR StreamName[0];
} FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;

typedef struct _FILE_ALLOCATION_INFORMATION {
	LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFORMATION, *PFILE_ALLOCATION_INFORMATION;

typedef struct _FILE_NAME_INFORMATION {
	ULONG FileNameLength;
	WCHAR FileName[0];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION 
{
	ULONG NextEntryOffset;
	ULONG FileIndex;
	ULONG FileNameLength;
	WCHAR FileName[0];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;


typedef struct _FILE_RENAME_INFORMATION {
	BOOLEAN Replace;
	HANDLE RootDir;
	ULONG FileNameLength;
	WCHAR FileName[0];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;


typedef struct _FILE_INTERNAL_INFORMATION {
	LARGE_INTEGER IndexNumber;
} FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

typedef struct _FILE_ACCESS_INFORMATION {
	ACCESS_MASK AccessFlags;
} FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;


typedef struct _FILE_MODE_INFORMATION {
	ULONG Mode;
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;


typedef struct _FILE_PIPE_INFORMATION {
	ULONG ReadMode;
	ULONG CompletionMode;
} FILE_PIPE_INFORMATION, *PFILE_PIPE_INFORMATION;

typedef struct _FILE_PIPE_LOCAL_INFORMATION {
	ULONG NamedPipeType;
	ULONG NamedPipeConfiguration;
	ULONG MaximumInstances;
	ULONG CurrentInstances;
	ULONG InboundQuota;
	ULONG ReadDataAvailable;
	ULONG OutboundQuota;
	ULONG WriteQuotaAvailable;
	ULONG NamedPipeState;
	ULONG NamedPipeEnd;
} FILE_PIPE_LOCAL_INFORMATION, *PFILE_PIPE_LOCAL_INFORMATION;

typedef struct _FILE_PIPE_REMOTE_INFORMATION {
	LARGE_INTEGER CollectDataTime;
	ULONG MaximumCollectionCount;
} FILE_PIPE_REMOTE_INFORMATION, *PFILE_PIPE_REMOTE_INFORMATION;

typedef struct _FILE_MAILSLOT_QUERY_INFORMATION {
	ULONG MaxMessageSize;
	ULONG Unknown; /* ?? */
	ULONG NextSize;
	ULONG MessageCount;
	LARGE_INTEGER Timeout;
} FILE_MAILSLOT_QUERY_INFORMATION, *PFILE_MAILSLOT_QUERY_INFORMATION;

typedef struct _FILE_MAILSLOT_SET_INFORMATION {
	LARGE_INTEGER Timeout;
} FILE_MAILSLOT_SET_INFORMATION, *PFILE_MAILSLOT_SET_INFORMATION;

typedef struct _FILE_COMPRESSION_INFORMATION {
	LARGE_INTEGER CompressedFileSize;
	USHORT CompressionFormat;
	UCHAR CompressionUnitShift;
	UCHAR ChunkShift;
	UCHAR ClusterShift;
	UCHAR Reserved[3];
} FILE_COMPRESSION_INFORMATION, *PFILE_COMPRESSION_INFORMATION;

typedef struct _FILE_COMPLETION_INFORMATION { // Information Class 30
   HANDLE IoCompletionHandle;
   ULONG CompletionKey;
} FILE_COMPLETION_INFORMATION, *PFILE_COMPLETION_INFORMATION;

typedef struct _FILE_ALL_INFORMATION {
	FILE_BASIC_INFORMATION BasicInformation;
	FILE_STANDARD_INFORMATION StandardInformation;
	FILE_INTERNAL_INFORMATION InternalInformation;
	FILE_EA_INFORMATION EaInformation;
	FILE_ACCESS_INFORMATION AccessInformation;
	FILE_POSITION_INFORMATION PositionInformation;
	FILE_MODE_INFORMATION ModeInformation;
	FILE_ALIGNMENT_INFORMATION AlignmentInformation;
	FILE_NAME_INFORMATION NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;


// file system information structures

typedef struct _FILE_FS_DEVICE_INFORMATION {
	DEVICE_TYPE DeviceType;
	ULONG Characteristics;
} FILE_FS_DEVICE_INFORMATION,  *PFILE_FS_DEVICE_INFORMATION;


typedef struct _FILE_FS_VOLUME_INFORMATION {
	TIME VolumeCreationTime;
	ULONG VolumeSerialNumber;
	ULONG VolumeLabelLength;
	BOOLEAN SupportsObjects;
	WCHAR VolumeLabel[0];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

typedef struct _FILE_FS_SIZE_INFORMATION {
	LARGE_INTEGER TotalAllocationUnits;
	LARGE_INTEGER AvailableAllocationUnits;
	ULONG SectorsPerAllocationUnit;
	ULONG BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

typedef struct _FILE_FS_ATTRIBUTE_INFORMATION {
	ULONG FileSystemAttributes;
	LONG MaximumComponentNameLength;
	ULONG FileSystemNameLength;
	WCHAR FileSystemName[0];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;

/*
	FileSystemAttributes is one of the following values:

	FILE_CASE_SENSITIVE_SEARCH      0x00000001
        FILE_CASE_PRESERVED_NAMES       0x00000002
        FILE_UNICODE_ON_DISK            0x00000004
        FILE_PERSISTENT_ACLS            0x00000008
        FILE_FILE_COMPRESSION           0x00000010
        FILE_VOLUME_QUOTAS              0x00000020
        FILE_VOLUME_IS_COMPRESSED       0x00008000
*/
typedef struct _FILE_FS_LABEL_INFORMATION {
	ULONG VolumeLabelLength;
	WCHAR VolumeLabel[0];
} FILE_FS_LABEL_INFORMATION, *PFILE_FS_LABEL_INFORMATION;

// read file scatter / write file scatter
//FIXME I am a win32 struct aswell

typedef union _FILE_SEGMENT_ELEMENT {
	PVOID Buffer;
	ULONG Alignment;
}FILE_SEGMENT_ELEMENT, *PFILE_SEGMENT_ELEMENT;

typedef struct _FILE_DIRECTORY_INFORMATION {
	ULONG	NextEntryOffset;
	ULONG	FileIndex;
	TIME CreationTime;
	TIME LastAccessTime;
	TIME LastWriteTime;
	TIME ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG FileAttributes;
	ULONG FileNameLength;
	WCHAR FileName[0];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_FULL_DIRECTORY_INFORMATION {
	ULONG	NextEntryOffset;
	ULONG	FileIndex;
	TIME CreationTime;
	TIME LastAccessTime;
	TIME LastWriteTime;
	TIME ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG FileAttributes;
	ULONG FileNameLength;
	ULONG EaSize;
	WCHAR FileName[0]; // variable size
} FILE_FULL_DIRECTORY_INFORMATION, *PFILE_FULL_DIRECTORY_INFORMATION,
  FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;


typedef struct _FILE_BOTH_DIRECTORY_INFORMATION {
	ULONG		NextEntryOffset;
	ULONG		FileIndex;
	TIME 		CreationTime;
	TIME 		LastAccessTime;
	TIME 		LastWriteTime;
	TIME 		ChangeTime;
	LARGE_INTEGER 	EndOfFile;
	LARGE_INTEGER 	AllocationSize;
	ULONG 		FileAttributes;
	ULONG 		FileNameLength;
	ULONG 		EaSize;
	CHAR 		ShortNameLength;
	WCHAR 		ShortName[12]; // 8.3 name
	WCHAR 		FileName[0];
} FILE_BOTH_DIRECTORY_INFORMATION, *PFILE_BOTH_DIRECTORY_INFORMATION,
  FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

/*
	NotifyFilter / CompletionFilter:

	FILE_NOTIFY_CHANGE_FILE_NAME        0x00000001
	FILE_NOTIFY_CHANGE_DIR_NAME         0x00000002
	FILE_NOTIFY_CHANGE_NAME             0x00000003
	FILE_NOTIFY_CHANGE_ATTRIBUTES       0x00000004
	FILE_NOTIFY_CHANGE_SIZE             0x00000008
	FILE_NOTIFY_CHANGE_LAST_WRITE       0x00000010
	FILE_NOTIFY_CHANGE_LAST_ACCESS      0x00000020
	FILE_NOTIFY_CHANGE_CREATION         0x00000040
	FILE_NOTIFY_CHANGE_EA               0x00000080
	FILE_NOTIFY_CHANGE_SECURITY         0x00000100
	FILE_NOTIFY_CHANGE_STREAM_NAME      0x00000200
	FILE_NOTIFY_CHANGE_STREAM_SIZE      0x00000400
	FILE_NOTIFY_CHANGE_STREAM_WRITE     0x00000800
*/

typedef struct _FILE_NOTIFY_INFORMATION {
	ULONG Action;
	ULONG FileNameLength;
	WCHAR FileName[0]; 
} FILE_NOTIFY_INFORMATION;

#define FSCTL_GET_VOLUME_BITMAP			0x9006F
#define FSCTL_GET_RETRIEVAL_POINTERS		0x90073
#define FSCTL_MOVE_FILE				0x90074

typedef struct _MAPPING_PAIR
{
	ULONGLONG	Vcn;
	ULONGLONG	Lcn;
} MAPPING_PAIR, *PMAPPING_PAIR;

typedef struct _GET_RETRIEVAL_DESCRIPTOR
{
	ULONG		NumberOfPairs;
	ULONGLONG	StartVcn;
	MAPPING_PAIR	Pair[0]; // variable size 
} GET_RETRIEVAL_DESCRIPTOR, *PGET_RETRIEVAL_DESCRIPTOR;

typedef struct _MOVEFILE_DESCRIPTOR
{
	HANDLE            FileHandle;
	ULONG             Reserved;
	LARGE_INTEGER     StartVcn;
	LARGE_INTEGER     TargetLcn;
	ULONG             NumVcns;
	ULONG             Reserved1;
} MOVEFILE_DESCRIPTOR, *PMOVEFILE_DESCRIPTOR;

typedef struct _SECTION_BASIC_INFORMATION
{
  PVOID BaseAddress;
  ULONG Attributes;
  LARGE_INTEGER Size;
} SECTION_BASIC_INFORMATION, *PSECTION_BASIC_INFORMATION;

typedef enum _SECTION_INFORMATION_CLASS 
{
  SectionBasicInformation,
  SectionImageInformation,
} SECTION_INFORMATION_CLASS;

// shutdown action

typedef enum SHUTDOWN_ACTION_TAG {
  ShutdownNoReboot,
  ShutdownReboot,
  ShutdownPowerOff
} SHUTDOWN_ACTION;

typedef enum _IO_COMPLETION_INFORMATION_CLASS {
   IoCompletionBasicInformation
} IO_COMPLETION_INFORMATION_CLASS;

typedef struct _IO_COMPLETION_BASIC_INFORMATION {
   LONG SignalState;
} IO_COMPLETION_BASIC_INFORMATION, *PIO_COMPLETION_BASIC_INFORMATION;

#else /* __USE_W32API */

#define DebugDbgLoadSymbols ((DEBUG_CONTROL_CODE)0xffffffff)

#endif /* __USE_W32API */

#define NtCurrentProcess() ( (HANDLE) 0xFFFFFFFF )
#define NtCurrentThread() ( (HANDLE) 0xFFFFFFFE )
#if 1
extern ULONG NtBuildNumber;
#else
#ifdef __NTOSKRNL__
extern ULONG NtBuildNumber;
#else
extern ULONG NtBuildNumber;
#endif
#endif

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

// object handle information

#define ObjectBasicInformation			0
#define ObjectNameInformation			1
#define ObjectTypeInformation			2
#define ObjectAllInformation			3
#define ObjectDataInformation			4

typedef struct _ATOM_TABLE_INFORMATION
{
   ULONG NumberOfAtoms;
   RTL_ATOM Atoms[1];
} ATOM_TABLE_INFORMATION, *PATOM_TABLE_INFORMATION;


// mutant information

typedef enum _MUTANT_INFORMATION_CLASS
{
  MutantBasicInformation = 0
} MUTANT_INFORMATION_CLASS;

typedef struct _MUTANT_BASIC_INFORMATION
{
  LONG Count;
  BOOLEAN Owned;
  BOOLEAN Abandoned;
} MUTANT_BASIC_INFORMATION, *PMUTANT_BASIC_INFORMATION;


// SystemTimeOfDayInformation (3)
typedef
struct _SYSTEM_TIMEOFDAY_INFORMATION
{
	LARGE_INTEGER	BootTime;
	LARGE_INTEGER	CurrentTime;
	LARGE_INTEGER	TimeZoneBias;
	ULONG		TimeZoneId;
	ULONG		Reserved;
} SYSTEM_TIMEOFDAY_INFORMATION, *PSYSTEM_TIMEOFDAY_INFORMATION;

// SystemPathInformation (4)
// IT DOES NOT WORK
typedef
struct _SYSTEM_PATH_INFORMATION
{
	PVOID	Dummy;

} SYSTEM_PATH_INFORMATION, * PSYSTEM_PATH_INFORMATION;

// SystemProcessInformation (5)

#ifndef __USE_W32API

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

#endif /* __USE_W32API */

typedef struct _SYSTEM_PROCESSES_NT4
{
 SIZE_T         NextEntryDelta;
 ULONG          ThreadCount;
 ULONG          Reserved1[6];
 TIME           CreateTime;
 TIME           UserTime;
 TIME           KernelTime;
 UNICODE_STRING ProcessName;
 KPRIORITY      BasePriority;
 ULONG          ProcessId;
 ULONG          InheritedFromProcessId;
 ULONG          HandleCount;
 ULONG          Reserved2[2];
 VM_COUNTERS    VmCounters;
 SYSTEM_THREADS Threads[ANYSIZE_ARRAY];
} SYSTEM_PROCESSES_NT4, *PSYSTEM_PROCESSES_NT4;

typedef struct _SYSTEM_PROCESSES_NT5
{
 SIZE_T         NextEntryDelta;
 ULONG          ThreadCount;
 ULONG          Reserved1[6];
 TIME           CreateTime;
 TIME           UserTime;
 TIME           KernelTime;
 UNICODE_STRING ProcessName;
 KPRIORITY      BasePriority;
 ULONG          ProcessId;
 ULONG          InheritedFromProcessId;
 ULONG          HandleCount;
 ULONG          Reserved2[2];
 VM_COUNTERS    VmCounters;
 IO_COUNTERS    IoCounters;
 SYSTEM_THREADS Threads[ANYSIZE_ARRAY];
} SYSTEM_PROCESSES_NT5, *PSYSTEM_PROCESSES_NT5;

#ifndef __USE_W32API

/* Not sure. What version are we emulating? */
typedef SYSTEM_PROCESSES_NT5 SYSTEM_PROCESSES, *PSYSTEM_PROCESSES;

#endif /* __USE_W32API */

// SystemCallCountInformation (6)
typedef
struct _SYSTEM_SDT_INFORMATION
{
	ULONG	BufferLength;
	ULONG	NumberOfSystemServiceTables;
	ULONG	NumberOfServices [1];
	ULONG	ServiceCounters [1];

} SYSTEM_SDT_INFORMATION, *PSYSTEM_SDT_INFORMATION;

// SystemDeviceInformation (7)
typedef
struct _SYSTEM_DEVICE_INFORMATION
{
	ULONG	NumberOfDisks;
	ULONG	NumberOfFloppies;
	ULONG	NumberOfCdRoms;
	ULONG	NumberOfTapes;
	ULONG	NumberOfSerialPorts;
	ULONG	NumberOfParallelPorts;
} SYSTEM_DEVICE_INFORMATION, *PSYSTEM_DEVICE_INFORMATION;

// SystemProcessorPerformanceInformation (8)
// (one per processor in the system)
typedef
struct _SYSTEM_PROCESSORTIME_INFO
{
	TIME	TotalProcessorRunTime;
	TIME	TotalProcessorTime;
	TIME	TotalProcessorUserTime;
	TIME	TotalDPCTime;
	TIME	TotalInterruptTime;
	ULONG	TotalInterrupts;
	ULONG	Unused;

} SYSTEM_PROCESSORTIME_INFO, *PSYSTEM_PROCESSORTIME_INFO;

// SystemFlagsInformation (9)
typedef
struct _SYSTEM_FLAGS_INFORMATION
{
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
typedef
struct _SYSTEM_RESOURCE_LOCK_ENTRY
{
	ULONG	ResourceAddress;
	ULONG	Always1;
	ULONG	Unknown;
	ULONG	ActiveCount;
	ULONG	ContentionCount;
	ULONG	Unused[2];
	ULONG	NumberOfSharedWaiters;
	ULONG	NumberOfExclusiveWaiters;
	
} SYSTEM_RESOURCE_LOCK_ENTRY, *PSYSTEM_RESOURCE_LOCK_ENTRY;

typedef
struct _SYSTEM_RESOURCE_LOCK_INFO
{
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
typedef
struct _SYSTEM_VDM_INFORMATION
{
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
typedef
struct _POOL_TAG_STATS
{
	ULONG AllocationCount;
	ULONG FreeCount;
	ULONG SizeBytes;
	
} POOL_TAG_STATS;

typedef
struct _SYSTEM_POOL_TAG_ENTRY
{
	ULONG		Tag;
	POOL_TAG_STATS	Paged;
	POOL_TAG_STATS	NonPaged;

} SYSTEM_POOL_TAG_ENTRY, * PSYSTEM_POOL_TAG_ENTRY;

typedef
struct _SYSTEM_POOL_TAG_INFO
{
	ULONG			Count;
	SYSTEM_POOL_TAG_ENTRY	PoolEntry [1];

} SYSTEM_POOL_TAG_INFO, *PSYSTEM_POOL_TAG_INFO;

// SystemProcessorScheduleInfo (23)
typedef
struct _SYSTEM_PROCESSOR_SCHEDULE_INFO
{
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
typedef
struct _SYSTEM_PROCESSOR_FAULT_INFO
{
	ULONG	nAlignmentFixup;
	ULONG	nExceptionDispatches;
	ULONG	nFloatingEmulation;
	ULONG	Unknown;
	
} SYSTEM_PROCESSOR_FAULT_INFO, *PSYSTEM_PROCESSOR_FAULT_INFO;

// SystemCrashDumpStateInfo (34)
//

// SystemDebuggerInformation (35)
typedef
struct _SYSTEM_DEBUGGER_INFO
{
	BOOLEAN	KdDebuggerEnabled;
	BOOLEAN	KdDebuggerPresent;
	
} SYSTEM_DEBUGGER_INFO, *PSYSTEM_DEBUGGER_INFO;

// SystemInformation36 (36)
// UNKNOWN

// SystemQuotaInformation (37)
typedef
struct _SYSTEM_QUOTA_INFORMATION
{
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

// key restore flags

#define REG_WHOLE_HIVE_VOLATILE			1
#define REG_REFRESH_HIVE			2

// object type  access rights

#define OBJECT_TYPE_CREATE		0x0001
#define OBJECT_TYPE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)

// directory access rights

#define DIRECTORY_QUERY				0x0001
#define DIRECTORY_TRAVERSE			0x0002
#define DIRECTORY_CREATE_OBJECT			0x0004
#define DIRECTORY_CREATE_SUBDIRECTORY		0x0008

#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

// symbolic link access rights

#define SYMBOLIC_LINK_QUERY			0x0001
#define SYMBOLIC_LINK_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)


typedef struct _OBJECT_DATA_INFORMATION
{
	BOOLEAN bInheritHandle;
	BOOLEAN bProtectFromClose;
} OBJECT_DATA_INFORMATION, *POBJECT_DATA_INFORMATION;


typedef struct _OBJECT_TYPE_INFORMATION
{
	UNICODE_STRING	Name;
	UNICODE_STRING Type;
	ULONG TotalHandles;
	ULONG ReferenceCount;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;


// directory information

typedef struct _OBJDIR_INFORMATION {
	UNICODE_STRING ObjectName;
	UNICODE_STRING ObjectTypeName; // Directory, Device ...
	UCHAR          Data[0];
} OBJDIR_INFORMATION, *POBJDIR_INFORMATION;


/*
	 Action is one of the following values:

	FILE_ACTION_ADDED      	    	0x00000001
	FILE_ACTION_REMOVED     	0x00000002
	FILE_ACTION_MODIFIED       	0x00000003
	FILE_ACTION_RENAMED_OLD_NAME	0x00000004
	FILE_ACTION_RENAMED_NEW_NAME 	0x00000005
	FILE_ACTION_ADDED_STREAM   	0x00000006
	FILE_ACTION_REMOVED_STREAM  	0x00000007
	FILE_ACTION_MODIFIED_STREAM  	0x00000008

*/


// File System Control commands ( related to defragging )

#define	FSCTL_READ_MFT_RECORD			0x90068 // NTFS only

typedef struct _BITMAP_DESCRIPTOR
{
	ULONGLONG	StartLcn;
	ULONGLONG	ClustersToEndOfVol;
	BYTE		Map[0]; // variable size
} BITMAP_DESCRIPTOR, *PBITMAP_DESCRIPTOR;


//typedef enum _TIMER_TYPE 
//{
//	NotificationTimer,
//	SynchronizationTimer
//} TIMER_TYPE;

typedef struct _TIMER_BASIC_INFORMATION
{
  LARGE_INTEGER TimeRemaining;
  BOOLEAN SignalState;
} TIMER_BASIC_INFORMATION, *PTIMER_BASIC_INFORMATION;

typedef enum _TIMER_INFORMATION_CLASS
{
  TimerBasicInformation
} TIMER_INFORMATION_CLASS;

typedef
struct _LPC_PORT_BASIC_INFORMATION
{
	DWORD	Unknown0;
	DWORD	Unknown1;
	DWORD	Unknown2;
	DWORD	Unknown3;
	DWORD	Unknown4;
	DWORD	Unknown5;
	DWORD	Unknown6;
	DWORD	Unknown7;
	DWORD	Unknown8;
	DWORD	Unknown9;
	DWORD	Unknown10;
	DWORD	Unknown11;
	DWORD	Unknown12;
	DWORD	Unknown13;

} LPC_PORT_BASIC_INFORMATION, * PLPC_PORT_BASIC_INFORMATION;

#endif
