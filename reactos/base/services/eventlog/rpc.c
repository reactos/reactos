/*
 * PROJECT:         ReactOS EventLog Service
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/services/eventlog/rpc.c
 * PURPOSE:         RPC Port Interface support
 * COPYRIGHT:       Copyright 2005 Saveliy Tretiakov
 *                  Copyright 2008 Michael Martin
 *                  Copyright 2010-2011 Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "eventlog.h"

#define NDEBUG
#include <debug.h>

static LIST_ENTRY LogHandleListHead;
static CRITICAL_SECTION LogHandleListCs;

/* FUNCTIONS ****************************************************************/

static NTSTATUS
ElfDeleteEventLogHandle(PIELF_HANDLE LogHandle);

DWORD WINAPI RpcThreadRoutine(LPVOID lpParameter)
{
    RPC_STATUS Status;

    InitializeCriticalSection(&LogHandleListCs);
    InitializeListHead(&LogHandleListHead);

    Status = RpcServerUseProtseqEpW(L"ncacn_np", 20, L"\\pipe\\EventLog", NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        goto Quit;
    }

    Status = RpcServerRegisterIf(eventlog_v0_0_s_ifspec, NULL, NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        goto Quit;
    }

    Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerListen() failed (Status %lx)\n", Status);
    }

    EnterCriticalSection(&LogHandleListCs);
    while (!IsListEmpty(&LogHandleListHead))
    {
        IELF_HANDLE LogHandle = (IELF_HANDLE)CONTAINING_RECORD(LogHandleListHead.Flink, LOGHANDLE, LogHandleListEntry);
        ElfDeleteEventLogHandle(&LogHandle);
    }
    LeaveCriticalSection(&LogHandleListCs);

Quit:
    DeleteCriticalSection(&LogHandleListCs);

    return 0;
}


static NTSTATUS
ElfCreateEventLogHandle(PLOGHANDLE* LogHandle,
                        PUNICODE_STRING LogName,
                        BOOLEAN Create)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLOGHANDLE pLogHandle;
    PLOGFILE currentLogFile = NULL;
    DWORD i, LogsActive;
    PEVENTSOURCE pEventSource;

    DPRINT("ElfCreateEventLogHandle(%wZ)\n", LogName);

    *LogHandle = NULL;

    i = (LogName->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR);
    pLogHandle = HeapAlloc(GetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           FIELD_OFFSET(LOGHANDLE, szName[i]));
    if (!pLogHandle)
    {
        DPRINT1("Failed to allocate Heap!\n");
        return STATUS_NO_MEMORY;
    }

    StringCchCopyW(pLogHandle->szName, i, LogName->Buffer);

    /* Get the number of Log Files the EventLog service found */
    // NOTE: We could just as well loop only once within the list of logs
    // and retrieve what the code below that calls LogfListItemByIndex, does!!
    LogsActive = LogfListItemCount();
    if (LogsActive == 0)
    {
        DPRINT1("EventLog service reports no log files!\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Done;
    }

    /* If Creating, default to the Application Log in case we fail, as documented on MSDN */
    if (Create)
    {
        pEventSource = GetEventSourceByName(LogName->Buffer);
        DPRINT("EventSource: %p\n", pEventSource);
        if (pEventSource)
        {
            DPRINT("EventSource LogFile: %p\n", pEventSource->LogFile);
            pLogHandle->LogFile = pEventSource->LogFile;
        }
        else
        {
            DPRINT("EventSource LogFile: Application log file\n");
            pLogHandle->LogFile = LogfListItemByName(L"Application");
        }

        DPRINT("LogHandle LogFile: %p\n", pLogHandle->LogFile);
    }
    else
    {
        pLogHandle->LogFile = NULL;

        for (i = 1; i <= LogsActive; i++)
        {
            currentLogFile = LogfListItemByIndex(i);

            if (_wcsicmp(LogName->Buffer, currentLogFile->LogName) == 0)
            {
                pLogHandle->LogFile = currentLogFile;
                break;
            }
        }

        /* Use the application log if the desired log does not exist */
        if (pLogHandle->LogFile == NULL)
        {
            pLogHandle->LogFile = LogfListItemByName(L"Application");
            if (pLogHandle->LogFile == NULL)
            {
                DPRINT1("Application log is missing!\n");
                Status = STATUS_UNSUCCESSFUL;
                goto Done;
            }
        }

        /* Reset the current record */
        pLogHandle->CurrentRecord = 0;
    }

    if (!pLogHandle->LogFile)
        Status = STATUS_UNSUCCESSFUL;

Done:
    if (NT_SUCCESS(Status))
    {
        /* Append log handle */
        EnterCriticalSection(&LogHandleListCs);
        InsertTailList(&LogHandleListHead, &pLogHandle->LogHandleListEntry);
        LeaveCriticalSection(&LogHandleListCs);
        *LogHandle = pLogHandle;
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, pLogHandle);
    }

    return Status;
}


