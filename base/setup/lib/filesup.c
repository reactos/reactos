/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/lib/filesup.c
 * PURPOSE:         File support functions
 * PROGRAMMER:      Eric Kohl
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

#if 0

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

#endif

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

//
// NOTE: It may be possible to merge both DoesPathExist and DoesFileExist...
//
BOOLEAN
DoesPathExist(
    IN HANDLE RootDirectory OPTIONAL,
    IN PCWSTR PathName)
{
    NTSTATUS Status;
    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING Name;

    RtlInitUnicodeString(&Name, PathName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               RootDirectory,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
    if (NT_SUCCESS(Status))
        NtClose(FileHandle);
    else
        DPRINT1("Failed to open directory %wZ, Status 0x%08lx\n", &Name, Status);

    return NT_SUCCESS(Status);
}

BOOLEAN
DoesFileExist(
    IN HANDLE RootDirectory OPTIONAL,
    IN PCWSTR PathName OPTIONAL,
    IN PCWSTR FileName)
{
    NTSTATUS Status;
    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING Name;
    WCHAR FullName[MAX_PATH];

    CombinePaths(FullName, ARRAYSIZE(FullName), 2, PathName, FileName);
    RtlInitUnicodeString(&Name, FullName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               RootDirectory,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
    if (NT_SUCCESS(Status))
        NtClose(FileHandle);
    else
        DPRINT1("Failed to open file %wZ, Status 0x%08lx\n", &Name, Status);

    return NT_SUCCESS(Status);
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
    IN HANDLE RootDirectory OPTIONAL,
    IN PCWSTR PathName OPTIONAL,
    IN PCWSTR FileName,             // OPTIONAL
    OUT PHANDLE FileHandle,         // IN OUT PHANDLE OPTIONAL
    OUT PHANDLE SectionHandle,
    OUT PVOID* BaseAddress,
    OUT PULONG FileSize OPTIONAL)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    SIZE_T ViewSize;
    PVOID ViewBase;
    UNICODE_STRING Name;
    WCHAR FullName[MAX_PATH];

    CombinePaths(FullName, ARRAYSIZE(FullName), 2, PathName, FileName);
    RtlInitUnicodeString(&Name, FullName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               RootDirectory,
                               NULL);

    *FileHandle = NULL;
    *SectionHandle = NULL;

    Status = NtOpenFile(FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open file '%wZ', Status 0x%08lx\n", &Name, Status);
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
            DPRINT1("WARNING!! The file '%wZ' is too large!\n", &Name);

        *FileSize = FileInfo.EndOfFile.LowPart;

        DPRINT("File size: %lu\n", *FileSize);
    }

    /* Map the file in memory */

    /* Create the section */
    Status = NtCreateSection(SectionHandle,
                             SECTION_MAP_READ,
                             NULL,
                             NULL,
                             PAGE_READONLY,
                             SEC_COMMIT /* | SEC_IMAGE (_NO_EXECUTE) */,
                             *FileHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create a memory section for file '%wZ', Status 0x%08lx\n", &Name, Status);
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
                                PAGE_READONLY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to map a view for file %wZ, Status 0x%08lx\n", &Name, Status);
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
