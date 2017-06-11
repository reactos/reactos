/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/filesup.c
 * PURPOSE:         File support functions
 * PROGRAMMER:      Eric Kohl
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

static BOOLEAN HasCurrentCabinet = FALSE;
static WCHAR CurrentCabinetName[MAX_PATH];
static CAB_SEARCH Search;

static
NTSTATUS
SetupCreateSingleDirectory(
    PWCHAR DirectoryName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING PathName;
    HANDLE DirectoryHandle;
    NTSTATUS Status;

    if(!RtlCreateUnicodeString(&PathName, DirectoryName))
        return STATUS_NO_MEMORY;

    if (PathName.Length > sizeof(WCHAR) &&
        PathName.Buffer[PathName.Length / sizeof(WCHAR) - 2] == L'\\' &&
        PathName.Buffer[PathName.Length / sizeof(WCHAR) - 1] == L'.')
    {
        PathName.Length -= sizeof(WCHAR);
        PathName.Buffer[PathName.Length / sizeof(WCHAR)] = 0;
    }

    if (PathName.Length > sizeof(WCHAR) &&
        PathName.Buffer[PathName.Length / sizeof(WCHAR) - 1] == L'\\')
    {
        PathName.Length -= sizeof(WCHAR);
        PathName.Buffer[PathName.Length / sizeof(WCHAR)] = 0;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &PathName,
                               OBJ_CASE_INSENSITIVE | OBJ_INHERIT,
                               NULL,
                               NULL);

    Status = NtCreateFile(&DirectoryHandle,
                          FILE_LIST_DIRECTORY | FILE_TRAVERSE | FILE_ADD_FILE | FILE_ADD_SUBDIRECTORY | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_DIRECTORY,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          FILE_OPEN_FOR_BACKUP_INTENT | FILE_DIRECTORY_FILE,
                          NULL,
                          0);
    if (NT_SUCCESS(Status))
    {
        NtClose(DirectoryHandle);
    }

    RtlFreeUnicodeString(&PathName);

    return Status;
}

NTSTATUS
SetupCreateDirectory(
    PWCHAR PathName)
{
    PWCHAR PathBuffer = NULL;
    PWCHAR Ptr, EndPtr;
    ULONG BackslashCount;
    ULONG Size;
    NTSTATUS Status = STATUS_SUCCESS;

    Size = (wcslen(PathName) + 1) * sizeof(WCHAR);
    PathBuffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, Size);
    if (PathBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    wcscpy(PathBuffer, PathName);
    EndPtr = PathBuffer + wcslen(PathName);

    Ptr = PathBuffer;

    /* Skip the '\Device\HarddiskX\PartitionY\ part */
    BackslashCount = 0;
    while (Ptr < EndPtr && BackslashCount < 4)
    {
        if (*Ptr == L'\\')
            BackslashCount++;

        Ptr++;
    }

    while (Ptr < EndPtr)
    {
        if (*Ptr == L'\\')
        {
            *Ptr = 0;

            DPRINT("PathBuffer: %S\n", PathBuffer);
            if (!DoesPathExist(NULL, PathBuffer))
            {
                DPRINT("Create: %S\n", PathBuffer);
                Status = SetupCreateSingleDirectory(PathBuffer);
                if (!NT_SUCCESS(Status))
                    goto done;
            }

            *Ptr = L'\\';
        }

        Ptr++;
    }

    if (!DoesPathExist(NULL, PathBuffer))
    {
        DPRINT("Create: %S\n", PathBuffer);
        Status = SetupCreateSingleDirectory(PathBuffer);
        if (!NT_SUCCESS(Status))
            goto done;
    }

done:
    DPRINT("Done.\n");
    if (PathBuffer != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, PathBuffer);

    return Status;
}

