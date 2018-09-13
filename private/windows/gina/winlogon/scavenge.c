//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       scavenge.c
//
//  Contents:   Home of the scavenger thread
//
//  Classes:
//
//  Functions:
//
//  History:    7-28-97   RichardW   Stolen from lsa
//
//----------------------------------------------------------------------------

#include "winlogon.h"

HANDLE                  hReadScavConfig = NULL;     // Signals to restart wait
HANDLE                  hStateChangeEvent = NULL ;  // Big change
HANDLE                  ScavWaitHandles[MAXIMUM_WAIT_OBJECTS];
ULONG                   cScavWaitHandles;

LIST_ENTRY              TimerList ;
LIST_ENTRY              WaitList[ MAXIMUM_WAIT_OBJECTS ];

RTL_CRITICAL_SECTION    ScavLock ;

typedef enum _WL_SCAV_INSERT {
    ScavInsertHead,
    ScavInsertTail,
    ScavInsertSorted
} WL_SCAV_INSERT ;

#define SCAVENGER_WAIT_INTERVAL 60000L

//
// Internal flags:
//

#define SCAVFLAG_IN_PROGRESS    0x40000000  // Active
#define SCAVFLAG_ABOUT_TO_DIE   0x20000000  // About to be removed
#define SCAVFLAG_IMMEDIATE      0x08000000  // Immediate Execute
#define SCAVFLAG_STATE_CHANGE   0x04000000  // State Change
#define SCAVFLAG_TRIGGER_FREE   0x02000000  // Trigger will free
#define SCAVFLAG_NOTIFY_EVENT   0x01000000

#define SCAVFLAG_EXECUTE_INLINE 0x00800000  // Execute stub directly

#define NOTIFYFLAG_CHILD_SYNC   0x80000000  // All sub funcs are synchronous
#define NOTIFYFLAG_BLOCK_CALLS  0x40000000  // Block calls

#define NOTIFY_FLAG_SYNCHRONOUS 0x00000001

#if DBG
#define SCAVFLAG_ITEM_BREAK     0x10000000
#endif

//
// Define indices for well known events.  Shutdown is triggered when
// the console handler starts a shutdown.  config is when someone adds
// another notifier.  state is when the state of the machine has changed.
//

#define SCAVENGER_SHUTDOWN_EVENT    0
#define SCAVENGER_CONFIG_EVENT      1
#define SCAVENGER_NOTIFY_EVENT      2

#define LockScavenger() RtlEnterCriticalSection( &ScavLock )
#define UnlockScavenger() RtlLeaveCriticalSection( &ScavLock )

#define LockNotify()    RtlEnterCriticalSection( &NotifyLock )
#define UnlockNotify()  RtlLeaveCriticalSection( &NotifyLock )


//
// Define locking macros for the scav list
//

#define WlpRefScavItem( Item )     \
        {                           \
            RtlEnterCriticalSection( &ScavLock ); \
            ((PWL_SCAVENGER_ITEM) Item)->RefCount++ ; \
            RtlLeaveCriticalSection( &ScavLock ); \
        }

#define WlpRefScavItemUnsafe( Item )    \
        {                               \
            ((PWL_SCAVENGER_ITEM) Item)->RefCount++ ; \
        }




DWORD
WINAPI
WlpScavengerThread(
    PVOID   Ignored
    );


//+---------------------------------------------------------------------------
//
//  Function:   WlpDerefScavItem
//
//  Synopsis:   Dereference, optionally freeing a scavenger item
//
//  Arguments:  [Item] --
//
//  History:    6-03-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WlpDerefScavItem(
    PWL_SCAVENGER_ITEM Item
    )
{
    LockScavenger();

    Item->RefCount-- ;

    if ( Item->RefCount == 0 )
    {
        if ( Item->List.Flink )
        {
            RemoveEntryList( &Item->List );

            Item->List.Flink = NULL ;
        }

        //
        // Remove the handle from the wait list:
        //

        if ( Item->Type == NOTIFIER_TYPE_HANDLE_WAIT )
        {
            if ( IsListEmpty( &WaitList[ Item->HandleIndex ] ) )
            {
                //
                // Big swap action now:
                //

                cScavWaitHandles --;

                if ( Item->HandleIndex == cScavWaitHandles )
                {
                    //
                    // Easy case:  lower the count
                    //

                    NOTHING ;

                }
                else
                {
                    WaitList[ Item->HandleIndex ] = WaitList[ cScavWaitHandles ];
                    ScavWaitHandles[ Item->HandleIndex ] = ScavWaitHandles[ cScavWaitHandles ] ;
                }

                SetEvent( hReadScavConfig );
            }
        }

        LocalFree( Item );

    }

    UnlockScavenger();

}

