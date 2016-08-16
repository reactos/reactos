/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/eventlog/logport.c
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2002 Eric Kohl
 *                   Copyright 2005 Saveliy Tretiakov
 */

/* INCLUDES *****************************************************************/

#include "eventlog.h"

#include <ndk/lpcfuncs.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

static HANDLE ConnectPortHandle = NULL;
static HANDLE MessagePortHandle = NULL;
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
    ULONG Time;
    PEVENTLOGRECORD pRec;
    DWORD dwRecSize;
    NTSTATUS Status;
    PLOGFILE SystemLog = NULL;
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;

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
            // Message = (PIO_ERROR_LOG_MESSAGE)&Request.Message;
            Message = &Request.Message;

            if (!GetComputerNameW(szComputerName, &dwComputerNameLength))
            {
                szComputerName[0] = L'\0';
            }

            RtlTimeToSecondsSince1970(&Message->TimeStamp, &Time);

            // TODO: Log more information??

            pRec = LogfAllocAndBuildNewRecord(
                        &dwRecSize,
                        Time,
                        Message->Type,
                        Message->EntryData.EventCategory,
                        Message->EntryData.ErrorCode,
                        (PWSTR)((ULONG_PTR)Message + Message->DriverNameOffset), // FIXME: Use DriverNameLength too!
                        szComputerName,
                        0,
                        NULL,
                        Message->EntryData.NumberOfStrings,
                        (PWSTR)((ULONG_PTR)Message + Message->EntryData.StringOffset),
                        Message->EntryData.DumpDataSize,
                        (PVOID)((ULONG_PTR)Message + FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData)));

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
                Status = LogfWriteRecord(SystemLog, dwRecSize, pRec);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("ERROR WRITING TO EventLog %S (Status 0x%08lx)\n",
                            SystemLog->FileName, Status);
                }
            }

            LogfFreeRecord(pRec);
        }
    }

    return Status;
}
