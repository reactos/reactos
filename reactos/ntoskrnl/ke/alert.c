/* $Id:$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/alert.c
 * PURPOSE:         Alerts
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/


/* FUNCTIONS *****************************************************************/


BOOLEAN
STDCALL
KeTestAlertThread(IN KPROCESSOR_MODE AlertMode)
/*
 * FUNCTION: Tests whether there are any pending APCs for the current thread
 * and if so the APCs will be delivered on exit from kernel mode
 */
{
   KIRQL OldIrql;
   PKTHREAD Thread = KeGetCurrentThread();
   BOOLEAN OldState;
   
   ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
   
   OldIrql = KeAcquireDispatcherDatabaseLock();
   KiAcquireSpinLock(&Thread->ApcQueueLock);
   
   OldState = Thread->Alerted[AlertMode];
   
   /* If the Thread is Alerted, Clear it */
   if (OldState) {
      Thread->Alerted[AlertMode] = FALSE;
   } else if ((AlertMode == UserMode) && (!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]))) {
      /* If the mode is User and the Queue isn't empty, set Pending */
      Thread->ApcState.UserApcPending = TRUE;
   }
   
   KiReleaseSpinLock(&Thread->ApcQueueLock);
   KeReleaseDispatcherDatabaseLock(OldIrql);
   return OldState;
}


VOID
KeAlertThread(PKTHREAD Thread, KPROCESSOR_MODE AlertMode)
{
   KIRQL oldIrql;

   
   oldIrql = KeAcquireDispatcherDatabaseLock();
      

   /* Return if thread is already alerted. */
   if (Thread->Alerted[AlertMode] == FALSE)
   {
      if (Thread->State == THREAD_STATE_BLOCKED &&
          (AlertMode == KernelMode || Thread->WaitMode == AlertMode) &&
          Thread->Alertable)
      {
         KiAbortWaitThread(Thread, STATUS_ALERTED);
      }
      else
      {
         Thread->Alerted[AlertMode] = TRUE;
      }
   }
   
   KeReleaseDispatcherDatabaseLock(oldIrql);
 
}


/* 
 *
 * NOT EXPORTED
 */
NTSTATUS STDCALL
NtAlertResumeThread(IN  HANDLE ThreadHandle,
          OUT PULONG SuspendCount)
{
   UNIMPLEMENTED;
   return(STATUS_NOT_IMPLEMENTED);

}

/* 
 * @implemented
 *
 * EXPORTED
 */
NTSTATUS STDCALL
NtAlertThread (IN HANDLE ThreadHandle)
{
   KPROCESSOR_MODE PreviousMode;
   PETHREAD Thread;
   NTSTATUS Status;
   
   PreviousMode = ExGetPreviousMode();

   Status = ObReferenceObjectByHandle(ThreadHandle,
                  THREAD_SUSPEND_RESUME,
                  PsThreadType,
                  PreviousMode,
                  (PVOID*)&Thread,
                  NULL);
   if (!NT_SUCCESS(Status))
     {
   return(Status);
     }

   /* do an alert depending on the processor mode. If some kmode code wants to
      enforce a umode alert it should call KeAlertThread() directly. If kmode
      code wants to do a kmode alert it's sufficient to call it with Zw or just
      use KeAlertThread() directly */

   KeAlertThread(&Thread->Tcb, PreviousMode);
   
   ObDereferenceObject(Thread);
   return(STATUS_SUCCESS);
}


/* 
 * NOT EXPORTED
 */
NTSTATUS
STDCALL
NtTestAlert(VOID)
{
   KPROCESSOR_MODE PreviousMode;
   
   PreviousMode = ExGetPreviousMode();
   
   /* Check and Alert Thread if needed */
   
   return KeTestAlertThread(PreviousMode) ? STATUS_ALERTED : STATUS_SUCCESS;
}

