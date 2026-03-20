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

LIST_ENTRY CsrNtSessionHashTable[NUMBER_NT_SESSION_HASH_BUCKETS];

static
ULONG
NTAPI
CsrHashNtSession(IN ULONG SessionId)
{
    return (SessionId % NUMBER_NT_SESSION_HASH_BUCKETS);
}

/*++
 * @name CsrInitializeNtSessionList
 */
NTSTATUS
NTAPI
CsrInitializeNtSessionList(VOID)
{
    ULONG i;

    InitializeListHead(&CsrNtSessionList);
    for (i = 0; i < NUMBER_NT_SESSION_HASH_BUCKETS; i++)
    {
        InitializeListHead(&CsrNtSessionHashTable[i]);
    }

    return RtlInitializeCriticalSection(&CsrNtSessionLock);
}

PCSR_NT_SESSION
NTAPI
CsrCreateNtSession(IN ULONG SessionId)
{
    PCSR_NT_SESSION NtSession;
    ULONG i;

    NtSession = RtlAllocateHeap(CsrHeap, HEAP_ZERO_MEMORY, sizeof(CSR_NT_SESSION));
    if (!NtSession)
    {
        ASSERT(NtSession != NULL);
        return NULL;
    }

    NtSession->SessionId = SessionId;
    NtSession->ReferenceCount = 1;
    NtSession->State = CsrNtSessionStateActive;

    i = CsrHashNtSession(SessionId);

    CsrAcquireNtSessionLock();
    if (!IsListEmpty(&CsrNtSessionHashTable[i]))
    {
        PLIST_ENTRY NextEntry = CsrNtSessionHashTable[i].Flink;
        while (NextEntry != &CsrNtSessionHashTable[i])
        {
            PCSR_NT_SESSION ExistingSession;
            ExistingSession = CONTAINING_RECORD(NextEntry, CSR_NT_SESSION, SessionHashLink);
            if (ExistingSession->SessionId == SessionId)
            {
                CsrReleaseNtSessionLock();
                RtlFreeHeap(CsrHeap, 0, NtSession);
                return NULL;
            }
            NextEntry = NextEntry->Flink;
        }
    }

    InsertHeadList(&CsrNtSessionList, &NtSession->SessionLink);
    InsertHeadList(&CsrNtSessionHashTable[i], &NtSession->SessionHashLink);
    CsrReleaseNtSessionLock();

    return NtSession;
}

PCSR_NT_SESSION
NTAPI
CsrFindNtSession(IN ULONG SessionId)
{
    PLIST_ENTRY NextEntry;
    PLIST_ENTRY ListHead;
    PCSR_NT_SESSION NtSession;

    CsrAcquireNtSessionLock();

    ListHead = &CsrNtSessionHashTable[CsrHashNtSession(SessionId)];
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        NtSession = CONTAINING_RECORD(NextEntry, CSR_NT_SESSION, SessionHashLink);
        if (NtSession->SessionId == SessionId)
        {
            ASSERT(NtSession->ReferenceCount != 0);
            NtSession->ReferenceCount++;
            CsrReleaseNtSessionLock();
            return NtSession;
        }

        NextEntry = NextEntry->Flink;
    }

    CsrReleaseNtSessionLock();
    return NULL;
}

PCSR_NT_SESSION
NTAPI
CsrGetCurrentNtSession(VOID)
{
    return CsrFindNtSession(NtCurrentPeb()->SessionId);
}

BOOLEAN
NTAPI
CsrNtSessionExists(IN ULONG SessionId)
{
    PLIST_ENTRY NextEntry;
    PLIST_ENTRY ListHead;
    PCSR_NT_SESSION NtSession;

    CsrAcquireNtSessionLock();

    ListHead = &CsrNtSessionHashTable[CsrHashNtSession(SessionId)];
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        NtSession = CONTAINING_RECORD(NextEntry, CSR_NT_SESSION, SessionHashLink);
        if (NtSession->SessionId == SessionId)
        {
            CsrReleaseNtSessionLock();
            return TRUE;
        }

        NextEntry = NextEntry->Flink;
    }

    CsrReleaseNtSessionLock();
    return FALSE;
}

