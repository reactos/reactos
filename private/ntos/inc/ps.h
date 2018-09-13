/*++ BUILD Version: 0009    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ps.h

Abstract:

    This module contains the process structure public data structures and
    procedure prototypes to be used within the NT system.

Author:

    Mark Lucovsky       16-Feb-1989

Revision History:

--*/

#ifndef _PS_
#define _PS_

//
// Invalid handle table value.
//

#define PSP_INVALID_ID ((ULONG_PTR)(0x82)<<((sizeof(ULONG_PTR)-1)*8))

//
// Process Object
//

//
// Process object body.  A pointer to this structure is returned when a handle
// to a process object is referenced.  This structure contains a process control
// block (PCB) which is the kernel's representation of a process.
//

#define MEMORY_PRIORITY_BACKGROUND 0
#define MEMORY_PRIORITY_WASFOREGROUND 1
#define MEMORY_PRIORITY_FOREGROUND 2

typedef struct _MMSUPPORT_FLAGS {
    unsigned SessionSpace : 1;
    unsigned BeingTrimmed : 1;
    unsigned ProcessInSession : 1;
    unsigned SessionLeader : 1;
    unsigned TrimHard : 1;
    unsigned WorkingSetHard : 1;
    unsigned WriteWatch : 1;
    unsigned Filler : 25;
} MMSUPPORT_FLAGS;

typedef struct _MMSUPPORT {
    LARGE_INTEGER LastTrimTime;
    ULONG LastTrimFaultCount;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    ULONG MinimumWorkingSetSize;
    ULONG MaximumWorkingSetSize;
    struct _MMWSL *VmWorkingSetList;
    LIST_ENTRY WorkingSetExpansionLinks;
    UCHAR AllowWorkingSetAdjustment;
    BOOLEAN AddressSpaceBeingDeleted;
    UCHAR ForegroundSwitchCount;
    UCHAR MemoryPriority;

    union {
        ULONG LongFlags;
        MMSUPPORT_FLAGS Flags;
    } u;

    ULONG Claim;
    ULONG NextEstimationSlot;
    ULONG NextAgingSlot;
    ULONG EstimatedAvailable;

    ULONG GrowthSinceLastEstimate;

} MMSUPPORT;

typedef MMSUPPORT *PMMSUPPORT;

//
// Client impersonation information
//

typedef struct _PS_IMPERSONATION_INFORMATION {
    PACCESS_TOKEN Token;
    BOOLEAN CopyOnOpen;
    BOOLEAN EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
} PS_IMPERSONATION_INFORMATION, *PPS_IMPERSONATION_INFORMATION;


//
// Changes to the EPROCESS structure require that you re-run genoff for x86.
// This change is needed because Old debugger references the processes
// debug port. If this is not done then the user-debugger will not work.
// After running genoff, you must re-build ntsd !
//

typedef struct _EPROCESS_QUOTA_BLOCK {
    KSPIN_LOCK QuotaLock;
    ULONG ReferenceCount;
    SIZE_T QuotaPeakPoolUsage[2];
    SIZE_T QuotaPoolUsage[2];
    SIZE_T QuotaPoolLimit[2];
    SIZE_T PeakPagefileUsage;
    SIZE_T PagefileUsage;
    SIZE_T PagefileLimit;
} EPROCESS_QUOTA_BLOCK, *PEPROCESS_QUOTA_BLOCK;

#if DEVL

//
// Pagefault monitoring
//

typedef struct _PAGEFAULT_HISTORY {
    ULONG CurrentIndex;
    ULONG MaxIndex;
    KSPIN_LOCK SpinLock;
    PVOID Reserved;
    PROCESS_WS_WATCH_INFORMATION WatchInfo[1];
} PAGEFAULT_HISTORY, *PPAGEFAULT_HISTORY;
#endif // DEVL

#define PS_WS_TRIM_FROM_EXE_HEADER        1
#define PS_WS_TRIM_BACKGROUND_ONLY_APP    2

//
// Wow64 process stucture
//