//+---------------------------------------------------------------------------
//
//  Function:   WlpScavengerTrigger
//
//  Synopsis:   Actual Trigger
//
//  Arguments:  [Parameter] -- Item to call
//
//  History:    5-24-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
ULONG
WlpScavengerTrigger(
    PVOID   Parameter
    )
{
    PWL_SCAVENGER_ITEM Item ;

    Item = (PWL_SCAVENGER_ITEM) Parameter ;

    __try
    {
        (VOID) Item->Function( Item->Parameter );

    }
    __except( 1 )
    {
        NOTHING ;
    }

    WlpDerefScavItem( Item );

    return 0 ;
}


//+---------------------------------------------------------------------------
//
//  Function:   WlpTriggerScavengerItem
//
//  Synopsis:   Executes, either in line, or on another thread, the
//              Item passed in.
//
//  Arguments:  [Item] -- Item to execute
//
//  History:    5-24-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WlpTriggerScavengerItem(
    PWL_SCAVENGER_ITEM    Item,
    BOOL Ref
    )
{
    HANDLE Thread ;
    DWORD tid ;

    if ( Ref )
    {
        WlpRefScavItem( Item );
    }

    if ( Item->Flags & NOTIFIER_FLAG_NEW_THREAD )
    {
        Thread = CreateThread( 0, 0,
                      WlpScavengerTrigger, Item,
                      0, &tid );

        if ( Thread )
        {
            CloseHandle( Thread );
        }
    }
    else
    {
        WlpScavengerTrigger( Item );

    }

}


DWORD
WINAPI
WlpScavengerHandleChange(
    PVOID Ignored
    )
{
    return 0 ;
}


//+---------------------------------------------------------------------------
//
//  Function:   WlpInsertScavengerItem
//
//  Synopsis:   Insert a scavenger item into the designated list,
//              by one of the methods described.
//
//  Arguments:  [List]       -- Insert List
//              [Item]       -- Item
//              [InsertType] -- Method
//
//  History:    5-23-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WlpInsertScavengerItem(
    PLIST_ENTRY List,
    PWL_SCAVENGER_ITEM Item,
    WL_SCAV_INSERT InsertType
    )
{
    PLIST_ENTRY Scan ;
    PWL_SCAVENGER_ITEM Compare ;

    if (Item->Type == NOTIFIER_TYPE_INTERVAL)
    {
        Item->NextTrigger = GetCurrentTime() + (Item->Interval * 1000);
    }
    else
    {
        Item->NextTrigger = INFINITE;
    }

    LockScavenger();

    switch ( InsertType )
    {
        case ScavInsertHead:
            InsertHeadList( List, &Item->List );
            break;

        case ScavInsertTail:
            InsertTailList( List, &Item->List );
            break;

        case ScavInsertSorted:

            Scan = List->Flink ;

            while ( Scan != List )
            {
                Compare = CONTAINING_RECORD( Scan, WL_SCAVENGER_ITEM, List );

                if ( Compare->NextTrigger > Item->NextTrigger )
                {
                    break;
                }

                Scan = Scan->Flink ;
            }

            InsertHeadList( Scan, &Item->List );

            break;

    }

    UnlockScavenger();
}

