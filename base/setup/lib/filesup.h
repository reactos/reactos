/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/lib/filesup.h
 * PURPOSE:         File support functions
 * PROGRAMMER:      Eric Kohl
 */

#pragma once

#if 0

BOOLEAN
IsValidPath(
    IN PWCHAR InstallDir,
    IN ULONG Length);
#endif

HRESULT
ConcatPathsV(
    IN OUT PWSTR PathBuffer,
    IN SIZE_T cchPathSize,
    IN ULONG NumberOfPathComponents,
    IN va_list PathComponentsList);

HRESULT
CombinePathsV(
    OUT PWSTR PathBuffer,
    IN SIZE_T cchPathSize,
    IN ULONG NumberOfPathComponents,
    IN va_list PathComponentsList);

HRESULT
ConcatPaths(
    IN OUT PWSTR PathBuffer,
    IN SIZE_T cchPathSize,
    IN ULONG NumberOfPathComponents,
    IN /* PCWSTR */ ...);

HRESULT
CombinePaths(
    OUT PWSTR PathBuffer,
    IN SIZE_T cchPathSize,
    IN ULONG NumberOfPathComponents,
    IN /* PCWSTR */ ...);

BOOLEAN
DoesPathExist(
    IN HANDLE RootDirectory OPTIONAL,
    IN PCWSTR PathName);

BOOLEAN
DoesFileExist(
    IN HANDLE RootDirectory OPTIONAL,
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
    IN HANDLE RootDirectory OPTIONAL,
    IN PCWSTR PathName OPTIONAL,
    IN PCWSTR FileName,             // OPTIONAL
    OUT PHANDLE FileHandle,         // IN OUT PHANDLE OPTIONAL
    OUT PHANDLE SectionHandle,
    OUT PVOID* BaseAddress,
    OUT PULONG FileSize OPTIONAL);

BOOLEAN
UnMapFile(
    IN HANDLE SectionHandle,
    IN PVOID BaseAddress);

/* EOF */
