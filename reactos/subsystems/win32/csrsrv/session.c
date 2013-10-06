/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Client/Server Runtime SubSystem
 * FILE:            subsystems/win32/csrsrv/session.c
 * PURPOSE:         CSR Server DLL Session Implementation
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *******************************************************************/

#include "srv.h"

#define NDEBUG
#include <debug.h>

/* DATA ***********************************************************************/

RTL_CRITICAL_SECTION CsrNtSessionLock;
LIST_ENTRY CsrNtSessionList;

PSB_API_ROUTINE CsrServerSbApiDispatch[SbpMaxApiNumber - SbpCreateSession] =
{
    CsrSbCreateSession,
    CsrSbTerminateSession,
    CsrSbForeignSessionComplete,
    CsrSbCreateProcess
};

PCHAR CsrServerSbApiName[SbpMaxApiNumber - SbpCreateSession] =
{
    "SbCreateSession",
    "SbTerminateSession",
    "SbForeignSessionComplete",
    "SbCreateProcess"
};

/* PRIVATE FUNCTIONS **********************************************************/

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
    NtSession = RtlAllocateHeap(CsrHeap, HEAP_ZERO_MEMORY, sizeof(CSR_NT_SESSION));
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
    if ((--Session->ReferenceCount) == 0)
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

/* SESSION MANAGER FUNCTIONS **************************************************/

/*++
 * @name CsrSbCreateSession
 *
 * The CsrSbCreateSession API is called by the Session Manager whenever a new
 * session is created.
 *
 * @param ApiMessage
 *        Pointer to the Session Manager API Message.
 *
 * @return TRUE in case of success, FALSE otherwise.
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
    PCSR_THREAD CsrThread;
    PCSR_SERVER_DLL ServerDll;
    PVOID ProcessData;
    NTSTATUS Status;
    KERNEL_USER_TIMES KernelTimes;
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
        CsrReleaseProcessLock();

        ApiMessage->ReturnValue = STATUS_NO_MEMORY;
        return TRUE;
    }

    /* Setup the Thread Object */
    CsrThread->CreateTime = KernelTimes.CreateTime;
    CsrThread->ClientId = CreateSession->ProcessInfo.ClientId;
    CsrThread->ThreadHandle = hThread;
    ProtectHandle(hThread);
    CsrThread->Flags = 0;

    /* Insert it into the Process List */
    Status = CsrInsertThread(CsrProcess, CsrThread);
    if (!NT_SUCCESS(Status))
    {
        /* Bail out */
        CsrDeallocateProcess(CsrProcess);
        CsrDeallocateThread(CsrThread);
        CsrReleaseProcessLock();

        /* Strange as it seems, NTSTATUSes are actually returned */
        return (BOOLEAN)Status;
    }

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
        /* Get the current Server */
        ServerDll = CsrLoadedServerDll[i];

        /* Check if the DLL is loaded and has Process Data */
        if (ServerDll && ServerDll->SizeOfProcessData)
        {
            /* Write the pointer to the data */
            CsrProcess->ServerData[i] = ProcessData;

            /* Move to the next data location */
            ProcessData = (PVOID)((ULONG_PTR)ProcessData +
                                  ServerDll->SizeOfProcessData);
        }
        else
        {
            /* Nothing for this Process */
            CsrProcess->ServerData[i] = NULL;
        }
    }

    /* Insert the Process */
    CsrInsertProcess(NULL, CsrProcess);

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
 * @return TRUE in case of success, FALSE otherwise.
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
 * @return TRUE in case of success, FALSE otherwise.
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
 * @return TRUE in case of success, FALSE otherwise.
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

