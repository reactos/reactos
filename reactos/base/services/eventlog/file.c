/*
 * PROJECT:         ReactOS EventLog Service
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/services/eventlog/file.c
 * PURPOSE:         Event log file support wrappers
 * COPYRIGHT:       Copyright 2005 Saveliy Tretiakov
 *                  Michael Martin
 *                  Hermes Belusca-Maito
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


/* FUNCTIONS *****************************************************************/

// PELF_ALLOCATE_ROUTINE
static
PVOID NTAPI
LogfpAlloc(IN SIZE_T Size,
           IN ULONG Flags,
           IN ULONG Tag)
{
    UNREFERENCED_PARAMETER(Tag);
    return RtlAllocateHeap(GetProcessHeap(), Flags, Size);
}

// PELF_FREE_ROUTINE
static
VOID NTAPI
LogfpFree(IN PVOID Ptr,
          IN ULONG Flags)
{
    RtlFreeHeap(GetProcessHeap(), Flags, Ptr);
}

// PELF_FILE_READ_ROUTINE
static
NTSTATUS NTAPI
LogfpReadFile(IN  PEVTLOGFILE LogFile,
              IN  PLARGE_INTEGER FileOffset,
              OUT PVOID   Buffer,
              IN  SIZE_T  Length,
              OUT PSIZE_T ReadLength OPTIONAL)
{
    NTSTATUS Status;
    PLOGFILE pLogFile = (PLOGFILE)LogFile;
    IO_STATUS_BLOCK IoStatusBlock;

    if (ReadLength)
        *ReadLength = 0;

    Status = NtReadFile(pLogFile->FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        Buffer,
                        Length,
                        FileOffset,
                        NULL);

    if (ReadLength)
        *ReadLength = IoStatusBlock.Information;

    return Status;
}

// PELF_FILE_WRITE_ROUTINE
static
NTSTATUS NTAPI
LogfpWriteFile(IN  PEVTLOGFILE LogFile,
               IN  PLARGE_INTEGER FileOffset,
               IN  PVOID   Buffer,
               IN  SIZE_T  Length,
               OUT PSIZE_T WrittenLength OPTIONAL)
{
    NTSTATUS Status;
    PLOGFILE pLogFile = (PLOGFILE)LogFile;
    IO_STATUS_BLOCK IoStatusBlock;

    if (WrittenLength)
        *WrittenLength = 0;

    Status = NtWriteFile(pLogFile->FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         Buffer,
                         Length,
                         FileOffset,
                         NULL);

    if (WrittenLength)
        *WrittenLength = IoStatusBlock.Information;

    return Status;
}

// PELF_FILE_SET_SIZE_ROUTINE
static
NTSTATUS NTAPI
LogfpSetFileSize(IN PEVTLOGFILE LogFile,
                 IN ULONG FileSize,    // SIZE_T
                 IN ULONG OldFileSize) // SIZE_T
{
    NTSTATUS Status;
    PLOGFILE pLogFile = (PLOGFILE)LogFile;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_END_OF_FILE_INFORMATION FileEofInfo;
    FILE_ALLOCATION_INFORMATION FileAllocInfo;

    UNREFERENCED_PARAMETER(OldFileSize);

    // FIXME: Should we round up FileSize ??

    FileEofInfo.EndOfFile.QuadPart = FileSize;
    Status = NtSetInformationFile(pLogFile->FileHandle,
                                  &IoStatusBlock,
                                  &FileEofInfo,
                                  sizeof(FileEofInfo),
                                  FileEndOfFileInformation);
    if (!NT_SUCCESS(Status))
        return Status;

    FileAllocInfo.AllocationSize.QuadPart = FileSize;
    Status = NtSetInformationFile(pLogFile->FileHandle,
                                  &IoStatusBlock,
                                  &FileAllocInfo,
                                  sizeof(FileAllocInfo),
                                  FileAllocationInformation);

    return Status;
}

// PELF_FILE_FLUSH_ROUTINE
static
NTSTATUS NTAPI
LogfpFlushFile(IN PEVTLOGFILE LogFile,
               IN PLARGE_INTEGER FileOffset,
               IN ULONG Length)
{
    PLOGFILE pLogFile = (PLOGFILE)LogFile;
    IO_STATUS_BLOCK IoStatusBlock;

    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(Length);

    return NtFlushBuffersFile(pLogFile->FileHandle, &IoStatusBlock);
}

