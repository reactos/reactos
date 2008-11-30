/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CSR Sub System
 * FILE:            subsys/csr/csrsrv/api.c
 * PURPOSE:         CSR Server DLL API LPC Implementation
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include "srv.h"

#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/
BOOLEAN (*CsrClientThreadSetup)(VOID) = NULL;
ULONG CsrMaxApiRequestThreads;
UNICODE_STRING CsrSbApiPortName;
UNICODE_STRING CsrApiPortName;
HANDLE CsrSbApiPort;
HANDLE CsrApiPort;
PCSR_THREAD CsrSbApiRequestThreadPtr;
ULONG CsrpStaticThreadCount;
ULONG CsrpDynamicThreadTotal;

/* PRIVATE FUNCTIONS *********************************************************/

/*++
 * @name CsrCheckRequestThreads
 *
 * The CsrCheckRequestThreads routine checks if there are no more threads
 * to handle CSR API Requests, and creates a new thread if possible, to
 * avoid starvation.
 *
 * @param None.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         if a new thread couldn't be created.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrCheckRequestThreads(VOID)
{
    HANDLE hThread;
    CLIENT_ID ClientId;
    NTSTATUS Status;

    /* Decrease the count, and see if we're out */
    if (!(_InterlockedDecrement((PLONG)&CsrpStaticThreadCount)))
    {
        /* Check if we've still got space for a Dynamic Thread */
        if (CsrpDynamicThreadTotal < CsrMaxApiRequestThreads)
        {
            /* Create a new dynamic thread */
            Status = RtlCreateUserThread(NtCurrentProcess(),
                                         NULL,
                                         TRUE,
                                         0,
                                         0,
                                         0,
                                         (PVOID)CsrApiRequestThread,
                                         NULL,
                                         &hThread,
                                         &ClientId);
            /* Check success */
            if(NT_SUCCESS(Status))
            {
                /* Increase the thread counts */
                CsrpStaticThreadCount++;
                CsrpDynamicThreadTotal++;

                /* Add a new server thread */
                if (CsrAddStaticServerThread(hThread,
                                             &ClientId,
                                             CsrThreadIsServerThread))
                {
                    /* Activate it */
                    NtResumeThread(hThread,NULL);
                }
                else
                {
                    /* Failed to create a new static thread */
                    CsrpStaticThreadCount--;
                    CsrpDynamicThreadTotal--;

                    /* Terminate it */
                    NtTerminateThread(hThread,0);
                    NtClose(hThread);

                    /* Return */
                    return STATUS_UNSUCCESSFUL;
                }
            }
        }
    }

    /* Success */
    return STATUS_SUCCESS;
}

/*++
 * @name CsrSbApiPortInitialize
 *
 * The CsrSbApiPortInitialize routine initializes the LPC Port used for
 * communications with the Session Manager (SM) and initializes the static
 * thread that will handle connection requests and APIs.
 *
 * @param None
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrSbApiPortInitialize(VOID)
{
    ULONG Size;
    PSECURITY_DESCRIPTOR PortSd;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE hRequestThread;
    CLIENT_ID ClientId;

    /* Calculate how much space we'll need for the Port Name */
    Size = CsrDirectoryName.Length + sizeof(SB_PORT_NAME) + sizeof(WCHAR);

    /* Allocate space for it, and create it */
    CsrSbApiPortName.Buffer = RtlAllocateHeap(CsrHeap, 0, Size);
    CsrSbApiPortName.Length = 0;
    CsrSbApiPortName.MaximumLength = (USHORT)Size;
    RtlAppendUnicodeStringToString(&CsrSbApiPortName, &CsrDirectoryName);
    RtlAppendUnicodeToString(&CsrSbApiPortName, UNICODE_PATH_SEP);
    RtlAppendUnicodeToString(&CsrSbApiPortName, SB_PORT_NAME);

    /* Create Security Descriptor for this Port */
    CsrCreateLocalSystemSD(&PortSd);

    /* Initialize the Attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &CsrSbApiPortName,
                               0,
                               PortSd,
                               NULL);

    /* Create the Port Object */
    Status = NtCreatePort(&CsrSbApiPort,
                          &ObjectAttributes,
                          sizeof(SB_CONNECTION_INFO),
                          sizeof(SB_API_MESSAGE),
                          32 * sizeof(SB_API_MESSAGE));
    if(!NT_SUCCESS(Status))
    {

    }

    /* Create the Thread to handle the API Requests */
    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 TRUE,
                                 0,
                                 0,
                                 0,
                                 (PVOID)CsrSbApiRequestThread,
                                 NULL,
                                 &hRequestThread,
                                 &ClientId);
    if(!NT_SUCCESS(Status))
    {

    }

    /* Add it as a Static Server Thread */
    CsrSbApiRequestThreadPtr = CsrAddStaticServerThread(hRequestThread,
                                                        &ClientId,
                                                        0);

    /* Activate it */
    return NtResumeThread(hRequestThread, NULL);
}

