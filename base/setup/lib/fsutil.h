/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Filesystem support functions
 * COPYRIGHT:   Copyright 2003-2019 Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Copyright 2017-2019 Hermes Belusca-Maito
 */

#pragma once

#include <fmifs/fmifs.h>

/** QueryAvailableFileSystemFormat() **/
BOOLEAN
GetRegisteredFileSystems(
    IN ULONG Index,
    OUT PCWSTR* FileSystemName);

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


/** ChkdskEx() **/
NTSTATUS
ChkdskFileSystem_UStr(
    IN PUNICODE_STRING DriveRoot,
    IN PCWSTR FileSystemName,
    IN BOOLEAN FixErrors,
    IN BOOLEAN Verbose,
    IN BOOLEAN CheckOnlyIfDirty,
    IN BOOLEAN ScanDrive,
    IN PFMIFSCALLBACK Callback);

NTSTATUS
ChkdskFileSystem(
    IN PCWSTR DriveRoot,
    IN PCWSTR FileSystemName,
    IN BOOLEAN FixErrors,
    IN BOOLEAN Verbose,
    IN BOOLEAN CheckOnlyIfDirty,
    IN BOOLEAN ScanDrive,
    IN PFMIFSCALLBACK Callback);


/** FormatEx() **/
NTSTATUS
FormatFileSystem_UStr(
    IN PUNICODE_STRING DriveRoot,
    IN PCWSTR FileSystemName,
    IN FMIFS_MEDIA_FLAG MediaFlag,
    IN PUNICODE_STRING Label,
    IN BOOLEAN QuickFormat,
    IN ULONG ClusterSize,
    IN PFMIFSCALLBACK Callback);

NTSTATUS
FormatFileSystem(
    IN PCWSTR DriveRoot,
    IN PCWSTR FileSystemName,
    IN FMIFS_MEDIA_FLAG MediaFlag,
    IN PCWSTR Label,
    IN BOOLEAN QuickFormat,
    IN ULONG ClusterSize,
    IN PFMIFSCALLBACK Callback);


UCHAR
FileSystemToPartitionType(
    IN PCWSTR FileSystem,
    IN PULARGE_INTEGER StartSector,
    IN PULARGE_INTEGER SectorCount);


//
// Formatting routines
//

struct _PARTENTRY; // Defined in partlist.h

BOOLEAN
PreparePartitionForFormatting(
    IN struct _PARTENTRY* PartEntry,
    IN PCWSTR FileSystemName);

/* EOF */
