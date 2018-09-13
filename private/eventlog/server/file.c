/*

Copyright (c) 1990  Microsoft Corporation

Module Name:

    FILE.C

Abstract:

    This file contains the routines that deal with file-related operations.

Author:

    Rajen Shah  (rajens)    07-Aug-1991

Revision History:

    29-Aug-1994     Danl
        We no longer grow log files in place.  So there is no need to
        reserve the MaxConfigSize block of memory.

--*/

//
// INCLUDES
//

#include <eventp.h>
#include <alertmsg.h>  // ALERT_ELF manifests

//
// Macros
//

#define IS_EOF(Ptr, Size) \
    (Ptr)->Length == ELFEOFRECORDSIZE && \
        RtlCompareMemory (Ptr, &EOFRecord, Size) == Size

#ifdef CORRUPTED


BOOLEAN
VerifyLogIntegrity(
    PLOGFILE pLogFile
    )
/*++

Routine Description:

    This routine walks the log file to verify that it isn't corrupt


Arguments:

    A pointer to the LOGFILE structure for the log to validate.

Return Value:

    TRUE if log OK
    FALSE if it is corrupt

Note:


--*/
{

    PEVENTLOGRECORD pEventLogRecord;
    PVOID PhysicalStart;
    PVOID PhysicalEOF;
    PVOID BeginRecord;
    PVOID EndRecord;

    pEventLogRecord =
        (PEVENTLOGRECORD)((PBYTE) pLogFile->BaseAddress + pLogFile->BeginRecord);
    PhysicalStart =
        (PVOID) ((PBYTE) pLogFile->BaseAddress + FILEHEADERBUFSIZE);
    PhysicalEOF =
        (PVOID) ((PBYTE) pLogFile->BaseAddress + pLogFile->ViewSize);
    BeginRecord = (PVOID)((PBYTE) pLogFile->BaseAddress + pLogFile->BeginRecord);
    EndRecord = (PVOID)((PBYTE) pLogFile->BaseAddress + pLogFile->EndRecord);

    while(pEventLogRecord->Length != ELFEOFRECORDSIZE) {

        pEventLogRecord = (PEVENTLOGRECORD) NextRecordPosition (
            EVENTLOG_FORWARDS_READ,
            (PVOID) pEventLogRecord,
            pEventLogRecord->Length,
            BeginRecord,
            EndRecord,
            PhysicalEOF,
            PhysicalStart
            );

        if (!pEventLogRecord || pEventLogRecord->Length == 0) {

            ElfDbgPrintNC(("[ELF] The %ws logfile is corrupt\n",
                pLogFile->LogModuleName->Buffer));
            return(FALSE);
        }
    }

    return(TRUE);

}

#endif // CORRUPTED


NTSTATUS
FlushLogFile (
    PLOGFILE    pLogFile
    )

/*++

Routine Description:

    This routine flushes the file specified. It updates the file header,
    and then flushes the virtual memory which causes the data to get
    written to disk.

Arguments:

    pLogFile points to the log file structure.

Return Value:

    NONE

Note:


--*/
{
    NTSTATUS    Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PVOID       BaseAddress;
    SIZE_T      RegionSize;
    PELF_LOGFILE_HEADER pLogFileHeader;

    //
    // If the dirty bit is set, update the file header before flushing it.
    //
    if (pLogFile->Flags & ELF_LOGFILE_HEADER_DIRTY) {

        pLogFileHeader = (PELF_LOGFILE_HEADER) pLogFile->BaseAddress;

        pLogFile->Flags &= ~ELF_LOGFILE_HEADER_DIRTY; // Remove dirty bit
        pLogFileHeader->Flags = pLogFile->Flags;

        pLogFileHeader->StartOffset = pLogFile->BeginRecord;
        pLogFileHeader->EndOffset   = pLogFile->EndRecord;
        pLogFileHeader->CurrentRecordNumber = pLogFile->CurrentRecordNumber;
        pLogFileHeader->OldestRecordNumber = pLogFile->OldestRecordNumber;
    }

    //
    // Flush the memory in the section that is mapped to the file.
    //
    BaseAddress = pLogFile->BaseAddress;
    RegionSize = pLogFile->ViewSize;

    Status = NtFlushVirtualMemory(
                    NtCurrentProcess(),
                    &BaseAddress,
                    &RegionSize,
                    &IoStatusBlock
                    );

    return (Status);

}



