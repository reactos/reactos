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
   
   /* NOTE: Albert Almeida claims Alerted[1] is never used. Two kind of 
    * alerts _do_ seem useless. -Gunnar
    */
   OldState = Thread->Alerted[0];
   
   /* If the Thread is Alerted, Clear it */
   if (OldState) {
      Thread->Alerted[0] = FALSE;  
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
      

   /* Return if thread is already alerted.
    * NOTE: Albert Almeida claims Alerted[1] is never used. Two kind of 
    * alerts _do_ seem useless. -Gunnar
    */
   if (Thread->Alerted[0] == FALSE)
   {
      Thread->Alerted[0] = TRUE;

      if (Thread->State == THREAD_STATE_BLOCKED &&
          Thread->WaitMode == AlertMode &&
          Thread->Alertable)
      {
         KiAbortWaitThread(Thread, STATUS_ALERTED);
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

   /* FIXME: should we always use UserMode here, even if the ntoskrnl exported
    * ZwAlertThread was called?
    * -Gunnar
    */ 
   KeAlertThread((PKTHREAD)Thread, PreviousMode);
   
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
   /* Check and Alert Thread if needed */
   if (KeTestAlertThread(KeGetPreviousMode())) {
      return STATUS_ALERTED;
   } else {
      return STATUS_SUCCESS;
   }
}