typedef struct _WOW64_PROCESS {
    PVOID Wow64;
#if defined(_IA64_)
    FAST_MUTEX AlternateTableLock;
    PULONG AltPermBitmap;
    ULONG AltFlags;
#endif
} WOW64_PROCESS, *PWOW64_PROCESS;

#define PS_SET_BITS(Flags, Flag) \
    ExInterlockedSetBits (Flags, Flag)

#define PS_CLEAR_BITS(Flags, Flag) \
    ExInterlockedClearBits (Flags, Flag)

#define PS_SET_CLEAR_BITS(Flags, sFlag, cFlag) \
    ExInterlockedSetClearBits (Flags, sFlag, cFlag)

//
// Process structure.
//
// If you remove a field from this structure, please also
// remove the reference to it from within the kernel debugger
// (nt\private\sdktools\ntsd\ntkext.c)
//

typedef struct _EPROCESS {
    KPROCESS Pcb;
    NTSTATUS ExitStatus;
    KEVENT LockEvent;
    ULONG LockCount;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    PKTHREAD LockOwner;

    HANDLE UniqueProcessId;

    LIST_ENTRY ActiveProcessLinks;

    //
    // Quota Fields
    //

    SIZE_T QuotaPeakPoolUsage[2];
    SIZE_T QuotaPoolUsage[2];

    SIZE_T PagefileUsage;
    SIZE_T CommitCharge;
    SIZE_T PeakPagefileUsage;

    //
    // VmCounters
    //

    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;

    MMSUPPORT Vm;
    LIST_ENTRY SessionProcessLinks;

    PVOID DebugPort;
    PVOID ExceptionPort;
    PHANDLE_TABLE ObjectTable;

    //
    // Security
    //

    PACCESS_TOKEN Token;         // This field must never be null

    //

    FAST_MUTEX WorkingSetLock;
    PFN_NUMBER WorkingSetPage;
    BOOLEAN ProcessOutswapEnabled;
    BOOLEAN ProcessOutswapped;
    UCHAR AddressSpaceInitialized;
    BOOLEAN AddressSpaceDeleted;
    FAST_MUTEX AddressCreationLock;
    KSPIN_LOCK HyperSpaceLock;
    struct _ETHREAD *ForkInProgress;
    USHORT VmOperation;
    UCHAR ForkWasSuccessful;
    UCHAR MmAgressiveWsTrimMask;
    PKEVENT VmOperationEvent;
    PVOID PaeTop;
    ULONG LastFaultCount;
    ULONG ModifiedPageCount;
    PVOID VadRoot;
    PVOID VadHint;
    PVOID CloneRoot;
    PFN_NUMBER NumberOfPrivatePages;
    PFN_NUMBER NumberOfLockedPages;
    USHORT NextPageColor;
    BOOLEAN ExitProcessCalled;

    //
    // Used by Debug Subsystem
    //

    BOOLEAN CreateProcessReported;
    HANDLE SectionHandle;

    //
    // Peb
    //

    PPEB Peb;
    PVOID SectionBaseAddress;

    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    NTSTATUS LastThreadExitStatus;
    PPAGEFAULT_HISTORY WorkingSetWatch;
    HANDLE Win32WindowStation;
    HANDLE InheritedFromUniqueProcessId;
    ACCESS_MASK GrantedAccess;
    ULONG DefaultHardErrorProcessing;
    PVOID LdtInformation;
    PVOID VadFreeHint;
    PVOID VdmObjects;
    PVOID DeviceMap;

    //
    // Id of the Hydra session in which this process is running
    //

    ULONG SessionId;

    LIST_ENTRY PhysicalVadList;
    union {
        HARDWARE_PTE PageDirectoryPte;
        ULONGLONG Filler;
    };
    ULONG PaePageDirectoryPage;
    UCHAR ImageFileName[ 16 ];
    ULONG VmTrimFaultValue;
    BOOLEAN SetTimerResolution;
    UCHAR PriorityClass;
    union {
        struct {
            UCHAR SubSystemMinorVersion;
            UCHAR SubSystemMajorVersion;
        };
        USHORT SubSystemVersion;
    };
    PVOID Win32Process;
    struct _EJOB *Job;
    ULONG JobStatus;
    LIST_ENTRY JobLinks;
    PVOID LockedPagesList;

    //
    // Used by rdr/security for authentication
    //

    PVOID SecurityPort ;              
    PWOW64_PROCESS Wow64Process;

    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;

    SIZE_T CommitChargeLimit;
    SIZE_T CommitChargePeak;

    LIST_ENTRY ThreadListHead;

    PRTL_BITMAP VadPhysicalPagesBitMap;
    ULONG_PTR VadPhysicalPages;
    KSPIN_LOCK AweLock;
} EPROCESS;

