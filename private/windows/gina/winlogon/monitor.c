/****************************** Module Header ******************************\
* Module Name: monitor.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implements functions that handle waiting for an object to be signalled
* asynchronously.
*
* History:
* 01-11-93 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

//
// Define this to enable verbose output for this module
//

// #define DEBUG_MONITOR

#ifdef DEBUG_MONITOR
#define VerbosePrint(s) WLPrint(s)
#else
#define VerbosePrint(s)
#endif

#define LockMonitor(Monitor)    RtlEnterCriticalSection( &Monitor->CritSec )
#define UnlockMonitor(Monitor)  RtlLeaveCriticalSection( &Monitor->CritSec )


//+---------------------------------------------------------------------------
//
//  Function:   RefMonitor
//
//  Synopsis:   Safe Ref Count
//
//  Arguments:  [Monitor] --
//
//  History:    7-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
RefMonitor(
    POBJECT_MONITOR Monitor)
{
    LockMonitor( Monitor );

    Monitor->RefCount ++ ;

    UnlockMonitor( Monitor );

}


//+---------------------------------------------------------------------------
//
//  Function:   DerefMonitor
//
//  Synopsis:   Deref the monitor, cleaning up if refcount goes to zero
//
//  Arguments:  [Monitor] --
//
//  History:    7-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
DerefMonitor(
    POBJECT_MONITOR Monitor )
{
    LockMonitor( Monitor );

    DebugLog(( DEB_TRACE, "Deref of Monitor at %x\n", Monitor));

    Monitor->RefCount -- ;

    if ( Monitor->RefCount == 0 )
    {
        //
        // Clean up time:
        //

        DebugLog(( DEB_TRACE, "Cleaning up Monitor at %x\n", Monitor ));

        if ( Monitor->Thread )
        {
            CloseHandle( Monitor->Thread );
        }

        if ( Monitor->Flags & MONITOR_CLOSEOBJ )
        {
            if ( Monitor->Object != INVALID_HANDLE_VALUE )
            {
                CloseHandle( Monitor->Object );
            }
        }

        UnlockMonitor( Monitor );

        RtlDeleteCriticalSection( &Monitor->CritSec );

        Free( Monitor );

    }
    else
    {
        UnlockMonitor( Monitor );
    }

}

/***************************************************************************\
* FUNCTION: MonitorThread
*
* PURPOSE:  Entry point for object monitor thread
*
* RETURNS:  Windows error value
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

DWORD MonitorThread(
    LPVOID lpThreadParameter
    )
{
    POBJECT_MONITOR Monitor = (POBJECT_MONITOR)lpThreadParameter;
    DWORD WaitResult;
    NTSTATUS Status;
    HANDLE Handle;

    //
    // Wait forever for object to be signalled
    //

    LockMonitor( Monitor );

    Monitor->Flags |= MONITOR_ACTIVE ;

    Handle = Monitor->Object ;

    UnlockMonitor( Monitor );

    if ( Handle != INVALID_HANDLE_VALUE )
    {

        Status = NtWaitForSingleObject( Handle, TRUE, NULL);

        if (!NT_SUCCESS(Status) && (Status != STATUS_ALERTED))
        {
            DebugLog((DEB_ERROR, "MonitorThread:  NtWaitForSingleObject returned %x\n", Status));
        }
    }
    else
    {
        Status = STATUS_SUCCESS ;
    }

    //
    // Notify the appropriate window
    //
    if (Status != STATUS_ALERTED)
    {
        PostMessage( Monitor->hwndNotify,
                     WM_OBJECT_NOTIFY,
                     (WPARAM)Monitor,
                     (LPARAM)Monitor->CallerContext
                    );
    }

    LockMonitor( Monitor );

    if ( Monitor->Flags & MONITOR_CLOSEOBJ )
    {
        CloseHandle( Handle );
    }

    Monitor->Object = INVALID_HANDLE_VALUE ;

    Monitor->Flags = 0 ;

    UnlockMonitor( Monitor );

    DerefMonitor( Monitor );

    return(ERROR_SUCCESS);
}


/***************************************************************************\
* FUNCTION: CreateObjectMonitor
*
* PURPOSE:  Creates a monitor object that will wait on the specified object
*           and post a message to the specifed window when the object is
*           signalled.
*
* NOTES:    The object must have been opened for SYNCHRONIZE access.
*           The caller is responsible for closing the object handle
*           after the monitor object has been deleted.
*
* RETURNS:  Handle to the monitor instance or NULL on failure.
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

POBJECT_MONITOR
CreateObjectMonitor(
    HANDLE Object,
    HWND hwndNotify,
    DWORD CallerContext
    )
{
    POBJECT_MONITOR Monitor;
    DWORD ThreadId;
    NTSTATUS Status;

    //
    // Create monitor object
    //

    Monitor = Alloc(sizeof(OBJECT_MONITOR));
    if (Monitor == NULL) {
        return(NULL);
    }

    //
    // Initialize monitor fields
    //

    Status = RtlInitializeCriticalSection( &Monitor->CritSec );
    if ( !NT_SUCCESS( Status ) )
    {
        Free( Monitor );
        return( NULL );
    }

    LockMonitor( Monitor );

    Monitor->hwndNotify = hwndNotify;
    Monitor->Object = Object;
    Monitor->CallerContext = CallerContext;
    Monitor->RefCount = 1;
    Monitor->Flags = 0 ;

    //
    // Create the monitor thread
    //

    Monitor->Thread = CreateThread(
                        NULL,                       // Use default ACL
                        0,                          // Same stack size
                        MonitorThread,              // Start address
                        (LPVOID)Monitor,            // Parameter
                        CREATE_SUSPENDED,           // Creation flags
                        &ThreadId                   // Get the id back here
                        );

    if (Monitor->Thread == NULL) {

        DebugLog((DEB_ERROR, "Failed to create monitor thread, error = %d\n", GetLastError()));

        DerefMonitor( Monitor );

        return(NULL);
    }

    //
    // Thread was created, so add one for the new thread
    //

    RefMonitor( Monitor );

    //
    // Unlock the monitor struct.
    //

    UnlockMonitor( Monitor );

    //
    // Let the thread go.  Note, since we have a ref, and the thread has a ref, this won't
    // disappear.
    //

    ResumeThread( Monitor->Thread );


    return(Monitor);
}


/***************************************************************************\
* FUNCTION: DeleteObjectMonitor
*
* PURPOSE:  Deletes an instance of a monitor object
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

VOID
DeleteObjectMonitor(
    POBJECT_MONITOR Monitor,
    BOOLEAN fTerminate
    )
{
    BOOL Result;

    if ( fTerminate )
    {
        CancelObjectMonitor( Monitor );
    }

    DerefMonitor( Monitor );

}

//+---------------------------------------------------------------------------
//
//  Function:   CancelObjectMonitor
//
//  Synopsis:   Interrupts a monitor thread safely.
//
//  Arguments:  [Monitor] --
//
//  History:    7-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
CancelObjectMonitor(
    POBJECT_MONITOR Monitor )
{
    LockMonitor( Monitor );

    if ( Monitor->Flags & MONITOR_ACTIVE )
    {
        NtAlertThread( Monitor->Thread );
    }
    else
    {
        Monitor->Object = INVALID_HANDLE_VALUE ;
    }

    UnlockMonitor( Monitor );

}

//+---------------------------------------------------------------------------
//
//  Function:   CloseObjectMonitorObject
//
//  Synopsis:   Tags the handle to be closed when the monitor goes away
//
//  Arguments:  [Monitor] --
//
//  History:    7-19-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
CloseObjectMonitorObject(
    POBJECT_MONITOR Monitor
    )
{
    LockMonitor( Monitor );

    Monitor->Flags |= MONITOR_CLOSEOBJ ;

    UnlockMonitor( Monitor );
}
