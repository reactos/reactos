/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/eventlog/rpc.c
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2005 Saveliy Tretiakov
 *                   Copyright 2008 Michael Martin
 */

/* INCLUDES *****************************************************************/

#include "eventlog.h"

#define NDEBUG
#include <debug.h>

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


static NTSTATUS
ElfCreateEventLogHandle(PLOGHANDLE *LogHandle,
                        LPCWSTR Name,
                        BOOL Create)
{
    PLOGHANDLE lpLogHandle;
    PLOGFILE currentLogFile = NULL;
    INT i, LogsActive;
    PEVENTSOURCE pEventSource;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("ElfCreateEventLogHandle(Name: %S)\n", Name);

    lpLogHandle = HeapAlloc(GetProcessHeap(), 0, sizeof(LOGHANDLE)
                                  + ((wcslen(Name) + 1) * sizeof(WCHAR)));
    if (!lpLogHandle)
    {
        DPRINT1("Failed to allocate Heap!\n");
        return STATUS_NO_MEMORY;
    }

    wcscpy(lpLogHandle->szName, Name);

    /* Get the number of Log Files the EventLog service found */
    LogsActive = LogfListItemCount();
    if (LogsActive == 0)
    {
        DPRINT1("EventLog service reports no log files!\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Done;
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

            if (lpLogHandle->LogFile == NULL)
            {
                DPRINT1("Application log is missing!\n");
                Status = STATUS_UNSUCCESSFUL;
                goto Done;
            }

            lpLogHandle->CurrentRecord = LogfGetOldestRecord(lpLogHandle->LogFile);
        }
    }

    if (!lpLogHandle->LogFile)
        Status = STATUS_UNSUCCESSFUL;

Done:
    if (NT_SUCCESS(Status))
    {
        /* Append log handle */
        InsertTailList(&LogHandleListHead, &lpLogHandle->LogHandleListEntry);
        *LogHandle = lpLogHandle;
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, lpLogHandle);
    }

    return Status;
}


static NTSTATUS
ElfCreateBackupLogHandle(PLOGHANDLE *LogHandle,
                         PUNICODE_STRING FileName)
{
    PLOGHANDLE lpLogHandle;

    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("ElfCreateBackupLogHandle(FileName: %wZ)\n", FileName);

    lpLogHandle = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LOGHANDLE));
    if (lpLogHandle == NULL)
    {
        DPRINT1("Failed to allocate Heap!\n");
        return STATUS_NO_MEMORY;
    }

    /* Create the log file */
    Status = LogfCreate(&lpLogHandle->LogFile,
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
    lpLogHandle->Flags |= LOG_HANDLE_BACKUP_FILE;

    /* Get the current record */
    lpLogHandle->CurrentRecord = LogfGetOldestRecord(lpLogHandle->LogFile);

Done:
    if (NT_SUCCESS(Status))
    {
        /* Append log handle */
        InsertTailList(&LogHandleListHead, &lpLogHandle->LogHandleListEntry);
        *LogHandle = lpLogHandle;
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, lpLogHandle);
    }

    return Status;
}


PLOGHANDLE ElfGetLogHandleEntryByHandle(IELF_HANDLE EventLogHandle)
{
    PLIST_ENTRY CurrentEntry;
    PLOGHANDLE lpLogHandle;

    CurrentEntry = LogHandleListHead.Flink;
    while (CurrentEntry != &LogHandleListHead)
    {
        lpLogHandle = CONTAINING_RECORD(CurrentEntry,
                                        LOGHANDLE,
                                        LogHandleListEntry);
        CurrentEntry = CurrentEntry->Flink;

        if (lpLogHandle == EventLogHandle)
            return lpLogHandle;
    }

    return NULL;
}


static NTSTATUS
ElfDeleteEventLogHandle(IELF_HANDLE LogHandle)
{
    PLOGHANDLE lpLogHandle;

    lpLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!lpLogHandle)
    {
        return STATUS_INVALID_HANDLE;
    }

    RemoveEntryList(&lpLogHandle->LogHandleListEntry);
    LogfClose(lpLogHandle->LogFile, FALSE);

    HeapFree(GetProcessHeap(), 0, lpLogHandle);

    return STATUS_SUCCESS;
}