NTSTATUS
LogfCreate(PLOGFILE* LogFile,
           PCWSTR    LogName,
           PUNICODE_STRING FileName,
           ULONG     MaxSize,
           ULONG     Retention,
           BOOLEAN   Permanent,
           BOOLEAN   Backup)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION FileStdInfo;
    PLOGFILE pLogFile;
    SIZE_T LogNameLen;
    BOOLEAN CreateNew;

    pLogFile = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pLogFile));
    if (!pLogFile)
    {
        DPRINT1("Cannot allocate heap!\n");
        return STATUS_NO_MEMORY;
    }

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
        StringCchCopyW(pLogFile->LogName, LogNameLen, LogName);

    InitializeObjectAttributes(&ObjectAttributes,
                               FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    DPRINT("Going to create or open %wZ\n", FileName);
    Status = NtCreateFile(&pLogFile->FileHandle,
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
        DPRINT1("Cannot create file `%wZ' (Status 0x%08lx)\n", FileName, Status);
        goto Quit;
    }

    CreateNew = (IoStatusBlock.Information == FILE_CREATED);
    DPRINT("%wZ %s successfully\n", FileName, CreateNew ? "created" : "opened");

    /*
     * Retrieve the log file size and check whether the file is not too large;
     * this log format only supports files of theoretical size < 0xFFFFFFFF .
     *
     * As it happens that, on Windows (and ReactOS), retrieving the End-Of-File
     * information using NtQueryInformationFile with the FileEndOfFileInformation
     * class is invalid (who knows why...), use instead the FileStandardInformation
     * class, and the EndOfFile member of the returned FILE_STANDARD_INFORMATION
     * structure will give the desired information.
     */
    Status = NtQueryInformationFile(pLogFile->FileHandle,
                                    &IoStatusBlock,
                                    &FileStdInfo,
                                    sizeof(FileStdInfo),
                                    FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("EventLog: NtQueryInformationFile failed (Status 0x%08lx)\n", Status);
        goto Quit;
    }
    if (FileStdInfo.EndOfFile.HighPart != 0)
    {
        DPRINT1("EventLog: Log `%wZ' is too large.\n", FileName);
        Status = STATUS_EVENTLOG_FILE_CORRUPT; // STATUS_FILE_TOO_LARGE;
        goto Quit;
    }

    DPRINT("Initializing LogFile `%S'\n", pLogFile->LogName);

    Status = ElfCreateFile(&pLogFile->LogFile,
                           FileName,
                           FileStdInfo.EndOfFile.LowPart,
                           MaxSize,
                           Retention,
                           CreateNew,
                           Backup,
                           LogfpAlloc,
                           LogfpFree,
                           LogfpSetFileSize,
                           LogfpWriteFile,
                           LogfpReadFile,
                           LogfpFlushFile);
    if (!NT_SUCCESS(Status))
        goto Quit;

    pLogFile->Permanent = Permanent;

    RtlInitializeResource(&pLogFile->Lock);

    LogfListAddItem(pLogFile);

Quit:
    if (!NT_SUCCESS(Status))
    {
        if (pLogFile->FileHandle != NULL)
            NtClose(pLogFile->FileHandle);

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
    if (LogFile == NULL)
        return;

    if (!ForceClose && LogFile->Permanent)
        return;

    RtlAcquireResourceExclusive(&LogFile->Lock, TRUE);

    LogfListRemoveItem(LogFile);

    ElfCloseFile(&LogFile->LogFile);
    NtClose(LogFile->FileHandle);
    RtlFreeHeap(GetProcessHeap(), 0, LogFile->LogName);

    RtlDeleteResource(&LogFile->Lock);

    RtlFreeHeap(GetProcessHeap(), 0, LogFile);

    return;
}

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

NTSTATUS
LogfClearFile(PLOGFILE LogFile,
              PUNICODE_STRING BackupFileName)
{
    NTSTATUS Status;

    /* Lock the log file exclusive */
    RtlAcquireResourceExclusive(&LogFile->Lock, TRUE);

    if (BackupFileName->Length > 0)
    {
        /* Write a backup file */
        Status = LogfBackupFile(LogFile, BackupFileName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("LogfBackupFile failed (Status 0x%08lx)\n", Status);
            goto Quit;
        }
    }

    Status = ElfReCreateFile(&LogFile->LogFile);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LogfInitializeNew failed (Status 0x%08lx)\n", Status);
    }

Quit:
    /* Unlock the log file */
    RtlReleaseResource(&LogFile->Lock);
    return Status;
}

