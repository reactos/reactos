#ifndef __INCLUDE_INTERNAL_PSMGR_H
#define __INCLUDE_INTERNAL_PSMGR_H

#include <internal/hal/hal.h>

void PsInitThreadManagment(void);

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
   THREAD_STATE_SLEEPING,
     
   /*
    * Waiting to be freed
    */
   THREAD_STATE_TERMINATED,
};

NTSTATUS PsTerminateSystemThread(NTSTATUS ExitStatus);

/*
 * Functions the HAL must provide
 */

void HalInitFirstTask(PTHREAD_OBJECT thread);
void HalInitTask(PTHREAD_OBJECT thread, PKSTART_ROUTINE fn, 
		 PVOID StartContext);
void HalTaskSwitch(PTHREAD_OBJECT thread);

#endif
