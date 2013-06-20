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
HANDLE CsrSmApiPort;

PSB_API_ROUTINE CsrServerSbApiDispatch[5] =
{
    CsrSbCreateSession,
    CsrSbTerminateSession,
    CsrSbForeignSessionComplete,
    CsrSbCreateProcess,
    NULL
};

PCHAR CsrServerSbApiName[5] =
{
    "SbCreateSession",
    "SbTerminateSession",
    "SbForeignSessionComplete",
    "SbCreateProcess",
    "Unknown Csr Sb Api Number"
};

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


/* SESSION MANAGER FUNCTIONS**************************************************/

/*++
 * @name CsrSbCreateSession
 *
 * The CsrSbCreateSession API is called by the Session Manager whenever a new
 * session is created.
 *
 * @param ApiMessage
 *        Pointer to the Session Manager API Message.
 *
 * @return TRUE in case of success, FALSE othwerwise.
 *
 * @remarks The CsrSbCreateSession routine will initialize a new CSR NT
 *          Session and allocate a new CSR Process for the subsystem process.
 *
 *--*/
BOOLEAN
NTAPI
CsrSbCreateSession(IN PSB_API_MSG ApiMessage)
{
    PSB_CREATE_SESSION_MSG CreateSession = &ApiMessage->CreateSession;
    HANDLE hProcess, hThread;
    PCSR_PROCESS CsrProcess;
    NTSTATUS Status;
    KERNEL_USER_TIMES KernelTimes;
    PCSR_THREAD CsrThread;
    PVOID ProcessData;
    ULONG i;

    /* Save the Process and Thread Handles */
    hProcess = CreateSession->ProcessInfo.ProcessHandle;
    hThread = CreateSession->ProcessInfo.ThreadHandle;

    /* Lock the Processes */
    CsrAcquireProcessLock();

    /* Allocate a new process */
    CsrProcess = CsrAllocateProcess();
    if (!CsrProcess)
    {
        /* Fail */
        ApiMessage->ReturnValue = STATUS_NO_MEMORY;
        CsrReleaseProcessLock();
        return TRUE;
    }

    /* Set the exception port */
    Status = NtSetInformationProcess(hProcess,
                                     ProcessExceptionPort,
                                     &CsrApiPort,
                                     sizeof(HANDLE));

    /* Check for success */
    if (!NT_SUCCESS(Status))
    {
        /* Fail the request */
        CsrDeallocateProcess(CsrProcess);
        CsrReleaseProcessLock();

        /* Strange as it seems, NTSTATUSes are actually returned */
        return (BOOLEAN)STATUS_NO_MEMORY;
    }

    /* Get the Create Time */
    Status = NtQueryInformationThread(hThread,
                                      ThreadTimes,
                                      &KernelTimes,
                                      sizeof(KERNEL_USER_TIMES),
                                      NULL);

    /* Check for success */
    if (!NT_SUCCESS(Status))
    {
        /* Fail the request */
        CsrDeallocateProcess(CsrProcess);
        CsrReleaseProcessLock();

        /* Strange as it seems, NTSTATUSes are actually returned */
        return (BOOLEAN)Status;
    }

    /* Allocate a new Thread */
    CsrThread = CsrAllocateThread(CsrProcess);
    if (!CsrThread)
    {
        /* Fail the request */
        CsrDeallocateProcess(CsrProcess);
        ApiMessage->ReturnValue = STATUS_NO_MEMORY;
        CsrReleaseProcessLock();
        return TRUE;
    }

    /* Setup the Thread Object */
    CsrThread->CreateTime = KernelTimes.CreateTime;
    CsrThread->ClientId = CreateSession->ProcessInfo.ClientId;
    CsrThread->ThreadHandle = hThread;
    ProtectHandle(hThread);
    CsrThread->Flags = 0;

    /* Insert it into the Process List */
    CsrInsertThread(CsrProcess, CsrThread);

    /* Setup Process Data */
    CsrProcess->ClientId = CreateSession->ProcessInfo.ClientId;
    CsrProcess->ProcessHandle = hProcess;
    CsrProcess->NtSession = CsrAllocateNtSession(CreateSession->SessionId);

    /* Set the Process Priority */
    CsrSetBackgroundPriority(CsrProcess);

    /* Get the first data location */
    ProcessData = &CsrProcess->ServerData[CSR_SERVER_DLL_MAX];

    /* Loop every DLL */
    for (i = 0; i < CSR_SERVER_DLL_MAX; i++)
    {
        /* Check if the DLL is loaded and has Process Data */
        if (CsrLoadedServerDll[i] && CsrLoadedServerDll[i]->SizeOfProcessData)
        {
            /* Write the pointer to the data */
            CsrProcess->ServerData[i] = ProcessData;

            /* Move to the next data location */
            ProcessData = (PVOID)((ULONG_PTR)ProcessData +
                                  CsrLoadedServerDll[i]->SizeOfProcessData);
        }
        else
        {
            /* Nothing for this Process */
            CsrProcess->ServerData[i] = NULL;
        }
    }

    /* Insert the Process */
    CsrInsertProcess(NULL, NULL, CsrProcess);

    /* Activate the Thread */
    ApiMessage->ReturnValue = NtResumeThread(hThread, NULL);

    /* Release lock and return */
    CsrReleaseProcessLock();
    return TRUE;
}

/*++
 * @name CsrSbForeignSessionComplete
 *
 * The CsrSbForeignSessionComplete API is called by the Session Manager
 * whenever a foreign session is completed (ie: terminated).
 *
 * @param ApiMessage
 *        Pointer to the Session Manager API Message.
 *
 * @return TRUE in case of success, FALSE othwerwise.
 *
 * @remarks The CsrSbForeignSessionComplete API is not yet implemented.
 *
 *--*/
BOOLEAN
NTAPI
CsrSbForeignSessionComplete(IN PSB_API_MSG ApiMessage)
{
    /* Deprecated/Unimplemented in NT */
    ApiMessage->ReturnValue = STATUS_NOT_IMPLEMENTED;
    return TRUE;
}

/*++
 * @name CsrSbTerminateSession
 *
 * The CsrSbTerminateSession API is called by the Session Manager
 * whenever a foreign session should be destroyed.
 *
 * @param ApiMessage
 *        Pointer to the Session Manager API Message.
 *
 * @return TRUE in case of success, FALSE othwerwise.
 *
 * @remarks The CsrSbTerminateSession API is not yet implemented.
 *
 *--*/
BOOLEAN
NTAPI
CsrSbTerminateSession(IN PSB_API_MSG ApiMessage)
{
    ApiMessage->ReturnValue = STATUS_NOT_IMPLEMENTED;
    return TRUE;
}

/*++
 * @name CsrSbCreateProcess
 *
 * The CsrSbCreateProcess API is called by the Session Manager
 * whenever a foreign session is created and a new process should be started.
 *
 * @param ApiMessage
 *        Pointer to the Session Manager API Message.
 *
 * @return TRUE in case of success, FALSE othwerwise.
 *
 * @remarks The CsrSbCreateProcess API is not yet implemented.
 *
 *--*/
BOOLEAN
NTAPI
CsrSbCreateProcess(IN PSB_API_MSG ApiMessage)
{
    ApiMessage->ReturnValue = STATUS_NOT_IMPLEMENTED;
    return TRUE;
}

/* EOF */