NTSTATUS
SetupDeleteFile(
    IN PCWSTR FileName,
    IN BOOLEAN ForceDelete) // ForceDelete can be used to delete read-only files
{
    NTSTATUS Status;
    UNICODE_STRING NtPathU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    FILE_DISPOSITION_INFORMATION FileDispInfo;
    BOOLEAN RetryOnce = FALSE;

    /* Open the directory name that was passed in */
    RtlInitUnicodeString(&NtPathU, FileName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathU,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

Retry: // We go back there once if RetryOnce == TRUE
    Status = NtOpenFile(&FileHandle,
                        DELETE | FILE_READ_ATTRIBUTES |
                        (RetryOnce ? FILE_WRITE_ATTRIBUTES : 0),
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile failed with Status 0x%08lx\n", Status);
        return Status;
    }

    if (RetryOnce)
    {
        FILE_BASIC_INFORMATION FileInformation;

        Status = NtQueryInformationFile(FileHandle,
                                        &IoStatusBlock,
                                        &FileInformation,
                                        sizeof(FILE_BASIC_INFORMATION),
                                        FileBasicInformation);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtQueryInformationFile failed with Status 0x%08lx\n", Status);
            NtClose(FileHandle);
            return Status;
        }

        FileInformation.FileAttributes = FILE_ATTRIBUTE_NORMAL;
        Status = NtSetInformationFile(FileHandle,
                                      &IoStatusBlock,
                                      &FileInformation,
                                      sizeof(FILE_BASIC_INFORMATION),
                                      FileBasicInformation);
        NtClose(FileHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtSetInformationFile failed with Status 0x%08lx\n", Status);
            return Status;
        }
    }

    /* Ask for the file to be deleted */
    FileDispInfo.DeleteFile = TRUE;
    Status = NtSetInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileDispInfo,
                                  sizeof(FILE_DISPOSITION_INFORMATION),
                                  FileDispositionInformation);
    NtClose(FileHandle);

    if (!NT_SUCCESS(Status))
        DPRINT1("Deletion of file '%S' failed, Status 0x%08lx\n", FileName, Status);

    // FIXME: Check the precise value of Status!
    if (!NT_SUCCESS(Status) && ForceDelete && !RetryOnce)
    {
        /* Retry once */
        RetryOnce = TRUE;
        goto Retry;
    }

    /* Return result to the caller */
    return Status;
}

NTSTATUS
SetupCopyFile(
    IN PCWSTR SourceFileName,
    IN PCWSTR DestinationFileName,
    IN BOOLEAN FailIfExists)
{
    NTSTATUS Status;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandleSource;
    HANDLE FileHandleDest;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION FileStandard;
    FILE_BASIC_INFORMATION FileBasic;
    ULONG RegionSize;
    HANDLE SourceFileSection;
    PVOID SourceFileMap = NULL;
    SIZE_T SourceSectionSize = 0;
    LARGE_INTEGER ByteOffset;

    RtlInitUnicodeString(&FileName, SourceFileName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandleSource,
                        GENERIC_READ,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        FILE_SEQUENTIAL_ONLY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile failed: %x, %wZ\n", Status, &FileName);
        goto done;
    }

    Status = NtQueryInformationFile(FileHandleSource,
                                    &IoStatusBlock,
                                    &FileStandard,
                                    sizeof(FILE_STANDARD_INFORMATION),
                                    FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryInformationFile failed: %x\n", Status);
        goto closesrc;
    }

    Status = NtQueryInformationFile(FileHandleSource,
                                    &IoStatusBlock,
                                    &FileBasic,
                                    sizeof(FILE_BASIC_INFORMATION),
                                    FileBasicInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryInformationFile failed: %x\n", Status);
        goto closesrc;
    }

    Status = NtCreateSection(&SourceFileSection,
                             SECTION_MAP_READ,
                             NULL,
                             NULL,
                             PAGE_READONLY,
                             SEC_COMMIT,
                             FileHandleSource);
    if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateSection failed: %x, %S\n", Status, SourceFileName);
      goto closesrc;
    }

    Status = NtMapViewOfSection(SourceFileSection,
                                NtCurrentProcess(),
                                &SourceFileMap,
                                0,
                                0,
                                NULL,
                                &SourceSectionSize,
                                ViewUnmap,
                                0,
                                PAGE_READONLY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtMapViewOfSection failed: %x, %S\n", Status, SourceFileName);
        goto closesrcsec;
    }

    RtlInitUnicodeString(&FileName, DestinationFileName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&FileHandleDest,
                          GENERIC_WRITE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FileBasic.FileAttributes, // FILE_ATTRIBUTE_NORMAL,
                          0,
                          FailIfExists ? FILE_CREATE : FILE_OVERWRITE_IF,
                          FILE_NO_INTERMEDIATE_BUFFERING |
                          FILE_SEQUENTIAL_ONLY |
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateFile failed: %x, %wZ\n", Status, &FileName);
        goto unmapsrcsec;
    }

    RegionSize = (ULONG)PAGE_ROUND_UP(FileStandard.EndOfFile.u.LowPart);
    IoStatusBlock.Status = 0;
    ByteOffset.QuadPart = 0ULL;
    Status = NtWriteFile(FileHandleDest,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         SourceFileMap,
                         RegionSize,
                         &ByteOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile failed: %x:%x, iosb: %p src: %p, size: %x\n",
                Status, IoStatusBlock.Status, &IoStatusBlock, SourceFileMap, RegionSize);
        goto closedest;
    }

    /* Copy file date/time from source file */
    Status = NtSetInformationFile(FileHandleDest,
                                  &IoStatusBlock,
                                  &FileBasic,
                                  sizeof(FILE_BASIC_INFORMATION),
                                  FileBasicInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetInformationFile failed: %x\n", Status);
        goto closedest;
    }

    /* Shorten the file back to its real size after completing the write */
    Status = NtSetInformationFile(FileHandleDest,
                                  &IoStatusBlock,
                                  &FileStandard.EndOfFile,
                                  sizeof(FILE_END_OF_FILE_INFORMATION),
                                  FileEndOfFileInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetInformationFile failed: %x\n", Status);
    }

