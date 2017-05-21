/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/* COPYRIGHT:       See COPYING in the top level directory
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
                          DIRECTORY_ALL_ACCESS,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_DIRECTORY,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          FILE_DIRECTORY_FILE,
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
SetupCopyFile(
    PWCHAR SourceFileName,
    PWCHAR DestinationFileName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandleSource;
    HANDLE FileHandleDest;
    static IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION FileStandard;
    FILE_BASIC_INFORMATION FileBasic;
    ULONG RegionSize;
    UNICODE_STRING FileName;
    NTSTATUS Status;
    PVOID SourceFileMap = 0;
    HANDLE SourceFileSection;
    SIZE_T SourceSectionSize = 0;
    LARGE_INTEGER ByteOffset;

    RtlInitUnicodeString(&FileName,
                         SourceFileName);

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
                                    &IoStatusBlock,&FileBasic,
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
                                PAGE_READONLY );
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtMapViewOfSection failed: %x, %S\n", Status, SourceFileName);
        goto closesrcsec;
    }

    RtlInitUnicodeString(&FileName,
                         DestinationFileName);

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
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OVERWRITE_IF,
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
        DPRINT1("NtWriteFile failed: %x:%x, iosb: %p src: %p, size: %x\n", Status, IoStatusBlock.Status, &IoStatusBlock, SourceFileMap, RegionSize);
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

    /* shorten the file back to it's real size after completing the write */
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

HRESULT
ConcatPaths(
    IN OUT PWSTR PathElem1,
    IN SIZE_T cchPathSize,
    IN PCWSTR PathElem2 OPTIONAL)
{
    HRESULT hr;
    SIZE_T cchPathLen;

    if (!PathElem2)
        return S_OK;
    if (cchPathSize <= 1)
        return S_OK;

    cchPathLen = min(cchPathSize, wcslen(PathElem1));

    if (PathElem2[0] != L'\\' && cchPathLen > 0 && PathElem1[cchPathLen-1] != L'\\')
    {
        /* PathElem2 does not start with '\' and PathElem1 does not end with '\' */
        hr = StringCchCatW(PathElem1, cchPathSize, L"\\");
        if (FAILED(hr))
            return hr;
    }
    else if (PathElem2[0] == L'\\' && cchPathLen > 0 && PathElem1[cchPathLen-1] == L'\\')
    {
        /* PathElem2 starts with '\' and PathElem1 ends with '\' */
        while (*PathElem2 == L'\\')
            ++PathElem2; // Skip any backslash
    }
    hr = StringCchCatW(PathElem1, cchPathSize, PathElem2);
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

    if (PathName)
        StringCchCopyW(FullName, ARRAYSIZE(FullName), PathName);
    else
        FullName[0] = UNICODE_NULL;

    if (FileName)
        ConcatPaths(FullName, ARRAYSIZE(FullName), FileName);

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
    if (!Path)
    {
        DPRINT1("An error happened!\n");
        return FALSE;
    }
    else if (*Path && *Path != OBJ_NAME_PATH_SEPARATOR)
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
    if (!Path)
    {
        /* We fail here because wcstoul failed for whatever reason */
        DPRINT1("An error happened!\n");
        return FALSE;
    }
    else if (*Path && *Path != OBJ_NAME_PATH_SEPARATOR)
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

    if (PathName)
        StringCchCopyW(FullName, ARRAYSIZE(FullName), PathName);
    else
        FullName[0] = UNICODE_NULL;

    if (FileName)
        ConcatPaths(FullName, ARRAYSIZE(FullName), FileName);

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
        DPRINT1("Failed to open file %wZ, Status 0x%08lx\n", &Name, Status);
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
            DPRINT1("WARNING!! The file %wZ is too large!\n", Name);

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
        DPRINT1("Failed to create a memory section for file %wZ, Status 0x%08lx\n", &Name, Status);
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