NTSTATUS
LogfBackupFile(PLOGFILE LogFile,
               PUNICODE_STRING BackupFileName)
{
    NTSTATUS Status;
    LOGFILE BackupLogFile;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    DPRINT("LogfBackupFile(%p, %wZ)\n", LogFile, BackupFileName);

    /* Lock the log file shared */
    RtlAcquireResourceShared(&LogFile->Lock, TRUE);

    InitializeObjectAttributes(&ObjectAttributes,
                               BackupFileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&BackupLogFile.FileHandle,
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
        DPRINT("Cannot create backup file `%wZ' (Status 0x%08lx)\n", BackupFileName, Status);
        goto Quit;
    }

    Status = ElfBackupFile(&LogFile->LogFile,
                           &BackupLogFile.LogFile);

Quit:
    /* Close the backup file */
    if (BackupLogFile.FileHandle != NULL)
        NtClose(BackupLogFile.FileHandle);

    /* Unlock the log file */
    RtlReleaseResource(&LogFile->Lock);

    return Status;
}


static NTSTATUS
ReadRecord(IN  PEVTLOGFILE LogFile,
           IN  ULONG RecordNumber,
           OUT PEVENTLOGRECORD Record,
           IN  SIZE_T  BufSize, // Length
           OUT PSIZE_T BytesRead OPTIONAL,
           OUT PSIZE_T BytesNeeded OPTIONAL,
           IN  BOOLEAN Ansi)
{
    NTSTATUS Status;
    PEVENTLOGRECORD UnicodeBuffer = NULL;
    PEVENTLOGRECORD Src, Dst;
    ANSI_STRING StringA;
    UNICODE_STRING StringW;
    PVOID SrcPtr, DstPtr;
    DWORD i;
    DWORD dwPadding;
    DWORD dwRecordLength;
    PDWORD pLength;

    if (!Ansi)
    {
        return ElfReadRecord(LogFile,
                             RecordNumber,
                             Record,
                             BufSize,
                             BytesRead,
                             BytesNeeded);
    }

    if (BytesRead)
        *BytesRead = 0;

    if (BytesNeeded)
        *BytesNeeded = 0;

    UnicodeBuffer = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, BufSize);
    if (UnicodeBuffer == NULL)
    {
        DPRINT1("Alloc failed!\n");
        return STATUS_NO_MEMORY;
    }

    Status = ElfReadRecord(LogFile,
                           RecordNumber,
                           UnicodeBuffer,
                           BufSize,
                           BytesRead,
                           BytesNeeded);
    if (!NT_SUCCESS(Status))
        goto Quit;

    Src = UnicodeBuffer;
    Dst = Record;

    Dst->Reserved      = Src->Reserved;
    Dst->RecordNumber  = Src->RecordNumber;
    Dst->TimeGenerated = Src->TimeGenerated;
    Dst->TimeWritten   = Src->TimeWritten;
    Dst->EventID       = Src->EventID;
    Dst->EventType     = Src->EventType;
    Dst->EventCategory = Src->EventCategory;
    Dst->NumStrings    = Src->NumStrings;
    Dst->UserSidLength = Src->UserSidLength;
    Dst->DataLength    = Src->DataLength;

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
    dwPadding = sizeof(ULONG) - (((ULONG_PTR)DstPtr - (ULONG_PTR)Dst) % sizeof(ULONG));
    RtlZeroMemory(DstPtr, dwPadding);

    /* Set the record length at the beginning and the end of the record */
    dwRecordLength = (DWORD)((ULONG_PTR)DstPtr + dwPadding + sizeof(ULONG) - (ULONG_PTR)Dst);
    Dst->Length = dwRecordLength;
    pLength = (PDWORD)((ULONG_PTR)DstPtr + dwPadding);
    *pLength = dwRecordLength;

    if (BytesRead)
        *BytesRead = dwRecordLength;

    Status = STATUS_SUCCESS;