//+---------------------------------------------------------------------------
//
//  Function:   WlRegisterNotification
//
//  Synopsis:   Create a scavenger entry linked to the common scavenger
//              thread.
//
//  Effects:    Will restart the wait
//
//  Arguments:  [pFunction]   --    Function to call
//              [pvParameter] --    Parameter to pass function
//              [Type]        --    Type of scavenger function
//              [fItem]       --    Flags
//              [Interval]    --    Interval at which to call fn, in minutes
//              [hEvent]      --    Handle of event to wait on
//
//  History:    6-03-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PVOID
NTAPI
WlRegisterNotification(
    IN LPTHREAD_START_ROUTINE pFunction,
    IN PVOID pvParameter,
    IN ULONG Type,
    IN ULONG fItem,
    IN ULONG Interval,
    IN HANDLE hEvent)
{
    PWL_SCAVENGER_ITEM Item ;
    PLIST_ENTRY List ;
    WL_SCAV_INSERT InsertPos ;

    Item = (PWL_SCAVENGER_ITEM) LocalAlloc( LMEM_FIXED,
                                    sizeof( WL_SCAVENGER_ITEM ) );

    if ( !Item )
    {
        return NULL ;
    }

    Item->List.Flink = NULL ;

    Item->Type = Type ;
    Item->Function = pFunction ;
    Item->Parameter = pvParameter ;
    Item->RefCount = 1 ;
    Item->ScavCheck = SCAVMAGIC_ACTIVE;

    switch ( Type )
    {
        case NOTIFIER_TYPE_IMMEDIATE:

            Item->Interval = 0 ;

            fItem |= NOTIFIER_FLAG_ONE_SHOT ;

            List = &TimerList ;

            InsertPos = ScavInsertHead ;

            break;

        case NOTIFIER_TYPE_INTERVAL:

            Item->Interval = Interval ;

            List = &TimerList ;

            InsertPos = ScavInsertSorted ;

            break;

        case NOTIFIER_TYPE_HANDLE_WAIT:

            LockScavenger();

            if ( cScavWaitHandles == MAXIMUM_WAIT_OBJECTS )
            {
                LocalFree( Item );

                Item = NULL ;

                break;
            }

            Item->HandleIndex = cScavWaitHandles ;

            List = &WaitList[ cScavWaitHandles ];

            InsertPos = ScavInsertTail ;

            ScavWaitHandles[ cScavWaitHandles ] = hEvent ;

            cScavWaitHandles++ ;

            UnlockScavenger( );

            break;


        default:

            LocalFree( Item );
            Item = NULL ;

            break;

    }

    if ( !Item )
    {
        return NULL ;
    }

    //
    // Okay, we have set up the item, more or less.  Now, insert it
    // into the list we have selected for it.
    //

    Item->Flags = fItem ;

    WlpInsertScavengerItem( List, Item, InsertPos );

    SetEvent( hReadScavConfig );

    return Item ;

}


//+---------------------------------------------------------------------------
//
//  Function:   LsaICancelNotification
//
//  Arguments:  [pvScavHandle] --
//
//  History:    5-26-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
NTAPI
WlCancelNotification(
    PVOID       pvScavHandle
    )
{
    PWL_SCAVENGER_ITEM Item ;

    Item = (PWL_SCAVENGER_ITEM) pvScavHandle ;

    if ( Item->ScavCheck != SCAVMAGIC_ACTIVE )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    WlpDerefScavItem( Item );

    return STATUS_SUCCESS ;

}