/*++
 * @name CsrApiPortInitialize
 *
 * The CsrApiPortInitialize routine initializes the LPC Port used for
 * communications with the Client/Server Runtime (CSR) and initializes the
 * static thread that will handle connection requests and APIs.
 *
 * @param None
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrApiPortInitialize(VOID)
{
    ULONG Size;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE hRequestEvent, hThread;
    CLIENT_ID ClientId;
    PLIST_ENTRY ListHead, NextEntry;
    PCSR_THREAD ServerThread;

    /* Calculate how much space we'll need for the Port Name */
    Size = CsrDirectoryName.Length + sizeof(CSR_PORT_NAME) + sizeof(WCHAR);

    /* Allocate space for it, and create it */
    CsrApiPortName.Buffer = RtlAllocateHeap(CsrHeap, 0, Size);
    CsrApiPortName.Length = 0;
    CsrApiPortName.MaximumLength = (USHORT)Size;
    RtlAppendUnicodeStringToString(&CsrApiPortName, &CsrDirectoryName);
    RtlAppendUnicodeToString(&CsrApiPortName, UNICODE_PATH_SEP);
    RtlAppendUnicodeToString(&CsrApiPortName, CSR_PORT_NAME);

    /* FIXME: Create a Security Descriptor */

    /* Initialize the Attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &CsrApiPortName,
                               0,
                               NULL,
                               NULL /* FIXME*/);

    /* Create the Port Object */
    Status = NtCreatePort(&CsrApiPort,
                          &ObjectAttributes,
                          sizeof(CSR_CONNECTION_INFO),
                          sizeof(CSR_API_MESSAGE),
                          16 * PAGE_SIZE);
    if(!NT_SUCCESS(Status))
    {

    }

    /* Create the event the Port Thread will use */
    Status = NtCreateEvent(&hRequestEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);
    if(!NT_SUCCESS(Status))
    {

    }

    /* Create the Request Thread */
    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 TRUE,
                                 0,
                                 0,
                                 0,
                                 (PVOID)CsrApiRequestThread,
                                 (PVOID)hRequestEvent,
                                 &hThread,
                                 &ClientId);
    if(!NT_SUCCESS(Status))
    {

    }

    /* Add this as a static thread to CSRSRV */
    CsrAddStaticServerThread(hThread, &ClientId, CsrThreadIsServerThread);

    /* Get the Thread List Pointers */
    ListHead = &CsrRootProcess->ThreadList;
    NextEntry = ListHead->Flink;

    /* Start looping the list */
    while (NextEntry != ListHead)
    {
        /* Get the Thread */
        ServerThread = CONTAINING_RECORD(NextEntry, CSR_THREAD, Link);

        /* Start it up */
        Status = NtResumeThread(ServerThread->ThreadHandle, NULL);

        /* Is this a Server Thread? */
        if (ServerThread->Flags & CsrThreadIsServerThread)
        {
            /* If so, then wait for it to initialize */
            NtWaitForSingleObject(hRequestEvent, FALSE, NULL);
        }

        /* Next thread */
        NextEntry = NextEntry->Flink;
    }

    /* We don't need this anymore */
    NtClose(hRequestEvent);

    /* Return */
    return Status;
}

