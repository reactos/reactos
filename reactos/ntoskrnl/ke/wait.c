/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS project
 * FILE:            ntoskrnl/ke/wait.c
 * PURPOSE:         Manages non-busy waiting
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Phillip Susi
 */

/* NOTES ********************************************************************
 *
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static KSPIN_LOCK DispatcherDatabaseLock;

#define KeDispatcherObjectWakeOne(hdr, increment) KeDispatcherObjectWakeOneOrAll(hdr, increment, FALSE)
#define KeDispatcherObjectWakeAll(hdr, increment) KeDispatcherObjectWakeOneOrAll(hdr, increment, TRUE)

extern POBJECT_TYPE EXPORTED ExMutantObjectType;
extern POBJECT_TYPE EXPORTED ExSemaphoreObjectType;
extern POBJECT_TYPE EXPORTED ExTimerType;

/* FUNCTIONS *****************************************************************/

VOID KeInitializeDispatcherHeader(DISPATCHER_HEADER* Header,
				  ULONG Type,
				  ULONG Size,
				  ULONG SignalState)
{
   Header->Type = (UCHAR)Type;
   Header->Absolute = 0;
   Header->Inserted = 0;
   Header->Size = (UCHAR)Size;
   Header->SignalState = SignalState;
   InitializeListHead(&(Header->WaitListHead));
}


KIRQL
KeAcquireDispatcherDatabaseLock(VOID)
/*
 * PURPOSE: Acquires the dispatcher database lock for the caller
 */
{
   KIRQL OldIrql;

   DPRINT("KeAcquireDispatcherDatabaseLock()\n");

   KeAcquireSpinLock (&DispatcherDatabaseLock, &OldIrql);
   return OldIrql;
}


VOID
KeAcquireDispatcherDatabaseLockAtDpcLevel(VOID)
/*
 * PURPOSE: Acquires the dispatcher database lock for the caller
 */
{
   DPRINT("KeAcquireDispatcherDatabaseLockAtDpcLevel()\n");

   KeAcquireSpinLockAtDpcLevel (&DispatcherDatabaseLock);
}


VOID
KeReleaseDispatcherDatabaseLock(KIRQL OldIrql)
{
  DPRINT("KeReleaseDispatcherDatabaseLock(OldIrql %x)\n",OldIrql);
  if (!KeIsExecutingDpc() && 
      OldIrql < DISPATCH_LEVEL && 
      KeGetCurrentThread() != NULL && 
      KeGetCurrentThread() == KeGetCurrentPrcb()->IdleThread)
  {
    PsDispatchThreadNoLock(THREAD_STATE_READY);
    KeLowerIrql(OldIrql);
  }
  else
  {
    KeReleaseSpinLock(&DispatcherDatabaseLock, OldIrql);
  }
}


