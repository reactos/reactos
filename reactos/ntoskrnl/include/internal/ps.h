#ifndef __INCLUDE_INTERNAL_PS_H
#define __INCLUDE_INTERNAL_PS_H

/* Forward declarations. */
struct _KTHREAD;
struct _KTRAPFRAME;
struct _EJOB;

#include <internal/arch/ps.h>

extern LCID PsDefaultThreadLocaleId;
extern LCID PsDefaultSystemLocaleId;

/* Top level irp definitions. */
#define	FSRTL_FSP_TOP_LEVEL_IRP			(0x01)
#define	FSRTL_CACHE_TOP_LEVEL_IRP		(0x02)
#define	FSRTL_MOD_WRITE_TOP_LEVEL_IRP		(0x03)
#define	FSRTL_FAST_IO_TOP_LEVEL_IRP		(0x04)
#define	FSRTL_MAX_TOP_LEVEL_IRP_FLAG		(0x04)

#define PROCESS_STATE_TERMINATED (1)
#define PROCESS_STATE_ACTIVE     (2)

VOID PiInitDefaultLocale(VOID);
VOID PiInitProcessManager(VOID);
VOID PiShutdownProcessManager(VOID);
VOID PsInitThreadManagment(VOID);
VOID PsInitProcessManagment(VOID);
VOID PsInitIdleThread(VOID);
VOID PiTerminateProcessThreads(PEPROCESS Process, NTSTATUS ExitStatus);
VOID PsTerminateCurrentThread(NTSTATUS ExitStatus);
VOID PsTerminateOtherThread(PETHREAD Thread, NTSTATUS ExitStatus);
VOID PsReleaseThread(PETHREAD Thread);
VOID PsBeginThread(PKSTART_ROUTINE StartRoutine, PVOID StartContext);
VOID PsBeginThreadWithContextInternal(VOID);
VOID PiKillMostProcesses(VOID);
NTSTATUS STDCALL PiTerminateProcess(PEPROCESS Process, NTSTATUS ExitStatus);
VOID PiInitApcManagement(VOID);
VOID STDCALL PiDeleteThread(PVOID ObjectBody);
VOID PsReapThreads(VOID);
VOID PsInitializeThreadReaper(VOID);
VOID PsQueueThreadReap(PETHREAD Thread);
NTSTATUS
PsInitializeThread(PEPROCESS Process,
		   PETHREAD* ThreadPtr,
		   POBJECT_ATTRIBUTES ObjectAttributes,
		   KPROCESSOR_MODE AccessMode,
		   BOOLEAN First);

PACCESS_TOKEN STDCALL PsReferenceEffectiveToken(PETHREAD Thread,
					PTOKEN_TYPE TokenType,
					PUCHAR b,
					PSECURITY_IMPERSONATION_LEVEL Level);

NTSTATUS STDCALL PsOpenTokenOfProcess(HANDLE ProcessHandle,
			      PACCESS_TOKEN* Token);
VOID
STDCALL
PspTerminateProcessThreads(PEPROCESS Process,
                           NTSTATUS ExitStatus);
NTSTATUS PsSuspendThread(PETHREAD Thread, PULONG PreviousCount);
NTSTATUS PsResumeThread(PETHREAD Thread, PULONG PreviousCount);
NTSTATUS
STDCALL
PspAssignPrimaryToken(PEPROCESS Process,
                      HANDLE TokenHandle);
VOID STDCALL PsExitSpecialApc(PKAPC Apc,
		      PKNORMAL_ROUTINE *NormalRoutine,
		      PVOID *NormalContext,
		      PVOID *SystemArgument1,
		      PVOID *SystemArgument2);

NTSTATUS
STDCALL
PspInitializeProcessSecurity(PEPROCESS Process,
                             PEPROCESS Parent OPTIONAL);


VOID
STDCALL
PspSystemThreadStartup(PKSTART_ROUTINE StartRoutine,
                       PVOID StartContext);

NTSTATUS
PsInitializeIdleOrFirstThread (
    PEPROCESS Process,
    PETHREAD* ThreadPtr,
    PKSTART_ROUTINE StartRoutine,
    KPROCESSOR_MODE AccessMode,
    BOOLEAN First);
/*
 * Internal thread priorities, added by Phillip Susi
 * TODO: rebalence these to make use of all priorities... the ones above 16
 * can not all be used right now
 */
#define PROCESS_PRIO_IDLE			3
#define PROCESS_PRIO_NORMAL			8
#define PROCESS_PRIO_HIGH			13
#define PROCESS_PRIO_RT				18


