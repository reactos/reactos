#ifndef __INCLUDE_INTERNAL_PSMGR_H
#define __INCLUDE_INTERNAL_PSMGR_H

#include <internal/hal.h>

extern HANDLE SystemProcessHandle;

void PsInitThreadManagment(void);
VOID PsInitIdleThread(VOID);

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

void HalInitFirstTask(PKTHREAD thread);
BOOLEAN HalInitTask(PKTHREAD thread, PKSTART_ROUTINE fn, 
		 PVOID StartContext);
void HalTaskSwitch(PKTHREAD thread);

#endif