NTSTATUS
ElfpFlushFiles (
    )

/*++

Routine Description:

    This routine flushes all the log files and forces them on disk.
    It is usually called in preparation for a shutdown or a pause.

Arguments:

    NONE

Return Value:

    NONE

Note:


--*/

{

    PLOGFILE    pLogFile;
    NTSTATUS    Status = STATUS_SUCCESS;

    //
    // Make sure that there's at least one file to flush
    //

    if (IsListEmpty (&LogFilesHead) ) {
        return(STATUS_SUCCESS);
    }

    pLogFile
        = (PLOGFILE)
                CONTAINING_RECORD(LogFilesHead.Flink, LOGFILE, FileList);

    //
    // Go through this loop at least once. This ensures that the termination
    // condition will work correctly.
    //
    do {

        Status = FlushLogFile (pLogFile);

        pLogFile =                      // Get next one
            (PLOGFILE)
                CONTAINING_RECORD(pLogFile->FileList.Flink, LOGFILE, FileList);

    } while (   (pLogFile->FileList.Flink != LogFilesHead.Flink)
             && (NT_SUCCESS(Status)) ) ;

    return (Status);
}




NTSTATUS
ElfpCloseLogFile (
    PLOGFILE    pLogFile,
    DWORD       Flags
    )

/*++

Routine Description:

    This routine undoes whatever is done in ElfOpenLogFile.

Arguments:

    pLogFile points to the log file structure.

Return Value:

    NTSTATUS.

Note:


--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    PELF_LOGFILE_HEADER pLogFileHeader;
    PVOID BaseAddress;
    ULONG Size;

    ElfDbgPrint(("[ELF] Closing and unmapping log file:  %ws\n",
        pLogFile->LogFileName->Buffer));

#ifdef CORRUPTED

    //
    // Just for debugging a log corruption problem
    //

    if (!VerifyLogIntegrity(pLogFile)) {
       ElfDbgPrintNC(("[ELF] Integrity check failed in ElfpCloseLogFile\n"));
    }

#endif // CORRUPTED

    //
    // If the dirty bit is set, update the file header before closing it.
    // Check to be sure it's not a backup file that just had the dirty
    // bit set when it was copied.
    //

    if (pLogFile->Flags & ELF_LOGFILE_HEADER_DIRTY &&
        !(Flags & ELF_LOG_CLOSE_BACKUP)) {
        pLogFileHeader = (PELF_LOGFILE_HEADER) pLogFile->BaseAddress;
        pLogFileHeader->StartOffset = pLogFile->BeginRecord;
        pLogFileHeader->EndOffset   = pLogFile->EndRecord;
        pLogFileHeader->CurrentRecordNumber = pLogFile->CurrentRecordNumber;
        pLogFileHeader->OldestRecordNumber = pLogFile->OldestRecordNumber;
        pLogFile->Flags &= ~(ELF_LOGFILE_HEADER_DIRTY |
                                ELF_LOGFILE_ARCHIVE_SET);   // Remove dirty &
                                                            // archive bits
        pLogFileHeader->Flags = pLogFile->Flags;
    }

    //
    // Decrement the reference count, and if it goes to zero, unmap the file
    // and close the handle. Also force the close if fForceClosed is TRUE.
    //

    if ((--pLogFile->RefCount == 0) || Flags & ELF_LOG_CLOSE_FORCE) {

        //
        // Last user has gone.
        // Close all the views and the file and section handles.  Free up
        // any extra memory we had reserved, and unlink any structures
        //

        if (pLogFile->BaseAddress)     // Unmap it if it was allocated
            NtUnmapViewOfSection (
                NtCurrentProcess(),
                pLogFile->BaseAddress
                );

        if (pLogFile->SectionHandle)
            NtClose ( pLogFile->SectionHandle );

        if (pLogFile->FileHandle)
            NtClose ( pLogFile->FileHandle );
    }

    return (Status);

} // ElfpCloseLogFile


NTSTATUS
RevalidateLogHeader (
    PELF_LOGFILE_HEADER pLogFileHeader,
    PLOGFILE pLogFile
    )
/*++

Routine Description:

    This routine is called if we encounter a "dirty" log file. The
    routine walks through the file until it finds a signature for a valid log
    record.  It then walks forward thru the file until it finds the EOF
    record, or an invalid record.  Then it walks backwards from the first
    record it found until it finds the EOF record from the other direction.
    It then rebuilds the header and writes it back to the log.  If it can't
    find any valid records, it rebuilds the header to reflect an empty log
    file.  If it finds a trashed file, it writes 256 bytes of the log out in
    an event to the system log.

Arguments:

    pLogFileHeader points to the header of the log file.
    pLogFile points to the log file structure.

Return Value:

    NTSTATUS.

Note:

    This is an expensive routine since it scans the entire file.

    It assumes that the records are on DWORD boundaries.

--*/
{
    PVOID Start, End;
    PDWORD pSignature;
    PEVENTLOGRECORD pEvent;
    PEVENTLOGRECORD FirstRecord;
    PEVENTLOGRECORD FirstPhysicalRecord;
    PEVENTLOGRECORD pLastGoodRecord = NULL;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER ByteOffset;
    SIZE_T Size;

    ElfDbgPrint(("[ELF] Log file had dirty bit set, revalidating\n"));

    try {

        //
        // Physical start and end of log file (skipping header)
        //

        Start = (PVOID) ((PBYTE)pLogFile->BaseAddress + FILEHEADERBUFSIZE);
        End = (PVOID) ((PBYTE)pLogFile->BaseAddress +
            pLogFile->ActualMaxFileSize);

        //
        // First see if the log has wrapped.  The EOFRECORDSIZE is for the one
        // case where the EOF record wraps so that it's final length just replaces
        // the next records starting length
        //

        pEvent = (PEVENTLOGRECORD) Start;

        if (pEvent->Reserved != ELF_LOG_FILE_SIGNATURE
             ||
             pEvent->RecordNumber != 1
             ||
             pEvent->Length == ELFEOFRECORDSIZE) {

            //
            // Log has already wrapped, go looking for the first valid record
            //

            for (pSignature = (PDWORD) Start;
                    (PVOID) pSignature < End;
                    pSignature++)
            {
                if (*pSignature == ELF_LOG_FILE_SIGNATURE) {

                    //
                    // Make sure it's really a record
                    //

                    pEvent = CONTAINING_RECORD(pSignature, EVENTLOGRECORD, Reserved);

                    if (!ValidFilePos(pEvent, Start, End, End, pLogFileHeader, TRUE))
                    {
                        //
                        // Nope, not really, keep trying
                        //

                        continue;
                    }

                    //
                    // This is a valid record, Remember this so you can use
                    // it later
                    //

                    FirstPhysicalRecord = pEvent;

                    //
                    // Walk backwards from here (wrapping if necessary) until
                    // you hit the EOF record or an invalid record.
                    //

                    while (pEvent
                           &&
                           ValidFilePos(pEvent, Start, End, End, pLogFileHeader, TRUE))
                    {
                        //
                        // See if it's the EOF record
                        //

                        if (IS_EOF(pEvent,
                                   min (ELFEOFUNIQUEPART,
                                   (ULONG_PTR) ((PBYTE) End - (PBYTE) pEvent))))
                        {
                            break;
                        }

                        pLastGoodRecord = pEvent;
                        pEvent = NextRecordPosition (
                                     EVENTLOG_SEQUENTIAL_READ |
                                         EVENTLOG_BACKWARDS_READ,
                                     pEvent,
                                     pEvent->Length,
                                     0,
                                     0,
                                     End,
                                     Start);

                        //
                        // Make sure we're not in an infinite loop
                        //

                        if (pEvent == FirstPhysicalRecord) {
                            return(STATUS_UNSUCCESSFUL);
                        }
                    }

                    //
                    // Found the first record, now go look for the last
                    //

                    FirstRecord = pLastGoodRecord;
                    break;
                }
            }

            if (pSignature == End || pLastGoodRecord == NULL)
            {
                //
                // Either there were no valid records in the file or
                // the only valid record was the EOF record (which
                // means the log is trashed anyhow).  Give up
                // and we'll set it to an empty log file.
                //

                return(STATUS_UNSUCCESSFUL);
            }
        }
        else {

            //
            // We haven't wrapped yet
            //

            FirstPhysicalRecord = FirstRecord = Start;
        }


        //
        // Now read forward looking for the last good record
        //

        pEvent = FirstPhysicalRecord;

        while (pEvent && ValidFilePos(pEvent, Start, End, End, pLogFileHeader, TRUE)) {

            //
            // See if it's the EOF record
            //

            if (IS_EOF(pEvent, min (ELFEOFUNIQUEPART,
                (ULONG_PTR) ((PBYTE) End - (PBYTE) pEvent)))) {

                break;
            }

            pLastGoodRecord = pEvent;
            pEvent = NextRecordPosition (
                EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                pEvent,
                pEvent->Length,
                0,
                0,
                End,
                Start);

            //
            // Make sure we're not in an infinite loop
            //

            if (pEvent == FirstPhysicalRecord) {
                return(STATUS_UNSUCCESSFUL);
            }
        }

        //
        // Now we know the first record (FirstRecord) and the last record
        // (pLastGoodRecord) so we can create the header an EOF record and
        // write them out (EOF record gets written at pEvent)
        //
        // First the EOF record
        //

        EOFRecord.BeginRecord = (ULONG) ((PBYTE) FirstRecord - (PBYTE) pLogFileHeader);
        EOFRecord.EndRecord = (ULONG) ((PBYTE) pEvent - (PBYTE) pLogFileHeader);
        EOFRecord.CurrentRecordNumber =
            pLastGoodRecord->RecordNumber + 1;
        EOFRecord.OldestRecordNumber = FirstRecord->RecordNumber;

        ByteOffset = RtlConvertUlongToLargeInteger (
            (ULONG) ((PBYTE) pEvent - (PBYTE) pLogFileHeader));

        //
        // If the EOF record was wrapped, we can't write out the entire record at
        // once.  Instead, we'll write out as much as we can and then write the
        // rest out at the beginning of the log
        //

        Size = min((PBYTE) End - (PBYTE) pEvent, ELFEOFRECORDSIZE);

        Status = NtWriteFile(
                    pLogFile->FileHandle,   // Filehandle
                    NULL,                   // Event
                    NULL,                   // APC routine
                    NULL,                   // APC context
                    &IoStatusBlock,         // IO_STATUS_BLOCK
                    &EOFRecord,             // Buffer
                    (ULONG)Size,            // Length
                    &ByteOffset,            // Byteoffset
                    NULL);                  // Key

        if (!NT_SUCCESS(Status)) {

            ElfDbgPrint(("[ELF]: Log file header write failed 0x%lx\n",
                Status));
            return (Status);
        }

        if (Size != ELFEOFRECORDSIZE) {

            PBYTE   pBuff;

            pBuff = (PBYTE)&EOFRecord + Size;
            Size = ELFEOFRECORDSIZE - Size;
            ByteOffset = RtlConvertUlongToLargeInteger(FILEHEADERBUFSIZE);

            //
            // Make absolutely sure we have enough room to write the remainder of
            // the EOF record.  Note that this should always be the case since the
            // record was wrapped around to begin with.  To do this, make sure
            // that the number of bytes we're writing after the header is <= the
            // offset of the first record from the end of the header
            //

            ASSERT(Size <= (ULONG)((PBYTE) FirstRecord
                            - (PBYTE) pLogFileHeader
                            - FILEHEADERBUFSIZE));

            Status = NtWriteFile(
                        pLogFile->FileHandle,   // Filehandle
                        NULL,                   // Event
                        NULL,                   // APC routine
                        NULL,                   // APC context
                        &IoStatusBlock,         // IO_STATUS_BLOCK
                        pBuff,                  // Buffer
                        (ULONG)Size,            // Length
                        &ByteOffset,            // Byteoffset
                        NULL);                  // Key

            if (!NT_SUCCESS(Status)) {

                ElfDbgPrint(("[ELF]: Write of remainder of log file header failed 0x%lx\n",
                             Status));

                return (Status);
            }
        }

        //
        // Now the header
        //

        pLogFileHeader->StartOffset = (ULONG) ((PBYTE) FirstRecord- (PBYTE) pLogFileHeader);
        pLogFileHeader->EndOffset = (ULONG) ((PBYTE) pEvent- (PBYTE) pLogFileHeader);
        pLogFileHeader->CurrentRecordNumber =
            pLastGoodRecord->RecordNumber + 1;
        pLogFileHeader->OldestRecordNumber = FirstRecord->RecordNumber;
        pLogFileHeader->HeaderSize = pLogFileHeader->EndHeaderSize = FILEHEADERBUFSIZE;
        pLogFileHeader->Signature = ELF_LOG_FILE_SIGNATURE;
        pLogFileHeader->Flags = 0;

        if (pLogFileHeader->StartOffset != FILEHEADERBUFSIZE)
            pLogFileHeader->Flags |= ELF_LOGFILE_HEADER_WRAP;

        pLogFileHeader->MaxSize = pLogFile->ActualMaxFileSize;
        pLogFileHeader->Retention = pLogFile->Retention;
        pLogFileHeader->MajorVersion = ELF_VERSION_MAJOR;
        pLogFileHeader->MinorVersion = ELF_VERSION_MINOR;

        //
        // Now flush this to disk to commit it
        //

        Start = pLogFile->BaseAddress;
        Size = FILEHEADERBUFSIZE;

        Status = NtFlushVirtualMemory(
                        NtCurrentProcess(),
                        &Start,
                        &Size,
                        &IoStatusBlock
                        );

    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return(STATUS_UNSUCCESSFUL);
    }

    return (Status);

}


