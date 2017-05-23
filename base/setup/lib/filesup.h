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
ConcatPaths(
    IN OUT PWSTR PathElem1,
    IN SIZE_T cchPathSize,
    IN PCWSTR PathElem2 OPTIONAL);

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