VOID
KeReleaseDispatcherDatabaseLockFromDpcLevel(VOID)
{
  DPRINT("KeReleaseDispatcherDatabaseLock()\n");

  KeReleaseSpinLockFromDpcLevel(&DispatcherDatabaseLock);
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
         break;
         
      case InternalSemaphoreType:
         hdr->SignalState--;
         break;

      case InternalProcessType:
         break;

      case ThreadObject:
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
         ASSERT(hdr->SignalState <= 1);
         if (hdr->SignalState == 0)
         {
            if (Thread == NULL)
            {
               DPRINT("Thread == NULL!\n");
               KEBUGCHECK(0);
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
         KEBUGCHECK(0);
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

      ASSERT(hdr->SignalState <= 1);

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

/* Must be called with the dispatcher lock held */
BOOLEAN KiAbortWaitThread(PKTHREAD Thread, NTSTATUS WaitStatus)
{
   PKWAIT_BLOCK WaitBlock;
   BOOLEAN WasWaiting;

   /* if we are blocked, we must be waiting on something also */
   ASSERT((Thread->State == THREAD_STATE_BLOCKED) == (Thread->WaitBlockList != NULL));

   WaitBlock = (PKWAIT_BLOCK)Thread->WaitBlockList;
   WasWaiting = (WaitBlock != NULL);
   
   while (WaitBlock)
   {
      RemoveEntryList(&WaitBlock->WaitListEntry);
      WaitBlock = WaitBlock->NextWaitBlock;
   }
   
   Thread->WaitBlockList = NULL;

   if (WasWaiting)
   {
	   PsUnblockThread((PETHREAD)Thread, &WaitStatus, 0);
   }
   return WasWaiting;
}

static BOOLEAN
KeDispatcherObjectWakeOneOrAll(DISPATCHER_HEADER * hdr,
                               KPRIORITY increment,
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
      ASSERT(WaiterHead->Thread != NULL);
      ASSERT(WaiterHead->Thread->WaitBlockList != NULL);

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
         Abandoned |= KiSideEffectsBeforeWake(hdr, WaiterHead->Thread);
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
                  Abandoned |= KiSideEffectsBeforeWake(Waiter->Object, Waiter->Thread);
               }

               //no WaitAny objects can possibly be signaled since we are here
               ASSERT(!(Waiter->WaitType == WaitAny
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
         PsUnblockThread(CONTAINING_RECORD(WaiterHead->Thread, ETHREAD, Tcb),
                         &Status, increment);
      }
   }

   return WakedAny;
}


BOOLEAN KiDispatcherObjectWake(DISPATCHER_HEADER* hdr, KPRIORITY increment)
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
	return(KeDispatcherObjectWakeAll(hdr, increment));

      case InternalNotificationTimer:
	return(KeDispatcherObjectWakeAll(hdr, increment));

      case InternalSynchronizationEvent:
	return(KeDispatcherObjectWakeOne(hdr, increment));

      case InternalSynchronizationTimer:
	return(KeDispatcherObjectWakeOne(hdr, increment));

      case InternalQueueType:
	return(KeDispatcherObjectWakeOne(hdr, increment));
      
      case InternalSemaphoreType:
	DPRINT("hdr->SignalState %d\n", hdr->SignalState);
	if(hdr->SignalState>0)
	  {
	    do
	      {
		DPRINT("Waking one semaphore waiter\n");
		Ret = KeDispatcherObjectWakeOne(hdr, increment);
	      } while(hdr->SignalState > 0 &&  Ret) ;
	    return(Ret);
	  }
	else return FALSE;

     case InternalProcessType:
	return(KeDispatcherObjectWakeAll(hdr, increment));

     case ThreadObject:
       return(KeDispatcherObjectWakeAll(hdr, increment));

     case InternalMutexType:
       return(KeDispatcherObjectWakeOne(hdr, increment));
     }
   DbgPrint("Dispatcher object %x has unknown type %d\n", hdr, hdr->Type);
   KEBUGCHECK(0);
   return(FALSE);
}

/*
 * @implemented
 */
NTSTATUS STDCALL
KeDelayExecutionThread (KPROCESSOR_MODE	WaitMode,
			BOOLEAN		Alertable,
			PLARGE_INTEGER	Interval)
/*
 * FUNCTION: Puts the current thread into an alertable or nonalertable 
 * wait state for a given internal
 * ARGUMENTS:
 *          WaitMode = Processor mode in which the caller is waiting
 *          Altertable = Specifies if the wait is alertable
 *          Interval = Specifies the interval to wait
 * RETURNS: Status
 */
{
   PKTHREAD Thread = KeGetCurrentThread();

   KeSetTimer(&Thread->Timer, *Interval, NULL);
   return (KeWaitForSingleObject(&Thread->Timer,
				 (WaitMode == KernelMode) ? Executive : UserRequest, /* TMN: Was unconditionally Executive */
				 WaitMode, /* TMN: Was UserMode */
				 Alertable,
				 NULL));
}

/*
 * @implemented
 */
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