NTSTATUS
ElfOpenLogFile (
    PLOGFILE    pLogFile,
    ELF_LOG_TYPE LogType
    )

/*++

Routine Description:

    Open the log file, create it if it does not exist.
    Create a section and map a view into the log file.
    Write out the header to the file, if it is newly created.
    If "dirty", update the "start" and "end" pointers by scanning
    the file. Set AUTOWRAP if the "start" does not start right after
    the file header.

Arguments:

    pLogFile points to the log file structure, with the relevant data
             filled in.

    CreateOptions are the options to be passed to NtCreateFile which
             indicate whether to open an existing file, or to create it
             if it does not exist.

Return Value:

    NTSTATUS.

Note:

    It is up to the caller to set the RefCount in the Logfile structure.

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER MaximumSizeOfSection;
    LARGE_INTEGER ByteOffset;
    PELF_LOGFILE_HEADER pLogFileHeader;
    FILE_STANDARD_INFORMATION FileStandardInfo;
    ULONG IoStatusInformation;
    ULONG FileDesiredAccess;
    ULONG SectionDesiredAccess;
    ULONG SectionPageProtection;
    ULONG CreateOptions;
    ULONG CreateDisposition;
    SIZE_T ViewSize;

    //
    // File header in a new file has the "Start" and "End" pointers the
    // same since there are no records in the file.
    //

    static ELF_LOGFILE_HEADER FileHeaderBuf = {FILEHEADERBUFSIZE, // Size
                                         ELF_LOG_FILE_SIGNATURE,
                                         ELF_VERSION_MAJOR,
                                         ELF_VERSION_MINOR,
                                         FILEHEADERBUFSIZE, // Start offset
                                         FILEHEADERBUFSIZE, // End offset
                                         1,                 // Next record #
                                         0,                 // Oldest record #
                                         0,                 // Maxsize
                                         0,                 // Flags
                                         0,                 // Retention
                                         FILEHEADERBUFSIZE  // Size
                                         };

    //
    // Set the file open and section create options based on the type of log
    // file this is.
    //

    switch (LogType) {

        case ElfNormalLog:
            CreateDisposition = FILE_OPEN_IF;
            FileDesiredAccess = GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE;
            SectionDesiredAccess = SECTION_MAP_READ | SECTION_MAP_WRITE |
                    SECTION_QUERY | SECTION_EXTEND_SIZE;
            SectionPageProtection = PAGE_READWRITE;
            CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT;
            break;

        case ElfSecurityLog:
            CreateDisposition = FILE_OPEN_IF;
            FileDesiredAccess = GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE;
            SectionDesiredAccess = SECTION_MAP_READ | SECTION_MAP_WRITE |
                    SECTION_QUERY | SECTION_EXTEND_SIZE;
            SectionPageProtection = PAGE_READWRITE;
            CreateOptions = FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT;
            break;

        case ElfBackupLog:
            CreateDisposition = FILE_OPEN;
            FileDesiredAccess = GENERIC_READ | SYNCHRONIZE;
            SectionDesiredAccess = SECTION_MAP_READ | SECTION_QUERY;
            SectionPageProtection = PAGE_READONLY;
            CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT;
            break;

    }

    ElfDbgPrint (("[ELF] Opening and mapping %ws\n",
        pLogFile->LogFileName->Buffer));

    if (pLogFile->FileHandle != NULL) {

        //
        // The log file is already in use. Do not reopen or remap it.
        //

        ElfDbgPrint(("[ELF] Log file already in use by another module\n"));

    } else {

        //
        // Initialize the logfile structure so that it is easier to clean
        // up.
        //

        pLogFile->ActualMaxFileSize = ELF_DEFAULT_LOG_SIZE;
        pLogFile->Flags = 0;
        pLogFile->BaseAddress = NULL;
        pLogFile->SectionHandle = NULL;


        //
        // Set up the object attributes structure for the Log File
        //

        InitializeObjectAttributes(
                        &ObjectAttributes,
                        pLogFile->LogFileName,
                        OBJ_CASE_INSENSITIVE,
                        NULL,
                        NULL
                        );

        //
        // Open the Log File. Create it if it does not exist and it's not
        // being opened as a backup file.  If creating, create a file of
        // the maximum size configured.
        //

        MaximumSizeOfSection =
                RtlConvertUlongToLargeInteger (ELF_DEFAULT_LOG_SIZE);

        Status = NtCreateFile(
                    &pLogFile->FileHandle,
                    FileDesiredAccess,
                    &ObjectAttributes,
                    &IoStatusBlock,
                    &MaximumSizeOfSection,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ,
                    CreateDisposition,
                    CreateOptions,
                    NULL,
                    0);

        if (!NT_SUCCESS(Status)) {
            ElfDbgPrint(("[ELF] Log File Open Failed 0x%lx\n", Status));
            goto cleanup;
        }

        //
        // If the file already existed, get its size and use that as the
        // actual size of the file.
        //

        IoStatusInformation = (ULONG)IoStatusBlock.Information;            // Save it away

        if (!( IoStatusInformation & FILE_CREATED )) {
            ElfDbgPrint (("[Elf] Log file exists.\n"));

            Status = NtQueryInformationFile (
                        pLogFile->FileHandle,
                        &IoStatusBlock,
                        &FileStandardInfo,
                        sizeof (FileStandardInfo),
                        FileStandardInformation
                        );

            if (!NT_SUCCESS(Status)) {
                ElfDbgPrint(("[ELF] QueryInformation failed 0x%lx\n", Status));
                goto cleanup;
            } else {


                ElfDbgPrint(("[ELF] Use existing log file size: 0x%lx:%lx\n",
                     FileStandardInfo.EndOfFile.HighPart,
                     FileStandardInfo.EndOfFile.LowPart
                    ));

                MaximumSizeOfSection.LowPart =
                            FileStandardInfo.EndOfFile.LowPart;
                MaximumSizeOfSection.HighPart =
                            FileStandardInfo.EndOfFile.HighPart;

                //
                // Make sure that the high DWORD of the file size is ZERO.
                //

                ASSERT (MaximumSizeOfSection.HighPart == 0);

                //
                // If the filesize if 0, set it to the minimum size
                //

                if (MaximumSizeOfSection.LowPart == 0) {
                    MaximumSizeOfSection.LowPart = ELF_DEFAULT_LOG_SIZE;
                }

                //
                // Set actual size of file
                //

                pLogFile->ActualMaxFileSize =
                        MaximumSizeOfSection.LowPart;

                //
                // If the size of the log file is reduced, a clear must
                // happen for this to take effect
                //

                if (pLogFile->ActualMaxFileSize > pLogFile->ConfigMaxFileSize) {
                    pLogFile->ConfigMaxFileSize = pLogFile->ActualMaxFileSize;
                }

            }
        }

        //
        // Create a section mapped to the Log File just opened
        //

        Status = NtCreateSection(
                    &pLogFile->SectionHandle,
                    SectionDesiredAccess,
                    NULL,
                    &MaximumSizeOfSection,
                    SectionPageProtection,
                    SEC_COMMIT,
                    pLogFile->FileHandle
                    );


        if (!NT_SUCCESS(Status)) {

            ElfDbgPrintNC(("[ELF] Log Mem Section Create Failed 0x%lx\n",
                Status));
            goto cleanup;
        }

        //
        // Map a view of the Section into the eventlog address space
        //

        ViewSize = 0;

        Status = NtMapViewOfSection(
                        pLogFile->SectionHandle,
                        NtCurrentProcess(),
                        &pLogFile->BaseAddress,
                        0,
                        0,
                        NULL,
                        &ViewSize,
                        ViewUnmap,
                        0,
                        SectionPageProtection);

        pLogFile->ViewSize = (ULONG)ViewSize;
        if (!NT_SUCCESS(Status)) {

            ElfDbgPrintNC(("[ELF] Log Mem Sect Map View failed 0x%lx\n",
                Status));
            goto cleanup;
        }

        //
        // If the file was just created, write out the file header.
        //

        if ( IoStatusInformation & FILE_CREATED ) {
            ElfDbgPrint(("[ELF] File was created\n"));
JustCreated:
            FileHeaderBuf.MaxSize = pLogFile->ActualMaxFileSize;
            FileHeaderBuf.Flags   = 0;
            FileHeaderBuf.Retention     = pLogFile->Retention;

            //
            // Copy the header into the file
            //

            ByteOffset = RtlConvertUlongToLargeInteger ( 0 );
            Status = NtWriteFile(
                        pLogFile->FileHandle,   // Filehandle
                        NULL,                   // Event
                        NULL,                   // APC routine
                        NULL,                   // APC context
                        &IoStatusBlock,         // IO_STATUS_BLOCK
                        &FileHeaderBuf,         // Buffer
                        FILEHEADERBUFSIZE,      // Length
                        &ByteOffset,            // Byteoffset
                        NULL);                  // Key


            if (!NT_SUCCESS(Status)) {

                ElfDbgPrint(("[ELF]: Log file header write failed 0x%lx\n",
                    Status));
                goto cleanup;
            }

            //
            // Copy the "EOF" record right after the header
            //

            ByteOffset = RtlConvertUlongToLargeInteger ( FILEHEADERBUFSIZE );
            Status = NtWriteFile(
                        pLogFile->FileHandle,   // Filehandle
                        NULL,                   // Event
                        NULL,                   // APC routine
                        NULL,                   // APC context
                        &IoStatusBlock,         // IO_STATUS_BLOCK
                        &EOFRecord,             // Buffer
                        ELFEOFRECORDSIZE,       // Length
                        &ByteOffset,            // Byteoffset
                        NULL);                  // Key

            if (!NT_SUCCESS(Status)) {

                ElfDbgPrint(("[ELF]: Log file header write failed 0x%lx\n",
                    Status));
                goto cleanup;
            }
        }

        //
        // Check to ensure that this is a valid log file. We look at the
        // size of the header and the signature to see if they match, as
        // well as checking the version number.
        //

        pLogFileHeader = (PELF_LOGFILE_HEADER)(pLogFile->BaseAddress);

        if (  (pLogFileHeader->HeaderSize != FILEHEADERBUFSIZE)
            ||(pLogFileHeader->EndHeaderSize != FILEHEADERBUFSIZE)
            ||(pLogFileHeader->Signature  != ELF_LOG_FILE_SIGNATURE)
            ||(pLogFileHeader->MajorVersion != ELF_VERSION_MAJOR)
            ||(pLogFileHeader->MinorVersion != ELF_VERSION_MINOR)) {

                //
                // This file is corrupt, reset it to an empty log, unless
                // it's being opened as a backup file, if it is, fail the
                // open
                //

                ElfDbgPrint(("[ELF] Invalid file header found\n"));

                if (LogType == ElfBackupLog) {

                   Status = STATUS_EVENTLOG_FILE_CORRUPT;
                   goto cleanup;
                }
                else {
                    ElfpCreateQueuedAlert(ALERT_ELF_LogFileCorrupt, 1,
                        &pLogFile->LogModuleName->Buffer);
                    //
                    // Treat it like it was just created
                    //

                    goto JustCreated;
                }
        }
        else {

            //
            // If the "dirty" bit is set in the file header, then we need to
            // revalidate the BeginRecord and EndRecord fields since we did not
            // get a chance to write them out before the system was rebooted.
            // If the dirty bit is set and it's a backup file, just fail the
            // open.
            //

            if (pLogFileHeader->Flags & ELF_LOGFILE_HEADER_DIRTY) {

                if (LogType == ElfBackupLog) {

                   Status = STATUS_EVENTLOG_FILE_CORRUPT;
                   goto cleanup;
                }
                else {
                   Status = RevalidateLogHeader (pLogFileHeader, pLogFile);
                }
            }

            if (NT_SUCCESS(Status)) {

                //
                // Set the beginning and end record positions in our
                // data structure as well as the wrap flag if appropriate.
                //

                pLogFile->EndRecord = pLogFileHeader->EndOffset;
                pLogFile->BeginRecord = pLogFileHeader->StartOffset;
                if (pLogFileHeader->Flags & ELF_LOGFILE_HEADER_WRAP) {
                    pLogFile->Flags |= ELF_LOGFILE_HEADER_WRAP;
                }

                ElfDbgPrint(("[ELF] BeginRecord: 0x%lx     EndRecord: 0x%lx \n",
                  pLogFile->BeginRecord, pLogFile->EndRecord));
            }
            else {

                //
                // Couldn't validate the file, treat it like it was just
                // created (turn it into an empty file)
                //

                goto JustCreated;
            }

#ifdef CORRUPTED

            //
            // Just for debugging a log corruption problem
            //

            if (!VerifyLogIntegrity(pLogFile)) {
               ElfDbgPrintNC(("[ELF] Integrity check failed in ElfOpenLogFile\n"));
            }
#endif // CORRUPTED

        }

        //
        // Fill in the first and last record number values in the LogFile
        // data structure.
        //
        //SS:save the record number of the first record in this session
        //so that if the cluster service starts after the eventlog service
        //it will be able to forward the pending records for replication
        //when the cluster service registers
        pLogFile->SessionStartRecordNumber = pLogFileHeader->CurrentRecordNumber;
        pLogFile->CurrentRecordNumber = pLogFileHeader->CurrentRecordNumber;
        pLogFile->OldestRecordNumber = pLogFileHeader->OldestRecordNumber;

    }

    return (Status);

cleanup:

    // Clean up anything that got allocated

    if (pLogFile->ViewSize) {
        NtUnmapViewOfSection(NtCurrentProcess(), pLogFile->BaseAddress);
    }

    if (pLogFile->SectionHandle) {
        NtClose(pLogFile->SectionHandle);
    }

    if (pLogFile->FileHandle) {
        NtClose (pLogFile->FileHandle);
    }

    return (Status);

}
