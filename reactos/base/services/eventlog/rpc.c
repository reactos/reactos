/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/eventlog/rpc.c
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2005 Saveliy Tretiakov
 *                   Copyright 2008 Michael Martin
 */

/* INCLUDES *****************************************************************/

#include "eventlog.h"

LIST_ENTRY EventSourceListHead;

/* FUNCTIONS ****************************************************************/

DWORD WINAPI RpcThreadRoutine(LPVOID lpParameter)
{
    RPC_STATUS Status;

    InitializeListHead(&EventSourceListHead);

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

PEVENTSOURCE ElfCreateEventLogHandle(LPCWSTR Name, BOOL Create)
{
    PEVENTSOURCE lpEventSource;
    PLOGFILE currentLogFile = NULL;
    INT i, LogsActive;

    lpEventSource = HeapAlloc(GetProcessHeap(), 0, sizeof(EVENTSOURCE)
                                  + ((wcslen(Name) + 1) * sizeof(WCHAR)));
    if (!lpEventSource)
    {
        DPRINT1("Failed to allocate Heap!\n");
        return NULL;
    }

    wcscpy(lpEventSource->szName, Name);

    /* Get the number of Log Files the EventLog service found */
    LogsActive = LogfListItemCount();
    if (LogsActive == 0)
    {
        DPRINT1("EventLog service reports no log files!\n");
        goto Cleanup;
    }

    /* If Creating, default to the Application Log in case we fail, as documented on MSDN */
    if (Create == TRUE)
        lpEventSource->LogFile = LogfListItemByName(L"Application");
    else
        lpEventSource->LogFile = NULL;

    for (i = 1; i <= LogsActive; i++)
    {
        currentLogFile = LogfListItemByIndex(i);

        if (_wcsicmp(Name, currentLogFile->LogName) == 0)
        {
            lpEventSource->LogFile = LogfListItemByIndex(i);
            lpEventSource->CurrentRecord = LogfGetOldestRecord(lpEventSource->LogFile);
            break;
        }
    }

    if (!lpEventSource->LogFile)
        goto Cleanup;

    /* Append service record */
    InsertTailList(&EventSourceListHead, &lpEventSource->EventSourceListEntry);

    return lpEventSource;

Cleanup:
    HeapFree(GetProcessHeap(), 0, lpEventSource);

    return NULL;
}

PEVENTSOURCE ElfGetEventLogSourceEntryByHandle(IELF_HANDLE EventLogHandle)
{
    PEVENTSOURCE CurrentEventSource;

    if (IsListEmpty(&EventSourceListHead))
    {
        return NULL;
    }
    CurrentEventSource = CONTAINING_RECORD((PEVENTSOURCE)EventLogHandle, EVENTSOURCE, EventSourceListEntry);

    return CurrentEventSource;
}

BOOL ElfDeleteEventLogHandle(IELF_HANDLE EventLogHandle)
{
    PEVENTSOURCE lpEventSource = (PEVENTSOURCE)EventLogHandle;
    if (!ElfGetEventLogSourceEntryByHandle(lpEventSource))
    {
        return FALSE;
    }

    RemoveEntryList(&lpEventSource->EventSourceListEntry);
    HeapFree(GetProcessHeap(),0,lpEventSource);

    return TRUE;
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
    if (!ElfDeleteEventLogHandle(*LogHandle))
    {
        return STATUS_INVALID_HANDLE;
    }

    return STATUS_SUCCESS;
}


/* Function 3 */
NTSTATUS ElfrDeregisterEventSource(
    IELF_HANDLE *LogHandle)
{
    if (!ElfDeleteEventLogHandle(*LogHandle))
    {
        return STATUS_INVALID_HANDLE;
    }

    return STATUS_SUCCESS;
}


/* Function 4 */
NTSTATUS ElfrNumberOfRecords(
    IELF_HANDLE LogHandle,
    DWORD *NumberOfRecords)
{
    PEVENTSOURCE lpEventSource;

    lpEventSource = ElfGetEventLogSourceEntryByHandle(LogHandle);
    if (!lpEventSource)
    {
        return STATUS_INVALID_HANDLE;
    }

    *NumberOfRecords = lpEventSource->LogFile->Header.CurrentRecordNumber;

    return STATUS_SUCCESS;
}


/* Function 5 */
NTSTATUS ElfrOldestRecord(
    IELF_HANDLE LogHandle,
    DWORD *OldestRecordNumber)
{
    PEVENTSOURCE lpEventSource;

    lpEventSource = ElfGetEventLogSourceEntryByHandle(LogHandle);
    if (!lpEventSource)
    {
        return STATUS_INVALID_HANDLE;
    }

    if (!OldestRecordNumber)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *OldestRecordNumber = 0;
    *OldestRecordNumber = LogfGetOldestRecord(lpEventSource->LogFile);
    return STATUS_SUCCESS;
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
    if ((MajorVersion != 1) || (MinorVersion != 1))
        return STATUS_INVALID_PARAMETER;

    /* RegModuleName must be an empty string */
    if (RegModuleName->Length > 0)
        return STATUS_INVALID_PARAMETER;

    /*FIXME: UNCServerName must specify the server */

    /*FIXME: Must verify that caller has read access */

    *LogHandle = ElfCreateEventLogHandle(ModuleName->Buffer, FALSE);

    if (*LogHandle == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

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
    if ((MajorVersion != 1) || (MinorVersion != 1))
        return STATUS_INVALID_PARAMETER;

    /* RegModuleName must be an empty string */
    if (RegModuleName->Length > 0)
        return STATUS_INVALID_PARAMETER;

    /*FIXME: UNCServerName must specify the server or empty for local */

    /*FIXME: Must verify that caller has write access */

    *LogHandle = ElfCreateEventLogHandle(ModuleName->Buffer, TRUE);

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
    PEVENTSOURCE lpEventSource;
    DWORD dwError;
    DWORD RecordNumber;

    lpEventSource = ElfGetEventLogSourceEntryByHandle(LogHandle);
    if (!lpEventSource)
    {
        return STATUS_INVALID_HANDLE;
    }

    if (!Buffer) 
        return I_RpcMapWin32Status(ERROR_INVALID_PARAMETER);

    /* If sequential read, retrieve the CurrentRecord from this log handle */
    if (ReadFlags & EVENTLOG_SEQUENTIAL_READ)
    {
        RecordNumber = lpEventSource->CurrentRecord;
    }
    else
    {
        RecordNumber = RecordOffset;
    }

    dwError = LogfReadEvent(lpEventSource->LogFile, ReadFlags, &RecordNumber,
                            NumberOfBytesToRead, Buffer, NumberOfBytesRead, MinNumberOfBytesNeeded);

    /* Update the handles CurrentRecord if success*/
    if (dwError == ERROR_SUCCESS)
    {
        lpEventSource->CurrentRecord = RecordNumber;
    }

    return I_RpcMapWin32Status(dwError);
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
    PBYTE LogBuffer;
    PEVENTSOURCE lpEventSource;
    DWORD lastRec;
    DWORD recSize;
    DWORD dwStringsSize = 0;
    DWORD dwError = ERROR_SUCCESS;
    WCHAR *lpStrings;

    lpEventSource = ElfGetEventLogSourceEntryByHandle(LogHandle);
    if (!lpEventSource)
    {
        return STATUS_INVALID_HANDLE;
    }

    /* Flags must be 0 */
    if (Flags)
    {
        return STATUS_INVALID_PARAMETER;
    }

    lastRec = LogfGetCurrentRecord(lpEventSource->LogFile);

    for (i = 0; i < NumStrings; i++)
    {
        switch (EventType)
        {
            case EVENTLOG_SUCCESS:
                DPRINT1("Success: %wZ\n", Strings[i]);
                break;

            case EVENTLOG_ERROR_TYPE:
                DPRINT1("Error: %wZ\n", Strings[i]);
                break;

            case EVENTLOG_WARNING_TYPE:
                DPRINT1("Warning: %wZ\n", Strings[i]);
                break;

            case EVENTLOG_INFORMATION_TYPE:
                DPRINT1("Info: %wZ\n", Strings[i]);
                break;

            default:
                DPRINT1("Type %hu: %wZ\n", EventType, Strings[i]);
                break;
        }
        dwStringsSize += (wcslen(Strings[i]->Buffer) + 1) * sizeof(WCHAR);
    }

    lpStrings = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, dwStringsSize * 2);
    if (!lpStrings)
    {
        DPRINT1("Failed to allocate heap\n");
        return STATUS_NO_MEMORY;
    }

    int pos = 0;
    for (i = 0; i < NumStrings; i++)
    {
        wcscpy((WCHAR*)(lpStrings + pos), Strings[i]->Buffer);
        pos += (wcslen(Strings[i]->Buffer) + 1) * sizeof(WCHAR);
    }

    LogBuffer = LogfAllocAndBuildNewRecord(&recSize,
                                           lastRec,
                                           EventType,
                                           EventCategory,
                                           EventID,
                                           lpEventSource->szName,
                                           ComputerName->Buffer,
                                           sizeof(UserSID),
                                           &UserSID,
                                           NumStrings,
                                           (WCHAR*)lpStrings,
                                           DataSize,
                                           Data);

    dwError = LogfWriteData(lpEventSource->LogFile, recSize, LogBuffer);
    if (!dwError)
    {
        DPRINT1("ERROR WRITING TO EventLog %S\n",lpEventSource->LogFile->FileName);
    }

    LogfFreeRecord(LogBuffer);

    HeapFree(GetProcessHeap(), 0, lpStrings);

    return I_RpcMapWin32Status(dwError);
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
    UNICODE_STRING UNCServerNameW = { 0, 0, NULL };
    UNICODE_STRING ModuleNameW    = { 0, 0, NULL };
    UNICODE_STRING RegModuleNameW = { 0, 0, NULL };
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