inline BOOL
KiIsObjectWaitable(PVOID Object)
{
    POBJECT_HEADER Header;
    Header = BODY_TO_HEADER(Object);
    if (Header->ObjectType == ExEventObjectType ||
	Header->ObjectType == ExIoCompletionType ||
	Header->ObjectType == ExMutantObjectType ||
	Header->ObjectType == ExSemaphoreObjectType ||
	Header->ObjectType == ExTimerType ||
	Header->ObjectType == PsProcessType ||
	Header->ObjectType == PsThreadType ||
	Header->ObjectType == IoFileObjectType)
    {
       return TRUE;
    }
    else
    {
       return FALSE;
    }
}

/*
 * @implemented
 */
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
   KIRQL OldIrql;
   BOOLEAN Abandoned;
   NTSTATUS WaitStatus;

   DPRINT("Entering KeWaitForMultipleObjects(Count %lu Object[] %p) "
          "PsGetCurrentThread() %x\n", Count, Object, PsGetCurrentThread());

   ASSERT(0 < Count && Count <= MAXIMUM_WAIT_OBJECTS);

   CurrentThread = KeGetCurrentThread();

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
      if (Count > MAXIMUM_WAIT_OBJECTS)
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
      KeSetTimer(&CurrentThread->Timer, *Timeout, NULL);
   }

   do
   {
      if (CurrentThread->WaitNext)
      {
         CurrentThread->WaitNext = FALSE;
         OldIrql = CurrentThread->WaitIrql;
      }
      else
      {
         OldIrql = KeAcquireDispatcherDatabaseLock ();
      }

    /* Get the current Wait Status */
    WaitStatus = CurrentThread->WaitStatus;
   
    if (Alertable) {
    
        /* If the Thread is Alerted, set the Wait Status accordingly */    
        if (CurrentThread->Alerted[(int)WaitMode]) {
            
            CurrentThread->Alerted[(int)WaitMode] = FALSE;
            DPRINT("Thread was Alerted\n");
            WaitStatus = STATUS_ALERTED;
            
        /* If there are User APCs Pending, then we can't really be alertable */
        } else if ((!IsListEmpty(&CurrentThread->ApcState.ApcListHead[UserMode])) && 
                   (WaitMode == UserMode)) {
            
            DPRINT1("APCs are Pending\n");
            CurrentThread->ApcState.UserApcPending = TRUE;
            WaitStatus = STATUS_USER_APC;
        }
        
    /* If there are User APCs Pending and we are waiting in usermode, then we must notify the caller */
    } else if ((CurrentThread->ApcState.UserApcPending) && (WaitMode == UserMode)) {
            DPRINT1("APCs are Pending\n");
            WaitStatus = STATUS_USER_APC;
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

               KeReleaseDispatcherDatabaseLock(OldIrql);               
               
               if (Timeout != NULL && Timeout->QuadPart != 0)
               {
                  KeCancelTimer(&CurrentThread->Timer);
               }

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

         KeReleaseDispatcherDatabaseLock(OldIrql);
         
         if (Timeout != NULL && Timeout->QuadPart != 0)
         {
            KeCancelTimer(&CurrentThread->Timer);
         }

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
         KeReleaseDispatcherDatabaseLock(OldIrql);
         return STATUS_TIMEOUT;
      }

      /*
       * Check if we have already timed out
       */
      if (Timeout != NULL && KiIsObjectSignalled(&CurrentThread->Timer.Header, CurrentThread))
      {
         KiSideEffectsBeforeWake(&CurrentThread->Timer.Header, CurrentThread);
         KeReleaseDispatcherDatabaseLock(OldIrql);
         KeCancelTimer(&CurrentThread->Timer);
         return (STATUS_TIMEOUT);
      }

      /* Append wait block to the KTHREAD wait block list */
      CurrentThread->WaitBlockList = blk = WaitBlockArray;

      /*
       * Set up the wait
       */
      CurrentThread->WaitStatus = WaitStatus;;

      for (i = 0; i < Count; i++)
      {
         hdr = (DISPATCHER_HEADER *) KiGetWaitableObjectFromObject(Object[i]);

         blk->Object = KiGetWaitableObjectFromObject(Object[i]);
         blk->Thread = CurrentThread;
         blk->WaitKey = (USHORT)(STATUS_WAIT_0 + i);
         blk->WaitType = (USHORT)WaitType;

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

      //kernel queues
      if (CurrentThread->Queue && WaitReason != WrQueue)
      {
         DPRINT("queue: sleep on something else\n");
         CurrentThread->Queue->CurrentCount--;  
         
         //wake another thread
         if (CurrentThread->Queue->CurrentCount < CurrentThread->Queue->MaximumCount &&
             !IsListEmpty(&CurrentThread->Queue->EntryListHead))
         {
            KiDispatcherObjectWake(&CurrentThread->Queue->Header, IO_NO_INCREMENT);
         }
      }

      PsBlockThread(&Status, Alertable, WaitMode, TRUE, OldIrql, (UCHAR)WaitReason);

      //kernel queues
      OldIrql = KeAcquireDispatcherDatabaseLock ();
      if (CurrentThread->Queue && WaitReason != WrQueue)
      {
         DPRINT("queue: wake from something else\n");
         CurrentThread->Queue->CurrentCount++;
      }
      if (Status == STATUS_KERNEL_APC)
      {
         CurrentThread->WaitNext = TRUE;
         CurrentThread->WaitIrql = OldIrql;
      }
      else
      {
         KeReleaseDispatcherDatabaseLock(OldIrql);
      }
      
   } while (Status == STATUS_KERNEL_APC);
   

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
NtWaitForMultipleObjects(IN ULONG ObjectCount,
			 IN PHANDLE ObjectsArray,
			 IN WAIT_TYPE WaitType,
			 IN BOOLEAN Alertable,
			 IN PLARGE_INTEGER TimeOut  OPTIONAL)
{
   KWAIT_BLOCK WaitBlockArray[MAXIMUM_WAIT_OBJECTS];
   HANDLE SafeObjectsArray[MAXIMUM_WAIT_OBJECTS];
   PVOID ObjectPtrArray[MAXIMUM_WAIT_OBJECTS];
   ULONG i, j;
   KPROCESSOR_MODE PreviousMode;
   LARGE_INTEGER SafeTimeOut;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("NtWaitForMultipleObjects(ObjectCount %lu ObjectsArray[] %x, Alertable %d, "
	  "TimeOut %x)\n", ObjectCount,ObjectsArray,Alertable,TimeOut);

   PreviousMode = ExGetPreviousMode();

   if (ObjectCount > MAXIMUM_WAIT_OBJECTS)
     return STATUS_UNSUCCESSFUL;
   if (0 == ObjectCount)
     return STATUS_INVALID_PARAMETER;

   if(PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForRead(ObjectsArray,
                    ObjectCount * sizeof(ObjectsArray[0]),
                    sizeof(ULONG));
       /* make a copy so we don't have to guard with SEH later and keep track of
          what objects we referenced in case dereferencing pointers suddenly fails */
       RtlCopyMemory(SafeObjectsArray, ObjectsArray, ObjectCount * sizeof(ObjectsArray[0]));
       ObjectsArray = SafeObjectsArray;
       
       if(TimeOut != NULL)
       {
         ProbeForRead(TimeOut,
                      sizeof(LARGE_INTEGER),
                      sizeof(ULONG));
         /* make a local copy of the timeout on the stack */
         SafeTimeOut = *TimeOut;
         TimeOut = &SafeTimeOut;
       }
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
     
     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }

   /* reference all objects */
   for (i = 0; i < ObjectCount; i++)
     {
        Status = ObReferenceObjectByHandle(ObjectsArray[i],
                                           SYNCHRONIZE,
                                           NULL,
                                           PreviousMode,
                                           &ObjectPtrArray[i],
                                           NULL);
        if (!NT_SUCCESS(Status) || !KiIsObjectWaitable(ObjectPtrArray[i]))
          {
             if (NT_SUCCESS(Status))
	       {
	         DPRINT1("Waiting for object type '%wZ' is not supported\n", 
		         &BODY_TO_HEADER(ObjectPtrArray[i])->ObjectType->TypeName);
	         Status = STATUS_HANDLE_NOT_WAITABLE;
		 i++;
	       }
             /* dereference all referenced objects */
             for (j = 0; j < i; j++)
               {
                  ObDereferenceObject(ObjectPtrArray[j]);
               }

             return(Status);
          }
     }

   Status = KeWaitForMultipleObjects(ObjectCount,
                                     ObjectPtrArray,
                                     WaitType,
                                     UserRequest,
                                     PreviousMode,
                                     Alertable,
				     TimeOut,
                                     WaitBlockArray);

   /* dereference all objects */
   for (i = 0; i < ObjectCount; i++)
     {
        ObDereferenceObject(ObjectPtrArray[i]);
     }

   return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtWaitForSingleObject(IN HANDLE ObjectHandle,
		      IN BOOLEAN Alertable,
		      IN PLARGE_INTEGER TimeOut  OPTIONAL)
{
   PVOID ObjectPtr;
   KPROCESSOR_MODE PreviousMode;
   LARGE_INTEGER SafeTimeOut;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("NtWaitForSingleObject(ObjectHandle %x, Alertable %d, TimeOut %x)\n",
	  ObjectHandle,Alertable,TimeOut);

   PreviousMode = ExGetPreviousMode();
   
   if(TimeOut != NULL && PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForRead(TimeOut,
                    sizeof(LARGE_INTEGER),
                    sizeof(ULONG));
       /* make a copy on the stack */
       SafeTimeOut = *TimeOut;
       TimeOut = &SafeTimeOut;
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
     
     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }

   Status = ObReferenceObjectByHandle(ObjectHandle,
				      SYNCHRONIZE,
				      NULL,
				      PreviousMode,
				      &ObjectPtr,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   if (!KiIsObjectWaitable(ObjectPtr))
     {
       DPRINT1("Waiting for object type '%wZ' is not supported\n", 
	       &BODY_TO_HEADER(ObjectPtr)->ObjectType->TypeName);
       Status = STATUS_HANDLE_NOT_WAITABLE;
     }
   else
     {
       Status = KeWaitForSingleObject(ObjectPtr,
				      UserRequest,
				      PreviousMode,
				      Alertable,
				      TimeOut);
     }

   ObDereferenceObject(ObjectPtr);

   return(Status);
}


NTSTATUS STDCALL
NtSignalAndWaitForSingleObject(IN HANDLE ObjectHandleToSignal,
			       IN HANDLE WaitableObjectHandle,
			       IN BOOLEAN Alertable,
			       IN PLARGE_INTEGER TimeOut  OPTIONAL)
{
   KPROCESSOR_MODE PreviousMode;
   DISPATCHER_HEADER* hdr;
   PVOID SignalObj;
   PVOID WaitObj;
   LARGE_INTEGER SafeTimeOut;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();
   
   if(TimeOut != NULL && PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForRead(TimeOut,
                    sizeof(LARGE_INTEGER),
                    sizeof(ULONG));
       /* make a copy on the stack */
       SafeTimeOut = *TimeOut;
       TimeOut = &SafeTimeOut;
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
     
     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }
   
   Status = ObReferenceObjectByHandle(ObjectHandleToSignal,
				      0,
				      NULL,
				      PreviousMode,
				      &SignalObj,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }

   Status = ObReferenceObjectByHandle(WaitableObjectHandle,
				      SYNCHRONIZE,
				      NULL,
				      PreviousMode,
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
				  PreviousMode,
				  Alertable,
				  TimeOut);

   ObDereferenceObject(SignalObj);
   ObDereferenceObject(WaitObj);

   return Status;
}