/* Function 0 */
NTSTATUS ElfrClearELFW(
    IELF_HANDLE LogHandle,
    PRPC_UNICODE_STRING BackupFileName)
{
    PLOGHANDLE lpLogHandle;

    DPRINT("ElfrClearELFW()\n");

    lpLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!lpLogHandle)
    {
        return STATUS_INVALID_HANDLE;
    }

    /* Fail, if the log file is a backup file */
    if (lpLogHandle->Flags & LOG_HANDLE_BACKUP_FILE)
        return STATUS_INVALID_HANDLE;

    return LogfClearFile(lpLogHandle->LogFile,
                         (PUNICODE_STRING)BackupFileName);
}


/* Function 1 */
NTSTATUS ElfrBackupELFW(
    IELF_HANDLE LogHandle,
    PRPC_UNICODE_STRING BackupFileName)
{
    PLOGHANDLE lpLogHandle;

    DPRINT("ElfrBackupELFW()\n");

    lpLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!lpLogHandle)
    {
        return STATUS_INVALID_HANDLE;
    }

    return LogfBackupFile(lpLogHandle->LogFile,
                          (PUNICODE_STRING)BackupFileName);
}


/* Function 2 */
NTSTATUS ElfrCloseEL(
    IELF_HANDLE *LogHandle)
{
    return ElfDeleteEventLogHandle(*LogHandle);
}


/* Function 3 */
NTSTATUS ElfrDeregisterEventSource(
    IELF_HANDLE *LogHandle)
{
    return ElfDeleteEventLogHandle(*LogHandle);
}


/* Function 4 */
NTSTATUS ElfrNumberOfRecords(
    IELF_HANDLE LogHandle,
    DWORD *NumberOfRecords)
{
    PLOGHANDLE lpLogHandle;
    PLOGFILE lpLogFile;

    DPRINT("ElfrNumberOfRecords()\n");

    lpLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!lpLogHandle)
    {
        return STATUS_INVALID_HANDLE;
    }

    lpLogFile = lpLogHandle->LogFile;

    DPRINT("Oldest: %lu  Current: %lu\n",
           lpLogFile->Header.OldestRecordNumber,
           lpLogFile->Header.CurrentRecordNumber);

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

    *OldestRecordNumber = LogfGetOldestRecord(lpLogHandle->LogFile);

    return STATUS_SUCCESS;
}


/* Function 6 */
NTSTATUS ElfrChangeNotify(
    IELF_HANDLE *LogHandle,
    RPC_CLIENT_ID ClientId,
    DWORD Event)
{
    DPRINT("ElfrChangeNotify()");

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

    return ElfCreateEventLogHandle((PLOGHANDLE *)LogHandle,
                                   ModuleName->Buffer,
                                   FALSE);
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
    DPRINT("ElfrRegisterEventSourceW()\n");

    if ((MajorVersion != 1) || (MinorVersion != 1))
        return STATUS_INVALID_PARAMETER;

    /* RegModuleName must be an empty string */
    if (RegModuleName->Length > 0)
        return STATUS_INVALID_PARAMETER;

    DPRINT("ModuleName: %S\n", ModuleName->Buffer);

    /*FIXME: UNCServerName must specify the server or empty for local */

    /*FIXME: Must verify that caller has write access */

    return ElfCreateEventLogHandle((PLOGHANDLE *)LogHandle,
                                   ModuleName->Buffer,
                                   TRUE);
}