/*++
 * @name CsrApiRequestThread
 *
 * The CsrApiRequestThread routine handles incoming messages or connection
 * requests on the CSR API LPC Port.
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
NTSTATUS
NTAPI
CsrApiRequestThread(IN PVOID Parameter)
{
    PTEB Teb = NtCurrentTeb();
    LARGE_INTEGER TimeOut;
    PCSR_THREAD CurrentThread;
    NTSTATUS Status;
    PCSR_API_MESSAGE ReplyMsg = NULL;
    CSR_API_MESSAGE ReceiveMsg;
    PCSR_THREAD CsrThread;
    PCSR_PROCESS CsrProcess;
    PHARDERROR_MSG HardErrorMsg;
    PVOID PortContext;
    ULONG MessageType;
    ULONG i;
    PCSR_SERVER_DLL ServerDll;
    PCLIENT_DIED_MSG ClientDiedMsg;
    PDBGKM_MSG DebugMessage;
    ULONG ServerId, ApiId;
    ULONG Reply;

    /* Probably because of the way GDI is loaded, this has to be done here */
    Teb->GdiClientPID = HandleToUlong(Teb->Cid.UniqueProcess);
    Teb->GdiClientTID = HandleToUlong(Teb->Cid.UniqueThread);

    /* Set up the timeout for the connect (30 seconds) */
    TimeOut.QuadPart = -30 * 1000 * 1000 * 10;

    /* Connect to user32 */
    while (!CsrConnectToUser())
    {
        /* Keep trying until we get a response */
        Teb->Win32ClientInfo[0] = 0;
        NtDelayExecution(FALSE, &TimeOut);
    }

    /* Get our thread */
    CurrentThread = Teb->CsrClientThread;

    /* If we got an event... */
    if (Parameter)
    {
        /* Set it, to let stuff waiting on us load */
        NtSetEvent((HANDLE)Parameter, NULL);

        /* Increase the Thread Counts */
        _InterlockedIncrement((PLONG)&CsrpStaticThreadCount);
        _InterlockedIncrement((PLONG)&CsrpDynamicThreadTotal);
    }

    /* Now start the loop */
    while (TRUE)
    {
        /* Make sure the real CID is set */
        Teb->RealClientId = Teb->Cid;

        /* Wait for a message to come through */
        Status = NtReplyWaitReceivePort(CsrApiPort,
                                        &PortContext,
                                        (PPORT_MESSAGE)ReplyMsg,
                                        (PPORT_MESSAGE)&ReceiveMsg);

        /* Check if we didn't get success */
        if(Status != STATUS_SUCCESS)
        {
            /* If we only got a warning, keep going */
            if (NT_SUCCESS(Status)) continue;

            /* We failed big time, so start out fresh */
            ReplyMsg = NULL;
            continue;
        }

        /* Use whatever Client ID we got */
        Teb->RealClientId = ReceiveMsg.Header.ClientId;

        /* Get the Message Type */
        MessageType = ReceiveMsg.Header.u2.s2.Type;

        /* Handle connection requests */
        if (MessageType == LPC_CONNECTION_REQUEST)
        {
            /* Handle the Connection Request */
            CsrApiHandleConnectionRequest(&ReceiveMsg);
            ReplyMsg = NULL;
            continue;
        }

        /* It's some other kind of request. Get the lock for the lookup*/
        CsrAcquireProcessLock();

        /* Now do the lookup to get the CSR_THREAD */
        CsrThread = CsrLocateThreadByClientId(&CsrProcess,
                                              &ReceiveMsg.Header.ClientId);

        /* Did we find a thread? */
        if(!CsrThread)
        {
            /* This wasn't a CSR Thread, release lock */
            CsrReleaseProcessLock();

            /* If this was an exception, handle it */
            if (MessageType == LPC_EXCEPTION)
            {
                ReplyMsg = &ReceiveMsg;
                ReplyMsg->Status = DBG_CONTINUE;
            }
            else if (MessageType == LPC_PORT_CLOSED ||
                     MessageType == LPC_CLIENT_DIED)
            {
                /* The Client or Port are gone, loop again */
                ReplyMsg = NULL;
            }
            else if (MessageType == LPC_ERROR_EVENT)
            {
                /* If it's a hard error, handle this too */
                HardErrorMsg = (PHARDERROR_MSG)&ReceiveMsg;

                /* Default it to unhandled */
                HardErrorMsg->Response = ResponseNotHandled;

                /* Check if there are free api threads */
                CsrCheckRequestThreads();
                if (CsrpStaticThreadCount)
                {
                    /* Loop every Server DLL */
                    for (i = 0; i < CSR_SERVER_DLL_MAX; i++)
                    {
                        /* Get the Server DLL */
                        ServerDll = CsrLoadedServerDll[i];

                        /* Check if it's valid and if it has a Hard Error Callback */
                        if (ServerDll && ServerDll->HardErrorCallback)
                        {
                            /* Call it */
                            (*ServerDll->HardErrorCallback)(CsrThread, HardErrorMsg);

                            /* If it's handled, get out of here */
                            if (HardErrorMsg->Response != ResponseNotHandled) break;
                        }
                    }
                }

                /* Increase the thread count */
                _InterlockedIncrement((PLONG)&CsrpStaticThreadCount);

                /* If the response was 0xFFFFFFFF, we'll ignore it */
                if (HardErrorMsg->Response == 0xFFFFFFFF)
                {
                    ReplyMsg = NULL;
                }
                else
                {
                    ReplyMsg = &ReceiveMsg;
                }
            }
            else if (MessageType == LPC_REQUEST)
            {
                /* This is an API Message coming from a non-CSR Thread */
                ReplyMsg = &ReceiveMsg;
                ReplyMsg->Status = STATUS_ILLEGAL_FUNCTION;
            }
            else if (MessageType == LPC_DATAGRAM)
            {
                /* This is an API call, get the Server ID */
                ServerId = CSR_SERVER_ID_FROM_OPCODE(ReceiveMsg.Opcode);

                /* Make sure that the ID is within limits, and the Server DLL loaded */
                if ((ServerId >= CSR_SERVER_DLL_MAX) ||
                    (!(ServerDll = CsrLoadedServerDll[ServerId])))
                {
                    /* We are beyond the Maximum Server ID */
                    ReplyMsg = NULL;
                }
                else
                {
                    /* Get the API ID */
                    ApiId = CSR_API_ID_FROM_OPCODE(ReceiveMsg.Opcode);

                    /* Normalize it with our Base ID */
                    ApiId -= ServerDll->ApiBase;

                    /* Make sure that the ID is within limits, and the entry exists */
                    if ((ApiId >= ServerDll->HighestApiSupported))
                    {
                        /* We are beyond the Maximum API ID, or it doesn't exist */
                        ReplyMsg = NULL;
                    }

                    /* Assume success */
                    ReceiveMsg.Status = STATUS_SUCCESS;

                    /* Validation complete, start SEH */
                    _SEH2_TRY
                    {
                        /* Make sure we have enough threads */
                        CsrCheckRequestThreads();

                        /* Call the API and get the result */
                        ReplyMsg = NULL;
                        (ServerDll->DispatchTable[ApiId])(&ReceiveMsg, &Reply);

                        /* Increase the static thread count */
                        _InterlockedIncrement((PLONG)&CsrpStaticThreadCount);
                    }
                    _SEH_EXCEPT(CsrUnhandledExceptionFilter)
                    {
                        ReplyMsg = NULL;
                    }
                    _SEH2_END;
                }
            }
            else
            {
                /* Some other ignored message type */
                ReplyMsg = NULL;
            }

            /* Keep going */
            continue;
        }

        /* We have a valid thread, was this an LPC Request? */
        if (MessageType != LPC_REQUEST)
        {
            /* It's not an API, check if the client died */
            if (MessageType == LPC_CLIENT_DIED)
            {
                /* Get the information and check if it matches our thread */
                ClientDiedMsg = (PCLIENT_DIED_MSG)&ReceiveMsg;
                if (ClientDiedMsg->CreateTime.QuadPart == CsrThread->CreateTime.QuadPart)
                {
                    /* Reference the thread */
                    CsrThread->ReferenceCount++;

                    /* Destroy the thread in the API Message */
                    CsrDestroyThread(&ReceiveMsg.Header.ClientId);

                    /* Check if the thread was actually ourselves */
                    if (CsrProcess->ThreadCount == 1)
                    {
                        /* Kill the process manually here */
                        CsrDestroyProcess(&CsrThread->ClientId, 0);
                    }

                    /* Remove our extra reference */
                    CsrLockedDereferenceThread(CsrThread);
                }

                /* Release the lock and keep looping */
                CsrReleaseProcessLock();
                ReplyMsg = NULL;
                continue;
            }

            /* Reference the thread and release the lock */
            CsrThread->ReferenceCount++;
            CsrReleaseProcessLock();

            /* Check if this was an exception */
            if (MessageType == LPC_EXCEPTION)
            {
                /* Kill the process */
                NtTerminateProcess(CsrProcess->ProcessHandle, STATUS_ABANDONED);

                /* Destroy it from CSR */
                CsrDestroyProcess(&ReceiveMsg.Header.ClientId, STATUS_ABANDONED);

                /* Return a Debug Message */
                DebugMessage = (PDBGKM_MSG)&ReceiveMsg;
                DebugMessage->ReturnedStatus = DBG_CONTINUE;
                ReplyMsg = &ReceiveMsg;

                /* Remove our extra reference */
                CsrDereferenceThread(CsrThread);
            }
            else if (MessageType == LPC_ERROR_EVENT)
            {
                /* If it's a hard error, handle this too */
                HardErrorMsg = (PHARDERROR_MSG)&ReceiveMsg;

                /* Default it to unhandled */
                HardErrorMsg->Response = ResponseNotHandled;

                /* Check if there are free api threads */
                CsrCheckRequestThreads();
                if (CsrpStaticThreadCount)
                {
                    /* Loop every Server DLL */
                    for (i = 0; i < CSR_SERVER_DLL_MAX; i++)
                    {
                        /* Get the Server DLL */
                        ServerDll = CsrLoadedServerDll[i];

                        /* Check if it's valid and if it has a Hard Error Callback */
                        if (ServerDll && ServerDll->HardErrorCallback)
                        {
                            /* Call it */
                            (*ServerDll->HardErrorCallback)(CsrThread, HardErrorMsg);

                            /* If it's handled, get out of here */
                            if (HardErrorMsg->Response != ResponseNotHandled) break;
                        }
                    }
                }

                /* Increase the thread count */
                _InterlockedIncrement((PLONG)&CsrpStaticThreadCount);

                /* If the response was 0xFFFFFFFF, we'll ignore it */
                if (HardErrorMsg->Response == 0xFFFFFFFF)
                {
                    ReplyMsg = NULL;
                }
                else
                {
                    CsrDereferenceThread(CsrThread);
                    ReplyMsg = &ReceiveMsg;
                }
            }
            else
            {
                /* Something else */
                CsrDereferenceThread(CsrThread);
                ReplyMsg = NULL;
            }

            /* Keep looping */
            continue;
        }

        /* We got an API Request */
        CsrDereferenceThread(CsrThread);
        CsrReleaseProcessLock();

        /* FIXME: Handle the API */

    }

    /* We're out of the loop for some reason, terminate! */
    NtTerminateThread(NtCurrentThread(), Status);
    return Status;
}

