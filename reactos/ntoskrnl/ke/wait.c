/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS project
 * FILE:            ntoskrnl/ke/wait.c
 * PURPOSE:         Manages non-busy waiting
 * 
 * PROGRAMMERS:     Alex Ionescu - Fixes and optimization.
 *                  Gunnar Dalsnes - Implementation
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static KSPIN_LOCK DispatcherDatabaseLock;

/* Tells us if the Timer or Event is a Syncronization or Notification Object */
#define TIMER_OR_EVENT_TYPE 0x7L

/* One of the Reserved Wait Blocks, this one is for the Thread's Timer */
#define TIMER_WAIT_BLOCK 0x3L

/* FUNCTIONS *****************************************************************/

VOID
inline
FASTCALL
KiCheckAlertability(BOOLEAN Alertable,
                    PKTHREAD CurrentThread,
                    KPROCESSOR_MODE WaitMode,
                    PNTSTATUS Status)
{
    /* At this point, we have to do a wait, so make sure we can make the thread Alertable if requested */
    if (Alertable) {
    
        /* If the Thread is Alerted, set the Wait Status accordingly */    
        if (CurrentThread->Alerted[(int)WaitMode]) {
            
            CurrentThread->Alerted[(int)WaitMode] = FALSE;
            DPRINT("Thread was Alerted\n");
            *Status = STATUS_ALERTED;
            
        /* If there are User APCs Pending, then we can't really be alertable */
        } else if ((!IsListEmpty(&CurrentThread->ApcState.ApcListHead[UserMode])) && 
                    (WaitMode == UserMode)) {
            
            DPRINT("APCs are Pending\n");
            CurrentThread->ApcState.UserApcPending = TRUE;
            *Status = STATUS_USER_APC;
        }
    
    /* If there are User APCs Pending and we are waiting in usermode, then we must notify the caller */
    } else if ((CurrentThread->ApcState.UserApcPending) && (WaitMode == UserMode)) {
            DPRINT("APCs are Pending\n");
            *Status = STATUS_USER_APC;
    }
}

/*
 * @implemented
 *
 * FUNCTION: Puts the current thread into an alertable or nonalertable 
 * wait state for a given internal
 * ARGUMENTS:
 *          WaitMode = Processor mode in which the caller is waiting
 *          Altertable = Specifies if the wait is alertable
 *          Interval = Specifies the interval to wait
 * RETURNS: Status
 */
