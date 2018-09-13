/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ALERT.C

Abstract:

    This file contains the routine that sends the alert to the admin.

Author:

    Rajen Shah  (rajens)    28-Aug-1991


Revision History:


--*/

//
// INCLUDES
//

#include <eventp.h>
#include <string.h>
#include <lmalert.h>                // LAN Manager alert structures



BOOL
SendAdminAlert (
            ULONG   MessageID,
            ULONG   NumStrings,
            UNICODE_STRING  *pStrings
            )


/*++

Routine Description:

    This routine raises an ADMIN alert with the message specified.


Arguments:

    MessageID  - Message ID.
    NumStrings - Number of replacement strings.
    pStrings   - Array of UNICODE_STRING Replacement strings.

Return Value:

    NONE

Note:

    This routine dynamically links to netapi32.dll to avoid a build
    dependency.

--*/
{

    typedef DWORD (*ALERT_FNC_PTR)(
        IN LPTSTR AlertEventName,
        IN LPVOID VariableInfo,
        IN DWORD VariableInfoSize,
        IN LPTSTR ServiceName
        );


    NET_API_STATUS NetStatus;

    BYTE AlertBuffer[ELF_ADMIN_ALERT_BUFFER_SIZE + sizeof(ADMIN_OTHER_INFO)];
    ADMIN_OTHER_INFO UNALIGNED *VariableInfo = (PADMIN_OTHER_INFO) AlertBuffer;

    DWORD DataSize;
    DWORD i;
    LPWSTR pReplaceString;

    HANDLE dllHandle;
    ALERT_FNC_PTR AlertFunction;

    //
    // Dynamically link to netapi.dll.  If it's not there just return.  Return
    // TRUE so we won't try to send this alert again.
    //

    dllHandle = LoadLibraryA("NetApi32.Dll");
    if ( dllHandle == INVALID_HANDLE_VALUE ) {
        ElfDbgPrint(("[ELF] Failed to load DLL NetApi32.Dll: %ld\n",
                     GetLastError()));
        return(TRUE);
    }

    //
    // Get the address of the service's main entry point.  This
    // entry point has a well-known name.
    //

    AlertFunction = (ALERT_FNC_PTR)GetProcAddress(
                                                dllHandle,
                                                "NetAlertRaiseEx"
                                                );
    if (AlertFunction == NULL ) {
        ElfDbgPrint(("[ELF] Can't find entry NetAlertRaiseEx in NetApi32.Dll: %ld\n",
                     GetLastError()));
        return(TRUE);

    }

    VariableInfo->alrtad_errcode = MessageID;
    VariableInfo->alrtad_numstrings = NumStrings;

    pReplaceString = (LPWSTR)(AlertBuffer + sizeof(ADMIN_OTHER_INFO));

    //
    // Copy over the replacement strings
    //

    for (i = 0; i < NumStrings; i++) {
        RtlMoveMemory(pReplaceString, pStrings[i].Buffer,
            pStrings[i].MaximumLength);
        pReplaceString = (LPWSTR) ((PBYTE) pReplaceString +
            pStrings[i].MaximumLength);
    }

    DataSize = (DWORD) ((PBYTE) pReplaceString - (PBYTE) AlertBuffer);

    NetStatus = (*AlertFunction)(ALERT_ADMIN_EVENT,
                                 AlertBuffer,
                                 DataSize,
                                 EVENTLOG_SVC_NAMEW);

    if (NetStatus != NERR_Success) {
        ElfDbgPrint(("[ELF] NetAlertRaiseEx failed with %d\n", NetStatus));
        ElfDbgPrint(("[ELF] Attempted to raise alert %d\n", MessageID));

        //
        // Probably just not started yet, try again later
        //

        return(FALSE);
    }

    return(TRUE);
}
