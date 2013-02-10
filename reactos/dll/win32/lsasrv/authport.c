/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/lsasrv/authport.c
 * PURPOSE:     LsaAuthenticationPort server routines
 * COPYRIGHT:   Copyright 2009 Eric Kohl
 */

/* INCLUDES ****************************************************************/


#include "lsasrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(lsasrv);


static HANDLE PortThreadHandle = NULL;
static HANDLE AuthPortHandle = NULL;


/* FUNCTIONS ***************************************************************/

static NTSTATUS
LsapCallAuthenticationPackage(PLSA_API_MSG RequestMsg)
{
    TRACE("(%p)\n", RequestMsg);

    return STATUS_SUCCESS;
}


static NTSTATUS
LsapDeregisterLogonProcess(PLSA_API_MSG RequestMsg)
{
    TRACE("(%p)\n", RequestMsg);

    return STATUS_SUCCESS;
}


static NTSTATUS
LsapLogonUser(PLSA_API_MSG RequestMsg)
{
    TRACE("(%p)\n", RequestMsg);

    return STATUS_SUCCESS;
}


static NTSTATUS
LsapLookupAuthenticationPackage(PLSA_API_MSG RequestMsg)
{
    TRACE("(%p)\n", RequestMsg);

    TRACE("PackageName: %s\n", RequestMsg->LookupAuthenticationPackage.Request.PackageName);

    RequestMsg->LookupAuthenticationPackage.Reply.Package = 0x12345678;

    return STATUS_SUCCESS;
}


NTSTATUS WINAPI
AuthPortThreadRoutine(PVOID Param)
{
    PLSA_API_MSG ReplyMsg = NULL;
    LSA_API_MSG RequestMsg;
    NTSTATUS Status;

    HANDLE ConnectionHandle = NULL;
    PVOID Context = NULL;
    BOOLEAN Accept;
    REMOTE_PORT_VIEW RemotePortView;

    TRACE("AuthPortThreadRoutine() called\n");

    Status = STATUS_SUCCESS;

    for (;;)
    {
        Status = NtReplyWaitReceivePort(AuthPortHandle,
                                        0,
                                        &ReplyMsg->h,
                                        &RequestMsg.h);
        if (!NT_SUCCESS(Status))
        {
            TRACE("NtReplyWaitReceivePort() failed (Status %lx)\n", Status);
            break;
        }

        TRACE("Received message\n");

        switch (RequestMsg.h.u2.s2.Type)
        {
            case LPC_CONNECTION_REQUEST:
                TRACE("Port connection request\n");

                RemotePortView.Length = sizeof(REMOTE_PORT_VIEW);

                TRACE("Logon Process Name: %s\n", RequestMsg.ConnectInfo.LogonProcessNameBuffer);

                RequestMsg.ConnectInfo.OperationalMode = 0x43218765;
                RequestMsg.ConnectInfo.Status = STATUS_SUCCESS;

                Accept = TRUE;
                Status = NtAcceptConnectPort(&ConnectionHandle,
                                             &Context,
                                             &RequestMsg.h,
                                             Accept,
                                             NULL,
                                             &RemotePortView);
                if (!NT_SUCCESS(Status))
                {
                    ERR("NtAcceptConnectPort failed (Status 0x%lx)\n", Status);
                    return Status;
                }

                Status = NtCompleteConnectPort(ConnectionHandle);
                if (!NT_SUCCESS(Status))
                {
                    ERR("NtCompleteConnectPort failed (Status 0x%lx)\n", Status);
                    return Status;
                }

                ReplyMsg = NULL;
                break;

            case LPC_PORT_CLOSED:
                TRACE("Port closed\n");
                ReplyMsg = NULL;
                break;

            case LPC_CLIENT_DIED:
                TRACE("Client died\n");
                ReplyMsg = NULL;
                break;

            default:
                TRACE("Received request (ApiNumber: %lu)\n", RequestMsg.ApiNumber);

                switch (RequestMsg.ApiNumber)
                {
                    case LSASS_REQUEST_CALL_AUTHENTICATION_PACKAGE:
                        RequestMsg.Status = LsapCallAuthenticationPackage(&RequestMsg);
                        break;

                    case LSASS_REQUEST_DEREGISTER_LOGON_PROCESS:
                        RequestMsg.Status = LsapDeregisterLogonProcess(&RequestMsg);
                        break;

                    case LSASS_REQUEST_LOGON_USER:
                        RequestMsg.Status = LsapLogonUser(&RequestMsg);
                        break;

                    case LSASS_REQUEST_LOOKUP_AUTHENTICATION_PACKAGE:
                        RequestMsg.Status = LsapLookupAuthenticationPackage(&RequestMsg);
                        break;

                    default:
                        RequestMsg.Status = STATUS_SUCCESS; /* FIXME */
                        break;
                }

                ReplyMsg = &RequestMsg;
                break;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
StartAuthenticationPort(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING PortName;
    DWORD ThreadId;
    NTSTATUS Status;

    RtlInitUnicodeString(&PortName,
                         L"\\LsaAuthenticationPort");

    InitializeObjectAttributes(&ObjectAttributes,
                               &PortName,
                               0,
                               NULL,
                               NULL);

    Status = NtCreatePort(&AuthPortHandle,
                          &ObjectAttributes,
                          sizeof(LSA_CONNECTION_INFO),
                          sizeof(LSA_API_MSG),
                          sizeof(LSA_API_MSG) * 32);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtCreatePort() failed (Status %lx)\n", Status);
        return Status;
    }

    PortThreadHandle = CreateThread(NULL,
                                    0x1000,
                                    (LPTHREAD_START_ROUTINE)AuthPortThreadRoutine,
                                    NULL,
                                    0,
                                    &ThreadId);


    return STATUS_SUCCESS;
}

/* EOF */
