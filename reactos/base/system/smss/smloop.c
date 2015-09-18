/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smloop.c
 * PURPOSE:         Main SMSS Code
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "smss.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

typedef struct _SMP_CLIENT_CONTEXT
{
    PVOID Subsystem;
    HANDLE ProcessHandle;
    HANDLE PortHandle;
    ULONG dword10;
} SMP_CLIENT_CONTEXT, *PSMP_CLIENT_CONTEXT;

typedef
NTSTATUS
(NTAPI *PSM_API_HANDLER)(
     IN PSM_API_MSG SmApiMsg,
     IN PSMP_CLIENT_CONTEXT ClientContext,
     IN HANDLE SmApiPort
);

volatile LONG SmTotalApiThreads;
HANDLE SmUniqueProcessId;

/* API HANDLERS ***************************************************************/

NTSTATUS
NTAPI
SmpCreateForeignSession(IN PSM_API_MSG SmApiMsg,
                        IN PSMP_CLIENT_CONTEXT ClientContext,
                        IN HANDLE SmApiPort)
{
    DPRINT1("%s is not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
SmpSessionComplete(IN PSM_API_MSG SmApiMsg,
                   IN PSMP_CLIENT_CONTEXT ClientContext,
                   IN HANDLE SmApiPort)
{
    DPRINT1("%s is not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
SmpTerminateForeignSession(IN PSM_API_MSG SmApiMsg,
                           IN PSMP_CLIENT_CONTEXT ClientContext,
                           IN HANDLE SmApiPort)
{
    DPRINT1("%s is not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
SmpExecPgm(IN PSM_API_MSG SmApiMsg,
           IN PSMP_CLIENT_CONTEXT ClientContext,
           IN HANDLE SmApiPort)
{
    HANDLE ProcessHandle;
    NTSTATUS Status;
    PSM_EXEC_PGM_MSG SmExecPgm;
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* Open the client process */
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = NtOpenProcess(&ProcessHandle,
                           PROCESS_DUP_HANDLE,
                           &ObjectAttributes,
                           &SmApiMsg->h.ClientId);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        DPRINT1("SmExecPgm: NtOpenProcess Failed %lx\n", Status);
        return Status;
    }

    /* Copy the process information out of the message */
    SmExecPgm = &SmApiMsg->u.ExecPgm;
    ProcessInformation = SmExecPgm->ProcessInformation;

    /* Duplicate the process handle */
    Status = NtDuplicateObject(ProcessHandle,
                               SmExecPgm->ProcessInformation.ProcessHandle,
                               NtCurrentProcess(),
                               &ProcessInformation.ProcessHandle,
                               PROCESS_ALL_ACCESS,
                               0,
                               0);
    if (!NT_SUCCESS(Status))
    {
        /* Close the handle and fail */
        NtClose(ProcessHandle);
        DPRINT1("SmExecPgm: NtDuplicateObject (Process) Failed %lx\n", Status);
        return Status;
    }

    /* Duplicate the thread handle */
    Status = NtDuplicateObject(ProcessHandle,
                               SmExecPgm->ProcessInformation.ThreadHandle,
                               NtCurrentProcess(),
                               &ProcessInformation.ThreadHandle,
                               THREAD_ALL_ACCESS,
                               0,
                               0);
    if (!NT_SUCCESS(Status))
    {
        /* Close both handles and fail */
        NtClose(ProcessInformation.ProcessHandle);
        NtClose(ProcessHandle);
        DPRINT1("SmExecPgm: NtDuplicateObject (Thread) Failed %lx\n", Status);
        return Status;
    }

    /* Close the process handle and call the internal client API */
    NtClose(ProcessHandle);
    return SmpSbCreateSession(NULL,
                              NULL,
                              &ProcessInformation,
                              0,
                              SmExecPgm->DebugFlag ? &SmApiMsg->h.ClientId : NULL);
}

NTSTATUS
NTAPI
SmpLoadDeferedSubsystem(IN PSM_API_MSG SmApiMsg,
                        IN PSMP_CLIENT_CONTEXT ClientContext,
                        IN HANDLE SmApiPort)
{
    DPRINT1("%s is not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
SmpStartCsr(IN PSM_API_MSG SmApiMsg,
            IN PSMP_CLIENT_CONTEXT ClientContext,
            IN HANDLE SmApiPort)
{
    DPRINT1("%s is not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
SmpStopCsr(IN PSM_API_MSG SmApiMsg,
           IN PSMP_CLIENT_CONTEXT ClientContext,
           IN HANDLE SmApiPort)
{
    DPRINT1("%s is not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

PSM_API_HANDLER SmpApiDispatch[SmpMaxApiNumber - SmpCreateForeignSessionApi] =
{
    SmpCreateForeignSession,
    SmpSessionComplete,
    SmpTerminateForeignSession,
    SmpExecPgm,
    SmpLoadDeferedSubsystem,
    SmpStartCsr,
    SmpStopCsr
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
SmpHandleConnectionRequest(IN HANDLE SmApiPort,
                           IN PSB_API_MSG SbApiMsg)
{
    BOOLEAN Accept = TRUE;
    HANDLE PortHandle, ProcessHandle;
    ULONG SessionId;
    UNICODE_STRING SubsystemPort;
    SMP_CLIENT_CONTEXT *ClientContext;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    REMOTE_PORT_VIEW PortView;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    PSMP_SUBSYSTEM CidSubsystem, TypeSubsystem;

    /* Initialize QoS data */
    SecurityQos.ImpersonationLevel = SecurityIdentification;
    SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQos.EffectiveOnly = TRUE;

    /* Check if this is SM connecting to itself */
    if (SbApiMsg->h.ClientId.UniqueProcess == SmUniqueProcessId)
    {
        /* No need to get any handle -- assume session 0 */
        ProcessHandle = NULL;
        SessionId = 0;
    }
    else
    {
        /* Reference the foreign process */
        InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
        Status = NtOpenProcess(&ProcessHandle,
                               PROCESS_QUERY_INFORMATION,
                               &ObjectAttributes,
                               &SbApiMsg->h.ClientId);
        if (!NT_SUCCESS(Status)) Accept = FALSE;

        /* Get its session ID */
        SmpGetProcessMuSessionId(ProcessHandle, &SessionId);
    }

    /* See if we already know about the caller's subystem */
    CidSubsystem = SmpLocateKnownSubSysByCid(&SbApiMsg->h.ClientId);
    if ((CidSubsystem) && (Accept))
    {
        /* Check if we already have a subsystem for this kind of image */
        TypeSubsystem = SmpLocateKnownSubSysByType(SessionId,
                                                   SbApiMsg->ConnectionInfo.SubsystemType);
        if (TypeSubsystem == CidSubsystem)
        {
            /* Someone is trying to take control of an existing subsystem, fail */
            Accept = FALSE;
            DPRINT1("SMSS: Connection from SubSystem rejected\n");
            DPRINT1("SMSS: Image type already being served\n");
        }
        else
        {
            /* Set this image type as the type for this subsystem */
            CidSubsystem->ImageType = SbApiMsg->ConnectionInfo.SubsystemType;
        }

        /* Drop the reference we had acquired */
        if (TypeSubsystem) SmpDereferenceSubsystem(TypeSubsystem);
    }

    /* Check if we'll be accepting the connection */
    if (Accept)
    {
        /* We will, so create a client context for it */
        ClientContext = RtlAllocateHeap(SmpHeap, 0, sizeof(SMP_CLIENT_CONTEXT));
        if (ClientContext)
        {
            ClientContext->ProcessHandle = ProcessHandle;
            ClientContext->Subsystem = CidSubsystem;
            ClientContext->dword10 = 0;
            ClientContext->PortHandle = NULL;
        }
        else
        {
            /* Failed to allocate a client context, so reject the connection */
            DPRINT1("Rejecting connectiond due to lack of memory\n");
            Accept = FALSE;
        }
    }
    else
    {
        /* Use a bogus context since we're going to reject the message */
        ClientContext = (PSMP_CLIENT_CONTEXT)SbApiMsg;
    }

    /* Now send the actual accept reply (which could be a rejection) */
    PortView.Length = sizeof(PortView);
    Status = NtAcceptConnectPort(&PortHandle,
                                 ClientContext,
                                 &SbApiMsg->h,
                                 Accept,
                                 NULL,
                                 &PortView);
    if (!(Accept) || !(NT_SUCCESS(Status)))
    {
        /* Close the process handle, reference the subsystem, and exit */
        DPRINT1("Accept failed or rejected: %lx\n", Status);
        if (ClientContext != (PVOID)SbApiMsg) RtlFreeHeap(SmpHeap, 0, ClientContext);
        if (ProcessHandle) NtClose(ProcessHandle);
        if (CidSubsystem) SmpDereferenceSubsystem(CidSubsystem);
        return Status;
    }

    /* Save the port handle now that we've accepted it */
    if (ClientContext) ClientContext->PortHandle = PortHandle;
    if (CidSubsystem) CidSubsystem->PortHandle = PortHandle;

    /* Complete the port connection */
    Status = NtCompleteConnectPort(PortHandle);
    if ((NT_SUCCESS(Status)) && (CidSubsystem))
    {
        /* This was an actual subsystem, so connect back to it */
        SbApiMsg->ConnectionInfo.SbApiPortName[119] = UNICODE_NULL;
        RtlCreateUnicodeString(&SubsystemPort,
                               SbApiMsg->ConnectionInfo.SbApiPortName);
        Status = NtConnectPort(&CidSubsystem->SbApiPort,
                               &SubsystemPort,
                               &SecurityQos,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("SMSS: Connect back to Sb %wZ failed %lx\n", &SubsystemPort, Status);
        }
        RtlFreeUnicodeString(&SubsystemPort);

        /* Now that we're connected, signal the event handle */
        NtSetEvent(CidSubsystem->Event, NULL);
    }
    else if (CidSubsystem)
    {
        /* We failed to complete the connection, so clear the port handle */
        DPRINT1("Completing the connection failed: %lx\n", Status);
        CidSubsystem->PortHandle = NULL;
    }

    /* Dereference the subsystem and return the result */
    if (CidSubsystem) SmpDereferenceSubsystem(CidSubsystem);
    return Status;
}

ULONG
NTAPI
SmpApiLoop(IN PVOID Parameter)
{
    HANDLE SmApiPort = (HANDLE)Parameter;
    NTSTATUS Status;
    PSMP_CLIENT_CONTEXT ClientContext;
    PSM_API_MSG ReplyMsg = NULL;
    SM_API_MSG RequestMsg;
    PROCESS_BASIC_INFORMATION ProcessInformation;
    LARGE_INTEGER Timeout;

    /* Increase the number of API threads for throttling code for later */
    _InterlockedExchangeAdd(&SmTotalApiThreads, 1);

    /* Mark us critical */
    RtlSetThreadIsCritical(TRUE, NULL, TRUE);

    /* Set the PID of the SM process itself for later checking */
    NtQueryInformationProcess(NtCurrentProcess(),
                              ProcessBasicInformation,
                              &ProcessInformation,
                              sizeof(ProcessInformation),
                              NULL);
    SmUniqueProcessId = (HANDLE)ProcessInformation.UniqueProcessId;

    /* Now process incoming messages */
    while (TRUE)
    {
        /* Begin waiting on a request */
        Status = NtReplyWaitReceivePort(SmApiPort,
                                        (PVOID*)&ClientContext,
                                        &ReplyMsg->h,
                                        &RequestMsg.h);
        if (Status == STATUS_NO_MEMORY)
        {
            /* Ran out of memory, so do a little timeout and try again */
            if (ReplyMsg) DPRINT1("SMSS: Failed to reply to calling thread, retrying.\n");
            Timeout.QuadPart = -50000000;
            NtDelayExecution(FALSE, &Timeout);
            continue;
        }

        /* Check what kind of request we received */
        switch (RequestMsg.h.u2.s2.Type)
        {
            /* A new connection */
            case LPC_CONNECTION_REQUEST:
                /* Create the right structures for it */
                SmpHandleConnectionRequest(SmApiPort, (PSB_API_MSG)&RequestMsg);
                ReplyMsg =  NULL;
                break;

            /* A closed connection */
            case LPC_PORT_CLOSED:
                /* Destroy any state we had for this client */
                DPRINT1("Port closed\n");
                //if (ClientContext) SmpPushDeferredClientContext(ClientContext);
                ReplyMsg = NULL;
                break;

            /* An actual API message */
            default:
                if (!ClientContext)
                {
                    ReplyMsg = NULL;
                    break;
                }

                RequestMsg.ReturnValue = STATUS_PENDING;

                /* Check if the API is valid */
                if (RequestMsg.ApiNumber >= SmpMaxApiNumber)
                {
                    /* It isn't, fail */
                    DPRINT1("Invalid API: %lx\n", RequestMsg.ApiNumber);
                    Status = STATUS_NOT_IMPLEMENTED;
                }
                else if ((RequestMsg.ApiNumber <= SmpTerminateForeignSessionApi) &&
                         !(ClientContext->Subsystem))
                {
                    /* It's valid, but doesn't have a subsystem with it */
                    DPRINT1("Invalid session API\n");
                    Status = STATUS_INVALID_PARAMETER;
                }
                else
                {
                    /* It's totally okay, so call the dispatcher for it */
                    Status = SmpApiDispatch[RequestMsg.ApiNumber](&RequestMsg,
                                                                  ClientContext,
                                                                  SmApiPort);
                }

                /* Write the result valud and return the message back */
                RequestMsg.ReturnValue = Status;
                ReplyMsg = &RequestMsg;
                break;
        }
    }
    return STATUS_SUCCESS;
}