/*++
 * @name CsrApiHandleConnectionRequest
 *
 * The CsrApiHandleConnectionRequest routine handles and accepts a new
 * connection request to the CSR API LPC Port.
 *
 * @param ApiMessage
 *        Pointer to the incoming CSR API Message which contains the
 *        connection request.
 *
 * @return STATUS_SUCCESS in case of success, or status code which caused
 *         the routine to error.
 *
 * @remarks This routine is responsible for attaching the Shared Section to
 *          new clients connecting to CSR.
 *
 *--*/
NTSTATUS
NTAPI
CsrApiHandleConnectionRequest(IN PCSR_API_MESSAGE ApiMessage)
{
    PCSR_THREAD CsrThread = NULL;
    PCSR_PROCESS CsrProcess = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PCSR_CONNECTION_INFO ConnectInfo = &ApiMessage->ConnectionInfo;
    BOOLEAN AllowConnection = FALSE;
    REMOTE_PORT_VIEW RemotePortView;
    HANDLE hPort;

    /* Acquire the Process Lock */
    CsrAcquireProcessLock();

    /* Lookup the CSR Thread */
    CsrThread = CsrLocateThreadByClientId(NULL, &ApiMessage->Header.ClientId);

    /* Check if we have a thread */
    if (CsrThread)
    {
        /* Get the Process */
        CsrProcess = CsrThread->Process;

        /* Make sure we have a Process as well */
        if (CsrProcess)
        {
            /* Reference the Process */
            CsrProcess->ReferenceCount++;

            /* Release the lock */
            CsrReleaseProcessLock();

            /* Duplicate the Object Directory */
            Status = NtDuplicateObject(NtCurrentProcess(),
                                       CsrObjectDirectory,
                                       CsrProcess->ProcessHandle,
                                       &ConnectInfo->ObjectDirectory,
                                       0,
                                       0,
                                       DUPLICATE_SAME_ACCESS |
                                       DUPLICATE_SAME_ATTRIBUTES);

            /* Acquire the lock */
            CsrAcquireProcessLock();

            /* Check for success */
            if (NT_SUCCESS(Status))
            {
                /* Attach the Shared Section */
                Status = CsrSrvAttachSharedSection(CsrProcess, ConnectInfo);

                /* Check how this went */
                if (NT_SUCCESS(Status)) AllowConnection = TRUE;
            }

            /* Dereference the project */
            CsrProcess->ReferenceCount--;
        }
    }

    /* Release the lock */
    CsrReleaseProcessLock();

    /* Setup the Port View Structure */
    RemotePortView.Length = sizeof(REMOTE_PORT_VIEW);
    RemotePortView.ViewSize = 0;
    RemotePortView.ViewBase = NULL;

    /* Save the Process ID */
    ConnectInfo->ProcessId = NtCurrentTeb()->Cid.UniqueProcess;

    /* Accept the Connection */
    Status = NtAcceptConnectPort(&hPort,
                                 AllowConnection ? UlongToPtr(CsrProcess->SequenceNumber) : 0,
                                 &ApiMessage->Header,
                                 AllowConnection,
                                 NULL,
                                 &RemotePortView);

    /* Check if the connection was established, or if we allowed it */
    if (NT_SUCCESS(Status) && AllowConnection)
    {
        /* Set some Port Data in the Process */
        CsrProcess->ClientPort = hPort;
        CsrProcess->ClientViewBase = (ULONG_PTR)RemotePortView.ViewBase;
        CsrProcess->ClientViewBounds = (ULONG_PTR)((ULONG_PTR)RemotePortView.ViewBase +
                                                   (ULONG_PTR)RemotePortView.ViewSize);

        /* Complete the connection */
        Status = NtCompleteConnectPort(hPort);
    }

    /* The accept or complete could've failed, let debug builds know */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSS: Failure to accept connection. Status: %lx\n", Status);
    }

    /* Return status to caller */
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
    SB_API_MESSAGE ReceiveMsg;
    PSB_API_MESSAGE ReplyMsg = NULL;
    PVOID PortContext;
    ULONG MessageType;

    /* Start the loop */
    while (TRUE)
    {
        /* Wait for a message to come in */
        Status = NtReplyWaitReceivePort(CsrSbApiPort,
                                        &PortContext,
                                        (PPORT_MESSAGE)ReplyMsg,
                                        (PPORT_MESSAGE)&ReceiveMsg);

        /* Check if we didn't get success */
        if(Status != STATUS_SUCCESS)
        {
            /* If we only got a warning, keep going */
            if (NT_SUCCESS(Status)) continue;

            /* We failed big time, so start out fresh */
            ReplyMsg = NULL;
            continue;
        }

        /* Save the message type */
        MessageType = ReceiveMsg.Header.u2.s2.Type;

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
         * It's an API Message, check if it's within limits. If it's not, the
         * NT Behaviour is to set this to the Maximum API.
         */
        if (ReceiveMsg.Opcode > 4) ReceiveMsg.Opcode = 4;

        /* Reuse the message */
        ReplyMsg = &ReceiveMsg;

        /* Make sure that the message is supported */
        if (ReceiveMsg.Opcode < 4)
        {
            /* Call the API */
            if (!(CsrServerSbApiDispatch[ReceiveMsg.Opcode])(&ReceiveMsg))
            {
                /* It failed, so return nothing */
                ReplyMsg = NULL;
            }
        }
        else
        {
            /* We don't support this API Number */
            ReplyMsg->Status = STATUS_NOT_IMPLEMENTED;
        }
    }
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
CsrSbApiHandleConnectionRequest(IN PSB_API_MESSAGE Message)
{
    NTSTATUS Status;
    REMOTE_PORT_VIEW RemotePortView;
    HANDLE hPort;

    /* Set the Port View Structure Length */
    RemotePortView.Length = sizeof(REMOTE_PORT_VIEW);

    /* Accept the connection */
    Status = NtAcceptConnectPort(&hPort,
                                 NULL,
                                 (PPORT_MESSAGE)Message,
                                 TRUE,
                                 NULL,
                                 &RemotePortView);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSS: Sb Accept Connection failed %lx\n", Status);
        return Status;
    }

    /* Complete the Connection */
    if (!NT_SUCCESS(Status = NtCompleteConnectPort(hPort)))
    {
        DPRINT1("CSRSS: Sb Complete Connection failed %lx\n",Status);
    }

    /* Return status */
    return Status;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name CsrCallServerFromServer
 * @implemented NT4
 *
 * The CsrCallServerFromServer routine calls a CSR API from within a server.
 * It avoids using LPC messages since the request isn't coming from a client.
 *
 * @param ReceiveMsg
 *        Pointer to the CSR API Message to send to the server.
 *
 * @param ReplyMsg
 *        Pointer to the CSR API Message to receive from the server.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_ILLEGAL_FUNCTION
 *         if the opcode is invalid, or STATUS_ACCESS_VIOLATION if there
 *         was a problem executing the API.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrCallServerFromServer(PCSR_API_MESSAGE ReceiveMsg,
                        PCSR_API_MESSAGE ReplyMsg)
{
    ULONG ServerId;
    PCSR_SERVER_DLL ServerDll;
    ULONG ApiId;
    ULONG Reply;
    NTSTATUS Status;

    /* Get the Server ID */
    ServerId = CSR_SERVER_ID_FROM_OPCODE(ReceiveMsg->Opcode);

    /* Make sure that the ID is within limits, and the Server DLL loaded */
    if ((ServerId >= CSR_SERVER_DLL_MAX) ||
        (!(ServerDll = CsrLoadedServerDll[ServerId])))
    {
        /* We are beyond the Maximum Server ID */
        ReplyMsg->Status = (ULONG)STATUS_ILLEGAL_FUNCTION;
        return STATUS_ILLEGAL_FUNCTION;
    }
    else
    {
        /* Get the API ID */
        ApiId = CSR_API_ID_FROM_OPCODE(ReceiveMsg->Opcode);

        /* Normalize it with our Base ID */
        ApiId -= ServerDll->ApiBase;

        /* Make sure that the ID is within limits, and the entry exists */
        if ((ApiId >= ServerDll->HighestApiSupported) ||
            (ServerDll->ValidTable && !ServerDll->ValidTable[ApiId]))
        {
            /* We are beyond the Maximum API ID, or it doesn't exist */
            ReplyMsg->Status = (ULONG)STATUS_ILLEGAL_FUNCTION;
            return STATUS_ILLEGAL_FUNCTION;
        }
    }

    /* Validation complete, start SEH */
    _SEH2_TRY
    {
        /* Call the API and get the result */
        Status = (ServerDll->DispatchTable[ApiId])(ReceiveMsg, &Reply);

        /* Return the result, no matter what it is */
        ReplyMsg->Status = Status;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* If we got an exception, return access violation */
        ReplyMsg->Status = STATUS_ACCESS_VIOLATION;
    }
    _SEH2_END;

    /* Return success */
    return STATUS_SUCCESS;
}