/* Function 9 */
NTSTATUS ElfrOpenBELW(
    EVENTLOG_HANDLE_W UNCServerName,
    PRPC_UNICODE_STRING BackupFileName,
    DWORD MajorVersion,
    DWORD MinorVersion,
    IELF_HANDLE *LogHandle)
{
    DPRINT("ElfrOpenBELW(%wZ)\n", BackupFileName);

    if ((MajorVersion != 1) || (MinorVersion != 1))
        return STATUS_INVALID_PARAMETER;

    /*FIXME: UNCServerName must specify the server */

    /*FIXME: Must verify that caller has read access */

    return ElfCreateBackupLogHandle((PLOGHANDLE *)LogHandle,
                                    (PUNICODE_STRING)BackupFileName);
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
        return STATUS_INVALID_PARAMETER;

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
                            NumberOfBytesToRead, Buffer, NumberOfBytesRead, MinNumberOfBytesNeeded,
                            FALSE);

    /* Update the handles CurrentRecord if success*/
    if (dwError == ERROR_SUCCESS)
    {
        lpLogHandle->CurrentRecord = RecordNumber;
    }

    /* HACK!!! */
    if (dwError == ERROR_HANDLE_EOF)
        return STATUS_END_OF_FILE;

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
    UNICODE_STRING BackupFileNameW;
    NTSTATUS Status;

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
NTSTATUS ElfrBackupELFA(
    IELF_HANDLE LogHandle,
    PRPC_STRING BackupFileName)
{
    UNICODE_STRING BackupFileNameW;
    NTSTATUS Status;

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
NTSTATUS ElfrOpenELA(
    EVENTLOG_HANDLE_A UNCServerName,
    PRPC_STRING ModuleName,
    PRPC_STRING RegModuleName,
    DWORD MajorVersion,
    DWORD MinorVersion,
    IELF_HANDLE *LogHandle)
{
    UNICODE_STRING ModuleNameW;
    NTSTATUS Status;

    if ((MajorVersion != 1) || (MinorVersion != 1))
        return STATUS_INVALID_PARAMETER;

    /* RegModuleName must be an empty string */
    if (RegModuleName->Length > 0)
        return STATUS_INVALID_PARAMETER;

    Status = RtlAnsiStringToUnicodeString(&ModuleNameW, (PANSI_STRING)ModuleName, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* FIXME: Must verify that caller has read access */

    Status = ElfCreateEventLogHandle((PLOGHANDLE *)LogHandle,
                                     ModuleNameW.Buffer,
                                     FALSE);

    RtlFreeUnicodeString(&ModuleNameW);

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
    UNICODE_STRING ModuleNameW;
    NTSTATUS Status;

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

    Status = ElfCreateEventLogHandle((PLOGHANDLE *)LogHandle,
                                     ModuleNameW.Buffer,
                                     TRUE);

    RtlFreeUnicodeString(&ModuleNameW);

    return Status;
}


/* Function 16 */
NTSTATUS ElfrOpenBELA(
    EVENTLOG_HANDLE_A UNCServerName,
    PRPC_STRING BackupFileName,
    DWORD MajorVersion,
    DWORD MinorVersion,
    IELF_HANDLE *LogHandle)
{
    UNICODE_STRING BackupFileNameW;
    NTSTATUS Status;

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

    /*FIXME: UNCServerName must specify the server */

    /*FIXME: Must verify that caller has read access */

    Status = ElfCreateBackupLogHandle((PLOGHANDLE *)LogHandle,
                                      &BackupFileNameW);

    RtlFreeUnicodeString(&BackupFileNameW);

    return Status;
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
    PLOGHANDLE lpLogHandle;
    DWORD dwError;
    DWORD RecordNumber;

    lpLogHandle = ElfGetLogHandleEntryByHandle(LogHandle);
    if (!lpLogHandle)
    {
        return STATUS_INVALID_HANDLE;
    }

    if (!Buffer)
        return STATUS_INVALID_PARAMETER;

    /* If sequential read, retrieve the CurrentRecord from this log handle */
    if (ReadFlags & EVENTLOG_SEQUENTIAL_READ)
    {
        RecordNumber = lpLogHandle->CurrentRecord;
    }
    else
    {
        RecordNumber = RecordOffset;
    }

    dwError = LogfReadEvent(lpLogHandle->LogFile,
                            ReadFlags,
                            &RecordNumber,
                            NumberOfBytesToRead,
                            Buffer,
                            NumberOfBytesRead,
                            MinNumberOfBytesNeeded,
                            TRUE);

    /* Update the handles CurrentRecord if success*/
    if (dwError == ERROR_SUCCESS)
    {
        lpLogHandle->CurrentRecord = RecordNumber;
    }

    /* HACK!!! */
    if (dwError == ERROR_HANDLE_EOF)
        return STATUS_END_OF_FILE;

    return I_RpcMapWin32Status(dwError);
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
    UNICODE_STRING ComputerNameW;
    PUNICODE_STRING *StringsArrayW = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
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
        StringsArrayW = HeapAlloc(MyHeap,
                                  HEAP_ZERO_MEMORY,
                                  NumStrings * sizeof (PUNICODE_STRING));
        if (StringsArrayW == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto Done;
        }

        for (i = 0; i < NumStrings; i++)
        {
            if (Strings[i] != NULL)
            {
                StringsArrayW[i] = HeapAlloc(MyHeap,
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
            if (StringsArrayW[i] != NULL)
            {
                if (StringsArrayW[i]->Buffer)
                {
                    RtlFreeUnicodeString(StringsArrayW[i]);
                    HeapFree(MyHeap, 0, StringsArrayW[i]);
                }
            }
        }

        HeapFree(MyHeap, 0, StringsArrayW);
    }

    RtlFreeUnicodeString(&ComputerNameW);

    return Status;
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
