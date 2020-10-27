/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Filesystem Recognition support functions,
 *              using NT OS functionality.
 * COPYRIGHT:   Copyright 2017-2020 Hermes Belusca-Maito
 */

#pragma once

NTSTATUS
GetFileSystemNameByHandle(
    IN HANDLE PartitionHandle,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize);

NTSTATUS
GetFileSystemName_UStr(
    IN PUNICODE_STRING PartitionPath,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize);

NTSTATUS
GetFileSystemName(
    IN PCWSTR Partition,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize);

NTSTATUS
InferFileSystemByHandle(
    IN HANDLE PartitionHandle,
    IN UCHAR PartitionType,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize);

NTSTATUS
InferFileSystem(
    IN PCWSTR Partition,
    IN UCHAR PartitionType,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize);

UCHAR
FileSystemToPartitionType(
    IN PCWSTR FileSystem,
    IN PULARGE_INTEGER StartSector,
    IN PULARGE_INTEGER SectorCount);

/* EOF */