/*++
 * @name CsrConnectToUser
 * @implemented NT4
 *
 * The CsrConnectToUser connects to the User subsystem.
 *
 * @param None
 *
 * @return A pointer to the CSR Thread
 *
 * @remarks None.
 *
 *--*/
PCSR_THREAD
NTAPI
CsrConnectToUser(VOID)
{
    NTSTATUS Status;
    ANSI_STRING DllName;
    UNICODE_STRING TempName;
    HANDLE hUser32;
    STRING StartupName;
    PTEB Teb = NtCurrentTeb();
    PCSR_THREAD CsrThread;

    /* Check if we didn't already find it */
    if (!CsrClientThreadSetup)
    {
        /* Get the DLL Handle for user32.dll */
        RtlInitAnsiString(&DllName, "user32");
        RtlAnsiStringToUnicodeString(&TempName, &DllName, TRUE);
        Status = LdrGetDllHandle(NULL,
                                 NULL,
                                 &TempName,
                                 &hUser32);
        RtlFreeUnicodeString(&TempName);

        /* If we got teh handle, get the Client Thread Startup Entrypoint */
        if (NT_SUCCESS(Status))
        {
            RtlInitAnsiString(&StartupName,"ClientThreadSetup");
            Status = LdrGetProcedureAddress(hUser32,
                                            &StartupName,
                                            0,
                                            (PVOID)&CsrClientThreadSetup);
        }
    }

    /* Connect to user32 */
    CsrClientThreadSetup();

    /* Save pointer to this thread in TEB */
    CsrThread = CsrLocateThreadInProcess(NULL, &Teb->Cid);
    if (CsrThread) Teb->CsrClientThread = CsrThread;

    /* Return it */
    return CsrThread;
}

