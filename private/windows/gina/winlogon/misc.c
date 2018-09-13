//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       misc.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-25-94   RichardW   Created
//
//----------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop

HANDLE    g_hEventLog;

/***************************************************************************\
* ReportWinlogonEvent
*
* Reports winlogon event by calling ReportEvent.
*
* History:
* 10-Dec-93  JohanneC   Created
*
\***************************************************************************/
#define MAX_EVENT_STRINGS 8

DWORD
ReportWinlogonEvent(
    IN PTERMINAL pTerm,
    IN WORD EventType,
    IN DWORD EventId,
    IN DWORD SizeOfRawData,
    IN PVOID RawData,
    IN DWORD NumberOfStrings,
    ...
    )
{
    va_list arglist;
    ULONG i;
    PWSTR Strings[ MAX_EVENT_STRINGS ];
    DWORD rv;

    va_start( arglist, NumberOfStrings );

    if (NumberOfStrings > MAX_EVENT_STRINGS) {
        NumberOfStrings = MAX_EVENT_STRINGS;
    }

    for (i=0; i<NumberOfStrings; i++) {
        Strings[ i ] = va_arg( arglist, PWSTR );
    }

    if (g_hEventLog == NULL) {

        g_hEventLog = RegisterEventSource(NULL, EVENTLOG_SOURCE);

        if (g_hEventLog == NULL) {
            return ERROR_INVALID_HANDLE;
        }
    }

    if (!ReportEvent( g_hEventLog,
                       EventType,
                       0,            // event category
                       EventId,
                       NULL,
                       (WORD)NumberOfStrings,
                       SizeOfRawData,
                       Strings,
                       RawData) ) {
        rv = GetLastError();
        DebugLog((DEB_ERROR,  "WINLOGON: ReportEvent( %u ) failed - %u\n", EventId, GetLastError() ));
    } else {
        rv = ERROR_SUCCESS;
    }

    return rv;
}

/***************************************************************************\
* TimeoutMessageBox
*
* Same as a normal message box, but times out if there is no user input
* for the specified number of seconds
* For convenience, this api takes string resource ids rather than string
* pointers as input. The resources are loaded from the .exe module
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

int
TimeoutMessageBox(
    PTERMINAL pTerm,
    HWND hwnd,
    UINT IdText,
    UINT IdCaption,
    UINT wType)
{
    TCHAR    CaptionBuffer[MAX_STRING_BYTES];
    PTCHAR   Caption = CaptionBuffer;
    TCHAR    Text[3 * MAX_STRING_BYTES];

    LoadString(NULL, IdText, Text, ARRAYSIZE(Text));

    if (IdCaption != 0) {
        LoadString(NULL, IdCaption, Caption, ARRAYSIZE(CaptionBuffer));
    } else {
        Caption = NULL;
    }

    return WlxMessageBox(pTerm, hwnd, Text, Caption, wType);
}
