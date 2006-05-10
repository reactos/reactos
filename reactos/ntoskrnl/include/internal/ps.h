#ifndef __INCLUDE_INTERNAL_PS_H
#define __INCLUDE_INTERNAL_PS_H

/* Forward declarations. */
struct _KTHREAD;
struct _KTRAPFRAME;
struct _EJOB;

#include <internal/arch/ps.h>

//
// ROS Process
//
#include <pshpack4.h>
typedef struct _ROS_EPROCESS
{
    KPROCESS Pcb;
    EX_PUSH_LOCK ProcessLock;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    EX_RUNDOWN_REF RundownProtect;
    HANDLE UniqueProcessId;
    LIST_ENTRY ActiveProcessLinks;
    ULONG QuotaUsage[3];
    ULONG QuotaPeak[3];
    ULONG CommitCharge;
    ULONG PeakVirtualSize;
    ULONG VirtualSize;
    LIST_ENTRY SessionProcessLinks;
    PVOID DebugPort;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    union
    {
        PVOID ExceptionPortData;
        ULONG ExceptionPortValue;
        UCHAR ExceptionPortState:3;
    };
#else
    PVOID ExceptionPort;
#endif
    PHANDLE_TABLE ObjectTable;
    EX_FAST_REF Token;
    ULONG WorkingSetPage;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    EX_PUSH_LOCK AddressCreationLock;
    PETHREAD RotateInProgress;
#else
    KGUARDED_MUTEX AddressCreationLock;
    KSPIN_LOCK HyperSpaceLock;
#endif
    PETHREAD ForkInProgress;
    ULONG HardwareTrigger;
    MM_AVL_TABLE PhysicalVadroot;
    PVOID CloneRoot;
    ULONG NumberOfPrivatePages;
    ULONG NumberOfLockedPages;
    PVOID *Win32Process;
    struct _EJOB *Job;
    PVOID SectionObject;
    PVOID SectionBaseAddress;
    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    PPAGEFAULT_HISTORY WorkingSetWatch;
    PVOID Win32WindowStation;
    HANDLE InheritedFromUniqueProcessId;
    PVOID LdtInformation;
    PVOID VadFreeHint;
    PVOID VdmObjects;
    PVOID DeviceMap;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG AlpcPagedPoolQuotaCache;
    PVOID EtwDataSource;
    PVOID FreeTebHint;
#else
    PVOID Spare0[3];
#endif
    union
    {
        HARDWARE_PTE_X86 PagedirectoryPte;
        ULONGLONG Filler;
    };
    ULONG Session;
    CHAR ImageFileName[16];
    LIST_ENTRY JobLinks;
    PVOID LockedPagesList;
    LIST_ENTRY ThreadListHead;
    PVOID SecurityPort;
    PVOID PaeTop;
    ULONG ActiveThreads;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG ImagePathHash;
#else
    ACCESS_MASK GrantedAccess;
#endif
    ULONG DefaultHardErrorProcessing;
    NTSTATUS LastThreadExitStatus;
    struct _PEB* Peb;
    EX_FAST_REF PrefetchTrace;
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
    ULONG CommitChargeLimit;
    ULONG CommitChargePeak;
    PVOID AweInfo;
    SE_AUDIT_PROCESS_CREATION_INFO SeAuditProcessCreationInfo;
    MMSUPPORT Vm;
    LIST_ENTRY MmProcessLinks;
    ULONG ModifiedPageCount;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    union
    {
        struct
        {
            ULONG JobNotReallyActive:1;
            ULONG AccountingFolded:1;
            ULONG NewProcessReported:1;
            ULONG ExitProcessReported:1;
            ULONG ReportCommitChanges:1;
            ULONG LastReportMemory:1;
            ULONG ReportPhysicalPageChanges:1;
            ULONG HandleTableRundown:1;
            ULONG NeedsHandleRundown:1;
            ULONG RefTraceEnabled:1;
            ULONG NumaAware:1;
            ULONG ProtectedProcess:1;
            ULONG DefaultPagePriority:3;
            ULONG ProcessDeleteSelf:1;
            ULONG ProcessVerifierTarget:1;
        };
        ULONG Flags2;
    };
#else
    ULONG JobStatus;
#endif
    union
    {
        struct
        {
            ULONG CreateReported:1;
            ULONG NoDebugInherit:1;
            ULONG ProcessExiting:1;
            ULONG ProcessDelete:1;
            ULONG Wow64SplitPages:1;
            ULONG VmDeleted:1;
            ULONG OutswapEnabled:1;
            ULONG Outswapped:1;
            ULONG ForkFailed:1;
            ULONG Wow64VaSpace4Gb:1;
            ULONG AddressSpaceInitialized:2;
            ULONG SetTimerResolution:1;
            ULONG BreakOnTermination:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
            ULONG DeprioritizeViews:1;
#else
            ULONG SessionCreationUnderway:1;
#endif
            ULONG WriteWatch:1;
            ULONG ProcessInSession:1;
            ULONG OverrideAddressSpace:1;
            ULONG HasAddressSpace:1;
            ULONG LaunchPrefetched:1;
            ULONG InjectInpageErrors:1;
            ULONG VmTopDown:1;
            ULONG ImageNotifyDone:1;
            ULONG PdeUpdateNeeded:1;
            ULONG VdmAllowed:1;
            ULONG SmapAllowed:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
            ULONG ProcessInserted:1;
#else
            ULONG CreateFailed:1;
#endif
            ULONG DefaultIoPriority:3;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
            ULONG SparePsFlags1:2;
#else
            ULONG Spare1:1;
            ULONG Spare2:1;
#endif
        };
        ULONG Flags;
    };
    NTSTATUS ExitStatus;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    USHORT Spare7;
#else
    USHORT NextPageColor;
#endif
    union
    {
        struct
        {
            UCHAR SubSystemMinorVersion;
            UCHAR SubSystemMajorVersion;
        };
        USHORT SubSystemVersion;
    };
    UCHAR PriorityClass;
    MM_AVL_TABLE VadRoot;
    ULONG Cookie;
    KEVENT LockEvent;
    ULONG LockCount;
    struct _KTHREAD *LockOwner;
    MADDRESS_SPACE AddressSpace;
} ROS_EPROCESS, *PROS_EPROCESS;
#include <poppack.h>

