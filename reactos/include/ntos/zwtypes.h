#ifndef __INCLUDE_DDK_ZWTYPES_H
#define __INCLUDE_DDK_ZWTYPES_H

#define NtCurrentProcess() ( (HANDLE) 0xFFFFFFFF )
#define NtCurrentThread() ( (HANDLE) 0xFFFFFFFE )

#if defined(_NTOSKRNL_)
extern DECL_EXPORT ULONG NtBuildNumber;
#else
extern DECL_IMPORT ULONG NtBuildNumber;
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
#define MaxProcessInfoClass			26
/* ReactOS private. */
#define ProcessImageFileName			(MaxProcessInfoClass + 1)
#define ProcessDesktop                          (MaxProcessInfoClass + 2)
#define RosMaxProcessInfoClass			(MaxProcessInfoClass + 2)

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
#define RosMaxThreadInfoClass			17


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


// semaphore information

typedef enum _SEMAPHORE_INFORMATION_CLASS
{
	SemaphoreBasicInformation		= 0
} SEMAPHORE_INFORMATION_CLASS;

typedef struct _SEMAPHORE_BASIC_INFORMATION
{
	LONG CurrentCount;
	LONG MaximumCount;
} SEMAPHORE_BASIC_INFORMATION, *PSEMAPHORE_BASIC_INFORMATION;


// event information

typedef enum _EVENT_INFORMATION_CLASS
{
	EventBasicInformation			= 0
} EVENT_INFORMATION_CLASS;

typedef struct _EVENT_BASIC_INFORMATION
{
	EVENT_TYPE EventType;
	LONG EventState;
} EVENT_BASIC_INFORMATION, *PEVENT_BASIC_INFORMATION;


// system information
// {Nt|Zw}{Query|Set}SystemInformation
// (GN means Gary Nebbet in "NT/W2K Native API Reference")

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

// SystemModuleInformation (11)
typedef
struct _SYSTEM_MODULE_ENTRY
{
	ULONG	Unknown1;
	ULONG	Unknown2;
	PVOID	BaseAddress;
	ULONG	Size;
	ULONG	Flags;
	ULONG	EntryIndex;
	USHORT	NameLength; /* Length of module name not including the path, this field contains valid value only for NTOSKRNL module*/
	USHORT	PathLength; /* Length of 'directory path' part of modulename*/
	CHAR	Name [256];
} SYSTEM_MODULE_ENTRY, * PSYSTEM_MODULE_ENTRY;

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

// memory information

#define MemoryBasicInformation			0

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
// thread information

#if 0
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
#endif

typedef struct _OBJECT_DATA_INFORMATION
{
	BOOLEAN bInheritHandle;
	BOOLEAN bProtectFromClose;
} OBJECT_DATA_INFORMATION, *POBJECT_DATA_INFORMATION;


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