#define PS_JOB_STATUS_NOT_REALLY_ACTIVE      0x00000001
#define PS_JOB_STATUS_ACCOUNTING_FOLDED      0x00000002
#define PS_JOB_STATUS_NEW_PROCESS_REPORTED   0x00000004
#define PS_JOB_STATUS_EXIT_PROCESS_REPORTED  0x00000008
#define PS_JOB_STATUS_REPORT_COMMIT_CHANGES  0x00000010
#define PS_JOB_STATUS_LAST_REPORT_MEMORY     0x00000020

typedef EPROCESS *PEPROCESS;


//
// Thread Object
//
// Thread object body.  A pointer to this structure is returned when a handle
// to a thread object is referenced.  This structure contains a thread control
// block (TCB) which is the kernel's representation of a thread.
//
// If you remove a field from this structure, please also
// remove the reference to it from within the kernel debugger
// (nt\private\sdktools\ntsd\ntkext.c)
//

//
// The upper 4 bits of the CreateTime should be zero on initialization so
// that the shift doesn't destroy anything.
//

#define PS_GET_THREAD_CREATE_TIME(Thread) ((Thread)->CreateTime.QuadPart >> 3)

#define PS_SET_THREAD_CREATE_TIME(Thread, InputCreateTime) \
            ((Thread)->CreateTime.QuadPart = (InputCreateTime.QuadPart << 3))

typedef struct _ETHREAD {
    KTHREAD Tcb;
    union {

        //
        // The fact that this is a union means that all accesses to CreateTime
        // must be sanitized using the two macros above.
        //

        LARGE_INTEGER CreateTime;
    
        //
        // These fields are accessed only by the owning thread, but can be
        // accessed from within a special kernel APC so IRQL protection must
        // be applied.
        //
    
        struct {
            unsigned NestedFaultCount : 2;
            unsigned ApcNeeded : 1;
        };
    };

    union {
        LARGE_INTEGER ExitTime;
        LIST_ENTRY LpcReplyChain;
    };
    union {
        NTSTATUS ExitStatus;
        PVOID OfsChain;
    };

    //
    // Registry
    //

    LIST_ENTRY PostBlockList;
    LIST_ENTRY TerminationPortList;     // also used as reaper links

    KSPIN_LOCK ActiveTimerListLock;
    LIST_ENTRY ActiveTimerListHead;

    CLIENT_ID Cid;

    //
    // Lpc
    //

    KSEMAPHORE LpcReplySemaphore;
    PVOID LpcReplyMessage;          // -> Message that contains the reply
    ULONG LpcReplyMessageId;        // MessageId this thread is waiting for reply to

    //
    // Security
    //
    //
    //    Client - If non null, indicates the thread is impersonating
    //        a client.
    //

    ULONG PerformanceCountLow;
    PPS_IMPERSONATION_INFORMATION ImpersonationInfo;


    //
    // Io
    //

    LIST_ENTRY IrpList;

    //
    //  File Systems
    //

    ULONG_PTR TopLevelIrp;  // either NULL, an Irp or a flag defined in FsRtl.h
    struct _DEVICE_OBJECT *DeviceToVerify;

    //
    // Mm
    //

    ULONG ReadClusterSize;
    BOOLEAN ForwardClusterOnly;
    BOOLEAN DisablePageFaultClustering;

    BOOLEAN DeadThread;
    BOOLEAN HideFromDebugger;

    ULONG HasTerminated;

    //
    // Client/server
    //

    ACCESS_MASK GrantedAccess;
    PEPROCESS ThreadsProcess;
    PVOID StartAddress;
    union {
        PVOID Win32StartAddress;
        ULONG LpcReceivedMessageId;
    };
    BOOLEAN LpcExitThreadCalled;
    BOOLEAN HardErrorsAreDisabled;
    BOOLEAN LpcReceivedMsgIdValid;
    BOOLEAN ActiveImpersonationInfo;
    LONG PerformanceCountHigh;

    LIST_ENTRY ThreadListEntry;

} ETHREAD;
typedef ETHREAD *PETHREAD;

