/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     File support functions.
 * COPYRIGHT:   Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Copyright 2017-2018 Hermes Belusca-Maito
 */

#pragma once

NTSTATUS
SetupCreateDirectory(
    _In_ PCWSTR PathName);

NTSTATUS
SetupDeleteFile(
    IN PCWSTR FileName,
    IN BOOLEAN ForceDelete); // ForceDelete can be used to delete read-only files

NTSTATUS
SetupCopyFile(
    IN PCWSTR SourceFileName,
    IN PCWSTR DestinationFileName,
    IN BOOLEAN FailIfExists);

#ifndef _WINBASE_

#define MOVEFILE_REPLACE_EXISTING   1
#define MOVEFILE_COPY_ALLOWED       2
#define MOVEFILE_WRITE_THROUGH      8

#endif

NTSTATUS
SetupMoveFile(
    IN PCWSTR ExistingFileName,
    IN PCWSTR NewFileName,
    IN ULONG Flags);

NTSTATUS
ConcatPathsV(
    IN OUT PWSTR PathBuffer,
    IN SIZE_T cchPathSize,
    IN ULONG NumberOfPathComponents,
    IN va_list PathComponentsList);

NTSTATUS
CombinePathsV(
    OUT PWSTR PathBuffer,
    IN SIZE_T cchPathSize,
    IN ULONG NumberOfPathComponents,
    IN va_list PathComponentsList);

NTSTATUS
ConcatPaths(
    IN OUT PWSTR PathBuffer,
    IN SIZE_T cchPathSize,
    IN ULONG NumberOfPathComponents,
    IN /* PCWSTR */ ...);

NTSTATUS
CombinePaths(
    OUT PWSTR PathBuffer,
    IN SIZE_T cchPathSize,
    IN ULONG NumberOfPathComponents,
    IN /* PCWSTR */ ...);

BOOLEAN
DoesPathExist_UStr(
    _In_opt_ HANDLE RootDirectory,
    _In_ PCUNICODE_STRING PathName,
    _In_ BOOLEAN IsDirectory);

BOOLEAN
DoesPathExist(
    _In_opt_ HANDLE RootDirectory,
    _In_ PCWSTR PathName,
    _In_ BOOLEAN IsDirectory);

#define DoesDirExist(RootDirectory, DirName)    \
    DoesPathExist((RootDirectory), (DirName), TRUE)

#define DoesFileExist(RootDirectory, FileName)  \
    DoesPathExist((RootDirectory), (FileName), FALSE)

// FIXME: DEPRECATED! HACKish function that needs to be deprecated!
BOOLEAN
DoesFileExist_2(
    IN PCWSTR PathName OPTIONAL,
    IN PCWSTR FileName);

BOOLEAN
NtPathToDiskPartComponents(
    IN PCWSTR NtPath,
    OUT PULONG pDiskNumber,
    OUT PULONG pPartNumber,
    OUT PCWSTR* PathComponent OPTIONAL);

NTSTATUS
OpenAndMapFile(
    _In_opt_ HANDLE RootDirectory,
    _In_ PCWSTR PathNameToFile,
    _Out_opt_ PHANDLE FileHandle,
    _Out_opt_ PULONG FileSize,
    _Out_ PHANDLE SectionHandle,
    _Out_ PVOID* BaseAddress,
    _In_ BOOLEAN ReadWriteAccess);

NTSTATUS
MapFile(
    _In_ HANDLE FileHandle,
    _Out_ PHANDLE SectionHandle,
    _Out_ PVOID* BaseAddress,
    _In_ BOOLEAN ReadWriteAccess);

BOOLEAN
UnMapFile(
    _In_ HANDLE SectionHandle,
    _In_ PVOID BaseAddress);

#define UnMapAndCloseFile(FileHandle, SectionHandle, BaseAddress)   \
do {    \
    UnMapFile((SectionHandle), (BaseAddress));  \
    NtClose(FileHandle);                        \
} while (0)

/* EOF */
