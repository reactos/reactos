/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS project
 * FILE:                 ntoskrnl/ke/wait.c
 * PURPOSE:              Manages non-busy waiting
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *           21/07/98: Created
 *	     12/1/99:  Phillip Susi: Fixed wake code in KeDispatcherObjectWake
 *		   so that things like KeWaitForXXX() return the correct value
 */

/* NOTES ********************************************************************
 *
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <internal/ob.h>
#include <internal/id.h>
#include <ntos/ntdef.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static KSPIN_LOCK DispatcherDatabaseLock;
static BOOLEAN WaitSet = FALSE;
static KIRQL oldlvl = PASSIVE_LEVEL;
static PKTHREAD Owner = NULL;

#define KeDispatcherObjectWakeOne(hdr) KeDispatcherObjectWakeOneOrAll(hdr, FALSE)
#define KeDispatcherObjectWakeAll(hdr) KeDispatcherObjectWakeOneOrAll(hdr, TRUE)

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
   KeAcquireSpinLock(&DispatcherDatabaseLock, &oldlvl);
   WaitSet = Wait;
   Owner = KeGetCurrentThread();
}

VOID KeReleaseDispatcherDatabaseLockAtDpcLevel(BOOLEAN Wait)
{
  DPRINT("KeReleaseDispatcherDatabaseLockAtDpcLevel(Wait %x)\n", Wait);
  assert(Wait == WaitSet);
  if (!Wait)
    {
      Owner = NULL;
      KeReleaseSpinLockFromDpcLevel(&DispatcherDatabaseLock);
    }
}

VOID KeReleaseDispatcherDatabaseLock(BOOLEAN Wait)
{
   DPRINT("KeReleaseDispatcherDatabaseLock(Wait %x)\n",Wait);
   assert(Wait==WaitSet);
   if (!Wait)
     {
	Owner = NULL;
	KeReleaseSpinLock(&DispatcherDatabaseLock, oldlvl);
     }
}

static BOOLEAN
KiSideEffectsBeforeWake(DISPATCHER_HEADER * hdr,
                        PKTHREAD Thread)
/*
 * FUNCTION: Perform side effects on object before a wait for a thread is
 *           satisfied
 */
{
   BOOLEAN Abandoned = FALSE;

   switch (hdr->Type)
   {
      case InternalSynchronizationEvent:
         hdr->SignalState = 0;
         break;
      
      case InternalQueueType:
      case InternalSemaphoreType:
         hdr->SignalState--;
         break;

      case InternalProcessType:
         break;

      case InternalThreadType:
         break;

      case InternalNotificationEvent:
         break;

      case InternalSynchronizationTimer:
         hdr->SignalState = FALSE;
         break;

      case InternalNotificationTimer:
         break;

      case InternalMutexType:
      {
         PKMUTEX Mutex;

         Mutex = CONTAINING_RECORD(hdr, KMUTEX, Header);
         hdr->SignalState--;
         assert(hdr->SignalState <= 1);
         if (hdr->SignalState == 0)
         {
            if (Thread == NULL)
            {
               DPRINT("Thread == NULL!\n");
               KeBugCheck(0);
            }
            Abandoned = Mutex->Abandoned;
            if (Thread != NULL)
               InsertTailList(&Thread->MutantListHead, &Mutex->MutantListEntry);
            Mutex->OwnerThread = Thread;
            Mutex->Abandoned = FALSE;
         }
      }
         break;

      default:
         DbgPrint("(%s:%d) Dispatcher object %x has unknown type\n", __FILE__, __LINE__, hdr);
         KeBugCheck(0);
   }

   return Abandoned;
}

