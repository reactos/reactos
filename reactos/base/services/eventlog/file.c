/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/eventlog/file.c
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2005 Saveliy Tretiakov
                     Michael Martin
 */

/* INCLUDES *****************************************************************/

#include "eventlog.h"

#include <ndk/iofuncs.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

static LIST_ENTRY LogFileListHead;
static CRITICAL_SECTION LogFileListCs;

/* FUNCTIONS ****************************************************************/

static NTSTATUS
LogfInitializeNew(PLOGFILE LogFile,
                  ULONG ulMaxSize,
                  ULONG ulRetention)
{
    IO_STATUS_BLOCK IoStatusBlock;
    EVENTLOGEOF EofRec;
    NTSTATUS Status;

    ZeroMemory(&LogFile->Header, sizeof(EVENTLOGHEADER));
    SetFilePointer(LogFile->hFile, 0, NULL, FILE_BEGIN);
    SetEndOfFile(LogFile->hFile);

    LogFile->Header.HeaderSize = sizeof(EVENTLOGHEADER);
    LogFile->Header.Signature = LOGFILE_SIGNATURE;
    LogFile->Header.MajorVersion = MAJORVER;
    LogFile->Header.MinorVersion = MINORVER;
    LogFile->Header.StartOffset = sizeof(EVENTLOGHEADER);
    LogFile->Header.EndOffset = sizeof(EVENTLOGHEADER);
    LogFile->Header.CurrentRecordNumber = 1;
    LogFile->Header.OldestRecordNumber = 1;
    LogFile->Header.MaxSize = ulMaxSize;
    LogFile->Header.Flags = 0;
    LogFile->Header.Retention = ulRetention;
    LogFile->Header.EndHeaderSize = sizeof(EVENTLOGHEADER);

    Status = NtWriteFile(LogFile->hFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &LogFile->Header,
                         sizeof(EVENTLOGHEADER),
                         NULL,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    EofRec.RecordSizeBeginning = sizeof(EVENTLOGEOF);
    EofRec.Ones = 0x11111111;
    EofRec.Twos = 0x22222222;
    EofRec.Threes = 0x33333333;
    EofRec.Fours = 0x44444444;
    EofRec.BeginRecord = LogFile->Header.StartOffset;
    EofRec.EndRecord = LogFile->Header.EndOffset;
    EofRec.CurrentRecordNumber = LogFile->Header.CurrentRecordNumber;
    EofRec.OldestRecordNumber = LogFile->Header.OldestRecordNumber;
    EofRec.RecordSizeEnd = sizeof(EVENTLOGEOF);

    Status = NtWriteFile(LogFile->hFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &EofRec,
                         sizeof(EVENTLOGEOF),
                         NULL,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    Status = NtFlushBuffersFile(LogFile->hFile,
                                &IoStatusBlock);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtFlushBuffersFile failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}


static NTSTATUS
LogfInitializeExisting(PLOGFILE LogFile,
                       BOOL Backup)
{
    DWORD dwRead;
    DWORD dwRecordsNumber = 0;
    DWORD dwRecSize, dwRecSign, dwFilePointer;
    PDWORD pdwRecSize2;
    PEVENTLOGRECORD RecBuf;
    BOOL OvewrWrittenRecords = FALSE;

    DPRINT("Initializing LogFile %S\n",LogFile->LogName);

    if (SetFilePointer(LogFile->hFile, 0, NULL, FILE_BEGIN) ==
        INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer failed! %d\n", GetLastError());
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    if (!ReadFile(LogFile->hFile,
                  &LogFile->Header,
                  sizeof(EVENTLOGHEADER),
                  &dwRead,
                  NULL))
    {
        DPRINT1("ReadFile failed! %d\n", GetLastError());
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    if (dwRead != sizeof(EVENTLOGHEADER))
    {
        DPRINT("EventLog: Invalid file %S.\n", LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    if (LogFile->Header.HeaderSize != sizeof(EVENTLOGHEADER) ||
        LogFile->Header.EndHeaderSize != sizeof(EVENTLOGHEADER))
    {
        DPRINT("EventLog: Invalid header size in %S.\n", LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    if (LogFile->Header.Signature != LOGFILE_SIGNATURE)
    {
        DPRINT("EventLog: Invalid signature %x in %S.\n",
               LogFile->Header.Signature, LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    if (LogFile->Header.EndOffset > GetFileSize(LogFile->hFile, NULL) + 1)
    {
        DPRINT("EventLog: Invalid eof offset %x in %S.\n",
               LogFile->Header.EndOffset, LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    /* Set the read location to the oldest record */
    dwFilePointer = SetFilePointer(LogFile->hFile, LogFile->Header.StartOffset, NULL, FILE_BEGIN);
    if (dwFilePointer == INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer failed! %d\n", GetLastError());
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    for (;;)
    {
        dwFilePointer = SetFilePointer(LogFile->hFile, 0, NULL, FILE_CURRENT);

        if (dwFilePointer == INVALID_SET_FILE_POINTER)
        {
            DPRINT1("SetFilePointer failed! %d\n", GetLastError());
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }

        /* If the EVENTLOGEOF info has been reached and the oldest record was not immediately after the Header */
        if ((dwFilePointer == LogFile->Header.EndOffset) && (LogFile->Header.StartOffset != sizeof(EVENTLOGHEADER)))
        {
            OvewrWrittenRecords = TRUE;
            /* The file has records that overwrote old ones so read them */
            dwFilePointer = SetFilePointer(LogFile->hFile, sizeof(EVENTLOGHEADER), NULL, FILE_BEGIN);
        }

        if (!ReadFile(LogFile->hFile,
                      &dwRecSize,
                      sizeof(dwRecSize),
                      &dwRead,
                      NULL))
        {
            DPRINT1("ReadFile failed! %d\n", GetLastError());
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }

        if (dwRead != sizeof(dwRecSize))
            break;

        if (!ReadFile(LogFile->hFile,
                      &dwRecSign,
                      sizeof(dwRecSign),
                      &dwRead,
                      NULL))
        {
            DPRINT1("ReadFile() failed! %d\n", GetLastError());
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }

        if (dwRead != sizeof(dwRecSize))
            break;

        if (dwRecSign != LOGFILE_SIGNATURE ||
            dwRecSize + dwFilePointer > GetFileSize(LogFile->hFile, NULL) + 1 ||
            dwRecSize < sizeof(EVENTLOGRECORD))
        {
            break;
        }

        if (SetFilePointer(LogFile->hFile,
                           -((LONG) sizeof(DWORD) * 2),
                           NULL,
                           FILE_CURRENT) == INVALID_SET_FILE_POINTER)
        {
            DPRINT1("SetFilePointer() failed! %d", GetLastError());
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }

        RecBuf = (PEVENTLOGRECORD) HeapAlloc(MyHeap, 0, dwRecSize);
        if (RecBuf == NULL)
        {
            DPRINT1("Can't allocate heap!\n");
            return STATUS_NO_MEMORY;
        }

        if (!ReadFile(LogFile->hFile, RecBuf, dwRecSize, &dwRead, NULL))
        {
            DPRINT1("ReadFile() failed! %d\n", GetLastError());
            HeapFree(MyHeap, 0, RecBuf);
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }

        if (dwRead != dwRecSize)
        {
            HeapFree(MyHeap, 0, RecBuf);
            break;
        }

        /* if OvewrWrittenRecords is TRUE and this record has already been read */
        if ((OvewrWrittenRecords == TRUE) && (RecBuf->RecordNumber == LogFile->Header.OldestRecordNumber))
        {
            HeapFree(MyHeap, 0, RecBuf);
            break;
        }

        pdwRecSize2 = (PDWORD) (((PBYTE) RecBuf) + dwRecSize - 4);

        if (*pdwRecSize2 != dwRecSize)
        {
            DPRINT1("Invalid RecordSizeEnd of record %d (%x) in %S\n",
                    dwRecordsNumber, *pdwRecSize2, LogFile->LogName);
            HeapFree(MyHeap, 0, RecBuf);
            break;
        }

        dwRecordsNumber++;

        if (!LogfAddOffsetInformation(LogFile,
                                      RecBuf->RecordNumber,
                                      dwFilePointer))
        {
            DPRINT1("LogfAddOffsetInformation() failed!\n");
            HeapFree(MyHeap, 0, RecBuf);
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }

        HeapFree(MyHeap, 0, RecBuf);
    }

    LogFile->Header.CurrentRecordNumber = dwRecordsNumber + LogFile->Header.OldestRecordNumber;
    if (LogFile->Header.CurrentRecordNumber == 0)
        LogFile->Header.CurrentRecordNumber = 1;

    if (!Backup)
    {
        if (SetFilePointer(LogFile->hFile, 0, NULL, FILE_BEGIN) ==
            INVALID_SET_FILE_POINTER)
        {
            DPRINT1("SetFilePointer() failed! %d\n", GetLastError());
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }

        if (!WriteFile(LogFile->hFile,
                       &LogFile->Header,
                       sizeof(EVENTLOGHEADER),
                       &dwRead,
                       NULL))
        {
            DPRINT1("WriteFile failed! %d\n", GetLastError());
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }

        if (!FlushFileBuffers(LogFile->hFile))
        {
            DPRINT1("FlushFileBuffers failed! %d\n", GetLastError());
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
LogfCreate(PLOGFILE *LogFile,
           WCHAR *LogName,
           PUNICODE_STRING FileName,
           ULONG ulMaxSize,
           ULONG ulRetention,
           BOOL Permanent,
           BOOL Backup)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PLOGFILE pLogFile;
    BOOL bCreateNew = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    pLogFile = (LOGFILE *) HeapAlloc(MyHeap, HEAP_ZERO_MEMORY, sizeof(LOGFILE));
    if (!pLogFile)
    {
        DPRINT1("Can't allocate heap!\n");
        return STATUS_NO_MEMORY;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&pLogFile->hFile,
                          Backup ? (GENERIC_READ | SYNCHRONIZE) : (GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE),
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ,
                          Backup ? FILE_OPEN : FILE_OPEN_IF,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't create file %wZ (Status: 0x%08lx)\n", FileName, Status);
        goto fail;
    }

    bCreateNew = (IoStatusBlock.Information == FILE_CREATED) ? TRUE: FALSE;

    pLogFile->LogName =
        (WCHAR *) HeapAlloc(MyHeap,
                            HEAP_ZERO_MEMORY,
                            (lstrlenW(LogName) + 1) * sizeof(WCHAR));
    if (pLogFile->LogName == NULL)
    {
        DPRINT1("Can't allocate heap\n");
        Status = STATUS_NO_MEMORY;
        goto fail;
    }

    if (LogName)
        StringCchCopy(pLogFile->LogName,lstrlenW(LogName) + 1, LogName);

    pLogFile->FileName =
        (WCHAR *) HeapAlloc(MyHeap,
                            HEAP_ZERO_MEMORY,
                            (lstrlenW(FileName->Buffer) + 1) * sizeof(WCHAR));
    if (pLogFile->FileName == NULL)
    {
        DPRINT1("Can't allocate heap\n");
        Status = STATUS_NO_MEMORY;
        goto fail;
    }

    StringCchCopy(pLogFile->FileName, lstrlenW(FileName->Buffer) + 1, FileName->Buffer);

    pLogFile->OffsetInfo =
        (PEVENT_OFFSET_INFO) HeapAlloc(MyHeap,
                                       HEAP_ZERO_MEMORY,
                                       sizeof(EVENT_OFFSET_INFO) * 64);
    if (pLogFile->OffsetInfo == NULL)
    {
        DPRINT1("Can't allocate heap\n");
        Status = STATUS_NO_MEMORY;
        goto fail;
    }

    pLogFile->OffsetInfoSize = 64;

    pLogFile->Permanent = Permanent;

    if (bCreateNew)
        Status = LogfInitializeNew(pLogFile, ulMaxSize, ulRetention);
    else
        Status = LogfInitializeExisting(pLogFile, Backup);

    if (!NT_SUCCESS(Status))
        goto fail;

    RtlInitializeResource(&pLogFile->Lock);

    LogfListAddItem(pLogFile);

  fail:
    if (!NT_SUCCESS(Status))
    {
        if ((pLogFile->hFile != NULL) && (pLogFile->hFile != INVALID_HANDLE_VALUE))
            CloseHandle(pLogFile->hFile);

        if (pLogFile->OffsetInfo)
            HeapFree(MyHeap, 0, pLogFile->OffsetInfo);

        if (pLogFile->FileName)
            HeapFree(MyHeap, 0, pLogFile->FileName);

        if (pLogFile->LogName)
            HeapFree(MyHeap, 0, pLogFile->LogName);

        HeapFree(MyHeap, 0, pLogFile);
    }
    else
    {
        *LogFile = pLogFile;
    }

    return Status;
}


VOID
LogfClose(PLOGFILE LogFile,
          BOOL ForceClose)
{
    if (LogFile == NULL)
        return;

    if ((ForceClose == FALSE) &&
        (LogFile->Permanent == TRUE))
        return;

    RtlAcquireResourceExclusive(&LogFile->Lock, TRUE);

    FlushFileBuffers(LogFile->hFile);
    CloseHandle(LogFile->hFile);
    LogfListRemoveItem(LogFile);

    RtlDeleteResource(&LogFile->Lock);

    HeapFree(MyHeap, 0, LogFile->LogName);
    HeapFree(MyHeap, 0, LogFile->FileName);
    HeapFree(MyHeap, 0, LogFile->OffsetInfo);
    HeapFree(MyHeap, 0, LogFile);

    return;
}

VOID LogfCloseAll(VOID)
{
    while (!IsListEmpty(&LogFileListHead))
    {
        LogfClose(LogfListHead(), TRUE);
    }

    DeleteCriticalSection(&LogFileListCs);
}

VOID LogfListInitialize(VOID)
{
    InitializeCriticalSection(&LogFileListCs);
    InitializeListHead(&LogFileListHead);
}

PLOGFILE LogfListHead(VOID)
{
    return CONTAINING_RECORD(LogFileListHead.Flink, LOGFILE, ListEntry);
}

PLOGFILE LogfListItemByName(WCHAR * Name)
{
    PLIST_ENTRY CurrentEntry;
    PLOGFILE Result = NULL;

    EnterCriticalSection(&LogFileListCs);

    CurrentEntry = LogFileListHead.Flink;
    while (CurrentEntry != &LogFileListHead)
    {
        PLOGFILE Item = CONTAINING_RECORD(CurrentEntry,
                                          LOGFILE,
                                          ListEntry);

        if (Item->LogName && !lstrcmpi(Item->LogName, Name))
        {
            Result = Item;
            break;
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    LeaveCriticalSection(&LogFileListCs);
    return Result;
}

/* Index starting from 1 */
INT LogfListItemIndexByName(WCHAR * Name)
{
    PLIST_ENTRY CurrentEntry;
    INT Result = 0;
    INT i = 1;

    EnterCriticalSection(&LogFileListCs);

    CurrentEntry = LogFileListHead.Flink;
    while (CurrentEntry != &LogFileListHead)
    {
        PLOGFILE Item = CONTAINING_RECORD(CurrentEntry,
                                          LOGFILE,
                                          ListEntry);

        if (Item->LogName && !lstrcmpi(Item->LogName, Name))
        {
            Result = i;
            break;
        }

        CurrentEntry = CurrentEntry->Flink;
        i++;
    }

    LeaveCriticalSection(&LogFileListCs);
    return Result;
}

/* Index starting from 1 */
PLOGFILE LogfListItemByIndex(INT Index)
{
    PLIST_ENTRY CurrentEntry;
    PLOGFILE Result = NULL;
    INT i = 1;

    EnterCriticalSection(&LogFileListCs);

    CurrentEntry = LogFileListHead.Flink;
    while (CurrentEntry != &LogFileListHead)
    {
        if (i == Index)
        {
            Result = CONTAINING_RECORD(CurrentEntry, LOGFILE, ListEntry);
            break;
        }

        CurrentEntry = CurrentEntry->Flink;
        i++;
    }

    LeaveCriticalSection(&LogFileListCs);
    return Result;
}

INT LogfListItemCount(VOID)
{
    PLIST_ENTRY CurrentEntry;
    INT i = 0;

    EnterCriticalSection(&LogFileListCs);

    CurrentEntry = LogFileListHead.Flink;
    while (CurrentEntry != &LogFileListHead)
    {
        CurrentEntry = CurrentEntry->Flink;
        i++;
    }

    LeaveCriticalSection(&LogFileListCs);
    return i;
}

VOID LogfListAddItem(PLOGFILE Item)
{
    EnterCriticalSection(&LogFileListCs);
    InsertTailList(&LogFileListHead, &Item->ListEntry);
    LeaveCriticalSection(&LogFileListCs);
}

VOID LogfListRemoveItem(PLOGFILE Item)
{
    EnterCriticalSection(&LogFileListCs);
    RemoveEntryList(&Item->ListEntry);
    LeaveCriticalSection(&LogFileListCs);
}

static BOOL
ReadAnsiLogEntry(HANDLE hFile,
                 LPVOID lpBuffer,
                 DWORD nNumberOfBytesToRead,
                 LPDWORD lpNumberOfBytesRead)
{
    PEVENTLOGRECORD Dst;
    PEVENTLOGRECORD Src;
    ANSI_STRING StringA;
    UNICODE_STRING StringW;
    LPWSTR SrcPtr;
    LPSTR DstPtr;
    LPWSTR SrcString;
    LPSTR DstString;
    LPVOID lpUnicodeBuffer = NULL;
    DWORD dwRead = 0;
    DWORD i;
    DWORD dwPadding;
    DWORD dwEntryLength;
    PDWORD pLength;
    NTSTATUS Status;
    BOOL ret = TRUE;

    *lpNumberOfBytesRead = 0;

    lpUnicodeBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nNumberOfBytesToRead);
    if (lpUnicodeBuffer == NULL)
    {
        DPRINT1("Alloc failed!\n");
        return FALSE;
    }

    if (!ReadFile(hFile, lpUnicodeBuffer, nNumberOfBytesToRead, &dwRead, NULL))
    {
        DPRINT1("Read failed!\n");
        ret = FALSE;
        goto done;
    }

    Dst = (PEVENTLOGRECORD)lpBuffer;
    Src = (PEVENTLOGRECORD)lpUnicodeBuffer;

    Dst->TimeGenerated = Src->TimeGenerated;
    Dst->Reserved = Src->Reserved;
    Dst->RecordNumber = Src->RecordNumber;
    Dst->TimeWritten = Src->TimeWritten;
    Dst->EventID = Src->EventID;
    Dst->EventType = Src->EventType;
    Dst->EventCategory = Src->EventCategory;
    Dst->NumStrings = Src->NumStrings;
    Dst->UserSidLength = Src->UserSidLength;
    Dst->DataLength = Src->DataLength;

    SrcPtr = (LPWSTR)((DWORD_PTR)Src + sizeof(EVENTLOGRECORD));
    DstPtr = (LPSTR)((DWORD_PTR)Dst + sizeof(EVENTLOGRECORD));

    /* Convert the module name */
    RtlInitUnicodeString(&StringW, SrcPtr);
    Status = RtlUnicodeStringToAnsiString(&StringA, &StringW, TRUE);
    if (NT_SUCCESS(Status))
    {
        RtlCopyMemory(DstPtr, StringA.Buffer, StringA.MaximumLength);

        DstPtr = (PVOID)((DWORD_PTR)DstPtr + StringA.MaximumLength);

        RtlFreeAnsiString(&StringA);
    }

    /* Convert the computer name */
    if (NT_SUCCESS(Status))
    {
        SrcPtr = (PWSTR)((DWORD_PTR)SrcPtr + StringW.MaximumLength);

        RtlInitUnicodeString(&StringW, SrcPtr);
        Status = RtlUnicodeStringToAnsiString(&StringA, &StringW, TRUE);
        if (NT_SUCCESS(Status))
        {
            RtlCopyMemory(DstPtr, StringA.Buffer, StringA.MaximumLength);

            DstPtr = (PVOID)((DWORD_PTR)DstPtr + StringA.MaximumLength);

            RtlFreeAnsiString(&StringA);
        }
    }

    /* Add the padding and the User SID*/
    if (NT_SUCCESS(Status))
    {
        dwPadding = sizeof(DWORD) - (((DWORD_PTR)DstPtr - (DWORD_PTR)Dst) % sizeof(DWORD));
        RtlZeroMemory(DstPtr, dwPadding);

        DstPtr = (LPSTR)((DWORD_PTR)DstPtr + dwPadding);
        RtlCopyMemory(DstPtr,
                      (PVOID)((DWORD_PTR)Src + Src->UserSidOffset),
                      Src->UserSidLength);

        Dst->UserSidOffset = (DWORD)((DWORD_PTR)DstPtr - (DWORD_PTR)Dst);
    }


    /* Convert the strings */
    if (NT_SUCCESS(Status))
    {
        DstPtr = (PVOID)((DWORD_PTR)DstPtr + Src->UserSidLength);

        SrcString = (LPWSTR)((DWORD_PTR)Src + (DWORD)Src->StringOffset);
        DstString = (LPSTR)DstPtr;

        for (i = 0; i < Dst->NumStrings; i++)
        {
            RtlInitUnicodeString(&StringW, SrcString);

            RtlUnicodeStringToAnsiString(&StringA, &StringW, TRUE);

            RtlCopyMemory(DstString, StringA.Buffer, StringA.MaximumLength);

            SrcString = (LPWSTR)((DWORD_PTR)SrcString +
                                 (DWORD)StringW.MaximumLength);

            DstString = (LPSTR)((DWORD_PTR)DstString +
                                (DWORD)StringA.MaximumLength);

            RtlFreeAnsiString(&StringA);
        }

        Dst->StringOffset = (DWORD)((DWORD_PTR)DstPtr - (DWORD_PTR)Dst);


        /* Copy the binary data */
        DstPtr = (PVOID)DstString;
        Dst->DataOffset = (DWORD_PTR)DstPtr - (DWORD_PTR)Dst;

        RtlCopyMemory(DstPtr, (PVOID)((DWORD_PTR)Src + Src->DataOffset), Src->DataLength);

        /* Add the padding */
        DstPtr = (PVOID)((DWORD_PTR)DstPtr + Src->DataLength);
        dwPadding = sizeof(DWORD) - (((DWORD_PTR)DstPtr-(DWORD_PTR)Dst) % sizeof(DWORD));
        RtlZeroMemory(DstPtr, dwPadding);

        dwEntryLength = (DWORD)((DWORD_PTR)DstPtr + dwPadding + sizeof(DWORD) - (DWORD_PTR)Dst);

        /* Set the entry length at the end of the entry*/
        pLength = (PDWORD)((DWORD_PTR)DstPtr + dwPadding);
        *pLength = dwEntryLength;
        Dst->Length = dwEntryLength;

        *lpNumberOfBytesRead = dwEntryLength;
    }

done:
    if (lpUnicodeBuffer != NULL)
        HeapFree(GetProcessHeap(), 0, lpUnicodeBuffer);

    return ret;
}


DWORD LogfReadEvent(PLOGFILE LogFile,
                   DWORD Flags,
                   DWORD * RecordNumber,
                   DWORD BufSize,
                   PBYTE Buffer,
                   DWORD * BytesRead,
                   DWORD * BytesNeeded,
                   BOOL Ansi)
{
    DWORD dwOffset, dwRead, dwRecSize;
    DWORD dwBufferUsage = 0, dwRecNum;

    if (Flags & EVENTLOG_FORWARDS_READ && Flags & EVENTLOG_BACKWARDS_READ)
        return ERROR_INVALID_PARAMETER;

    if (!(Flags & EVENTLOG_FORWARDS_READ) && !(Flags & EVENTLOG_BACKWARDS_READ))
        return ERROR_INVALID_PARAMETER;

    if (!Buffer || !BytesRead || !BytesNeeded)
        return ERROR_INVALID_PARAMETER;

    if ((*RecordNumber==0) && !(EVENTLOG_SEQUENTIAL_READ))
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwRecNum = *RecordNumber;

    RtlAcquireResourceShared(&LogFile->Lock, TRUE);

    *BytesRead = 0;
    *BytesNeeded = 0;

    dwOffset = LogfOffsetByNumber(LogFile, dwRecNum);

    if (!dwOffset)
    {
        RtlReleaseResource(&LogFile->Lock);
        return ERROR_HANDLE_EOF;
    }

    if (SetFilePointer(LogFile->hFile, dwOffset, NULL, FILE_BEGIN) ==
        INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer() failed!\n");
        goto Done;
    }

    if (!ReadFile(LogFile->hFile, &dwRecSize, sizeof(DWORD), &dwRead, NULL))
    {
        DPRINT1("ReadFile() failed!\n");
        goto Done;
    }

    if (dwRecSize > BufSize)
    {
        *BytesNeeded = dwRecSize;
        RtlReleaseResource(&LogFile->Lock);
        return ERROR_INSUFFICIENT_BUFFER;
    }

    if (SetFilePointer(LogFile->hFile,
                       -((LONG) sizeof(DWORD)),
                       NULL,
                       FILE_CURRENT) == INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer() failed!\n");
        goto Done;
    }

    if (Ansi == TRUE)
    {
        if (!ReadAnsiLogEntry(LogFile->hFile, Buffer, dwRecSize, &dwRead))
        {
            DPRINT1("ReadAnsiLogEntry() failed!\n");
            goto Done;
        }
    }
    else
    {
        if (!ReadFile(LogFile->hFile, Buffer, dwRecSize, &dwRead, NULL))
        {
            DPRINT1("ReadFile() failed!\n");
            goto Done;
        }
    }

    dwBufferUsage += dwRead;

    while (dwBufferUsage <= BufSize)
    {
        if (Flags & EVENTLOG_FORWARDS_READ)
            dwRecNum++;
        else
            dwRecNum--;

        dwOffset = LogfOffsetByNumber(LogFile, dwRecNum);
        if (!dwOffset)
            break;

        if (SetFilePointer(LogFile->hFile, dwOffset, NULL, FILE_BEGIN) ==
            INVALID_SET_FILE_POINTER)
        {
            DPRINT1("SetFilePointer() failed!\n");
            goto Done;
        }

        if (!ReadFile(LogFile->hFile,
                      &dwRecSize,
                      sizeof(DWORD),
                      &dwRead,
                      NULL))
        {
            DPRINT1("ReadFile() failed!\n");
            goto Done;
        }

        if (dwBufferUsage + dwRecSize > BufSize)
            break;

        if (SetFilePointer(LogFile->hFile,
                           -((LONG) sizeof(DWORD)),
                           NULL,
                           FILE_CURRENT) == INVALID_SET_FILE_POINTER)
        {
            DPRINT1("SetFilePointer() failed!\n");
            goto Done;
        }

        if (Ansi == TRUE)
        {
            if (!ReadAnsiLogEntry(LogFile->hFile,
                                  Buffer + dwBufferUsage,
                                  dwRecSize,
                                  &dwRead))
            {
                DPRINT1("ReadAnsiLogEntry() failed!\n");
                goto Done;
            }
        }
        else
        {
            if (!ReadFile(LogFile->hFile,
                          Buffer + dwBufferUsage,
                          dwRecSize,
                          &dwRead,
                          NULL))
            {
                DPRINT1("ReadFile() failed!\n");
                goto Done;
            }
        }

        dwBufferUsage += dwRead;
    }

    *BytesRead = dwBufferUsage;
    * RecordNumber = dwRecNum;
    RtlReleaseResource(&LogFile->Lock);
    return ERROR_SUCCESS;

Done:
    DPRINT1("LogfReadEvent failed with %x\n",GetLastError());
    RtlReleaseResource(&LogFile->Lock);
    return GetLastError();
}

BOOL LogfWriteData(PLOGFILE LogFile, DWORD BufSize, PBYTE Buffer)
{
    DWORD dwWritten;
    DWORD dwRead;
    SYSTEMTIME st;
    EVENTLOGEOF EofRec;
    PEVENTLOGRECORD RecBuf;
    LARGE_INTEGER logFileSize;
    ULONG RecOffSet;
    ULONG WriteOffSet;

    if (!Buffer)
        return FALSE;

    GetSystemTime(&st);
    SystemTimeToEventTime(&st, &((PEVENTLOGRECORD) Buffer)->TimeWritten);

    RtlAcquireResourceExclusive(&LogFile->Lock, TRUE);

    if (!GetFileSizeEx(LogFile->hFile, &logFileSize))
    {
        RtlReleaseResource(&LogFile->Lock);
        return FALSE;
    }

    /* If the size of the file is over MaxSize */
    if ((logFileSize.QuadPart + BufSize)> LogFile->Header.MaxSize)
    {
        ULONG OverWriteLength = 0;
        WriteOffSet = LogfOffsetByNumber(LogFile, LogFile->Header.OldestRecordNumber);
        RecBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(EVENTLOGRECORD));
        /* Determine how many records need to be overwritten */
        while (TRUE)
        {
            DPRINT("EventLogFile has reached maximume size\n");

            if (!RecBuf)
            {
                DPRINT1("Failed to allocate buffer for OldestRecord!\n");
                HeapFree(GetProcessHeap(), 0, RecBuf);
                RtlReleaseResource(&LogFile->Lock);
                return FALSE;
            }

            /* Get the oldest record data */
            RecOffSet = LogfOffsetByNumber(LogFile, LogFile->Header.OldestRecordNumber);

            if (SetFilePointer(LogFile->hFile,
                               RecOffSet,
                               NULL,
                               FILE_BEGIN) == INVALID_SET_FILE_POINTER)
            {
                DPRINT1("SetFilePointer() failed! %d\n", GetLastError());
                HeapFree(GetProcessHeap(), 0, RecBuf);
                RtlReleaseResource(&LogFile->Lock);
                return FALSE;
            }

            if (!ReadFile(LogFile->hFile, RecBuf, sizeof(EVENTLOGRECORD), &dwRead, NULL))
            {
                DPRINT1("ReadFile() failed!\n");
                HeapFree(GetProcessHeap(), 0, RecBuf);
                RtlReleaseResource(&LogFile->Lock);
                return FALSE;
            }

            if (RecBuf->Reserved != LOGFILE_SIGNATURE)
            {
                DPRINT1("LogFile corrupt!\n");
                HeapFree(GetProcessHeap(), 0, RecBuf);
                RtlReleaseResource(&LogFile->Lock);
                return FALSE;
            }

            LogfDeleteOffsetInformation(LogFile,LogFile->Header.OldestRecordNumber);

            LogFile->Header.OldestRecordNumber++;

            OverWriteLength += RecBuf->Length;
            /* Check the size of the record as the record adding may be larger */
            if (OverWriteLength >= BufSize)
            {
                DPRINT("Record will fit. Length %d, BufSize %d\n", OverWriteLength, BufSize);
                LogFile->Header.StartOffset = LogfOffsetByNumber(LogFile, LogFile->Header.OldestRecordNumber);
                break;
            }
        }
        HeapFree(GetProcessHeap(), 0, RecBuf);
    }
    else
        WriteOffSet = LogFile->Header.EndOffset;

    if (SetFilePointer(LogFile->hFile,
                       WriteOffSet,
                       NULL,
                       FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer() failed! %d\n", GetLastError());
        RtlReleaseResource(&LogFile->Lock);
        return FALSE;
    }

    if (!WriteFile(LogFile->hFile, Buffer, BufSize, &dwWritten, NULL))
    {
        DPRINT1("WriteFile() failed! %d\n", GetLastError());
        RtlReleaseResource(&LogFile->Lock);
        return FALSE;
    }

    if (!LogfAddOffsetInformation(LogFile,
                                  LogFile->Header.CurrentRecordNumber,
                                  WriteOffSet))
    {
        RtlReleaseResource(&LogFile->Lock);
        return FALSE;
    }

    LogFile->Header.CurrentRecordNumber++;

    if (WriteOffSet == LogFile->Header.EndOffset)
    {
        LogFile->Header.EndOffset += dwWritten;
    }
    if (SetFilePointer(LogFile->hFile,
                       LogFile->Header.EndOffset,
                       NULL,
                       FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer() failed! %d\n", GetLastError());
        RtlReleaseResource(&LogFile->Lock);
        return FALSE;
    }

    EofRec.Ones = 0x11111111;
    EofRec.Twos = 0x22222222;
    EofRec.Threes = 0x33333333;
    EofRec.Fours = 0x44444444;
    EofRec.RecordSizeBeginning = sizeof(EVENTLOGEOF);
    EofRec.RecordSizeEnd = sizeof(EVENTLOGEOF);
    EofRec.CurrentRecordNumber = LogFile->Header.CurrentRecordNumber;
    EofRec.OldestRecordNumber = LogFile->Header.OldestRecordNumber;
    EofRec.BeginRecord = LogFile->Header.StartOffset;
    EofRec.EndRecord = LogFile->Header.EndOffset;

    if (!WriteFile(LogFile->hFile,
                   &EofRec,
                   sizeof(EVENTLOGEOF),
                   &dwWritten,
                   NULL))
    {
        DPRINT1("WriteFile() failed! %d\n", GetLastError());
        RtlReleaseResource(&LogFile->Lock);
        return FALSE;
    }

    if (SetFilePointer(LogFile->hFile, 0, NULL, FILE_BEGIN) ==
        INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer() failed! %d\n", GetLastError());
        RtlReleaseResource(&LogFile->Lock);
        return FALSE;
    }

    if (!WriteFile(LogFile->hFile,
                   &LogFile->Header,
                   sizeof(EVENTLOGHEADER),
                   &dwWritten,
                   NULL))
    {
        DPRINT1("WriteFile failed! LastError = %d\n", GetLastError());
        RtlReleaseResource(&LogFile->Lock);
        return FALSE;
    }

    if (!FlushFileBuffers(LogFile->hFile))
    {
        DPRINT1("FlushFileBuffers() failed! %d\n", GetLastError());
        RtlReleaseResource(&LogFile->Lock);
        return FALSE;
    }

    RtlReleaseResource(&LogFile->Lock);
    return TRUE;
}


NTSTATUS
LogfClearFile(PLOGFILE LogFile,
              PUNICODE_STRING BackupFileName)
{
    NTSTATUS Status;

    RtlAcquireResourceExclusive(&LogFile->Lock, TRUE);

    if (BackupFileName->Length > 0)
    {
        /* Write a backup file */
        Status = LogfBackupFile(LogFile,
                                BackupFileName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("LogfBackupFile failed (Status: 0x%08lx)\n", Status);
            return Status;
        }
    }

    Status = LogfInitializeNew(LogFile,
                               LogFile->Header.MaxSize,
                               LogFile->Header.Retention);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LogfInitializeNew failed (Status: 0x%08lx)\n", Status);
    }

    RtlReleaseResource(&LogFile->Lock);

    return Status;
}


NTSTATUS
LogfBackupFile(PLOGFILE LogFile,
               PUNICODE_STRING BackupFileName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    EVENTLOGHEADER Header;
    EVENTLOGEOF EofRec;
    HANDLE FileHandle = NULL;
    ULONG i;
    LARGE_INTEGER FileOffset;
    NTSTATUS Status;
    PUCHAR Buffer = NULL;

    DWORD dwOffset, dwRead, dwRecSize;

    DPRINT1("LogfBackupFile(%p, %wZ)\n", LogFile, BackupFileName);

    /* Lock the log file shared */
    RtlAcquireResourceShared(&LogFile->Lock, TRUE);

    InitializeObjectAttributes(&ObjectAttributes,
                               BackupFileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&FileHandle,
                          GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ,
                          FILE_CREATE,
                          FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Can't create backup file %wZ (Status: 0x%08lx)\n", BackupFileName, Status);
        goto Done;
    }

    /* Initialize the (dirty) log file header */
    Header.HeaderSize = sizeof(EVENTLOGHEADER);
    Header.Signature = LOGFILE_SIGNATURE;
    Header.MajorVersion = MAJORVER;
    Header.MinorVersion = MINORVER;
    Header.StartOffset = sizeof(EVENTLOGHEADER);
    Header.EndOffset = sizeof(EVENTLOGHEADER);
    Header.CurrentRecordNumber = 1;
    Header.OldestRecordNumber = 1;
    Header.MaxSize = LogFile->Header.MaxSize;
    Header.Flags = ELF_LOGFILE_HEADER_DIRTY;
    Header.Retention = LogFile->Header.Retention;
    Header.EndHeaderSize = sizeof(EVENTLOGHEADER);

    /* Write the (dirty) log file header */
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &Header,
                         sizeof(EVENTLOGHEADER),
                         NULL,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to write the log file header (Status: 0x%08lx)\n", Status);
        goto Done;
    }

    for (i = LogFile->Header.OldestRecordNumber; i < LogFile->Header.CurrentRecordNumber; i++)
    {
        dwOffset = LogfOffsetByNumber(LogFile, i);
        if (dwOffset == 0)
            break;

        if (SetFilePointer(LogFile->hFile, dwOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            DPRINT1("SetFilePointer() failed!\n");
            goto Done;
        }

        if (!ReadFile(LogFile->hFile, &dwRecSize, sizeof(DWORD), &dwRead, NULL))
        {
            DPRINT1("ReadFile() failed!\n");
            goto Done;
        }

        if (SetFilePointer(LogFile->hFile, dwOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            DPRINT1("SetFilePointer() failed!\n");
            goto Done;
        }

        Buffer = HeapAlloc(MyHeap, 0, dwRecSize);
        if (Buffer == NULL)
        {
            DPRINT1("HeapAlloc() failed!\n");
            goto Done;
        }

        if (!ReadFile(LogFile->hFile, Buffer, dwRecSize, &dwRead, NULL))
        {
            DPRINT1("ReadFile() failed!\n");
            goto Done;
        }

        /* Write the event record */
        Status = NtWriteFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             Buffer,
                             dwRecSize,
                             NULL,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtWriteFile() failed! (Status: 0x%08lx)\n", Status);
            goto Done;
        }

        /* Update the header information */
        Header.EndOffset += dwRecSize;

        /* Free the buffer */
        HeapFree(MyHeap, 0, Buffer);
        Buffer = NULL;
    }

    /* Initialize the EOF record */
    EofRec.RecordSizeBeginning = sizeof(EVENTLOGEOF);
    EofRec.Ones = 0x11111111;
    EofRec.Twos = 0x22222222;
    EofRec.Threes = 0x33333333;
    EofRec.Fours = 0x44444444;
    EofRec.BeginRecord = sizeof(EVENTLOGHEADER);
    EofRec.EndRecord = Header.EndOffset;
    EofRec.CurrentRecordNumber = LogFile->Header.CurrentRecordNumber;
    EofRec.OldestRecordNumber = LogFile->Header.OldestRecordNumber;
    EofRec.RecordSizeEnd = sizeof(EVENTLOGEOF);

    /* Write the EOF record */
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &EofRec,
                         sizeof(EVENTLOGEOF),
                         NULL,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile() failed!\n");
        goto Done;
    }

    /* Update the header information */
    Header.CurrentRecordNumber = LogFile->Header.CurrentRecordNumber;
    Header.OldestRecordNumber = LogFile->Header.OldestRecordNumber;
    Header.MaxSize = Header.EndOffset + sizeof(EVENTLOGEOF);
    Header.Flags = 0;

    /* Write the (clean) log file header */
    FileOffset.QuadPart = 0;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &Header,
                         sizeof(EVENTLOGHEADER),
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile() failed! (Status: 0x%08lx)\n", Status);
    }

Done:
    /* Free the buffer */
    if (Buffer != NULL)
        HeapFree(MyHeap, 0, Buffer);

    /* Close the backup file */
    if (FileHandle != NULL)
        NtClose(FileHandle);

    /* Unlock the log file */
    RtlReleaseResource(&LogFile->Lock);

    return Status;
}


/* Returns 0 if nothing found. */
ULONG LogfOffsetByNumber(PLOGFILE LogFile, DWORD RecordNumber)
{
    DWORD i;

    for (i = 0; i < LogFile->OffsetInfoNext; i++)
    {
        if (LogFile->OffsetInfo[i].EventNumber == RecordNumber)
            return LogFile->OffsetInfo[i].EventOffset;
    }
    return 0;
}

DWORD LogfGetOldestRecord(PLOGFILE LogFile)
{
    return LogFile->Header.OldestRecordNumber;
}

DWORD LogfGetCurrentRecord(PLOGFILE LogFile)
{
    return LogFile->Header.CurrentRecordNumber;
}

BOOL LogfDeleteOffsetInformation(PLOGFILE LogFile, ULONG ulNumber)
{
    DWORD i;

    if (ulNumber != LogFile->OffsetInfo[0].EventNumber)
    {
        return FALSE;
    }

    for (i = 0; i < LogFile->OffsetInfoNext - 1; i++)
    {
        LogFile->OffsetInfo[i].EventNumber = LogFile->OffsetInfo[i + 1].EventNumber;
        LogFile->OffsetInfo[i].EventOffset = LogFile->OffsetInfo[i + 1].EventOffset;
    }
    LogFile->OffsetInfoNext--;
    return TRUE;
}

BOOL LogfAddOffsetInformation(PLOGFILE LogFile, ULONG ulNumber, ULONG ulOffset)
{
    LPVOID NewOffsetInfo;

    if (LogFile->OffsetInfoNext == LogFile->OffsetInfoSize)
    {
        NewOffsetInfo = HeapReAlloc(MyHeap,
                                    HEAP_ZERO_MEMORY,
                                    LogFile->OffsetInfo,
                                    (LogFile->OffsetInfoSize + 64) *
                                        sizeof(EVENT_OFFSET_INFO));

        if (!NewOffsetInfo)
        {
            DPRINT1("Can't reallocate heap.\n");
            return FALSE;
        }

        LogFile->OffsetInfo = (PEVENT_OFFSET_INFO) NewOffsetInfo;
        LogFile->OffsetInfoSize += 64;
    }

    LogFile->OffsetInfo[LogFile->OffsetInfoNext].EventNumber = ulNumber;
    LogFile->OffsetInfo[LogFile->OffsetInfoNext].EventOffset = ulOffset;
    LogFile->OffsetInfoNext++;

    return TRUE;
}

PBYTE LogfAllocAndBuildNewRecord(LPDWORD lpRecSize,
                                 DWORD   dwRecordNumber,
                                 WORD    wType,
                                 WORD    wCategory,
                                 DWORD   dwEventId,
                                 LPCWSTR SourceName,
                                 LPCWSTR ComputerName,
                                 DWORD   dwSidLength,
                                 PSID    lpUserSid,
                                 WORD    wNumStrings,
                                 WCHAR   * lpStrings,
                                 DWORD   dwDataSize,
                                 LPVOID  lpRawData)
{
    DWORD dwRecSize;
    PEVENTLOGRECORD pRec;
    SYSTEMTIME SysTime;
    WCHAR *str;
    UINT i, pos;
    PBYTE Buffer;

    dwRecSize =
        sizeof(EVENTLOGRECORD) + (lstrlenW(ComputerName) +
                                  lstrlenW(SourceName) + 2) * sizeof(WCHAR);

    if (dwRecSize % 4 != 0)
        dwRecSize += 4 - (dwRecSize % 4);

    dwRecSize += dwSidLength;

    for (i = 0, str = lpStrings; i < wNumStrings; i++)
    {
        dwRecSize += (lstrlenW(str) + 1) * sizeof(WCHAR);
        str += lstrlenW(str) + 1;
    }

    dwRecSize += dwDataSize;
    if (dwRecSize % 4 != 0)
        dwRecSize += 4 - (dwRecSize % 4);

    dwRecSize += 4;

    Buffer = HeapAlloc(MyHeap, HEAP_ZERO_MEMORY, dwRecSize);

    if (!Buffer)
    {
        DPRINT1("Can't allocate heap!\n");
        return NULL;
    }

    pRec = (PEVENTLOGRECORD) Buffer;
    pRec->Length = dwRecSize;
    pRec->Reserved = LOGFILE_SIGNATURE;
    pRec->RecordNumber = dwRecordNumber;

    GetSystemTime(&SysTime);
    SystemTimeToEventTime(&SysTime, &pRec->TimeGenerated);
    SystemTimeToEventTime(&SysTime, &pRec->TimeWritten);

    pRec->EventID = dwEventId;
    pRec->EventType = wType;
    pRec->EventCategory = wCategory;

    pos = sizeof(EVENTLOGRECORD);

    lstrcpyW((WCHAR *) (Buffer + pos), SourceName);
    pos += (lstrlenW(SourceName) + 1) * sizeof(WCHAR);
    lstrcpyW((WCHAR *) (Buffer + pos), ComputerName);
    pos += (lstrlenW(ComputerName) + 1) * sizeof(WCHAR);

    pRec->UserSidOffset = pos;

    if (pos % 4 != 0)
        pos += 4 - (pos % 4);

    if (dwSidLength)
    {
        CopyMemory(Buffer + pos, lpUserSid, dwSidLength);
        pRec->UserSidLength = dwSidLength;
        pRec->UserSidOffset = pos;
        pos += dwSidLength;
    }

    pRec->StringOffset = pos;
    for (i = 0, str = lpStrings; i < wNumStrings; i++)
    {
        lstrcpyW((WCHAR *) (Buffer + pos), str);
        pos += (lstrlenW(str) + 1) * sizeof(WCHAR);
        str += lstrlenW(str) + 1;
    }
    pRec->NumStrings = wNumStrings;

    pRec->DataOffset = pos;
    if (dwDataSize)
    {
        pRec->DataLength = dwDataSize;
        CopyMemory(Buffer + pos, lpRawData, dwDataSize);
        pos += dwDataSize;
    }

    if (pos % 4 != 0)
        pos += 4 - (pos % 4);

    *((PDWORD) (Buffer + pos)) = dwRecSize;

    *lpRecSize = dwRecSize;
    return Buffer;
}


VOID
LogfReportEvent(WORD wType,
                WORD wCategory,
                DWORD dwEventId,
                WORD wNumStrings,
                WCHAR *lpStrings,
                DWORD dwDataSize,
                LPVOID lpRawData)
{
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
    PEVENTSOURCE pEventSource = NULL;
    PBYTE logBuffer;
    DWORD lastRec;
    DWORD recSize;
    DWORD dwError;

    if (!GetComputerNameW(szComputerName, &dwComputerNameLength))
    {
        szComputerName[0] = 0;
    }

    pEventSource = GetEventSourceByName(L"EventLog");
    if (pEventSource == NULL)
    {
        return;
    }

    lastRec = LogfGetCurrentRecord(pEventSource->LogFile);

    logBuffer = LogfAllocAndBuildNewRecord(&recSize,
                                           lastRec,
                                           wType,
                                           wCategory,
                                           dwEventId,
                                           pEventSource->szName,
                                           (LPCWSTR)szComputerName,
                                           0,
                                           NULL,
                                           wNumStrings,
                                           lpStrings,
                                           dwDataSize,
                                           lpRawData);

    dwError = LogfWriteData(pEventSource->LogFile, recSize, logBuffer);
    if (!dwError)
    {
        DPRINT1("ERROR WRITING TO EventLog %S\n", pEventSource->LogFile->FileName);
    }

    LogfFreeRecord(logBuffer);
}
