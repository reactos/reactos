/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/eventlog/logport.c
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2002 Eric Kohl
 *                   Copyright 2005 Saveliy Tretiakov
 */

/* INCLUDES *****************************************************************/

#include "eventlog.h"

/* GLOBALS ******************************************************************/

HANDLE ConnectPortHandle = NULL;
HANDLE MessagePortHandle = NULL;
extern BOOL onLiveCD;

/* FUNCTIONS ****************************************************************/

NTSTATUS WINAPI PortThreadRoutine(PVOID Param)
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = InitLogPort();
    if (!NT_SUCCESS(Status))
        return Status;

    while (NT_SUCCESS(Status))
        Status = ProcessPortMessage();

    if (ConnectPortHandle != NULL)
        NtClose(ConnectPortHandle);

    if (MessagePortHandle != NULL)
        NtClose(MessagePortHandle);

    return Status;
}

NTSTATUS InitLogPort(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING PortName;
    PORT_MESSAGE Request;
    NTSTATUS Status;

    ConnectPortHandle = NULL;
    MessagePortHandle = NULL;

    RtlInitUnicodeString(&PortName, L"\\ErrorLogPort");
    InitializeObjectAttributes(&ObjectAttributes, &PortName, 0, NULL, NULL);

    Status = NtCreatePort(&ConnectPortHandle,
                          &ObjectAttributes,
                          0,
                          0x100,
                          0x2000);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreatePort() failed (Status %lx)\n", Status);
        goto ByeBye;
    }

    Status = NtListenPort(ConnectPortHandle, &Request);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtListenPort() failed (Status %lx)\n", Status);
        goto ByeBye;
    }

    Status = NtAcceptConnectPort(&MessagePortHandle, ConnectPortHandle,
                                 NULL, TRUE, NULL, NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtAcceptConnectPort() failed (Status %lx)\n", Status);
        goto ByeBye;
    }

    Status = NtCompleteConnectPort(MessagePortHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCompleteConnectPort() failed (Status %lx)\n", Status);
        goto ByeBye;
    }

  ByeBye:
    if (!NT_SUCCESS(Status))
    {
        if (ConnectPortHandle != NULL)
            NtClose(ConnectPortHandle);

        if (MessagePortHandle != NULL)
            NtClose(MessagePortHandle);
    }
    return Status;
}

NTSTATUS ProcessPortMessage(VOID)
{
    IO_ERROR_LPC Request;
    PIO_ERROR_LOG_MESSAGE Message;
    PEVENTLOGRECORD pRec;
    ULONG ulRecNum;
    DWORD dwRecSize;
    NTSTATUS Status;
    PLOGFILE SystemLog = NULL;

    DPRINT("ProcessPortMessage() called\n");

    SystemLog = LogfListItemByName(L"System");

    while (TRUE)
    {
        Status = NtReplyWaitReceivePort(MessagePortHandle,
                                        0,
                                        NULL,
                                        &Request.Header);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtReplyWaitReceivePort() failed (Status %lx)\n", Status);
            break;
        }

        DPRINT("Received message\n");

        if (Request.Header.u2.s2.Type == LPC_PORT_CLOSED)
        {
            DPRINT("Port closed\n");
            return STATUS_SUCCESS;
        }

        if (Request.Header.u2.s2.Type == LPC_REQUEST)
        {
            DPRINT("Received request\n");
        }
        else if (Request.Header.u2.s2.Type == LPC_DATAGRAM)
        {
            DPRINT("Received datagram\n");
            Message = (PIO_ERROR_LOG_MESSAGE) & Request.Message;
            ulRecNum = SystemLog ? SystemLog->Header.NextRecord : 0;

            pRec = (PEVENTLOGRECORD) LogfAllocAndBuildNewRecord(&dwRecSize,
                    ulRecNum, Message->Type, Message->EntryData.EventCategory,
                    Message->EntryData.ErrorCode,
                    (WCHAR *) (((PBYTE) Message) + Message->DriverNameOffset),
                    L"MyComputer",  /* FIXME */
                    0,
                    NULL,
                    Message->EntryData.NumberOfStrings,
                    (WCHAR *) (((PBYTE) Message) + Message->EntryData.StringOffset),
                    Message->EntryData.DumpDataSize,
                    (LPVOID) (((PBYTE) Message) + sizeof(IO_ERROR_LOG_PACKET) -
                        sizeof(ULONG)));

            if (pRec == NULL)
            {
                DPRINT("LogfAllocAndBuildNewRecord failed!\n");
                return STATUS_NO_MEMORY;
            }

            DPRINT("dwRecSize = %d\n", dwRecSize);

            DPRINT("\n --- EVENTLOG RECORD ---\n");
            PRINT_RECORD(pRec);
            DPRINT("\n");

            if (!onLiveCD && SystemLog)
            {
                if (!LogfWriteData(SystemLog, dwRecSize, (PBYTE) pRec))
                    DPRINT("LogfWriteData failed!\n");
                else
                    DPRINT("Data written to Log!\n");
            }

            LogfFreeRecord(pRec);
        }
    }

    return Status;
}
