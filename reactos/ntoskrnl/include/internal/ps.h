#ifndef __INCLUDE_INTERNAL_PS_H
#define __INCLUDE_INTERNAL_PS_H

/* Forward declarations. */
struct _KTHREAD;
struct _KTRAPFRAME;
struct _EJOB;

#include <internal/arch/ps.h>

/* Top level irp definitions. */
#define	FSRTL_FSP_TOP_LEVEL_IRP         (0x01)
#define	FSRTL_CACHE_TOP_LEVEL_IRP       (0x02)
#define	FSRTL_MOD_WRITE_TOP_LEVEL_IRP   (0x03)
#define	FSRTL_FAST_IO_TOP_LEVEL_IRP     (0x04)
#define	FSRTL_MAX_TOP_LEVEL_IRP_FLAG    (0x04)

#define PSP_MAX_CREATE_THREAD_NOTIFY            8
#define PSP_MAX_LOAD_IMAGE_NOTIFY               8
#define PSP_MAX_CREATE_PROCESS_NOTIFY           8

#define PSP_JOB_SCHEDULING_CLASSES              10

VOID
NTAPI
PspShutdownProcessManager(VOID);

VOID
NTAPI
PsInitThreadManagment(VOID);

VOID
INIT_FUNCTION
NTAPI
PiInitProcessManager(VOID);

VOID
NTAPI
PsInitProcessManagment(VOID);

VOID
NTAPI
PsInitIdleThread(VOID);

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

NTSTATUS
NTAPI
PspSetPrimaryToken(
    IN PEPROCESS Process,
    IN HANDLE TokenHandle OPTIONAL,
    IN PTOKEN Token OPTIONAL
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

PEPROCESS
STDCALL
PsGetNextProcess(PEPROCESS OldProcess);

VOID
STDCALL
PsIdleThreadMain(PVOID Context);

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

extern LCID PsDefaultThreadLocaleId;
extern LCID PsDefaultSystemLocaleId;
extern LIST_ENTRY PspReaperListHead;
extern WORK_QUEUE_ITEM PspReaperWorkItem;
extern BOOLEAN PspReaping;
extern PEPROCESS PsInitialSystemProcess;
extern PEPROCESS PsIdleProcess;
extern LIST_ENTRY PsActiveProcessHead;
extern KGUARDED_MUTEX PspActiveProcessMutex;
extern LARGE_INTEGER ShortPsLockDelay;
extern EPROCESS_QUOTA_BLOCK PspDefaultQuotaBlock;
extern PHANDLE_TABLE PspCidTable;
extern PCREATE_THREAD_NOTIFY_ROUTINE
PspThreadNotifyRoutine[PSP_MAX_CREATE_THREAD_NOTIFY];
extern PCREATE_PROCESS_NOTIFY_ROUTINE
PspProcessNotifyRoutine[PSP_MAX_CREATE_PROCESS_NOTIFY];
extern PLOAD_IMAGE_NOTIFY_ROUTINE
PspLoadImageNotifyRoutine[PSP_MAX_LOAD_IMAGE_NOTIFY];
extern PLEGO_NOTIFY_ROUTINE PspLegoNotifyRoutine;
extern ULONG PspThreadNotifyRoutineCount;
extern PKWIN32_PROCESS_CALLOUT PspW32ProcessCallout;
extern PKWIN32_THREAD_CALLOUT PspW32ThreadCallout;
extern PVOID PspSystemDllEntryPoint;
extern PVOID PspSystemDllBase;
extern BOOLEAN PspUseJobSchedulingClasses;
extern CHAR PspJobSchedulingClasses[PSP_JOB_SCHEDULING_CLASSES];
#include "ps_x.h"

#endif /* __INCLUDE_INTERNAL_PS_H */
