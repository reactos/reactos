/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    snmpevts.c

Abstract:

    Eventlog message routines for the SNMP Service.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID
SNMP_FUNC_TYPE
ReportSnmpEvent(
    DWORD   nMsgId, 
    DWORD   nSubStrings, 
    LPTSTR *ppSubStrings,
    DWORD   nErrorCode
    )

/*++

Routine Description:

    Reports event with EventLog service.

Arguments:

    nMsgId - message identifier.

    nSubStrings - number of message strings.
    
    ppSubStrings - pointer to array of message strings.
    
    nErrorCode - error code to be attached to event.            

Return Values:

    Returns true if successful.

--*/

{
    HANDLE lh;
    WORD   wEventType;
    LPVOID lpData;
    WORD   cbData;

    //    
    // determine type of event from message id.  note that
    // all debug messages regardless of their severity are
    // listed under SNMP_EVENT_DEBUG_TRACE (informational).
    // see snmpevts.h for the entire list of event messages.
    //

    switch ( nMsgId >> 30 ) {

    case STATUS_SEVERITY_INFORMATIONAL:
    case STATUS_SEVERITY_SUCCESS:
        wEventType = EVENTLOG_INFORMATION_TYPE;
        break;

    case STATUS_SEVERITY_WARNING:
        wEventType = EVENTLOG_WARNING_TYPE;
        break;

    case STATUS_SEVERITY_ERROR:
    default:
        wEventType = EVENTLOG_ERROR_TYPE;
        break;
    }

    // determine size of data by whether error present
    cbData = (nErrorCode == NO_ERROR) ? 0 : sizeof(DWORD);
    lpData = (nErrorCode == NO_ERROR) ? NULL : &nErrorCode;

    // attempt to register event sources
    if (lh = RegisterEventSource(NULL, TEXT("SNMP"))) {

        // report
        ReportEvent(
           lh,
           wEventType,
           0,                  // event category
           nMsgId,
           NULL,               // user sids
           (WORD)nSubStrings,
           cbData,
           ppSubStrings,
           lpData
           );

        // deregister event source
        DeregisterEventSource(lh);
    }
}