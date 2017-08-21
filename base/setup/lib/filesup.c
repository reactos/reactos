/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/filesup.c
 * PURPOSE:         File support functions
 * PROGRAMMERS:     Eric Kohl
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>


// ACHTUNG! HAXX FIXME!!
#define _SEH2_TRY
#define _SEH2_LEAVE     goto __SEH2_FINALLY__label;
#define _SEH2_FINALLY   __SEH2_FINALLY__label:
#define _SEH2_END


/* FUNCTIONS ****************************************************************/

// TODO: Move SetupCreateDirectory later...

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

HRESULT
ConcatPathsV(
    IN OUT PWSTR PathBuffer,
    IN SIZE_T cchPathSize,
    IN ULONG NumberOfPathComponents,
    IN va_list PathComponentsList)
{
    HRESULT hr = S_OK;
    SIZE_T cchPathLen;
    PCWSTR PathComponent;

    if (cchPathSize < 1)
        return S_OK;

    while (NumberOfPathComponents--)
    {
        PathComponent = va_arg(PathComponentsList, PCWSTR);
        if (!PathComponent)
            continue;

        cchPathLen = min(cchPathSize, wcslen(PathBuffer));
        if (cchPathLen >= cchPathSize)
            return STRSAFE_E_INSUFFICIENT_BUFFER;

        if (PathComponent[0] != OBJ_NAME_PATH_SEPARATOR &&
            cchPathLen > 0 && PathBuffer[cchPathLen-1] != OBJ_NAME_PATH_SEPARATOR)
        {
            /* PathComponent does not start with '\' and PathBuffer does not end with '\' */
            hr = StringCchCatW(PathBuffer, cchPathSize, L"\\");
            if (FAILED(hr))
                return hr;
        }
        else if (PathComponent[0] == OBJ_NAME_PATH_SEPARATOR &&
                 cchPathLen > 0 && PathBuffer[cchPathLen-1] == OBJ_NAME_PATH_SEPARATOR)
        {
            /* PathComponent starts with '\' and PathBuffer ends with '\' */
            while (*PathComponent == OBJ_NAME_PATH_SEPARATOR)
                ++PathComponent; // Skip any backslash
        }
        hr = StringCchCatW(PathBuffer, cchPathSize, PathComponent);
        if (FAILED(hr))
            return hr;
    }

    return hr;
}

HRESULT
CombinePathsV(
    OUT PWSTR PathBuffer,
    IN SIZE_T cchPathSize,
    IN ULONG NumberOfPathComponents,
    IN va_list PathComponentsList)
{
    if (cchPathSize < 1)
        return S_OK;

    *PathBuffer = UNICODE_NULL;
    return ConcatPathsV(PathBuffer, cchPathSize,
                        NumberOfPathComponents, PathComponentsList);
}

HRESULT
ConcatPaths(
    IN OUT PWSTR PathBuffer,
    IN SIZE_T cchPathSize,
    IN ULONG NumberOfPathComponents,
    IN /* PCWSTR */ ...)
{
    HRESULT hr;
    va_list PathComponentsList;

    if (cchPathSize < 1)
        return S_OK;

    va_start(PathComponentsList, NumberOfPathComponents);
    hr = ConcatPathsV(PathBuffer, cchPathSize,
                      NumberOfPathComponents, PathComponentsList);
    va_end(PathComponentsList);

    return hr;
}

HRESULT
CombinePaths(
    OUT PWSTR PathBuffer,
    IN SIZE_T cchPathSize,
    IN ULONG NumberOfPathComponents,
    IN /* PCWSTR */ ...)
{
    HRESULT hr;
    va_list PathComponentsList;

    if (cchPathSize < 1)
        return S_OK;

    *PathBuffer = UNICODE_NULL;

    va_start(PathComponentsList, NumberOfPathComponents);
    hr = CombinePathsV(PathBuffer, cchPathSize,
                       NumberOfPathComponents, PathComponentsList);
    va_end(PathComponentsList);

    return hr;
}

