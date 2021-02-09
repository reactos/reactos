/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Filesystem Recognition support functions,
 *              using NT OS functionality.
 * COPYRIGHT:   Copyright 2017-2020 Hermes Belusca-Maito
 */

#pragma once

NTSTATUS
GetFileSystemName_UStr(
    IN PUNICODE_STRING PartitionPath OPTIONAL,
    IN HANDLE PartitionHandle OPTIONAL,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize);

NTSTATUS
GetFileSystemName(
    IN PCWSTR PartitionPath OPTIONAL,
    IN HANDLE PartitionHandle OPTIONAL,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize);

NTSTATUS
InferFileSystem(
    IN PCWSTR PartitionPath OPTIONAL,
    IN HANDLE PartitionHandle OPTIONAL,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize);

UCHAR
FileSystemToMBRPartitionType(
    IN PCWSTR FileSystem,
    IN ULONGLONG StartSector,
    IN ULONGLONG SectorCount);

/* EOF */
