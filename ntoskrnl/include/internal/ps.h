/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/internal/ps.h
 * PURPOSE:         Internal header for the Process Manager
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

//
// Define this if you want debugging support
//
#define _PS_DEBUG_                                      0x00

//
// These define the Debug Masks Supported
//
#define PS_THREAD_DEBUG                                 0x01
#define PS_PROCESS_DEBUG                                0x02
#define PS_SECURITY_DEBUG                               0x04
#define PS_JOB_DEBUG                                    0x08
#define PS_NOTIFICATIONS_DEBUG                          0x10
#define PS_WIN32K_DEBUG                                 0x20
#define PS_STATE_DEBUG                                  0x40
#define PS_QUOTA_DEBUG                                  0x80
#define PS_KILL_DEBUG                                   0x100
#define PS_REF_DEBUG                                    0x200

//
// Debug/Tracing support
//
#if _PS_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define PSTRACE(x, ...)                                     \
    {                                                       \
        DbgPrintEx("%s [%.16s] - ",                         \
                   __FUNCTION__,                            \
                   PsGetCurrentProcess()->ImageFileName);   \
        DbgPrintEx(__VA_ARGS__);                            \
    }
#else
#define PSTRACE(x, ...)                                     \
    if (x & PspTraceLevel)                                  \
    {                                                       \
        DbgPrint("%s [%.16s] - ",                           \
                 __FUNCTION__,                              \
                 PsGetCurrentProcess()->ImageFileName);     \
        DbgPrint(__VA_ARGS__);                              \
    }
#endif
#define PSREFTRACE(x)                                       \
    PSTRACE(PS_REF_DEBUG,                                   \
            "Pointer Count [%p] @%d: %lx\n",                \
            x,                                              \
            __LINE__,                                       \
            OBJECT_TO_OBJECT_HEADER(x)->PointerCount)
#else
#define PSTRACE(x, fmt, ...) DPRINT(fmt, ##__VA_ARGS__)
#define PSREFTRACE(x)
#endif

//
// Maximum Count of Notification Routines
//
#define PSP_MAX_CREATE_THREAD_NOTIFY            8
#define PSP_MAX_LOAD_IMAGE_NOTIFY               8
#define PSP_MAX_CREATE_PROCESS_NOTIFY           8

//
// Maximum Job Scheduling Classes
//
#define PSP_JOB_SCHEDULING_CLASSES              10

//
// Process Quota Threshold Values
//
#define PSP_NON_PAGED_POOL_QUOTA_THRESHOLD              0x10000
#define PSP_PAGED_POOL_QUOTA_THRESHOLD                  0x80000

//
// Thread "Set/Get Context" Context Structure
//
typedef struct _GET_SET_CTX_CONTEXT
{
    KAPC Apc;
    KEVENT Event;
    KPROCESSOR_MODE Mode;
    CONTEXT Context;
} GET_SET_CTX_CONTEXT, *PGET_SET_CTX_CONTEXT;

//
// Initialization Functions
//
VOID
NTAPI
PspShutdownProcessManager(
    VOID
);

CODE_SEG("INIT")
BOOLEAN
NTAPI
PsInitSystem(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

//
// Utility Routines
//
PETHREAD
NTAPI
PsGetNextProcessThread(
    IN PEPROCESS Process,
    IN PETHREAD Thread OPTIONAL
);

PEPROCESS
NTAPI
PsGetNextProcess(
    IN PEPROCESS OldProcess OPTIONAL
);

NTSTATUS
NTAPI
PspMapSystemDll(
    IN PEPROCESS Process,
    OUT PVOID *DllBase,
    IN BOOLEAN UseLargePages
);

CODE_SEG("INIT")
NTSTATUS
NTAPI
PsLocateSystemDll(
    VOID
);

NTSTATUS
NTAPI
PspGetSystemDllEntryPoints(
    VOID
);

VOID
NTAPI
PsChangeQuantumTable(
    IN BOOLEAN Immediate,
    IN ULONG PrioritySeparation
);

NTSTATUS
NTAPI
PsReferenceProcessFilePointer(
    IN PEPROCESS Process,
    OUT PFILE_OBJECT *FileObject
);

//
// Process Routines
//
NTSTATUS
NTAPI
PspCreateProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess OPTIONAL,
    IN ULONG Flags,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL,
    IN BOOLEAN InJob
);