/*++
 * @name CsrQueryApiPort
 * @implemented NT4
 *
 * The CsrQueryApiPort routine returns a handle to the CSR API LPC port.
 *
 * @param None.
 *
 * @return A handle to the port.
 *
 * @remarks None.
 *
 *--*/
HANDLE
NTAPI
CsrQueryApiPort(VOID)
{
    DPRINT("CSRSRV: %s called\n", __FUNCTION__);
    return CsrApiPort;
}

/*++
 * @name CsrCaptureArguments
 * @implemented NT5.1
 *
 * The CsrCaptureArguments routine validates a CSR Capture Buffer and
 * re-captures it into a server CSR Capture Buffer.
 *
 * @param CsrThread
 *        Pointer to the CSR Thread performing the validation.
 *
 * @param ApiMessage
 *        Pointer to the CSR API Message containing the Capture Buffer
 *        that needs to be validated.
 *
 * @return TRUE if validation succeeded, FALSE otherwise.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
NTAPI
CsrCaptureArguments(IN PCSR_THREAD CsrThread,
                    IN PCSR_API_MESSAGE ApiMessage)
{
    PCSR_CAPTURE_BUFFER LocalCaptureBuffer = NULL;
    ULONG LocalLength = 0;
    PCSR_CAPTURE_BUFFER RemoteCaptureBuffer = NULL;
    SIZE_T BufferDistance = 0;
    ULONG PointerCount = 0;
    ULONG_PTR **PointerOffsets = NULL;
    ULONG_PTR *CurrentPointer = NULL;

    /* Use SEH to make sure this is valid */
    _SEH2_TRY
    {
        /* Get the buffer we got from whoever called NTDLL */
        LocalCaptureBuffer = ApiMessage->CsrCaptureData;
        LocalLength = LocalCaptureBuffer->Size;

        /* Now check if the buffer is inside our mapped section */
        if (((ULONG_PTR)LocalCaptureBuffer < CsrThread->Process->ClientViewBase) ||
            (((ULONG_PTR)LocalCaptureBuffer + LocalLength) >= CsrThread->Process->ClientViewBounds))
        {
            /* Return failure */
            ApiMessage->Status = STATUS_INVALID_PARAMETER;
            _SEH2_YIELD(return FALSE);
        }

        /* Check if the Length is valid */
        if (((LocalCaptureBuffer->PointerCount * 4 + sizeof(CSR_CAPTURE_BUFFER)) >
            LocalLength) ||(LocalLength > MAXWORD))
        {
            /* Return failure */
            ApiMessage->Status = STATUS_INVALID_PARAMETER;
            _SEH2_YIELD(return FALSE);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Return failure */
        ApiMessage->Status = STATUS_INVALID_PARAMETER;
        _SEH2_YIELD(return FALSE);
    } _SEH2_END;

    /* We validated the incoming buffer, now allocate the remote one */
    RemoteCaptureBuffer = RtlAllocateHeap(CsrHeap, 0, LocalLength);
    if (!RemoteCaptureBuffer)
    {
        /* We're out of memory */
        ApiMessage->Status = STATUS_NO_MEMORY;
        return FALSE;
    }

    /* Copy the client's buffer */
    RtlMoveMemory(RemoteCaptureBuffer, LocalCaptureBuffer, LocalLength);

    /* Copy the length */
    RemoteCaptureBuffer->Size = LocalLength;

    /* Calculate the difference between our buffer and the client's */
    BufferDistance = (ULONG_PTR)RemoteCaptureBuffer - (ULONG_PTR)LocalCaptureBuffer;

    /* Save the pointer count and offset pointer */
    PointerCount = RemoteCaptureBuffer->PointerCount;
    PointerOffsets = (ULONG_PTR**)(RemoteCaptureBuffer + 1);

    /* Start the loop */
    while (PointerCount)
    {
        /* Get the current pointer */
        if ((CurrentPointer = *PointerOffsets++))
        {
            /* Add it to the CSR Message structure */
            CurrentPointer += (ULONG_PTR)ApiMessage;

            /* Validate the bounds of the current pointer */
            if ((*CurrentPointer >= CsrThread->Process->ClientViewBase) &&
                (*CurrentPointer < CsrThread->Process->ClientViewBounds))
            {
                /* Modify the pointer to take into account its new position */
                *CurrentPointer += BufferDistance;
            }
            else
            {
                /* Invalid pointer, fail */
                ApiMessage->Status = (ULONG)STATUS_INVALID_PARAMETER;
            }
        }

        /* Move to the next Pointer */
        PointerCount--;
    }

    /* Check if we got success */
    if (ApiMessage->Status != STATUS_SUCCESS)
    {
        /* Failure. Free the buffer and return*/
        RtlFreeHeap(CsrHeap, 0, RemoteCaptureBuffer);
        return FALSE;
    }
    else
    {
        /* Success, save the previous buffer */
        RemoteCaptureBuffer->PreviousCaptureBuffer = LocalCaptureBuffer;
        ApiMessage->CsrCaptureData = RemoteCaptureBuffer;
    }

    /* Success */
    return TRUE;
}