closedest:
    NtClose(FileHandleDest);

unmapsrcsec:
    NtUnmapViewOfSection(NtCurrentProcess(), SourceFileMap);

closesrcsec:
    NtClose(SourceFileSection);

closesrc:
    NtClose(FileHandleSource);

done:
    return Status;
}

/*
 * Synchronized with its kernel32 counterpart, but we don't manage reparse points here.
 */
NTSTATUS
SetupMoveFile(
    IN PCWSTR ExistingFileName,
    IN PCWSTR NewFileName,
    IN ULONG Flags)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PFILE_RENAME_INFORMATION RenameInfo;
    UNICODE_STRING NewPathU, ExistingPathU;
    HANDLE SourceHandle = NULL;
    BOOLEAN ReplaceIfExists;

    RtlInitUnicodeString(&ExistingPathU, ExistingFileName);
    RtlInitUnicodeString(&NewPathU, NewFileName);

    _SEH2_TRY
    {
        ReplaceIfExists = !!(Flags & MOVEFILE_REPLACE_EXISTING);

        /* Unless we manage a proper opening, we'll attempt to reopen without reparse support */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &ExistingPathU,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        /* Attempt to open source file */
        Status = NtOpenFile(&SourceHandle,
                            FILE_READ_ATTRIBUTES | DELETE | SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            FILE_OPEN_FOR_BACKUP_INTENT | ((Flags & MOVEFILE_WRITE_THROUGH) ? FILE_WRITE_THROUGH : 0));
        if (!NT_SUCCESS(Status))
        {
            if (Status != STATUS_INVALID_PARAMETER)
            {
                _SEH2_LEAVE;
            }
        }

        /* At that point, we MUST have a source handle */
        ASSERT(SourceHandle);

        /* Allocate renaming buffer and fill it */
        RenameInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, NewPathU.Length + sizeof(FILE_RENAME_INFORMATION));
        if (RenameInfo == NULL)
        {
            Status = STATUS_NO_MEMORY;
            _SEH2_LEAVE;
        }

        RtlCopyMemory(&RenameInfo->FileName, NewPathU.Buffer, NewPathU.Length);
        RenameInfo->ReplaceIfExists = ReplaceIfExists;
        RenameInfo->RootDirectory = NULL;
        RenameInfo->FileNameLength = NewPathU.Length;

        /* Attempt to rename the file */
        Status = NtSetInformationFile(SourceHandle,
                                      &IoStatusBlock,
                                      RenameInfo,
                                      NewPathU.Length + sizeof(FILE_RENAME_INFORMATION),
                                      FileRenameInformation);
        RtlFreeHeap(RtlGetProcessHeap(), 0, RenameInfo);
        if (NT_SUCCESS(Status))
        {
            /* If it succeeded, all fine, quit */
            _SEH2_LEAVE;
        }
        /*
         * If we failed for any other reason than not the same device, fail.
         * If we failed because of different devices, only allow renaming
         * if user allowed copy.
         */
        if (Status != STATUS_NOT_SAME_DEVICE || !(Flags & MOVEFILE_COPY_ALLOWED))
        {
            /* ReactOS hack! To be removed once all FSD have proper renaming support
             * Just leave status to error and leave
             */
            if (Status == STATUS_NOT_IMPLEMENTED)
            {
                DPRINT1("Forcing copy, renaming not supported by FSD\n");
            }
            else
            {
                _SEH2_LEAVE;
            }
        }

        /* Close the source file */
        NtClose(SourceHandle);
        SourceHandle = NULL;

        /* Perform the file copy */
        Status = SetupCopyFile(ExistingFileName,
                               NewFileName,
                               !ReplaceIfExists);

        /* If it succeeded, delete the source file */
        if (NT_SUCCESS(Status))
        {
            /* Force-delete files even if read-only */
            SetupDeleteFile(ExistingFileName, TRUE);
        }
    }
    _SEH2_FINALLY
    {
        if (SourceHandle)
            NtClose(SourceHandle);
    }
    _SEH2_END;

    return Status;
}

