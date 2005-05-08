/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/timer.c
 * PURPOSE:         Handle Kernel Timers (Kernel-part of Executive Timers)
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net) - Reimplemented some parts, fixed many bugs.
 *                  David Welch (welch@mcmail.com) &  Phillip Susi - Original Implementation.
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ****************************************************************/

LIST_ENTRY KiTimerListHead;

#define SYSTEM_TIME_UNITS_PER_MSEC (10000)

/* FUNCTIONS **************************************************************/

BOOLEAN STDCALL KiInsertTimer(PKTIMER Timer, LARGE_INTEGER DueTime);
VOID STDCALL KiHandleExpiredTimer(PKTIMER Timer);

/*
 * @implemented
 *
 * FUNCTION: Removes a timer from the system timer list
 * ARGUMENTS:
 *       Timer = timer to cancel
 * RETURNS: True if the timer was running
 *          False otherwise
 */
BOOLEAN 
STDCALL
KeCancelTimer(PKTIMER Timer)
{
    KIRQL OldIrql;
    BOOLEAN Inserted = FALSE;
   
    DPRINT("KeCancelTimer(Timer %x)\n",Timer);

    /* Lock the Database and Raise IRQL */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Check if it's inserted, and remove it if it is */
    if ((Inserted = Timer->Header.Inserted)) {
        
        /* Remove from list */
        DPRINT("Timer was inserted, removing\n");
        RemoveEntryList(&Timer->TimerListEntry);
        Timer->Header.Inserted = FALSE;
        
    } else {
    
        DPRINT("Timer was not inserted\n");
    }

    /* Release Dispatcher Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);

    /* Return the old state */
    DPRINT("Old State: %d\n", Inserted);
    return(Inserted);
}

/*
 * @implemented
 *
 * FUNCTION: Initalizes a kernel timer object
 * ARGUMENTS:
 *          Timer = caller supplied storage for the timer
 * NOTE: This function initializes a notification timer
 */
VOID 
STDCALL
KeInitializeTimer (PKTIMER Timer)

{
    /* Call the New Function */
    KeInitializeTimerEx(Timer, NotificationTimer);
}

/*
 * @implemented
 *
 * FUNCTION: Initializes a kernel timer object
 * ARGUMENTS:
 *          Timer = caller supplied storage for the timer
 *          Type = the type of timer (notification or synchronization)
 * NOTE: When a notification type expires all waiting threads are released
 * and the timer remains signalled until it is explicitly reset. When a 
 * syncrhonization timer expires its state is set to signalled until a
 * single waiting thread is released and then the timer is reset.
 */
VOID 
STDCALL
KeInitializeTimerEx (PKTIMER Timer,
                     TIMER_TYPE Type)
{

    DPRINT("KeInitializeTimerEx(%x, %d)\n", Timer, Type);
    
    /* Initialize the Dispatch Header */
    KeInitializeDispatcherHeader(&Timer->Header,
                                 TimerNotificationObject + Type,
                                 sizeof(KTIMER) / sizeof(ULONG),
                                 FALSE);
   
    /* Initalize the Other data */
    Timer->DueTime.QuadPart = 0;
    Timer->Period = 0;
}


/*
 * @implemented
 */
BOOLEAN 
STDCALL
KeReadStateTimer (PKTIMER Timer)
{
    /* Return the Signal State */
    return (BOOLEAN)Timer->Header.SignalState;
}

/*
 * @implemented
 *
 * FUNCTION: Sets the absolute or relative interval at which a timer object
 * is to be set to the signaled state and optionally supplies a 
 * CustomTimerDpc to be executed when the timer expires.
 * ARGUMENTS:
 *          Timer = Points to a previously initialized timer object
 *          DueTimer = If positive then absolute time to expire at
 *                     If negative then the relative time to expire at
 *          Dpc = If non-NULL then a dpc to be called when the timer expires
 * RETURNS: True if the timer was already in the system timer queue
 *          False otherwise
 */