//
// Initial PEB
//

typedef struct _INITIAL_PEB {
    BOOLEAN InheritedAddressSpace;      // These four fields cannot change unless the
    BOOLEAN ReadImageFileExecOptions;   //
    BOOLEAN BeingDebugged;              //
    BOOLEAN SpareBool;                  //
    HANDLE Mutant;                      // PEB structure is also updated.
} INITIAL_PEB, *PINITIAL_PEB;

typedef struct _PS_JOB_TOKEN_FILTER {
    ULONG CapturedSidCount ;
    PSID_AND_ATTRIBUTES CapturedSids ;
    ULONG CapturedSidsLength ;

    ULONG CapturedGroupCount ;
    PSID_AND_ATTRIBUTES CapturedGroups ;
    ULONG CapturedGroupsLength ;

    ULONG CapturedPrivilegeCount ;
    PLUID_AND_ATTRIBUTES CapturedPrivileges ;
    ULONG CapturedPrivilegesLength ;
} PS_JOB_TOKEN_FILTER, * PPS_JOB_TOKEN_FILTER ;

//
// Job Object
//
typedef struct _EJOB {
    KEVENT Event;
    LIST_ENTRY JobLinks;
    LIST_ENTRY ProcessListHead;
    ERESOURCE JobLock;

    //
    // Accounting Info
    //

    LARGE_INTEGER TotalUserTime;
    LARGE_INTEGER TotalKernelTime;
    LARGE_INTEGER ThisPeriodTotalUserTime;
    LARGE_INTEGER ThisPeriodTotalKernelTime;
    ULONG TotalPageFaultCount;
    ULONG TotalProcesses;
    ULONG ActiveProcesses;
    ULONG TotalTerminatedProcesses;

    //
    // Limitable Attributes
    //

    LARGE_INTEGER PerProcessUserTimeLimit;
    LARGE_INTEGER PerJobUserTimeLimit;
    ULONG LimitFlags;
    SIZE_T MinimumWorkingSetSize;
    SIZE_T MaximumWorkingSetSize;
    ULONG ActiveProcessLimit;
    KAFFINITY Affinity;
    UCHAR PriorityClass;

    //
    // UI restrictions
    //

    ULONG UIRestrictionsClass;

    //
    // Security Limitations:  write once, read always
    //

    ULONG SecurityLimitFlags ;
    PACCESS_TOKEN Token ;
    PPS_JOB_TOKEN_FILTER Filter ;

    //
    // End Of Job Time Limit
    //

    ULONG EndOfJobTimeAction;
    PVOID CompletionPort;
    PVOID CompletionKey;

    ULONG SessionId;

    ULONG SchedulingClass;

    ULONGLONG ReadOperationCount;
    ULONGLONG WriteOperationCount;
    ULONGLONG OtherOperationCount;
    ULONGLONG ReadTransferCount;
    ULONGLONG WriteTransferCount;
    ULONGLONG OtherTransferCount;

    //
    // Extended Limits
    //

    IO_COUNTERS IoInfo;         // not used yet
    SIZE_T ProcessMemoryLimit;
    SIZE_T JobMemoryLimit;
    SIZE_T PeakProcessMemoryUsed;
    SIZE_T PeakJobMemoryUsed;
    SIZE_T CurrentJobMemoryUsed;

    FAST_MUTEX MemoryLimitsLock;

} EJOB;
typedef EJOB *PEJOB;


