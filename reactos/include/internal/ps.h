#ifndef __INCLUDE_INTERNAL_PSMGR_H
#define __INCLUDE_INTERNAL_PSMGR_H

#include <internal/hal.h>

extern PEPROCESS SystemProcess;
extern HANDLE SystemProcessHandle;

/* ntoskrnl/ps/thread.c */
extern POBJECT_TYPE	PsThreadType;
extern POBJECT_TYPE	PsProcessType;
extern PETHREAD		CurrentThread;

VOID PiInitProcessManager(VOID);
VOID PiShutdownProcessManager(VOID);
VOID PsInitThreadManagment(VOID);
VOID PsInitProcessManagment(VOID);
VOID PsInitIdleThread(VOID);
VOID PsDispatchThread(ULONG NewThreadStatus);
VOID PiTerminateProcessThreads(PEPROCESS Process, NTSTATUS ExitStatus);
VOID PsTerminateOtherThread(PETHREAD Thread, NTSTATUS ExitStatus);
VOID PsReleaseThread(PETHREAD Thread);
VOID PsBeginThread(PKSTART_ROUTINE StartRoutine, PVOID StartContext);
VOID PsBeginThreadWithContextInternal(VOID);
VOID PiKillMostProcesses(VOID);
NTSTATUS STDCALL PiTerminateProcess(PEPROCESS Process, NTSTATUS ExitStatus);

#define THREAD_STATE_INVALID    (0)
#define THREAD_STATE_RUNNABLE   (1)
#define THREAD_STATE_RUNNING    (2)
#define THREAD_STATE_SUSPENDED  (3)
#define THREAD_STATE_TERMINATED (4)
#define THREAD_STATE_MAX        (5)
     
/*
 * Functions the HAL must provide
 */

void HalInitFirstTask(PETHREAD thread);
NTSTATUS HalInitTask(PETHREAD thread, PKSTART_ROUTINE fn, PVOID StartContext);
void HalTaskSwitch(PKTHREAD thread);
NTSTATUS HalInitTaskWithContext(PETHREAD Thread, PCONTEXT Context);
NTSTATUS HalReleaseTask(PETHREAD Thread);
VOID PiDeleteProcess(PVOID ObjectBody);

#endif
