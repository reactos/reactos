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

/* FUNCTIONS *****************************************************************/

#ifdef DBG

VOID
KiValidateDispatcherObject(IN PDISPATCHER_HEADER  Object)
{
  if ((Object->Type < InternalBaseType)
    || (Object->Type > InternalTypeMaximum))
		{
      assertmsg(FALSE, ("Bad dispatcher object type %d\n", Object->Type));
		}
}


PKTHREAD
KiGetDispatcherObjectOwner(IN PDISPATCHER_HEADER  Header)
{
  switch (Header->Type)
  {
    case InternalMutexType:
    {
      PKMUTEX Mutex = (PKMUTEX)Header;
      return Mutex->OwnerThread;
    }

  default:
    /* No owner */
    return NULL;
  }
}


VOID KiDeadlockDetectionMutualResource(IN PDISPATCHER_HEADER  CurrentObject,
  IN PKTHREAD  CurrentThread,
  IN PKTHREAD  OwnerThread)
/*
 * PURPOSE: Perform deadlock detection. 
 *          At this point, OwnerThread owns CurrentObject and CurrentThread
 *          is waiting on CurrentObject
 *          Deadlock happens if OwnerThread is waiting on a resource that
 *          InitialThread owns.
 * NOTE: Called with the dispatcher database locked
 */
{
  PDISPATCHER_HEADER Object;
  PKWAIT_BLOCK WaitBlock;
  PKTHREAD Thread;

  DPRINT("KiDeadlockDetectionMutualResource("
    "CurrentObject 0x%.08x,  CurrentThread 0x%.08x  OwnerThread 0x%.08x)\n",
    CurrentObject, CurrentThread, OwnerThread);

  /* Go through all dispather objects that OwnerThread is waiting on */
  for (WaitBlock = OwnerThread->WaitBlockList;
    WaitBlock;
    WaitBlock = WaitBlock->NextWaitBlock)
    {
      Object = WaitBlock->Object;

      /* If OwnerThread is waiting on a resource that CurrentThread has
       * acquired then we have a deadlock */
      Thread = KiGetDispatcherObjectOwner(Object);
      if ((Thread != NULL) && (Thread == CurrentThread))
        {
          DbgPrint("Deadlock detected!\n");
          DbgPrint("Thread 0x%.08x is waiting on Object 0x%.08x of type %d "
            " which is owned by Thread 0x%.08x\n",
            Thread, CurrentObject, CurrentObject->Type, OwnerThread);
          DbgPrint("Thread 0x%.08x is waiting on Object 0x%.08x of type %d "
            " which is owned by Thread 0x%.08x\n",
            OwnerThread, Object, Object->Type, Thread);
          KeBugCheck(0);
        }
    }
}


VOID KiDeadlockDetection(IN PDISPATCHER_HEADER  Header)
/*
 * PURPOSE: Perform deadlock detection
 * NOTE: Called with the dispatcher database locked
 */
{
  PLIST_ENTRY CurrentEntry;
  PKWAIT_BLOCK WaitBlock;
  PKWAIT_BLOCK Current;
  PKTHREAD OwnerThread;
  PKTHREAD Thread;

  DPRINT("KiDeadlockDetection(Header %x)\n", Header);
  if (IsListEmpty(&(Header->WaitListHead)))
    return;

  CurrentEntry = Header->WaitListHead.Flink;
  Current = CONTAINING_RECORD(CurrentEntry, KWAIT_BLOCK, WaitListEntry);

  /* Go through all threads waiting on this dispatcher object */
  for (WaitBlock = Current->Thread->WaitBlockList;
    WaitBlock;
    WaitBlock = WaitBlock->NextWaitBlock)
    {
      Thread = WaitBlock->Thread;

      DPRINT("KiDeadlockDetection: WaitBlock->Thread %x  waiting on  WaitBlock->Object %x\n",
        WaitBlock->Thread, WaitBlock->Object);

      /* If another thread is currently owning this dispatcher object,
         see if we have a deadlock */
      OwnerThread = KiGetDispatcherObjectOwner(Current->Object);
      DPRINT("KiDeadlockDetection: OwnerThread %x\n", OwnerThread);
      if ((OwnerThread != NULL) && (OwnerThread != Thread))
        {
          KiDeadlockDetectionMutualResource(Current->Object,
            Current->Thread,
            OwnerThread);
        }
    }
}

#endif /* DBG */


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

static VOID KiSideEffectsBeforeWake(DISPATCHER_HEADER* hdr,
				    PKTHREAD Thread,
				    PBOOLEAN Abandoned)