//
// Global Variables
//

extern ULONG PsPrioritySeperation;
extern ULONG PsRawPrioritySeparation;
extern LIST_ENTRY PsActiveProcessHead;
extern UNICODE_STRING PsNtDllPathName;
extern PVOID PsSystemDllBase;
extern FAST_MUTEX PsProcessSecurityLock;
extern PEPROCESS PsInitialSystemProcess;
extern PVOID PsNtosImageBase;
extern PVOID PsHalImageBase;
extern LIST_ENTRY PsLoadedModuleList;
extern ERESOURCE PsLoadedModuleResource;
extern LCID PsDefaultSystemLocaleId;
extern LCID PsDefaultThreadLocaleId;
extern LANGID PsDefaultUILanguageId;
extern LANGID PsInstallUILanguageId;
extern PEPROCESS PsIdleProcess;
extern BOOLEAN PsReaperActive;
extern LIST_ENTRY PsReaperListHead;
extern WORK_QUEUE_ITEM PsReaperWorkItem;

BOOLEAN
PsChangeJobMemoryUsage(
    SSIZE_T Amount
    );

VOID
PsReportProcessMemoryLimitViolation(
    VOID
    );

#if DEVL
#define THREAD_HIT_SLOTS 750
extern ULONG PsThreadHits[THREAD_HIT_SLOTS];
VOID
PsThreadHit(
    IN PETHREAD Thread
    );
#endif // DEVL

VOID
PsEnforceExecutionTimeLimits(
    VOID
    );

