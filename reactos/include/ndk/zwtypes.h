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

/* DEPENDENCIES **************************************************************/
#include "rtltypes.h"
#include <ddk/cfg.h>

/* EXPORTED DATA *************************************************************/

/* CONSTANTS *****************************************************************/
#define MAX_BUS_NAME 24

#define EVENT_READ_ACCESS             1
#define EVENT_WRITE_ACCESS            2

#define PORT_ALL_ACCESS               (0x1)

#define EVENT_PAIR_ALL_ACCESS    (0x1F0000L)

#define OBJECT_TYPE_CREATE 0x0001
#define OBJECT_TYPE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)

/* For ProcessDeviceMap */
#define DOSDEVICE_DRIVE_UNKNOWN    0
#define DOSDEVICE_DRIVE_CALCULATE  1
#define DOSDEVICE_DRIVE_REMOVABLE  2
#define DOSDEVICE_DRIVE_FIXED      3
#define DOSDEVICE_DRIVE_REMOTE     4
#define DOSDEVICE_DRIVE_CDROM      5
#define DOSDEVICE_DRIVE_RAMDISK    6

/* ENUMERATIONS **************************************************************/

typedef enum _HARDERROR_RESPONSE_OPTION
{
    OptionAbortRetryIgnore,
    OptionOk,
    OptionOkCancel,
    OptionRetryCancel,
    OptionYesNo,
    OptionYesNoCancel,
    OptionShutdownSystem
} HARDERROR_RESPONSE_OPTION, *PHARDERROR_RESPONSE_OPTION;

typedef enum _HARDERROR_RESPONSE
{
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

typedef enum SHUTDOWN_ACTION_TAG
{
    ShutdownNoReboot,
    ShutdownReboot,
    ShutdownPowerOff
} SHUTDOWN_ACTION;

typedef enum _PLUGPLAY_BUS_CLASS
{
    SystemBus,
    PlugPlayVirtualBus,
    MaxPlugPlayBusClass
} PLUGPLAY_BUS_CLASS, *PPLUGPLAY_BUS_CLASS;

typedef enum _PLUGPLAY_VIRTUAL_BUS_TYPE
{
    Root,
    MaxPlugPlayVirtualBusType
} PLUGPLAY_VIRTUAL_BUS_TYPE, *PPLUGPLAY_VIRTUAL_BUS_TYPE;

typedef enum _SYSTEM_DOCK_STATE
{
    SystemDockStateUnknown,
    SystemUndocked,
    SystemDocked
} SYSTEM_DOCK_STATE, *PSYSTEM_DOCK_STATE;

/**** Information Classes ****/

/*
 * Process (extra ones not defined in DDK)
 */
typedef enum _PROCESS_INFORMATION_FLAGS
{
    ProcessUnknown33 = 33,
    ProcessUnknown34,
    ProcessUnknown35,
    ProcessCookie,
    MaximumProcessInformationClass
} PROCESS_INFORMATION_FLAGS;

/*
 * System
 */
typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemBasicInformation,
    SystemProcessorInformation,
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemPathInformation, /* OBSOLETE: USE KUSER_SHARED_DATA */
    SystemProcessInformation,
    SystemCallCountInformation,
    SystemDeviceInformation,
    SystemProcessorPerformanceInformation,
    SystemFlagsInformation,
    SystemCallTimeInformation,
    SystemModuleInformation,
    SystemLocksInformation,
    SystemStackTraceInformation,
    SystemPagedPoolInformation,
    SystemNonPagedPoolInformation,
    SystemHandleInformation,
    SystemObjectInformation,
    SystemPageFileInformation,
    SystemVdmInstemulInformation,
    SystemVdmBopInformation,
    SystemFileCacheInformation,
    SystemPoolTagInformation,
    SystemInterruptInformation,
    SystemDpcBehaviorInformation,
    SystemFullMemoryInformation,
    SystemLoadGdiDriverInformation,
    SystemUnloadGdiDriverInformation,
    SystemTimeAdjustmentInformation,
    SystemSummaryMemoryInformation,
    SystemNextEventIdInformation,
    SystemEventIdsInformation,
    SystemCrashDumpInformation,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemExtendServiceTableInformation,
    SystemPrioritySeperation,
    SystemPlugPlayBusInformation,
    SystemDockInformation,
    _SystemPowerInformation, /* FIXME */
    SystemProcessorSpeedInformation,
    SystemCurrentTimeZoneInformation,
    SystemLookasideInformation,
    SystemTimeSlipNotification,
    SystemSessionCreate,
    SystemSessionDetach,
    SystemSessionInformation,
    SystemRangeStartInformation,
    SystemVerifierInformation,
    SystemAddVerifier,
    SystemSessionProcessesInformation,
    SystemInformationClassMax
} SYSTEM_INFORMATION_CLASS;