extern LCID PsDefaultThreadLocaleId;
extern LCID PsDefaultSystemLocaleId;
extern LIST_ENTRY PspReaperListHead;
extern WORK_QUEUE_ITEM PspReaperWorkItem;
extern BOOLEAN PspReaping;
extern PEPROCESS PsInitialSystemProcess;
extern PEPROCESS PsIdleProcess;
extern LIST_ENTRY PsActiveProcessHead;
extern FAST_MUTEX PspActiveProcessMutex;
extern LARGE_INTEGER ShortPsLockDelay, PsLockTimeout;
extern EPROCESS_QUOTA_BLOCK PspDefaultQuotaBlock;

/* Top level irp definitions. */
#define	FSRTL_FSP_TOP_LEVEL_IRP         (0x01)
#define	FSRTL_CACHE_TOP_LEVEL_IRP       (0x02)
#define	FSRTL_MOD_WRITE_TOP_LEVEL_IRP   (0x03)
#define	FSRTL_FAST_IO_TOP_LEVEL_IRP     (0x04)
#define	FSRTL_MAX_TOP_LEVEL_IRP_FLAG    (0x04)

#define MAX_PROCESS_NOTIFY_ROUTINE_COUNT    8
#define MAX_LOAD_IMAGE_NOTIFY_ROUTINE_COUNT  8

VOID
NTAPI
PiInitDefaultLocale(VOID);

VOID
NTAPI
PiInitProcessManager(VOID);

VOID
NTAPI
PiShutdownProcessManager(VOID);

VOID
NTAPI
PsInitThreadManagment(VOID);

VOID
NTAPI
PsInitProcessManagment(VOID);

VOID
NTAPI
PsInitIdleThread(VOID);

VOID
NTAPI
PiTerminateProcessThreads(
    PEPROCESS Process,
    NTSTATUS ExitStatus
);

VOID
NTAPI
PsTerminateCurrentThread(NTSTATUS ExitStatus);

VOID
NTAPI
PsTerminateOtherThread(
    PETHREAD Thread,
    NTSTATUS ExitStatus
);

VOID
NTAPI
PsReleaseThread(PETHREAD Thread);

VOID
NTAPI
PsBeginThread(
    PKSTART_ROUTINE StartRoutine,
    PVOID StartContext
);

VOID
NTAPI
PsBeginThreadWithContextInternal(VOID);

VOID
NTAPI
PiKillMostProcesses(VOID);

NTSTATUS
STDCALL
PiTerminateProcess(
    PEPROCESS Process,
    NTSTATUS ExitStatus
);

VOID
NTAPI
PiInitApcManagement(VOID);

VOID
STDCALL
PiDeleteThread(PVOID ObjectBody);

VOID
NTAPI
PsReapThreads(VOID);

VOID
NTAPI
PsInitializeThreadReaper(VOID);

VOID
NTAPI
PsQueueThreadReap(PETHREAD Thread);

NTSTATUS
NTAPI
PsInitializeThread(
    PEPROCESS Process,
    PETHREAD* ThreadPtr,
    POBJECT_ATTRIBUTES ObjectAttributes,
    KPROCESSOR_MODE AccessMode,
    BOOLEAN First
);

PACCESS_TOKEN
STDCALL
PsReferenceEffectiveToken(
    PETHREAD Thread,
    PTOKEN_TYPE TokenType,
    PUCHAR b,
    PSECURITY_IMPERSONATION_LEVEL Level
);