/*++
 * @name CsrSbApiHandleConnectionRequest
 *
 * The CsrSbApiHandleConnectionRequest routine handles and accepts a new
 * connection request to the SM API LPC Port.
 *
 * @param ApiMessage
 *        Pointer to the incoming CSR API Message which contains the
 *        connection request.
 *
 * @return STATUS_SUCCESS in case of success, or status code which caused
 *         the routine to error.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrSbApiHandleConnectionRequest(IN PSB_API_MSG Message)
{
    NTSTATUS Status;
    REMOTE_PORT_VIEW RemotePortView;
    HANDLE hPort;

    /* Set the Port View Structure Length */
    RemotePortView.Length = sizeof(REMOTE_PORT_VIEW);

    /* Accept the connection */
    Status = NtAcceptConnectPort(&hPort,
                                 NULL,
                                 &Message->h,
                                 TRUE,
                                 NULL,
                                 &RemotePortView);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSS: Sb Accept Connection failed %lx\n", Status);
        return Status;
    }

    /* Complete the Connection */
    Status = NtCompleteConnectPort(hPort);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSS: Sb Complete Connection failed %lx\n",Status);
    }

    /* Return status */
    return Status;
}

/*++
 * @name CsrSbApiRequestThread
 *
 * The CsrSbApiRequestThread routine handles incoming messages or connection
 * requests on the SM API LPC Port.
 *
 * @param Parameter
 *        System-default user-defined parameter. Unused.
 *
 * @return The thread exit code, if the thread is terminated.
 *
 * @remarks Before listening on the port, the routine will first attempt
 *          to connect to the user subsystem.
 *
 *--*/
VOID
NTAPI
CsrSbApiRequestThread(IN PVOID Parameter)
{
    NTSTATUS Status;
    SB_API_MSG ReceiveMsg;
    PSB_API_MSG ReplyMsg = NULL;
    PVOID PortContext;
    ULONG MessageType;

    /* Start the loop */
    while (TRUE)
    {
        /* Wait for a message to come in */
        Status = NtReplyWaitReceivePort(CsrSbApiPort,
                                        &PortContext,
                                        &ReplyMsg->h,
                                        &ReceiveMsg.h);

        /* Check if we didn't get success */
        if (Status != STATUS_SUCCESS)
        {
            /* If we only got a warning, keep going */
            if (NT_SUCCESS(Status)) continue;

            /* We failed big time, so start out fresh */
            ReplyMsg = NULL;
            DPRINT1("CSRSS: ReceivePort failed - Status == %X\n", Status);
            continue;
        }

        /* Save the message type */
        MessageType = ReceiveMsg.h.u2.s2.Type;

        /* Check if this is a connection request */
        if (MessageType == LPC_CONNECTION_REQUEST)
        {
            /* Handle connection request */
            CsrSbApiHandleConnectionRequest(&ReceiveMsg);

            /* Start over */
            ReplyMsg = NULL;
            continue;
        }

        /* Check if the port died */
        if (MessageType == LPC_PORT_CLOSED)
        {
            /* Close the handle if we have one */
            if (PortContext) NtClose((HANDLE)PortContext);

            /* Client died, start over */
            ReplyMsg = NULL;
            continue;
        }
        else if (MessageType == LPC_CLIENT_DIED)
        {
            /* Client died, start over */
            ReplyMsg = NULL;
            continue;
        }

        /*
         * It's an API Message, check if it's within limits. If it's not,
         * the NT Behaviour is to set this to the Maximum API.
         */
        if (ReceiveMsg.ApiNumber > SbpMaxApiNumber)
        {
            ReceiveMsg.ApiNumber = SbpMaxApiNumber;
            DPRINT1("CSRSS: %lx is invalid Sb ApiNumber\n", ReceiveMsg.ApiNumber);
        }

        /* Reuse the message */
        ReplyMsg = &ReceiveMsg;

        /* Make sure that the message is supported */
        if (ReceiveMsg.ApiNumber < SbpMaxApiNumber)
        {
            /* Call the API */
            if (!CsrServerSbApiDispatch[ReceiveMsg.ApiNumber](&ReceiveMsg))
            {
                DPRINT1("CSRSS: %s Session Api called and failed\n",
                        CsrServerSbApiName[ReceiveMsg.ApiNumber]);

                /* It failed, so return nothing */
                ReplyMsg = NULL;
            }
        }
        else
        {
            /* We don't support this API Number */
            ReplyMsg->ReturnValue = STATUS_NOT_IMPLEMENTED;
        }
    }
}

/* EOF */