static NTSTATUS
ElfCreateBackupLogHandle(PLOGHANDLE* LogHandle,
                         PUNICODE_STRING FileName)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLOGHANDLE pLogHandle;

    DPRINT("ElfCreateBackupLogHandle(%wZ)\n", FileName);

    *LogHandle = NULL;

    pLogHandle = HeapAlloc(GetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           sizeof(LOGHANDLE));
    if (pLogHandle == NULL)
    {
        DPRINT1("Failed to allocate Heap!\n");
        return STATUS_NO_MEMORY;
    }

    /* Create the log file */
    Status = LogfCreate(&pLogHandle->LogFile,
                        NULL,
                        FileName,
                        0,
                        0,
                        FALSE,
                        TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create the log file! (Status 0x%08lx)\n", Status);
        goto Done;
    }

    /* Set the backup flag */
    pLogHandle->Flags |= LOG_HANDLE_BACKUP_FILE;

    /* Reset the current record */
    pLogHandle->CurrentRecord = 0;

Done:
    if (NT_SUCCESS(Status))
    {
        /* Append log handle */
        EnterCriticalSection(&LogHandleListCs);
        InsertTailList(&LogHandleListHead, &pLogHandle->LogHandleListEntry);
        LeaveCriticalSection(&LogHandleListCs);
        *LogHandle = pLogHandle;
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, pLogHandle);
    }

    return Status;
}


static PLOGHANDLE
ElfGetLogHandleEntryByHandle(IELF_HANDLE EventLogHandle)
{
    PLIST_ENTRY CurrentEntry;
    PLOGHANDLE Handle, pLogHandle = NULL;

    EnterCriticalSection(&LogHandleListCs);

    CurrentEntry = LogHandleListHead.Flink;
    while (CurrentEntry != &LogHandleListHead)
    {
        Handle = CONTAINING_RECORD(CurrentEntry,
                                   LOGHANDLE,
                                   LogHandleListEntry);
        CurrentEntry = CurrentEntry->Flink;

        if (Handle == EventLogHandle)
        {
            pLogHandle = Handle;
            break;
        }
    }

    LeaveCriticalSection(&LogHandleListCs);

    return pLogHandle;
}


static NTSTATUS
ElfDeleteEventLogHandle(PIELF_HANDLE LogHandle)
{
    PLOGHANDLE pLogHandle;

    pLogHandle = ElfGetLogHandleEntryByHandle(*LogHandle);
    if (!pLogHandle)
        return STATUS_INVALID_HANDLE;

    EnterCriticalSection(&LogHandleListCs);
    RemoveEntryList(&pLogHandle->LogHandleListEntry);
    LeaveCriticalSection(&LogHandleListCs);

    LogfClose(pLogHandle->LogFile, FALSE);

    HeapFree(GetProcessHeap(), 0, pLogHandle);

    *LogHandle = NULL;

    return STATUS_SUCCESS;
}


/* Function 0 */
NTSTATUS
ElfrClearELFW(
    IELF_HANDLE LogHandle,
    PRPC_UNICODE_STRING BackupFileName)
{
    PLOGHANDLE pLogHandle;

    DPRINT("ElfrClearELFW()\n");

    pLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!pLogHandle)
        return STATUS_INVALID_HANDLE;

    /* Fail, if the log file is a backup file */
    if (pLogHandle->Flags & LOG_HANDLE_BACKUP_FILE)
        return STATUS_INVALID_HANDLE;

    return LogfClearFile(pLogHandle->LogFile,
                         (PUNICODE_STRING)BackupFileName);
}


