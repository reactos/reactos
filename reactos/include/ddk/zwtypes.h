#ifndef __INCLUDE_DDK_ZWTYPES_H
#define __INCLUDE_DDK_ZWTYPES_H

#define NtCurrentProcess() ( (HANDLE) 0xFFFFFFFF )
#define NtCurrentThread() ( (HANDLE) 0xFFFFFFFE )

typedef PVOID RTL_ATOM;

#ifdef __NTOSKRNL__
extern ULONG EXPORTED NtBuildNumber;
#else
extern ULONG IMPORTED NtBuildNumber;
#endif


// event access mask

#define EVENT_READ_ACCESS			1
#define EVENT_WRITE_ACCESS			2


// file disposition values


#define FILE_SUPERSEDE                  0x0000
#define FILE_OPEN                       0x0001
#define FILE_CREATE                     0x0002
#define FILE_OPEN_IF                    0x0003
#define FILE_OVERWRITE                  0x0004
#define FILE_OVERWRITE_IF               0x0005
#define FILE_MAXIMUM_DISPOSITION        0x0005

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
#define ProcessImageFileName                    22
#define MaxProcessInfoClass			23

// thread query / set information class
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
#define MaxThreadInfoClass			15

// object handle information

#define ObjectBasicInformation			0
#define ObjectNameInformation			1
#define ObjectTypeInformation			2
#define ObjectAllInformation			3
#define ObjectDataInformation			4

// semaphore information

#define SemaphoreBasicInformation		0

// event information

typedef enum _EVENT_INFORMATION_CLASS
{
	EventBasicInformation			= 0
} EVENT_INFORMATION_CLASS;

// system information
// {Nt|Zw}{Query|Set}SystemInformation

typedef
enum _SYSTEM_INFORMATION_CLASS
{
	SystemInformationClassMin		= 0,
	SystemBasicInformation			= 0,	/* Q */
	SystemProcessorInformation		= 1,	/* Q */
	SystemPerformanceInformation		= 2,	/* Q */
	SystemTimeInformation			= 3,	/* Q */
	SystemPathInformation			= 4,
	SystemProcessInformation		= 5,	/* Q */
	SystemServiceDescriptorTableInfo	= 6,	/* Q */
	SystemIoConfigInformation		= 7,	/* Q */
	SystemProcessorTimeInformation		= 8,	/* Q */
	SystemNtGlobalFlagInformation		= 9,	/* QS */
	SystemInformation10			= 10,
	SystemModuleInformation			= 11,	/* Q */
	SystemResourceLockInformation		= 12,	/* Q */
	SystemInformation13			= 13,
	SystemInformation14			= 14,
	SystemInformation15			= 15,
	SystemHandleInformation			= 16,	/* Q */
	SystemObjectInformation			= 17,	/* Q */
	SystemPageFileInformation		= 18,	/* Q */
	SystemInstructionEmulationInfo		= 19,	/* Q */
	SystemInformation20			= 20,
	SystemCacheInformation			= 21,	/* QS */
	SystemPoolTagInformation		= 22,	/* Q (checked build only) */
	SystemProcessorScheduleInfo		= 23,	/* Q */
	SystemDpcInformation			= 24,	/* QS */
	SystemFullMemoryInformation		= 25,
	SystemLoadGdiDriverInformation		= 26,	/* S (callable) */
	SystemUnloadGdiDriverInformation	= 27,	/* S (callable) */
	SystemTimeAdjustmentInformation		= 28,	/* QS */
	SystemInformation29			= 29,
	SystemInformation30			= 30,
	SystemInformation31			= 31,
	SystemCrashDumpSectionInfo		= 32,	/* Q */
	SystemProcessorFaultCountInfo		= 33,	/* Q */
	SystemCrashDumpStateInfo		= 34,	/* Q */
	SystemDebuggerInformation		= 35,	/* Q */
	SystemThreadSwitchCountersInfo		= 36,	/* Q */
	SystemQuotaInformation			= 37,	/* QS */
	SystemLoadDriver			= 38,	/* S */
	SystemPrioritySeparationInfo		= 39,	/* S */
	SystemInformation40			= 40,
	SystemInformation41			= 41,
	SystemInformation42			= 42,
	SystemInformation43			= 43,
	SystemTimeZoneInformation		= 44,	/* QS */
	SystemLookasideInformation		= 45,	/* Q */
	SystemInformationClassMax

} SYSTEM_INFORMATION_CLASS;

