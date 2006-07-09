#ifndef __INCLUDE_INTERNAL_PS_H
#define __INCLUDE_INTERNAL_PS_H

/* Forward declarations. */
struct _KTHREAD;
struct _KTRAPFRAME;
struct _EJOB;

#include <internal/arch/ps.h>

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
PspShutdownProcessManager(VOID);

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

PETHREAD
NTAPI
PsGetNextProcessThread(
    IN PEPROCESS Process,
    IN PETHREAD Thread OPTIONAL
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

NTSTATUS
STDCALL
PspTerminateThreadByPointer(
    PETHREAD Thread,
    NTSTATUS ExitStatus,
    BOOLEAN bSelf
);

NTSTATUS
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

VOID
STDCALL
PspExitProcess(BOOLEAN LastThread,
               PEPROCESS Process);

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
    PEPROCESS Process,
    BOOLEAN Timeout
);

VOID
NTAPI
PsUnlockProcess(PEPROCESS Process);

VOID
NTAPI
PspRemoveProcessFromJob(
    IN PEPROCESS Process,
    IN PEJOB Job
);

NTSTATUS
NTAPI
PspDeleteLdt(IN PEPROCESS Process);

NTSTATUS
NTAPI
PspDeleteVdmObjects(IN PEPROCESS Process);

VOID
NTAPI
PspDeleteProcessSecurity(IN PEPROCESS Process);

VOID
NTAPI
PspDeleteThreadSecurity(IN PETHREAD Thread);

VOID
NTAPI
PspExitProcessFromJob(
    IN PEJOB Job,
    IN PEPROCESS Process
);

#endif /* __INCLUDE_INTERNAL_PS_H */