BOOLEAN
PsInitSystem (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTSTATUS
PsLocateSystemDll (
    VOID
    );

VOID
PsChangeQuantumTable(
    BOOLEAN ModifyActiveProcesses,
    ULONG PrioritySeparation
    );

//
// Get Gurrent Prototypes
//

#define THREAD_TO_PROCESS(thread) ((thread)->ThreadsProcess)
#define IS_SYSTEM_THREAD(thread)                                    \
            (((thread)->Tcb.Teb == NULL) ||                         \
            (IS_SYSTEM_ADDRESS((thread)->Tcb.Teb)))

#define PsGetCurrentProcess() (CONTAINING_RECORD(((KeGetCurrentThread())->ApcState.Process),EPROCESS,Pcb))

#define PsGetCurrentThread() (CONTAINING_RECORD((KeGetCurrentThread()),ETHREAD,Tcb))



//
// VOID
// PsLockProcessSecurityFields(VOID)
//

#define PsLockProcessSecurityFields( ) ExAcquireFastMutex( &PsProcessSecurityLock )

//
// VOID
// PsFreeProcessSecurityFields(VOID);
//

#define PsFreeProcessSecurityFields( ) ExReleaseFastMutex( &PsProcessSecurityLock )

//
// Exit special kernel mode APC routine.
//

VOID
PsExitSpecialApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

// begin_ntddk begin_wdm begin_nthal begin_ntifs
//
// System Thread and Process Creation and Termination
//

NTKERNELAPI
NTSTATUS
PsCreateSystemThread(
    OUT PHANDLE ThreadHandle,
    IN ULONG DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ProcessHandle OPTIONAL,
    OUT PCLIENT_ID ClientId OPTIONAL,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext
    );

NTKERNELAPI
NTSTATUS
PsTerminateSystemThread(
    IN NTSTATUS ExitStatus
    );

// end_ntddk end_wdm end_nthal end_ntifs

NTSTATUS
PsCreateSystemProcess(
    OUT PHANDLE ProcessHandle,
    IN ULONG DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL
    );

typedef
VOID (*PLEGO_NOTIFY_ROUTINE)(
    PKTHREAD Thread
    );

ULONG
PsSetLegoNotifyRoutine(
    PLEGO_NOTIFY_ROUTINE LegoNotifyRoutine
    );



// begin_ntifs begin_ntddk

typedef
VOID
(*PCREATE_PROCESS_NOTIFY_ROUTINE)(
    IN HANDLE ParentId,
    IN HANDLE ProcessId,
    IN BOOLEAN Create
    );

NTSTATUS
PsSetCreateProcessNotifyRoutine(
    IN PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
    IN BOOLEAN Remove
    );

typedef
VOID
(*PCREATE_THREAD_NOTIFY_ROUTINE)(
    IN HANDLE ProcessId,
    IN HANDLE ThreadId,
    IN BOOLEAN Create
    );

NTSTATUS
PsSetCreateThreadNotifyRoutine(
    IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine
    );

//
// Structures for Load Image Notify
//

typedef struct _IMAGE_INFO {
    union {
        ULONG Properties;
        struct {
            ULONG ImageAddressingMode  : 8;  // code addressing mode
            ULONG SystemModeImage      : 1;  // system mode image
            ULONG ImageMappedToAllPids : 1;  // image mapped into all processes
            ULONG Reserved             : 22;
        };
    };
    PVOID       ImageBase;
    ULONG       ImageSelector;
    SIZE_T      ImageSize;
    ULONG       ImageSectionNumber;
} IMAGE_INFO, *PIMAGE_INFO;

#define IMAGE_ADDRESSING_MODE_32BIT     3

typedef
VOID
(*PLOAD_IMAGE_NOTIFY_ROUTINE)(
    IN PUNICODE_STRING FullImageName,
    IN HANDLE ProcessId,                // pid into which image is being mapped
    IN PIMAGE_INFO ImageInfo
    );

NTSTATUS
PsSetLoadImageNotifyRoutine(
    IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine
    );
// end_ntddk end_ntifs

// begin_ntsrv
//
// Security Support
//

NTSTATUS
PsAssignImpersonationToken(
    IN PETHREAD Thread,
    IN HANDLE Token
    );

NTKERNELAPI
PACCESS_TOKEN
PsReferencePrimaryToken(
    IN PEPROCESS Process
    );

// end_ntsrv
// begin_ntifs
//
// VOID
// PsDereferencePrimaryToken(
//    IN PACCESS_TOKEN PrimaryToken
//    );
//
#define PsDereferencePrimaryToken(T) (ObDereferenceObject((T)))

// end_ntifs

#define PsProcessAuditId(Process)    ((Process)->UniqueProcessId)

NTKERNELAPI
PACCESS_TOKEN
PsReferenceImpersonationToken(
    IN PETHREAD Thread,
    OUT PBOOLEAN CopyOnOpen,
    OUT PBOOLEAN EffectiveOnly,
    OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    );

PACCESS_TOKEN
PsReferenceEffectiveToken(
    IN PETHREAD Thread,
    OUT PTOKEN_TYPE TokenType,
    OUT PBOOLEAN EffectiveOnly,
    OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    );

// begin_ntifs
//
// VOID
// PsDereferenceImpersonationToken(
//    In PACCESS_TOKEN ImpersonationToken
//    );
//
#define PsDereferenceImpersonationToken(T)                                          \
            {if (ARGUMENT_PRESENT(T)) {                                       \
                (ObDereferenceObject((T)));                                   \
             } else {                                                         \
                ;                                                             \
             }                                                                \
            }

LARGE_INTEGER
PsGetProcessExitTime(
    VOID
    );

// end_ntifs
#if defined(_NTDDK_) || defined(_NTIFS_)

// begin_ntifs
BOOLEAN
PsIsThreadTerminating(
    IN PETHREAD Thread
    );

// end_ntifs

#else

//
// BOOLEAN
// PsIsThreadTerminating(
//   IN PETHREAD Thread
//   )
//
//  Returns TRUE if thread is in the process of terminating.
//

#define PsIsThreadTerminating(T)                                            \
    (T)->HasTerminated

#endif

extern BOOLEAN PsImageNotifyEnabled;

VOID
PsCallImageNotifyRoutines(
    IN PUNICODE_STRING FullImageName,
    IN HANDLE ProcessId,                // pid into which image is being mapped
    IN PIMAGE_INFO ImageInfo
    );

NTSTATUS
PsImpersonateClient(
    IN PETHREAD Thread,
    IN PACCESS_TOKEN Token,
    IN BOOLEAN CopyOnOpen,
    IN BOOLEAN EffectiveOnly,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    );

// begin_ntsrv

BOOLEAN
PsDisableImpersonation(
    IN PETHREAD Thread,
    IN PSE_IMPERSONATION_STATE ImpersonationState
    );

VOID
PsRestoreImpersonation(
    IN PETHREAD Thread,
    IN PSE_IMPERSONATION_STATE ImpersonationState
    );

// end_ntsrv

NTKERNELAPI
VOID
PsRevertToSelf(
    VOID
    );


NTSTATUS
PsOpenTokenOfThread(
    IN HANDLE ThreadHandle,
    IN BOOLEAN OpenAsSelf,
    OUT PACCESS_TOKEN *Token,
    OUT PBOOLEAN CopyOnOpen,
    OUT PBOOLEAN EffectiveOnly,
    OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    );

NTSTATUS
PsOpenTokenOfProcess(
    IN HANDLE ProcessHandle,
    OUT PACCESS_TOKEN *Token
    );

NTSTATUS
PsOpenTokenOfJob(
    IN HANDLE JobHandle,
    OUT PACCESS_TOKEN * Token
    );

//
// Cid
//

NTSTATUS
PsLookupProcessThreadByCid(
    IN PCLIENT_ID Cid,
    OUT PEPROCESS *Process OPTIONAL,
    OUT PETHREAD *Thread
    );

NTKERNELAPI
NTSTATUS
PsLookupProcessByProcessId(
    IN HANDLE ProcessId,
    OUT PEPROCESS *Process
    );

NTKERNELAPI
NTSTATUS
PsLookupThreadByThreadId(
    IN HANDLE ThreadId,
    OUT PETHREAD *Thread
    );

// begin_ntifs
//
// Quota Operations
//

VOID
PsChargePoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN ULONG_PTR Amount
    );