static BOOLEAN
KiIsObjectSignalled(DISPATCHER_HEADER * hdr,
                    PKTHREAD Thread)
{
   if (hdr->Type == InternalMutexType)
   {
      PKMUTEX Mutex;

      Mutex = CONTAINING_RECORD(hdr, KMUTEX, Header);

      assert(hdr->SignalState <= 1);

      if ((hdr->SignalState < 1 && Mutex->OwnerThread == Thread) || hdr->SignalState == 1)
      {
         return (TRUE);
      }
      else
      {
         return (FALSE);
      }
   }

   if (hdr->SignalState <= 0)
   {
      return (FALSE);
   }
   else
   {
      return (TRUE);
   }
}

VOID KeRemoveAllWaitsThread(PETHREAD Thread, NTSTATUS WaitStatus)
{
   PKWAIT_BLOCK WaitBlock;
   BOOLEAN WasWaiting = FALSE;

   KeAcquireDispatcherDatabaseLock(FALSE);

   WaitBlock = (PKWAIT_BLOCK)Thread->Tcb.WaitBlockList;
   if (WaitBlock != NULL)
     {
	WasWaiting = TRUE;
     }
   while (WaitBlock != NULL)
     {
	RemoveEntryList(&WaitBlock->WaitListEntry);
	WaitBlock = WaitBlock->NextWaitBlock;
     }
   Thread->Tcb.WaitBlockList = NULL;

   if (WasWaiting)
     {
	PsUnblockThread(Thread, &WaitStatus);
     }

   KeReleaseDispatcherDatabaseLock(FALSE);
}

