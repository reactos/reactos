/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/lsasrv/session.c
 * PURPOSE:     Logon session management routines
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

#include "lsasrv.h"

typedef struct _LSAP_LOGON_SESSION
{
    LIST_ENTRY Entry;
    LUID LogonId;
} LSAP_LOGON_SESSION, *PLSAP_LOGON_SESSION;


/* GLOBALS *****************************************************************/

LIST_ENTRY SessionListHead;
ULONG SessionCount;

/* FUNCTIONS ***************************************************************/

VOID
LsapInitLogonSessions(VOID)
{
    InitializeListHead(&SessionListHead);
    SessionCount = 0;
}


static
PLSAP_LOGON_SESSION
LsapGetLogonSession(IN PLUID LogonId)
{
    PLIST_ENTRY SessionEntry;
    PLSAP_LOGON_SESSION CurrentSession;

    SessionEntry = SessionListHead.Flink;
    while (SessionEntry != &SessionListHead)
    {
        CurrentSession = CONTAINING_RECORD(SessionEntry,
                                           LSAP_LOGON_SESSION,
                                           Entry);
        if (RtlEqualLuid(&CurrentSession->LogonId, LogonId))
            return CurrentSession;

        SessionEntry = SessionEntry->Flink;
    }

    return NULL;
}


NTSTATUS
NTAPI
LsapCreateLogonSession(IN PLUID LogonId)
{
    PLSAP_LOGON_SESSION Session;

    TRACE("()\n");

    /* Fail, if a session already exists */
    if (LsapGetLogonSession(LogonId) != NULL)
        return STATUS_LOGON_SESSION_COLLISION;

    /* Allocate a new session entry */
    Session = RtlAllocateHeap(RtlGetProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              sizeof(LSAP_LOGON_SESSION));
    if (Session == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize the session entry */
    RtlCopyLuid(&Session->LogonId, LogonId);

    /* Insert the new session into the session list */
    InsertTailList(&SessionListHead, &Session->Entry);
    SessionCount++;

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
LsapDeleteLogonSession(IN PLUID LogonId)
{
    PLSAP_LOGON_SESSION Session;

    TRACE("()\n");

    /* Fail, if the session does not exist */
    Session = LsapGetLogonSession(LogonId);
    if (Session == NULL)
        return STATUS_NO_SUCH_LOGON_SESSION;

    /* Remove the session entry from the list */
    RemoveEntryList(&Session->Entry);
    SessionCount--;

    /* Free the session entry */
    RtlFreeHeap(RtlGetProcessHeap(), 0, Session);

    return STATUS_SUCCESS;
}

/* EOF */
