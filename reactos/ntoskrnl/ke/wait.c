/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS project
 * FILE:                 ntoskrnl/ke/wait.c
 * PURPOSE:              Manages non-busy waiting
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *           21/07/98: Created
 */

/* NOTES ********************************************************************
 * 
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/ke.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static KSPIN_LOCK DispatcherDatabaseLock = {0,};
static BOOLEAN WaitSet = FALSE;
static KIRQL oldlvl = PASSIVE_LEVEL;
static PKTHREAD Owner = NULL; 

/* FUNCTIONS *****************************************************************/

VOID KeInitializeDispatcherHeader(DISPATCHER_HEADER* Header,
				  ULONG Type,
				  ULONG Size,
				  ULONG SignalState)
{
   Header->Type = Type;
   Header->Absolute = 0;
   Header->Inserted = 0;
   Header->Size = Size;
   Header->SignalState = SignalState;
   InitializeListHead(&(Header->WaitListHead));
}

VOID KeAcquireDispatcherDatabaseLock(BOOLEAN Wait)
/*
 * PURPOSE: Acquires the dispatcher database lock for the caller
 */
{
   DPRINT("KeAcquireDispatcherDatabaseLock(Wait %x)\n",Wait);
   if (WaitSet && Owner == KeGetCurrentThread())
     {
	return;
     }
   KeAcquireSpinLock(&DispatcherDatabaseLock,&oldlvl);
   WaitSet = Wait;
   Owner = KeGetCurrentThread();
}

VOID KeReleaseDispatcherDatabaseLock(BOOLEAN Wait)
{
   DPRINT("KeReleaseDispatcherDatabaseLock(Wait %x)\n",Wait);  
   assert(Wait==WaitSet);
   if (!Wait)
     {
	Owner = NULL;
	KeReleaseSpinLock(&DispatcherDatabaseLock,oldlvl);
     }
}

VOID KeDispatcherObjectWakeAll(DISPATCHER_HEADER* hdr)
{
   PKWAIT_BLOCK current;
   PLIST_ENTRY current_entry;
   
   while (!IsListEmpty(&(hdr->WaitListHead)))
     {
	current_entry = RemoveHeadList(&hdr->WaitListHead);
	current = CONTAINING_RECORD(current_entry,KWAIT_BLOCK,
					    WaitListEntry);
	DPRINT("Waking %x\n",current->Thread);
	PsResumeThread(CONTAINING_RECORD(current->Thread,ETHREAD,Tcb));
     };
}

BOOLEAN KeDispatcherObjectWakeOne(DISPATCHER_HEADER* hdr)
{
   PKWAIT_BLOCK current;
   PLIST_ENTRY current_entry;
   
   DPRINT("KeDispatcherObjectWakeOn(hdr %x)\n",hdr);
   DPRINT("hdr->WaitListHead.Flink %x hdr->WaitListHead.Blink %x\n",
	  hdr->WaitListHead.Flink,hdr->WaitListHead.Blink);
   if (IsListEmpty(&(hdr->WaitListHead)))
     {
	return(FALSE);
     }
   current_entry=RemoveHeadList(&(hdr->WaitListHead));
   current = CONTAINING_RECORD(current_entry,KWAIT_BLOCK,
			       WaitListEntry);
   DPRINT("current_entry %x current %x\n",current_entry,current);
   DPRINT("Waking %x\n",current->Thread);
   PsResumeThread(CONTAINING_RECORD(current->Thread,ETHREAD,Tcb));
   return(TRUE);
}

VOID KeDispatcherObjectWake(DISPATCHER_HEADER* hdr)
{
   
   DPRINT("Entering KeDispatcherObjectWake(hdr %x)\n",hdr);
//   DPRINT("hdr->WaitListHead %x hdr->WaitListHead.Flink %x\n",
//	  &hdr->WaitListHead,hdr->WaitListHead.Flink);
   if (hdr->Type==NotificationEvent)
     {
	KeDispatcherObjectWakeAll(hdr);
     }
   if (hdr->Type==SynchronizationEvent)
     {
        if (KeDispatcherObjectWakeOne(hdr))
        {
           hdr->SignalState=FALSE;
        }
     }
}
 
   
NTSTATUS KeWaitForSingleObject(PVOID Object,
			       KWAIT_REASON WaitReason,
			       KPROCESSOR_MODE WaitMode,
			       BOOLEAN Alertable,
			       PLARGE_INTEGER Timeout)
