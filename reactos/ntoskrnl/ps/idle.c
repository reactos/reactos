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

extern PEPROCESS PsIdleProcess;

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

/* 
 * HACK-O-RAMA
 * Antique vestigial code left alive for the sole purpose of First/Idle Thread
 * creation until I can merge my fix for properly creating them.
 */
NTSTATUS
PsInitializeIdleOrFirstThread(PEPROCESS Process,
                              PETHREAD* ThreadPtr,
                              PKSTART_ROUTINE StartRoutine,
                              KPROCESSOR_MODE AccessMode,
                              BOOLEAN First)
{
    PETHREAD Thread;
    PVOID KernelStack;
    extern unsigned int init_stack;

    PAGED_CODE();

    Thread = ExAllocatePool(NonPagedPool, sizeof(ETHREAD));

    RtlZeroMemory(Thread, sizeof(ETHREAD));
    Thread->ThreadsProcess = Process;

    DPRINT("Thread = %x\n",Thread);

    if (First) 
    {
        KernelStack = (PVOID)init_stack;
    }
    else
    {
        KernelStack = MmCreateKernelStack(FALSE);
    }
    
    KeInitializeThread(&Process->Pcb, 
                       &Thread->Tcb, 
                       PspSystemThreadStartup,
                       StartRoutine,
                       NULL,
                       NULL,
                       NULL,
                       KernelStack);
    Thread->Tcb.ApcQueueable = TRUE;
    
    InitializeListHead(&Thread->IrpList);

    DPRINT("Thread->Cid.UniqueThread %d\n",Thread->Cid.UniqueThread);

    *ThreadPtr = Thread;

    return STATUS_SUCCESS;
}

/* 
 * HACK-O-RAMA
 * Antique vestigial code left alive for the sole purpose of First/Idle Thread
 * creation until I can merge my fix for properly creating them.
 */
VOID 
INIT_FUNCTION
PsInitIdleThread(VOID)
{
    PETHREAD IdleThread;
    KIRQL oldIrql;

    PsInitializeIdleOrFirstThread(PsIdleProcess,
                                  &IdleThread,
                                  PsIdleThreadMain,
                                  KernelMode,
                                  FALSE);

    oldIrql = KeAcquireDispatcherDatabaseLock ();
    KiUnblockThread(&IdleThread->Tcb, NULL, 0);
    KeReleaseDispatcherDatabaseLock(oldIrql);

    KeGetCurrentPrcb()->IdleThread = &IdleThread->Tcb;
    KeSetPriorityThread(&IdleThread->Tcb, LOW_PRIORITY);
    KeSetAffinityThread(&IdleThread->Tcb, 1 << 0);
}
