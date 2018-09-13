/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    msgalias.h

Abstract:

    Prototypes for functions that add/delete message aliases.

Author:

    Dan Lafferty (danl)     20-Mar-1991

Environment:

    User Mode -Win32

Notes:

    optional-notes

Revision History:

    20-Mar-1991     danl
        created
        .
        .
    least-recent-revision-date email-name
        description

--*/

//
// GetProcAddr Prototypes
//

typedef DWORD   (*PMSG_NAME_ENUM) (
                    LPWSTR      servername,
                    DWORD       level,
                    LPBYTE      *bufptr,
                    DWORD       prefmaxlen,
                    LPDWORD     entriesread,
                    LPDWORD     totalentries,
                    LPDWORD     resume_handle
                    );

typedef DWORD   (*PMSG_NAME_DEL) (
                    LPWSTR servername,
                    LPWSTR msgname
                    );


//
// Function Prototypes
//

VOID
DeleteMsgAliases(
    VOID
    );


VOID
TickleMessenger(VOID);
