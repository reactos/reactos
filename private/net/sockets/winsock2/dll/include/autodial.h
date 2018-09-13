/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    autodial.h

Abstract:

    This module contains definitions for
    Autodial support in Winsock.

Author:

    Anthony Discolo (adiscolo)    15-May-1996

Revision History:

--*/

VOID
InitializeAutodial(VOID);

VOID
UninitializeAutodial(VOID);

BOOL
WSAttemptAutodialAddr(
    IN const struct sockaddr FAR *name,
    IN int namelen
    );

BOOL
WSAttemptAutodialName(
    IN const LPWSAQUERYSETW lpqsRestrictions
    );

VOID
WSNoteSuccessfulHostentLookup(
    IN const char FAR *name,
    IN const ULONG ipaddr
    );

