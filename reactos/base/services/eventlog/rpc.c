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

LIST_ENTRY LogHandleListHead;

/* FUNCTIONS ****************************************************************/

DWORD WINAPI RpcThreadRoutine(LPVOID lpParameter)
{
    RPC_STATUS Status;

    InitializeListHead(&LogHandleListHead);

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

PLOGHANDLE ElfCreateEventLogHandle(LPCWSTR Name, BOOL Create)
{
    PLOGHANDLE lpLogHandle;
    PLOGFILE currentLogFile = NULL;
    INT i, LogsActive;
    PEVENTSOURCE pEventSource;

    DPRINT("ElfCreateEventLogHandle(Name: %S)\n", Name);

    lpLogHandle = HeapAlloc(GetProcessHeap(), 0, sizeof(LOGHANDLE)
                                  + ((wcslen(Name) + 1) * sizeof(WCHAR)));
    if (!lpLogHandle)
    {
        DPRINT1("Failed to allocate Heap!\n");
        return NULL;
    }

    wcscpy(lpLogHandle->szName, Name);

    /* Get the number of Log Files the EventLog service found */
    LogsActive = LogfListItemCount();
    if (LogsActive == 0)
    {
        DPRINT1("EventLog service reports no log files!\n");
        goto Cleanup;
    }

    /* If Creating, default to the Application Log in case we fail, as documented on MSDN */
    if (Create == TRUE)
    {
        pEventSource = GetEventSourceByName(Name);
        DPRINT("EventSource: %p\n", pEventSource);
        if (pEventSource)
        {
            DPRINT("EventSource LogFile: %p\n", pEventSource->LogFile);
            lpLogHandle->LogFile = pEventSource->LogFile;
        }
        else
        {
            DPRINT("EventSource LogFile: Application log file\n");
            lpLogHandle->LogFile = LogfListItemByName(L"Application");
        }

        DPRINT("LogHandle LogFile: %p\n", lpLogHandle->LogFile);
    }
    else
    {
        lpLogHandle->LogFile = NULL;

        for (i = 1; i <= LogsActive; i++)
        {
            currentLogFile = LogfListItemByIndex(i);

            if (_wcsicmp(Name, currentLogFile->LogName) == 0)
            {
                lpLogHandle->LogFile = LogfListItemByIndex(i);
                lpLogHandle->CurrentRecord = LogfGetOldestRecord(lpLogHandle->LogFile);
                break;
            }
        }

        /* Use the application log if the desired log does not exist */
        if (lpLogHandle->LogFile == NULL)
        {
            lpLogHandle->LogFile = LogfListItemByName(L"Application");
            lpLogHandle->CurrentRecord = LogfGetOldestRecord(lpLogHandle->LogFile);
        }
    }

    if (!lpLogHandle->LogFile)
        goto Cleanup;

    /* Append log handle */
    InsertTailList(&LogHandleListHead, &lpLogHandle->LogHandleListEntry);

    return lpLogHandle;

Cleanup:
    HeapFree(GetProcessHeap(), 0, lpLogHandle);

    return NULL;
}

PLOGHANDLE ElfGetLogHandleEntryByHandle(IELF_HANDLE EventLogHandle)
{
    PLOGHANDLE lpLogHandle;

    if (IsListEmpty(&LogHandleListHead))
    {
        return NULL;
    }

    lpLogHandle = CONTAINING_RECORD((PLOGHANDLE)EventLogHandle, LOGHANDLE, LogHandleListEntry);

    return lpLogHandle;
}

BOOL ElfDeleteEventLogHandle(IELF_HANDLE EventLogHandle)
{
    PLOGHANDLE lpLogHandle = (PLOGHANDLE)EventLogHandle;
    if (!ElfGetLogHandleEntryByHandle(lpLogHandle))
    {
        return FALSE;
    }

    RemoveEntryList(&lpLogHandle->LogHandleListEntry);
    HeapFree(GetProcessHeap(),0,lpLogHandle);

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
    PLOGHANDLE lpLogHandle;
    PLOGFILE lpLogFile;

    lpLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!lpLogHandle)
    {
        return STATUS_INVALID_HANDLE;
    }

    lpLogFile = lpLogHandle->LogFile;

    if (lpLogFile->Header.OldestRecordNumber == 0)
        *NumberOfRecords = 0;
    else
        *NumberOfRecords = lpLogFile->Header.CurrentRecordNumber -
                           lpLogFile->Header.OldestRecordNumber;

    return STATUS_SUCCESS;
}