static BOOLEAN
KeDispatcherObjectWakeOneOrAll(DISPATCHER_HEADER * hdr,
                               BOOLEAN WakeAll)
{
   PKWAIT_BLOCK Waiter;
   PKWAIT_BLOCK WaiterHead;
   PLIST_ENTRY EnumEntry;
   NTSTATUS Status;
   BOOLEAN Abandoned;
   BOOLEAN AllSignaled;
   BOOLEAN WakedAny = FALSE;

   DPRINT("KeDispatcherObjectWakeOnOrAll(hdr %x)\n", hdr);
   DPRINT ("hdr->WaitListHead.Flink %x hdr->WaitListHead.Blink %x\n",
           hdr->WaitListHead.Flink, hdr->WaitListHead.Blink);

   if (IsListEmpty(&hdr->WaitListHead))
   {
      return (FALSE);
   }

   //enum waiters for this dispatcher object
   EnumEntry = hdr->WaitListHead.Flink;
   while (EnumEntry != &hdr->WaitListHead && (WakeAll || !WakedAny))
   {
      WaiterHead = CONTAINING_RECORD(EnumEntry, KWAIT_BLOCK, WaitListEntry);
      DPRINT("current_entry %x current %x\n", EnumEntry, WaiterHead);
      EnumEntry = EnumEntry->Flink;
      assert(WaiterHead->Thread->WaitBlockList != NULL);

      Abandoned = FALSE;

      if (WaiterHead->WaitType == WaitAny)
      {
         DPRINT("WaitAny: Remove all wait blocks.\n");
         for (Waiter = WaiterHead->Thread->WaitBlockList; Waiter; Waiter = Waiter->NextWaitBlock)
         {
            RemoveEntryList(&Waiter->WaitListEntry);
         }

         WaiterHead->Thread->WaitBlockList = NULL;

         /*
          * If a WakeAll KiSideEffectsBeforeWake(hdr,.. will be called several times,
          * but thats ok since WakeAll objects has no sideeffects.
          */
         Abandoned = KiSideEffectsBeforeWake(hdr, WaiterHead->Thread) ? TRUE : Abandoned;
      }
      else
      {
         DPRINT("WaitAll: All WaitAll objects must be signaled.\n");

         AllSignaled = TRUE;

         //all WaitAll obj. for thread need to be signaled to satisfy a wake
         for (Waiter = WaiterHead->Thread->WaitBlockList; Waiter; Waiter = Waiter->NextWaitBlock)
         {
            //no need to check hdr since it has to be signaled
            if (Waiter->WaitType == WaitAll && Waiter->Object != hdr)
            {
               if (!KiIsObjectSignalled(Waiter->Object, Waiter->Thread))
               {
                  AllSignaled = FALSE;
                  break;
               }
            }
         }

         if (AllSignaled)
         {
            for (Waiter = WaiterHead->Thread->WaitBlockList; Waiter; Waiter = Waiter->NextWaitBlock)
            {
               RemoveEntryList(&Waiter->WaitListEntry);

               if (Waiter->WaitType == WaitAll)
               {
                  Abandoned = KiSideEffectsBeforeWake(Waiter->Object, Waiter->Thread)
                     ? TRUE : Abandoned;
               }

               //no WaitAny objects can possibly be signaled since we are here
               assert(!(Waiter->WaitType == WaitAny
					  && KiIsObjectSignalled(Waiter->Object, Waiter->Thread)));
            }

            WaiterHead->Thread->WaitBlockList = NULL;
         }
      }

      if (WaiterHead->Thread->WaitBlockList == NULL)
      {
         Status = WaiterHead->WaitKey;
         if (Abandoned)
         {
            DPRINT("Abandoned mutex among objects");
            Status += STATUS_ABANDONED_WAIT_0;
         }

         WakedAny = TRUE;
         DPRINT("Waking %x status = %x\n", WaiterHead->Thread, Status);
         PsUnblockThread(CONTAINING_RECORD(WaiterHead->Thread, ETHREAD, Tcb), &Status);
      }
   }

   return WakedAny;
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
      case InternalNotificationEvent:
	return(KeDispatcherObjectWakeAll(hdr));

      case InternalNotificationTimer:
	return(KeDispatcherObjectWakeAll(hdr));

      case InternalSynchronizationEvent:
	return(KeDispatcherObjectWakeOne(hdr));

      case InternalSynchronizationTimer:
	return(KeDispatcherObjectWakeOne(hdr));

      case InternalQueueType:
      case InternalSemaphoreType:
	DPRINT("hdr->SignalState %d\n", hdr->SignalState);
	if(hdr->SignalState>0)
	  {
	    do
	      {
		DPRINT("Waking one semaphore waiter\n");
		Ret = KeDispatcherObjectWakeOne(hdr);
	      } while(hdr->SignalState > 0 &&  Ret) ;
	    return(Ret);
	  }
	else return FALSE;

     case InternalProcessType:
	return(KeDispatcherObjectWakeAll(hdr));

     case InternalThreadType:
       return(KeDispatcherObjectWakeAll(hdr));

     case InternalMutexType:
       return(KeDispatcherObjectWakeOne(hdr));
     }
   DbgPrint("Dispatcher object %x has unknown type %d\n", hdr, hdr->Type);
   KeBugCheck(0);
   return(FALSE);
}


NTSTATUS STDCALL
KeWaitForSingleObject(PVOID Object,
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
   return KeWaitForMultipleObjects(1,
                                   &Object,
                                   WaitAny,
                                   WaitReason,
                                   WaitMode,
                                   Alertable,
                                   Timeout,
                                   NULL);
}


inline
PVOID
KiGetWaitableObjectFromObject(PVOID Object)
{
   //special case when waiting on file objects
   if ( ((PDISPATCHER_HEADER)Object)->Type == InternalFileType)
   {
      return &((PFILE_OBJECT)Object)->Event;
   }

   return Object;
}


