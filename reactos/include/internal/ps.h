#ifndef __INCLUDE_INTERNAL_PSMGR_H
#define __INCLUDE_INTERNAL_PSMGR_H

#include <internal/hal.h>

extern PEPROCESS SystemProcess;
extern HANDLE SystemProcessHandle;

extern POBJECT_TYPE PsThreadType;
extern POBJECT_TYPE PsProcessType;

void PsInitThreadManagment(void);
VOID PsInitProcessManagment(VOID);
VOID PsInitIdleThread(VOID);
VOID PsDispatchThread(VOID);
VOID PiTerminateProcessThreads(PEPROCESS Process, NTSTATUS ExitStatus);
VOID PsTerminateOtherThread(PETHREAD Thread, NTSTATUS ExitStatus);
VOID PsReleaseThread(PETHREAD Thread);

/*
 * PURPOSE: Thread states
 */
enum
{
   /*
    * PURPOSE: Don't touch 
    */
   THREAD_STATE_INVALID,
     
  /*
    * PURPOSE: Waiting to be dispatched
    */
   THREAD_STATE_RUNNABLE,
     
   /*
    * PURPOSE: Currently running
    */
   THREAD_STATE_RUNNING,
     
   /*
    * PURPOSE: Doesn't want to run
    */
   THREAD_STATE_SUSPENDED,
     
   /*
    * Waiting to be freed
    */
   THREAD_STATE_TERMINATED,
};

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
