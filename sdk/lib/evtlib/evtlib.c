/*
 * PROJECT:         ReactOS EventLog File Library
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            sdk/lib/evtlib/evtlib.c
 * PURPOSE:         Provides functionality for reading and writing
 *                  EventLog files in the NT <= 5.2 (.evt) format.
 * PROGRAMMERS:     Copyright 2005 Saveliy Tretiakov
 *                  Michael Martin
 *                  Hermes Belusca-Maito
 */

/* INCLUDES ******************************************************************/

#include "evtlib.h"

#define NDEBUG
#include <debug.h>

#define EVTLTRACE(...)  DPRINT("EvtLib: " __VA_ARGS__)
// Once things become stabilized enough, replace all the EVTLTRACE1 by EVTLTRACE
#define EVTLTRACE1(...)  DPRINT1("EvtLib: " __VA_ARGS__)


/* GLOBALS *******************************************************************/

static const EVENTLOGEOF EOFRecord =
{
    sizeof(EOFRecord),
    0x11111111, 0x22222222, 0x33333333, 0x44444444,
    0, 0, 0, 0,
    sizeof(EOFRecord)
};

/* HELPER FUNCTIONS **********************************************************/

static NTSTATUS
ReadLogBuffer(
    IN  PEVTLOGFILE LogFile,
    OUT PVOID   Buffer,
    IN  SIZE_T  Length,
    OUT PSIZE_T ReadLength OPTIONAL,
    IN  PLARGE_INTEGER ByteOffset,
    OUT PLARGE_INTEGER NextOffset OPTIONAL)
{
    NTSTATUS Status;
    LARGE_INTEGER FileOffset;
    SIZE_T BufSize;
    SIZE_T ReadBufLength = 0, OldReadBufLength;

    ASSERT(LogFile->CurrentSize <= LogFile->Header.MaxSize);
    ASSERT(ByteOffset->QuadPart <= LogFile->CurrentSize);

    if (ReadLength)
        *ReadLength = 0;

    if (NextOffset)
        NextOffset->QuadPart = 0LL;

    /* Read the first part of the buffer */
    FileOffset = *ByteOffset;
    BufSize = min(Length, LogFile->CurrentSize - FileOffset.QuadPart);

    Status = LogFile->FileRead(LogFile,
                               &FileOffset,
                               Buffer,
                               BufSize,
                               &ReadBufLength);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE("FileRead() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    if (Length > BufSize)
    {
        OldReadBufLength = ReadBufLength;

        /*
         * The buffer was splitted in two, its second part
         * is to be read at the beginning of the log.
         */
        Buffer = (PVOID)((ULONG_PTR)Buffer + BufSize);
        BufSize = Length - BufSize;
        FileOffset.QuadPart = sizeof(EVENTLOGHEADER);

        Status = LogFile->FileRead(LogFile,
                                   &FileOffset,
                                   Buffer,
                                   BufSize,
                                   &ReadBufLength);
        if (!NT_SUCCESS(Status))
        {
            EVTLTRACE("FileRead() failed (Status 0x%08lx)\n", Status);
            return Status;
        }
        /* Add the read number of bytes from the first read */
        ReadBufLength += OldReadBufLength;
    }

    if (ReadLength)
        *ReadLength = ReadBufLength;

    /* We return the offset just after the end of the read buffer */
    if (NextOffset)
        NextOffset->QuadPart = FileOffset.QuadPart + BufSize;

    return Status;
}

static NTSTATUS
WriteLogBuffer(
    IN  PEVTLOGFILE LogFile,
    IN  PVOID   Buffer,
    IN  SIZE_T  Length,
    OUT PSIZE_T WrittenLength OPTIONAL,
    IN  PLARGE_INTEGER ByteOffset,
    OUT PLARGE_INTEGER NextOffset OPTIONAL)
{
    NTSTATUS Status;
    LARGE_INTEGER FileOffset;
    SIZE_T BufSize;
    SIZE_T WrittenBufLength = 0, OldWrittenBufLength;

    /* We must have write access to the log file */
    ASSERT(!LogFile->ReadOnly);

    /*
     * It is expected that the log file is already correctly expanded
     * before we can write in it. Therefore the following assertions hold.
     */
    ASSERT(LogFile->CurrentSize <= LogFile->Header.MaxSize);
    ASSERT(ByteOffset->QuadPart <= LogFile->CurrentSize);

    if (WrittenLength)
        *WrittenLength = 0;

    if (NextOffset)
        NextOffset->QuadPart = 0LL;

    /* Write the first part of the buffer */
    FileOffset = *ByteOffset;
    BufSize = min(Length, LogFile->CurrentSize - FileOffset.QuadPart);

    Status = LogFile->FileWrite(LogFile,
                                &FileOffset,
                                Buffer,
                                BufSize,
                                &WrittenBufLength);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE("FileWrite() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    if (Length > BufSize)
    {
        OldWrittenBufLength = WrittenBufLength;

        /*
         * The buffer was splitted in two, its second part
         * is written at the beginning of the log.
         */
        Buffer = (PVOID)((ULONG_PTR)Buffer + BufSize);
        BufSize = Length - BufSize;
        FileOffset.QuadPart = sizeof(EVENTLOGHEADER);

        Status = LogFile->FileWrite(LogFile,
                                    &FileOffset,
                                    Buffer,
                                    BufSize,
                                    &WrittenBufLength);
        if (!NT_SUCCESS(Status))
        {
            EVTLTRACE("FileWrite() failed (Status 0x%08lx)\n", Status);
            return Status;
        }
        /* Add the written number of bytes from the first write */
        WrittenBufLength += OldWrittenBufLength;

        /* The log wraps */
        LogFile->Header.Flags |= ELF_LOGFILE_HEADER_WRAP;
    }

    if (WrittenLength)
        *WrittenLength = WrittenBufLength;

    /* We return the offset just after the end of the written buffer */
    if (NextOffset)
        NextOffset->QuadPart = FileOffset.QuadPart + BufSize;

    return Status;
}


/* Returns 0 if nothing is found */
static ULONG
ElfpOffsetByNumber(
    IN PEVTLOGFILE LogFile,
    IN ULONG RecordNumber)
{
    UINT i;

    for (i = 0; i < LogFile->OffsetInfoNext; i++)
    {
        if (LogFile->OffsetInfo[i].EventNumber == RecordNumber)
            return LogFile->OffsetInfo[i].EventOffset;
    }
    return 0;
}

#define OFFSET_INFO_INCREMENT   64

static BOOL
ElfpAddOffsetInformation(
    IN PEVTLOGFILE LogFile,
    IN ULONG ulNumber,
    IN ULONG ulOffset)
{
    PVOID NewOffsetInfo;

    if (LogFile->OffsetInfoNext == LogFile->OffsetInfoSize)
    {
        /* Allocate a new offset table */
        NewOffsetInfo = LogFile->Allocate((LogFile->OffsetInfoSize + OFFSET_INFO_INCREMENT) *
                                              sizeof(EVENT_OFFSET_INFO),
                                          HEAP_ZERO_MEMORY,
                                          TAG_ELF);
        if (!NewOffsetInfo)
        {
            EVTLTRACE1("Cannot reallocate heap.\n");
            return FALSE;
        }

        /* Free the old offset table and use the new one */
        if (LogFile->OffsetInfo)
        {
            /* Copy the handles from the old table to the new one */
            RtlCopyMemory(NewOffsetInfo,
                          LogFile->OffsetInfo,
                          LogFile->OffsetInfoSize * sizeof(EVENT_OFFSET_INFO));
            LogFile->Free(LogFile->OffsetInfo, 0, TAG_ELF);
        }
        LogFile->OffsetInfo = (PEVENT_OFFSET_INFO)NewOffsetInfo;
        LogFile->OffsetInfoSize += OFFSET_INFO_INCREMENT;
    }

    LogFile->OffsetInfo[LogFile->OffsetInfoNext].EventNumber = ulNumber;
    LogFile->OffsetInfo[LogFile->OffsetInfoNext].EventOffset = ulOffset;
    LogFile->OffsetInfoNext++;

    return TRUE;
}

static BOOL
ElfpDeleteOffsetInformation(
    IN PEVTLOGFILE LogFile,
    IN ULONG ulNumberMin,
    IN ULONG ulNumberMax)
{
    UINT i;

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
ElfpInitNewFile(
    IN PEVTLOGFILE LogFile,
    IN ULONG FileSize,
    IN ULONG MaxSize,
    IN ULONG Retention)
{
    NTSTATUS Status;
    LARGE_INTEGER FileOffset;
    SIZE_T WrittenLength;
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
    LogFile->Header.MaxSize = ROUND_UP(MaxSize, sizeof(ULONG));
    LogFile->CurrentSize = LogFile->Header.MaxSize; // or: FileSize ??
    LogFile->FileSetSize(LogFile, LogFile->CurrentSize, 0);

    LogFile->Header.Flags = 0;
    LogFile->Header.Retention = Retention;
    LogFile->Header.EndHeaderSize = sizeof(EVENTLOGHEADER);

    /* Write the header */
    FileOffset.QuadPart = 0LL;
    Status = LogFile->FileWrite(LogFile,
                                &FileOffset,
                                &LogFile->Header,
                                sizeof(EVENTLOGHEADER),
                                &WrittenLength);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE1("FileWrite() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Initialize the ELF_EOF_RECORD and write it */
    RtlCopyMemory(&EofRec, &EOFRecord, sizeof(EOFRecord));
    EofRec.BeginRecord = LogFile->Header.StartOffset;
    EofRec.EndRecord   = LogFile->Header.EndOffset;
    EofRec.CurrentRecordNumber = LogFile->Header.CurrentRecordNumber;
    EofRec.OldestRecordNumber  = LogFile->Header.OldestRecordNumber;

    Status = LogFile->FileWrite(LogFile,
                                NULL,
                                &EofRec,
                                sizeof(EofRec),
                                &WrittenLength);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE1("FileWrite() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    Status = LogFile->FileFlush(LogFile, NULL, 0);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE1("FileFlush() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
ElfpInitExistingFile(
    IN PEVTLOGFILE LogFile,
    IN ULONG FileSize,
    // IN ULONG MaxSize,
    IN ULONG Retention)
{
    NTSTATUS Status;
    LARGE_INTEGER FileOffset, NextOffset;
    SIZE_T ReadLength;
    ULONG RecordNumber = 0;
    ULONG RecOffset;
    PULONG pRecSize2;
    EVENTLOGEOF EofRec;
    EVENTLOGRECORD RecBuf;
    PEVENTLOGRECORD pRecBuf;
    BOOLEAN Wrapping = FALSE;
    BOOLEAN IsLogDirty = FALSE;

    /* Read the log header */
    FileOffset.QuadPart = 0LL;
    Status = LogFile->FileRead(LogFile,
                               &FileOffset,
                               &LogFile->Header,
                               sizeof(EVENTLOGHEADER),
                               &ReadLength);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE1("FileRead() failed (Status 0x%08lx)\n", Status);
        return STATUS_EVENTLOG_FILE_CORRUPT; // return Status;
    }
    if (ReadLength != sizeof(EVENTLOGHEADER))
    {
        EVTLTRACE("Invalid file `%wZ'.\n", &LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    /* Header validity checks */

    if (LogFile->Header.HeaderSize != sizeof(EVENTLOGHEADER) ||
        LogFile->Header.EndHeaderSize != sizeof(EVENTLOGHEADER))
    {
        EVTLTRACE("Invalid header size in `%wZ'.\n", &LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    if (LogFile->Header.Signature != LOGFILE_SIGNATURE)
    {
        EVTLTRACE("Invalid signature %x in `%wZ'.\n",
               LogFile->Header.Signature, &LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    IsLogDirty = (LogFile->Header.Flags & ELF_LOGFILE_HEADER_DIRTY);

    /* If the log is read-only (e.g. a backup log) and is dirty, then it is corrupted */
    if (LogFile->ReadOnly && IsLogDirty)
    {
        EVTLTRACE("Read-only log `%wZ' is dirty.\n", &LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    LogFile->CurrentSize = FileSize;
    // FIXME!! What to do? And what to do if the MaxSize from the registry
    // is strictly less than the CurrentSize?? Should we "reduce" the log size
    // by clearing it completely??
    // --> ANSWER: Save the new MaxSize somewhere, and only when the log is
    //     being cleared, use the new MaxSize to resize (ie. shrink) it.
    // LogFile->FileSetSize(LogFile, LogFile->CurrentSize, 0);

    /* Adjust the log maximum size if needed */
    if (LogFile->CurrentSize > LogFile->Header.MaxSize)
        LogFile->Header.MaxSize = LogFile->CurrentSize;

    /*
     * Reset the log retention value. The value stored
     * in the log file is just for information purposes.
     */
    LogFile->Header.Retention = Retention;

    /*
     * For a non-read-only dirty log, the most up-to-date information about
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
            EVTLTRACE1("EOF record not found!\n");
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }

        /* Attempt to read the fixed part of an EVENTLOGEOF (may wrap) */
        Status = ReadLogBuffer(LogFile,
                               &EofRec,
                               EVENTLOGEOF_SIZE_FIXED,
                               &ReadLength,
                               &FileOffset,
                               NULL);
        if (!NT_SUCCESS(Status))
        {
            EVTLTRACE1("ReadLogBuffer failed (Status 0x%08lx)\n", Status);
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }
        if (ReadLength != EVENTLOGEOF_SIZE_FIXED)
        {
            EVTLTRACE1("Cannot read at most an EOF record!\n");
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
                           &EofRec,
                           sizeof(EofRec),
                           &ReadLength,
                           &FileOffset,
                           NULL);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE1("ReadLogBuffer failed (Status 0x%08lx)\n", Status);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }
    if (ReadLength != sizeof(EofRec))
    {
        EVTLTRACE1("Cannot read the full EOF record!\n");
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    /* Complete validity checks */
    if ((EofRec.RecordSizeEnd != EofRec.RecordSizeBeginning) ||
        (EofRec.EndRecord != FileOffset.QuadPart))
    {
        DPRINT1("EOF record %llx is corrupted (0x%x vs. 0x%x ; 0x%x vs. 0x%llx), expected 0x%x 0x%x!\n",
                FileOffset.QuadPart,
                EofRec.RecordSizeEnd, EofRec.RecordSizeBeginning,
                EofRec.EndRecord, FileOffset.QuadPart,
                EOFRecord.RecordSizeEnd, EOFRecord.RecordSizeBeginning);
        DPRINT1("RecordSizeEnd = 0x%x\n", EofRec.RecordSizeEnd);
        DPRINT1("RecordSizeBeginning = 0x%x\n", EofRec.RecordSizeBeginning);
        DPRINT1("EndRecord = 0x%x\n", EofRec.EndRecord);
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
    if (!LogFile->ReadOnly && IsLogDirty)
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
        EVTLTRACE("Invalid start offset 0x%x in `%wZ'.\n",
               LogFile->Header.StartOffset, &LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }
    if ((LogFile->Header.EndOffset >= LogFile->CurrentSize) ||
        (LogFile->Header.EndOffset & 3) != 0) // EndOffset % sizeof(ULONG) != 0
    {
        EVTLTRACE("Invalid EOF offset 0x%x in `%wZ'.\n",
               LogFile->Header.EndOffset, &LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    if ((LogFile->Header.StartOffset != LogFile->Header.EndOffset) &&
        (LogFile->Header.MaxSize - LogFile->Header.StartOffset < sizeof(EVENTLOGRECORD)))
    {
        /*
         * If StartOffset does not point to EndOffset i.e. to an EVENTLOGEOF,
         * it should point to a non-splitted EVENTLOGRECORD.
         */
        EVTLTRACE("Invalid start offset 0x%x in `%wZ'.\n",
               LogFile->Header.StartOffset, &LogFile->FileName);
        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    if ((LogFile->Header.StartOffset < LogFile->Header.EndOffset) &&
        (LogFile->Header.EndOffset - LogFile->Header.StartOffset < sizeof(EVENTLOGRECORD)))
    {
        /*
         * In non-wrapping case, there must be enough space between StartOffset
         * and EndOffset to contain at least a full EVENTLOGRECORD.
         */
        EVTLTRACE("Invalid start offset 0x%x or end offset 0x%x in `%wZ'.\n",
               LogFile->Header.StartOffset, LogFile->Header.EndOffset, &LogFile->FileName);
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
            EVTLTRACE("Invalid EOF offset 0x%x in `%wZ'.\n",
                   LogFile->Header.EndOffset, &LogFile->FileName);
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
            EVTLTRACE("Invalid EOF offset 0x%x in `%wZ'.\n",
                   LogFile->Header.EndOffset, &LogFile->FileName);
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

    DPRINT("StartOffset = 0x%x, EndOffset = 0x%x\n",
           LogFile->Header.StartOffset, LogFile->Header.EndOffset);

    /*
     * For non-read-only logs of size < MaxSize, reorganize the events
     * such that they do not wrap as soon as we write new ones.
     */
#if 0
    if (!LogFile->ReadOnly)
    {
        pRecBuf = LogFile->Allocate(RecBuf.Length, 0, TAG_ELF_BUF);
        if (pRecBuf == NULL)
        {
            DPRINT1("Cannot allocate temporary buffer, skip event reorganization.\n");
            goto Continue;
        }

        // TODO: Do the job!
    }

Continue:

    DPRINT1("StartOffset = 0x%x, EndOffset = 0x%x\n",
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
        Status = LogFile->FileRead(LogFile,
                                   &FileOffset,
                                   &RecBuf,
                                   sizeof(RecBuf),
                                   &ReadLength);
        if (!NT_SUCCESS(Status))
        {
            EVTLTRACE1("FileRead() failed (Status 0x%08lx)\n", Status);
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }
        if (ReadLength != sizeof(RecBuf))
        {
            DPRINT1("Length != sizeof(RecBuf)\n");
            break;
        }

        if (RecBuf.Reserved != LOGFILE_SIGNATURE ||
            RecBuf.Length < sizeof(EVENTLOGRECORD))
        {
            DPRINT1("RecBuf problem\n");
            break;
        }

        /* Allocate a full EVENTLOGRECORD (header + data) */
        pRecBuf = LogFile->Allocate(RecBuf.Length, 0, TAG_ELF_BUF);
        if (pRecBuf == NULL)
        {
            EVTLTRACE1("Cannot allocate heap!\n");
            return STATUS_NO_MEMORY;
        }

        /* Attempt to read the full EVENTLOGRECORD (can wrap) */
        Status = ReadLogBuffer(LogFile,
                               pRecBuf,
                               RecBuf.Length,
                               &ReadLength,
                               &FileOffset,
                               &NextOffset);
        if (!NT_SUCCESS(Status))
        {
            EVTLTRACE1("ReadLogBuffer failed (Status 0x%08lx)\n", Status);
            LogFile->Free(pRecBuf, 0, TAG_ELF_BUF);
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }
        if (ReadLength != RecBuf.Length)
        {
            DPRINT1("Oh oh!!\n");
            LogFile->Free(pRecBuf, 0, TAG_ELF_BUF);
            break;
        }

        // /* If OverWrittenRecords is TRUE and this record has already been read */
        // if (OverWrittenRecords && (pRecBuf->RecordNumber == LogFile->Header.OldestRecordNumber))
        // {
            // LogFile->Free(pRecBuf, 0, TAG_ELF_BUF);
            // break;
        // }

        pRecSize2 = (PULONG)((ULONG_PTR)pRecBuf + RecBuf.Length - 4);

        if (*pRecSize2 != RecBuf.Length)
        {
            EVTLTRACE1("Invalid RecordSizeEnd of record %d (0x%x) in `%wZ'\n",
                    RecordNumber, *pRecSize2, &LogFile->FileName);
            LogFile->Free(pRecBuf, 0, TAG_ELF_BUF);
            break;
        }

        EVTLTRACE("Add new record %d @ offset 0x%x\n", pRecBuf->RecordNumber, FileOffset.QuadPart);

        RecordNumber++;

        if (!ElfpAddOffsetInformation(LogFile,
                                      pRecBuf->RecordNumber,
                                      FileOffset.QuadPart))
        {
            EVTLTRACE1("ElfpAddOffsetInformation() failed!\n");
            LogFile->Free(pRecBuf, 0, TAG_ELF_BUF);
            return STATUS_EVENTLOG_FILE_CORRUPT;
        }

        LogFile->Free(pRecBuf, 0, TAG_ELF_BUF);

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
    if (RecordNumber != 0 && LogFile->Header.OldestRecordNumber == 0)
        LogFile->Header.OldestRecordNumber = 1;

    LogFile->Header.CurrentRecordNumber = RecordNumber + LogFile->Header.OldestRecordNumber;
    if (LogFile->Header.CurrentRecordNumber == 0)
        LogFile->Header.CurrentRecordNumber = 1;

    /* Flush the log if it is not read-only */
    if (!LogFile->ReadOnly)
    {
        Status = ElfFlushFile(LogFile);
        if (!NT_SUCCESS(Status))
        {
            EVTLTRACE1("ElfFlushFile() failed (Status 0x%08lx)\n", Status);
            return STATUS_EVENTLOG_FILE_CORRUPT; // Status;
        }
    }

    return STATUS_SUCCESS;
}


/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
ElfCreateFile(
    IN OUT PEVTLOGFILE LogFile,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN ULONG    FileSize,
    IN ULONG    MaxSize,
    IN ULONG    Retention,
    IN BOOLEAN  CreateNew,
    IN BOOLEAN  ReadOnly,
    IN PELF_ALLOCATE_ROUTINE   Allocate,
    IN PELF_FREE_ROUTINE       Free,
    IN PELF_FILE_SET_SIZE_ROUTINE FileSetSize,
    IN PELF_FILE_WRITE_ROUTINE FileWrite,
    IN PELF_FILE_READ_ROUTINE  FileRead,
    IN PELF_FILE_FLUSH_ROUTINE FileFlush) // What about Seek ??
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(LogFile);

    /* Creating a new log file with the 'ReadOnly' flag set is incompatible */
    if (CreateNew && ReadOnly)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(LogFile, sizeof(*LogFile));

    LogFile->Allocate  = Allocate;
    LogFile->Free      = Free;
    LogFile->FileSetSize = FileSetSize;
    LogFile->FileWrite = FileWrite;
    LogFile->FileRead  = FileRead;
    LogFile->FileFlush = FileFlush;

    /* Copy the log file name if provided (optional) */
    RtlInitEmptyUnicodeString(&LogFile->FileName, NULL, 0);
    if (FileName && FileName->Buffer && FileName->Length &&
        (FileName->Length <= FileName->MaximumLength))
    {
        LogFile->FileName.Buffer = LogFile->Allocate(FileName->Length,
                                                     HEAP_ZERO_MEMORY,
                                                     TAG_ELF);
        if (LogFile->FileName.Buffer)
        {
            LogFile->FileName.MaximumLength = FileName->Length;
            RtlCopyUnicodeString(&LogFile->FileName, FileName);
        }
    }

    LogFile->OffsetInfo = LogFile->Allocate(OFFSET_INFO_INCREMENT * sizeof(EVENT_OFFSET_INFO),
                                            HEAP_ZERO_MEMORY,
                                            TAG_ELF);
    if (LogFile->OffsetInfo == NULL)
    {
        EVTLTRACE1("Cannot allocate heap\n");
        Status = STATUS_NO_MEMORY;
        goto Quit;
    }
    LogFile->OffsetInfoSize = OFFSET_INFO_INCREMENT;
    LogFile->OffsetInfoNext = 0;

    // FIXME: Always use the regitry values for MaxSize,
    // even for existing logs!

    // FIXME: On Windows, EventLog uses the MaxSize setting
    // from the registry itself; the MaxSize from the header
    // is just for information purposes.

    EVTLTRACE("Initializing log file `%wZ'\n", &LogFile->FileName);

    LogFile->ReadOnly = ReadOnly; // !CreateNew && ReadOnly;

    if (CreateNew)
        Status = ElfpInitNewFile(LogFile, FileSize, MaxSize, Retention);
    else
        Status = ElfpInitExistingFile(LogFile, FileSize, /* MaxSize, */ Retention);

Quit:
    if (!NT_SUCCESS(Status))
    {
        if (LogFile->OffsetInfo)
            LogFile->Free(LogFile->OffsetInfo, 0, TAG_ELF);

        if (LogFile->FileName.Buffer)
            LogFile->Free(LogFile->FileName.Buffer, 0, TAG_ELF);
    }

    return Status;
}

NTSTATUS
NTAPI
ElfReCreateFile(
    IN PEVTLOGFILE LogFile)
{
    ASSERT(LogFile);

    return ElfpInitNewFile(LogFile,
                           LogFile->CurrentSize,
                           LogFile->Header.MaxSize,
                           LogFile->Header.Retention);
}

NTSTATUS
NTAPI
ElfBackupFile(
    IN PEVTLOGFILE LogFile,
    IN PEVTLOGFILE BackupLogFile)
{
    NTSTATUS Status;

    LARGE_INTEGER FileOffset;
    SIZE_T ReadLength, WrittenLength;
    PEVENTLOGHEADER Header;
    EVENTLOGRECORD RecBuf;
    EVENTLOGEOF EofRec;
    ULONG i;
    ULONG RecOffset;
    PVOID Buffer = NULL;

    ASSERT(LogFile);

    RtlZeroMemory(BackupLogFile, sizeof(*BackupLogFile));

    BackupLogFile->FileSetSize = LogFile->FileSetSize;
    BackupLogFile->FileWrite = LogFile->FileWrite;
    BackupLogFile->FileFlush = LogFile->FileFlush;

    // BackupLogFile->CurrentSize = LogFile->CurrentSize;

    BackupLogFile->ReadOnly = FALSE;

    /* Initialize the (dirty) log file header */
    Header = &BackupLogFile->Header;
    Header->HeaderSize = sizeof(EVENTLOGHEADER);
    Header->Signature = LOGFILE_SIGNATURE;
    Header->MajorVersion = MAJORVER;
    Header->MinorVersion = MINORVER;
    Header->StartOffset = sizeof(EVENTLOGHEADER);
    Header->EndOffset = sizeof(EVENTLOGHEADER);
    Header->CurrentRecordNumber = 1;
    Header->OldestRecordNumber = 0;
    Header->MaxSize = LogFile->Header.MaxSize;
    Header->Flags = ELF_LOGFILE_HEADER_DIRTY;
    Header->Retention = LogFile->Header.Retention;
    Header->EndHeaderSize = sizeof(EVENTLOGHEADER);

    /* Write the (dirty) log file header */
    FileOffset.QuadPart = 0LL;
    Status = BackupLogFile->FileWrite(BackupLogFile,
                                      &FileOffset,
                                      Header,
                                      sizeof(EVENTLOGHEADER),
                                      &WrittenLength);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE1("Failed to write the log file header (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    for (i = LogFile->Header.OldestRecordNumber; i < LogFile->Header.CurrentRecordNumber; i++)
    {
        RecOffset = ElfpOffsetByNumber(LogFile, i);
        if (RecOffset == 0)
            break;

        /* Read the next EVENTLOGRECORD header at once (it cannot be split) */
        FileOffset.QuadPart = RecOffset;
        Status = LogFile->FileRead(LogFile,
                                   &FileOffset,
                                   &RecBuf,
                                   sizeof(RecBuf),
                                   &ReadLength);
        if (!NT_SUCCESS(Status))
        {
            EVTLTRACE1("FileRead() failed (Status 0x%08lx)\n", Status);
            goto Quit;
        }

        // if (ReadLength != sizeof(RecBuf))
            // break;

        Buffer = LogFile->Allocate(RecBuf.Length, 0, TAG_ELF_BUF);
        if (Buffer == NULL)
        {
            EVTLTRACE1("Allocate() failed!\n");
            goto Quit;
        }

        /* Read the full EVENTLOGRECORD (header + data) with wrapping */
        Status = ReadLogBuffer(LogFile,
                               Buffer,
                               RecBuf.Length,
                               &ReadLength,
                               &FileOffset,
                               NULL);
        if (!NT_SUCCESS(Status))
        {
            EVTLTRACE1("ReadLogBuffer failed (Status 0x%08lx)\n", Status);
            LogFile->Free(Buffer, 0, TAG_ELF_BUF);
            // Status = STATUS_EVENTLOG_FILE_CORRUPT;
            goto Quit;
        }

        /* Write the event record (no wrap for the backup log) */
        Status = BackupLogFile->FileWrite(BackupLogFile,
                                          NULL,
                                          Buffer,
                                          RecBuf.Length,
                                          &WrittenLength);
        if (!NT_SUCCESS(Status))
        {
            EVTLTRACE1("FileWrite() failed (Status 0x%08lx)\n", Status);
            LogFile->Free(Buffer, 0, TAG_ELF_BUF);
            goto Quit;
        }

        /* Update the header information */
        Header->EndOffset += RecBuf.Length;

        /* Free the buffer */
        LogFile->Free(Buffer, 0, TAG_ELF_BUF);
        Buffer = NULL;
    }

// Quit:

    /* Initialize the ELF_EOF_RECORD and write it (no wrap for the backup log) */
    RtlCopyMemory(&EofRec, &EOFRecord, sizeof(EOFRecord));
    EofRec.BeginRecord = Header->StartOffset;
    EofRec.EndRecord   = Header->EndOffset;
    EofRec.CurrentRecordNumber = LogFile->Header.CurrentRecordNumber;
    EofRec.OldestRecordNumber  = LogFile->Header.OldestRecordNumber;

    Status = BackupLogFile->FileWrite(BackupLogFile,
                                      NULL,
                                      &EofRec,
                                      sizeof(EofRec),
                                      &WrittenLength);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE1("FileWrite() failed (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    /* Update the header information */
    Header->CurrentRecordNumber = LogFile->Header.CurrentRecordNumber;
    Header->OldestRecordNumber  = LogFile->Header.OldestRecordNumber;
    Header->MaxSize = ROUND_UP(Header->EndOffset + sizeof(EofRec), sizeof(ULONG));
    Header->Flags = 0; // FIXME?

    /* Flush the log file - Write the (clean) log file header */
    Status = ElfFlushFile(BackupLogFile);

Quit:
    return Status;
}

NTSTATUS
NTAPI
ElfFlushFile(
    IN PEVTLOGFILE LogFile)
{
    NTSTATUS Status;
    LARGE_INTEGER FileOffset;
    SIZE_T WrittenLength;

    ASSERT(LogFile);

    if (LogFile->ReadOnly)
        return STATUS_SUCCESS; // STATUS_ACCESS_DENIED;

    /*
     * NOTE that both the EOF record *AND* the log file header
     * are supposed to be already updated!
     * We just remove the dirty log bit.
     */
    LogFile->Header.Flags &= ~ELF_LOGFILE_HEADER_DIRTY;

    /* Update the log file header */
    FileOffset.QuadPart = 0LL;
    Status = LogFile->FileWrite(LogFile,
                                &FileOffset,
                                &LogFile->Header,
                                sizeof(EVENTLOGHEADER),
                                &WrittenLength);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE1("FileWrite() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Flush the log file */
    Status = LogFile->FileFlush(LogFile, NULL, 0);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE1("FileFlush() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
ElfCloseFile(  // ElfFree
    IN PEVTLOGFILE LogFile)
{
    ASSERT(LogFile);

    /* Flush the log file */
    ElfFlushFile(LogFile);

    /* Free the data */
    LogFile->Free(LogFile->OffsetInfo, 0, TAG_ELF);

    if (LogFile->FileName.Buffer)
        LogFile->Free(LogFile->FileName.Buffer, 0, TAG_ELF);
    RtlInitEmptyUnicodeString(&LogFile->FileName, NULL, 0);
}

NTSTATUS
NTAPI
ElfReadRecord(
    IN  PEVTLOGFILE LogFile,
    IN  ULONG RecordNumber,
    OUT PEVENTLOGRECORD Record,
    IN  SIZE_T  BufSize, // Length
    OUT PSIZE_T BytesRead OPTIONAL,
    OUT PSIZE_T BytesNeeded OPTIONAL)
{
    NTSTATUS Status;
    LARGE_INTEGER FileOffset;
    ULONG RecOffset;
    SIZE_T RecSize;
    SIZE_T ReadLength;

    ASSERT(LogFile);

    if (BytesRead)
        *BytesRead = 0;

    if (BytesNeeded)
        *BytesNeeded = 0;

    /* Retrieve the offset of the event record */
    RecOffset = ElfpOffsetByNumber(LogFile, RecordNumber);
    if (RecOffset == 0)
        return STATUS_NOT_FOUND;

    /* Retrieve its full size */
    FileOffset.QuadPart = RecOffset;
    Status = LogFile->FileRead(LogFile,
                               &FileOffset,
                               &RecSize,
                               sizeof(RecSize),
                               &ReadLength);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE1("FileRead() failed (Status 0x%08lx)\n", Status);
        // Status = STATUS_EVENTLOG_FILE_CORRUPT;
        return Status;
    }

    /* Check whether the buffer is big enough to hold the event record */
    if (BufSize < RecSize)
    {
        if (BytesNeeded)
            *BytesNeeded = RecSize;

        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Read the event record into the buffer */
    FileOffset.QuadPart = RecOffset;
    Status = ReadLogBuffer(LogFile,
                           Record,
                           RecSize,
                           &ReadLength,
                           &FileOffset,
                           NULL);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE1("ReadLogBuffer failed (Status 0x%08lx)\n", Status);
        // Status = STATUS_EVENTLOG_FILE_CORRUPT;
    }

    if (BytesRead)
        *BytesRead = ReadLength;

    return Status;
}

NTSTATUS
NTAPI
ElfWriteRecord(
    IN PEVTLOGFILE LogFile,
    IN PEVENTLOGRECORD Record,
    IN SIZE_T BufSize)
{
    NTSTATUS Status;
    LARGE_INTEGER FileOffset, NextOffset;
    SIZE_T ReadLength, WrittenLength;
    EVENTLOGEOF EofRec;
    EVENTLOGRECORD RecBuf;
    ULONG FreeSpace = 0;
    ULONG UpperBound;
    ULONG RecOffset, WriteOffset;

    ASSERT(LogFile);

    if (LogFile->ReadOnly)
        return STATUS_ACCESS_DENIED;

    // ASSERT(sizeof(*Record) == sizeof(RecBuf));

    if (!Record || BufSize < sizeof(*Record))
        return STATUS_INVALID_PARAMETER;

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
        EVTLTRACE("The event log file has reached maximum size (0x%x), wrapping...\n"
               "UpperBound = 0x%x, WriteOffset = 0x%x, BufSize = 0x%x\n",
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
            RecOffset = ElfpOffsetByNumber(LogFile, OldestRecordNumber);
            if (RecOffset == 0)
            {
                EVTLTRACE1("Record number %d cannot be found, or log file is full and cannot wrap!\n", OldestRecordNumber);
                LogFile->Header.Flags |= ELF_LOGFILE_LOGFULL_WRITTEN;
                return STATUS_LOG_FILE_FULL;
            }

            RtlZeroMemory(&RecBuf, sizeof(RecBuf));

            FileOffset.QuadPart = RecOffset;
            Status = LogFile->FileRead(LogFile,
                                       &FileOffset,
                                       &RecBuf,
                                       sizeof(RecBuf),
                                       &ReadLength);
            if (!NT_SUCCESS(Status))
            {
                EVTLTRACE1("FileRead() failed (Status 0x%08lx)\n", Status);
                // Status = STATUS_EVENTLOG_FILE_CORRUPT;
                return Status;
            }

            if (RecBuf.Reserved != LOGFILE_SIGNATURE)
            {
                EVTLTRACE1("The event log file is corrupted!\n");
                return STATUS_EVENTLOG_FILE_CORRUPT;
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
                EVTLTRACE1("The event log file is full and cannot wrap because of the retention policy.\n");
                LogFile->Header.Flags |= ELF_LOGFILE_LOGFULL_WRITTEN;
                return STATUS_LOG_FILE_FULL;
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

        EVTLTRACE("Record will fit. FreeSpace %d, BufSize %d\n", FreeSpace, BufSize);

        /* The log records are wrapping */
        LogFile->Header.Flags |= ELF_LOGFILE_HEADER_WRAP;


        // FIXME: May lead to corruption if the other subsequent calls fail...

        /*
         * We have validated all the region of events to be discarded,
         * now we can perform their deletion.
         */
        ElfpDeleteOffsetInformation(LogFile, OrgOldestRecordNumber, OldestRecordNumber - 1);
        LogFile->Header.OldestRecordNumber = OldestRecordNumber;
        LogFile->Header.StartOffset = ElfpOffsetByNumber(LogFile, OldestRecordNumber);
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

        EVTLTRACE("MaxSize = 0x%x, StartOffset = 0x%x, WriteOffset = 0x%x, EndOffset = 0x%x, BufSize = 0x%x\n"
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
        EVTLTRACE1("Expanding the log file from %lu to %lu\n",
                LogFile->CurrentSize, LogFile->Header.MaxSize);

        LogFile->CurrentSize = LogFile->Header.MaxSize;
        LogFile->FileSetSize(LogFile, LogFile->CurrentSize, 0);
    }

    /* Since we can write events in the log, clear the log full flag */
    LogFile->Header.Flags &= ~ELF_LOGFILE_LOGFULL_WRITTEN;

    /* Pad the end of the log */
    // if (LogFile->Header.EndOffset + sizeof(RecBuf) > LogFile->Header.MaxSize)
    if (WriteOffset < LogFile->Header.EndOffset)
    {
        /* Pad all the space from LogFile->Header.EndOffset to LogFile->Header.MaxSize */
        WrittenLength = ROUND_DOWN(LogFile->Header.MaxSize - LogFile->Header.EndOffset, sizeof(ULONG));
        RtlFillMemoryUlong(&RecBuf, WrittenLength, 0x00000027);

        FileOffset.QuadPart = LogFile->Header.EndOffset;
        Status = LogFile->FileWrite(LogFile,
                                    &FileOffset,
                                    &RecBuf,
                                    WrittenLength,
                                    &WrittenLength);
        if (!NT_SUCCESS(Status))
        {
            EVTLTRACE1("FileWrite() failed (Status 0x%08lx)\n", Status);
            // return Status;
        }
    }

    /* Write the event record buffer with possible wrap at offset sizeof(EVENTLOGHEADER) */
    FileOffset.QuadPart = WriteOffset;
    Status = WriteLogBuffer(LogFile,
                            Record,
                            BufSize,
                            &WrittenLength,
                            &FileOffset,
                            &NextOffset);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE1("WriteLogBuffer failed (Status 0x%08lx)\n", Status);
        return Status;
    }
    /* FileOffset now contains the offset just after the end of the record buffer */
    FileOffset = NextOffset;

    if (!ElfpAddOffsetInformation(LogFile,
                                  Record->RecordNumber,
                                  WriteOffset))
    {
        return STATUS_NO_MEMORY; // STATUS_EVENTLOG_FILE_CORRUPT;
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
                            &EofRec,
                            sizeof(EofRec),
                            &WrittenLength,
                            &FileOffset,
                            &NextOffset);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE1("WriteLogBuffer failed (Status 0x%08lx)\n", Status);
        return Status;
    }
    FileOffset = NextOffset;

    /* Flush the log file */
    Status = ElfFlushFile(LogFile);
    if (!NT_SUCCESS(Status))
    {
        EVTLTRACE1("ElfFlushFile() failed (Status 0x%08lx)\n", Status);
        return STATUS_EVENTLOG_FILE_CORRUPT; // Status;
    }

    return Status;
}

ULONG
NTAPI
ElfGetOldestRecord(
    IN PEVTLOGFILE LogFile)
{
    ASSERT(LogFile);
    return LogFile->Header.OldestRecordNumber;
}

ULONG
NTAPI
ElfGetCurrentRecord(
    IN PEVTLOGFILE LogFile)
{
    ASSERT(LogFile);
    return LogFile->Header.CurrentRecordNumber;
}

ULONG
NTAPI
ElfGetFlags(
    IN PEVTLOGFILE LogFile)
{
    ASSERT(LogFile);
    return LogFile->Header.Flags;
}

#if DBG
VOID PRINT_HEADER(PEVENTLOGHEADER Header)
{
    ULONG Flags = Header->Flags;

    EVTLTRACE1("PRINT_HEADER(0x%p)\n", Header);

    DbgPrint("HeaderSize    = %lu\n" , Header->HeaderSize);
    DbgPrint("Signature     = 0x%x\n", Header->Signature);
    DbgPrint("MajorVersion  = %lu\n" , Header->MajorVersion);
    DbgPrint("MinorVersion  = %lu\n" , Header->MinorVersion);
    DbgPrint("StartOffset   = 0x%x\n", Header->StartOffset);
    DbgPrint("EndOffset     = 0x%x\n", Header->EndOffset);
    DbgPrint("CurrentRecordNumber = %lu\n", Header->CurrentRecordNumber);
    DbgPrint("OldestRecordNumber  = %lu\n", Header->OldestRecordNumber);
    DbgPrint("MaxSize       = 0x%x\n", Header->MaxSize);
    DbgPrint("Retention     = 0x%x\n", Header->Retention);
    DbgPrint("EndHeaderSize = %lu\n" , Header->EndHeaderSize);
    DbgPrint("Flags: ");
    if (Flags & ELF_LOGFILE_HEADER_DIRTY)
    {
        DbgPrint("ELF_LOGFILE_HEADER_DIRTY");
        Flags &= ~ELF_LOGFILE_HEADER_DIRTY;
    }
    if (Flags) DbgPrint(" | ");
    if (Flags & ELF_LOGFILE_HEADER_WRAP)
    {
        DbgPrint("ELF_LOGFILE_HEADER_WRAP");
        Flags &= ~ELF_LOGFILE_HEADER_WRAP;
    }
    if (Flags) DbgPrint(" | ");
    if (Flags & ELF_LOGFILE_LOGFULL_WRITTEN)
    {
        DbgPrint("ELF_LOGFILE_LOGFULL_WRITTEN");
        Flags &= ~ELF_LOGFILE_LOGFULL_WRITTEN;
    }
    if (Flags) DbgPrint(" | ");
    if (Flags & ELF_LOGFILE_ARCHIVE_SET)
    {
        DbgPrint("ELF_LOGFILE_ARCHIVE_SET");
        Flags &= ~ELF_LOGFILE_ARCHIVE_SET;
    }
    if (Flags) DbgPrint(" | 0x%x", Flags);
    DbgPrint("\n");
}
#endif