BOOLEAN
DoesPathExistEx(
    IN HANDLE RootDirectory OPTIONAL,
    IN PCWSTR PathName,
    IN BOOLEAN IsDirectory)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    RtlInitUnicodeString(&Name, PathName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               RootDirectory,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        IsDirectory ? (FILE_LIST_DIRECTORY | SYNCHRONIZE)
                                    :  FILE_GENERIC_READ, // Contains SYNCHRONIZE
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT |
                            (IsDirectory ? FILE_DIRECTORY_FILE
                                         : FILE_NON_DIRECTORY_FILE));
    if (NT_SUCCESS(Status))
        NtClose(FileHandle);
    else
        DPRINT1("Failed to open %s '%wZ', Status 0x%08lx\n",
                IsDirectory ? "directory" : "file",
                &Name, Status);

    return NT_SUCCESS(Status);
}

// FIXME: DEPRECATED! HACKish function that needs to be deprecated!
BOOLEAN
DoesFileExist_2(
    IN PCWSTR PathName OPTIONAL,
    IN PCWSTR FileName)
{
    WCHAR FullName[MAX_PATH];
    CombinePaths(FullName, ARRAYSIZE(FullName), 2, PathName, FileName);
    return DoesFileExist(NULL, FullName);
}

/*
 * The format of NtPath should be:
 *    \Device\HarddiskXXX\PartitionYYY[\path] ,
 * where XXX and YYY respectively represent the hard disk and partition numbers,
 * and [\path] represent an optional path (separated by '\\').
 *
 * If a NT path of such a form is correctly parsed, the function returns respectively:
 * - in pDiskNumber: the hard disk number XXX,
 * - in pPartNumber: the partition number YYY,
 * - in PathComponent: pointer value (inside NtPath) to the beginning of \path.
 *
 * NOTE: The function does not accept leading whitespace.
 */
BOOLEAN
NtPathToDiskPartComponents(
    IN PCWSTR NtPath,
    OUT PULONG pDiskNumber,
    OUT PULONG pPartNumber,
    OUT PCWSTR* PathComponent OPTIONAL)
{
    ULONG DiskNumber, PartNumber;
    PCWSTR Path;

    *pDiskNumber = 0;
    *pPartNumber = 0;
    if (PathComponent) *PathComponent = NULL;

    Path = NtPath;

    if (_wcsnicmp(Path, L"\\Device\\Harddisk", 16) != 0)
    {
        /* The NT path doesn't start with the prefix string, thus it cannot be a hard disk device path */
        DPRINT1("'%S' : Not a possible hard disk device.\n", NtPath);
        return FALSE;
    }

    Path += 16;

    /* A number must be present now */
    if (!iswdigit(*Path))
    {
        DPRINT1("'%S' : expected a number! Not a regular hard disk device.\n", Path);
        return FALSE;
    }
    DiskNumber = wcstoul(Path, (PWSTR*)&Path, 10);

    /* Either NULL termination, or a path separator must be present now */
    if (*Path && *Path != OBJ_NAME_PATH_SEPARATOR)
    {
        DPRINT1("'%S' : expected a path separator!\n", Path);
        return FALSE;
    }

    if (!*Path)
    {
        DPRINT1("The path only specified a hard disk (and nothing else, like a partition...), so we stop there.\n");
        goto Quit;
    }

    /* Here, *Path == L'\\' */

    if (_wcsnicmp(Path, L"\\Partition", 10) != 0)
    {
        /* Actually, \Partition is optional so, if we don't have it, we still return success. Or should we? */
        DPRINT1("'%S' : unexpected format!\n", NtPath);
        goto Quit;
    }

    Path += 10;

    /* A number must be present now */
    if (!iswdigit(*Path))
    {
        /* If we don't have a number it means this part of path is actually not a partition specifier, so we shouldn't fail either. Or should we? */
        DPRINT1("'%S' : expected a number!\n", Path);
        goto Quit;
    }
    PartNumber = wcstoul(Path, (PWSTR*)&Path, 10);

    /* Either NULL termination, or a path separator must be present now */
    if (*Path && *Path != OBJ_NAME_PATH_SEPARATOR)
    {
        /* We shouldn't fail here because it just means this part of path is actually not a partition specifier. Or should we? */
        DPRINT1("'%S' : expected a path separator!\n", Path);
        goto Quit;
    }

    /* OK, here we really have a partition specifier: return its number */
    *pPartNumber = PartNumber;

Quit:
    /* Return the disk number */
    *pDiskNumber = DiskNumber;

    /* Return the path component also, if the user wants it */
    if (PathComponent) *PathComponent = Path;

    return TRUE;
}