VOID STDCALL PiDeleteProcess(PVOID ObjectBody);

VOID
STDCALL
PspReapRoutine(PVOID Context);

VOID
STDCALL
PspExitThread(NTSTATUS ExitStatus);

extern LIST_ENTRY PspReaperListHead;
extern WORK_QUEUE_ITEM PspReaperWorkItem;
extern BOOLEAN PspReaping;
extern PEPROCESS PsInitialSystemProcess;
extern PEPROCESS PsIdleProcess;
extern LIST_ENTRY PsActiveProcessHead;
extern FAST_MUTEX PspActiveProcessMutex;
extern LARGE_INTEGER ShortPsLockDelay, PsLockTimeout;
extern EPROCESS_QUOTA_BLOCK PspDefaultQuotaBlock;

VOID
STDCALL
PspTerminateThreadByPointer(PETHREAD Thread,
                            NTSTATUS ExitStatus);

VOID PsUnfreezeOtherThread(PETHREAD Thread);
VOID PsFreezeOtherThread(PETHREAD Thread);
VOID PsFreezeProcessThreads(PEPROCESS Process);
VOID PsUnfreezeProcessThreads(PEPROCESS Process);
ULONG PsEnumThreadsByProcess(PEPROCESS Process);
PEPROCESS STDCALL PsGetNextProcess(PEPROCESS OldProcess);
VOID
PsApplicationProcessorInit(VOID);
VOID
PsPrepareForApplicationProcessorInit(ULONG Id);
VOID STDCALL
PsIdleThreadMain(PVOID Context);

VOID STDCALL
PiSuspendThreadRundownRoutine(PKAPC Apc);
VOID STDCALL
PiSuspendThreadKernelRoutine(PKAPC Apc,
			     PKNORMAL_ROUTINE* NormalRoutine,
			     PVOID* NormalContext,
			     PVOID* SystemArgument1,
			     PVOID* SystemArguemnt2);
VOID STDCALL
PiSuspendThreadNormalRoutine(PVOID NormalContext,
			     PVOID SystemArgument1,
			     PVOID SystemArgument2);
VOID
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

extern LONG PiNrThreadsAwaitingReaping;

NTSTATUS
PsInitWin32Thread (PETHREAD Thread);

VOID
PsTerminateWin32Process (PEPROCESS Process);

VOID
PsTerminateWin32Thread (PETHREAD Thread);

VOID
PsInitialiseW32Call(VOID);

VOID
STDCALL
PspRunCreateThreadNotifyRoutines(PETHREAD, BOOLEAN);

VOID
STDCALL
PspRunCreateProcessNotifyRoutines(PEPROCESS, BOOLEAN);

VOID
STDCALL
PspRunLegoRoutine(IN PKTHREAD Thread);

VOID INIT_FUNCTION PsInitJobManagment(VOID);

VOID
STDCALL
PspInheritQuota(PEPROCESS Process, PEPROCESS ParentProcess);

VOID
STDCALL
PspDestroyQuotaBlock(PEPROCESS Process);

/* CLIENT ID */

NTSTATUS PsCreateCidHandle(PVOID Object, POBJECT_TYPE ObjectType, PHANDLE Handle);
NTSTATUS PsDeleteCidHandle(HANDLE CidHandle, POBJECT_TYPE ObjectType);
PHANDLE_TABLE_ENTRY PsLookupCidHandle(HANDLE CidHandle, POBJECT_TYPE ObjectType, PVOID *Object);
VOID PsUnlockCidHandle(PHANDLE_TABLE_ENTRY CidEntry);
NTSTATUS PsLockProcess(PEPROCESS Process, BOOLEAN Timeout);
VOID PsUnlockProcess(PEPROCESS Process);

#define ETHREAD_TO_KTHREAD(pEThread) (&(pEThread)->Tcb)
#define KTHREAD_TO_ETHREAD(pKThread) (CONTAINING_RECORD((pKThread), ETHREAD, Tcb))
#define EPROCESS_TO_KPROCESS(pEProcess) (&(pEProcess)->Pcb)
#define KPROCESS_TO_EPROCESS(pKProcess) (CONTAINING_RECORD((pKProcess), EPROCESS, Pcb))

#define MAX_PROCESS_NOTIFY_ROUTINE_COUNT    8
#define MAX_LOAD_IMAGE_NOTIFY_ROUTINE_COUNT  8

#endif /* __INCLUDE_INTERNAL_PS_H */