VOID
PsReturnPoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN ULONG_PTR Amount
    );
// end_ntifs

//
// Context Management
//

VOID
PspContextToKframes(
    OUT PKTRAP_FRAME TrapFrame,
    OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN PCONTEXT Context
    );

VOID
PspContextFromKframes(
    OUT PKTRAP_FRAME TrapFrame,
    OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN PCONTEXT Context
    );

VOID
PsReturnSharedPoolQuota(
    IN PEPROCESS_QUOTA_BLOCK QuotaBlock,
    IN ULONG_PTR PagedAmount,
    IN ULONG_PTR NonPagedAmount
    );

PEPROCESS_QUOTA_BLOCK
PsChargeSharedPoolQuota(
    IN PEPROCESS Process,
    IN ULONG_PTR PagedAmount,
    IN ULONG_PTR NonPagedAmount
    );


typedef enum _PSLOCKPROCESSMODE {
    PsLockPollOnTimeout,
    PsLockReturnTimeout,
    PsLockWaitForever,
    PsLockIAmExiting
} PSLOCKPROCESSMODE;

NTSTATUS
PsLockProcess(
    IN PEPROCESS Process,
    IN KPROCESSOR_MODE WaitMode,
    IN PSLOCKPROCESSMODE LockMode
    );

VOID
PsUnlockProcess(
    IN PEPROCESS Process
    );


//
// Exception Handling
//

BOOLEAN
PsForwardException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN BOOLEAN DebugException,
    IN BOOLEAN SecondChance
    );

typedef
NTSTATUS
(*PKWIN32_PROCESS_CALLOUT) (
    IN PEPROCESS Process,
    IN BOOLEAN Initialize
    );


typedef enum _PSW32JOBCALLOUTTYPE {
    PsW32JobCalloutSetInformation,
    PsW32JobCalloutAddProcess,
    PsW32JobCalloutTerminate
} PSW32JOBCALLOUTTYPE;