/*
 * Object
 */
typedef enum _OBJECT_INFORMATION_CLASS
{
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectAllTypesInformation,
    ObjectHandleInformation
} OBJECT_INFORMATION_CLASS;

/*
 * Memory
 */
typedef enum _MEMORY_INFORMATION_CLASS
{
    MemoryBasicInformation,
    MemoryWorkingSetList,
    MemorySectionName,
    MemoryBasicVlmInformation
} MEMORY_INFORMATION_CLASS;

/*
 * Mutant
 */
typedef enum _MUTANT_INFORMATION_CLASS
{
    MutantBasicInformation
} MUTANT_INFORMATION_CLASS;

/*
 * Atom
 */
typedef enum _ATOM_INFORMATION_CLASS
{
    AtomBasicInformation,
    AtomTableInformation,
} ATOM_INFORMATION_CLASS;

/*
 * Section
 */
typedef enum _SECTION_INFORMATION_CLASS
{
    SectionBasicInformation,
    SectionImageInformation,
} SECTION_INFORMATION_CLASS;

/*
 * Timer
 */
typedef enum _TIMER_INFORMATION_CLASS {
    TimerBasicInformation
} TIMER_INFORMATION_CLASS;

/*
 * Semaphore
 */
typedef enum _SEMAPHORE_INFORMATION_CLASS
{
    SemaphoreBasicInformation
} SEMAPHORE_INFORMATION_CLASS;

/*
 * Event
 */
typedef enum _EVENT_INFORMATION_CLASS
{
    EventBasicInformation
} EVENT_INFORMATION_CLASS;

/*
 * I/O Completion
 */
typedef enum _IO_COMPLETION_INFORMATION_CLASS
{
    IoCompletionBasicInformation
} IO_COMPLETION_INFORMATION_CLASS;

/*
 *  PlugPlay
 */
typedef enum _PLUGPLAY_CONTROL_CLASS
{
    PlugPlayControlUserResponse = 0x07,
    PlugPlayControlProperty = 0x0A,
    PlugPlayControlGetRelatedDevice = 0x0C,
    PlugPlayControlDeviceStatus = 0x0E
} PLUGPLAY_CONTROL_CLASS;

/* TYPES *********************************************************************/

typedef unsigned short LANGID;
typedef LANGID *PLANGID;
struct _PLUGPLAY_EVENT_BLOCK; /* FIXME: Ask Filip if it's OK to define it */

/**** Information Structures ****/
/*
 * Mutant
 */

/* Class 0 */
typedef struct _MUTANT_BASIC_INFORMATION
{
    LONG CurrentCount;
    BOOLEAN OwnedByCaller;
    BOOLEAN AbandonedState;
} MUTANT_BASIC_INFORMATION, *PMUTANT_BASIC_INFORMATION;

/*
 * Thread
 */