/* Function 1 */
NTSTATUS
ElfrBackupELFW(
    IELF_HANDLE LogHandle,
    PRPC_UNICODE_STRING BackupFileName)
{
    PLOGHANDLE pLogHandle;

    DPRINT("ElfrBackupELFW()\n");

    pLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!pLogHandle)
        return STATUS_INVALID_HANDLE;

    return LogfBackupFile(pLogHandle->LogFile,
                          (PUNICODE_STRING)BackupFileName);
}


/* Function 2 */
NTSTATUS
ElfrCloseEL(
    PIELF_HANDLE LogHandle)
{
    return ElfDeleteEventLogHandle(LogHandle);
}


/* Function 3 */
NTSTATUS
ElfrDeregisterEventSource(
    PIELF_HANDLE LogHandle)
{
    return ElfDeleteEventLogHandle(LogHandle);
}


/* Function 4 */
NTSTATUS
ElfrNumberOfRecords(
    IELF_HANDLE LogHandle,
    PULONG NumberOfRecords)
{
    PLOGHANDLE pLogHandle;
    PLOGFILE pLogFile;
    ULONG OldestRecordNumber, CurrentRecordNumber;

    DPRINT("ElfrNumberOfRecords()\n");

    pLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!pLogHandle)
        return STATUS_INVALID_HANDLE;

    if (!NumberOfRecords)
        return STATUS_INVALID_PARAMETER;

    pLogFile = pLogHandle->LogFile;

    /* Lock the log file shared */
    RtlAcquireResourceShared(&pLogFile->Lock, TRUE);

    OldestRecordNumber  = ElfGetOldestRecord(&pLogFile->LogFile);
    CurrentRecordNumber = ElfGetCurrentRecord(&pLogFile->LogFile);

    /* Unlock the log file */
    RtlReleaseResource(&pLogFile->Lock);

    DPRINT("Oldest: %lu  Current: %lu\n",
           OldestRecordNumber, CurrentRecordNumber);

    if (OldestRecordNumber == 0)
    {
        /* OldestRecordNumber == 0 when the log is empty */
        *NumberOfRecords = 0;
    }
    else
    {
        /* The log contains events */
        *NumberOfRecords = CurrentRecordNumber - OldestRecordNumber;
    }

    return STATUS_SUCCESS;
}


/* Function 5 */
NTSTATUS
ElfrOldestRecord(
    IELF_HANDLE LogHandle,
    PULONG OldestRecordNumber)
{
    PLOGHANDLE pLogHandle;
    PLOGFILE pLogFile;

    pLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!pLogHandle)
        return STATUS_INVALID_HANDLE;

    if (!OldestRecordNumber)
        return STATUS_INVALID_PARAMETER;

    pLogFile = pLogHandle->LogFile;

    /* Lock the log file shared */
    RtlAcquireResourceShared(&pLogFile->Lock, TRUE);

    *OldestRecordNumber = ElfGetOldestRecord(&pLogFile->LogFile);

    /* Unlock the log file */
    RtlReleaseResource(&pLogFile->Lock);

    return STATUS_SUCCESS;
}


/* Function 6 */
NTSTATUS
ElfrChangeNotify(
    IELF_HANDLE LogHandle,
    RPC_CLIENT_ID ClientId,
    ULONG Event)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 7 */
NTSTATUS
ElfrOpenELW(
    EVENTLOG_HANDLE_W UNCServerName,
    PRPC_UNICODE_STRING ModuleName,
    PRPC_UNICODE_STRING RegModuleName,
    ULONG MajorVersion,
    ULONG MinorVersion,
    PIELF_HANDLE LogHandle)
{
    if ((MajorVersion != 1) || (MinorVersion != 1))
        return STATUS_INVALID_PARAMETER;

    /* RegModuleName must be an empty string */
    if (RegModuleName->Length > 0)
        return STATUS_INVALID_PARAMETER;

    /* FIXME: UNCServerName must specify the server */

    /* FIXME: Must verify that caller has read access */

    return ElfCreateEventLogHandle((PLOGHANDLE*)LogHandle,
                                   (PUNICODE_STRING)ModuleName,
                                   FALSE);
}