Quit:
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
    ULONG RecNum;
    SIZE_T ReadLength, NeededSize;
    ULONG BufferUsage;

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

    /* Lock the log file shared */
    RtlAcquireResourceShared(&LogFile->Lock, TRUE);

    /*
     * In sequential read mode, a record number of 0 means we need
     * to determine where to start the read operation. Otherwise
     * we just use the provided record number.
     */
    if ((Flags & EVENTLOG_SEQUENTIAL_READ) && (*RecordNumber == 0))
    {
        if (Flags & EVENTLOG_FORWARDS_READ)
        {
            *RecordNumber = ElfGetOldestRecord(&LogFile->LogFile);
        }
        else // if (Flags & EVENTLOG_BACKWARDS_READ)
        {
            *RecordNumber = ElfGetCurrentRecord(&LogFile->LogFile) - 1;
        }
    }

    RecNum = *RecordNumber;

    *BytesRead = 0;
    *BytesNeeded = 0;

    BufferUsage = 0;
    do
    {
        Status = ReadRecord(&LogFile->LogFile,
                            RecNum,
                            (PEVENTLOGRECORD)(Buffer + BufferUsage),
                            BufSize - BufferUsage,
                            &ReadLength,
                            &NeededSize,
                            Ansi);
        if (Status == STATUS_NOT_FOUND)
        {
            if (BufferUsage == 0)
            {
                Status = STATUS_END_OF_FILE;
                goto Quit;
            }
            else
            {
                break;
            }
        }
        else
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            if (BufferUsage == 0)
            {
                *BytesNeeded = NeededSize;
                // Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }
            else
            {
                break;
            }
        }
        else
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ElfReadRecord failed (Status 0x%08lx)\n", Status);
            goto Quit;
        }

        /* Go to the next event record */
        /*
         * NOTE: This implicitly supposes that all the other record numbers
         * are consecutive (and do not jump than more than one unit); but if
         * it is not the case, then we would prefer here to call some
         * "get_next_record_number" function.
         */
        if (Flags & EVENTLOG_FORWARDS_READ)
            RecNum++;
        else // if (Flags & EVENTLOG_BACKWARDS_READ)
            RecNum--;

        BufferUsage += ReadLength;
    }
    while (BufferUsage <= BufSize);

    *BytesRead = BufferUsage;
    *RecordNumber = RecNum;

    Status = STATUS_SUCCESS;

Quit:
    /* Unlock the log file */
    RtlReleaseResource(&LogFile->Lock);

    if (!NT_SUCCESS(Status))
        DPRINT1("LogfReadEvents failed (Status 0x%08lx)\n", Status);

    return Status;
}

NTSTATUS
LogfWriteRecord(PLOGFILE LogFile,
                PEVENTLOGRECORD Record,
                SIZE_T BufSize)
{
    NTSTATUS Status;
    LARGE_INTEGER SystemTime;

    // ASSERT(sizeof(*Record) == sizeof(RecBuf));

    if (!Record || BufSize < sizeof(*Record))
        return STATUS_INVALID_PARAMETER;

    /* Lock the log file exclusive */
    RtlAcquireResourceExclusive(&LogFile->Lock, TRUE);

    /*
     * Retrieve the record written time now, that will also be compared
     * with the existing events timestamps in case the log is wrapping.
     */
    NtQuerySystemTime(&SystemTime);
    RtlTimeToSecondsSince1970(&SystemTime, &Record->TimeWritten);

    Status = ElfWriteRecord(&LogFile->LogFile, Record, BufSize);
    if (Status == STATUS_LOG_FILE_FULL)
    {
        /* The event log file is full, queue a message box for the user and exit */
        // TODO!
        DPRINT1("Log file `%S' is full!\n", LogFile->LogName);
    }

    /* Unlock the log file */
    RtlReleaseResource(&LogFile->Lock);

    return Status;
}


