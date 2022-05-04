/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     Local Security Authority Server DLL
 * FILE:        dll/win32/lsasrv/srm.c
 * PURPOSE:     Security Reference Monitor Server
 *
 * PROGRAMMERS: Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ****************************************************************/

#include "lsasrv.h"
#include <ndk/ntndk.h>

/* GLOBALS *****************************************************************/

HANDLE SeLsaCommandPort;
HANDLE SeRmCommandPort;

/* FUNCTIONS ***************************************************************/

static
VOID
LsapComponentTest(
    PLSAP_RM_API_MESSAGE Message)
{
    ERR("Security: LSA Component Test Command Received\n");
}

static
VOID
LsapAdtWriteLog(
    PLSAP_RM_API_MESSAGE Message)
{
    ERR("LsapAdtWriteLog\n");
}

static
VOID
LsapAsync(
    PLSAP_RM_API_MESSAGE Message)
{
    ERR("LsapAsync\n");
}

static
DWORD
WINAPI
LsapRmServerThread(
    PVOID StartContext)
{
    LSAP_RM_API_MESSAGE Message;
    PPORT_MESSAGE ReplyMessage;
    REMOTE_PORT_VIEW RemotePortView;
    HANDLE MessagePort, DummyPortHandle;
    NTSTATUS Status;

    /* Initialize the port message */
    Message.Header.u1.s1.TotalLength = sizeof(Message);
    Message.Header.u1.s1.DataLength = 0;

    /* Listen on the LSA command port */
    Status = NtListenPort(SeLsaCommandPort, &Message.Header);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapRmServerThread - Port Listen failed 0x%lx\n", Status);
        return Status;
    }

    /* Setup the Port View Structure */
    RemotePortView.Length = sizeof(REMOTE_PORT_VIEW);
    RemotePortView.ViewSize = 0;
    RemotePortView.ViewBase = NULL;

    /* Accept the connection */
    Status = NtAcceptConnectPort(&MessagePort,
                                 0,
                                 &Message.Header,
                                 TRUE,
                                 NULL,
                                 &RemotePortView);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapRmServerThread - Port Accept Connect failed 0x%lx\n", Status);
        return Status;
    }

    /* Complete the connection */
    Status = NtCompleteConnectPort(MessagePort);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapRmServerThread - Port Complete Connect failed 0x%lx\n", Status);
        return Status;
    }

    /* No reply yet */
    ReplyMessage = NULL;

    /* Start looping */
    while (TRUE)
    {
        /* Wait for a message */
        Status = NtReplyWaitReceivePort(MessagePort,
                                        NULL,
                                        ReplyMessage,
                                        &Message.Header);
        if (!NT_SUCCESS(Status))
        {
            ERR("LsapRmServerThread - Failed to get message: 0x%lx\n", Status);
            ReplyMessage = NULL;
            continue;
        }

        /* Check if this is a connection request */
        if (Message.Header.u2.s2.Type == LPC_CONNECTION_REQUEST)
        {
            /* Reject connection request */
            NtAcceptConnectPort(&DummyPortHandle,
                                NULL,
                                &Message.Header,
                                FALSE,
                                NULL,
                                NULL);

            /* Start over */
            ReplyMessage = NULL;
            continue;
        }

        /* Check if this is an actual request */
        if (Message.Header.u2.s2.Type == LPC_REQUEST)
        {
            ReplyMessage = &Message.Header;

            switch (Message.ApiNumber)
            {
                case LsapAdtWriteLogApi:
                    LsapAdtWriteLog(&Message);
                    break;

                case LsapAsyncApi:
                    LsapAsync(&Message);
                    break;

                case LsapComponentTestApi:
                    LsapComponentTest(&Message);
                    break;

                default:
                    ERR("LsapRmServerThread - invalid API number: 0x%lx\n",
                        Message.ApiNumber);
                    ReplyMessage = NULL;
            }

            continue;
        }

        ERR("LsapRmServerThread - unexpected message type: 0x%lx\n",
            Message.Header.u2.s2.Type);

        /* Start over */
        ReplyMessage = NULL;
    }
}