//+---------------------------------------------------------------------------
//
//  Function:   WlpInitializeScavenger
//
//  Synopsis:   Initialize Scavenger,
//
//  Arguments:  (none)
//
//  History:    5-26-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WlpInitializeScavenger(
    VOID
    )
{
    ULONG i ;
    PVOID hNotify ;
    HANDLE hThread ;
    DWORD tid ;

    //
    // Initialize the list of timers
    //

    InitializeListHead( &TimerList );

    for (i = 0 ; i < MAXIMUM_WAIT_OBJECTS ; i++ )
    {
        InitializeListHead( &WaitList[ i ] );
    }


    RtlInitializeCriticalSection( &ScavLock );


    //
    // Event set whenever the list of stuff changes
    //

    hReadScavConfig = CreateEvent(NULL, FALSE, FALSE, NULL);


    //
    // Create basic entries
    //

    hNotify = WlRegisterNotification( WlpScavengerHandleChange,
                                        0,
                                        NOTIFIER_TYPE_HANDLE_WAIT,
                                        SCAVFLAG_EXECUTE_INLINE,
                                        0,
                                        hReadScavConfig );


    hThread = CreateThread( NULL, 0,
                                WlpScavengerThread, 0,
                                0, &tid );

    if ( hThread )
    {
        CloseHandle( hThread );

        return TRUE ;
    }

    return FALSE ;

}

//+---------------------------------------------------------------------------
//
//  Function:   WlpScavengerThread
//
//  Synopsis:   Scavenger thread.
//
//  Arguments:  [Ignored] --
//
//  History:    6-03-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
WlpScavengerThread(
    PVOID   Ignored
    )
{
    PWL_SCAVENGER_ITEM CurrentItem ;
    DWORD Timeout ;
    DWORD WaitStatus ;
    PLIST_ENTRY Pop ;


    while ( 1 )
    {
        LockScavenger();

        if ( !IsListEmpty( &TimerList ) )
        {
            CurrentItem = CONTAINING_RECORD( TimerList.Flink, WL_SCAVENGER_ITEM, List );

            if ( CurrentItem->Interval )
            {

                Timeout = CurrentItem->NextTrigger - GetCurrentTime();

                if ( Timeout > CurrentItem->NextTrigger ) {

                    Timeout += 0xFFFFFFFF ;
                }
            }
            else
            {
                Timeout = 0 ;
            }

        }
        else
        {
            Timeout = INFINITE ;
        }

        UnlockScavenger();

        WaitStatus = WaitForMultipleObjectsEx(
                        cScavWaitHandles,       // # of handles
                        ScavWaitHandles,        // handles
                        FALSE,                  // Wait for all?
                        Timeout,
                        FALSE );

        if ( ( WaitStatus >= WAIT_OBJECT_0 ) &&
             ( WaitStatus <= WAIT_OBJECT_0 + MAXIMUM_WAIT_OBJECTS ) )
        {
            LockScavenger();

            Pop = WaitList[ WaitStatus - WAIT_OBJECT_0 ].Flink ;

            while ( Pop != &WaitList[ WaitStatus - WAIT_OBJECT_0 ] )
            {
                CurrentItem = CONTAINING_RECORD( Pop, WL_SCAVENGER_ITEM, List );

                Pop = Pop->Flink ;

                if ( CurrentItem->Flags & NOTIFIER_FLAG_ONE_SHOT )
                {
                    RemoveEntryList( &CurrentItem->List );

                    CurrentItem->List.Flink = NULL ;
                }

                WlpRefScavItemUnsafe( CurrentItem );

                UnlockScavenger();

                WlpTriggerScavengerItem( CurrentItem, FALSE );

                LockScavenger();

                if ( CurrentItem->Flags & NOTIFIER_FLAG_ONE_SHOT )
                {
                    WlpDerefScavItem( CurrentItem );
                }

            }

            UnlockScavenger();

            continue;
        }
        else
        {
            LockScavenger();

            if ( IsListEmpty( &TimerList ) )
            {
                continue;
            }

            Pop = RemoveHeadList( &TimerList );

            Pop->Flink = NULL ;

            CurrentItem = CONTAINING_RECORD( Pop, WL_SCAVENGER_ITEM, List );

            WlpRefScavItemUnsafe( CurrentItem );

            UnlockScavenger();

            WlpTriggerScavengerItem( CurrentItem, FALSE );

            if ( CurrentItem->Flags & NOTIFIER_FLAG_ONE_SHOT )
            {
                WlpDerefScavItem( CurrentItem );
            }
            else
            {
                WlpInsertScavengerItem( &TimerList, CurrentItem, ScavInsertSorted);
            }

        }

    }

    return 0 ;
}

