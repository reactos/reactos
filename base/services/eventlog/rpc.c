/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/eventlog/rpc.c
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2005 Saveliy Tretiakov
 */

/* INCLUDES *****************************************************************/

#include "eventlog.h"

/* FUNCTIONS ****************************************************************/

DWORD STDCALL RpcThreadRoutine(LPVOID lpParameter)
{
    RPC_STATUS Status;

    Status = RpcServerUseProtseqEpW(L"ncacn_np", 20, L"\\pipe\\EventLog", NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerRegisterIf(eventlog_v0_0_s_ifspec, NULL, NULL);

    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);

    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerListen() failed (Status %lx)\n", Status);
    }

    return 0;
}


/* Function 0 */
NTSTATUS ElfrClearELFW(
    IELF_HANDLE LogHandle,
    PRPC_UNICODE_STRING BackupFileName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 1 */
NTSTATUS ElfrBackupELFW(
    IELF_HANDLE LogHandle,
    PRPC_UNICODE_STRING BackupFileName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 2 */
NTSTATUS ElfrCloseEL(
    IELF_HANDLE *LogHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 3 */
NTSTATUS ElfrDeregisterEventSource(
    IELF_HANDLE *LogHandle)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}


/* Function 4 */
NTSTATUS ElfrNumberOfRecords(
    IELF_HANDLE LogHandle,
    DWORD *NumberOfRecords)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 5 */
NTSTATUS ElfrOldestRecord(
    IELF_HANDLE LogHandle,
    DWORD *OldestRecordNumber)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 6 */
NTSTATUS ElfrChangeNotify(
    IELF_HANDLE *LogHandle,
    RPC_CLIENT_ID ClientId,
    DWORD Event)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 7 */
NTSTATUS ElfrOpenELW(
    EVENTLOG_HANDLE_W UNCServerName,
    PRPC_UNICODE_STRING ModuleName,
    PRPC_UNICODE_STRING RegModuleName,
    DWORD MajorVersion,
    DWORD MinorVersion,
    IELF_HANDLE *LogHandle)
{
    UNIMPLEMENTED;
    *LogHandle = (IELF_HANDLE)1;
    return STATUS_SUCCESS;
}


/* Function 8 */
NTSTATUS ElfrRegisterEventSourceW(
    EVENTLOG_HANDLE_W UNCServerName,
    PRPC_UNICODE_STRING ModuleName,
    PRPC_UNICODE_STRING RegModuleName,
    DWORD MajorVersion,
    DWORD MinorVersion,
    IELF_HANDLE *LogHandle)
{
    UNIMPLEMENTED;
    *LogHandle = (IELF_HANDLE)1;
    return STATUS_SUCCESS;
}


/* Function 9 */
NTSTATUS ElfrOpenBELW(
    EVENTLOG_HANDLE_W UNCServerName,
    PRPC_UNICODE_STRING BackupFileName,
    DWORD MajorVersion,
    DWORD MinorVersion,
    IELF_HANDLE *LogHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 10 */
NTSTATUS ElfrReadELW(
    IELF_HANDLE LogHandle,
    DWORD ReadFlags,
    DWORD RecordOffset,
    RULONG NumberOfBytesToRead,
    BYTE *Buffer,
    DWORD *NumberOfBytesRead,
    DWORD *MinNumberOfBytesNeeded)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 11 */
NTSTATUS ElfrReportEventW(
    IELF_HANDLE LogHandle,
    DWORD Time,
    USHORT EventType,
    USHORT EventCategory,
    DWORD EventID,
    USHORT NumStrings,
    DWORD DataSize,
    PRPC_UNICODE_STRING ComputerName,
    PRPC_SID UserSID,
    PRPC_UNICODE_STRING Strings[],
    BYTE *Data,
    USHORT Flags,
    DWORD *RecordNumber,
    DWORD *TimeWritten)
{
    USHORT i;

    /* partial stub */
    for (i = 0; i < NumStrings; i++)
    {
        switch (EventType)
        {
            case EVENTLOG_SUCCESS:
                DPRINT1("Success: %S\n", Strings[i]);
                break;

            case EVENTLOG_ERROR_TYPE:
                DPRINT1("Error: %S\n", Strings[i]);
                break;

            case EVENTLOG_WARNING_TYPE:
                DPRINT1("Warning: %S\n", Strings[i]);
                break;

            case EVENTLOG_INFORMATION_TYPE:
                DPRINT1("Info: %S\n", Strings[i]);
                break;

            default:
                DPRINT1("Type %hu: %S\n", EventType, Strings[i]);
                break;
        }
    }

    return STATUS_SUCCESS;
}


/* Function 12 */
NTSTATUS ElfrClearELFA(
    IELF_HANDLE LogHandle,
    PRPC_STRING BackupFileName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 13 */
NTSTATUS ElfrBackupELFA(
    IELF_HANDLE LogHandle,
    PRPC_STRING BackupFileName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 14 */
NTSTATUS ElfrOpenELA(
    EVENTLOG_HANDLE_A UNCServerName,
    PRPC_STRING ModuleName,
    PRPC_STRING RegModuleName,
    DWORD MajorVersion,
    DWORD MinorVersion,
    IELF_HANDLE *LogHandle)
{
    UNICODE_STRING UNCServerNameW = { 0, };
    UNICODE_STRING ModuleNameW = { 0, };
    UNICODE_STRING RegModuleNameW = { 0, };
    NTSTATUS Status;

    if (UNCServerName &&
        !RtlCreateUnicodeStringFromAsciiz(&UNCServerNameW, UNCServerName))
    {
        return STATUS_NO_MEMORY;
    }

    if (ModuleName &&
        !RtlAnsiStringToUnicodeString(&ModuleNameW, (PANSI_STRING)ModuleName, TRUE))
    {
        RtlFreeUnicodeString(&UNCServerNameW);
        return STATUS_NO_MEMORY;
    }

    if (RegModuleName &&
        !RtlAnsiStringToUnicodeString(&RegModuleNameW, (PANSI_STRING)RegModuleName, TRUE))
    {
        RtlFreeUnicodeString(&UNCServerNameW);
        RtlFreeUnicodeString(&ModuleNameW);
        return STATUS_NO_MEMORY;
    }

    Status = ElfrOpenELW(
        UNCServerName ? UNCServerNameW.Buffer : NULL,
        ModuleName ? (PRPC_UNICODE_STRING)&ModuleNameW : NULL,
        RegModuleName ? (PRPC_UNICODE_STRING)&RegModuleNameW : NULL,
        MajorVersion,
        MinorVersion,
        LogHandle);

    RtlFreeUnicodeString(&UNCServerNameW);
    RtlFreeUnicodeString(&ModuleNameW);
    RtlFreeUnicodeString(&RegModuleNameW);

    return Status;
}


/* Function 15 */
NTSTATUS ElfrRegisterEventSourceA(
    EVENTLOG_HANDLE_A UNCServerName,
    PRPC_STRING ModuleName,
    PRPC_STRING RegModuleName,
    DWORD MajorVersion,
    DWORD MinorVersion,
    IELF_HANDLE *LogHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 16 */
NTSTATUS ElfrOpenBELA(
    EVENTLOG_HANDLE_A UNCServerName,
    PRPC_STRING BackupFileName,
    DWORD MajorVersion,
    DWORD MinorVersion,
    IELF_HANDLE *LogHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 17 */
NTSTATUS ElfrReadELA(
    IELF_HANDLE LogHandle,
    DWORD ReadFlags,
    DWORD RecordOffset,
    RULONG NumberOfBytesToRead,
    BYTE *Buffer,
    DWORD *NumberOfBytesRead,
    DWORD *MinNumberOfBytesNeeded)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 18 */
NTSTATUS ElfrReportEventA(
    IELF_HANDLE LogHandle,
    DWORD Time,
    USHORT EventType,
    USHORT EventCategory,
    DWORD EventID,
    USHORT NumStrings,
    DWORD DataSize,
    PRPC_STRING ComputerName,
    PRPC_SID UserSID,
    PRPC_STRING Strings[],
    BYTE *Data,
    USHORT Flags,
    DWORD *RecordNumber,
    DWORD *TimeWritten)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 19 */
NTSTATUS ElfrRegisterClusterSvc(
    handle_t BindingHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 20 */
NTSTATUS ElfrDeregisterClusterSvc(
    handle_t BindingHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 21 */
NTSTATUS ElfrWriteClusterEvents(
    handle_t BindingHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 22 */
NTSTATUS ElfrGetLogInformation(
    IELF_HANDLE LogHandle,
    DWORD InfoLevel,
    BYTE *Buffer,
    DWORD cbBufSize,
    DWORD *pcbBytesNeeded)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 23 */
NTSTATUS ElfrFlushEL(
    IELF_HANDLE LogHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 24 */
NTSTATUS ElfrReportEventAndSourceW(
    IELF_HANDLE LogHandle,
    DWORD Time,
    USHORT EventType,
    USHORT EventCategory,
    ULONG EventID,
    PRPC_UNICODE_STRING SourceName,
    USHORT NumStrings,
    DWORD DataSize,
    PRPC_UNICODE_STRING ComputerName,
    PRPC_SID UserSID,
    PRPC_UNICODE_STRING Strings[],
    BYTE *Data,
    USHORT Flags,
    DWORD *RecordNumber,
    DWORD *TimeWritten)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


void __RPC_FAR *__RPC_USER midl_user_allocate(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


void __RPC_USER IELF_HANDLE_rundown(IELF_HANDLE LogHandle)
{
}
