/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/eventlog/file.c
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2005 Saveliy Tretiakov
 *                   Michael Martin
 *                   Hermes Belusca-Maito
 */

/* INCLUDES ******************************************************************/

#include "eventlog.h"

#include <ndk/iofuncs.h>
#include <ndk/kefuncs.h>

#define NDEBUG
#include <debug.h>

/* LOG FILE LIST - GLOBALS ***************************************************/

static LIST_ENTRY LogFileListHead;
static CRITICAL_SECTION LogFileListCs;

/* LOG FILE LIST - FUNCTIONS *************************************************/

VOID LogfCloseAll(VOID)
{
    EnterCriticalSection(&LogFileListCs);

    while (!IsListEmpty(&LogFileListHead))
    {
        LogfClose(CONTAINING_RECORD(LogFileListHead.Flink, LOGFILE, ListEntry), TRUE);
    }

    LeaveCriticalSection(&LogFileListCs);

    DeleteCriticalSection(&LogFileListCs);
}

VOID LogfListInitialize(VOID)
{
    InitializeCriticalSection(&LogFileListCs);
    InitializeListHead(&LogFileListHead);
}

PLOGFILE LogfListItemByName(LPCWSTR Name)
{
    PLIST_ENTRY CurrentEntry;
    PLOGFILE Item, Result = NULL;

    ASSERT(Name);

    EnterCriticalSection(&LogFileListCs);

    CurrentEntry = LogFileListHead.Flink;
    while (CurrentEntry != &LogFileListHead)
    {
        Item = CONTAINING_RECORD(CurrentEntry, LOGFILE, ListEntry);

        if (Item->LogName && !_wcsicmp(Item->LogName, Name))
        {
            Result = Item;
            break;
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    LeaveCriticalSection(&LogFileListCs);
    return Result;
}

#if 0
/* Index starting from 1 */
DWORD LogfListItemIndexByName(LPCWSTR Name)
{
    PLIST_ENTRY CurrentEntry;
    DWORD Result = 0;
    DWORD i = 1;

    ASSERT(Name);

    EnterCriticalSection(&LogFileListCs);

    CurrentEntry = LogFileListHead.Flink;
    while (CurrentEntry != &LogFileListHead)
    {
        PLOGFILE Item = CONTAINING_RECORD(CurrentEntry, LOGFILE, ListEntry);

        if (Item->LogName && !_wcsicmp(Item->LogName, Name))
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
#endif

/* Index starting from 1 */
PLOGFILE LogfListItemByIndex(DWORD Index)
{
    PLIST_ENTRY CurrentEntry;
    PLOGFILE Result = NULL;
    DWORD i = 1;

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

DWORD LogfListItemCount(VOID)
{
    PLIST_ENTRY CurrentEntry;
    DWORD i = 0;

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

static VOID
LogfListAddItem(PLOGFILE Item)
{
    EnterCriticalSection(&LogFileListCs);
    InsertTailList(&LogFileListHead, &Item->ListEntry);
    LeaveCriticalSection(&LogFileListCs);
}

static VOID
LogfListRemoveItem(PLOGFILE Item)
{
    EnterCriticalSection(&LogFileListCs);
    RemoveEntryList(&Item->ListEntry);
    LeaveCriticalSection(&LogFileListCs);
}


/* GLOBALS *******************************************************************/

static const EVENTLOGEOF EOFRecord =
{
    sizeof(EOFRecord),
    0x11111111, 0x22222222, 0x33333333, 0x44444444,
    0, 0, 0, 0,
    sizeof(EOFRecord)
};

/* FUNCTIONS *****************************************************************/

static NTSTATUS
ReadLogBuffer(IN  PLOGFILE LogFile,
              OUT PIO_STATUS_BLOCK IoStatusBlock,
              OUT PVOID Buffer,
              IN  ULONG Length,
              IN  PLARGE_INTEGER ByteOffset,
              OUT PLARGE_INTEGER NextOffset OPTIONAL)
{
    NTSTATUS Status;
    ULONG BufSize;
    LARGE_INTEGER FileOffset;

    ASSERT(LogFile->CurrentSize <= LogFile->Header.MaxSize);
    // ASSERT(ByteOffset->QuadPart <= LogFile->Header.MaxSize);
    ASSERT(ByteOffset->QuadPart <= LogFile->CurrentSize);

    if (NextOffset)
        NextOffset->QuadPart = 0LL;

    /* Read the first part of the buffer */
    FileOffset = *ByteOffset;
    BufSize = min(Length, LogFile->CurrentSize - FileOffset.QuadPart);

    Status = NtReadFile(LogFile->hFile,
                        NULL,
                        NULL,
                        NULL,
                        IoStatusBlock,
                        Buffer,
                        BufSize,
                        &FileOffset,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtReadFile failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    if (Length > BufSize)
    {
        ULONG_PTR Information = IoStatusBlock->Information;

        /*
         * The buffer was splitted in two, its second part
         * is to be read at the beginning of the log.
         */
        Buffer = (PVOID)((ULONG_PTR)Buffer + BufSize);
        BufSize = Length - BufSize;
        FileOffset.QuadPart = sizeof(EVENTLOGHEADER);

        Status = NtReadFile(LogFile->hFile,
                            NULL,
                            NULL,
                            NULL,
                            IoStatusBlock,
                            Buffer,
                            BufSize,
                            &FileOffset,
                            NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtReadFile failed (Status 0x%08lx)\n", Status);
            return Status;
        }
        /* Add the read number of bytes from the first read */
        IoStatusBlock->Information += Information;
    }

    /* We return the offset just after the end of the read buffer */
    if (NextOffset)
        NextOffset->QuadPart = FileOffset.QuadPart + BufSize;

    return Status;
}

static NTSTATUS
WriteLogBuffer(IN  PLOGFILE LogFile,
               OUT PIO_STATUS_BLOCK IoStatusBlock,
               IN  PVOID Buffer,
               IN  ULONG Length,
               IN  PLARGE_INTEGER ByteOffset,
               OUT PLARGE_INTEGER NextOffset OPTIONAL)
{
    NTSTATUS Status;
    ULONG BufSize;
    LARGE_INTEGER FileOffset;

    ASSERT(LogFile->CurrentSize <= LogFile->Header.MaxSize);
    ASSERT(ByteOffset->QuadPart <= LogFile->Header.MaxSize);
    ASSERT(ByteOffset->QuadPart <= LogFile->CurrentSize);

    if (NextOffset)
        NextOffset->QuadPart = 0LL;

    /* Write the first part of the buffer */
    FileOffset = *ByteOffset;
    BufSize = min(Length, LogFile->CurrentSize /* LogFile->Header.MaxSize */ - FileOffset.QuadPart);

    Status = NtWriteFile(LogFile->hFile,
                         NULL,
                         NULL,
                         NULL,
                         IoStatusBlock,
                         Buffer,
                         BufSize,
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    if (Length > BufSize)
    {
        ULONG_PTR Information = IoStatusBlock->Information;

        /*
         * The buffer was splitted in two, its second part is written
         * at the beginning of the log.
         */
        Buffer = (PVOID)((ULONG_PTR)Buffer + BufSize);
        BufSize = Length - BufSize;
        FileOffset.QuadPart = sizeof(EVENTLOGHEADER);

        Status = NtWriteFile(LogFile->hFile,
                             NULL,
                             NULL,
                             NULL,
                             IoStatusBlock,
                             Buffer,
                             BufSize,
                             &FileOffset,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtWriteFile failed (Status 0x%08lx)\n", Status);
            return Status;
        }
        /* Add the written number of bytes from the first write */
        IoStatusBlock->Information += Information;

        /* The log wraps */
        LogFile->Header.Flags |= ELF_LOGFILE_HEADER_WRAP;
    }

    /* We return the offset just after the end of the written buffer */
    if (NextOffset)
        NextOffset->QuadPart = FileOffset.QuadPart + BufSize;

    return Status;
}


/* Returns 0 if nothing is found */
static ULONG
LogfOffsetByNumber(PLOGFILE LogFile,
                   DWORD RecordNumber)
{
    DWORD i;

    for (i = 0; i < LogFile->OffsetInfoNext; i++)
    {
        if (LogFile->OffsetInfo[i].EventNumber == RecordNumber)
            return LogFile->OffsetInfo[i].EventOffset;
    }
    return 0;
}

static BOOL
LogfAddOffsetInformation(PLOGFILE LogFile,
                         ULONG ulNumber,
                         ULONG ulOffset)
{
    PVOID NewOffsetInfo;

    if (LogFile->OffsetInfoNext == LogFile->OffsetInfoSize)
    {
        NewOffsetInfo = HeapReAlloc(GetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    LogFile->OffsetInfo,
                                    (LogFile->OffsetInfoSize + 64) *
                                        sizeof(EVENT_OFFSET_INFO));

        if (!NewOffsetInfo)
        {
            DPRINT1("Cannot reallocate heap.\n");
            return FALSE;
        }

        LogFile->OffsetInfo = (PEVENT_OFFSET_INFO)NewOffsetInfo;
        LogFile->OffsetInfoSize += 64;
    }

    LogFile->OffsetInfo[LogFile->OffsetInfoNext].EventNumber = ulNumber;
    LogFile->OffsetInfo[LogFile->OffsetInfoNext].EventOffset = ulOffset;
    LogFile->OffsetInfoNext++;

    return TRUE;
}

static BOOL
LogfDeleteOffsetInformation(PLOGFILE LogFile,
                            ULONG ulNumberMin,
                            ULONG ulNumberMax)
{
    DWORD i;

    if (ulNumberMin > ulNumberMax)
        return FALSE;

    /* Remove records ulNumberMin to ulNumberMax inclusive */
    while (ulNumberMin <= ulNumberMax)
    {
        /*
         * As the offset information is listed in increasing order, and we want
         * to keep the list without holes, we demand that ulNumberMin is the first
         * element in the list.
         */
        if (ulNumberMin != LogFile->OffsetInfo[0].EventNumber)
            return FALSE;

        /*
         * RtlMoveMemory(&LogFile->OffsetInfo[0],
         *               &LogFile->OffsetInfo[1],
         *               sizeof(EVENT_OFFSET_INFO) * (LogFile->OffsetInfoNext - 1));
         */
        for (i = 0; i < LogFile->OffsetInfoNext - 1; i++)
        {
            LogFile->OffsetInfo[i].EventNumber = LogFile->OffsetInfo[i + 1].EventNumber;
            LogFile->OffsetInfo[i].EventOffset = LogFile->OffsetInfo[i + 1].EventOffset;
        }
        LogFile->OffsetInfoNext--;

        /* Go to the next offset information */
        ulNumberMin++;
    }

    return TRUE;
}

static NTSTATUS
LogfInitializeNew(PLOGFILE LogFile,
                  ULONG ulMaxSize,
                  ULONG ulRetention)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER FileOffset;
    EVENTLOGEOF EofRec;

    /* Initialize the event log header */
    RtlZeroMemory(&LogFile->Header, sizeof(EVENTLOGHEADER));

    LogFile->Header.HeaderSize = sizeof(EVENTLOGHEADER);
    LogFile->Header.Signature  = LOGFILE_SIGNATURE;
    LogFile->Header.MajorVersion = MAJORVER;
    LogFile->Header.MinorVersion = MINORVER;

    /* Set the offset to the oldest record */
    LogFile->Header.StartOffset = sizeof(EVENTLOGHEADER);
    /* Set the offset to the ELF_EOF_RECORD */
    LogFile->Header.EndOffset = sizeof(EVENTLOGHEADER);
    /* Set the number of the next record that will be added */
    LogFile->Header.CurrentRecordNumber = 1;
    /* The event log is empty, there is no record so far */
    LogFile->Header.OldestRecordNumber = 0;

    // FIXME: Windows' EventLog log file sizes are always multiple of 64kB
    // but that does not mean the real log size is == file size.

    /* Round MaxSize to be a multiple of ULONG (normally on Windows: multiple of 64 kB) */
    LogFile->Header.MaxSize = ROUND_UP(ulMaxSize, sizeof(ULONG));
    LogFile->CurrentSize = LogFile->Header.MaxSize;

    LogFile->Header.Flags = 0;
    LogFile->Header.Retention = ulRetention;
    LogFile->Header.EndHeaderSize = sizeof(EVENTLOGHEADER);

    /* Write the header */
    SetFilePointer(LogFile->hFile, 0, NULL, FILE_BEGIN);
    SetEndOfFile(LogFile->hFile);

    FileOffset.QuadPart = 0LL;
    Status = NtWriteFile(LogFile->hFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &LogFile->Header,
                         sizeof(EVENTLOGHEADER),
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Initialize the ELF_EOF_RECORD and write it */
    RtlCopyMemory(&EofRec, &EOFRecord, sizeof(EOFRecord));
    EofRec.BeginRecord = LogFile->Header.StartOffset;
    EofRec.EndRecord   = LogFile->Header.EndOffset;
    EofRec.CurrentRecordNumber = LogFile->Header.CurrentRecordNumber;
    EofRec.OldestRecordNumber  = LogFile->Header.OldestRecordNumber;

    Status = NtWriteFile(LogFile->hFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &EofRec,
                         sizeof(EofRec),
                         NULL,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    Status = NtFlushBuffersFile(LogFile->hFile, &IoStatusBlock);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtFlushBuffersFile failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
LogfInitializeExisting(PLOGFILE LogFile,
                       BOOLEAN Backup)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER FileOffset, NextOffset;
    LARGE_INTEGER LogFileSize;
    DWORD dwRecordsNumber = 0;
    ULONG RecOffset;
    PDWORD pdwRecSize2;
    EVENTLOGEOF EofRec;
    EVENTLOGRECORD RecBuf;
    PEVENTLOGRECORD pRecBuf;
    BOOLEAN Wrapping = FALSE;
    BOOLEAN IsLogDirty = FALSE;

    DPRINT("Initializing LogFile %S\n", LogFile->LogName);

    /* Read the log header */
    FileOffset.QuadPart = 0LL;
    Status = NtReadFile(LogFile->hFile,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        &LogFile->Header,
                        sizeof(EVENTLOGHEADER),
                        &FileOffset,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtReadFile failed (Status 0x%08lx)\n", Status);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }
    if (IoStatusBlock.Information != sizeof(EVENTLOGHEADER))
    {
        DPRINT("EventLog: Invalid file %S.\n", LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    /* Header validity checks */

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

    IsLogDirty = (LogFile->Header.Flags & ELF_LOGFILE_HEADER_DIRTY);

    /* If the log is a backup log that is dirty, then it is corrupted */
    if (Backup && IsLogDirty)
    {
        DPRINT("EventLog: Backup log %S is dirty.\n", LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    /*
     * Retrieve the log file size and check whether the file is not too large;
     * this log format only supports files of theoretical size < 0xFFFFFFFF .
     */
    if (!GetFileSizeEx(LogFile->hFile, &LogFileSize))
        return I_RpcMapWin32Status(GetLastError());

    if (LogFileSize.HighPart != 0)
    {
        DPRINT1("EventLog: Log %S is too large.\n", LogFile->FileName);
        // return STATUS_FILE_TOO_LARGE;
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    LogFile->CurrentSize = LogFileSize.LowPart; // LogFileSize.QuadPart;

    /* Adjust the log maximum size if needed */
    if (LogFile->CurrentSize > LogFile->Header.MaxSize)
        LogFile->Header.MaxSize = LogFile->CurrentSize;

    /*
     * In a non-backup dirty log, the most up-to-date information about
     * the Start/End offsets and the Oldest and Current event record numbers
     * are found in the EOF record. We need to locate the EOF record without
     * relying on the log header's EndOffset, then patch the log header with
     * the values from the EOF record.
     */
    if ((LogFile->Header.EndOffset >= sizeof(EVENTLOGHEADER)) &&
        (LogFile->Header.EndOffset <  LogFile->CurrentSize) &&
        (LogFile->Header.EndOffset & 3) == 0) // EndOffset % sizeof(ULONG) == 0
    {
        /* The header EOF offset may be valid, try to start with it */
        RecOffset = LogFile->Header.EndOffset;
    }
    else
    {
        /* The header EOF offset could not be valid, so start from the beginning */
        RecOffset = sizeof(EVENTLOGHEADER);
    }

    FileOffset.QuadPart = RecOffset;
    Wrapping = FALSE;

    for (;;)
    {
        if (Wrapping && FileOffset.QuadPart >= RecOffset)
        {
            DPRINT1("EOF record not found!\n");
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }

        /* Attempt to read the fixed part of an EVENTLOGEOF (may wrap) */
        Status = ReadLogBuffer(LogFile,
                               &IoStatusBlock,
                               &EofRec,
                               EVENTLOGEOF_SIZE_FIXED,
                               &FileOffset,
                               NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ReadLogBuffer failed (Status 0x%08lx)\n", Status);
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }
        if (IoStatusBlock.Information != EVENTLOGEOF_SIZE_FIXED)
        {
            DPRINT1("Cannot read at most an EOF record!\n");
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }

        /* Is it an EVENTLOGEOF record? */
        if (RtlCompareMemory(&EofRec, &EOFRecord, EVENTLOGEOF_SIZE_FIXED) == EVENTLOGEOF_SIZE_FIXED)
        {
            DPRINT("Found EOF record at %llx\n", FileOffset.QuadPart);

            /* Got it! Break the loop and continue */
            break;
        }

        /* No, continue looping */
        if (*(PULONG)((ULONG_PTR)&EofRec + sizeof(ULONG)) == *(PULONG)(&EOFRecord))
            FileOffset.QuadPart += sizeof(ULONG);
        else
        if (*(PULONG)((ULONG_PTR)&EofRec + 2*sizeof(ULONG)) == *(PULONG)(&EOFRecord))
            FileOffset.QuadPart += 2*sizeof(ULONG);
        else
        if (*(PULONG)((ULONG_PTR)&EofRec + 3*sizeof(ULONG)) == *(PULONG)(&EOFRecord))
            FileOffset.QuadPart += 3*sizeof(ULONG);
        else
        if (*(PULONG)((ULONG_PTR)&EofRec + 4*sizeof(ULONG)) == *(PULONG)(&EOFRecord))
            FileOffset.QuadPart += 4*sizeof(ULONG);
        else
            FileOffset.QuadPart += 5*sizeof(ULONG); // EVENTLOGEOF_SIZE_FIXED

        if (FileOffset.QuadPart >= LogFile->CurrentSize /* LogFile->Header.MaxSize */)
        {
            /* Wrap the offset */
            FileOffset.QuadPart -= LogFile->CurrentSize /* LogFile->Header.MaxSize */ - sizeof(EVENTLOGHEADER);
            Wrapping = TRUE;
        }
    }
    /*
     * The only way to be there is to have found a valid EOF record.
     * Otherwise the previous loop has failed and STATUS_EVENTLOG_FILE_CORRUPT
     * was returned.
     */

    /* Read the full EVENTLOGEOF (may wrap) and validate it */
    Status = ReadLogBuffer(LogFile,
                           &IoStatusBlock,
                           &EofRec,
                           sizeof(EofRec),
                           &FileOffset,
                           NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ReadLogBuffer failed (Status 0x%08lx)\n", Status);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }
    if (IoStatusBlock.Information != sizeof(EofRec))
    {
        DPRINT1("Cannot read the full EOF record!\n");
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    /* Complete validity checks */
    if ((EofRec.RecordSizeEnd != EofRec.RecordSizeBeginning) ||
        (EofRec.EndRecord != FileOffset.QuadPart))
    {
        DPRINT1("EOF record %llx is corrupted (0x%x vs. 0x%x ; 0x%x vs. 0x%llx), expected %x %x!\n", FileOffset.QuadPart,
                EofRec.RecordSizeEnd, EofRec.RecordSizeBeginning,
                EofRec.EndRecord, FileOffset.QuadPart,
                EOFRecord.RecordSizeEnd, EOFRecord.RecordSizeBeginning);
        DPRINT1("RecordSizeEnd = %x\n", EofRec.RecordSizeEnd);
        DPRINT1("RecordSizeBeginning = %x\n", EofRec.RecordSizeBeginning);
        DPRINT1("EndRecord = %x\n", EofRec.EndRecord);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    /* The EOF record is valid, break the loop and continue */

    /* If the log is not dirty, the header values should correspond to the EOF ones */
    if (!IsLogDirty)
    {
        if ( (LogFile->Header.StartOffset != EofRec.BeginRecord) ||
             (LogFile->Header.EndOffset   != EofRec.EndRecord)   ||
             (LogFile->Header.CurrentRecordNumber != EofRec.CurrentRecordNumber) ||
             (LogFile->Header.OldestRecordNumber  != EofRec.OldestRecordNumber) )
        {
            DPRINT1("\n"
                    "Log header or EOF record is corrupted:\n"
                    "    StartOffset: 0x%x, expected 0x%x; EndOffset: 0x%x, expected 0x%x;\n"
                    "    CurrentRecordNumber: %d, expected %d; OldestRecordNumber: %d, expected %d.\n",
                    LogFile->Header.StartOffset, EofRec.BeginRecord,
                    LogFile->Header.EndOffset  , EofRec.EndRecord,
                    LogFile->Header.CurrentRecordNumber, EofRec.CurrentRecordNumber,
                    LogFile->Header.OldestRecordNumber , EofRec.OldestRecordNumber);

            return STATUS_EVENTLOG_FILE_CORRUPT;
        }
    }

    /* If the log is dirty, patch the log header with the values from the EOF record */
    if (!Backup && IsLogDirty)
    {
        LogFile->Header.StartOffset = EofRec.BeginRecord;
        LogFile->Header.EndOffset   = EofRec.EndRecord;
        LogFile->Header.CurrentRecordNumber = EofRec.CurrentRecordNumber;
        LogFile->Header.OldestRecordNumber  = EofRec.OldestRecordNumber;
    }

    /*
     * FIXME! During operations the EOF record is the one that is the most
     * updated (its Oldest & Current record numbers are always up-to
     * date) while the ones from the header may be unsync. When closing
     * (or flushing?) the event log, the header's record numbers get
     * updated with the same values as the ones stored in the EOF record.
     */

    /* Verify Start/End offsets boundaries */

    if ((LogFile->Header.StartOffset >= LogFile->CurrentSize) ||
        (LogFile->Header.StartOffset & 3) != 0) // StartOffset % sizeof(ULONG) != 0
    {
        DPRINT("EventLog: Invalid start offset %x in %S.\n",
               LogFile->Header.StartOffset, LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }
    if ((LogFile->Header.EndOffset >= LogFile->CurrentSize) ||
        (LogFile->Header.EndOffset & 3) != 0) // EndOffset % sizeof(ULONG) != 0
    {
        DPRINT("EventLog: Invalid EOF offset %x in %S.\n",
               LogFile->Header.EndOffset, LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    if ((LogFile->Header.StartOffset != LogFile->Header.EndOffset) &&
        (LogFile->Header.MaxSize - LogFile->Header.StartOffset < sizeof(EVENTLOGRECORD)))
    {
        /*
         * If StartOffset does not point to EndOffset i.e. to an EVENTLOGEOF,
         * it should point to a non-splitted EVENTLOGRECORD.
         */
        DPRINT("EventLog: Invalid start offset %x in %S.\n",
               LogFile->Header.StartOffset, LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    if ((LogFile->Header.StartOffset < LogFile->Header.EndOffset) &&
        (LogFile->Header.EndOffset - LogFile->Header.StartOffset < sizeof(EVENTLOGRECORD)))
    {
        /*
         * In non-wrapping case, there must be enough space between StartOffset
         * and EndOffset to contain at least a full EVENTLOGRECORD.
         */
        DPRINT("EventLog: Invalid start offset %x or end offset %x in %S.\n",
               LogFile->Header.StartOffset, LogFile->Header.EndOffset, LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    if (LogFile->Header.StartOffset <= LogFile->Header.EndOffset)
    {
        /*
         * Non-wrapping case: the (wrapping) free space starting at EndOffset
         * must be able to contain an EVENTLOGEOF.
         */
        if (LogFile->Header.MaxSize - LogFile->Header.EndOffset +
            LogFile->Header.StartOffset - sizeof(EVENTLOGHEADER) < sizeof(EVENTLOGEOF))
        {
            DPRINT("EventLog: Invalid EOF offset %x in %S.\n",
                   LogFile->Header.EndOffset, LogFile->FileName);
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }
    }
    else // if (LogFile->Header.StartOffset > LogFile->Header.EndOffset)
    {
        /*
         * Wrapping case: the free space between EndOffset and StartOffset
         * must be able to contain an EVENTLOGEOF.
         */
        if (LogFile->Header.StartOffset - LogFile->Header.EndOffset < sizeof(EVENTLOGEOF))
        {
            DPRINT("EventLog: Invalid EOF offset %x in %S.\n",
                   LogFile->Header.EndOffset, LogFile->FileName);
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }
    }

    /* Start enumerating the event records from the beginning */
    RecOffset = LogFile->Header.StartOffset;
    FileOffset.QuadPart = RecOffset;
    Wrapping = FALSE;

    // // FIXME! FIXME!
    // if (!(LogFile->Header.Flags & ELF_LOGFILE_HEADER_WRAP))
    // {
        // DPRINT1("Log file was wrapping but the flag was not set! Fixing...\n");
        // LogFile->Header.Flags |= ELF_LOGFILE_HEADER_WRAP;
    // }

    DPRINT("StartOffset = %x, EndOffset = %x\n",
           LogFile->Header.StartOffset, LogFile->Header.EndOffset);

    /*
     * For non-backup logs of size < MaxSize, reorganize the events such that
     * they do not wrap as soon as we write new ones.
     */
#if 0
    if (!Backup)
    {
        pRecBuf = RtlAllocateHeap(GetProcessHeap(), 0, RecBuf.Length);
        if (pRecBuf == NULL)
        {
            DPRINT1("Cannot allocate temporary buffer, skip event reorganization.\n");
            goto Continue;
        }

        // TODO: Do the job!
    }

Continue:

    DPRINT("StartOffset = %x, EndOffset = %x\n",
           LogFile->Header.StartOffset, LogFile->Header.EndOffset);
#endif

    while (FileOffset.QuadPart != LogFile->Header.EndOffset)
    {
        if (Wrapping && FileOffset.QuadPart >= RecOffset)
        {
            /* We have finished enumerating all the event records */
            break;
        }

        /* Read the next EVENTLOGRECORD header at once (it cannot be split) */
        Status = NtReadFile(LogFile->hFile,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            &RecBuf,
                            sizeof(RecBuf),
                            &FileOffset,
                            NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtReadFile failed (Status 0x%08lx)\n", Status);
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }
        if (IoStatusBlock.Information != sizeof(RecBuf))
        {
            DPRINT("Length != sizeof(RecBuf)\n");
            break;
        }

        if (RecBuf.Reserved != LOGFILE_SIGNATURE ||
            RecBuf.Length < sizeof(EVENTLOGRECORD))
        {
            DPRINT("RecBuf problem\n");
            break;
        }

        /* Allocate a full EVENTLOGRECORD (header + data) */
        pRecBuf = RtlAllocateHeap(GetProcessHeap(), 0, RecBuf.Length);
        if (pRecBuf == NULL)
        {
            DPRINT1("Cannot allocate heap!\n");
            return STATUS_NO_MEMORY;
        }

        /* Attempt to read the full EVENTLOGRECORD (can wrap) */
        Status = ReadLogBuffer(LogFile,
                               &IoStatusBlock,
                               pRecBuf,
                               RecBuf.Length,
                               &FileOffset,
                               &NextOffset);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ReadLogBuffer failed (Status 0x%08lx)\n", Status);
            RtlFreeHeap(GetProcessHeap(), 0, pRecBuf);
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }
        if (IoStatusBlock.Information != RecBuf.Length)
        {
            DPRINT1("Oh oh!!\n");
            RtlFreeHeap(GetProcessHeap(), 0, pRecBuf);
            break;
        }

        // /* If OverWrittenRecords is TRUE and this record has already been read */
        // if (OverWrittenRecords && (pRecBuf->RecordNumber == LogFile->Header.OldestRecordNumber))
        // {
            // RtlFreeHeap(GetProcessHeap(), 0, pRecBuf);
            // break;
        // }

        pdwRecSize2 = (PDWORD)((ULONG_PTR)pRecBuf + RecBuf.Length - 4);

        if (*pdwRecSize2 != RecBuf.Length)
        {
            DPRINT1("Invalid RecordSizeEnd of record %d (%x) in %S\n",
                    dwRecordsNumber, *pdwRecSize2, LogFile->LogName);
            RtlFreeHeap(GetProcessHeap(), 0, pRecBuf);
            break;
        }

        DPRINT("Add new record %d - %x\n", pRecBuf->RecordNumber, FileOffset.QuadPart);

        dwRecordsNumber++;

        if (!LogfAddOffsetInformation(LogFile,
                                      pRecBuf->RecordNumber,
                                      FileOffset.QuadPart))
        {
            DPRINT1("LogfAddOffsetInformation() failed!\n");
            RtlFreeHeap(GetProcessHeap(), 0, pRecBuf);
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }

        RtlFreeHeap(GetProcessHeap(), 0, pRecBuf);

        if (NextOffset.QuadPart == LogFile->Header.EndOffset)
        {
            /* We have finished enumerating all the event records */
            DPRINT("NextOffset.QuadPart == LogFile->Header.EndOffset, break\n");
            break;
        }

        /*
         * If this was the last event record before the end of the log file,
         * the next one should start at the beginning of the log and the space
         * between the last event record and the end of the file is padded.
         */
        if (LogFile->Header.MaxSize - NextOffset.QuadPart < sizeof(EVENTLOGRECORD))
        {
            /* Wrap to the beginning of the log */
            DPRINT("Wrap!\n");
            NextOffset.QuadPart = sizeof(EVENTLOGHEADER);
        }

        /*
         * If the next offset to read is below the current offset,
         * this means we are wrapping.
         */
        if (FileOffset.QuadPart > NextOffset.QuadPart)
        {
            DPRINT("Wrapping = TRUE;\n");
            Wrapping = TRUE;
        }

        /* Move the current offset */
        FileOffset = NextOffset;
    }

    /* If the event log was empty, it will now contain one record */
    if (dwRecordsNumber != 0 && LogFile->Header.OldestRecordNumber == 0)
        LogFile->Header.OldestRecordNumber = 1;

    LogFile->Header.CurrentRecordNumber = dwRecordsNumber + LogFile->Header.OldestRecordNumber;
    if (LogFile->Header.CurrentRecordNumber == 0)
        LogFile->Header.CurrentRecordNumber = 1;

    if (!Backup)
    {
        FileOffset.QuadPart = 0LL;
        Status = NtWriteFile(LogFile->hFile,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             &LogFile->Header,
                             sizeof(EVENTLOGHEADER),
                             &FileOffset,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtWriteFile failed (Status 0x%08lx)\n", Status);
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }

        Status = NtFlushBuffersFile(LogFile->hFile, &IoStatusBlock);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtFlushBuffersFile failed (Status 0x%08lx)\n", Status);
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
LogfCreate(PLOGFILE* LogFile,
           PCWSTR    LogName,
           PUNICODE_STRING FileName,
           ULONG     ulMaxSize,
           ULONG     ulRetention,
           BOOLEAN   Permanent,
           BOOLEAN   Backup)
{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PLOGFILE pLogFile;
    SIZE_T LogNameLen;
    BOOLEAN CreateNew = FALSE;

    pLogFile = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LOGFILE));
    if (!pLogFile)
    {
        DPRINT1("Cannot allocate heap!\n");
        return STATUS_NO_MEMORY;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&pLogFile->hFile,
                          Backup ? (GENERIC_READ | SYNCHRONIZE)
                                 : (GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE),
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
        DPRINT1("Cannot create file %wZ (Status: 0x%08lx)\n", FileName, Status);
        goto Quit;
    }

    CreateNew = (IoStatusBlock.Information == FILE_CREATED);

    LogNameLen = (LogName ? wcslen(LogName) : 0) + 1;
    pLogFile->LogName = RtlAllocateHeap(GetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  LogNameLen * sizeof(WCHAR));
    if (pLogFile->LogName == NULL)
    {
        DPRINT1("Cannot allocate heap\n");
        Status = STATUS_NO_MEMORY;
        goto Quit;
    }

    if (LogName)
        StringCchCopy(pLogFile->LogName, LogNameLen, LogName);

    pLogFile->FileName = RtlAllocateHeap(GetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   /*(wcslen(FileName->Buffer) + 1) * sizeof(WCHAR)*/
                                   FileName->Length + sizeof(UNICODE_NULL));
    if (pLogFile->FileName == NULL)
    {
        DPRINT1("Cannot allocate heap\n");
        Status = STATUS_NO_MEMORY;
        goto Quit;
    }

    StringCchCopy(pLogFile->FileName,
                  /*wcslen(FileName->Buffer) + 1*/ (FileName->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR),
                  FileName->Buffer);

    pLogFile->OffsetInfo = RtlAllocateHeap(GetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     sizeof(EVENT_OFFSET_INFO) * 64);
    if (pLogFile->OffsetInfo == NULL)
    {
        DPRINT1("Cannot allocate heap\n");
        Status = STATUS_NO_MEMORY;
        goto Quit;
    }
    pLogFile->OffsetInfoSize = 64;
    pLogFile->OffsetInfoNext = 0;

    pLogFile->Permanent = Permanent;

    // FIXME: Always use the regitry values for MaxSize & Retention,
    // even for existing logs!

    // FIXME: On Windows, EventLog uses the MaxSize setting
    // from the registry itself; the MaxSize from the header
    // is just for information purposes.

    if (CreateNew)
        Status = LogfInitializeNew(pLogFile, ulMaxSize, ulRetention);
    else
        Status = LogfInitializeExisting(pLogFile, Backup);

    if (!NT_SUCCESS(Status))
        goto Quit;

    RtlInitializeResource(&pLogFile->Lock);

    LogfListAddItem(pLogFile);

Quit:
    if (!NT_SUCCESS(Status))
    {
        if ((pLogFile->hFile != NULL) && (pLogFile->hFile != INVALID_HANDLE_VALUE))
            NtClose(pLogFile->hFile);

        if (pLogFile->OffsetInfo)
            RtlFreeHeap(GetProcessHeap(), 0, pLogFile->OffsetInfo);

        if (pLogFile->FileName)
            RtlFreeHeap(GetProcessHeap(), 0, pLogFile->FileName);

        if (pLogFile->LogName)
            RtlFreeHeap(GetProcessHeap(), 0, pLogFile->LogName);

        RtlFreeHeap(GetProcessHeap(), 0, pLogFile);
    }
    else
    {
        *LogFile = pLogFile;
    }

    return Status;
}

VOID
LogfClose(PLOGFILE LogFile,
          BOOLEAN  ForceClose)
{
    IO_STATUS_BLOCK IoStatusBlock;

    if (LogFile == NULL)
        return;

    if (!ForceClose && LogFile->Permanent)
        return;

    RtlAcquireResourceExclusive(&LogFile->Lock, TRUE);

    NtFlushBuffersFile(LogFile->hFile, &IoStatusBlock);
    NtClose(LogFile->hFile);
    LogfListRemoveItem(LogFile);

    RtlDeleteResource(&LogFile->Lock);

    RtlFreeHeap(GetProcessHeap(), 0, LogFile->LogName);
    RtlFreeHeap(GetProcessHeap(), 0, LogFile->FileName);
    RtlFreeHeap(GetProcessHeap(), 0, LogFile->OffsetInfo);
    RtlFreeHeap(GetProcessHeap(), 0, LogFile);

    return;
}


static NTSTATUS
ReadAnsiLogEntry(IN  PLOGFILE LogFile,
                 OUT PIO_STATUS_BLOCK IoStatusBlock,
                 OUT PVOID Buffer,
                 IN  ULONG Length,
                 IN  PLARGE_INTEGER ByteOffset,
                 OUT PLARGE_INTEGER NextOffset OPTIONAL)
{
    NTSTATUS Status;
    PVOID UnicodeBuffer = NULL;
    PEVENTLOGRECORD Src, Dst;
    ANSI_STRING StringA;
    UNICODE_STRING StringW;
    PVOID SrcPtr, DstPtr;
    // DWORD dwRead;
    DWORD i;
    DWORD dwPadding;
    DWORD dwEntryLength;
    PDWORD pLength;

    IoStatusBlock->Information = 0;

    UnicodeBuffer = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, Length);
    if (UnicodeBuffer == NULL)
    {
        DPRINT1("Alloc failed!\n");
        return STATUS_NO_MEMORY;
    }

    Status = ReadLogBuffer(LogFile,
                           IoStatusBlock,
                           UnicodeBuffer,
                           Length,
                           ByteOffset,
                           NextOffset);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ReadLogBuffer failed (Status 0x%08lx)\n", Status);
        // Status = STATUS_EVENTLOG_FILE_CORRUPT;
        goto done;
    }
    // dwRead = IoStatusBlock->Information;

    Src = (PEVENTLOGRECORD)UnicodeBuffer;
    Dst = (PEVENTLOGRECORD)Buffer;

    Dst->Reserved = Src->Reserved;
    Dst->RecordNumber = Src->RecordNumber;
    Dst->TimeGenerated = Src->TimeGenerated;
    Dst->TimeWritten = Src->TimeWritten;
    Dst->EventID = Src->EventID;
    Dst->EventType = Src->EventType;
    Dst->EventCategory = Src->EventCategory;
    Dst->NumStrings = Src->NumStrings;
    Dst->UserSidLength = Src->UserSidLength;
    Dst->DataLength = Src->DataLength;

    SrcPtr = (PVOID)((ULONG_PTR)Src + sizeof(EVENTLOGRECORD));
    DstPtr = (PVOID)((ULONG_PTR)Dst + sizeof(EVENTLOGRECORD));

    /* Convert the module name */
    RtlInitUnicodeString(&StringW, SrcPtr);
    Status = RtlUnicodeStringToAnsiString(&StringA, &StringW, TRUE);
    if (NT_SUCCESS(Status))
    {
        RtlCopyMemory(DstPtr, StringA.Buffer, StringA.MaximumLength);
        DstPtr = (PVOID)((ULONG_PTR)DstPtr + StringA.MaximumLength);

        RtlFreeAnsiString(&StringA);
    }
    else
    {
        RtlZeroMemory(DstPtr, StringW.MaximumLength / sizeof(WCHAR));
        DstPtr = (PVOID)((ULONG_PTR)DstPtr + StringW.MaximumLength / sizeof(WCHAR));
    }
    SrcPtr = (PVOID)((ULONG_PTR)SrcPtr + StringW.MaximumLength);

    /* Convert the computer name */
    RtlInitUnicodeString(&StringW, SrcPtr);
    Status = RtlUnicodeStringToAnsiString(&StringA, &StringW, TRUE);
    if (NT_SUCCESS(Status))
    {
        RtlCopyMemory(DstPtr, StringA.Buffer, StringA.MaximumLength);
        DstPtr = (PVOID)((ULONG_PTR)DstPtr + StringA.MaximumLength);

        RtlFreeAnsiString(&StringA);
    }
    else
    {
        RtlZeroMemory(DstPtr, StringW.MaximumLength / sizeof(WCHAR));
        DstPtr = (PVOID)((ULONG_PTR)DstPtr + StringW.MaximumLength / sizeof(WCHAR));
    }

    /* Add the padding and the User SID */
    dwPadding = sizeof(ULONG) - (((ULONG_PTR)DstPtr - (ULONG_PTR)Dst) % sizeof(ULONG));
    RtlZeroMemory(DstPtr, dwPadding);

    SrcPtr = (PVOID)((ULONG_PTR)Src + Src->UserSidOffset);
    DstPtr = (PVOID)((ULONG_PTR)DstPtr + dwPadding);

    Dst->UserSidOffset = (DWORD)((ULONG_PTR)DstPtr - (ULONG_PTR)Dst);
    RtlCopyMemory(DstPtr, SrcPtr, Src->UserSidLength);

    /* Convert the strings */
    SrcPtr = (PVOID)((ULONG_PTR)Src + Src->StringOffset);
    DstPtr = (PVOID)((ULONG_PTR)DstPtr + Src->UserSidLength);
    Dst->StringOffset = (DWORD)((ULONG_PTR)DstPtr - (ULONG_PTR)Dst);

    for (i = 0; i < Dst->NumStrings; i++)
    {
        RtlInitUnicodeString(&StringW, SrcPtr);
        Status = RtlUnicodeStringToAnsiString(&StringA, &StringW, TRUE);
        if (NT_SUCCESS(Status))
        {
            RtlCopyMemory(DstPtr, StringA.Buffer, StringA.MaximumLength);
            DstPtr = (PVOID)((ULONG_PTR)DstPtr + StringA.MaximumLength);

            RtlFreeAnsiString(&StringA);
        }
        else
        {
            RtlZeroMemory(DstPtr, StringW.MaximumLength / sizeof(WCHAR));
            DstPtr = (PVOID)((ULONG_PTR)DstPtr + StringW.MaximumLength / sizeof(WCHAR));
        }
        SrcPtr = (PVOID)((ULONG_PTR)SrcPtr + StringW.MaximumLength);
    }

    /* Copy the binary data */
    SrcPtr = (PVOID)((ULONG_PTR)Src + Src->DataOffset);
    Dst->DataOffset = (ULONG_PTR)DstPtr - (ULONG_PTR)Dst;
    RtlCopyMemory(DstPtr, SrcPtr, Src->DataLength);
    DstPtr = (PVOID)((ULONG_PTR)DstPtr + Src->DataLength);

    /* Add the padding */
    dwPadding = sizeof(ULONG) - (((ULONG_PTR)DstPtr-(ULONG_PTR)Dst) % sizeof(ULONG));
    RtlZeroMemory(DstPtr, dwPadding);

    dwEntryLength = (DWORD)((ULONG_PTR)DstPtr + dwPadding + sizeof(ULONG) - (ULONG_PTR)Dst);

    /* Set the entry length at the end of the entry */
    pLength = (PDWORD)((ULONG_PTR)DstPtr + dwPadding);
    *pLength = dwEntryLength;
    Dst->Length = dwEntryLength;

    IoStatusBlock->Information = dwEntryLength;

    Status = STATUS_SUCCESS;

done:
    if (UnicodeBuffer != NULL)
        RtlFreeHeap(GetProcessHeap(), 0, UnicodeBuffer);

    return Status;
}

/*
 * NOTE:
 *   'RecordNumber' is a pointer to the record number at which the read operation
 *   should start. If the record number is 0 and the flags given in the 'Flags'
 *   parameter contain EVENTLOG_SEQUENTIAL_READ, an adequate record number is
 *   computed.
 */
NTSTATUS
LogfReadEvents(PLOGFILE LogFile,
               ULONG    Flags,
               PULONG   RecordNumber,
               ULONG    BufSize,
               PBYTE    Buffer,
               PULONG   BytesRead,
               PULONG   BytesNeeded,
               BOOLEAN  Ansi)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER FileOffset;
    DWORD dwOffset, dwRead, dwRecSize;
    DWORD dwBufferUsage, dwRecNum;

    /* Parameters validation */

    /* EVENTLOG_SEQUENTIAL_READ and EVENTLOG_SEEK_READ are mutually exclusive */
    if ((Flags & EVENTLOG_SEQUENTIAL_READ) && (Flags & EVENTLOG_SEEK_READ))
        return STATUS_INVALID_PARAMETER;

    if (!(Flags & EVENTLOG_SEQUENTIAL_READ) && !(Flags & EVENTLOG_SEEK_READ))
        return STATUS_INVALID_PARAMETER;

    /* EVENTLOG_FORWARDS_READ and EVENTLOG_BACKWARDS_READ are mutually exclusive */
    if ((Flags & EVENTLOG_FORWARDS_READ) && (Flags & EVENTLOG_BACKWARDS_READ))
        return STATUS_INVALID_PARAMETER;

    if (!(Flags & EVENTLOG_FORWARDS_READ) && !(Flags & EVENTLOG_BACKWARDS_READ))
        return STATUS_INVALID_PARAMETER;

    if (!Buffer || !BytesRead || !BytesNeeded)
        return STATUS_INVALID_PARAMETER;

    /* In seek read mode, a record number of 0 is invalid */
    if (!(Flags & EVENTLOG_SEQUENTIAL_READ) && (*RecordNumber == 0))
        return STATUS_INVALID_PARAMETER;

    /*
     * In sequential read mode, a record number of 0 means we need
     * to determine where to start the read operation. Otherwise
     * we just use the provided record number.
     */
    if ((Flags & EVENTLOG_SEQUENTIAL_READ) && (*RecordNumber == 0))
    {
        if (Flags & EVENTLOG_FORWARDS_READ)
        {
            *RecordNumber = LogFile->Header.OldestRecordNumber;
        }
        else // if (Flags & EVENTLOG_BACKWARDS_READ)
        {
            *RecordNumber = LogFile->Header.CurrentRecordNumber - 1;
        }
    }

    dwRecNum = *RecordNumber;

    RtlAcquireResourceShared(&LogFile->Lock, TRUE);

    *BytesRead = 0;
    *BytesNeeded = 0;

    dwBufferUsage = 0;
    do
    {
        dwOffset = LogfOffsetByNumber(LogFile, dwRecNum);
        if (dwOffset == 0)
        {
            if (dwBufferUsage == 0)
            {
                RtlReleaseResource(&LogFile->Lock);
                return STATUS_END_OF_FILE;
            }
            else
            {
                break;
            }
        }

        FileOffset.QuadPart = dwOffset;
        Status = NtReadFile(LogFile->hFile,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            &dwRecSize,
                            sizeof(dwRecSize),
                            &FileOffset,
                            NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtReadFile failed (Status 0x%08lx)\n", Status);
            // Status = STATUS_EVENTLOG_FILE_CORRUPT;
            goto Done;
        }
        // dwRead = IoStatusBlock.Information;

        if (dwBufferUsage + dwRecSize > BufSize)
        {
            if (dwBufferUsage == 0)
            {
                *BytesNeeded = dwRecSize;
                RtlReleaseResource(&LogFile->Lock);
                return STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                break;
            }
        }

        FileOffset.QuadPart = dwOffset;
        if (Ansi)
        {
            Status = ReadAnsiLogEntry(LogFile,
                                      &IoStatusBlock,
                                      Buffer + dwBufferUsage,
                                      dwRecSize,
                                      &FileOffset,
                                      NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("ReadAnsiLogEntry failed (Status 0x%08lx)\n", Status);
                // Status = STATUS_EVENTLOG_FILE_CORRUPT;
                goto Done;
            }
        }
        else
        {
            Status = ReadLogBuffer(LogFile,
                                   &IoStatusBlock,
                                   Buffer + dwBufferUsage,
                                   dwRecSize,
                                   &FileOffset,
                                   NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("ReadLogBuffer failed (Status 0x%08lx)\n", Status);
                // Status = STATUS_EVENTLOG_FILE_CORRUPT;
                goto Done;
            }
        }
        dwRead = IoStatusBlock.Information;

        /* Go to the next event record */
        /*
         * NOTE: This implicitely supposes that all the other record numbers
         * are consecutive (and do not jump than more than one unit); but if
         * it is not the case, then we would prefer here to call some
         * "get_next_record_number" function.
         */
        if (Flags & EVENTLOG_FORWARDS_READ)
            dwRecNum++;
        else // if (Flags & EVENTLOG_BACKWARDS_READ)
            dwRecNum--;

        dwBufferUsage += dwRead;
    }
    while (dwBufferUsage <= BufSize);

    *BytesRead = dwBufferUsage;
    *RecordNumber = dwRecNum;
    RtlReleaseResource(&LogFile->Lock);
    return STATUS_SUCCESS;

Done:
    DPRINT1("LogfReadEvents failed (Status 0x%08lx)\n", Status);
    RtlReleaseResource(&LogFile->Lock);
    return Status;
}

NTSTATUS
LogfWriteRecord(PLOGFILE LogFile,
                ULONG BufSize, // SIZE_T
                PEVENTLOGRECORD Record)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER FileOffset, NextOffset;
    DWORD dwWritten;
    // DWORD dwRead;
    LARGE_INTEGER SystemTime;
    EVENTLOGEOF EofRec;
    EVENTLOGRECORD RecBuf;
    ULONG FreeSpace = 0;
    ULONG UpperBound;
    ULONG RecOffset, WriteOffset;

    // ASSERT(sizeof(*Record) == sizeof(RecBuf));

    if (!Record || BufSize < sizeof(*Record))
        return STATUS_INVALID_PARAMETER;

    RtlAcquireResourceExclusive(&LogFile->Lock, TRUE);

    /*
     * Retrieve the record written time now, that will also be compared
     * with the existing events timestamps in case the log is wrapping.
     */
    NtQuerySystemTime(&SystemTime);
    RtlTimeToSecondsSince1970(&SystemTime, &Record->TimeWritten);

    Record->RecordNumber = LogFile->Header.CurrentRecordNumber;

    /* Compute the available log free space */
    if (LogFile->Header.StartOffset <= LogFile->Header.EndOffset)
        FreeSpace = LogFile->Header.MaxSize - LogFile->Header.EndOffset + LogFile->Header.StartOffset - sizeof(EVENTLOGHEADER);
    else // if (LogFile->Header.StartOffset > LogFile->Header.EndOffset)
        FreeSpace = LogFile->Header.StartOffset - LogFile->Header.EndOffset;

    LogFile->Header.Flags |= ELF_LOGFILE_HEADER_DIRTY;

    /* If the event log was empty, it will now contain one record */
    if (LogFile->Header.OldestRecordNumber == 0)
        LogFile->Header.OldestRecordNumber = 1;

    /* By default we append the new record at the old EOF record offset */
    WriteOffset = LogFile->Header.EndOffset;

    /*
     * Check whether the log is going to wrap (the events being overwritten).
     */

    if (LogFile->Header.StartOffset <= LogFile->Header.EndOffset)
        UpperBound = LogFile->Header.MaxSize;
    else // if (LogFile->Header.StartOffset > LogFile->Header.EndOffset)
        UpperBound = LogFile->Header.StartOffset;

    // if (LogFile->Header.MaxSize - WriteOffset < BufSize + sizeof(EofRec))
    if (UpperBound - WriteOffset < BufSize + sizeof(EofRec))
    {
        DPRINT("EventLogFile has reached maximum size (%x), wrapping...\n"
               "UpperBound = %x, WriteOffset = %x, BufSize = %x\n",
               LogFile->Header.MaxSize, UpperBound, WriteOffset, BufSize);
        /* This will be done later */
    }

    if ( (LogFile->Header.StartOffset < LogFile->Header.EndOffset) &&
         (LogFile->Header.MaxSize - WriteOffset < sizeof(RecBuf)) ) // (UpperBound - WriteOffset < sizeof(RecBuf))
    {
        // ASSERT(UpperBound  == LogFile->Header.MaxSize);
        // ASSERT(WriteOffset == LogFile->Header.EndOffset);

        /*
         * We cannot fit the EVENTLOGRECORD header of the buffer before
         * the end of the file. We need to pad the end of the log with
         * 0x00000027, normally we will need to pad at most 0x37 bytes
         * (corresponding to sizeof(EVENTLOGRECORD) - 1).
         */

        /* Rewind to the beginning of the log, just after the header */
        WriteOffset = sizeof(EVENTLOGHEADER);
        /**/UpperBound = LogFile->Header.StartOffset;/**/

        FreeSpace = LogFile->Header.StartOffset - WriteOffset;

        LogFile->Header.Flags |= ELF_LOGFILE_HEADER_WRAP;
    }
    /*
     * Otherwise, we can fit the header and only part
     * of the data will overwrite the oldest records.
     *
     * It might be possible that all the event record can fit in one piece,
     * but that the EOF record needs to be split. This is not a problem,
     * EVENTLOGEOF can be splitted while EVENTLOGRECORD cannot be.
     */

    if (UpperBound - WriteOffset < BufSize + sizeof(EofRec))
    {
        ULONG OrgOldestRecordNumber, OldestRecordNumber;

        // DPRINT("EventLogFile has reached maximum size, wrapping...\n");

        OldestRecordNumber = OrgOldestRecordNumber = LogFile->Header.OldestRecordNumber;

        // FIXME: Assert whether LogFile->Header.StartOffset is the beginning of a record???
        // NOTE: It should be, by construction (and this should have been checked when
        // initializing a new, or existing log).

        /*
         * Determine how many old records need to be overwritten.
         * Check the size of the record as the record added may be larger.
         * Need to take into account that we append the EOF record.
         */
        while (FreeSpace < BufSize + sizeof(EofRec))
        {
            /* Get the oldest record data */
            RecOffset = LogfOffsetByNumber(LogFile, OldestRecordNumber);
            if (RecOffset == 0)
            {
                // TODO: It cannot, queue a message box for the user and exit.
                // See also below...
                DPRINT1("Record number %d cannot be found, or LogFile is full and cannot wrap!\n", OldestRecordNumber);
                Status = STATUS_LOG_FILE_FULL; // STATUS_LOG_FULL;
                goto Quit;
            }

            RtlZeroMemory(&RecBuf, sizeof(RecBuf));

            FileOffset.QuadPart = RecOffset;
            Status = NtReadFile(LogFile->hFile,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                &RecBuf,
                                sizeof(RecBuf),
                                &FileOffset,
                                NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtReadFile failed (Status 0x%08lx)\n", Status);
                // Status = STATUS_EVENTLOG_FILE_CORRUPT;
                goto Quit;
            }
            // dwRead = IoStatusBlock.Information;

            if (RecBuf.Reserved != LOGFILE_SIGNATURE)
            {
                DPRINT1("LogFile corrupt!\n");
                Status = STATUS_EVENTLOG_FILE_CORRUPT;
                goto Quit;
            }

            /*
             * Check whether this event can be overwritten by comparing its
             * written timestamp with the log's retention value. This value
             * is the time interval, in seconds, that events records are
             * protected from being overwritten.
             *
             * If the retention value is zero the events are always overwritten.
             *
             * If the retention value is non-zero, when the age of an event,
             * in seconds, reaches or exceeds this value, it can be overwritten.
             * Also if the events are in the future, we do not overwrite them.
             */
            if (LogFile->Header.Retention != 0 &&
                (Record->TimeWritten <  RecBuf.TimeWritten ||
                (Record->TimeWritten >= RecBuf.TimeWritten &&
                 Record->TimeWritten -  RecBuf.TimeWritten < LogFile->Header.Retention)))
            {
                // TODO: It cannot, queue a message box for the user and exit.
                DPRINT1("LogFile is full and cannot wrap!\n");
                Status = STATUS_LOG_FILE_FULL; // STATUS_LOG_FULL;
                goto Quit;
            }

            /*
             * Advance the oldest record number, add the event record length
             * (as long as it is valid...) then take account for the possible
             * paddind after the record, in case this is the last one at the
             * end of the file.
             */
            OldestRecordNumber++;
            RecOffset += RecBuf.Length;
            FreeSpace += RecBuf.Length;

            /*
             * If this was the last event record before the end of the log file,
             * the next one should start at the beginning of the log and the space
             * between the last event record and the end of the file is padded.
             */
            if (LogFile->Header.MaxSize - RecOffset < sizeof(EVENTLOGRECORD))
            {
                /* Add the padding size */
                FreeSpace += LogFile->Header.MaxSize - RecOffset;
            }
        }

        DPRINT("Record will fit. FreeSpace %d, BufSize %d\n", FreeSpace, BufSize);

        /* The log records are wrapping */
        LogFile->Header.Flags |= ELF_LOGFILE_HEADER_WRAP;


        // FIXME: May lead to corruption if the other subsequent calls fail...

        /*
         * We have validated all the region of events to be discarded,
         * now we can perform their deletion.
         */
        LogfDeleteOffsetInformation(LogFile, OrgOldestRecordNumber, OldestRecordNumber - 1);
        LogFile->Header.OldestRecordNumber = OldestRecordNumber;
        LogFile->Header.StartOffset = LogfOffsetByNumber(LogFile, OldestRecordNumber);
        if (LogFile->Header.StartOffset == 0)
        {
            /*
             * We have deleted all the existing event records to make place
             * for the new one. We can put it at the start of the event log.
             */
            LogFile->Header.StartOffset = sizeof(EVENTLOGHEADER);
            WriteOffset = LogFile->Header.StartOffset;
            LogFile->Header.EndOffset = WriteOffset;
        }

        DPRINT1("MaxSize = %x, StartOffset = %x, WriteOffset = %x, EndOffset = %x, BufSize = %x\n"
                "OldestRecordNumber = %d\n",
                LogFile->Header.MaxSize, LogFile->Header.StartOffset, WriteOffset, LogFile->Header.EndOffset, BufSize,
                OldestRecordNumber);
    }

    /*
     * Expand the log file if needed.
     * NOTE: It may be needed to perform this task a bit sooner if we need
     * such a thing for performing read operations, in the future...
     * Or if this operation needs to modify 'FreeSpace'...
     */
    if (LogFile->CurrentSize < LogFile->Header.MaxSize)
    {
        DPRINT1("Expanding the log file from %lu to %lu\n",
                LogFile->CurrentSize, LogFile->Header.MaxSize);

        /* For the moment this is a trivial operation */
        LogFile->CurrentSize = LogFile->Header.MaxSize;
    }

    /* Pad the end of the log */
    // if (LogFile->Header.EndOffset + sizeof(RecBuf) > LogFile->Header.MaxSize)
    if (WriteOffset < LogFile->Header.EndOffset)
    {
        /* Pad all the space from LogFile->Header.EndOffset to LogFile->Header.MaxSize */
        dwWritten = ROUND_DOWN(LogFile->Header.MaxSize - LogFile->Header.EndOffset, sizeof(ULONG));
        RtlFillMemoryUlong(&RecBuf, dwWritten, 0x00000027);

        FileOffset.QuadPart = LogFile->Header.EndOffset;
        Status = NtWriteFile(LogFile->hFile,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             &RecBuf,
                             dwWritten,
                             &FileOffset,
                             NULL);
        // dwWritten = IoStatusBlock.Information;
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtWriteFile failed (Status 0x%08lx)\n", Status);
            // goto Quit;
        }
    }

    /* Write the event record buffer with possible wrap at offset sizeof(EVENTLOGHEADER) */
    FileOffset.QuadPart = WriteOffset;
    Status = WriteLogBuffer(LogFile,
                            &IoStatusBlock,
                            Record,
                            BufSize,
                            &FileOffset,
                            &NextOffset);
    // dwWritten = IoStatusBlock.Information;
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WriteLogBuffer failed (Status 0x%08lx)\n", Status);
        goto Quit;
    }
    /* FileOffset now contains the offset just after the end of the record buffer */
    FileOffset = NextOffset;

    if (!LogfAddOffsetInformation(LogFile,
                                  Record->RecordNumber,
                                  WriteOffset))
    {
        Status = STATUS_NO_MEMORY; // STATUS_EVENTLOG_FILE_CORRUPT;
        goto Quit;
    }

    LogFile->Header.CurrentRecordNumber++;
    if (LogFile->Header.CurrentRecordNumber == 0)
        LogFile->Header.CurrentRecordNumber = 1;

    /*
     * Write the new EOF record offset just after the event record.
     * The EOF record can wrap (be splitted) if less than sizeof(EVENTLOGEOF)
     * bytes remains between the end of the record and the end of the log file.
     */
    LogFile->Header.EndOffset = FileOffset.QuadPart;

    RtlCopyMemory(&EofRec, &EOFRecord, sizeof(EOFRecord));
    EofRec.BeginRecord = LogFile->Header.StartOffset;
    EofRec.EndRecord   = LogFile->Header.EndOffset;
    EofRec.CurrentRecordNumber = LogFile->Header.CurrentRecordNumber;
    EofRec.OldestRecordNumber  = LogFile->Header.OldestRecordNumber;

    // FileOffset.QuadPart = LogFile->Header.EndOffset;
    Status = WriteLogBuffer(LogFile,
                            &IoStatusBlock,
                            &EofRec,
                            sizeof(EofRec),
                            &FileOffset,
                            &NextOffset);
    // dwWritten = IoStatusBlock.Information;
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WriteLogBuffer failed (Status 0x%08lx)\n", Status);
        goto Quit;
    }
    FileOffset = NextOffset;

    LogFile->Header.Flags &= ELF_LOGFILE_HEADER_DIRTY;

    /* Update the event log header */
    FileOffset.QuadPart = 0LL;
    Status = NtWriteFile(LogFile->hFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &LogFile->Header,
                         sizeof(EVENTLOGHEADER),
                         &FileOffset,
                         NULL);
    // dwWritten = IoStatusBlock.Information;
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile failed (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    Status = NtFlushBuffersFile(LogFile->hFile, &IoStatusBlock);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtFlushBuffersFile failed (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    Status = STATUS_SUCCESS;

Quit:
    RtlReleaseResource(&LogFile->Lock);
    return Status;
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
        Status = LogfBackupFile(LogFile, BackupFileName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("LogfBackupFile failed (Status: 0x%08lx)\n", Status);
            goto Quit;
        }
    }

    Status = LogfInitializeNew(LogFile,
                               LogFile->Header.MaxSize,
                               LogFile->Header.Retention);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LogfInitializeNew failed (Status: 0x%08lx)\n", Status);
    }

Quit:
    RtlReleaseResource(&LogFile->Lock);
    return Status;
}

NTSTATUS
LogfBackupFile(PLOGFILE LogFile,
               PUNICODE_STRING BackupFileName)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER FileOffset;
    HANDLE FileHandle = NULL;
    EVENTLOGHEADER Header;
    EVENTLOGRECORD RecBuf;
    EVENTLOGEOF EofRec;
    ULONG i;
    ULONG RecOffset;
    PVOID Buffer = NULL;

    // DWORD dwRead;

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
        DPRINT("Cannot create backup file %wZ (Status: 0x%08lx)\n", BackupFileName, Status);
        goto Done;
    }

    /* Initialize the (dirty) log file header */
    Header.HeaderSize = sizeof(Header);
    Header.Signature = LOGFILE_SIGNATURE;
    Header.MajorVersion = MAJORVER;
    Header.MinorVersion = MINORVER;
    Header.StartOffset = sizeof(Header);
    Header.EndOffset = sizeof(Header);
    Header.CurrentRecordNumber = 1;
    Header.OldestRecordNumber = 0;
    Header.MaxSize = LogFile->Header.MaxSize;
    Header.Flags = ELF_LOGFILE_HEADER_DIRTY;
    Header.Retention = LogFile->Header.Retention;
    Header.EndHeaderSize = sizeof(Header);

    /* Write the (dirty) log file header */
    FileOffset.QuadPart = 0LL;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &Header,
                         sizeof(Header),
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to write the log file header (Status: 0x%08lx)\n", Status);
        goto Done;
    }

    for (i = LogFile->Header.OldestRecordNumber; i < LogFile->Header.CurrentRecordNumber; i++)
    {
        RecOffset = LogfOffsetByNumber(LogFile, i);
        if (RecOffset == 0)
            break;

        /* Read the next EVENTLOGRECORD header at once (it cannot be split) */
        FileOffset.QuadPart = RecOffset;
        Status = NtReadFile(LogFile->hFile,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            &RecBuf,
                            sizeof(RecBuf),
                            &FileOffset,
                            NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtReadFile failed (Status 0x%08lx)\n", Status);
            goto Done;
        }
        // dwRead = IoStatusBlock.Information;

        // if (dwRead != sizeof(RecBuf))
            // break;

        Buffer = RtlAllocateHeap(GetProcessHeap(), 0, RecBuf.Length);
        if (Buffer == NULL)
        {
            DPRINT1("RtlAllocateHeap() failed!\n");
            goto Done;
        }

        /* Read the full EVENTLOGRECORD (header + data) with wrapping */
        Status = ReadLogBuffer(LogFile,
                               &IoStatusBlock,
                               Buffer,
                               RecBuf.Length,
                               &FileOffset,
                               NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ReadLogBuffer failed (Status 0x%08lx)\n", Status);
            RtlFreeHeap(GetProcessHeap(), 0, Buffer);
            // Status = STATUS_EVENTLOG_FILE_CORRUPT;
            goto Done;
        }
        // dwRead = IoStatusBlock.Information;

        /* Write the event record (no wrap for the backup log) */
        Status = NtWriteFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             Buffer,
                             RecBuf.Length,
                             NULL,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtWriteFile() failed! (Status: 0x%08lx)\n", Status);
            RtlFreeHeap(GetProcessHeap(), 0, Buffer);
            goto Done;
        }

        /* Update the header information */
        Header.EndOffset += RecBuf.Length;

        /* Free the buffer */
        RtlFreeHeap(GetProcessHeap(), 0, Buffer);
        Buffer = NULL;
    }

    /* Initialize the EOF record */
    RtlCopyMemory(&EofRec, &EOFRecord, sizeof(EOFRecord));
    EofRec.BeginRecord = Header.StartOffset;
    EofRec.EndRecord   = Header.EndOffset;
    EofRec.CurrentRecordNumber = LogFile->Header.CurrentRecordNumber;
    EofRec.OldestRecordNumber  = LogFile->Header.OldestRecordNumber;

    /* Write the EOF record (no wrap for the backup log) */
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &EofRec,
                         sizeof(EofRec),
                         NULL,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile() failed!\n");
        goto Done;
    }

    /* Update the header information */
    Header.CurrentRecordNumber = LogFile->Header.CurrentRecordNumber;
    Header.OldestRecordNumber  = LogFile->Header.OldestRecordNumber;
    Header.MaxSize = ROUND_UP(Header.EndOffset + sizeof(EofRec), sizeof(ULONG));
    Header.Flags = 0;

    /* Write the (clean) log file header */
    FileOffset.QuadPart = 0LL;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &Header,
                         sizeof(Header),
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile() failed! (Status: 0x%08lx)\n", Status);
    }

Done:
    /* Close the backup file */
    if (FileHandle != NULL)
        NtClose(FileHandle);

    /* Unlock the log file */
    RtlReleaseResource(&LogFile->Lock);

    return Status;
}


PEVENTLOGRECORD
LogfAllocAndBuildNewRecord(PULONG lpRecSize,
                           ULONG  Time,
                           USHORT wType,
                           USHORT wCategory,
                           ULONG  dwEventId,
                           PCWSTR SourceName,
                           PCWSTR ComputerName,
                           ULONG  dwSidLength,
                           PSID   lpUserSid,
                           USHORT wNumStrings,
                           PWSTR  lpStrings,
                           ULONG  dwDataSize,
                           PVOID  lpRawData)
{
    DWORD dwRecSize;
    PEVENTLOGRECORD pRec;
    PWSTR str;
    UINT i, pos;
    SIZE_T SourceNameLen, ComputerNameLen, StringLen;
    PBYTE Buffer;

    SourceNameLen = (SourceName ? wcslen(SourceName) : 0) + 1;
    ComputerNameLen = (ComputerName ? wcslen(ComputerName) : 0) + 1;

    dwRecSize = sizeof(EVENTLOGRECORD) + (SourceNameLen + ComputerNameLen) * sizeof(WCHAR);

    /* Align on DWORD boundary for the SID */
    dwRecSize = ROUND_UP(dwRecSize, sizeof(ULONG));

    dwRecSize += dwSidLength;

    /* Add the sizes for the strings array */
    ASSERT((lpStrings == NULL && wNumStrings == 0) ||
           (lpStrings != NULL && wNumStrings >= 0));
    for (i = 0, str = lpStrings; i < wNumStrings; i++)
    {
        StringLen = wcslen(str) + 1; // str must be != NULL
        dwRecSize += StringLen * sizeof(WCHAR);
        str += StringLen;
    }

    /* Add the data size */
    dwRecSize += dwDataSize;

    /* Align on DWORD boundary for the full structure */
    dwRecSize = ROUND_UP(dwRecSize, sizeof(ULONG));

    /* Size of the trailing 'Length' member */
    dwRecSize += sizeof(ULONG);

    Buffer = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, dwRecSize);
    if (!Buffer)
    {
        DPRINT1("Cannot allocate heap!\n");
        return NULL;
    }

    pRec = (PEVENTLOGRECORD)Buffer;
    pRec->Length = dwRecSize;
    pRec->Reserved = LOGFILE_SIGNATURE;

    /*
     * Do not assign here any precomputed record number to the event record.
     * The true record number will be assigned atomically and sequentially in
     * LogfWriteRecord, so that all the event records will have consistent and
     * unique record numbers.
     */
    pRec->RecordNumber = 0;

    /*
     * Set the generated time, and temporarily set the written time
     * with the generated time.
     */
    pRec->TimeGenerated = Time;
    pRec->TimeWritten   = Time;

    pRec->EventID = dwEventId;
    pRec->EventType = wType;
    pRec->EventCategory = wCategory;

    pos = sizeof(EVENTLOGRECORD);

    if (SourceName)
        StringCchCopy((PWSTR)(Buffer + pos), SourceNameLen, SourceName);
    pos += SourceNameLen * sizeof(WCHAR);
    if (ComputerName)
        StringCchCopy((PWSTR)(Buffer + pos), ComputerNameLen, ComputerName);
    pos += ComputerNameLen * sizeof(WCHAR);

    /* Align on DWORD boundary for the SID */
    pos = ROUND_UP(pos, sizeof(ULONG));

    pRec->UserSidLength = 0;
    pRec->UserSidOffset = 0;
    if (dwSidLength)
    {
        RtlCopyMemory(Buffer + pos, lpUserSid, dwSidLength);
        pRec->UserSidLength = dwSidLength;
        pRec->UserSidOffset = pos;
        pos += dwSidLength;
    }

    pRec->StringOffset = pos;
    for (i = 0, str = lpStrings; i < wNumStrings; i++)
    {
        StringLen = wcslen(str) + 1; // str must be != NULL
        StringCchCopy((PWSTR)(Buffer + pos), StringLen, str);
        str += StringLen;
        pos += StringLen * sizeof(WCHAR);
    }
    pRec->NumStrings = wNumStrings;

    pRec->DataLength = 0;
    pRec->DataOffset = 0;
    if (dwDataSize)
    {
        RtlCopyMemory(Buffer + pos, lpRawData, dwDataSize);
        pRec->DataLength = dwDataSize;
        pRec->DataOffset = pos;
        pos += dwDataSize;
    }

    /* Align on DWORD boundary for the full structure */
    pos = ROUND_UP(pos, sizeof(ULONG));

    /* Initialize the trailing 'Length' member */
    *((PDWORD) (Buffer + pos)) = dwRecSize;

    *lpRecSize = dwRecSize;
    return pRec;
}

VOID
LogfReportEvent(USHORT wType,
                USHORT wCategory,
                ULONG  dwEventId,
                USHORT wNumStrings,
                PWSTR  lpStrings,
                ULONG  dwDataSize,
                PVOID  lpRawData)
{
    NTSTATUS Status;
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
    PEVENTLOGRECORD logBuffer;
    LARGE_INTEGER SystemTime;
    ULONG Time;
    DWORD recSize;

    if (!EventLogSource)
        return;

    if (!GetComputerNameW(szComputerName, &dwComputerNameLength))
    {
        szComputerName[0] = L'\0';
    }

    NtQuerySystemTime(&SystemTime);
    RtlTimeToSecondsSince1970(&SystemTime, &Time);

    logBuffer = LogfAllocAndBuildNewRecord(&recSize,
                                           Time,
                                           wType,
                                           wCategory,
                                           dwEventId,
                                           EventLogSource->szName,
                                           szComputerName,
                                           0,
                                           NULL,
                                           wNumStrings,
                                           lpStrings,
                                           dwDataSize,
                                           lpRawData);

    Status = LogfWriteRecord(EventLogSource->LogFile, recSize, logBuffer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR WRITING TO EventLog %S (Status 0x%08lx)\n",
                EventLogSource->LogFile->FileName, Status);
    }

    LogfFreeRecord(logBuffer);
}