NTSTATUS
STDCALL
PsOpenTokenOfProcess(
    HANDLE ProcessHandle,
    PACCESS_TOKEN* Token
);

VOID
STDCALL
PspTerminateProcessThreads(
    PEPROCESS Process,
    NTSTATUS ExitStatus
);

NTSTATUS
NTAPI
PsSuspendThread(
    PETHREAD Thread,
    PULONG PreviousCount
);

NTSTATUS
NTAPI
PsResumeThread(
    PETHREAD Thread,
    PULONG PreviousCount
);

NTSTATUS
STDCALL
PspAssignPrimaryToken(
    PEPROCESS Process,
    HANDLE TokenHandle
);

VOID
STDCALL
PsExitSpecialApc(
    PKAPC Apc,
    PKNORMAL_ROUTINE *NormalRoutine,
    PVOID *NormalContext,
    PVOID *SystemArgument1,
    PVOID *SystemArgument2
);

NTSTATUS
STDCALL
PspInitializeProcessSecurity(
    PEPROCESS Process,
    PEPROCESS Parent OPTIONAL
);

VOID
STDCALL
PspSystemThreadStartup(
    PKSTART_ROUTINE StartRoutine,
    PVOID StartContext
);

NTSTATUS
NTAPI
PsInitializeIdleOrFirstThread(
    PEPROCESS Process,
    PETHREAD* ThreadPtr,
    PKSTART_ROUTINE StartRoutine,
    KPROCESSOR_MODE AccessMode,
    BOOLEAN First
);

VOID
STDCALL
PiDeleteProcess(PVOID ObjectBody);

VOID
STDCALL
PspReapRoutine(PVOID Context);

VOID
STDCALL
PspExitThread(NTSTATUS ExitStatus);

VOID
STDCALL
PspTerminateThreadByPointer(
    PETHREAD Thread,
    NTSTATUS ExitStatus
);

VOID
NTAPI
PsUnfreezeOtherThread(PETHREAD Thread);

VOID
NTAPI
PsFreezeOtherThread(PETHREAD Thread);

VOID
NTAPI
PsFreezeProcessThreads(PEPROCESS Process);

VOID
NTAPI
PsUnfreezeProcessThreads(PEPROCESS Process);

ULONG
NTAPI
PsEnumThreadsByProcess(PEPROCESS Process);

PEPROCESS
STDCALL
PsGetNextProcess(PEPROCESS OldProcess);

VOID
NTAPI
PsApplicationProcessorInit(VOID);

VOID
NTAPI
PsPrepareForApplicationProcessorInit(ULONG Id);

VOID
STDCALL
PsIdleThreadMain(PVOID Context);

VOID
STDCALL
PiSuspendThreadRundownRoutine(PKAPC Apc);

VOID
STDCALL
PiSuspendThreadKernelRoutine(
    PKAPC Apc,
    PKNORMAL_ROUTINE* NormalRoutine,
    PVOID* NormalContext,
    PVOID* SystemArgument1,
    PVOID* SystemArguemnt2
);

VOID
STDCALL
PiSuspendThreadNormalRoutine(
    PVOID NormalContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
);

VOID
NTAPI
PsInitialiseSuspendImplementation(VOID);

NTSTATUS
STDCALL
PspExitProcess(PEPROCESS Process);

VOID
STDCALL
PspDeleteProcess(PVOID ObjectBody);

VOID
STDCALL
PspDeleteThread(PVOID ObjectBody);


NTSTATUS
NTAPI
PsInitWin32Thread(PETHREAD Thread);

VOID
NTAPI
PsTerminateWin32Process(PEPROCESS Process);

VOID
NTAPI
PsTerminateWin32Thread(PETHREAD Thread);

VOID
NTAPI
PsInitialiseW32Call(VOID);

VOID
STDCALL
PspRunCreateThreadNotifyRoutines(
    PETHREAD,
    BOOLEAN
);

VOID
STDCALL
PspRunCreateProcessNotifyRoutines(
    PEPROCESS,
    BOOLEAN
);

VOID
STDCALL
PspRunLegoRoutine(IN PKTHREAD Thread);

VOID
NTAPI
INIT_FUNCTION
PsInitJobManagment(VOID);

VOID
STDCALL
PspInheritQuota(
    PEPROCESS Process,
    PEPROCESS ParentProcess
);

VOID
STDCALL
PspDestroyQuotaBlock(PEPROCESS Process);

NTSTATUS
STDCALL
PspMapSystemDll(
    PEPROCESS Process,
    PVOID *DllBase
);

NTSTATUS
STDCALL
PsLocateSystemDll(VOID);

NTSTATUS
STDCALL
PspGetSystemDllEntryPoints(VOID);

NTSTATUS 
NTAPI
PsLockProcess(
    PROS_EPROCESS Process,
    BOOLEAN Timeout
);

VOID
NTAPI
PsUnlockProcess(PROS_EPROCESS Process);

#endif /* __INCLUDE_INTERNAL_PS_H */
