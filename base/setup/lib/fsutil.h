/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Filesystem Format and ChkDsk support functions.
 * COPYRIGHT:   Copyright 2003-2019 Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Copyright 2017-2020 Hermes Belusca-Maito
 */

#pragma once

#include <fmifs/fmifs.h>

/** QueryAvailableFileSystemFormat() **/
BOOLEAN
GetRegisteredFileSystems(
    IN ULONG Index,
    OUT PCWSTR* FileSystemName);


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


//
// Bootsector routines
//

#define FAT_BOOTSECTOR_SIZE     (1 * SECTORSIZE)
#define FAT32_BOOTSECTOR_SIZE   (1 * SECTORSIZE) // Counts only the primary sector.
#define BTRFS_BOOTSECTOR_SIZE   (3 * SECTORSIZE)

typedef NTSTATUS
(/*NTAPI*/ *PFS_INSTALL_BOOTCODE)(
    IN PCWSTR SrcPath,          // Bootsector source file (on the installation medium)
    IN HANDLE DstPath,          // Where to save the bootsector built from the source + partition information
    IN HANDLE RootPartition);   // Partition holding the (old) bootsector data information

NTSTATUS
InstallFatBootCode(
    IN PCWSTR SrcPath,
    IN HANDLE DstPath,
    IN HANDLE RootPartition);

#define InstallFat12BootCode    InstallFatBootCode
#define InstallFat16BootCode    InstallFatBootCode

NTSTATUS
InstallFat32BootCode(
    IN PCWSTR SrcPath,
    IN HANDLE DstPath,
    IN HANDLE RootPartition);

NTSTATUS
InstallBtrfsBootCode(
    IN PCWSTR SrcPath,
    IN HANDLE DstPath,
    IN HANDLE RootPartition);


//
// Formatting routines
//

NTSTATUS
ChkdskPartition(
    IN PPARTENTRY PartEntry,
    IN BOOLEAN FixErrors,
    IN BOOLEAN Verbose,
    IN BOOLEAN CheckOnlyIfDirty,
    IN BOOLEAN ScanDrive,
    IN PFMIFSCALLBACK Callback);

NTSTATUS
FormatPartition(
    IN PPARTENTRY PartEntry,
    IN PCWSTR FileSystemName,
    IN FMIFS_MEDIA_FLAG MediaFlag,
    IN PCWSTR Label,
    IN BOOLEAN QuickFormat,
    IN ULONG ClusterSize,
    IN PFMIFSCALLBACK Callback);

/* EOF */