VOID
NTAPI
CsrReferenceNtSession(IN PCSR_NT_SESSION Session)
{
    CsrAcquireNtSessionLock();

    ASSERT(!IsListEmpty(&Session->SessionLink));
    ASSERT(Session->SessionId != 0);
    ASSERT(Session->ReferenceCount != 0);

    Session->ReferenceCount++;

    CsrReleaseNtSessionLock();
}

VOID
NTAPI
CsrDestroyNtSession(IN PCSR_NT_SESSION Session,
                    IN NTSTATUS ExitStatus)
{
    Session->State = CsrNtSessionStateTerminating;
    SmSessionComplete(CsrSmApiPort, Session->SessionId, ExitStatus);
    RtlFreeHeap(CsrHeap, 0, Session);
}

VOID
NTAPI
CsrDereferenceNtSession(IN PCSR_NT_SESSION Session,
                        IN NTSTATUS ExitStatus)
{
    CsrAcquireNtSessionLock();

    ASSERT(!IsListEmpty(&Session->SessionLink));
    ASSERT(Session->SessionId != 0);
    ASSERT(Session->ReferenceCount != 0);

    if ((--Session->ReferenceCount) == 0)
    {
        RemoveEntryList(&Session->SessionHashLink);
        RemoveEntryList(&Session->SessionLink);
        CsrReleaseNtSessionLock();
        CsrDestroyNtSession(Session, ExitStatus);
    }
    else
    {
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
    PSB_CREATE_SESSION_MSG CreateSession = &ApiMessage->u.CreateSession;
    HANDLE hProcess, hThread;
    PCSR_PROCESS CsrProcess;
    PCSR_THREAD CsrThread;
    PCSR_SERVER_DLL ServerDll;
    PVOID ProcessData;
    NTSTATUS Status;
    KERNEL_USER_TIMES KernelTimes;
    PCSR_NT_SESSION NtSession;
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

    /* Set the Exception Port for us */
    Status = NtSetInformationProcess(hProcess,
                                     ProcessExceptionPort,
                                     &CsrApiPort,
                                     sizeof(CsrApiPort));

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
                                      sizeof(KernelTimes),
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

    /* Create and attach the Session */
    NtSession = CsrCreateNtSession(CreateSession->SessionId);
    if (!NtSession)
    {
        CsrDeallocateProcess(CsrProcess);
        CsrReleaseProcessLock();

        ApiMessage->ReturnValue = STATUS_NO_MEMORY;
        return TRUE;
    }

    CsrProcess->NtSession = NtSession;
    CsrAcquireNtSessionLock();
    CsrProcess->NtSession->ProcessCount++;
    CsrReleaseNtSessionLock();

    /* Allocate a new Thread */
    CsrThread = CsrAllocateThread(CsrProcess);
    if (!CsrThread)
    {
        /* Fail the request */
        CsrAcquireNtSessionLock();
        ASSERT(CsrProcess->NtSession->ProcessCount != 0);
        CsrProcess->NtSession->ProcessCount--;
        CsrReleaseNtSessionLock();
        CsrDereferenceNtSession(CsrProcess->NtSession, STATUS_NO_MEMORY);
        CsrProcess->NtSession = NULL;
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
        CsrAcquireNtSessionLock();
        ASSERT(CsrProcess->NtSession->ProcessCount != 0);
        CsrProcess->NtSession->ProcessCount--;
        CsrReleaseNtSessionLock();
        CsrDereferenceNtSession(CsrProcess->NtSession, Status);
        CsrProcess->NtSession = NULL;
        CsrDeallocateProcess(CsrProcess);
        CsrDeallocateThread(CsrThread);
        CsrReleaseProcessLock();

        /* Strange as it seems, NTSTATUSes are actually returned */
        return (BOOLEAN)Status;
    }

    /* Setup Process Data */
    CsrProcess->ClientId = CreateSession->ProcessInfo.ClientId;
    CsrProcess->ProcessHandle = hProcess;

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