/* Class 0 */
typedef struct _THREAD_BASIC_INFORMATION
{
    NTSTATUS  ExitStatus;
    PVOID TebBaseAddress;
    CLIENT_ID ClientId;
    KAFFINITY AffinityMask;
    KPRIORITY Priority;
    KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

/*
 * Atom
 */

/* Class 0 */
typedef struct _ATOM_BASIC_INFORMATION {
    USHORT UsageCount;
    USHORT Flags;
    USHORT NameLength;
    WCHAR Name[1];
} ATOM_BASIC_INFORMATION, *PATOM_BASIC_INFORMATION;

/* Class 1 */
typedef struct _ATOM_TABLE_INFORMATION
{
    ULONG NumberOfAtoms;
    USHORT Atoms[1];
} ATOM_TABLE_INFORMATION, *PATOM_TABLE_INFORMATION;

/*
 * Object Directory
 */
typedef struct _OBJECT_DIRECTORY_INFORMATION
{
    UNICODE_STRING ObjectName;
    UNICODE_STRING ObjectTypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

/*
 * Memory
 */

 /* Class 1 */
typedef struct _MEMORY_WORKING_SET_LIST
{
    ULONG NumberOfPages;
    ULONG WorkingSetList[1];
} MEMORY_WORKING_SET_LIST, *PMEMORY_WORKING_SET_LIST;

/* Class 2 */
typedef struct
{
    UNICODE_STRING SectionFileName;
    WCHAR          NameBuffer[ANYSIZE_ARRAY];
} MEMORY_SECTION_NAME, *PMEMORY_SECTION_NAME;

/*
 * Section
 */

/* Class 0 */
typedef struct _SECTION_BASIC_INFORMATION
{
    PVOID           BaseAddress;
    ULONG           Attributes;
    LARGE_INTEGER   Size;
} SECTION_BASIC_INFORMATION, *PSECTION_BASIC_INFORMATION;

/* Class 1 */
typedef struct _SECTION_IMAGE_INFORMATION
{
    ULONG     EntryPoint;
    ULONG     Unknown1;
    ULONG_PTR StackReserve;
    ULONG_PTR StackCommit;
    ULONG     Subsystem;
    USHORT    MinorSubsystemVersion;
    USHORT    MajorSubsystemVersion;
    ULONG     Unknown2;
    ULONG     Characteristics;
    USHORT    ImageNumber;
    BOOLEAN   Executable;
    UCHAR     Unknown3;
    ULONG     Unknown4[3];
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

/*
 * Object
 */

/* Class 4 */
typedef struct _OBJECT_HANDLE_ATTRIBUTE_INFORMATION
{
    BOOLEAN Inherit;
    BOOLEAN ProtectFromClose;
} OBJECT_HANDLE_ATTRIBUTE_INFORMATION, *POBJECT_HANDLE_ATTRIBUTE_INFORMATION;

/*
 * Timer
 */

/* Class 0 */
typedef struct _TIMER_BASIC_INFORMATION
{
    LARGE_INTEGER TimeRemaining;
    BOOLEAN SignalState;
} TIMER_BASIC_INFORMATION, *PTIMER_BASIC_INFORMATION;

/*
 * Semaphore
 */

/* Class 0 */
typedef struct _SEMAPHORE_BASIC_INFORMATION
{
    LONG CurrentCount;
    LONG MaximumCount;
} SEMAPHORE_BASIC_INFORMATION, *PSEMAPHORE_BASIC_INFORMATION;

/*
 * Event
 */
/* Class 0 */
typedef struct _EVENT_BASIC_INFORMATION
{
    EVENT_TYPE EventType;
    LONG EventState;
} EVENT_BASIC_INFORMATION, *PEVENT_BASIC_INFORMATION;

/*
 * Process
 */

/* Class 0 */
typedef struct _PROCESS_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    KAFFINITY AffinityMask;
    KPRIORITY BasePriority;
    ULONG UniqueProcessId;
    ULONG InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;


/* Class 4 */
typedef struct _KERNEL_USER_TIMES
{
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES, *PKERNEL_USER_TIMES;

/* Class 9 */
typedef struct _PROCESS_ACCESS_TOKEN
{
    HANDLE Token;
    HANDLE Thread;
} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

/* Class 16 */
typedef struct _PROCESS_PRIORITY_CLASS
{
    BOOLEAN Foreground;
    UCHAR   PriorityClass;
} PROCESS_PRIORITY_CLASS, *PPROCESS_PRIORITY_CLASS;

/* Class 23 */
typedef struct _PROCESS_DEVICEMAP_INFORMATION
{
    union
    {
        struct
        {
            HANDLE DirectoryHandle;
        } Set;
        struct
        {
            ULONG DriveMap;
            UCHAR DriveType[32];
        } Query;
    };
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

/* Class 24 */
typedef struct _PROCESS_SESSION_INFORMATION
{
    ULONG SessionId;
} PROCESS_SESSION_INFORMATION, *PPROCESS_SESSION_INFORMATION;

/*
 * System
 */

/* Class 0 */
typedef struct _SYSTEM_BASIC_INFORMATION
{
    ULONG Reserved;
    ULONG TimerResolution;
    ULONG PageSize;
    ULONG NumberOfPhysicalPages;
    ULONG LowestPhysicalPageNumber;
    ULONG HighestPhysicalPageNumber;
    ULONG AllocationGranularity;
    ULONG MinimumUserModeAddress;
    ULONG MaximumUserModeAddress;
    KAFFINITY ActiveProcessorsAffinityMask;
    CCHAR NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

/* Class 1 */
typedef struct _SYSTEM_PROCESSOR_INFORMATION
{
    USHORT ProcessorArchitecture;
    USHORT ProcessorLevel;
    USHORT ProcessorRevision;
    USHORT Reserved;
    ULONG ProcessorFeatureBits;
} SYSTEM_PROCESSOR_INFORMATION, *PSYSTEM_PROCESSOR_INFORMATION;

/* Class 2 */
typedef struct _SYSTEM_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER IdleProcessTime;
    LARGE_INTEGER IoReadTransferCount;
    LARGE_INTEGER IoWriteTransferCount;
    LARGE_INTEGER IoOtherTransferCount;
    ULONG IoReadOperationCount;
    ULONG IoWriteOperationCount;
    ULONG IoOtherOperationCount;
    ULONG AvailablePages;
    ULONG CommittedPages;
    ULONG CommitLimit;
    ULONG PeakCommitment;
    ULONG PageFaultCount;
    ULONG CopyOnWriteCount;
    ULONG TransitionCount;
    ULONG CacheTransitionCount;
    ULONG DemandZeroCount;
    ULONG PageReadCount;
    ULONG PageReadIoCount;
    ULONG CacheReadCount;
    ULONG CacheIoCount;
    ULONG DirtyPagesWriteCount;
    ULONG DirtyWriteIoCount;
    ULONG MappedPagesWriteCount;
    ULONG MappedWriteIoCount;
    ULONG PagedPoolPages;
    ULONG NonPagedPoolPages;
    ULONG PagedPoolAllocs;
    ULONG PagedPoolFrees;
    ULONG NonPagedPoolAllocs;
    ULONG NonPagedPoolFrees;
    ULONG FreeSystemPtes;
    ULONG ResidentSystemCodePage;
    ULONG TotalSystemDriverPages;
    ULONG TotalSystemCodePages;
    ULONG NonPagedPoolLookasideHits;
    ULONG PagedPoolLookasideHits;
    ULONG Spare3Count;
    ULONG ResidentSystemCachePage;
    ULONG ResidentPagedPoolPage;
    ULONG ResidentSystemDriverPage;
    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadResourceMiss;
    ULONG CcFastReadNotPossible;
    ULONG CcFastMdlReadNoWait;
    ULONG CcFastMdlReadWait;
    ULONG CcFastMdlReadResourceMiss;
    ULONG CcFastMdlReadNotPossible;
    ULONG CcMapDataNoWait;
    ULONG CcMapDataWait;
    ULONG CcMapDataNoWaitMiss;
    ULONG CcMapDataWaitMiss;
    ULONG CcPinMappedDataCount;
    ULONG CcPinReadNoWait;
    ULONG CcPinReadWait;
    ULONG CcPinReadNoWaitMiss;
    ULONG CcPinReadWaitMiss;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;
    ULONG CcCopyReadWaitMiss;
    ULONG CcMdlReadNoWait;
    ULONG CcMdlReadWait;
    ULONG CcMdlReadNoWaitMiss;
    ULONG CcMdlReadWaitMiss;
    ULONG CcReadAheadIos;
    ULONG CcLazyWriteIos;
    ULONG CcLazyWritePages;
    ULONG CcDataFlushes;
    ULONG CcDataPages;
    ULONG ContextSwitches;
    ULONG FirstLevelTbFills;
    ULONG SecondLevelTbFills;
    ULONG SystemCalls;
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

/* Class 3 */
typedef struct _SYSTEM_TIMEOFDAY_INFORMATION
{
    LARGE_INTEGER BootTime;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER TimeZoneBias;
    ULONG TimeZoneId;
    ULONG Reserved;
} SYSTEM_TIMEOFDAY_INFORMATION, *PSYSTEM_TIMEOFDAY_INFORMATION;

/* Class 4 */
/* This class is obsoleted, please use KUSER_SHARED_DATA instead */

/* Class 5 */
typedef struct _SYSTEM_THREAD_INFORMATION
{
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    CLIENT_ID ClientId;
    KPRIORITY Priority;
    LONG BasePriority;
    ULONG ContextSwitches;
    ULONG ThreadState;
    ULONG WaitReason;
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

typedef struct _SYSTEM_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG PageDirectoryFrame;

    /*
     * This part corresponds to VM_COUNTERS_EX.
     * NOTE: *NOT* THE SAME AS VM_COUNTERS!
     */
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
    ULONG PrivateUsage;

    /* This part corresponds to IO_COUNTERS */
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;

    /* Finally, the array of Threads */
    SYSTEM_THREAD_INFORMATION TH[1];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

/* Class 6 */
typedef struct _SYSTEM_CALL_COUNT_INFORMATION
{
    ULONG Length;
    ULONG NumberOfTables;
} SYSTEM_CALL_COUNT_INFORMATION, *PSYSTEM_CALL_COUNT_INFORMATION;

/* Class 7 */
typedef struct _SYSTEM_DEVICE_INFORMATION
{
    ULONG NumberOfDisks;
    ULONG NumberOfFloppies;
    ULONG NumberOfCdRoms;
    ULONG NumberOfTapes;
    ULONG NumberOfSerialPorts;
    ULONG NumberOfParallelPorts;
} SYSTEM_DEVICE_INFORMATION, *PSYSTEM_DEVICE_INFORMATION;

/* Class 8 */
typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

/* Class 9 */
typedef struct _SYSTEM_FLAGS_INFORMATION
{
    ULONG Flags;
} SYSTEM_FLAGS_INFORMATION, *PSYSTEM_FLAGS_INFORMATION;

/* Class 10 */
typedef struct _SYSTEM_CALL_TIME_INFORMATION
{
    ULONG Length;
    ULONG TotalCalls;
    LARGE_INTEGER TimeOfCalls[1];
} SYSTEM_CALL_TIME_INFORMATION, *PSYSTEM_CALL_TIME_INFORMATION;

/* Class 11 */
typedef struct _SYSTEM_MODULE_INFORMATION_ENTRY
{
    ULONG  Unknown1;
    ULONG  Unknown2;
    PVOID  Base;
    ULONG  Size;
    ULONG  Flags;
    USHORT  Index;
    USHORT  NameLength;
    USHORT  LoadCount;
    USHORT  PathLength;
    CHAR  ImageName[256];
} SYSTEM_MODULE_INFORMATION_ENTRY, *PSYSTEM_MODULE_INFORMATION_ENTRY;
typedef struct _SYSTEM_MODULE_INFORMATION
{
    ULONG Count;
    SYSTEM_MODULE_INFORMATION_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

/* Class 12 */
typedef struct _SYSTEM_RESOURCE_LOCK_ENTRY
{
    ULONG  ResourceAddress;
    ULONG  Always1;
    ULONG  Unknown;
    ULONG  ActiveCount;
    ULONG  ContentionCount;
    ULONG  Unused[2];
    ULONG  NumberOfSharedWaiters;
    ULONG  NumberOfExclusiveWaiters;
} SYSTEM_RESOURCE_LOCK_ENTRY, *PSYSTEM_RESOURCE_LOCK_ENTRY;

typedef struct _SYSTEM_RESOURCE_LOCK_INFO
{
    ULONG Count;
    SYSTEM_RESOURCE_LOCK_ENTRY Lock[1];
} SYSTEM_RESOURCE_LOCK_INFO, *PSYSTEM_RESOURCE_LOCK_INFO;

/* Class 13 */
typedef struct _SYSTEM_BACKTRACE_INFORMATION_ENTRY
{
    /* FIXME */
} SYSTEM_BACKTRACE_INFORMATION_ENTRY, *PSYSTEM_BACKTRACE_INFORMATION_ENTRY;

typedef struct _SYSTEM_BACKTRACE_INFORMATION
{
    /* FIXME */
    ULONG Unknown[4];
    ULONG Count;
    SYSTEM_BACKTRACE_INFORMATION_ENTRY Trace[1];
} SYSTEM_BACKTRACE_INFORMATION, *PSYSTEM_BACKTRACE_INFORMATION;

/* Class 14 - 15 */
typedef struct _SYSTEM_POOL_ENTRY
{
    BOOLEAN Allocated;
    BOOLEAN Spare0;
    USHORT AllocatorBackTraceIndex;
    ULONG Size;
    union
    {
        UCHAR Tag[4];
        ULONG TagUlong;
        PVOID ProcessChargedQuota;
    };
} SYSTEM_POOL_ENTRY, *PSYSTEM_POOL_ENTRY;

typedef struct _SYSTEM_POOL_INFORMATION
{
    ULONG TotalSize;
    PVOID FirstEntry;
    USHORT EntryOverhead;
    BOOLEAN PoolTagPresent;
    BOOLEAN Spare0;
    ULONG NumberOfEntries;
    SYSTEM_POOL_ENTRY Entries[1];
} SYSTEM_POOL_INFORMATION, *PSYSTEM_POOL_INFORMATION;

/* Class 16 */
typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
    USHORT UniqueProcessId;
    USHORT CreatorBackTraceIndex;
    UCHAR ObjectTypeIndex;
    UCHAR HandleAttributes;
    USHORT HandleValue;
    PVOID Object;
    ULONG GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
    ULONG NumberOfHandles;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

/* Class 17 */
typedef struct _SYSTEM_OBJECTTYPE_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG NumberOfObjects;
    ULONG NumberOfHandles;
    ULONG TypeIndex;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccessMask;
    ULONG PoolType;
    BOOLEAN SecurityRequired;
    BOOLEAN WaitableObject;
    UNICODE_STRING TypeName;
} SYSTEM_OBJECTTYPE_INFORMATION, *PSYSTEM_OBJECTTYPE_INFORMATION;

typedef struct _SYSTEM_OBJECT_INFORMATION
{
    ULONG NextEntryOffset;
    PVOID Object;
    HANDLE CreatorUniqueProcess;
    USHORT CreatorBackTraceIndex;
    USHORT Flags;
    LONG PointerCount;
    LONG HandleCount;
    ULONG PagedPoolCharge;
    ULONG NonPagedPoolCharge;
    HANDLE ExclusiveProcessId;
    PVOID SecurityDescriptor;
    OBJECT_NAME_INFORMATION NameInfo;
} SYSTEM_OBJECT_INFORMATION, *PSYSTEM_OBJECT_INFORMATION;

/* Class 18 */
typedef struct _SYSTEM_PAGEFILE_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG TotalSize;
    ULONG TotalInUse;
    ULONG PeakUsage;
    UNICODE_STRING PageFileName;
} SYSTEM_PAGEFILE_INFORMATION, *PSYSTEM_PAGEFILE_INFORMATION;

/* Class 19 */
typedef struct _SYSTEM_VDM_INSTEMUL_INFO
{
    ULONG SegmentNotPresent;
    ULONG VdmOpcode0F;
    ULONG OpcodeESPrefix;
    ULONG OpcodeCSPrefix;
    ULONG OpcodeSSPrefix;
    ULONG OpcodeDSPrefix;
    ULONG OpcodeFSPrefix;
    ULONG OpcodeGSPrefix;
    ULONG OpcodeOPER32Prefix;
    ULONG OpcodeADDR32Prefix;
    ULONG OpcodeINSB;
    ULONG OpcodeINSW;
    ULONG OpcodeOUTSB;
    ULONG OpcodeOUTSW;
    ULONG OpcodePUSHF;
    ULONG OpcodePOPF;
    ULONG OpcodeINTnn;
    ULONG OpcodeINTO;
    ULONG OpcodeIRET;
    ULONG OpcodeINBimm;
    ULONG OpcodeINWimm;
    ULONG OpcodeOUTBimm;
    ULONG OpcodeOUTWimm ;
    ULONG OpcodeINB;
    ULONG OpcodeINW;
    ULONG OpcodeOUTB;
    ULONG OpcodeOUTW;
    ULONG OpcodeLOCKPrefix;
    ULONG OpcodeREPNEPrefix;
    ULONG OpcodeREPPrefix;
    ULONG OpcodeHLT;
    ULONG OpcodeCLI;
    ULONG OpcodeSTI;
    ULONG BopCount;
} SYSTEM_VDM_INSTEMUL_INFO, *PSYSTEM_VDM_INSTEMUL_INFO;

/* Class 20 */
typedef struct _SYSTEM_VDM_BOP_INFO
{
    /* FIXME */
    PVOID Dummy;
} SYSTEM_VDM_BOP_INFO, *PSYSTEM_VDM_BOP_INFO;

/* Class 21 */
typedef struct _SYSTEM_CACHE_INFORMATION
{
    ULONG CurrentSize;
    ULONG PeakSize;
    ULONG PageFaultCount;
    ULONG MinimumWorkingSet;
    ULONG MaximumWorkingSet;
    ULONG CurrentSizeIncludingTransitionInPages;
    ULONG PeakSizeIncludingTransitionInPages;
    ULONG Unused[2];
} SYSTEM_CACHE_INFORMATION, *PSYSTEM_CACHE_INFORMATION;

/* Class 22 */
typedef struct _SYSTEM_POOLTAG
{
    union
    {
        UCHAR Tag[4];
        ULONG TagUlong;
    };
    ULONG PagedAllocs;
    ULONG PagedFrees;
    ULONG PagedUsed;
    ULONG NonPagedAllocs;
    ULONG NonPagedFrees;
    ULONG NonPagedUsed;
} SYSTEM_POOLTAG, *PSYSTEM_POOLTAG;
typedef struct _SYSTEM_POOLTAG_INFORMATION
{
    ULONG Count;
    SYSTEM_POOLTAG TagInfo[1];
} SYSTEM_POOLTAG_INFORMATION, *PSYSTEM_POOLTAG_INFORMATION;

/* Class 23 */
typedef struct _SYSTEM_INTERRUPT_INFORMATION
{
    ULONG ContextSwitches;
    ULONG DpcCount;
    ULONG DpcRate;
    ULONG TimeIncrement;
    ULONG DpcBypassCount;
    ULONG ApcBypassCount;
} SYSTEM_INTERRUPT_INFORMATION, *PSYSTEM_INTERRUPT_INFORMATION;

/* Class 24 */
typedef struct _SYSTEM_DPC_BEHAVIOR_INFORMATION
{
    ULONG Spare;
    ULONG DpcQueueDepth;
    ULONG MinimumDpcRate;
    ULONG AdjustDpcThreshold;
    ULONG IdealDpcRate;
} SYSTEM_DPC_BEHAVIOR_INFORMATION, *PSYSTEM_DPC_BEHAVIOR_INFORMATION;

/* Class 25 */
typedef struct _SYSTEM_MEMORY_INFO
{
    PUCHAR StringOffset;
    USHORT ValidCount;
    USHORT TransitionCount;
    USHORT ModifiedCount;
    USHORT PageTableCount;
} SYSTEM_MEMORY_INFO, *PSYSTEM_MEMORY_INFO;
typedef struct _SYSTEM_MEMORY_INFORMATION
{
    ULONG InfoSize;
    ULONG StringStart;
    SYSTEM_MEMORY_INFO Memory[1];
} SYSTEM_MEMORY_INFORMATION, *PSYSTEM_MEMORY_INFORMATION;

/* Class 26 */
typedef struct _SYSTEM_GDI_DRIVER_INFORMATION
{
    UNICODE_STRING DriverName;
    PVOID ImageAddress;
    PVOID SectionPointer;
    PVOID EntryPoint;
    PIMAGE_EXPORT_DIRECTORY ExportSectionPointer;
} SYSTEM_GDI_DRIVER_INFORMATION, *PSYSTEM_GDI_DRIVER_INFORMATION;

/* Class 27 */
/* Not an actuall class, simply a PVOID to the ImageAddress */

/* Class 28 */
typedef struct _SYSTEM_QUERY_TIME_ADJUST_INFORMATION
{
    ULONG TimeAdjustment;
    ULONG TimeIncrement;
    BOOLEAN Enable;
} SYSTEM_QUERY_TIME_ADJUST_INFORMATION, *PSYSTEM_QUERY_TIME_ADJUST_INFORMATION;

typedef struct _SYSTEM_SET_TIME_ADJUST_INFORMATION
{
    ULONG TimeAdjustment;
    BOOLEAN Enable;
} SYSTEM_SET_TIME_ADJUST_INFORMATION, *PSYSTEM_SET_TIME_ADJUST_INFORMATION;

/* Class 29 - Same as 25 */

/* Class 30 - 31 */
/* FIXME */

/* Class 32 */
typedef struct _SYSTEM_CRASH_DUMP_INFORMATION
{
    HANDLE CrashDumpSection;
} SYSTEM_CRASH_DUMP_INFORMATION, *PSYSTEM_CRASH_DUMP_INFORMATION;

/* Class 33 */
typedef struct _SYSTEM_EXCEPTION_INFORMATION
{
    ULONG AlignmentFixupCount;
    ULONG ExceptionDispatchCount;
    ULONG FloatingEmulationCount;
    ULONG ByteWordEmulationCount;
} SYSTEM_EXCEPTION_INFORMATION, *PSYSTEM_EXCEPTION_INFORMATION;

/* Class 34 */
typedef struct _SYSTEM_CRASH_STATE_INFORMATION
{
    ULONG ValidCrashDump;
} SYSTEM_CRASH_STATE_INFORMATION, *PSYSTEM_CRASH_STATE_INFORMATION;

/* Class 35 */
typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION
{
    BOOLEAN KernelDebuggerEnabled;
    BOOLEAN KernelDebuggerNotPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION, *PSYSTEM_KERNEL_DEBUGGER_INFORMATION;

/* Class 36 */
typedef struct _SYSTEM_CONTEXT_SWITCH_INFORMATION {
    ULONG ContextSwitches;
    ULONG FindAny;
    ULONG FindLast;
    ULONG FindIdeal;
    ULONG IdleAny;
    ULONG IdleCurrent;
    ULONG IdleLast;
    ULONG IdleIdeal;
    ULONG PreemptAny;
    ULONG PreemptCurrent;
    ULONG PreemptLast;
    ULONG SwitchToIdle;
} SYSTEM_CONTEXT_SWITCH_INFORMATION, *PSYSTEM_CONTEXT_SWITCH_INFORMATION;

/* Class 37 */
typedef struct _SYSTEM_REGISTRY_QUOTA_INFORMATION
{
    ULONG RegistryQuotaAllowed;
    ULONG RegistryQuotaUsed;
    ULONG PagedPoolSize;
} SYSTEM_REGISTRY_QUOTA_INFORMATION, *PSYSTEM_REGISTRY_QUOTA_INFORMATION;

/* Class 38 */
/* Not a structure, simply send the UNICODE_STRING */

/* Class 39 */
/* Not a structure, simply send a ULONG containing the new separation */

/* Class 40 */
typedef struct _PLUGPLAY_BUS_TYPE
{
    PLUGPLAY_BUS_CLASS BusClass;
    union
    {
        INTERFACE_TYPE SystemBusType;
        PLUGPLAY_VIRTUAL_BUS_TYPE PlugPlayVirtualBusType;
    };
} PLUGPLAY_BUS_TYPE, *PPLUGPLAY_BUS_TYPE;

typedef struct _PLUGPLAY_BUS_INSTANCE
{
    PLUGPLAY_BUS_TYPE BusType;
    ULONG BusNumber;
    WCHAR BusName[MAX_BUS_NAME];
} PLUGPLAY_BUS_INSTANCE, *PPLUGPLAY_BUS_INSTANCE;

typedef struct _SYSTEM_PLUGPLAY_BUS_INFORMATION
{
    ULONG BusCount;
    PLUGPLAY_BUS_INSTANCE BusInstance[1];
} SYSTEM_PLUGPLAY_BUS_INFORMATION, *PSYSTEM_PLUGPLAY_BUS_INFORMATION;

/* Class 41 */
typedef struct _SYSTEM_DOCK_INFORMATION
{
    SYSTEM_DOCK_STATE DockState;
    INTERFACE_TYPE DeviceBusType;
    ULONG DeviceBusNumber;
    ULONG SlotNumber;
} SYSTEM_DOCK_INFORMATION, *PSYSTEM_DOCK_INFORMATION;

/* Class 42 */
/* FIXME: Conflict with WINNT.H */
typedef struct __SYSTEM_POWER_INFORMATION
{
    BOOLEAN SystemSuspendSupported;
    BOOLEAN SystemHibernateSupported;
    BOOLEAN ResumeTimerSupportsSuspend;
    BOOLEAN ResumeTimerSupportsHibernate;
    BOOLEAN LidSupported;
    BOOLEAN TurboSettingSupported;
    BOOLEAN TurboMode;
    BOOLEAN SystemAcOrDc;
    BOOLEAN PowerDownDisabled;
    LARGE_INTEGER SpindownDrives;
} _SYSTEM_POWER_INFORMATION, *P_SYSTEM_POWER_INFORMATION;

/* Class 43 */
typedef struct _SYSTEM_LEGACY_DRIVER_INFORMATION
{
    PNP_VETO_TYPE VetoType;
    UNICODE_STRING VetoDriver;
    /* Buffer Follows */
} SYSTEM_LEGACY_DRIVER_INFORMATION, *PSYSTEM_LEGACY_DRIVER_INFORMATION;

/* Class 44 */
typedef struct _TIME_ZONE_INFORMATION RTL_TIME_ZONE_INFORMATION;

/* Class 45 */
typedef struct _SYSTEM_LOOKASIDE_INFORMATION
{
    USHORT CurrentDepth;
    USHORT MaximumDepth;
    ULONG TotalAllocates;
    ULONG AllocateMisses;
    ULONG TotalFrees;
    ULONG FreeMisses;
    ULONG Type;
    ULONG Tag;
    ULONG Size;
} SYSTEM_LOOKASIDE_INFORMATION, *PSYSTEM_LOOKASIDE_INFORMATION;

/* Class 46 */
/* Not a structure. Only a HANDLE for the SlipEvent; */

/* Class 47 */
/* Not a structure. Only a ULONG for the SessionId; */

/* Class 48 */
/* Not a structure. Only a ULONG for the SessionId; */

/* Class 49 */
/* FIXME */

/* Class 50 */
/* Not a structure. Only a ULONG_PTR for the SystemRangeStart */

/* Class 51 */
/* FIXME. Based on MM_DRIVER_VERIFIER_DATA */

/* Class 52 */
/* FIXME */

/* Class 53 */
typedef struct _SYSTEM_SESSION_PROCESSES_INFORMATION
{
    ULONG    SessionId;
    ULONG    BufferSize;
    PVOID    Buffer; /* same format as in SystemProcessInformation */
} SYSTEM_SESSION_PROCESSES_INFORMATION, * PSYSTEM_SESSION_PROCESSES_INFORMATION;

/* Class 54-81 */
/* FIXME */


#endif