/*
 * FUNCTION: Puts the current thread into a wait state until the
 * given dispatcher object is set to signalled 
 * ARGUMENTS:
 *         Object = Object to wait on
 *         WaitReason = Reason for the wait (debugging aid)
 *         WaitMode = Can be KernelMode or UserMode, if UserMode then
 *                    user-mode APCs can be delivered and the thread's
 *                    stack can be paged out
 *         Altertable = Specifies if the wait is a alertable
 *         Timeout = Optional timeout value
 * RETURNS: Status
 */
{
   DISPATCHER_HEADER* hdr = (DISPATCHER_HEADER *)Object;
   KWAIT_BLOCK blk;
   
   DPRINT("Entering KeWaitForSingleObject(Object %x)\n",Object);

   KeAcquireDispatcherDatabaseLock(FALSE);

   if (hdr->SignalState)
   {
      if (hdr->Type == SynchronizationEvent)
	{
	   hdr->SignalState=FALSE;
	}
      KeReleaseDispatcherDatabaseLock(FALSE);
      return(STATUS_SUCCESS);
   }

   if (Timeout!=NULL)
     {
	KeAddThreadTimeout(KeGetCurrentThread(),Timeout);
     }
   
   blk.Object=Object;
   blk.Thread=KeGetCurrentThread();
   blk.WaitKey = WaitReason;     // Assumed
   blk.WaitType = WaitMode;      // Assumed
   blk.NextWaitBlock = NULL;
   InsertTailList(&(hdr->WaitListHead),&(blk.WaitListEntry));
//   DPRINT("hdr->WaitListHead.Flink %x hdr->WaitListHead.Blink %x\n",
//          hdr->WaitListHead.Flink,hdr->WaitListHead.Blink);
   KeReleaseDispatcherDatabaseLock(FALSE);
   PsSuspendThread(PsGetCurrentThread());
   return(STATUS_SUCCESS);
}

NTSTATUS KeWaitForMultipleObjects(ULONG Count,
				  PVOID Object[],
				  WAIT_TYPE WaitType,
				  KWAIT_REASON WaitReason,
				  KPROCESSOR_MODE WaitMode,
				  BOOLEAN Alertable,
				  PLARGE_INTEGER Timeout,
				  PKWAIT_BLOCK WaitBlockArray)
{
   UNIMPLEMENTED;
}

VOID KeInitializeDispatcher(VOID)
{
   KeInitializeSpinLock(&DispatcherDatabaseLock);
}

NTSTATUS STDCALL NtWaitForMultipleObjects (IN ULONG Count,
					   IN HANDLE Object[],
					   IN CINT WaitType,
					   IN BOOLEAN Alertable,
					   IN PLARGE_INTEGER Time)
{
   return(ZwWaitForMultipleObjects(Count,
				   Object,
				   WaitType,
				   Alertable,
				   Time));
}

NTSTATUS STDCALL ZwWaitForMultipleObjects (IN ULONG Count,
					   IN HANDLE Object[],
					   IN CINT WaitType,
					   IN BOOLEAN Alertable,
					   IN PLARGE_INTEGER Time)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtWaitForSingleObject (IN HANDLE Object,
					IN BOOLEAN Alertable,
					IN PLARGE_INTEGER Time)
{
   return(ZwWaitForSingleObject(Object,
				Alertable,
				Time));
}

NTSTATUS STDCALL ZwWaitForSingleObject (IN HANDLE Object,
					IN BOOLEAN Alertable,
					IN PLARGE_INTEGER Time)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtSignalAndWaitForSingleObject(
				 IN HANDLE EventHandle,
	                         IN BOOLEAN Alertable,
	                         IN PLARGE_INTEGER Time,
	                         PULONG NumberOfWaitingThreads OPTIONAL)
{
   return(ZwSignalAndWaitForSingleObject(EventHandle,
					 Alertable,
					 Time,
					 NumberOfWaitingThreads));
}

NTSTATUS STDCALL ZwSignalAndWaitForSingleObject(
				 IN HANDLE EventHandle,
				 IN BOOLEAN Alertable,
				 IN PLARGE_INTEGER Time,
				 PULONG NumberOfWaitingThreads OPTIONAL)
{
   UNIMPLEMENTED;
}