/*++
 * @name CsrReleaseCapturedArguments
 * @implemented NT5.1
 *
 * The CsrReleaseCapturedArguments routine releases a Capture Buffer
 * that was previously captured with CsrCaptureArguments.
 *
 * @param ApiMessage
 *        Pointer to the CSR API Message containing the Capture Buffer
 *        that needs to be released.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
CsrReleaseCapturedArguments(IN PCSR_API_MESSAGE ApiMessage)
{
    PCSR_CAPTURE_BUFFER RemoteCaptureBuffer;
    PCSR_CAPTURE_BUFFER LocalCaptureBuffer;
    SIZE_T BufferDistance;
    ULONG PointerCount;
    ULONG_PTR **PointerOffsets;
    ULONG_PTR *CurrentPointer;

    /* Get the capture buffers */
    RemoteCaptureBuffer = ApiMessage->CsrCaptureData;
    LocalCaptureBuffer = RemoteCaptureBuffer->PreviousCaptureBuffer;

    /* Free the previous one */
    RemoteCaptureBuffer->PreviousCaptureBuffer = NULL;

    /* Find out the difference between the two buffers */
    BufferDistance = (ULONG_PTR)LocalCaptureBuffer - (ULONG_PTR)RemoteCaptureBuffer;

    /* Save the pointer count and offset pointer */
    PointerCount = RemoteCaptureBuffer->PointerCount;
    PointerOffsets = (ULONG_PTR**)(RemoteCaptureBuffer + 1);

    /* Start the loop */
    while (PointerCount)
    {
        /* Get the current pointer */
        if ((CurrentPointer = *PointerOffsets++))
        {
            /* Add it to the CSR Message structure */
            CurrentPointer += (ULONG_PTR)ApiMessage;

            /* Modify the pointer to take into account its new position */
            *CurrentPointer += BufferDistance;
        }

        /* Move to the next Pointer */
        PointerCount--;
    }

    /* Copy the data back */
    RtlMoveMemory(LocalCaptureBuffer,
                  RemoteCaptureBuffer,
                  RemoteCaptureBuffer->Size);

    /* Free our allocated buffer */
    RtlFreeHeap(CsrHeap, 0, RemoteCaptureBuffer);
}

