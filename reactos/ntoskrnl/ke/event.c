/*
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/event.c
 * PURPOSE:         Implements events
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
VOID 
STDCALL 
KeClearEvent(PKEVENT Event)
{
    DPRINT("KeClearEvent(Event %x)\n", Event);
    
    /* Reset Signal State */
    Event->Header.SignalState = FALSE;
}

/*
 * @implemented
 */
VOID 
STDCALL 
KeInitializeEvent(PKEVENT Event,
                  EVENT_TYPE Type,
                  BOOLEAN State)
{
    DPRINT("KeInitializeEvent(Event %x)\n", Event);
    
    /* Initialize the Dispatcher Header */
    KeInitializeDispatcherHeader(&Event->Header,
                                 Type,
                                 sizeof(Event) / sizeof(ULONG),
                                 State);
}

/*
 * @implemented
 */
VOID 
STDCALL 
KeInitializeEventPair(PKEVENT_PAIR EventPair)
{
    DPRINT("KeInitializeEventPair(Event %x)\n", EventPair);
    
    /* Initialize the Event Pair Type and Size */
    EventPair->Type = EventPairObject;
    EventPair->Size = sizeof(KEVENT_PAIR);
    
    /* Initialize the two Events */
    KeInitializeEvent(&EventPair->LowEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent(&EventPair->HighEvent, SynchronizationEvent, FALSE);
}

/*
 * @implemented
 */
LONG STDCALL
KePulseEvent(IN PKEVENT Event,
             IN KPRIORITY Increment,
             IN BOOLEAN Wait)
{
    KIRQL OldIrql;
    LONG PreviousState;

    DPRINT("KePulseEvent(Event %x, Wait %x)\n",Event,Wait);
    
    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();
    
    /* Save the Old State */
    PreviousState = Event->Header.SignalState;
    
    /* Check if we are non-signaled and we have stuff in the Wait Queue */
    if (!PreviousState && !IsListEmpty(&Event->Header.WaitListHead)) {
        
        /* Set the Event to Signaled */
        Event->Header.SignalState = 1;
    
        /* Wake the Event */
        KiWaitTest(&Event->Header, Increment);
    }
    
    /* Unsignal it */
    Event->Header.SignalState = 0;
    
    /* Check what wait state was requested */
    if (Wait == FALSE) {
    
        /* Wait not requested, release Dispatcher Database and return */    
        KeReleaseDispatcherDatabaseLock(OldIrql);
        
    } else {
        
        /* Return Locked and with a Wait */
        KTHREAD *Thread = KeGetCurrentThread();
        Thread->WaitNext = TRUE;
        Thread->WaitIrql = OldIrql;
    }

    /* Return the previous State */
    return PreviousState;
}

/*
 * @implemented
 */
LONG 
STDCALL 
KeReadStateEvent(PKEVENT Event)
{
    /* Return the Signal State */
    return Event->Header.SignalState;
}

/*
 * @implemented
 */
LONG 
STDCALL 
KeResetEvent(PKEVENT Event)
{
    KIRQL OldIrql;
    LONG PreviousState;
    
    DPRINT("KeResetEvent(Event %x)\n",Event);
    
    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();
    
    /* Save the Previous State */
    PreviousState = Event->Header.SignalState;
    
    /* Set it to zero */
    Event->Header.SignalState = 0;
    
    /* Release Dispatcher Database and return previous state */    
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return PreviousState;
}

/*
 * @implemented
 */
LONG
STDCALL 
KeSetEvent(PKEVENT Event,
           KPRIORITY Increment,
           BOOLEAN Wait)
{
    KIRQL OldIrql;
    LONG PreviousState;
    PKWAIT_BLOCK WaitBlock;

    DPRINT("KeSetEvent(Event %x, Wait %x)\n",Event,Wait);

    /* Lock the Dispathcer Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();
    
    /* Save the Previous State */
    PreviousState = Event->Header.SignalState;
    
    /* Check if we have stuff in the Wait Queue */
    if (IsListEmpty(&Event->Header.WaitListHead)) {
        
        /* Set the Event to Signaled */
        DPRINT("Empty Wait Queue, Signal the Event\n");
        Event->Header.SignalState = 1;
        
    } else {
    
        /* Get the Wait Block */
        WaitBlock = CONTAINING_RECORD(Event->Header.WaitListHead.Flink,
                                      KWAIT_BLOCK,
                                      WaitListEntry);
        
        
        /* Check the type of event */
        if (Event->Header.Type == NotificationEvent || WaitBlock->WaitType == WaitAll) {
            
            if (PreviousState == 0) {
                
                /* We must do a full wait satisfaction */
                DPRINT("Notification Event or WaitAll, Wait on the Event and Signal\n");
                Event->Header.SignalState = 1;
                KiWaitTest(&Event->Header, Increment);
            }
                
        } else {
        
            /* We can satisfy wait simply by waking the thread, since our signal state is 0 now */        
            DPRINT("WaitAny or Sync Event, just unwait the thread\n");
            KiAbortWaitThread(WaitBlock->Thread, WaitBlock->WaitKey, Increment);
        }
    }
    
    /* Check what wait state was requested */
    if (Wait == FALSE) {
    
        /* Wait not requested, release Dispatcher Database and return */    
        KeReleaseDispatcherDatabaseLock(OldIrql);
        
    } else {
        
        /* Return Locked and with a Wait */
        KTHREAD *Thread = KeGetCurrentThread();
        Thread->WaitNext = TRUE;
        Thread->WaitIrql = OldIrql;
    }

    /* Return the previous State */
    DPRINT("Done: %d\n", PreviousState);
    return PreviousState;
}

/*
 * @implemented
 */
VOID
STDCALL
KeSetEventBoostPriority(IN PKEVENT Event,
                        IN PKTHREAD *Thread OPTIONAL)
{
    PKTHREAD WaitingThread;
    KIRQL OldIrql;

    DPRINT("KeSetEventBoostPriority(Event %x, Thread %x)\n",Event,Thread);
    
    /* Acquire Dispatcher Database Lock */
    OldIrql = KeAcquireDispatcherDatabaseLock();
    
    /* If our wait list is empty, then signal the event and return */
    if (IsListEmpty(&Event->Header.WaitListHead)) {
    
        Event->Header.SignalState = 1;
    
    } else {
        
        /* Get Thread that is currently waiting. First get the Wait Block, then the Thread */
        WaitingThread = CONTAINING_RECORD(Event->Header.WaitListHead.Flink, 
                                          KWAIT_BLOCK, 
                                          WaitListEntry)->Thread;

        /* Return it to caller if requested */
        if ARGUMENT_PRESENT(Thread) *Thread = WaitingThread;

        /* Reset the Quantum and Unwait the Thread */
        WaitingThread->Quantum = WaitingThread->ApcState.Process->ThreadQuantum;
        KiAbortWaitThread(WaitingThread, STATUS_SUCCESS, EVENT_INCREMENT);
    }
   
    /* Release the Dispatcher Database Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);
}

/* EOF */