/*
 * FUNCTION: Perform side effects on object before a wait for a thread is
 *           satisfied
 */
{
  if (Abandoned != NULL)
    *Abandoned = FALSE;

   switch (hdr->Type)
     {
      case InternalSynchronizationEvent:
	hdr->SignalState = 0;
	break;
	
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
		      DPRINT1("Thread == NULL!\n");
		      KeBugCheck(0);
		    }

		  if (Abandoned != NULL)
		    *Abandoned = Mutex->Abandoned;
		  if (Thread != NULL)
		    InsertTailList(&Thread->MutantListHead,
				   &Mutex->MutantListEntry);
		  Mutex->OwnerThread = Thread;
		  Mutex->Abandoned = FALSE;
	       }
	  }
	break;
	
      default:
	DbgPrint("(%s:%d) Dispatcher object %x has unknown type\n",
		 __FILE__,__LINE__,hdr);
	KeBugCheck(0);
     }
}

static BOOLEAN
KiIsObjectSignalled(DISPATCHER_HEADER* hdr,
		    PKTHREAD Thread,
		    PBOOLEAN Abandoned)
{
  if (Abandoned != NULL)
    *Abandoned = FALSE;

   if (hdr->Type == InternalMutexType)
     {
        PKMUTEX Mutex;
	
	Mutex = CONTAINING_RECORD(hdr, KMUTEX, Header);
	
	assert(hdr->SignalState <= 1);
	if ((hdr->SignalState < 1 && Mutex->OwnerThread == Thread) ||
	    hdr->SignalState == 1)
	  {
	     KiSideEffectsBeforeWake(hdr,
				     Thread,
				     Abandoned);
	     return(TRUE);
	  }
	else
	  {
	     return(FALSE);
	  }
     }
   if (hdr->SignalState <= 0)
     {
	return(FALSE);
     }
   else
     {
	KiSideEffectsBeforeWake(hdr,
				Thread,
				Abandoned);
	return(TRUE);
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

static BOOLEAN KeDispatcherObjectWakeAll(DISPATCHER_HEADER* hdr)
{
  PKWAIT_BLOCK current;
  PLIST_ENTRY current_entry;
  PKWAIT_BLOCK PrevBlock;
  NTSTATUS Status;
  BOOLEAN Abandoned;

  DPRINT("KeDispatcherObjectWakeAll(hdr %x)\n",hdr);
  
  if (IsListEmpty(&hdr->WaitListHead))
    {
      return(FALSE);
    }
  
  while (!IsListEmpty(&hdr->WaitListHead))
    {
      current_entry = RemoveHeadList(&hdr->WaitListHead);
      current = CONTAINING_RECORD(current_entry,
				  KWAIT_BLOCK,
				  WaitListEntry);
      DPRINT("Waking %x\n",current->Thread);
      if (current->WaitType == WaitAny)
	{
	  DPRINT("WaitAny: Remove all wait blocks.\n");
	  for(PrevBlock = current->Thread->WaitBlockList; PrevBlock;
	      PrevBlock = PrevBlock->NextWaitBlock)
	    {
	      if (PrevBlock != current)
		RemoveEntryList(&PrevBlock->WaitListEntry);
	    }
	  current->Thread->WaitBlockList = 0;
	}
      else
	{
	  DPRINT("WaitAll: Remove the current wait block only.\n");
	  
	  PrevBlock = current->Thread->WaitBlockList;
	  if (PrevBlock == current)
	    {
	      DPRINT( "WaitAll: Current block is list head.\n" );
	      current->Thread->WaitBlockList = current->NextWaitBlock;
	    }
	  else
	    {
	      DPRINT( "WaitAll: Current block is not list head.\n" );
	      while (PrevBlock && PrevBlock->NextWaitBlock != current)
		{
		  PrevBlock = PrevBlock->NextWaitBlock;
		}
	      if (PrevBlock)
		{
		  PrevBlock->NextWaitBlock = current->NextWaitBlock;
		}
	    }
	}
      KiSideEffectsBeforeWake(hdr, current->Thread, &Abandoned);
      Status = current->WaitKey;
      if (Abandoned)
	Status += STATUS_ABANDONED_WAIT_0;
      if (current->Thread->WaitBlockList == NULL)
	{
	  PsUnblockThread(CONTAINING_RECORD(current->Thread,ETHREAD,Tcb),
			  &Status);
	}
    }
  return(TRUE);
}

static BOOLEAN KeDispatcherObjectWakeOne(DISPATCHER_HEADER* hdr)
{
  PKWAIT_BLOCK current;
  PLIST_ENTRY current_entry;
  PKWAIT_BLOCK PrevBlock;
  NTSTATUS Status;
  BOOLEAN Abandoned;

  DPRINT("KeDispatcherObjectWakeOn(hdr %x)\n",hdr);
  DPRINT("hdr->WaitListHead.Flink %x hdr->WaitListHead.Blink %x\n",
	 hdr->WaitListHead.Flink,hdr->WaitListHead.Blink);
  if (IsListEmpty(&(hdr->WaitListHead)))
    {
      return(FALSE);
    }
  current_entry = RemoveHeadList(&(hdr->WaitListHead));
  current = CONTAINING_RECORD(current_entry,KWAIT_BLOCK,
			      WaitListEntry);
  DPRINT("current_entry %x current %x\n",current_entry,current);

   if (current->WaitType == WaitAny)
     {
        DPRINT("WaitAny: Remove all wait blocks.\n");
	for( PrevBlock = current->Thread->WaitBlockList; PrevBlock; PrevBlock = PrevBlock->NextWaitBlock )
	  if( PrevBlock != current )
	    RemoveEntryList( &(PrevBlock->WaitListEntry) );
	current->Thread->WaitBlockList = 0;
     }
   else
     {
        DPRINT("WaitAll: Remove the current wait block only.\n");

        PrevBlock = current->Thread->WaitBlockList;
        if (PrevBlock == current)
           {
              DPRINT( "WaitAll: Current block is list head.\n" );
              current->Thread->WaitBlockList = current->NextWaitBlock;
           }
        else
           {
              DPRINT( "WaitAll: Current block is not list head.\n" );
              while ( PrevBlock && PrevBlock->NextWaitBlock != current)
                {
                   PrevBlock = PrevBlock->NextWaitBlock;
                }
              if (PrevBlock)
                {
                   PrevBlock->NextWaitBlock = current->NextWaitBlock;
                }
	   }
    }

  DPRINT("Waking %x\n",current->Thread);
  KiSideEffectsBeforeWake(hdr, current->Thread, &Abandoned);
  Status = current->WaitKey;
  if (Abandoned)
    Status += STATUS_ABANDONED_WAIT_0;
  PsUnblockThread(CONTAINING_RECORD(current->Thread, ETHREAD, Tcb),
		  &Status);
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
      case InternalNotificationEvent:
	return(KeDispatcherObjectWakeAll(hdr));
	
      case InternalNotificationTimer:
	return(KeDispatcherObjectWakeAll(hdr));
	
      case InternalSynchronizationEvent:
	return(KeDispatcherObjectWakeOne(hdr));

      case InternalSynchronizationTimer:
	return(KeDispatcherObjectWakeOne(hdr));
	
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
   DISPATCHER_HEADER* hdr = (DISPATCHER_HEADER *)Object;
   PKTHREAD CurrentThread;
   NTSTATUS Status;
   KIRQL WaitIrql;
   BOOLEAN Abandoned;

   assert_irql(DISPATCH_LEVEL);

   VALIDATE_DISPATCHER_OBJECT(Object);

   CurrentThread = KeGetCurrentThread();
   WaitIrql = KeGetCurrentIrql();

   /*
    * Set up the timeout
    * FIXME: Check for zero timeout
    */
   if (Timeout != NULL)
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
	   return(STATUS_USER_APC);
	 }

       /*
	* If the object is signalled
	*/
       if (KiIsObjectSignalled(hdr, CurrentThread, &Abandoned))
	 {
	   KeReleaseDispatcherDatabaseLock(FALSE);
	   if (Timeout != NULL)
	     {
	       KeCancelTimer(&KeGetCurrentThread()->Timer);
	     }
	   if (Abandoned == TRUE)
	     return(STATUS_ABANDONED_WAIT_0);
	   return(STATUS_WAIT_0);
	 }

       /*
	* Check if we have already timed out
	*/
       if (Timeout != NULL && 
	   KiIsObjectSignalled(&CurrentThread->Timer.Header, CurrentThread, NULL))
	 {
	   KeReleaseDispatcherDatabaseLock(FALSE);
	   if (Timeout != NULL)
	     {
	       KeCancelTimer(&KeGetCurrentThread()->Timer);
	     }
	   return(STATUS_TIMEOUT);
	 }

#ifdef DBG

   if (hdr->Type == InternalMutexType)
     {
        PKMUTEX Mutex;

	      Mutex = CONTAINING_RECORD(hdr, KMUTEX, Header);
        assertmsg(Mutex->OwnerThread != CurrentThread,
				  ("Recursive locking of mutex (0x%.08x)\n", Mutex));
    }

#endif /* DBG */
		
	/*
   * Set up for a wait
 	 */
       CurrentThread->WaitStatus = STATUS_UNSUCCESSFUL;
       /* Append wait block to the KTHREAD wait block list */
       CurrentThread->WaitBlockList = &CurrentThread->WaitBlock[0];
       CurrentThread->WaitBlock[0].Object = Object;
       CurrentThread->WaitBlock[0].Thread = CurrentThread;
       CurrentThread->WaitBlock[0].WaitKey = STATUS_WAIT_0;
       CurrentThread->WaitBlock[0].WaitType = WaitAny;
       InsertTailList(&hdr->WaitListHead, 
		      &CurrentThread->WaitBlock[0].WaitListEntry);
       if (Timeout != NULL)
	 {
	   CurrentThread->WaitBlock[0].NextWaitBlock = 
	     &CurrentThread->WaitBlock[1];
	   CurrentThread->WaitBlock[1].Object = (PVOID)&CurrentThread->Timer;
	   CurrentThread->WaitBlock[1].Thread = CurrentThread;
	   CurrentThread->WaitBlock[1].WaitKey = STATUS_TIMEOUT;
	   CurrentThread->WaitBlock[1].WaitType = WaitAny;
	   CurrentThread->WaitBlock[1].NextWaitBlock = NULL;
	   InsertTailList(&CurrentThread->Timer.Header.WaitListHead,
			  &CurrentThread->WaitBlock[1].WaitListEntry);
	 }
       else
	 {
	   CurrentThread->WaitBlock[0].NextWaitBlock = NULL;
	 }

#ifdef DBG
			/*
		   * Do deadlock detection in checked version
       * NOTE: This must be done after the the dispatcher object is put on
       * the wait block list.
		 	 */
		  KiDeadlockDetection(Object);
#endif /* DBG */

       PsBlockThread(&Status, (UCHAR)Alertable, WaitMode, TRUE, WaitIrql);
     } while (Status == STATUS_KERNEL_APC);
   
   if (Timeout != NULL)
     {
       KeCancelTimer(&KeGetCurrentThread()->Timer);
     }

   DPRINT("Returning from KeWaitForSingleObject()\n");
   return(Status);
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
  DISPATCHER_HEADER* hdr;
  PKWAIT_BLOCK blk;
  PKTHREAD CurrentThread;
  ULONG CountSignaled;
  ULONG i;
  NTSTATUS Status;
  KIRQL WaitIrql;
  BOOLEAN Abandoned;

  DPRINT("Entering KeWaitForMultipleObjects(Count %lu Object[] %p) "
	 "PsGetCurrentThread() %x\n",Count,Object,PsGetCurrentThread());

  assert_irql(APC_LEVEL);

  CountSignaled = 0;
  CurrentThread = KeGetCurrentThread();
  WaitIrql = KeGetCurrentIrql();

  /*
   * Work out where we are going to put the wait blocks
   */
  if (WaitBlockArray == NULL)
    {
      if (Count > THREAD_WAIT_OBJECTS)
        {
	  DbgPrint("(%s:%d) Too many objects!\n",
		   __FILE__,__LINE__);
	  return(STATUS_UNSUCCESSFUL);
        }
      blk = &CurrentThread->WaitBlock[0];
    }
  else
    {
      if (Count > EX_MAXIMUM_WAIT_OBJECTS)
        {
	  DbgPrint("(%s:%d) Too many objects!\n",
		   __FILE__,__LINE__);
	  return(STATUS_UNSUCCESSFUL);
        }
      blk = WaitBlockArray;
    }

  /*
   * Set up the timeout if required
   */
  if (Timeout != NULL)
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
	   return(STATUS_USER_APC);
	 }

       /*
	* Check if the wait is already satisfied
	*/
       for (i = 0; i < Count; i++)
	 {
	   hdr = (DISPATCHER_HEADER *)Object[i];

     VALIDATE_DISPATCHER_OBJECT(hdr);
	   
	   if (KiIsObjectSignalled(hdr, CurrentThread, &Abandoned))
	     {
	       CountSignaled++;
	       
	       if (WaitType == WaitAny)
		 {
		   KeReleaseDispatcherDatabaseLock(FALSE);
		   DPRINT("One object is already signaled!\n");
		   if (Abandoned == TRUE)
		     return(STATUS_ABANDONED_WAIT_0 + i);
		   return(STATUS_WAIT_0 + i);
		 }
	     }
	 }
       
       if ((WaitType == WaitAll) && (CountSignaled == Count))
	 {
	   KeReleaseDispatcherDatabaseLock(FALSE);
	   DPRINT("All objects are already signaled!\n");
	   return(STATUS_WAIT_0);
	 }
    
       /*
	* Check if we have already timed out
	*/
       if (Timeout != NULL && 
	   KiIsObjectSignalled(&CurrentThread->Timer.Header, CurrentThread, NULL))
	 {
	   KeReleaseDispatcherDatabaseLock(FALSE);
	   if (Timeout != NULL)
	     {
	       KeCancelTimer(&KeGetCurrentThread()->Timer);
	     }
	   return(STATUS_TIMEOUT);
	 }

       /* Append wait block to the KTHREAD wait block list */
       CurrentThread->WaitBlockList = blk;
  
       /*
	* Set up the wait
	*/
       for (i = 0; i < Count; i++)
	 {
	   hdr = (DISPATCHER_HEADER *)Object[i];
	   assertmsg(hdr != NULL, ("Waiting on uninitialized object\n"));
	   blk->Object = Object[i];
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

	   InsertTailList(&hdr->WaitListHead, &blk->WaitListEntry);
	   
	   blk = blk->NextWaitBlock;
	 }
       if (Timeout != NULL)
	 {
	   CurrentThread->WaitBlock[3].Object = (PVOID)&CurrentThread->Timer;
	   CurrentThread->WaitBlock[3].Thread = CurrentThread;
	   CurrentThread->WaitBlock[3].WaitKey = STATUS_TIMEOUT;
	   CurrentThread->WaitBlock[3].WaitType = WaitAny;
	   CurrentThread->WaitBlock[3].NextWaitBlock = NULL;
	   InsertTailList(&CurrentThread->Timer.Header.WaitListHead,
			  &CurrentThread->WaitBlock[3].WaitListEntry);
	 }

       PsBlockThread(&Status, Alertable, WaitMode, TRUE, WaitIrql);
    } while(Status == STATUS_KERNEL_APC);
  
  if (Timeout != NULL)
    {
      KeCancelTimer(&KeGetCurrentThread()->Timer);
    }

  DPRINT("Returning from KeWaitForMultipleObjects()\n");
  return(Status);
}