typedef struct _WIN32_JOBCALLOUT_PARAMETERS {
    PVOID Job;
    PSW32JOBCALLOUTTYPE CalloutType;
    IN PVOID Data;
} WIN32_JOBCALLOUT_PARAMETERS, *PKWIN32_JOBCALLOUT_PARAMETERS;


typedef
NTSTATUS
(*PKWIN32_JOB_CALLOUT) (
    IN PKWIN32_JOBCALLOUT_PARAMETERS Parm
     );


typedef enum _PSW32THREADCALLOUTTYPE {
    PsW32ThreadCalloutInitialize,
    PsW32ThreadCalloutExit
} PSW32THREADCALLOUTTYPE;

typedef
NTSTATUS
(*PKWIN32_THREAD_CALLOUT) (
    IN PETHREAD Thread,
    IN PSW32THREADCALLOUTTYPE CalloutType
    );

typedef enum _PSPOWEREVENTTYPE {
    PsW32FullWake,
    PsW32EventCode,
    PsW32PowerPolicyChanged,
    PsW32SystemPowerState,
    PsW32SystemTime,
    PsW32DisplayState,
    PsW32CapabilitiesChanged,
    PsW32SetStateFailed,
    PsW32GdiOff,
    PsW32GdiOn
} PSPOWEREVENTTYPE;

typedef struct _WIN32_POWEREVENT_PARAMETERS {
    PSPOWEREVENTTYPE EventNumber;
    ULONG_PTR Code;
} WIN32_POWEREVENT_PARAMETERS, *PKWIN32_POWEREVENT_PARAMETERS;

typedef struct _WIN32_POWERSTATE_PARAMETERS {
    BOOLEAN Promotion;
    POWER_ACTION SystemAction;
    SYSTEM_POWER_STATE MinSystemState;
    ULONG Flags;
} WIN32_POWERSTATE_PARAMETERS, *PKWIN32_POWERSTATE_PARAMETERS;

typedef
NTSTATUS
(*PKWIN32_POWEREVENT_CALLOUT) (
    IN PKWIN32_POWEREVENT_PARAMETERS Parm
    );

typedef
NTSTATUS
(*PKWIN32_POWERSTATE_CALLOUT) (
    IN PKWIN32_POWERSTATE_PARAMETERS Parm
    );


NTKERNELAPI
VOID
PsEstablishWin32Callouts(
    IN PKWIN32_PROCESS_CALLOUT ProcessCallout,
    IN PKWIN32_THREAD_CALLOUT ThreadCallout,
    IN PKWIN32_GLOBALATOMTABLE_CALLOUT GlobalAtomTableCallout,
    IN PKWIN32_POWEREVENT_CALLOUT PowerEventCallout,
    IN PKWIN32_POWERSTATE_CALLOUT PowerStateCallout,
    IN PKWIN32_JOB_CALLOUT JobCallout,
    IN PVOID BatchFlushRoutine
    );

typedef enum _PSPROCESSPRIORITYMODE {
    PsProcessPriorityBackground,
    PsProcessPriorityForeground,
    PsProcessPrioritySpinning
} PSPROCESSPRIORITYMODE;

NTKERNELAPI
VOID
PsSetProcessPriorityByClass(
    IN PEPROCESS Process,
    IN PSPROCESSPRIORITYMODE PriorityMode
    );

#if DEVL
NTSTATUS
PsWatchWorkingSet(
    IN NTSTATUS Status,
    IN PVOID PcValue,
    IN PVOID Va
    );

#endif // DEVL

// begin_ntddk begin_nthal begin_ntifs

HANDLE
PsGetCurrentProcessId( VOID );

HANDLE
PsGetCurrentThreadId( VOID );

BOOLEAN
PsGetVersion(
    PULONG MajorVersion OPTIONAL,
    PULONG MinorVersion OPTIONAL,
    PULONG BuildNumber OPTIONAL,
    PUNICODE_STRING CSDVersion OPTIONAL
    );

// end_ntddk end_nthal end_ntifs

#endif // _PS_
