/* $Id$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/idle.c
 * PURPOSE:         Using idle time
 * 
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

/** System idle thread procedure
 *
 */
VOID STDCALL
PsIdleThreadMain(PVOID Context)
{
   KIRQL oldlvl;

   PKPRCB Prcb = KeGetCurrentPrcb();
   
   for(;;)
     {
       if (Prcb->DpcData[0].DpcQueueDepth > 0)
	 {
	   KeRaiseIrql(DISPATCH_LEVEL,&oldlvl);
	   KiDispatchInterrupt();
	   KeLowerIrql(oldlvl);
	 }

       NtYieldExecution();

       Ke386HaltProcessor();
     }
}


/** Initialization of system idle thread
 *
 */ 
VOID INIT_FUNCTION
PsInitIdleThread(VOID)
{
   NTSTATUS Status;
   PETHREAD IdleThread;
   KIRQL oldIrql;
   
   Status = PsInitializeThread(NULL,
			       &IdleThread,
			       NULL,
			       FALSE);
   if (!NT_SUCCESS(Status))
     {
        DPRINT1("Couldn't create idle system thread! Status: 0x%x\n", Status);
        KEBUGCHECK(0);
        return;
     }
   
   IdleThread->StartAddress = PsIdleThreadMain;
   Status = KiArchInitThread(&IdleThread->Tcb, PsIdleThreadMain, NULL);
   if (!NT_SUCCESS(Status))
     {
        DPRINT1("Couldn't initialize system idle thread! Status: 0x%x\n", Status);
        ObDereferenceObject(IdleThread);
        KEBUGCHECK(0);
        return;
     }

   oldIrql = KeAcquireDispatcherDatabaseLock ();
   PsUnblockThread(IdleThread, NULL, 0);
   KeReleaseDispatcherDatabaseLock(oldIrql);

   KeGetCurrentPrcb()->IdleThread = &IdleThread->Tcb;
   KeSetPriorityThread(&IdleThread->Tcb, LOW_PRIORITY);
   KeSetAffinityThread(&IdleThread->Tcb, 1 << 0);

}
