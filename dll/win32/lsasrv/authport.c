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

NTSTATUS WINAPI
AuthPortThreadRoutine(PVOID Param)
{
    LSASS_REQUEST Request;
    PPORT_MESSAGE Reply = NULL;
    NTSTATUS Status;

    HANDLE ConnectionHandle = NULL;
    PVOID Context = NULL;
    BOOLEAN Accept;

    TRACE("AuthPortThreadRoutine() called\n");

    Status = STATUS_SUCCESS;

    for (;;)
    {
        Status = NtReplyWaitReceivePort(AuthPortHandle,
                                        0,
                                        Reply,
                                        &Request.Header);
        if (!NT_SUCCESS(Status))
        {
            TRACE("NtReplyWaitReceivePort() failed (Status %lx)\n", Status);
            break;
        }

        TRACE("Received message\n");

        if (Request.Header.u2.s2.Type == LPC_CONNECTION_REQUEST)
        {
            TRACE("Port connection request\n");

            Accept = TRUE;
            NtAcceptConnectPort(&ConnectionHandle,
                                &Context,
                                &Request.Header,
                                Accept,
                                NULL,
                                NULL);


            NtCompleteConnectPort(ConnectionHandle);

        }
        else if (Request.Header.u2.s2.Type == LPC_PORT_CLOSED ||
                 Request.Header.u2.s2.Type == LPC_CLIENT_DIED)
        {
            TRACE("Port closed or client died request\n");

//            return STATUS_UNSUCCESSFUL;
        }
        else if (Request.Header.u2.s2.Type == LPC_REQUEST)
        {
            TRACE("Received request (Type: %lu)\n", Request.Type);

        }
        else if (Request.Header.u2.s2.Type == LPC_DATAGRAM)
        {
            TRACE("Received datagram\n");

        }
    }

    return Status;
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
                          0,
                          0x100,
                          0x2000);
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