VOID KeInitializeDispatcher(VOID)
{
   KeInitializeSpinLock(&DispatcherDatabaseLock);
}

NTSTATUS STDCALL
NtWaitForMultipleObjects(IN ULONG Count,
			 IN HANDLE Object [],
			 IN CINT WaitType,
			 IN BOOLEAN Alertable,
			 IN PLARGE_INTEGER Time)
{
   KWAIT_BLOCK WaitBlockArray[EX_MAXIMUM_WAIT_OBJECTS];
   PVOID ObjectPtrArray[EX_MAXIMUM_WAIT_OBJECTS];
   NTSTATUS Status;
   ULONG i, j;

   DPRINT("NtWaitForMultipleObjects(Count %lu Object[] %x, Alertable %d, "
	  "Time %x)\n", Count,Object,Alertable,Time);

   if (Count > EX_MAXIMUM_WAIT_OBJECTS)
     return STATUS_UNSUCCESSFUL;

   /* reference all objects */
   for (i = 0; i < Count; i++)
     {
        Status = ObReferenceObjectByHandle(Object[i],
                                           SYNCHRONIZE,
                                           NULL,
                                           UserMode,
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
                                     UserMode,
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
   
   DPRINT("NtWaitForSingleObject(Object %x, Alertable %d, Time %x)\n",
	  Object,Alertable,Time);
   
   Status = ObReferenceObjectByHandle(Object,
				      SYNCHRONIZE,
				      NULL,
				      UserMode,
				      &ObjectPtr,
				      NULL);
   if (!NT_SUCCESS(Status))
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


NTSTATUS STDCALL
NtSignalAndWaitForSingleObject(IN HANDLE SignalObject,
			       IN HANDLE WaitObject,
			       IN BOOLEAN Alertable,
			       IN PLARGE_INTEGER Time)
{
   KPROCESSOR_MODE ProcessorMode;
   DISPATCHER_HEADER* hdr;
   PVOID SignalObj;
   PVOID WaitObj;
   NTSTATUS Status;

   ProcessorMode = ExGetPreviousMode();
   Status = ObReferenceObjectByHandle(SignalObject,
				      0,
				      NULL,
				      ProcessorMode,
				      &SignalObj,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }

   Status = ObReferenceObjectByHandle(WaitObject,
				      SYNCHRONIZE,
				      NULL,
				      ProcessorMode,
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
				  ProcessorMode,
				  Alertable,
				  Time);

   ObDereferenceObject(SignalObj);
   ObDereferenceObject(WaitObj);

   return Status;
}