// SystemBasicInformation (0)
typedef
struct _SYSTEM_BASIC_INFORMATION
{
	DWORD	AlwaysZero;
	ULONG	KeMaximumIncrement;
	ULONG	MmPageSize;
	ULONG	MmNumberOfPhysicalPages;
	ULONG	MmLowestPhysicalPage;
	ULONG	MmHighestPhysicalPage;
	PVOID	MmLowestUserAddress;
	PVOID	MmLowestUserAddress1;
	PVOID	MmHighestUserAddress;
	DWORD	KeActiveProcessors;
	USHORT	KeNumberProcessors;
	
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

// SystemProcessorInformation (1)
typedef
struct _SYSTEM_PROCESSOR_INFORMATION
{
	USHORT	KeProcessorArchitecture;
	USHORT	KeProcessorLevel;
	USHORT	KeProcessorRevision;
	USHORT	AlwaysZero;
	DWORD	KeFeatureBits;

} SYSTEM_PROCESSOR_INFORMATION, *PSYSTEM_PROCESSOR_INFORMATION;

// SystemPerformanceInfo (2)
typedef
struct _SYSTEM_PERFORMANCE_INFO
{
	LARGE_INTEGER	TotalProcessorTime;
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

} SYSTEM_PERFORMANCE_INFO, *PSYSTEM_PERFORMANCE_INFO;

// SystemTimeInformation (3)
typedef
struct _SYSTEM_TIME_INFORMATION
{
	TIME	KeBootTime;
	TIME	KeSystemTime;
	TIME	ExpTimeZoneBias;
	ULONG	ExpTimeZoneId;
	ULONG	Unused;
	
} SYSTEM_TIME_INFORMATION, *PSYSTEM_TIME_INFORMATION;

// SystemPathInformation (4)
// IT DOES NOT WORK
typedef
struct _SYSTEM_PATH_INFORMATION
{
	PVOID	Dummy;

} SYSTEM_PATH_INFORMATION, * PSYSTEM_PATH_INFORMATION;

// SystemProcessThreadInfo (5)
typedef
struct _SYSTEM_THREAD_INFORMATION
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

typedef
struct SYSTEM_PROCESS_INFORMATION
{
	ULONG				RelativeOffset;
	ULONG				ThreadCount;
	ULONG				Unused1 [6];
	TIME				CreateTime;
	TIME				UserTime;
	TIME				KernelTime;
	UNICODE_STRING			Name;
	ULONG				BasePriority;
	ULONG				ProcessId;
	ULONG				ParentProcessId;
	ULONG				HandleCount;
	ULONG				Unused2[2];
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

// SystemServiceDescriptorTableInfo (6)
typedef
struct _SYSTEM_SDT_INFORMATION
{
	ULONG	BufferLength;
	ULONG	NumberOfSystemServiceTables;
	ULONG	NumberOfServices [1];
	ULONG	ServiceCounters [1];

} SYSTEM_SDT_INFORMATION, *PSYSTEM_SDT_INFORMATION;

// SystemIoConfigInformation (7)
typedef
struct _SYSTEM_IOCONFIG_INFORMATION
{
	ULONG	DiskCount;
	ULONG	FloppyCount;
	ULONG	CdRomCount;
	ULONG	TapeCount;
	ULONG	SerialCount;
	ULONG	ParallelCount;
	
} SYSTEM_IOCONFIG_INFORMATION, *PSYSTEM_IOCONFIG_INFORMATION;

// SystemProcessorTimeInformation (8)
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

// SystemNtGlobalFlagInformation (9)
typedef
struct _SYSTEM_GLOBAL_FLAG_INFO
{
	ULONG	NtGlobalFlag;

} SYSTEM_GLOBAL_FLAG_INFO, * PSYSTEM_GLOBAL_FLAG_INFO;

// SystemInformation10 (10)
// UNKNOWN

// SystemModuleInformation (11)
typedef
struct _SYSTEM_MODULE_ENTRY
{
	ULONG	Unused;
	ULONG	Always0;
	ULONG	ModuleBaseAddress;
	ULONG	ModuleSize;
	ULONG	Unknown;
	ULONG	ModuleEntryIndex;
	USHORT	ModuleNameLength; /* Length of module name not including the path, this field contains valid value only for NTOSKRNL module*/
	USHORT	ModulePathLength; /* Length of 'directory path' part of modulename*/
	CHAR	ModuleName [256];

} SYSTEM_MODULE_ENTRY, * PSYSTEM_MODULE_ENTRY;

typedef
struct _SYSTEM_MODULE_INFORMATION
{
	ULONG			Count;
	SYSTEM_MODULE_ENTRY	Module [1];
	
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

// SystemResourceLockInformation (12)
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
// UNKNOWN
typedef
struct _SYSTEM_OBJECT_INFORMATION
{
	DWORD	Unknown;
	/* FIXME */
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

// SystemInformation25 (25)
// UNKNOWN

// SystemLoadGdiDriverInformation (26)
// SystemUnloadGdiDriverInformation (27)
typedef struct _SYSTEM_GDI_DRIVER_INFORMATION
{
	UNICODE_STRING		DriverName;
	PVOID			ImageAddress;
	PVOID			SectionPointer;
	PVOID			EntryPoint;
//	PIMAGE_EXPORT_DIRECTORY	ExportSectionPointer;
	PVOID			ExportSectionPointer;
} SYSTEM_GDI_DRIVER_INFORMATION, *PSYSTEM_GDI_DRIVER_INFORMATION;

// SystemTimeAdjustmentInformation (28)
// (what is the right one?)
#if 0
typedef
struct _SYSTEM_TIME_ADJUSTMENT_INFO
{
	TIME	TimeAdjustment;
	BOOL	TimeAdjustmentDisabled;

} SYSTEM_TIME_ADJUSTMENT_INFO, *PSYSTEM_TIME_ADJUSTMENT_INFO;
#else
typedef
struct _SYSTEM_TIME_ADJUSTMENT_INFO
{
	ULONG	KeTimeAdjustment;
	ULONG	KeMaximumIncrement;
	BOOLEAN	KeTimeSynchronization;
	
} SYSTEM_TIME_ADJUSTMENT_INFO, *PSYSTEM_TIME_ADJUSTMENT_INFO;
#endif

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

// SystemLoadDriver (38)
typedef
struct _SYSTEM_DRIVER_LOAD
{
	UNICODE_STRING	DriverRegistryEntry;
	
} SYSTEM_DRIVER_LOAD, *PSYSTEM_DRIVER_LOAD;



// memory information

#define MemoryBasicInformation			0

// shutdown action

typedef enum SHUTDOWN_ACTION_TAG {
	ShutdownNoReboot,
	ShutdownReboot,
	ShutdownPowerOff
} SHUTDOWN_ACTION;

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

typedef struct _PROCESS_WS_WATCH_INFORMATION
{
	PVOID FaultingPc;
	PVOID FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION
{
	NTSTATUS ExitStatus;
	PPEB PebBaseAddress;
	KAFFINITY AffinityMask;
	KPRIORITY BasePriority;
	ULONG UniqueProcessId;
	ULONG InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef struct _QUOTA_LIMITS
{
	ULONG PagedPoolLimit;
	ULONG NonPagedPoolLimit;
	ULONG MinimumWorkingSetSize;
	ULONG MaximumWorkingSetSize;
	ULONG PagefileLimit;
	TIME TimeLimit;
} QUOTA_LIMITS, *PQUOTA_LIMITS;

typedef struct _IO_COUNTERS
{
	ULONG ReadOperationCount;
	ULONG WriteOperationCount;
	ULONG OtherOperationCount;
	LARGE_INTEGER ReadTransferCount;
	LARGE_INTEGER WriteTransferCount;
	LARGE_INTEGER OtherTransferCount;
} IO_COUNTERS, *PIO_COUNTERS;


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


typedef struct _PROCESS_ACCESS_TOKEN
{
	HANDLE Token;
	HANDLE Thread;
} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

typedef struct _KERNEL_USER_TIMES
{
	TIME CreateTime;
	TIME ExitTime;
	TIME KernelTime;
	TIME UserTime;
} KERNEL_USER_TIMES;
typedef KERNEL_USER_TIMES *PKERNEL_USER_TIMES;

// thread information

// incompatible with MS NT

typedef struct _THREAD_BASIC_INFORMATION
{
	NTSTATUS ExitStatus;
	PVOID TebBaseAddress;
	KAFFINITY AffinityMask;
	KPRIORITY BasePriority;
	ULONG UniqueThreadId;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

// object information

typedef struct _OBJECT_NAME_INFORMATION
{
	UNICODE_STRING	Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;



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

// file information

typedef struct _FILE_BASIC_INFORMATION
{
	TIME CreationTime;
	TIME LastAccessTime;
	TIME LastWriteTime;
	TIME ChangeTime;
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

typedef struct _FILE_COMPRESSION_INFORMATION {
	LARGE_INTEGER CompressedFileSize;
	USHORT CompressionFormat;
	UCHAR CompressionUnitShift;
	UCHAR ChunkShift;
	UCHAR ClusterShift;
	UCHAR Reserved[3];
} FILE_COMPRESSION_INFORMATION, *PFILE_COMPRESSION_INFORMATION;

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

// directory information

typedef struct _OBJDIR_INFORMATION {
	UNICODE_STRING ObjectName;
	UNICODE_STRING ObjectTypeName; // Directory, Device ...
	UCHAR          Data[0];
} OBJDIR_INFORMATION, *POBJDIR_INFORMATION;


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


//FIXME: I am a win32 object
typedef
VOID
(*PTIMERAPCROUTINE)(
	LPVOID lpArgToCompletionRoutine,
	DWORD dwTimerLowValue,
	DWORD dwTimerHighValue
	);


// File System Control commands ( related to defragging )

#define	FSCTL_READ_MFT_RECORD			0x90068 // NTFS only
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

typedef struct _BITMAP_DESCRIPTOR
{
	ULONGLONG	StartLcn;
	ULONGLONG	ClustersToEndOfVol;
	BYTE		Map[0]; // variable size
} BITMAP_DESCRIPTOR, *PBITMAP_DESCRIPTOR;

typedef struct _MOVEFILE_DESCRIPTOR
{
	HANDLE            FileHandle;
	ULONG             Reserved;
	LARGE_INTEGER     StartVcn;
	LARGE_INTEGER     TargetLcn;
	ULONG             NumVcns;
	ULONG             Reserved1;
} MOVEFILE_DESCRIPTOR, *PMOVEFILE_DESCRIPTOR;


// semaphore information

typedef struct _SEMAPHORE_BASIC_INFORMATION
{
	ULONG CurrentCount;
	ULONG MaximumCount;
} SEMAPHORE_BASIC_INFORMATION, *PSEMAPHORE_BASIC_INFORMATION;

// event information

typedef struct _EVENT_BASIC_INFORMATION
{
	EVENT_TYPE EventType;
	LONG EventState;
} EVENT_BASIC_INFORMATION, *PEVENT_BASIC_INFORMATION;


//typedef enum _TIMER_TYPE 
//{
//	NotificationTimer,
//	SynchronizationTimer
//} TIMER_TYPE;

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