//
// Security Routines
//
PACCESS_TOKEN
NTAPI
PsReferenceEffectiveToken(
    IN PETHREAD Thread,
    OUT IN PTOKEN_TYPE TokenType,
    OUT PBOOLEAN EffectiveOnly,
    OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel
);

NTSTATUS
NTAPI
PsOpenTokenOfProcess(
    IN HANDLE ProcessHandle,
    OUT PACCESS_TOKEN* Token
);

NTSTATUS
NTAPI
PspSetPrimaryToken(
    IN PEPROCESS Process,
    IN HANDLE TokenHandle OPTIONAL,
    IN PACCESS_TOKEN Token OPTIONAL
);

NTSTATUS
NTAPI
PspInitializeProcessSecurity(
    IN PEPROCESS Process,
    IN PEPROCESS Parent OPTIONAL
);

VOID
NTAPI
PspDeleteProcessSecurity(
    IN PEPROCESS Process
);

VOID
NTAPI
PspDeleteThreadSecurity(
    IN PETHREAD Thread
);

//
// Reaping and Deletion
//
VOID
NTAPI
PsExitSpecialApc(
    PKAPC Apc,
    PKNORMAL_ROUTINE *NormalRoutine,
    PVOID *NormalContext,
    PVOID *SystemArgument1,
    PVOID *SystemArgument2
);

VOID
NTAPI
PspReapRoutine(
    IN PVOID Context
);

VOID
NTAPI
PspExitThread(
    IN NTSTATUS ExitStatus
);

NTSTATUS
NTAPI
PspTerminateThreadByPointer(
    IN PETHREAD Thread,
    IN NTSTATUS ExitStatus,
    IN BOOLEAN bSelf
);

VOID
NTAPI
PspExitProcess(
    IN BOOLEAN LastThread,
    IN PEPROCESS Process
);

NTSTATUS
NTAPI
PsTerminateProcess(
    IN PEPROCESS Process,
    IN NTSTATUS ExitStatus
);

VOID
NTAPI
PspDeleteProcess(
    IN PVOID ObjectBody
);

VOID
NTAPI
PspDeleteThread(
    IN PVOID ObjectBody
);

//
// Thread/Process Startup
//
VOID
NTAPI
PspSystemThreadStartup(
    PKSTART_ROUTINE StartRoutine,
    PVOID StartContext
);

VOID
NTAPI
PsIdleThreadMain(
    IN PVOID Context
);

//
// Quota Support
//
VOID
NTAPI
PspInheritQuota(
    _In_ PEPROCESS Process,
    _In_ PEPROCESS ParentProcess
);

VOID
NTAPI
PspDereferenceQuotaBlock(
    _In_opt_ PEPROCESS Process,
    _In_ PEPROCESS_QUOTA_BLOCK QuotaBlock
);

NTSTATUS
NTAPI
PsReturnProcessPageFileQuota(
    _In_ PEPROCESS Process,
    _In_ SIZE_T Amount
);

NTSTATUS
NTAPI
PsChargeProcessPageFileQuota(
    _In_ PEPROCESS Process,
    _In_ SIZE_T Amount
);

VOID
NTAPI
PsReturnSharedPoolQuota(
    _In_ PEPROCESS_QUOTA_BLOCK QuotaBlock,
    _In_ SIZE_T AmountToReturnPaged,
    _In_ SIZE_T AmountToReturnNonPaged
);

PEPROCESS_QUOTA_BLOCK
NTAPI
PsChargeSharedPoolQuota(
    _In_ PEPROCESS Process,
    _In_ SIZE_T AmountToChargePaged,
    _In_ SIZE_T AmountToChargeNonPaged
);

NTSTATUS
NTAPI
PspSetQuotaLimits(
    _In_ PEPROCESS Process,
    _In_ ULONG Unused,
    _In_ PVOID QuotaLimits,
    _In_ ULONG QuotaLimitsLength,
    _In_ KPROCESSOR_MODE PreviousMode);

#if defined(_X86_)
//
// VDM and LDT Support
//
VOID
NTAPI
PspDeleteLdt(
    IN PEPROCESS Process
);