BOOLEAN 
STDCALL
KeSetTimer (PKTIMER Timer,
            LARGE_INTEGER DueTime,
            PKDPC Dpc)
{
    /* Call the newer function and supply a period of 0 */
    return KeSetTimerEx(Timer, DueTime, 0, Dpc);
}

/*
 * @implemented
 *
 * FUNCTION: Sets the absolute or relative interval at which a timer object
 * is to be set to the signaled state and optionally supplies a 
 * CustomTimerDpc to be executed when the timer expires.
 * ARGUMENTS:
 *          Timer = Points to a previously initialized timer object
 *          DueTimer = If positive then absolute time to expire at
 *                     If negative then the relative time to expire at
 *          Dpc = If non-NULL then a dpc to be called when the timer expires
 * RETURNS: True if the timer was already in the system timer queue
 *          False otherwise
 */
BOOLEAN 
STDCALL
KeSetTimerEx (PKTIMER Timer,
              LARGE_INTEGER DueTime,
              LONG Period,
              PKDPC Dpc)
{
    KIRQL OldIrql;
    BOOLEAN Inserted;

    DPRINT("KeSetTimerEx(Timer %x, DueTime %I64d, Period %d, Dpc %x)\n", Timer, DueTime.QuadPart, Period, Dpc);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    /* Lock the Database and Raise IRQL */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Check if it's inserted, and remove it if it is */
    if ((Inserted = Timer->Header.Inserted)) {
        
        /* Remove from list */
        DPRINT("Timer was already inserted\n");
        RemoveEntryList(&Timer->TimerListEntry);
        Timer->Header.Inserted = FALSE;        
    }
    
    /* Set Default Timer Data */ 
    Timer->Dpc = Dpc;
    Timer->Period = Period;
    Timer->Header.SignalState = FALSE;
    
    /* Insert it */
    if (!KiInsertTimer(Timer, DueTime)) {

       KiHandleExpiredTimer(Timer);
    };

    /* Release Dispatcher Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);

    /* Return old state */
    DPRINT("Old State: %d\n", Inserted);
    return Inserted;
}

