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
//   DPRINT("KeAcquireDispatcherDatabaseLock(Wait %x)\n",Wait);
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
//   DPRINT("KeReleaseDispatcherDatabaseLock(Wait %x)\n",Wait);  
   assert(Wait==WaitSet);
   if (!Wait)
     {
	Owner = NULL;
	KeReleaseSpinLock(&DispatcherDatabaseLock,oldlvl);
     }
}

static BOOLEAN KeDispatcherObjectWakeAll(DISPATCHER_HEADER* hdr)
{
   PKWAIT_BLOCK current;
   PLIST_ENTRY current_entry;
   
   if (IsListEmpty(&hdr->WaitListHead))
     {
	return(FALSE);
     }
   
   while (!IsListEmpty(&(hdr->WaitListHead)))
     {
	current_entry = RemoveHeadList(&hdr->WaitListHead);
	current = CONTAINING_RECORD(current_entry,KWAIT_BLOCK,
					    WaitListEntry);
	DPRINT("Waking %x\n",current->Thread);
	PsResumeThread(CONTAINING_RECORD(current->Thread,ETHREAD,Tcb));
     };
   return(TRUE);
}

static BOOLEAN KeDispatcherObjectWakeOne(DISPATCHER_HEADER* hdr)
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
   if (hdr->Type == SemaphoreType)
      hdr->SignalState--;
   PsResumeThread(CONTAINING_RECORD(current->Thread,ETHREAD,Tcb));
   return(TRUE);
}

BOOLEAN KeDispatcherObjectWake(DISPATCHER_HEADER* hdr)
/*
 * FUNCTION: Wake threads waiting on a dispatcher object
 * NOTE: The exact semantics of waking are dependant on the type of object
 */
{
   BOOL Ret;
   
   DPRINT("Entering KeDispatcherObjectWake(hdr %x)\n",hdr);
//   DPRINT("hdr->WaitListHead %x hdr->WaitListHead.Flink %x\n",
//	  &hdr->WaitListHead,hdr->WaitListHead.Flink);
   DPRINT("hdr->Type %x\n",hdr->Type);
   switch (hdr->Type)
     {
      case NotificationEvent:
	return(KeDispatcherObjectWakeAll(hdr));
	
      case SynchronizationEvent:
	Ret = KeDispatcherObjectWakeOne(hdr);
	if (Ret)
	  {
	     hdr->SignalState = FALSE;
	  }
	return(Ret);
	
      case SemaphoreType:
	if(hdr->SignalState>0)
	{
          do
          {
     	    Ret = KeDispatcherObjectWakeOne(hdr);
          } while(hdr->SignalState > 0 &&  Ret) ;
	  return(Ret);
	}
	else return FALSE;
	
      case ID_PROCESS_OBJECT:
	return(KeDispatcherObjectWakeAll(hdr));
     }
   DbgPrint("Dispatcher object %x has unknown type\n",hdr);
   KeBugCheck(0);
   return(FALSE);
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
   
   DPRINT("Entering KeWaitForSingleObject(Object %x) "
	  "PsGetCurrentThread() %x\n",Object,PsGetCurrentThread());

   KeAcquireDispatcherDatabaseLock(FALSE);
   
   DPRINT("hdr->SignalState %d\n", hdr->SignalState);
   
   if (hdr->SignalState > 0)
   {
      switch (hdr->Type)
	{
	 case SynchronizationEvent:
	   hdr->SignalState = FALSE;
	   break;
	   
	 case SemaphoreType:
	   break;
	   
	 case ID_PROCESS_OBJECT:
	   break;
	   
	 case NotificationEvent:
	   break;
	   
	 default:
	   DbgPrint("(%s:%d) Dispatcher object %x has unknown type\n",
		    __FILE__,__LINE__,hdr);
	   KeBugCheck(0);
	   
	}
      KeReleaseDispatcherDatabaseLock(FALSE);
      return(STATUS_SUCCESS);
   }

   if (Timeout != NULL)
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
   DPRINT("Waiting at %s:%d with irql %d\n", __FILE__, __LINE__, 
	  KeGetCurrentIrql());
   PsSuspendThread(PsGetCurrentThread());
   
   if (Timeout != NULL)
     {
	KeCancelTimer(&KeGetCurrentThread()->Timer);
     }
   DPRINT("Returning from KeWaitForSingleObject()\n");
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
   PVOID ObjectPtr;
   NTSTATUS Status;
   
   DPRINT("ZwWaitForSingleObject(Object %x, Alertable %d, Time %x)\n",
	  Object,Alertable,Time);
   
   Status = ObReferenceObjectByHandle(Object,
				      SYNCHRONIZE,
				      NULL,
				      UserMode,
				      &ObjectPtr,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   Status = KeWaitForSingleObject(ObjectPtr,
				  UserMode,
				  UserMode,
				  Alertable,
				  Time);
   
   ObDereferenceObject(ObjectPtr);
   
   return(Status);
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
