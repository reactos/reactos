#ifndef __INCLUDE_INTERNAL_PS_H
#define __INCLUDE_INTERNAL_PS_H

#include <internal/hal.h>

extern HANDLE SystemProcessHandle;

/* ntoskrnl/ps/thread.c */
extern PETHREAD		CurrentThread;

VOID PiInitProcessManager(VOID);
VOID PiShutdownProcessManager(VOID);
VOID PsInitThreadManagment(VOID);
VOID PsInitProcessManagment(VOID);
VOID PsInitIdleThread(VOID);
VOID PsDispatchThread(ULONG NewThreadStatus);
VOID PsDispatchThreadNoLock(ULONG NewThreadStatus);
VOID PiTerminateProcessThreads(PEPROCESS Process, NTSTATUS ExitStatus);
VOID PsTerminateOtherThread(PETHREAD Thread, NTSTATUS ExitStatus);
VOID PsReleaseThread(PETHREAD Thread);
VOID PsBeginThread(PKSTART_ROUTINE StartRoutine, PVOID StartContext);
VOID PsBeginThreadWithContextInternal(VOID);
VOID PiKillMostProcesses(VOID);
NTSTATUS STDCALL PiTerminateProcess(PEPROCESS Process, NTSTATUS ExitStatus);
ULONG PsUnfreezeThread(PETHREAD Thread, PNTSTATUS WaitStatus);
ULONG PsFreezeThread(PETHREAD Thread, PNTSTATUS WaitStatus,
		     UCHAR Alertable, ULONG WaitMode);
VOID PiInitApcManagement(VOID);
VOID PiDeleteThread(PVOID ObjectBody);
VOID PiCloseThread(PVOID ObjectBody, ULONG HandleCount);
VOID PsReapThreads(VOID);
NTSTATUS PsInitializeThread(HANDLE ProcessHandle,
			    PETHREAD* ThreadPtr,
			    PHANDLE ThreadHandle,
			    ACCESS_MASK DesiredAccess,
			    POBJECT_ATTRIBUTES ObjectAttributes);

#define THREAD_STATE_INVALID      (0)
#define THREAD_STATE_RUNNABLE     (1)
#define THREAD_STATE_RUNNING      (2)
#define THREAD_STATE_SUSPENDED    (3)
#define THREAD_STATE_FROZEN       (4)
#define THREAD_STATE_TERMINATED_1 (5)
#define THREAD_STATE_TERMINATED_2 (6)
#define THREAD_STATE_MAX          (7)


// Internal thread priorities, added by Phillip Susi
// TODO: rebalence these to make use of all priorities... the ones above 16 can not all be used right now

#define PROCESS_PRIO_IDLE			3
#define PROCESS_PRIO_NORMAL			8
#define PROCESS_PRIO_HIGH			13
#define PROCESS_PRIO_RT				18

/*
 * Functions the HAL must provide
 */

void HalInitFirstTask(PETHREAD thread);
NTSTATUS HalInitTask(PETHREAD thread, PKSTART_ROUTINE fn, PVOID StartContext);
void HalTaskSwitch(PKTHREAD thread);
NTSTATUS HalInitTaskWithContext(PETHREAD Thread, PCONTEXT Context);
NTSTATUS HalReleaseTask(PETHREAD Thread);
VOID PiDeleteProcess(PVOID ObjectBody);
VOID PsReapThreads(VOID);
VOID PsUnfreezeOtherThread(PETHREAD Thread);
VOID PsFreezeOtherThread(PETHREAD Thread);
VOID PsFreezeProcessThreads(PEPROCESS Process);
VOID PsUnfreezeProcessThreads(PEPROCESS Process);
PEPROCESS PsGetNextProcess(PEPROCESS OldProcess);

#endif /* __INCLUDE_INTERNAL_PS_H */