VOID
NTAPI
PspDeleteVdmObjects(
    IN PEPROCESS Process
);

NTSTATUS
NTAPI
PspQueryDescriptorThread(
    IN PETHREAD Thread,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);
#endif

//
// Job Routines
//
VOID
NTAPI
PspExitProcessFromJob(
    IN PEJOB Job,
    IN PEPROCESS Process
);

VOID
NTAPI
PspRemoveProcessFromJob(
    IN PEPROCESS Process,
    IN PEJOB Job
);

CODE_SEG("INIT")
VOID
NTAPI
PspInitializeJobStructures(
    VOID
);

VOID
NTAPI
PspDeleteJob(
    IN PVOID ObjectBody
);

//
// State routines
//
NTSTATUS
NTAPI
PsResumeThread(
    IN PETHREAD Thread,
    OUT PULONG PreviousCount OPTIONAL
);

NTSTATUS
NTAPI
PsSuspendThread(
    IN PETHREAD Thread,
    OUT PULONG PreviousCount OPTIONAL
);

VOID
NTAPI
PspGetOrSetContextKernelRoutine(
    IN PKAPC Apc,
    IN OUT PKNORMAL_ROUTINE* NormalRoutine,
    IN OUT PVOID* NormalContext,
    IN OUT PVOID* SystemArgument1,
    IN OUT PVOID* SystemArgument2
);

BOOLEAN
NTAPI
PspIsProcessExiting(IN PEPROCESS Process);

//
// Apphelp functions
//
CODE_SEG("INIT")
NTSTATUS
NTAPI
ApphelpCacheInitialize(VOID);

VOID
NTAPI
ApphelpCacheShutdown(VOID);

//
// Global data inside the Process Manager
//
extern ULONG PspTraceLevel;
extern LCID PsDefaultThreadLocaleId;
extern LCID PsDefaultSystemLocaleId;
extern LIST_ENTRY PspReaperListHead;
extern WORK_QUEUE_ITEM PspReaperWorkItem;
extern BOOLEAN PspReaping;
extern PEPROCESS PsIdleProcess;
extern LIST_ENTRY PsActiveProcessHead;
extern KGUARDED_MUTEX PspActiveProcessMutex;
extern LARGE_INTEGER ShortPsLockDelay;
extern EPROCESS_QUOTA_BLOCK PspDefaultQuotaBlock;
extern PHANDLE_TABLE PspCidTable;
extern EX_CALLBACK PspThreadNotifyRoutine[PSP_MAX_CREATE_THREAD_NOTIFY];
extern EX_CALLBACK PspProcessNotifyRoutine[PSP_MAX_CREATE_PROCESS_NOTIFY];
extern EX_CALLBACK PspLoadImageNotifyRoutine[PSP_MAX_LOAD_IMAGE_NOTIFY];
extern PLEGO_NOTIFY_ROUTINE PspLegoNotifyRoutine;
extern ULONG PspThreadNotifyRoutineCount, PspProcessNotifyRoutineCount;
extern BOOLEAN PsImageNotifyEnabled;
extern PKWIN32_PROCESS_CALLOUT PspW32ProcessCallout;
extern PKWIN32_THREAD_CALLOUT PspW32ThreadCallout;
extern PVOID PspSystemDllEntryPoint;
extern PVOID PspSystemDllBase;
extern BOOLEAN PspUseJobSchedulingClasses;
extern CHAR PspJobSchedulingClasses[PSP_JOB_SCHEDULING_CLASSES];
extern ULONG PsRawPrioritySeparation;
extern ULONG PsPrioritySeparation;
extern POBJECT_TYPE _PsThreadType, _PsProcessType;
extern PTOKEN PspBootAccessToken;
extern GENERIC_MAPPING PspJobMapping;
extern POBJECT_TYPE PsJobType;
extern LARGE_INTEGER ShortPsLockDelay;
extern UNICODE_STRING PsNtDllPathName;
extern LIST_ENTRY PsLoadedModuleList;
extern KSPIN_LOCK PsLoadedModuleSpinLock;
extern ERESOURCE PsLoadedModuleResource;
extern ULONG_PTR PsNtosImageBase;

//
// Inlined Functions
//
#include "ps_x.h"