VOID
STDCALL
KiExpireTimers(PKDPC Dpc,
               PVOID DeferredContext,
               PVOID SystemArgument1,
               PVOID SystemArgument2)
{
    PKTIMER Timer;
    ULONGLONG InterruptTime;
    LIST_ENTRY ExpiredTimerList;
    PLIST_ENTRY CurrentEntry = NULL;
    KIRQL OldIrql;

    DPRINT("KiExpireTimers(Dpc: %x)\n", Dpc);
    
    /* Initialize the Expired Timer List */
    InitializeListHead(&ExpiredTimerList);

    /* Lock the Database and Raise IRQL */
    OldIrql = KeAcquireDispatcherDatabaseLock();
    
    /* Query Interrupt Times */ 
    InterruptTime = KeQueryInterruptTime();

    /* Loop through the Timer List and remove Expired Timers. Insert them into the Expired Listhead */
    CurrentEntry = KiTimerListHead.Flink;
    while (CurrentEntry != &KiTimerListHead) {
    
        /* Get the Current Timer */
        Timer = CONTAINING_RECORD(CurrentEntry, KTIMER, TimerListEntry);
        DPRINT("Looping for Timer: %x. Duetime: %I64d. InterruptTime %I64d \n", Timer, Timer->DueTime.QuadPart, InterruptTime);
        
        /* Check if we have to Expire it */
        if (InterruptTime < Timer->DueTime.QuadPart) break;
        
        CurrentEntry = CurrentEntry->Flink;
       
        /* Remove it from the Timer List, add it to the Expired List */
        RemoveEntryList(&Timer->TimerListEntry);
        InsertTailList(&ExpiredTimerList, &Timer->TimerListEntry);
    }
    
    /* Expire the Timers */
    while ((CurrentEntry = RemoveHeadList(&ExpiredTimerList)) != &ExpiredTimerList) {
        
        /* Get the Timer */
        Timer = CONTAINING_RECORD(CurrentEntry, KTIMER, TimerListEntry);
        DPRINT("Expiring Timer: %x\n", Timer);
        
        /* Expire it */
        KiHandleExpiredTimer(Timer);
    }

    DPRINT("Timers expired\n");

    /* Release Dispatcher Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);
}

/*
 * We enter this function at IRQL DISPATCH_LEVEL, and with the
 * Dispatcher Lock held!
 */
VOID 
STDCALL
KiHandleExpiredTimer(PKTIMER Timer)
{

    LARGE_INTEGER DueTime;
    DPRINT("HandleExpiredTime(Timer %x)\n", Timer);
    
    if(Timer->Header.Inserted) {

       /* First of all, remove the Timer */
       Timer->Header.Inserted = FALSE;
       RemoveEntryList(&Timer->TimerListEntry);
    }
    
    /* Set it as Signaled */
    DPRINT("Setting Timer as Signaled\n");
    Timer->Header.SignalState = TRUE;
    KiWaitTest(&Timer->Header, IO_NO_INCREMENT);   

    /* If the Timer is periodic, reinsert the timer with the new due time */
    if (Timer->Period) {
        
        /* Reinsert the Timer */
        DueTime.QuadPart = Timer->Period * -SYSTEM_TIME_UNITS_PER_MSEC;
        if (!KiInsertTimer(Timer, DueTime)) {

           /* FIXME: I will think about how to handle this and fix it ASAP -- Alex */
           DPRINT("CRITICAL UNHANDLED CASE: TIMER ALREADY EXPIRED!!!\n");
        };
    }
    
    /* Check if the Timer has a DPC */
    if (Timer->Dpc) {

        DPRINT("Timer->Dpc %x Timer->Dpc->DeferredRoutine %x\n", Timer->Dpc, Timer->Dpc->DeferredRoutine);
        
        /* Insert the DPC */
        KeInsertQueueDpc(Timer->Dpc,
                         NULL,
                         NULL);
        
        DPRINT("Finished dpc routine\n");
    }
}

/*
 * Note: This function is called with the Dispatcher Lock held.
 */
BOOLEAN
STDCALL
KiInsertTimer(PKTIMER Timer,
              LARGE_INTEGER DueTime)
{
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER DifferenceTime;
    ULONGLONG InterruptTime;
    
    DPRINT("KiInsertTimer(Timer %x DueTime %I64d)\n", Timer, DueTime.QuadPart);
    
    /* Set default data */
    Timer->Header.Inserted = TRUE;
    Timer->Header.Absolute = FALSE;
    if (!Timer->Period) Timer->Header.SignalState = FALSE;
    
    /* Convert to relative time if needed */
    if (DueTime.u.HighPart >= 0) {
        
        /* Get System Time */
        KeQuerySystemTime(&SystemTime);
        
        /* Do the conversion */
        DifferenceTime.QuadPart = SystemTime.QuadPart - DueTime.QuadPart;
        DPRINT("Time Difference is: %I64d\n", DifferenceTime.QuadPart);
        
        /* Make sure it hasn't already expired */
        if (DifferenceTime.u.HighPart >= 0) {
        
            /* Cancel everything */
            DPRINT("Timer already expired: %d\n", DifferenceTime.u.HighPart);
            Timer->Header.SignalState = TRUE;
            Timer->Header.Inserted = FALSE;
            return FALSE;
        }
        
        /* Set the time as Absolute */
        Timer->Header.Absolute = TRUE;
        DueTime = DifferenceTime;
    }
    
    /* Get the Interrupt Time */
    InterruptTime = KeQueryInterruptTime();
    
    /* Set the Final Due Time */
    Timer->DueTime.QuadPart = InterruptTime - DueTime.QuadPart;
    DPRINT("Final Due Time is: %I64d\n", Timer->DueTime.QuadPart);
    
    /* Now insert it into the Timer List */    
    DPRINT("Inserting Timer into list\n");
    InsertAscendingList(&KiTimerListHead, 
                        KTIMER,
                        TimerListEntry, 
                        Timer,
                        DueTime.QuadPart);
    
    return TRUE;
}
/* EOF */