/* Function 8 */
NTSTATUS
ElfrRegisterEventSourceW(
    EVENTLOG_HANDLE_W UNCServerName,
    PRPC_UNICODE_STRING ModuleName,
    PRPC_UNICODE_STRING RegModuleName,
    ULONG MajorVersion,
    ULONG MinorVersion,
    PIELF_HANDLE LogHandle)
{
    DPRINT("ElfrRegisterEventSourceW()\n");

    if ((MajorVersion != 1) || (MinorVersion != 1))
        return STATUS_INVALID_PARAMETER;

    /* RegModuleName must be an empty string */
    if (RegModuleName->Length > 0)
        return STATUS_INVALID_PARAMETER;

    DPRINT("ModuleName: %wZ\n", ModuleName);

    /* FIXME: UNCServerName must specify the server or empty for local */

    /* FIXME: Must verify that caller has write access */

    return ElfCreateEventLogHandle((PLOGHANDLE*)LogHandle,
                                   (PUNICODE_STRING)ModuleName,
                                   TRUE);
}


/* Function 9 */
NTSTATUS
ElfrOpenBELW(
    EVENTLOG_HANDLE_W UNCServerName,
    PRPC_UNICODE_STRING BackupFileName,
    ULONG MajorVersion,
    ULONG MinorVersion,
    PIELF_HANDLE LogHandle)
{
    DPRINT("ElfrOpenBELW(%wZ)\n", BackupFileName);

    if ((MajorVersion != 1) || (MinorVersion != 1))
        return STATUS_INVALID_PARAMETER;

    /* FIXME: UNCServerName must specify the server */

    /* FIXME: Must verify that caller has read access */

    return ElfCreateBackupLogHandle((PLOGHANDLE*)LogHandle,
                                    (PUNICODE_STRING)BackupFileName);
}


/* Function 10 */
NTSTATUS
ElfrReadELW(
    IELF_HANDLE LogHandle,
    ULONG ReadFlags,
    ULONG RecordOffset,
    RULONG NumberOfBytesToRead,
    PBYTE Buffer,
    PULONG NumberOfBytesRead,
    PULONG MinNumberOfBytesNeeded)
{
    NTSTATUS Status;
    PLOGHANDLE pLogHandle;
    ULONG RecordNumber;

    pLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!pLogHandle)
        return STATUS_INVALID_HANDLE;

    if (!Buffer)
        return STATUS_INVALID_PARAMETER;

    /* If sequential read, retrieve the CurrentRecord from this log handle */
    if (ReadFlags & EVENTLOG_SEQUENTIAL_READ)
    {
        RecordNumber = pLogHandle->CurrentRecord;
    }
    else // (ReadFlags & EVENTLOG_SEEK_READ)
    {
        RecordNumber = RecordOffset;
    }

    Status = LogfReadEvents(pLogHandle->LogFile,
                            ReadFlags,
                            &RecordNumber,
                            NumberOfBytesToRead,
                            Buffer,
                            NumberOfBytesRead,
                            MinNumberOfBytesNeeded,
                            FALSE);

    /* Update the handle's CurrentRecord if success */
    if (NT_SUCCESS(Status))
    {
        pLogHandle->CurrentRecord = RecordNumber;
    }

    return Status;
}