NTSTATUS
LsapRmInitializeServer(VOID)
{
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    HANDLE InitEvent;
    HANDLE ThreadHandle;
    DWORD ThreadId;
    NTSTATUS Status;

    /* Create the LSA command port */
    RtlInitUnicodeString(&Name, L"\\SeLsaCommandPort");
    InitializeObjectAttributes(&ObjectAttributes, &Name, 0, NULL, NULL);
    Status = NtCreatePort(&SeLsaCommandPort,
                          &ObjectAttributes,
                          0,
                          PORT_MAXIMUM_MESSAGE_LENGTH,
                          2 * PAGE_SIZE);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapRmInitializeServer - Port Create failed 0x%lx\n", Status);
        return Status;
    }

    /* Open the LSA init event */
    RtlInitUnicodeString(&Name, L"\\SeLsaInitEvent");
    InitializeObjectAttributes(&ObjectAttributes, &Name, 0, NULL, NULL);
    Status = NtOpenEvent(&InitEvent, 2, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapRmInitializeServer - Lsa Init Event Open failed 0x%lx\n", Status);
        return Status;
    }

    /* Signal the kernel, that we are ready */
    Status = NtSetEvent(InitEvent, 0);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapRmInitializeServer - Set Init Event failed 0x%lx\n", Status);
        return Status;
    }

    /* Setup the QoS structure */
    SecurityQos.ImpersonationLevel = SecurityIdentification;
    SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQos.EffectiveOnly = TRUE;

    /* Connect to the kernel server */
    RtlInitUnicodeString(&Name, L"\\SeRmCommandPort");
    Status = NtConnectPort(&SeRmCommandPort,
                           &Name,
                           &SecurityQos,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapRmInitializeServer - Connect to Rm Command Port failed 0x%lx\n", Status);
        return Status;
    }

    /* Create the server thread */
    ThreadHandle = CreateThread(NULL, 0, LsapRmServerThread, NULL, 0, &ThreadId);
    if (ThreadHandle == NULL)
    {
        ERR("LsapRmInitializeServer - Create Thread  failed 0x%lx\n", Status);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Close the server thread handle */
    CloseHandle(ThreadHandle);

    return STATUS_SUCCESS;
}

NTSTATUS
LsapRmCreateLogonSession(
    PLUID LogonId)
{
    SEP_RM_API_MESSAGE RequestMessage;
    SEP_RM_API_MESSAGE ReplyMessage;
    NTSTATUS Status;

    TRACE("LsapRmCreateLogonSession(%p)\n", LogonId);

    RequestMessage.Header.u2.ZeroInit = 0;
    RequestMessage.Header.u1.s1.TotalLength =
        (CSHORT)(sizeof(PORT_MESSAGE) + sizeof(ULONG) + sizeof(LUID));
    RequestMessage.Header.u1.s1.DataLength =
        RequestMessage.Header.u1.s1.TotalLength -
        (CSHORT)sizeof(PORT_MESSAGE);

    RequestMessage.ApiNumber = (ULONG)RmCreateLogonSession;
    RtlCopyLuid(&RequestMessage.u.LogonLuid, LogonId);

    ReplyMessage.Header.u2.ZeroInit = 0;
    ReplyMessage.Header.u1.s1.TotalLength =
        (CSHORT)(sizeof(PORT_MESSAGE) + sizeof(ULONG) + sizeof(NTSTATUS));
    ReplyMessage.Header.u1.s1.DataLength =
        ReplyMessage.Header.u1.s1.TotalLength -
        (CSHORT)sizeof(PORT_MESSAGE);

    ReplyMessage.u.ResultStatus = STATUS_SUCCESS;

    Status = NtRequestWaitReplyPort(SeRmCommandPort,
                                    (PPORT_MESSAGE)&RequestMessage,
                                    (PPORT_MESSAGE)&ReplyMessage);
    if (NT_SUCCESS(Status))
    {
        Status = ReplyMessage.u.ResultStatus;
    }

    return Status;
}

NTSTATUS
LsapRmDeleteLogonSession(
    PLUID LogonId)
{
    SEP_RM_API_MESSAGE RequestMessage;
    SEP_RM_API_MESSAGE ReplyMessage;
    NTSTATUS Status;

    TRACE("LsapRmDeleteLogonSession(%p)\n", LogonId);

    RequestMessage.Header.u2.ZeroInit = 0;
    RequestMessage.Header.u1.s1.TotalLength =
        (CSHORT)(sizeof(PORT_MESSAGE) + sizeof(ULONG) + sizeof(LUID));
    RequestMessage.Header.u1.s1.DataLength =
        RequestMessage.Header.u1.s1.TotalLength -
        (CSHORT)sizeof(PORT_MESSAGE);

    RequestMessage.ApiNumber = (ULONG)RmDeleteLogonSession;
    RtlCopyLuid(&RequestMessage.u.LogonLuid, LogonId);

    ReplyMessage.Header.u2.ZeroInit = 0;
    ReplyMessage.Header.u1.s1.TotalLength =
        (CSHORT)(sizeof(PORT_MESSAGE) + sizeof(ULONG) + sizeof(NTSTATUS));
    ReplyMessage.Header.u1.s1.DataLength =
        ReplyMessage.Header.u1.s1.TotalLength -
        (CSHORT)sizeof(PORT_MESSAGE);

    ReplyMessage.u.ResultStatus = STATUS_SUCCESS;

    Status = NtRequestWaitReplyPort(SeRmCommandPort,
                                    (PPORT_MESSAGE)&RequestMessage,
                                    (PPORT_MESSAGE)&ReplyMessage);
    if (NT_SUCCESS(Status))
    {
        Status = ReplyMessage.u.ResultStatus;
    }

    return Status;
}