NTSTATUS STDCALL
KeWaitForMultipleObjects(ULONG Count,
                         PVOID Object[],
                         WAIT_TYPE WaitType,
                         KWAIT_REASON WaitReason,
                         KPROCESSOR_MODE WaitMode,
                         BOOLEAN Alertable,
                         PLARGE_INTEGER Timeout,
                         PKWAIT_BLOCK WaitBlockArray)
{
   DISPATCHER_HEADER *hdr;
   PKWAIT_BLOCK blk;
   PKTHREAD CurrentThread;
   ULONG CountSignaled;
   ULONG i;
   NTSTATUS Status;
   KIRQL WaitIrql;
   BOOLEAN Abandoned;

   DPRINT("Entering KeWaitForMultipleObjects(Count %lu Object[] %p) "
          "PsGetCurrentThread() %x\n", Count, Object, PsGetCurrentThread());

   CurrentThread = KeGetCurrentThread();
   WaitIrql = KeGetCurrentIrql();

   /*
    * Work out where we are going to put the wait blocks
    */
   if (WaitBlockArray == NULL)
   {
      if (Count > THREAD_WAIT_OBJECTS)
      {
         DPRINT("(%s:%d) Too many objects!\n", __FILE__, __LINE__);
         return (STATUS_UNSUCCESSFUL);
      }
      WaitBlockArray = &CurrentThread->WaitBlock[0];
   }
   else
   {
      if (Count > EX_MAXIMUM_WAIT_OBJECTS)
      {
         DPRINT("(%s:%d) Too many objects!\n", __FILE__, __LINE__);
         return (STATUS_UNSUCCESSFUL);
      }
   }

   /*
    * Set up the timeout if required
    */
   if (Timeout != NULL && Timeout->QuadPart != 0)
   {
      KeInitializeTimer(&CurrentThread->Timer);
      KeSetTimer(&CurrentThread->Timer, *Timeout, NULL);
   }

   do
   {
      KeAcquireDispatcherDatabaseLock(FALSE);

 	  /*
       * If we are going to wait alertably and a user apc is pending
       * then return
       */
      if (Alertable && KiTestAlert())
      {
         KeReleaseDispatcherDatabaseLock(FALSE);
         return (STATUS_USER_APC);
      }

      /*
       * Check if the wait is (already) satisfied
       */
      CountSignaled = 0;
      Abandoned = FALSE;
      for (i = 0; i < Count; i++)
      {
         hdr = (DISPATCHER_HEADER *) KiGetWaitableObjectFromObject(Object[i]);

         if (KiIsObjectSignalled(hdr, CurrentThread))
         {
            CountSignaled++;

            if (WaitType == WaitAny)
            {
               Abandoned = KiSideEffectsBeforeWake(hdr, CurrentThread) ? TRUE : Abandoned;

               if (Timeout != NULL && Timeout->QuadPart != 0)
               {
                  KeCancelTimer(&CurrentThread->Timer);
               }

               KeReleaseDispatcherDatabaseLock(FALSE);

               DPRINT("One object is (already) signaled!\n");
               if (Abandoned == TRUE)
               {
                  return (STATUS_ABANDONED_WAIT_0 + i);
               }

               return (STATUS_WAIT_0 + i);
            }
         }
      }

      Abandoned = FALSE;
      if ((WaitType == WaitAll) && (CountSignaled == Count))
      {
         for (i = 0; i < Count; i++)
         {
            hdr = (DISPATCHER_HEADER *) KiGetWaitableObjectFromObject(Object[i]);
            Abandoned = KiSideEffectsBeforeWake(hdr, CurrentThread) ? TRUE : Abandoned;
         }

         if (Timeout != NULL && Timeout->QuadPart != 0)
         {
            KeCancelTimer(&CurrentThread->Timer);
         }

         KeReleaseDispatcherDatabaseLock(FALSE);
         DPRINT("All objects are (already) signaled!\n");

         if (Abandoned == TRUE)
         {
            return (STATUS_ABANDONED_WAIT_0);
         }

         return (STATUS_WAIT_0);
      }

      //zero timeout is used for testing if the object(s) can be immediately acquired
      if (Timeout != NULL && Timeout->QuadPart == 0)
      {
         KeReleaseDispatcherDatabaseLock(FALSE);
         return STATUS_TIMEOUT;
      }

      /*
       * Check if we have already timed out
       */
      if (Timeout != NULL && KiIsObjectSignalled(&CurrentThread->Timer.Header, CurrentThread))
      {
         KiSideEffectsBeforeWake(&CurrentThread->Timer.Header, CurrentThread);
         KeCancelTimer(&CurrentThread->Timer);
         KeReleaseDispatcherDatabaseLock(FALSE);
         return (STATUS_TIMEOUT);
      }

      /* Append wait block to the KTHREAD wait block list */
      CurrentThread->WaitBlockList = blk = WaitBlockArray;

      /*
       * Set up the wait
       */
      CurrentThread->WaitStatus = STATUS_UNSUCCESSFUL;

      for (i = 0; i < Count; i++)
      {
         hdr = (DISPATCHER_HEADER *) KiGetWaitableObjectFromObject(Object[i]);

         blk->Object = KiGetWaitableObjectFromObject(Object[i]);
         blk->Thread = CurrentThread;
         blk->WaitKey = STATUS_WAIT_0 + i;
         blk->WaitType = WaitType;

         if (i == (Count - 1))
         {
            if (Timeout != NULL)
            {
               blk->NextWaitBlock = &CurrentThread->WaitBlock[3];
            }
            else
            {
               blk->NextWaitBlock = NULL;
            }
         }
         else
         {
            blk->NextWaitBlock = blk + 1;
         }

         /*
          * add wait block to disp. obj. wait list
          * Use FIFO for all waits except for queues which use LIFO
          */
         if (WaitReason == WrQueue)
         {
            InsertHeadList(&hdr->WaitListHead, &blk->WaitListEntry);
         }
         else
         {
            InsertTailList(&hdr->WaitListHead, &blk->WaitListEntry);
         }

         blk = blk->NextWaitBlock;
      }

      if (Timeout != NULL)
      {
         CurrentThread->WaitBlock[3].Object = (PVOID) & CurrentThread->Timer;
         CurrentThread->WaitBlock[3].Thread = CurrentThread;
         CurrentThread->WaitBlock[3].WaitKey = STATUS_TIMEOUT;
         CurrentThread->WaitBlock[3].WaitType = WaitAny;
         CurrentThread->WaitBlock[3].NextWaitBlock = NULL;

         InsertTailList(&CurrentThread->Timer.Header.WaitListHead,
                        &CurrentThread->WaitBlock[3].WaitListEntry);
      }

      //io completion
      if (CurrentThread->Queue)
      {
         CurrentThread->Queue->RunningThreads--;   
         if (WaitReason != WrQueue && CurrentThread->Queue->RunningThreads < CurrentThread->Queue->MaximumThreads &&
             !IsListEmpty(&CurrentThread->Queue->EntryListHead))
         {
            KeDispatcherObjectWake(&CurrentThread->Queue->Header);
         }
      }

      PsBlockThread(&Status, Alertable, WaitMode, TRUE, WaitIrql, WaitReason);

      //io completion
      if (CurrentThread->Queue)
      {
         CurrentThread->Queue->RunningThreads++;
      }


   }
   while (Status == STATUS_KERNEL_APC);

   if (Timeout != NULL)
   {
      KeCancelTimer(&CurrentThread->Timer);
   }

   DPRINT("Returning from KeWaitForMultipleObjects()\n");
   return (Status);
}