/* Helper function for ElfrReportEventW/A and ElfrReportEventAndSourceW */
NTSTATUS
ElfrIntReportEventW(
    IELF_HANDLE LogHandle,
    ULONG Time,
    USHORT EventType,
    USHORT EventCategory,
    ULONG EventID,
    PRPC_UNICODE_STRING SourceName OPTIONAL,
    USHORT NumStrings,
    ULONG DataSize,
    PRPC_UNICODE_STRING ComputerName,
    PRPC_SID UserSID,
    PRPC_UNICODE_STRING Strings[],
    PBYTE Data,
    USHORT Flags,
    PULONG RecordNumber,
    PULONG TimeWritten)
{
    NTSTATUS Status;
    PLOGHANDLE pLogHandle;
    UNICODE_STRING LocalSourceName, LocalComputerName;
    PEVENTLOGRECORD LogBuffer;
    USHORT i;
    SIZE_T RecSize;
    ULONG dwStringsSize = 0;
    ULONG dwUserSidLength = 0;
    PWSTR lpStrings, str;

    pLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!pLogHandle)
        return STATUS_INVALID_HANDLE;

    /* Flags must be 0 */
    if (Flags)
        return STATUS_INVALID_PARAMETER;

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

            case EVENTLOG_AUDIT_SUCCESS:
                DPRINT("Audit Success: %wZ\n", Strings[i]);
                break;

            case EVENTLOG_AUDIT_FAILURE:
                DPRINT("Audit Failure: %wZ\n", Strings[i]);
                break;

            default:
                DPRINT1("Type %hu: %wZ\n", EventType, Strings[i]);
                break;
        }
        dwStringsSize += Strings[i]->Length + sizeof(UNICODE_NULL);
    }

    lpStrings = HeapAlloc(GetProcessHeap(), 0, dwStringsSize);
    if (!lpStrings)
    {
        DPRINT1("Failed to allocate heap\n");
        return STATUS_NO_MEMORY;
    }

    str = lpStrings;
    for (i = 0; i < NumStrings; i++)
    {
        RtlCopyMemory(str, Strings[i]->Buffer, Strings[i]->Length);
        str += Strings[i]->Length / sizeof(WCHAR);
        *str = UNICODE_NULL;
        str++;
    }

    if (UserSID)
        dwUserSidLength = FIELD_OFFSET(SID, SubAuthority[UserSID->SubAuthorityCount]);

    if (SourceName && SourceName->Buffer)
        LocalSourceName = *(PUNICODE_STRING)SourceName;
    else
        RtlInitUnicodeString(&LocalSourceName, pLogHandle->szName);

    LocalComputerName = *(PUNICODE_STRING)ComputerName;

    LogBuffer = LogfAllocAndBuildNewRecord(&RecSize,
                                           Time,
                                           EventType,
                                           EventCategory,
                                           EventID,
                                           &LocalSourceName,
                                           &LocalComputerName,
                                           dwUserSidLength,
                                           UserSID,
                                           NumStrings,
                                           lpStrings,
                                           DataSize,
                                           Data);
    if (LogBuffer == NULL)
    {
        DPRINT1("LogfAllocAndBuildNewRecord failed!\n");
        HeapFree(GetProcessHeap(), 0, lpStrings);
        return STATUS_NO_MEMORY;
    }

    Status = LogfWriteRecord(pLogHandle->LogFile, LogBuffer, RecSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR writing to event log `%S' (Status 0x%08lx)\n",
                pLogHandle->LogFile->LogName, Status);
    }

    if (NT_SUCCESS(Status))
    {
        /* Retrieve the two fields that were set by LogfWriteRecord into the record */
        if (RecordNumber)
            *RecordNumber = LogBuffer->RecordNumber;
        if (TimeWritten)
            *TimeWritten = LogBuffer->TimeWritten;
    }

    LogfFreeRecord(LogBuffer);

    HeapFree(GetProcessHeap(), 0, lpStrings);

    return Status;
}


/* Function 11 */
NTSTATUS
ElfrReportEventW(
    IELF_HANDLE LogHandle,
    ULONG Time,
    USHORT EventType,
    USHORT EventCategory,
    ULONG EventID,
    USHORT NumStrings,
    ULONG DataSize,
    PRPC_UNICODE_STRING ComputerName,
    PRPC_SID UserSID,
    PRPC_UNICODE_STRING Strings[],
    PBYTE Data,
    USHORT Flags,
    PULONG RecordNumber,
    PULONG TimeWritten)
{
    /* Call the helper function. The event source is provided via the log handle. */
    return ElfrIntReportEventW(LogHandle,
                               Time,
                               EventType,
                               EventCategory,
                               EventID,
                               NULL,
                               NumStrings,
                               DataSize,
                               ComputerName,
                               UserSID,
                               Strings,
                               Data,
                               Flags,
                               RecordNumber,
                               TimeWritten);
}


/* Function 12 */
NTSTATUS
ElfrClearELFA(
    IELF_HANDLE LogHandle,
    PRPC_STRING BackupFileName)
{
    NTSTATUS Status;
    UNICODE_STRING BackupFileNameW;

    Status = RtlAnsiStringToUnicodeString(&BackupFileNameW,
                                          (PANSI_STRING)BackupFileName,
                                          TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = ElfrClearELFW(LogHandle,
                           (PRPC_UNICODE_STRING)&BackupFileNameW);

    RtlFreeUnicodeString(&BackupFileNameW);

    return Status;
}


/* Function 13 */
NTSTATUS
ElfrBackupELFA(
    IELF_HANDLE LogHandle,
    PRPC_STRING BackupFileName)
{
    NTSTATUS Status;
    UNICODE_STRING BackupFileNameW;

    Status = RtlAnsiStringToUnicodeString(&BackupFileNameW,
                                          (PANSI_STRING)BackupFileName,
                                          TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = ElfrBackupELFW(LogHandle,
                            (PRPC_UNICODE_STRING)&BackupFileNameW);

    RtlFreeUnicodeString(&BackupFileNameW);

    return Status;
}


/* Function 14 */
NTSTATUS
ElfrOpenELA(
    EVENTLOG_HANDLE_A UNCServerName,
    PRPC_STRING ModuleName,
    PRPC_STRING RegModuleName,
    ULONG MajorVersion,
    ULONG MinorVersion,
    PIELF_HANDLE LogHandle)
{
    NTSTATUS Status;
    UNICODE_STRING ModuleNameW;

    if ((MajorVersion != 1) || (MinorVersion != 1))
        return STATUS_INVALID_PARAMETER;

    /* RegModuleName must be an empty string */
    if (RegModuleName->Length > 0)
        return STATUS_INVALID_PARAMETER;

    Status = RtlAnsiStringToUnicodeString(&ModuleNameW, (PANSI_STRING)ModuleName, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* FIXME: Must verify that caller has read access */

    Status = ElfCreateEventLogHandle((PLOGHANDLE*)LogHandle,
                                     &ModuleNameW,
                                     FALSE);

    RtlFreeUnicodeString(&ModuleNameW);

    return Status;
}


/* Function 15 */
NTSTATUS
ElfrRegisterEventSourceA(
    EVENTLOG_HANDLE_A UNCServerName,
    PRPC_STRING ModuleName,
    PRPC_STRING RegModuleName,
    ULONG MajorVersion,
    ULONG MinorVersion,
    PIELF_HANDLE LogHandle)
{
    NTSTATUS Status;
    UNICODE_STRING ModuleNameW;

    Status = RtlAnsiStringToUnicodeString(&ModuleNameW,
                                          (PANSI_STRING)ModuleName,
                                          TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAnsiStringToUnicodeString failed (Status 0x%08lx)\n", Status);
        return Status;
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

    Status = ElfCreateEventLogHandle((PLOGHANDLE*)LogHandle,
                                     &ModuleNameW,
                                     TRUE);

    RtlFreeUnicodeString(&ModuleNameW);

    return Status;
}


/* Function 16 */
NTSTATUS
ElfrOpenBELA(
    EVENTLOG_HANDLE_A UNCServerName,
    PRPC_STRING BackupFileName,
    ULONG MajorVersion,
    ULONG MinorVersion,
    PIELF_HANDLE LogHandle)
{
    NTSTATUS Status;
    UNICODE_STRING BackupFileNameW;

    DPRINT("ElfrOpenBELA(%Z)\n", BackupFileName);

    Status = RtlAnsiStringToUnicodeString(&BackupFileNameW,
                                          (PANSI_STRING)BackupFileName,
                                          TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAnsiStringToUnicodeString failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    if ((MajorVersion != 1) || (MinorVersion != 1))
    {
        RtlFreeUnicodeString(&BackupFileNameW);
        return STATUS_INVALID_PARAMETER;
    }

    /* FIXME: UNCServerName must specify the server */

    /* FIXME: Must verify that caller has read access */

    Status = ElfCreateBackupLogHandle((PLOGHANDLE*)LogHandle,
                                      &BackupFileNameW);

    RtlFreeUnicodeString(&BackupFileNameW);

    return Status;
}


/* Function 17 */
NTSTATUS
ElfrReadELA(
    IELF_HANDLE LogHandle,
    ULONG ReadFlags,
    ULONG RecordOffset,
    RULONG NumberOfBytesToRead,
    PBYTE Buffer,
    PULONG NumberOfBytesRead,
    PULONG MinNumberOfBytesNeeded)
{
    NTSTATUS Status;
    PLOGHANDLE pLogHandle;
    ULONG RecordNumber;

    pLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!pLogHandle)
        return STATUS_INVALID_HANDLE;

    if (!Buffer)
        return STATUS_INVALID_PARAMETER;

    /* If sequential read, retrieve the CurrentRecord from this log handle */
    if (ReadFlags & EVENTLOG_SEQUENTIAL_READ)
    {
        RecordNumber = pLogHandle->CurrentRecord;
    }
    else // (ReadFlags & EVENTLOG_SEEK_READ)
    {
        RecordNumber = RecordOffset;
    }

    Status = LogfReadEvents(pLogHandle->LogFile,
                            ReadFlags,
                            &RecordNumber,
                            NumberOfBytesToRead,
                            Buffer,
                            NumberOfBytesRead,
                            MinNumberOfBytesNeeded,
                            TRUE);

    /* Update the handle's CurrentRecord if success */
    if (NT_SUCCESS(Status))
    {
        pLogHandle->CurrentRecord = RecordNumber;
    }

    return Status;
}


/* Function 18 */
NTSTATUS
ElfrReportEventA(
    IELF_HANDLE LogHandle,
    ULONG Time,
    USHORT EventType,
    USHORT EventCategory,
    ULONG EventID,
    USHORT NumStrings,
    ULONG DataSize,
    PRPC_STRING ComputerName,
    PRPC_SID UserSID,
    PRPC_STRING Strings[],
    PBYTE Data,
    USHORT Flags,
    PULONG RecordNumber,
    PULONG TimeWritten)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING ComputerNameW;
    PUNICODE_STRING *StringsArrayW = NULL;
    USHORT i;

    DPRINT("ElfrReportEventA(%hu)\n", NumStrings);

#if 0
    for (i = 0; i < NumStrings; i++)
    {
        if (Strings[i] == NULL)
        {
            DPRINT1("String %hu is null\n", i);
        }
        else
        {
            DPRINT1("String %hu: %Z\n", i, Strings[i]);
        }
    }
#endif

    Status = RtlAnsiStringToUnicodeString((PUNICODE_STRING)&ComputerNameW,
                                          (PANSI_STRING)ComputerName,
                                          TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    if (NumStrings != 0)
    {
        StringsArrayW = HeapAlloc(GetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  NumStrings * sizeof(PUNICODE_STRING));
        if (StringsArrayW == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto Done;
        }

        for (i = 0; i < NumStrings; i++)
        {
            if (Strings[i] != NULL)
            {
                StringsArrayW[i] = HeapAlloc(GetProcessHeap(),
                                             HEAP_ZERO_MEMORY,
                                             sizeof(UNICODE_STRING));
                if (StringsArrayW[i] == NULL)
                {
                    Status = STATUS_NO_MEMORY;
                    break;
                }

                Status = RtlAnsiStringToUnicodeString(StringsArrayW[i],
                                                      (PANSI_STRING)Strings[i],
                                                      TRUE);
            }

            if (!NT_SUCCESS(Status))
                break;
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status = ElfrReportEventW(LogHandle,
                                  Time,
                                  EventType,
                                  EventCategory,
                                  EventID,
                                  NumStrings,
                                  DataSize,
                                  (PRPC_UNICODE_STRING)&ComputerNameW,
                                  UserSID,
                                  (PRPC_UNICODE_STRING*)StringsArrayW,
                                  Data,
                                  Flags,
                                  RecordNumber,
                                  TimeWritten);
    }

Done:
    if (StringsArrayW != NULL)
    {
        for (i = 0; i < NumStrings; i++)
        {
            if ((StringsArrayW[i] != NULL) && (StringsArrayW[i]->Buffer))
            {
                RtlFreeUnicodeString(StringsArrayW[i]);
                HeapFree(GetProcessHeap(), 0, StringsArrayW[i]);
            }
        }

        HeapFree(GetProcessHeap(), 0, StringsArrayW);
    }

    RtlFreeUnicodeString(&ComputerNameW);

    return Status;
}


/* Function 19 */
NTSTATUS
ElfrRegisterClusterSvc(
    handle_t BindingHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 20 */
NTSTATUS
ElfrDeregisterClusterSvc(
    handle_t BindingHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 21 */
NTSTATUS
ElfrWriteClusterEvents(
    handle_t BindingHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 22 */
NTSTATUS
ElfrGetLogInformation(
    IELF_HANDLE LogHandle,
    ULONG InfoLevel,
    PBYTE Buffer,
    ULONG cbBufSize,
    PULONG pcbBytesNeeded)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLOGHANDLE pLogHandle;
    PLOGFILE pLogFile;

    pLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!pLogHandle)
        return STATUS_INVALID_HANDLE;

    pLogFile = pLogHandle->LogFile;

    /* Lock the log file shared */
    RtlAcquireResourceShared(&pLogFile->Lock, TRUE);

    switch (InfoLevel)
    {
        case EVENTLOG_FULL_INFO:
        {
            LPEVENTLOG_FULL_INFORMATION efi = (LPEVENTLOG_FULL_INFORMATION)Buffer;

            *pcbBytesNeeded = sizeof(EVENTLOG_FULL_INFORMATION);
            if (cbBufSize < sizeof(EVENTLOG_FULL_INFORMATION))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            efi->dwFull = !!(ElfGetFlags(&pLogFile->LogFile) & ELF_LOGFILE_LOGFULL_WRITTEN);
            break;
        }

        default:
            Status = STATUS_INVALID_LEVEL;
            break;
    }

    /* Unlock the log file */
    RtlReleaseResource(&pLogFile->Lock);

    return Status;
}


/* Function 23 */
NTSTATUS
ElfrFlushEL(
    IELF_HANDLE LogHandle)
{
    NTSTATUS Status;
    PLOGHANDLE pLogHandle;
    PLOGFILE pLogFile;

    pLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!pLogHandle)
        return STATUS_INVALID_HANDLE;

    pLogFile = pLogHandle->LogFile;

    /* Lock the log file exclusive */
    RtlAcquireResourceExclusive(&pLogFile->Lock, TRUE);

    Status = ElfFlushFile(&pLogFile->LogFile);

    /* Unlock the log file */
    RtlReleaseResource(&pLogFile->Lock);

    return Status;
}


/* Function 24 */
NTSTATUS
ElfrReportEventAndSourceW(
    IELF_HANDLE LogHandle,
    ULONG Time,
    USHORT EventType,
    USHORT EventCategory,
    ULONG EventID,
    PRPC_UNICODE_STRING SourceName,
    USHORT NumStrings,
    ULONG DataSize,
    PRPC_UNICODE_STRING ComputerName,
    PRPC_SID UserSID,
    PRPC_UNICODE_STRING Strings[],
    PBYTE Data,
    USHORT Flags,
    PULONG RecordNumber,
    PULONG TimeWritten)
{
    /* Call the helper function. The event source is specified by the caller. */
    return ElfrIntReportEventW(LogHandle,
                               Time,
                               EventType,
                               EventCategory,
                               EventID,
                               SourceName,
                               NumStrings,
                               DataSize,
                               ComputerName,
                               UserSID,
                               Strings,
                               Data,
                               Flags,
                               RecordNumber,
                               TimeWritten);
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
    /* Close the handle */
    ElfDeleteEventLogHandle(&LogHandle); // ElfrCloseEL(&LogHandle);
}
