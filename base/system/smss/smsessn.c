/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smsessn.c
 * PURPOSE:         Main SMSS Code
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "smss.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

typedef struct _SMP_SESSION
{
    LIST_ENTRY Entry;
    ULONG SessionId;
    PSMP_SUBSYSTEM Subsystem;
    PSMP_SUBSYSTEM OtherSubsystem;
} SMP_SESSION, *PSMP_SESSION;

RTL_CRITICAL_SECTION SmpSessionListLock;
LIST_ENTRY SmpSessionListHead;
ULONG SmpNextSessionId;
ULONG SmpNextSessionIdScanMode;
BOOLEAN SmpDbgSsLoaded;
HANDLE SmpSessionsObjectDirectory;

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
SmpCheckDuplicateMuSessionId(IN ULONG MuSessionId)
{
    PSMP_SUBSYSTEM Subsystem;
    BOOLEAN FoundDuplicate = FALSE;
    PLIST_ENTRY NextEntry;

    /* Lock the subsystem database */
    RtlEnterCriticalSection(&SmpKnownSubSysLock);

    /* Scan each entry */
    NextEntry = SmpKnownSubSysHead.Flink;
    while (NextEntry != &SmpKnownSubSysHead)
    {
        /* Check if this entry has the same session ID */
        Subsystem = CONTAINING_RECORD(NextEntry, SMP_SUBSYSTEM, Entry);
        if (Subsystem->MuSessionId == MuSessionId)
        {
            /* Break out of here! */
            FoundDuplicate = TRUE;
            break;
        }

        /* Keep going */
        NextEntry = NextEntry->Flink;
    }

    /* Release the database and return the result */
    RtlLeaveCriticalSection(&SmpKnownSubSysLock);
    return FoundDuplicate;
}

PSMP_SESSION
NTAPI
SmpSessionIdToSession(IN ULONG SessionId)
{
    PSMP_SESSION Session, FoundSession = NULL;
    PLIST_ENTRY NextEntry;

    /* Loop the session list -- lock must already be held! */
    NextEntry = SmpSessionListHead.Flink;
    while (NextEntry != &SmpSessionListHead)
    {
        /* Check if this session's ID matches */
        Session = CONTAINING_RECORD(NextEntry, SMP_SESSION, Entry);
        if (Session->SessionId == SessionId)
        {
            /* Set this as the found session and break out */
            FoundSession = Session;
            break;
        }

        /* Keep going */
        NextEntry = NextEntry->Flink;
    }

    /* Return the session that was found and exit */
    return FoundSession;
}

VOID
NTAPI
SmpDeleteSession(IN ULONG SessionId)
{
    PSMP_SESSION Session;

    /* Enter the lock and get the session structure */
    RtlEnterCriticalSection(&SmpSessionListLock);
    Session = SmpSessionIdToSession(SessionId);
    if (Session)
    {
        /* Remove it from the list */
        RemoveEntryList(&Session->Entry);
        RtlLeaveCriticalSection(&SmpSessionListLock);

        /* Now free the structure outside of the lock */
        RtlFreeHeap(SmpHeap, 0, Session);
    }
    else
    {
        /* ID doesn't map to one of our structures, nothing to do... */
        RtlLeaveCriticalSection(&SmpSessionListLock);
    }
}

ULONG
NTAPI
SmpAllocateSessionId(IN PSMP_SUBSYSTEM Subsystem,
                     IN PSMP_SUBSYSTEM OtherSubsystem)
{
    ULONG SessionId;
    PSMP_SESSION Session;

    /* Allocate a new ID while under the lock */
    RtlEnterCriticalSection(&SmpSessionListLock);
    SessionId = SmpNextSessionId++;

    /* Check for overflow */
    if (SmpNextSessionIdScanMode)
    {
        /* Break if it happened */
        DbgPrint("SMSS: SessionId's Wrapped\n");
        DbgBreakPoint();
    }
    else
    {
        /* Detect it for next time */
        if (!SmpNextSessionId) SmpNextSessionIdScanMode = 1;
    }

    /* Allocate a session structure */
    Session = RtlAllocateHeap(SmpHeap, 0, sizeof(SMP_SESSION));
    if (Session)
    {
        /* Write the session data and insert it into the session list */
        Session->Subsystem = Subsystem;
        Session->SessionId = SessionId;
        Session->OtherSubsystem = OtherSubsystem;
        InsertTailList(&SmpSessionListHead, &Session->Entry);
    }
    else
    {
        DPRINT1("SMSS: Unable to keep track of session ID -- no memory available\n");
    }

    /* Release the session lock */
    RtlLeaveCriticalSection(&SmpSessionListLock);
    return SessionId;
}

NTSTATUS
NTAPI
SmpGetProcessMuSessionId(IN HANDLE ProcessHandle,
                         OUT PULONG SessionId)
{
    NTSTATUS Status;
    ULONG ProcessSession;

    /* Query the kernel for the session ID */
    Status = NtQueryInformationProcess(ProcessHandle,
                                       ProcessSessionInformation,
                                       &ProcessSession,
                                       sizeof(ProcessSession),
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Copy it back into the buffer */
        *SessionId = ProcessSession;
    }
    else
    {
        /* Failure -- assume session zero */
        DPRINT1("SMSS: GetProcessMuSessionId, Process=%p, Status=%x\n",
                ProcessHandle, Status);
        *SessionId = 0;
    }

    /* Return result */
    return Status;
}

NTSTATUS
NTAPI
SmpSetProcessMuSessionId(IN HANDLE ProcessHandle,
                         IN ULONG SessionId)
{
    NTSTATUS Status;

    /* Tell the kernel about the session ID */
    Status = NtSetInformationProcess(ProcessHandle,
                                     ProcessSessionInformation,
                                     &SessionId,
                                     sizeof(SessionId));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: SetProcessMuSessionId, Process=%p, Status=%x\n",
                ProcessHandle, Status);
    }

    /* Return */
    return Status;
}