VOID KeInitializeDispatcher(VOID)
{
   KeInitializeSpinLock(&DispatcherDatabaseLock);
}

NTSTATUS STDCALL
NtWaitForMultipleObjects(IN ULONG Count,
			 IN HANDLE Object [],
			 IN WAIT_TYPE WaitType,
			 IN BOOLEAN Alertable,
			 IN PLARGE_INTEGER Time)
{
   KWAIT_BLOCK WaitBlockArray[EX_MAXIMUM_WAIT_OBJECTS];
   PVOID ObjectPtrArray[EX_MAXIMUM_WAIT_OBJECTS];
   NTSTATUS Status;
   ULONG i, j;
   KPROCESSOR_MODE WaitMode;

   DPRINT("NtWaitForMultipleObjects(Count %lu Object[] %x, Alertable %d, "
	  "Time %x)\n", Count,Object,Alertable,Time);

   if (Count > EX_MAXIMUM_WAIT_OBJECTS)
     return STATUS_UNSUCCESSFUL;

   WaitMode = ExGetPreviousMode();

   /* reference all objects */
   for (i = 0; i < Count; i++)
     {
        Status = ObReferenceObjectByHandle(Object[i],
                                           SYNCHRONIZE,
                                           NULL,
                                           WaitMode,
                                           &ObjectPtrArray[i],
                                           NULL);
        if (Status != STATUS_SUCCESS)
          {
             /* dereference all referenced objects */
             for (j = 0; j < i; j++)
               {
                  ObDereferenceObject(ObjectPtrArray[j]);
               }

             return(Status);
          }
     }

   Status = KeWaitForMultipleObjects(Count,
                                     ObjectPtrArray,
                                     WaitType,
                                     UserRequest,
                                     WaitMode,
                                     Alertable,
                                     Time,
                                     WaitBlockArray);

   /* dereference all objects */
   for (i = 0; i < Count; i++)
     {
        ObDereferenceObject(ObjectPtrArray[i]);
     }

   return(Status);
}


