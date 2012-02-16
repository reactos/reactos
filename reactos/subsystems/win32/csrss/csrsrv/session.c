/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CSR Sub System
 * FILE:            subsys/csr/csrsrv/session.c
 * PURPOSE:         CSR Server DLL Session Implementation
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include "srv.h"

#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

RTL_CRITICAL_SECTION CsrNtSessionLock;
LIST_ENTRY CsrNtSessionList;

/* PRIVATE FUNCTIONS *********************************************************/

/*++
 * @name CsrInitializeNtSessionList
 *
 * The CsrInitializeNtSessionList routine sets up support for CSR Sessions.
 *
 * @param None
 *
 * @return None
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrInitializeNtSessionList(VOID)
{
    DPRINT("CSRSRV: %s called\n", __FUNCTION__);

    /* Initialize the Session List */
    InitializeListHead(&CsrNtSessionList);

    /* Initialize the Session Lock */
    return RtlInitializeCriticalSection(&CsrNtSessionLock);
}

/*++
 * @name CsrAllocateNtSession
 *
 * The CsrAllocateNtSession routine allocates a new CSR NT Session.
 *
 * @param SessionId
 *        Session ID of the CSR NT Session to allocate.
 *
 * @return Pointer to the newly allocated CSR NT Session.
 *
 * @remarks None.
 *
 *--*/
PCSR_NT_SESSION
NTAPI
CsrAllocateNtSession(IN ULONG SessionId)
{
    PCSR_NT_SESSION NtSession;

    /* Allocate an NT Session Object */
    NtSession = RtlAllocateHeap(CsrHeap, 0, sizeof(CSR_NT_SESSION));
    if (NtSession)
    {
        /* Setup the Session Object */
        NtSession->SessionId = SessionId;
        NtSession->ReferenceCount = 1;

        /* Insert it into the Session List */
        CsrAcquireNtSessionLock();
        InsertHeadList(&CsrNtSessionList, &NtSession->SessionLink);
        CsrReleaseNtSessionLock();
    }
    else
    {
        ASSERT(NtSession != NULL);
    }

    /* Return the Session (or NULL) */
    return NtSession;
}

/*++
 * @name CsrReferenceNtSession
 *
 * The CsrReferenceNtSession increases the reference count of a CSR NT Session.
 *
 * @param Session
 *        Pointer to the CSR NT Session to reference.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
CsrReferenceNtSession(IN PCSR_NT_SESSION Session)
{
    /* Acquire the lock */
    CsrAcquireNtSessionLock();

    /* Sanity checks */
    ASSERT(!IsListEmpty(&Session->SessionLink));
    ASSERT(Session->SessionId != 0);
    ASSERT(Session->ReferenceCount != 0);

    /* Increase the reference count */
    Session->ReferenceCount++;

    /* Release the lock */
    CsrReleaseNtSessionLock();
}

/*++
 * @name CsrDereferenceNtSession
 *
 * The CsrDereferenceNtSession decreases the reference count of a
 * CSR NT Session.
 *
 * @param Session
 *        Pointer to the CSR NT Session to reference.
 *
 * @param ExitStatus
 *        If this is the last reference to the session, this argument
 *        specifies the exit status.
 *
 * @return None.
 *
 * @remarks CsrDereferenceNtSession will complete the session if
 *          the last reference to it has been closed.
 *
 *--*/
VOID
NTAPI
CsrDereferenceNtSession(IN PCSR_NT_SESSION Session,
                        IN NTSTATUS ExitStatus)
{
    /* Acquire the lock */
    CsrAcquireNtSessionLock();

    /* Sanity checks */
    ASSERT(!IsListEmpty(&Session->SessionLink));
    ASSERT(Session->SessionId != 0);
    ASSERT(Session->ReferenceCount != 0);

    /* Dereference the Session Object */
    if (!(--Session->ReferenceCount))
    {
        /* Remove it from the list */
        RemoveEntryList(&Session->SessionLink);

        /* Release the lock */
        CsrReleaseNtSessionLock();

        /* Tell SM that we're done here */
        SmSessionComplete(CsrSmApiPort, Session->SessionId, ExitStatus);

        /* Free the Session Object */
        RtlFreeHeap(CsrHeap, 0, Session);
    }
    else
    {
        /* Release the lock, the Session is still active */
        CsrReleaseNtSessionLock();
    }
}

/* EOF */