NTSTATUS 
STDCALL
KeDelayExecutionThread(KPROCESSOR_MODE WaitMode,
                       BOOLEAN Alertable,
                       PLARGE_INTEGER Interval)
{
    PKWAIT_BLOCK TimerWaitBlock;
    PKTIMER ThreadTimer;
    PKTHREAD CurrentThread = KeGetCurrentThread();
    NTSTATUS Status;

    DPRINT("Entering KeDelayExecutionThread\n");

    /* Check if the lock is already held */
    if (CurrentThread->WaitNext)  {
        
        /* Lock is held, disable Wait Next */
        DPRINT("Lock is held\n");
        CurrentThread->WaitNext = FALSE;
        
    } else {
        
        /* Lock not held, acquire it */
        DPRINT("Lock is not held, acquiring\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    }
    
    /* Use built-in Wait block */
    TimerWaitBlock = &CurrentThread->WaitBlock[TIMER_WAIT_BLOCK];
    
    /* Start Wait Loop */
    do {
    
        /* We are going to wait no matter what (that's the point), so test Alertability */
        KiCheckAlertability(Alertable, CurrentThread, KernelMode, &Status);
        
        /* Set Timer */
        ThreadTimer = &CurrentThread->Timer;
        
        /* Setup the Wait Block */
        CurrentThread->WaitBlockList = TimerWaitBlock;
        TimerWaitBlock->Object = (PVOID)ThreadTimer;
        TimerWaitBlock->Thread = CurrentThread;
        TimerWaitBlock->WaitKey = (USHORT)STATUS_TIMEOUT;
        TimerWaitBlock->WaitType = WaitAny;         
        TimerWaitBlock->NextWaitBlock = NULL;
    
        /* Link the timer to this Wait Block */
        InitializeListHead(&ThreadTimer->Header.WaitListHead);
        InsertTailList(&ThreadTimer->Header.WaitListHead, &TimerWaitBlock->WaitListEntry);

        /* Insert the Timer into the Timer Lists and enable it */
        if (!KiInsertTimer(ThreadTimer, *Interval)) {    

            /* FIXME: Unhandled case...what should we do? */
            DPRINT1("Could not create timer for KeDelayExecutionThread\n");
        }    
        
        /* Handle Kernel Queues */
        if (CurrentThread->Queue) {
                 
            DPRINT("Waking Queue\n");
            KiWakeQueue(CurrentThread->Queue);
        }

        /* Block the Thread */
        DPRINT("Blocking the Thread: %d, %d, %x\n", Alertable, WaitMode, KeGetCurrentThread());
        PsBlockThread(&Status, 
                      Alertable, 
                      WaitMode, 
                      DelayExecution);
    
        /* Check if we were executing an APC or if we timed out */
        if (Status != STATUS_KERNEL_APC) {
           
            /* This is a good thing */
            if (Status == STATUS_TIMEOUT) Status = STATUS_SUCCESS;
            
            /* Return Status */
            return Status;
        }
        
        DPRINT("Looping Again\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    
    } while (TRUE);
    
    /* Release the Lock, we are done */
    DPRINT("Returning from KeDelayExecutionThread(), %x. Status: %d\n", KeGetCurrentThread(), Status);
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return Status;   
}

/*
 * @implemented
 *
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
NTSTATUS 
STDCALL
KeWaitForSingleObject(PVOID Object,
                      KWAIT_REASON WaitReason,
                      KPROCESSOR_MODE WaitMode,
                      BOOLEAN Alertable,
                      PLARGE_INTEGER Timeout)
{
    PDISPATCHER_HEADER CurrentObject;
    PKWAIT_BLOCK WaitBlock;
    PKWAIT_BLOCK TimerWaitBlock;
    PKTIMER ThreadTimer;
    PKTHREAD CurrentThread = KeGetCurrentThread();
    NTSTATUS Status;
    NTSTATUS WaitStatus;

    DPRINT("Entering KeWaitForSingleObject\n");
   
    /* Check if the lock is already held */
    if (CurrentThread->WaitNext)  {
        
        /* Lock is held, disable Wait Next */
        DPRINT("Lock is held\n");
        CurrentThread->WaitNext = FALSE;
        
    } else {
        
        /* Lock not held, acquire it */
        DPRINT("Lock is not held, acquiring\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    }

    /* Start the actual Loop */
    do {
        
        /* Get the current Wait Status */
        WaitStatus = CurrentThread->WaitStatus;
    
        /* Append wait block to the KTHREAD wait block list */
        CurrentThread->WaitBlockList = WaitBlock = &CurrentThread->WaitBlock[0];
           
        /* Get the Current Object */
        CurrentObject = (PDISPATCHER_HEADER)Object;
            
        /* FIXME: 
         * Temporary hack until my Object Manager re-write. Basically some objects, like
         * the File Object, but also LPCs and others, are actually waitable on their event.
         * The Object Manager sets this up in The ObjectTypeInformation->DefaultObject member,
         * by using pretty much the same kind of hack as us. Normal objects point to themselves
         * in that pointer. Then, NtWaitForXXX will populate the WaitList that gets sent to us by
         * using ->DefaultObject, so the proper actual objects will be sent to us. Until then however,
         * I will keep this hack here, since there's no need to make an interim hack until the rewrite
         * -- Alex Ionescu 24/02/05
         */
        if (CurrentObject->Type == IO_TYPE_FILE) {
                   
            DPRINT1("Hack used: %x\n", &((PFILE_OBJECT)CurrentObject)->Event);
            CurrentObject = (PDISPATCHER_HEADER)(&((PFILE_OBJECT)CurrentObject)->Event);
        }

        /* Check if the Object is Signaled */
        if (KiIsObjectSignaled(CurrentObject, CurrentThread)) {
                    
            /* Just unwait this guy and exit */
            if (CurrentObject->SignalState != MINLONG) {
                    
                /* It has a normal signal state, so unwait it and return */
                KiSatisfyObjectWait(CurrentObject, CurrentThread);
                Status = STATUS_WAIT_0;
                goto WaitDone;
                    
            } else {
                    
                /* Is this a Mutant? */
                if (CurrentObject->Type == MutantObject) {
                        
                    /* According to wasm.ru, we must raise this exception (tested and true) */
                    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
                    ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);                    
                } 
            }
        }
        
        /* Set up the Wait Block */
        WaitBlock->Object = CurrentObject;
        WaitBlock->Thread = CurrentThread;
        WaitBlock->WaitKey = (USHORT)(STATUS_WAIT_0);
        WaitBlock->WaitType = WaitAny;                       
        WaitBlock->NextWaitBlock = NULL;
                      
        /* Make sure we can satisfy the Alertable request */
        KiCheckAlertability(Alertable, CurrentThread, WaitMode, &Status);
    
        /* Set the Wait Status */
        CurrentThread->WaitStatus = Status;
           
        /* Enable the Timeout Timer if there was any specified */
        if (Timeout != NULL) {
              
            /* However if 0 timeout was specified, then we must fail since we need to peform a wait */
            if (!Timeout->QuadPart) {
                
                /* Return a timeout */
                Status = STATUS_TIMEOUT;
                goto WaitDone;
            }
                       
            /* Point to Timer Wait Block and Thread Timer */
            TimerWaitBlock = &CurrentThread->WaitBlock[TIMER_WAIT_BLOCK];
            ThreadTimer = &CurrentThread->Timer;

            /* Connect the Timer Wait Block */
            WaitBlock->NextWaitBlock = TimerWaitBlock;
            
            /* Set up the Timer Wait Block */
            TimerWaitBlock->Object = (PVOID)ThreadTimer;
            TimerWaitBlock->Thread = CurrentThread;
            TimerWaitBlock->WaitKey = STATUS_TIMEOUT;
            TimerWaitBlock->WaitType = WaitAny;
            TimerWaitBlock->NextWaitBlock = NULL;
            
            /* Link the timer to this Wait Block */
            InitializeListHead(&ThreadTimer->Header.WaitListHead);
            InsertTailList(&ThreadTimer->Header.WaitListHead, &TimerWaitBlock->WaitListEntry);

            /* Insert the Timer into the Timer Lists and enable it */
            if (!KiInsertTimer(ThreadTimer, *Timeout)) {    

                /* Return a timeout if we couldn't insert the timer for some reason */
                Status = STATUS_TIMEOUT;
                goto WaitDone;
            }    
        }

        /* Link the Object to this Wait Block */
        InsertTailList(&CurrentObject->WaitListHead, &WaitBlock->WaitListEntry);            
        
        /* Handle Kernel Queues */
        if (CurrentThread->Queue) {
                 
            DPRINT("Waking Queue\n");
            KiWakeQueue(CurrentThread->Queue);
        }

        /* Block the Thread */
        DPRINT("Blocking the Thread: %d, %d, %d, %x\n", Alertable, WaitMode, WaitReason, KeGetCurrentThread());
        PsBlockThread(&Status, 
                      Alertable, 
                      WaitMode, 
                      (UCHAR)WaitReason);
    
        /* Check if we were executing an APC */
        if (Status != STATUS_KERNEL_APC) {
           
            /* Return Status */
            return Status;
        }
        
        DPRINT("Looping Again\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    
    } while (TRUE);
    
WaitDone:
    /* Release the Lock, we are done */
    DPRINT("Returning from KeWaitForMultipleObjects(), %x. Status: %d\n", KeGetCurrentThread(), Status);
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return Status;       
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
    PDISPATCHER_HEADER CurrentObject;
    PKWAIT_BLOCK WaitBlock;
    PKWAIT_BLOCK TimerWaitBlock;
    PKTIMER ThreadTimer;
    PKTHREAD CurrentThread = KeGetCurrentThread();
    ULONG AllObjectsSignaled;
    ULONG WaitIndex;
    NTSTATUS Status;
    NTSTATUS WaitStatus;

    DPRINT("Entering KeWaitForMultipleObjects(Count %lu Object[] %p) "
            "PsGetCurrentThread() %x, Timeout %x\n", Count, Object, PsGetCurrentThread(), Timeout);

    /* Set the Current Thread */
    CurrentThread = KeGetCurrentThread();
    
    /* Check if the lock is already held */
    if (CurrentThread->WaitNext)  {
        
        /* Lock is held, disable Wait Next */
        DPRINT("Lock is held\n");
        CurrentThread->WaitNext = FALSE;
        
    } else {
        
        /* Lock not held, acquire it */
        DPRINT("Lock is not held, acquiring\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    }

    /* Make sure the Wait Count is valid for the Thread and Maximum Wait Objects */
    if (!WaitBlockArray) {
       
        /* Check in regards to the Thread Object Limit */
        if (Count > THREAD_WAIT_OBJECTS) {
            
            DPRINT1("(%s:%d) Too many objects!\n", __FILE__, __LINE__);
            KEBUGCHECK(MAXIMUM_WAIT_OBJECTS_EXCEEDED);
        }
        
        /* Use the Thread's Wait Block */
        WaitBlockArray = &CurrentThread->WaitBlock[0];
        
    } else {
      
        /* Using our own Block Array. Check in regards to System Object Limit */
        if (Count > MAXIMUM_WAIT_OBJECTS) {
            
            DPRINT1("(%s:%d) Too many objects!\n", __FILE__, __LINE__);
            KEBUGCHECK(MAXIMUM_WAIT_OBJECTS_EXCEEDED);
        }
    }
    
    /* Start the actual Loop */
    do {
        
        /* Get the current Wait Status */
        WaitStatus = CurrentThread->WaitStatus;
    
        /* Append wait block to the KTHREAD wait block list */
        CurrentThread->WaitBlockList = WaitBlock = WaitBlockArray;
      
        /* Check if the wait is (already) satisfied */
        AllObjectsSignaled = TRUE;
        
        /* First, we'll try to satisfy the wait directly */
        for (WaitIndex = 0; WaitIndex < Count; WaitIndex++) {
            
            /* Get the Current Object */
            CurrentObject = (PDISPATCHER_HEADER)Object[WaitIndex];
            
            /* FIXME: 
             * Temporary hack until my Object Manager re-write. Basically some objects, like
             * the File Object, but also LPCs and others, are actually waitable on their event.
             * The Object Manager sets this up in The ObjectTypeInformation->DefaultObject member,
             * by using pretty much the same kind of hack as us. Normal objects point to themselves
             * in that pointer. Then, NtWaitForXXX will populate the WaitList that gets sent to us by
             * using ->DefaultObject, so the proper actual objects will be sent to us. Until then however,
             * I will keep this hack here, since there's no need to make an interim hack until the rewrite
             * -- Alex Ionescu 24/02/05
             */
            if (CurrentObject->Type == IO_TYPE_FILE) {
                   
                DPRINT1("Hack used: %x\n", &((PFILE_OBJECT)CurrentObject)->Event);
                CurrentObject = (PDISPATCHER_HEADER)(&((PFILE_OBJECT)CurrentObject)->Event);
            }

            /* Check if the Object is Signaled */
            if (KiIsObjectSignaled(CurrentObject, CurrentThread)) {
                
                /* Check what kind of wait this is */ 
                if (WaitType == WaitAny) {
                    
                    /* This is a Wait Any, so just unwait this guy and exit */
                    if (CurrentObject->SignalState != MINLONG) {
                        
                        /* It has a normal signal state, so unwait it and return */
                        KiSatisfyObjectWait(CurrentObject, CurrentThread);
                        Status = STATUS_WAIT_0 | WaitIndex;
                        goto WaitDone;
                        
                    } else {
                        
                        /* Is this a Mutant? */
                        if (CurrentObject->Type == MutantObject) {
                            
                            /* According to wasm.ru, we must raise this exception (tested and true) */
                            KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
                            ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);                    
                        } 
                    }
                }
                
            } else {
                    
                /* One of the objects isn't signaled... if this is a WaitAll, we will fail later */
                AllObjectsSignaled = FALSE;
            }

            /* Set up a Wait Block for this Object */
            WaitBlock->Object = CurrentObject;
            WaitBlock->Thread = CurrentThread;
            WaitBlock->WaitKey = (USHORT)(STATUS_WAIT_0 + WaitIndex);
            WaitBlock->WaitType = (USHORT)WaitType;                       
            WaitBlock->NextWaitBlock = WaitBlock + 1;
                
            /* Move to the next Wait Block */
            WaitBlock = WaitBlock->NextWaitBlock;
        }
       
        /* Return to the Root Wait Block */
        WaitBlock--;
        WaitBlock->NextWaitBlock = NULL;
            
        /* Check if this is a Wait All and all the objects are signaled */
        if ((WaitType == WaitAll) && (AllObjectsSignaled)) {
                                
            /* Return to the Root Wait Block */
            WaitBlock = CurrentThread->WaitBlockList;
            
            /* Satisfy their Waits and return to the caller */
            KiSatisifyMultipleObjectWaits(WaitBlock);
            Status = STATUS_WAIT_0;
            goto WaitDone;
        }
   
        /* Make sure we can satisfy the Alertable request */
        KiCheckAlertability(Alertable, CurrentThread, WaitMode, &Status);
    
        /* Set the Wait Status */
        CurrentThread->WaitStatus = Status;
           
        /* Enable the Timeout Timer if there was any specified */
        if (Timeout != NULL) {
              
            /* However if 0 timeout was specified, then we must fail since we need to peform a wait */
            if (!Timeout->QuadPart) {
                
                /* Return a timeout */
                Status = STATUS_TIMEOUT;
                goto WaitDone;
            }
            
            /* Point to Timer Wait Block and Thread Timer */
            TimerWaitBlock = &CurrentThread->WaitBlock[TIMER_WAIT_BLOCK];
            ThreadTimer = &CurrentThread->Timer;

            /* Connect the Timer Wait Block */
            WaitBlock->NextWaitBlock = TimerWaitBlock;
            
            /* Set up the Timer Wait Block */
            TimerWaitBlock->Object = (PVOID)ThreadTimer;
            TimerWaitBlock->Thread = CurrentThread;
            TimerWaitBlock->WaitKey = STATUS_TIMEOUT;
            TimerWaitBlock->WaitType = WaitAny;
            TimerWaitBlock->NextWaitBlock = NULL;
            
            /* Link the timer to this Wait Block */
            InitializeListHead(&ThreadTimer->Header.WaitListHead);

            /* Insert the Timer into the Timer Lists and enable it */
            if (!KiInsertTimer(ThreadTimer, *Timeout)) {    

                /* Return a timeout if we couldn't insert the timer for some reason */
                Status = STATUS_TIMEOUT;
                goto WaitDone;
            }
        }

        /* Insert into Object's Wait List*/
        WaitBlock = CurrentThread->WaitBlockList;
        while (WaitBlock) {
            
            /* Get the Current Object */
            CurrentObject = WaitBlock->Object;
    
            /* Link the Object to this Wait Block */
            InsertTailList(&CurrentObject->WaitListHead, &WaitBlock->WaitListEntry);
            
            /* Move to the next Wait Block */
            WaitBlock = WaitBlock->NextWaitBlock;
        }
        
        /* Handle Kernel Queues */
        if (CurrentThread->Queue) {
                 
            DPRINT("Waking Queue\n");
            KiWakeQueue(CurrentThread->Queue);
        }

        /* Block the Thread */
        DPRINT("Blocking the Thread: %d, %d, %d, %x\n", Alertable, WaitMode, WaitReason, KeGetCurrentThread());
        PsBlockThread(&Status, 
                      Alertable, 
                      WaitMode,
                      (UCHAR)WaitReason);
    
        /* Check if we were executing an APC */
        if (Status != STATUS_KERNEL_APC) {
           
            /* Return Status */
            return Status;
        }
        
        DPRINT("Looping Again\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    
    } while (TRUE);
    
WaitDone:
    /* Release the Lock, we are done */
    DPRINT("Returning from KeWaitForMultipleObjects(), %x. Status: %d\n", KeGetCurrentThread(), Status);
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return Status;
}

VOID
FASTCALL
KiSatisfyObjectWait(PDISPATCHER_HEADER Object,
                    PKTHREAD Thread)

{
    /* Special case for Mutants */
    if (Object->Type == MutantObject) {
     
        /* Decrease the Signal State */
        Object->SignalState--;
        
        /* Check if it's now non-signaled */
        if (Object->SignalState == 0) {
        
            /* Set the Owner Thread */
            ((PKMUTANT)Object)->OwnerThread = Thread;
            
            /* Disable APCs if needed */
            Thread->KernelApcDisable -= ((PKMUTANT)Object)->ApcDisable;
            
            /* Check if it's abandoned */
            if (((PKMUTANT)Object)->Abandoned) {
            
                /* Unabandon it */
                ((PKMUTANT)Object)->Abandoned = FALSE;
                
                /* Return Status */
                Thread->WaitStatus = STATUS_ABANDONED;
            }
            
            /* Insert it into the Mutant List */
            InsertHeadList(&Thread->MutantListHead, &((PKMUTANT)Object)->MutantListEntry);
        }
    
    } else if ((Object->Type & TIMER_OR_EVENT_TYPE) == EventSynchronizationObject) {
    
        /* These guys (Syncronization Timers and Events) just get un-signaled */
        Object->SignalState = 0;
        
    } else if (Object->Type == SemaphoreObject) {

        /* These ones can have multiple signalings, so we only decrease it */
        Object->SignalState--;
    } 
}

VOID
FASTCALL
KiWaitTest(PDISPATCHER_HEADER Object,
           KPRIORITY Increment)
{
    PLIST_ENTRY WaitEntry;
    PLIST_ENTRY WaitList;
    PKWAIT_BLOCK CurrentWaitBlock;
    PKWAIT_BLOCK NextWaitBlock;
    
    /* Loop the Wait Entries */
    DPRINT("KiWaitTest for Object: %x\n", Object);
    WaitList = &Object->WaitListHead;
    WaitEntry = WaitList->Flink;
    while ((WaitEntry != WaitList) && (Object->SignalState > 0)) {
        
        /* Get the current wait block */
        CurrentWaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
        
        /* Check the current Wait Mode */
        if (CurrentWaitBlock->WaitType == WaitAny) {
        
            /* Easy case, satisfy only this wait */
            DPRINT("Satisfiying a Wait any\n");
            WaitEntry = WaitEntry->Blink;
            KiSatisfyObjectWait(Object, CurrentWaitBlock->Thread);
        
        } else {
        
            /* Everything must be satisfied */
            DPRINT("Checking for a Wait All\n");
            NextWaitBlock = CurrentWaitBlock->NextWaitBlock;
            
            /* Loop first to make sure they are valid */
            while (NextWaitBlock) {
            
                /* Check if the object is signaled */
                if (!KiIsObjectSignaled(Object, CurrentWaitBlock->Thread)) {
                
                    /* It's not, move to the next one */
                    DPRINT1("One of the object is non-signaled, sorry.\n");
                    goto SkipUnwait;
                }
                
                /* Go to the next Wait block */
                NextWaitBlock = NextWaitBlock->NextWaitBlock;
            }
                       
            /* All the objects are signaled, we can satisfy */
            DPRINT("Satisfiying a Wait All\n");
            WaitEntry = WaitEntry->Blink;
            KiSatisifyMultipleObjectWaits(CurrentWaitBlock);
        }
        
        /* All waits satisfied, unwait the thread */
        DPRINT("Unwaiting the Thread\n");
        KiAbortWaitThread(CurrentWaitBlock->Thread, CurrentWaitBlock->WaitKey, Increment);

SkipUnwait:
        /* Next entry */
        WaitEntry = WaitEntry->Flink;
    }
    
    DPRINT("Done\n");
}               

/* Must be called with the dispatcher lock held */
VOID
FASTCALL 
KiAbortWaitThread(PKTHREAD Thread, 
                  NTSTATUS WaitStatus,
                  KPRIORITY Increment)
{
    PKWAIT_BLOCK WaitBlock;

    /* If we are blocked, we must be waiting on something also */
    DPRINT("KiAbortWaitThread: %x, Status: %x, %x \n", Thread, WaitStatus, Thread->WaitBlockList);
    ASSERT((Thread->State == THREAD_STATE_BLOCKED) == (Thread->WaitBlockList != NULL));

    /* Remove the Wait Blocks from the list */
    DPRINT("Removing waits\n");
    WaitBlock = Thread->WaitBlockList;
    while (WaitBlock) {
        
        /* Remove it */
        DPRINT("Removing Waitblock: %x, %x\n", WaitBlock, WaitBlock->NextWaitBlock);
        RemoveEntryList(&WaitBlock->WaitListEntry);
        
        /* Go to the next one */
        WaitBlock = WaitBlock->NextWaitBlock;
    };
    
    /* Check if there's a Thread Timer */
    if (Thread->Timer.Header.Inserted) {
    
        /* Cancel the Thread Timer with the no-lock fastpath */
        DPRINT("Removing the Thread's Timer\n");
        Thread->Timer.Header.Inserted = FALSE;
        RemoveEntryList(&Thread->Timer.TimerListEntry);
    }
    
    /* Increment the Queue's active threads */
    if (Thread->Queue) {
    
        DPRINT("Incrementing Queue's active threads\n");
        Thread->Queue->CurrentCount++;
    }

    /* Reschedule the Thread */
    DPRINT("Unblocking the Thread\n");
    PsUnblockThread((PETHREAD)Thread, &WaitStatus, 0);
}

BOOLEAN
inline
FASTCALL
KiIsObjectSignaled(PDISPATCHER_HEADER Object,
                   PKTHREAD Thread)
{
    /* Mutants are...well...mutants! */
   if (Object->Type == MutantObject) {

        /* 
         * Because Cutler hates mutants, they are actually signaled if the Signal State is <= 0
         * Well, only if they are recursivly acquired (i.e if we own it right now).
         * Of course, they are also signaled if their signal state is 1.
         */
        if ((Object->SignalState <= 0 && ((PKMUTANT)Object)->OwnerThread == Thread) || 
            (Object->SignalState == 1)) {
            
            /* Signaled Mutant */
            return (TRUE);
            
        } else {
            
            /* Unsignaled Mutant */
            return (FALSE);
        }
    }
    
    /* Any other object is not a mutated freak, so let's use logic */
   return (!Object->SignalState <= 0);
}

BOOL
inline
FASTCALL
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
        Header->ObjectType == IoFileObjectType) {
        
        return TRUE;
    
    } else {
        
        return FALSE;
    }
}

VOID
inline
FASTCALL
KiSatisifyMultipleObjectWaits(PKWAIT_BLOCK WaitBlock)
{
    PKTHREAD WaitThread = WaitBlock->Thread;
    
    /* Loop through all the Wait Blocks, and wake each Object */
    while (WaitBlock) {
         
        /* Wake the Object */
        KiSatisfyObjectWait(WaitBlock->Object, WaitThread);
        WaitBlock = WaitBlock->NextWaitBlock;
    }      
}

VOID
inline
FASTCALL
KeInitializeDispatcherHeader(DISPATCHER_HEADER* Header,
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
inline
FASTCALL
KeAcquireDispatcherDatabaseLock(VOID)
{
    KIRQL OldIrql;

    KeAcquireSpinLock (&DispatcherDatabaseLock, &OldIrql);
    return OldIrql;
}

VOID
inline
FASTCALL
KeAcquireDispatcherDatabaseLockAtDpcLevel(VOID)
{
    KeAcquireSpinLockAtDpcLevel (&DispatcherDatabaseLock);
}

VOID 
inline
FASTCALL
KeInitializeDispatcher(VOID)
{
    /* Initialize the Dispatcher Lock */
    KeInitializeSpinLock(&DispatcherDatabaseLock);
}

VOID
inline
FASTCALL
KeReleaseDispatcherDatabaseLock(KIRQL OldIrql)
{
    /* If it's the idle thread, dispatch */
    if (!KeIsExecutingDpc() && OldIrql < DISPATCH_LEVEL && KeGetCurrentThread() != NULL && 
        KeGetCurrentThread() == KeGetCurrentPrcb()->IdleThread) {
        
        PsDispatchThreadNoLock(THREAD_STATE_READY);
        KeLowerIrql(OldIrql);
        
    } else {
        
        /* Just release the spin lock */
        KeReleaseSpinLock(&DispatcherDatabaseLock, OldIrql);
    }
}

VOID
inline
FASTCALL
KeReleaseDispatcherDatabaseLockFromDpcLevel(VOID)
{
    KeReleaseSpinLockFromDpcLevel(&DispatcherDatabaseLock);
}

/* EOF */