NTSTATUS STDCALL
NtWaitForSingleObject(IN HANDLE Object,
		      IN BOOLEAN Alertable,
		      IN PLARGE_INTEGER Time)
{
   PVOID ObjectPtr;
   NTSTATUS Status;
   KPROCESSOR_MODE WaitMode;

   DPRINT("NtWaitForSingleObject(Object %x, Alertable %d, Time %x)\n",
	  Object,Alertable,Time);

   WaitMode = ExGetPreviousMode();

   Status = ObReferenceObjectByHandle(Object,
				      SYNCHRONIZE,
				      NULL,
				      WaitMode,
				      &ObjectPtr,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   Status = KeWaitForSingleObject(ObjectPtr,
				  UserRequest,
				  WaitMode,
				  Alertable,
				  Time);

   ObDereferenceObject(ObjectPtr);

   return(Status);
}


NTSTATUS STDCALL
NtSignalAndWaitForSingleObject(IN HANDLE SignalObject,
			       IN HANDLE WaitObject,
			       IN BOOLEAN Alertable,
			       IN PLARGE_INTEGER Time)
{
   KPROCESSOR_MODE WaitMode;
   DISPATCHER_HEADER* hdr;
   PVOID SignalObj;
   PVOID WaitObj;
   NTSTATUS Status;

   WaitMode = ExGetPreviousMode();
   Status = ObReferenceObjectByHandle(SignalObject,
				      0,
				      NULL,
				      WaitMode,
				      &SignalObj,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }

   Status = ObReferenceObjectByHandle(WaitObject,
				      SYNCHRONIZE,
				      NULL,
				      WaitMode,
				      &WaitObj,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	ObDereferenceObject(SignalObj);
	return Status;
     }

   hdr = (DISPATCHER_HEADER *)SignalObj;
   switch (hdr->Type)
     {
      case InternalNotificationEvent:
      case InternalSynchronizationEvent:
	KeSetEvent(SignalObj,
		   EVENT_INCREMENT,
		   TRUE);
	break;

      case InternalMutexType:
	KeReleaseMutex(SignalObj,
		       TRUE);
	break;

      case InternalSemaphoreType:
	KeReleaseSemaphore(SignalObj,
			   SEMAPHORE_INCREMENT,
			   1,
			   TRUE);
	break;

      default:
	ObDereferenceObject(SignalObj);
	ObDereferenceObject(WaitObj);
	return STATUS_OBJECT_TYPE_MISMATCH;
     }

   Status = KeWaitForSingleObject(WaitObj,
				  UserRequest,
				  WaitMode,
				  Alertable,
				  Time);

   ObDereferenceObject(SignalObj);
   ObDereferenceObject(WaitObj);

   return Status;
}