PEVENTLOGRECORD
LogfAllocAndBuildNewRecord(PSIZE_T pRecSize,
                           ULONG   Time,
                           USHORT  wType,
                           USHORT  wCategory,
                           ULONG   dwEventId,
                           PUNICODE_STRING SourceName,
                           PUNICODE_STRING ComputerName,
                           ULONG   dwSidLength,
                           PSID    pUserSid,
                           USHORT  wNumStrings,
                           PWSTR   pStrings,
                           ULONG   dwDataSize,
                           PVOID   pRawData)
{
    SIZE_T RecSize;
    SIZE_T SourceNameSize, ComputerNameSize, StringLen;
    PBYTE Buffer;
    PEVENTLOGRECORD pRec;
    PWSTR str;
    UINT i, pos;

    SourceNameSize   = (SourceName   && SourceName->Buffer)   ? SourceName->Length   : 0;
    ComputerNameSize = (ComputerName && ComputerName->Buffer) ? ComputerName->Length : 0;

    RecSize = sizeof(EVENTLOGRECORD) + /* Add the sizes of the strings, NULL-terminated */
        SourceNameSize + ComputerNameSize + 2*sizeof(UNICODE_NULL);

    /* Align on DWORD boundary for the SID */
    RecSize = ROUND_UP(RecSize, sizeof(ULONG));

    RecSize += dwSidLength;

    /* Add the sizes for the strings array */
    ASSERT((pStrings == NULL && wNumStrings == 0) ||
           (pStrings != NULL && wNumStrings >= 0));
    for (i = 0, str = pStrings; i < wNumStrings; i++)
    {
        StringLen = wcslen(str) + 1; // str must be != NULL
        RecSize += StringLen * sizeof(WCHAR);
        str += StringLen;
    }

    /* Add the data size */
    RecSize += dwDataSize;

    /* Align on DWORD boundary for the full structure */
    RecSize = ROUND_UP(RecSize, sizeof(ULONG));

    /* Size of the trailing 'Length' member */
    RecSize += sizeof(ULONG);

    Buffer = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, RecSize);
    if (!Buffer)
    {
        DPRINT1("Cannot allocate heap!\n");
        return NULL;
    }

    pRec = (PEVENTLOGRECORD)Buffer;
    pRec->Length = RecSize;
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

    /* NOTE: Equivalents of RtlStringCbCopyUnicodeString calls */
    if (SourceNameSize)
    {
        StringCbCopyNW((PWSTR)(Buffer + pos), SourceNameSize + sizeof(UNICODE_NULL),
                       SourceName->Buffer, SourceNameSize);
    }
    pos += SourceNameSize + sizeof(UNICODE_NULL);
    if (ComputerNameSize)
    {
        StringCbCopyNW((PWSTR)(Buffer + pos), ComputerNameSize + sizeof(UNICODE_NULL),
                       ComputerName->Buffer, ComputerNameSize);
    }
    pos += ComputerNameSize + sizeof(UNICODE_NULL);

    /* Align on DWORD boundary for the SID */
    pos = ROUND_UP(pos, sizeof(ULONG));

    pRec->UserSidLength = 0;
    pRec->UserSidOffset = 0;
    if (dwSidLength)
    {
        RtlCopyMemory(Buffer + pos, pUserSid, dwSidLength);
        pRec->UserSidLength = dwSidLength;
        pRec->UserSidOffset = pos;
        pos += dwSidLength;
    }

    pRec->StringOffset = pos;
    for (i = 0, str = pStrings; i < wNumStrings; i++)
    {
        StringLen = wcslen(str) + 1; // str must be != NULL
        StringCchCopyW((PWSTR)(Buffer + pos), StringLen, str);
        str += StringLen;
        pos += StringLen * sizeof(WCHAR);
    }
    pRec->NumStrings = wNumStrings;

    pRec->DataLength = 0;
    pRec->DataOffset = 0;
    if (dwDataSize)
    {
        RtlCopyMemory(Buffer + pos, pRawData, dwDataSize);
        pRec->DataLength = dwDataSize;
        pRec->DataOffset = pos;
        pos += dwDataSize;
    }

    /* Align on DWORD boundary for the full structure */
    pos = ROUND_UP(pos, sizeof(ULONG));

    /* Initialize the trailing 'Length' member */
    *((PDWORD)(Buffer + pos)) = RecSize;

    *pRecSize = RecSize;
    return pRec;
}

VOID
LogfReportEvent(USHORT wType,
                USHORT wCategory,
                ULONG  dwEventId,
                USHORT wNumStrings,
                PWSTR  pStrings,
                ULONG  dwDataSize,
                PVOID  pRawData)
{
    NTSTATUS Status;
    UNICODE_STRING SourceName, ComputerName;
    PEVENTLOGRECORD LogBuffer;
    LARGE_INTEGER SystemTime;
    ULONG Time;
    SIZE_T RecSize;
    DWORD dwComputerNameLength;
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    if (!EventLogSource)
        return;

    RtlInitUnicodeString(&SourceName, EventLogSource->szName);

    dwComputerNameLength = ARRAYSIZE(szComputerName);
    if (!GetComputerNameW(szComputerName, &dwComputerNameLength))
        szComputerName[0] = L'\0';

    RtlInitUnicodeString(&ComputerName, szComputerName);

    NtQuerySystemTime(&SystemTime);
    RtlTimeToSecondsSince1970(&SystemTime, &Time);

    LogBuffer = LogfAllocAndBuildNewRecord(&RecSize,
                                           Time,
                                           wType,
                                           wCategory,
                                           dwEventId,
                                           &SourceName,
                                           &ComputerName,
                                           0,
                                           NULL,
                                           wNumStrings,
                                           pStrings,
                                           dwDataSize,
                                           pRawData);
    if (LogBuffer == NULL)
    {
        DPRINT1("LogfAllocAndBuildNewRecord failed!\n");
        return;
    }

    Status = LogfWriteRecord(EventLogSource->LogFile, LogBuffer, RecSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR writing to event log `%S' (Status 0x%08lx)\n",
                EventLogSource->LogFile->LogName, Status);
    }

    LogfFreeRecord(LogBuffer);
}