#ifdef __REACTOS__
NTSTATUS
SetupExtractFile(
    PWCHAR CabinetFileName,
    PWCHAR SourceFileName,
    PWCHAR DestinationPathName)
{
    ULONG CabStatus;

    DPRINT("SetupExtractFile(CabinetFileName %S, SourceFileName %S, DestinationPathName %S)\n",
           CabinetFileName, SourceFileName, DestinationPathName);

    if (HasCurrentCabinet)
    {
        DPRINT("CurrentCabinetName: %S\n", CurrentCabinetName);
    }

    if ((HasCurrentCabinet) && (wcscmp(CabinetFileName, CurrentCabinetName) == 0))
    {
        DPRINT("Using same cabinet as last time\n");

        /* Use our last location because the files should be sequential */
        CabStatus = CabinetFindNextFileSequential(SourceFileName, &Search);
        if (CabStatus != CAB_STATUS_SUCCESS)
        {
            DPRINT("Sequential miss on file: %S\n", SourceFileName);

            /* Looks like we got unlucky */
            CabStatus = CabinetFindFirst(SourceFileName, &Search);
        }
    }
    else
    {
        DPRINT("Using new cabinet\n");

        if (HasCurrentCabinet)
        {
            CabinetCleanup();
        }

        wcscpy(CurrentCabinetName, CabinetFileName);

        CabinetInitialize();
        CabinetSetEventHandlers(NULL, NULL, NULL);
        CabinetSetCabinetName(CabinetFileName);

        CabStatus = CabinetOpen();
        if (CabStatus == CAB_STATUS_SUCCESS)
        {
            DPRINT("Opened cabinet %S\n", CabinetGetCabinetName());
            HasCurrentCabinet = TRUE;
        }
        else
        {
            DPRINT("Cannot open cabinet (%d)\n", CabStatus);
            return STATUS_UNSUCCESSFUL;
        }

        /* We have to start at the beginning here */
        CabStatus = CabinetFindFirst(SourceFileName, &Search);
    }

    if (CabStatus != CAB_STATUS_SUCCESS)
    {
        DPRINT1("Unable to find '%S' in cabinet '%S'\n", SourceFileName, CabinetGetCabinetName());
        return STATUS_UNSUCCESSFUL;
    }

    CabinetSetDestinationPath(DestinationPathName);
    CabStatus = CabinetExtractFile(&Search);
    if (CabStatus != CAB_STATUS_SUCCESS)
    {
        DPRINT("Cannot extract file %S (%d)\n", SourceFileName, CabStatus);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}
#endif


BOOLEAN
IsValidPath(
    IN PWCHAR InstallDir,
    IN ULONG Length)
{
    UINT i;

    // TODO: Add check for 8.3 too.

    /* Check for whitespaces */
    for (i = 0; i < Length; i++)
    {
        if (iswspace(InstallDir[i]))
            return FALSE;
    }

    return TRUE;
}

/* EOF */
