/*++

Copyright (c) 1998-1998  Microsoft Corporation

Module Name:

    notify.c

Abstract:

    This module contains the implemention of the change notification functions

Author:

    Mac McLain      (MacM)      04-Feb-1998

Environment:

    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:


Revision History:

--*/

// These must be included first:
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>     // IN, LPVOID, etc.
#include <lmcons.h>     // NET_API_FUNCTION, etc.
#include <lmerr.h>      // LM Error codes
#include <ntlsa.h>      // Lsa change notification function prototypes
#include <lmconfig.h>   // Function prototypes
#include <winbase.h>    // CreateEvent...
#include <netlibnt.h>   // NetpNtStatusToApiStatus
//
// We dynamically load secur32.dll, in order to call the lsa policy change notification functions
// Keep it in sync with the definition in ntlsa.h
//
typedef NTSTATUS (* POLCHANGENOTIFYFN )( POLICY_NOTIFICATION_INFORMATION_CLASS, HANDLE );

NET_API_STATUS
NET_API_FUNCTION
NetRegisterDomainNameChangeNotification(
    PHANDLE NotificationEventHandle
    )
/*++

Routine Description:

    This function is used to register a notification for a domain name change.
    The waitable event that is returned gets signalled when ever the flat or
    dns domain name is changed.

Arguments:

    NotificationHandle - Where the handle to the created notification event is
        returned.


Return Value:

    NERR_Success - Success
    ERROR_INVALID_PARAMETER - A NULL NotificationEventHandle was given


--*/
{
    NTSTATUS Status;
    HANDLE EventHandle;
    DWORD Err = NERR_Success;

    if ( NotificationEventHandle == NULL ) {

        return( ERROR_INVALID_PARAMETER );
    }

    EventHandle = CreateEvent( NULL, FALSE, FALSE, NULL );

    if ( EventHandle == NULL ) {

        Err = GetLastError();

    } else {

        Status = LsaRegisterPolicyChangeNotification( PolicyNotifyDnsDomainInformation,
                                                      EventHandle );

        //
        // If the function was successful, return the event handle.  Otherwise,
        // close the event
        //
        if ( !NT_SUCCESS( Status ) ) {

            CloseHandle( EventHandle );
            Err = NetpNtStatusToApiStatus( Status );

        } else {

            *NotificationEventHandle = EventHandle;
        }


    }

    return( Err );
}




NET_API_STATUS
NET_API_FUNCTION
NetUnregisterDomainNameChangeNotification(
    HANDLE NotificationEventHandle
    )
/*++

Routine Description:

    This function is used to unregister a previously registered notification
    for a domain name change.

    The input handle is closed.

Arguments:

    NotificationHandle - The notification event handle to unregister


Return Value:

    NERR_Success - Success

    ERROR_INVALID_PARAMETER - A NULL NotificationEventHandle was given


--*/
{
    NTSTATUS Status;
    DWORD Err = NERR_Success;

    //
    // Parameter check
    //
    if ( NotificationEventHandle == NULL ) {

        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Unregister the event
    //
    Status = LsaUnregisterPolicyChangeNotification( PolicyNotifyDnsDomainInformation,
                                                    NotificationEventHandle );

    Err = NetpNtStatusToApiStatus( Status );

    //
    // If the unregister was successful, close the event handle
    //
    if ( Err == NERR_Success ) {

        CloseHandle( NotificationEventHandle );

    }


    return( Err );
}