/*++
 * @name CsrValidateMessageBuffer
 * @implemented NT5.1
 *
 * The CsrValidateMessageBuffer routine validates a captured message buffer
 * present in the CSR Api Message
 *
 * @param ApiMessage
 *        Pointer to the CSR API Message containing the CSR Capture Buffer.
 *
 * @param Buffer
 *        Pointer to the message buffer to validate.
 *
 * @param ArgumentSize
 *        Size of the message to check.
 *
 * @param ArgumentCount
 *        Number of messages to check.
 *
 * @return TRUE if validation suceeded, FALSE otherwise.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
NTAPI
CsrValidateMessageBuffer(IN PCSR_API_MESSAGE ApiMessage,
                         IN PVOID *Buffer,
                         IN ULONG ArgumentSize,
                         IN ULONG ArgumentCount)
{
    PCSR_CAPTURE_BUFFER CaptureBuffer = ApiMessage->CsrCaptureData;
    SIZE_T BufferDistance;
    ULONG PointerCount;
    ULONG_PTR **PointerOffsets;
    ULONG_PTR *CurrentPointer;
    ULONG i;

    /* Make sure there are some arguments */
    if (!ArgumentCount) return FALSE;

    /* Check if didn't get a buffer and there aren't any arguments to check */
    if (!(*Buffer) && (!(ArgumentCount * ArgumentSize))) return TRUE;

    /* Check if we have no capture buffer */
    if (!CaptureBuffer)
    {
        /* In this case, check only the Process ID */
        if (NtCurrentTeb()->Cid.UniqueProcess ==
            ApiMessage->Header.ClientId.UniqueProcess)
        {
            /* There is a match, validation succeeded */
            return TRUE;
        }
    }
    else
    {
        /* Make sure that there is still space left in the buffer */
        if ((CaptureBuffer->Size - (ULONG_PTR)*Buffer + (ULONG_PTR)CaptureBuffer) <
            (ArgumentCount * ArgumentSize))
        {
            /* Find out the difference between the two buffers */
            BufferDistance = (ULONG_PTR)Buffer - (ULONG_PTR)ApiMessage;

             /* Save the pointer count */
            PointerCount = CaptureBuffer->PointerCount;
            PointerOffsets = (ULONG_PTR**)(CaptureBuffer + 1);

            /* Start the loop */
            for (i = 0; i < PointerCount; i++)
            {
                /* Get the current pointer */
                CurrentPointer = *PointerOffsets++;

                /* Check if its' equal to the difference */
                if (*CurrentPointer == BufferDistance) return TRUE;
            }
        }
    }

    /* Failure */
    return FALSE;
}

/*++
 * @name CsrValidateMessageString
 * @implemented NT5.1
 *
 * The CsrValidateMessageString validates a captured Wide-Character String
 * present in a CSR API Message.
 *
 * @param ApiMessage
 *        Pointer to the CSR API Message containing the CSR Capture Buffer.
 *
 * @param MessageString
 *        Pointer to the buffer containing the string to validate.
 *
 * @return TRUE if validation suceeded, FALSE otherwise.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
NTAPI
CsrValidateMessageString(IN PCSR_API_MESSAGE ApiMessage,
                         IN LPWSTR *MessageString)
{
    DPRINT("CSRSRV: %s called\n", __FUNCTION__);
    return FALSE;
}

/* EOF */