/* Function 5 */
NTSTATUS ElfrOldestRecord(
    IELF_HANDLE LogHandle,
    DWORD *OldestRecordNumber)
{
    PLOGHANDLE lpLogHandle;

    lpLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!lpLogHandle)
    {
        return STATUS_INVALID_HANDLE;
    }

    if (!OldestRecordNumber)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *OldestRecordNumber = 0;
    *OldestRecordNumber = LogfGetOldestRecord(lpLogHandle->LogFile);
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
    DPRINT1("ElfrRegisterEventSourceW()\n");

    if ((MajorVersion != 1) || (MinorVersion != 1))
        return STATUS_INVALID_PARAMETER;

    /* RegModuleName must be an empty string */
    if (RegModuleName->Length > 0)
        return STATUS_INVALID_PARAMETER;

    DPRINT1("ModuleName: %S\n", ModuleName->Buffer);

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
    PLOGHANDLE lpLogHandle;
    DWORD dwError;
    DWORD RecordNumber;

    lpLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!lpLogHandle)
    {
        return STATUS_INVALID_HANDLE;
    }

    if (!Buffer) 
        return I_RpcMapWin32Status(ERROR_INVALID_PARAMETER);

    /* If sequential read, retrieve the CurrentRecord from this log handle */
    if (ReadFlags & EVENTLOG_SEQUENTIAL_READ)
    {
        RecordNumber = lpLogHandle->CurrentRecord;
    }
    else
    {
        RecordNumber = RecordOffset;
    }

    dwError = LogfReadEvent(lpLogHandle->LogFile, ReadFlags, &RecordNumber,
                            NumberOfBytesToRead, Buffer, NumberOfBytesRead, MinNumberOfBytesNeeded);

    /* Update the handles CurrentRecord if success*/
    if (dwError == ERROR_SUCCESS)
    {
        lpLogHandle->CurrentRecord = RecordNumber;
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
    PLOGHANDLE lpLogHandle;
    DWORD lastRec;
    DWORD recSize;
    DWORD dwStringsSize = 0;
    DWORD dwUserSidLength = 0;
    DWORD dwError = ERROR_SUCCESS;
    WCHAR *lpStrings;
    int pos = 0;

    lpLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!lpLogHandle)
    {
        return STATUS_INVALID_HANDLE;
    }

    /* Flags must be 0 */
    if (Flags)
    {
        return STATUS_INVALID_PARAMETER;
    }

    lastRec = LogfGetCurrentRecord(lpLogHandle->LogFile);

    for (i = 0; i < NumStrings; i++)
    {
        switch (EventType)
        {
            case EVENTLOG_SUCCESS:
                DPRINT("Success: %wZ\n", Strings[i]);
                break;

            case EVENTLOG_ERROR_TYPE:
                DPRINT("Error: %wZ\n", Strings[i]);
                break;

            case EVENTLOG_WARNING_TYPE:
                DPRINT("Warning: %wZ\n", Strings[i]);
                break;

            case EVENTLOG_INFORMATION_TYPE:
                DPRINT("Info: %wZ\n", Strings[i]);
                break;

            default:
                DPRINT1("Type %hu: %wZ\n", EventType, Strings[i]);
                break;
        }
        dwStringsSize += Strings[i]->Length + sizeof UNICODE_NULL;
    }

    lpStrings = HeapAlloc(GetProcessHeap(), 0, dwStringsSize);
    if (!lpStrings)
    {
        DPRINT1("Failed to allocate heap\n");
        return STATUS_NO_MEMORY;
    }

    for (i = 0; i < NumStrings; i++)
    {
        CopyMemory(lpStrings + pos, Strings[i]->Buffer, Strings[i]->Length);
        pos += Strings[i]->Length / sizeof(WCHAR);
        lpStrings[pos] = UNICODE_NULL;
        pos += sizeof UNICODE_NULL / sizeof(WCHAR);
    }

    if (UserSID)
        dwUserSidLength = FIELD_OFFSET(SID, SubAuthority[UserSID->SubAuthorityCount]);
    LogBuffer = LogfAllocAndBuildNewRecord(&recSize,
                                           lastRec,
                                           EventType,
                                           EventCategory,
                                           EventID,
                                           lpLogHandle->szName,
                                           ComputerName->Buffer,
                                           dwUserSidLength,
                                           UserSID,
                                           NumStrings,
                                           lpStrings,
                                           DataSize,
                                           Data);

    dwError = LogfWriteData(lpLogHandle->LogFile, recSize, LogBuffer);
    if (!dwError)
    {
        DPRINT1("ERROR WRITING TO EventLog %S\n", lpLogHandle->LogFile->FileName);
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
    UNICODE_STRING ModuleNameW;

    if ((MajorVersion != 1) || (MinorVersion != 1))
        return STATUS_INVALID_PARAMETER;

    /* RegModuleName must be an empty string */
    if (RegModuleName->Length > 0)
        return STATUS_INVALID_PARAMETER;

    RtlAnsiStringToUnicodeString(&ModuleNameW, (PANSI_STRING)ModuleName, TRUE);

    /* FIXME: Must verify that caller has read access */

    *LogHandle = ElfCreateEventLogHandle(ModuleNameW.Buffer, FALSE);

    RtlFreeUnicodeString(&ModuleNameW);

    if (*LogHandle == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
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
    UNICODE_STRING ModuleNameW    = { 0, 0, NULL };

    if (ModuleName &&
        !RtlAnsiStringToUnicodeString(&ModuleNameW, (PANSI_STRING)ModuleName, TRUE))
    {
        return STATUS_NO_MEMORY;
    }

    /* RegModuleName must be an empty string */
    if (RegModuleName->Length > 0)
    {
        RtlFreeUnicodeString(&ModuleNameW);
        return STATUS_INVALID_PARAMETER;
    }

    if ((MajorVersion != 1) || (MinorVersion != 1))
    {
        RtlFreeUnicodeString(&ModuleNameW);
        return STATUS_INVALID_PARAMETER;
    }

    /* FIXME: Must verify that caller has write access */

    *LogHandle = ElfCreateEventLogHandle(ModuleNameW.Buffer,
                                         TRUE);

    RtlFreeUnicodeString(&ModuleNameW);

    return STATUS_SUCCESS;
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
    NTSTATUS Status = STATUS_SUCCESS;

    /* FIXME: check handle first */

    switch (InfoLevel)
    {
        case EVENTLOG_FULL_INFO:
            {
                LPEVENTLOG_FULL_INFORMATION efi = (LPEVENTLOG_FULL_INFORMATION)Buffer;

                *pcbBytesNeeded = sizeof(EVENTLOG_FULL_INFORMATION);
                if (cbBufSize < sizeof(EVENTLOG_FULL_INFORMATION))
                {
                    return STATUS_BUFFER_TOO_SMALL;
                }

                efi->dwFull = 0; /* FIXME */
            }
            break;

        default:
            Status = STATUS_INVALID_LEVEL;
            break;
    }

    return Status;
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


void __RPC_FAR *__RPC_USER midl_user_allocate(SIZE_T len)
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