NTSTATUS
OpenAndMapFile(
    IN  HANDLE RootDirectory OPTIONAL,
    IN  PCWSTR PathNameToFile,
    OUT PHANDLE FileHandle,         // IN OUT PHANDLE OPTIONAL
    OUT PHANDLE SectionHandle,
    OUT PVOID* BaseAddress,
    OUT PULONG FileSize OPTIONAL,
    IN  BOOLEAN ReadWriteAccess)
{
    NTSTATUS Status;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG SectionPageProtection;
    SIZE_T ViewSize;
    PVOID ViewBase;

    /* Open the file */

    RtlInitUnicodeString(&FileName, PathNameToFile);

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               RootDirectory,
                               NULL);

    *FileHandle = NULL;
    *SectionHandle = NULL;

    Status = NtOpenFile(FileHandle,
                        FILE_GENERIC_READ | // Contains SYNCHRONIZE
                            (ReadWriteAccess ? FILE_GENERIC_WRITE : 0),
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open file '%wZ', Status 0x%08lx\n", &FileName, Status);
        return Status;
    }

    if (FileSize)
    {
        /* Query the file size */
        FILE_STANDARD_INFORMATION FileInfo;
        Status = NtQueryInformationFile(*FileHandle,
                                        &IoStatusBlock,
                                        &FileInfo,
                                        sizeof(FileInfo),
                                        FileStandardInformation);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtQueryInformationFile() failed (Status %lx)\n", Status);
            NtClose(*FileHandle);
            *FileHandle = NULL;
            return Status;
        }

        if (FileInfo.EndOfFile.HighPart != 0)
            DPRINT1("WARNING!! The file '%wZ' is too large!\n", &FileName);

        *FileSize = FileInfo.EndOfFile.LowPart;

        DPRINT("File size: %lu\n", *FileSize);
    }

    /* Map the file in memory */

    SectionPageProtection = (ReadWriteAccess ? PAGE_READWRITE : PAGE_READONLY);

    /* Create the section */
    Status = NtCreateSection(SectionHandle,
                             STANDARD_RIGHTS_REQUIRED | SECTION_QUERY |
                             SECTION_MAP_READ |
                                (ReadWriteAccess ? SECTION_MAP_WRITE : 0),
                             NULL,
                             NULL,
                             SectionPageProtection,
                             SEC_COMMIT /* | SEC_IMAGE (_NO_EXECUTE) */,
                             *FileHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create a memory section for file '%wZ', Status 0x%08lx\n", &FileName, Status);
        NtClose(*FileHandle);
        *FileHandle = NULL;
        return Status;
    }

    /* Map the section */
    ViewSize = 0;
    ViewBase = NULL;
    Status = NtMapViewOfSection(*SectionHandle,
                                NtCurrentProcess(),
                                &ViewBase,
                                0, 0,
                                NULL,
                                &ViewSize,
                                ViewShare,
                                0,
                                SectionPageProtection);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to map a view for file '%wZ', Status 0x%08lx\n", &FileName, Status);
        NtClose(*SectionHandle);
        *SectionHandle = NULL;
        NtClose(*FileHandle);
        *FileHandle = NULL;
        return Status;
    }

    *BaseAddress = ViewBase;
    return STATUS_SUCCESS;
}

BOOLEAN
UnMapFile(
    IN HANDLE SectionHandle,
    IN PVOID BaseAddress)
{
    NTSTATUS Status;
    BOOLEAN Success = TRUE;

    Status = NtUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UnMapFile: NtUnmapViewOfSection(0x%p) failed with Status 0x%08lx\n",
                BaseAddress, Status);
        Success = FALSE;
    }
    Status = NtClose(SectionHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UnMapFile: NtClose(0x%p) failed with Status 0x%08lx\n",
                SectionHandle, Status);
        Success = FALSE;
    }

    return Success;
}

/* EOF */
