/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Filesystem Format and ChkDsk support functions
 * COPYRIGHT:   Copyright 2003-2019 Casper S. Hornstrup <chorns@users.sourceforge.net>
 *              Copyright 2017-2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
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
    _In_ PUNICODE_STRING DriveRoot,
    _In_ PCWSTR FileSystemName,
    _In_ BOOLEAN FixErrors,
    _In_ BOOLEAN Verbose,
    _In_ BOOLEAN CheckOnlyIfDirty,
    _In_ BOOLEAN ScanDrive,
    _In_opt_ PFMIFSCALLBACK Callback);

NTSTATUS
ChkdskFileSystem(
    _In_ PCWSTR DriveRoot,
    _In_ PCWSTR FileSystemName,
    _In_ BOOLEAN FixErrors,
    _In_ BOOLEAN Verbose,
    _In_ BOOLEAN CheckOnlyIfDirty,
    _In_ BOOLEAN ScanDrive,
    _In_opt_ PFMIFSCALLBACK Callback);


/** FormatEx() **/
NTSTATUS
FormatFileSystem_UStr(
    _In_ PUNICODE_STRING DriveRoot,
    _In_ PCWSTR FileSystemName,
    _In_ FMIFS_MEDIA_FLAG MediaFlag,
    _In_opt_ PUNICODE_STRING Label,
    _In_ BOOLEAN QuickFormat,
    _In_ ULONG ClusterSize,
    _In_opt_ PFMIFSCALLBACK Callback);

NTSTATUS
FormatFileSystem(
    _In_ PCWSTR DriveRoot,
    _In_ PCWSTR FileSystemName,
    _In_ FMIFS_MEDIA_FLAG MediaFlag,
    _In_opt_ PCWSTR Label,
    _In_ BOOLEAN QuickFormat,
    _In_ ULONG ClusterSize,
    _In_opt_ PFMIFSCALLBACK Callback);


//
// Bootsector routines
//

#define FAT_BOOTSECTOR_SIZE     (1 * SECTORSIZE)
#define FAT32_BOOTSECTOR_SIZE   (1 * SECTORSIZE) // Counts only the primary sector.
#define BTRFS_BOOTSECTOR_SIZE   (3 * SECTORSIZE)
#define NTFS_BOOTSECTOR_SIZE   (16 * SECTORSIZE)

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

NTSTATUS
InstallNtfsBootCode(
    IN PCWSTR SrcPath,
    IN HANDLE DstPath,
    IN HANDLE RootPartition);


//
// Formatting routines
//

NTSTATUS
ChkdskPartition(
    _In_ PPARTENTRY PartEntry,
    _In_ BOOLEAN FixErrors,
    _In_ BOOLEAN Verbose,
    _In_ BOOLEAN CheckOnlyIfDirty,
    _In_ BOOLEAN ScanDrive,
    _In_opt_ PFMIFSCALLBACK Callback);

NTSTATUS
FormatPartition(
    _In_ PPARTENTRY PartEntry,
    _In_ PCWSTR FileSystemName,
    _In_ FMIFS_MEDIA_FLAG MediaFlag,
    _In_opt_ PCWSTR Label,
    _In_ BOOLEAN QuickFormat,
    _In_ ULONG ClusterSize,
    _In_opt_ PFMIFSCALLBACK Callback);


//
// FileSystem Volume Operations Queue
//

typedef enum _FSVOLNOTIFY
{
    FSVOLNOTIFY_STARTQUEUE = 0,
    FSVOLNOTIFY_ENDQUEUE,
    FSVOLNOTIFY_STARTSUBQUEUE,
    FSVOLNOTIFY_ENDSUBQUEUE,
// FSVOLNOTIFY_STARTPARTITION, FSVOLNOTIFY_ENDPARTITION,
    FSVOLNOTIFY_PARTITIONERROR,
    FSVOLNOTIFY_STARTFORMAT,
    FSVOLNOTIFY_ENDFORMAT,
    FSVOLNOTIFY_FORMATERROR,
    FSVOLNOTIFY_STARTCHECK,
    FSVOLNOTIFY_ENDCHECK,
    FSVOLNOTIFY_CHECKERROR,
    /**/ChangeSystemPartition/**/ // FIXME: Deprecate!
} FSVOLNOTIFY;

typedef enum _FSVOL_OP
{
/* Operations ****/
    FSVOL_FORMAT = 0,
    FSVOL_CHECK,
/* Response actions ****/
    FSVOL_ABORT = 0,
    FSVOL_DOIT,
    FSVOL_RETRY = FSVOL_DOIT,
    FSVOL_SKIP,
} FSVOL_OP;

typedef struct _FORMAT_VOLUME_INFO
{
    PVOLENTRY Volume;
    // PCWSTR NtPathPartition;
    NTSTATUS ErrorStatus;

/* Input information given by the 'FSVOLNOTIFY_STARTFORMAT' step ****/
    PCWSTR FileSystemName;
    FMIFS_MEDIA_FLAG MediaFlag;
    PCWSTR Label;
    BOOLEAN QuickFormat;
    ULONG ClusterSize;
    PFMIFSCALLBACK Callback;

} FORMAT_VOLUME_INFO, *PFORMAT_VOLUME_INFO;

typedef struct _CHECK_VOLUME_INFO
{
    PVOLENTRY Volume;
    // PCWSTR NtPathPartition;
    NTSTATUS ErrorStatus;

/* Input information given by the 'FSVOLNOTIFY_STARTCHECK' step ****/
    BOOLEAN FixErrors;
    BOOLEAN Verbose;
    BOOLEAN CheckOnlyIfDirty;
    BOOLEAN ScanDrive;
    PFMIFSCALLBACK Callback;

} CHECK_VOLUME_INFO, *PCHECK_VOLUME_INFO;

typedef FSVOL_OP
(CALLBACK *PFSVOL_CALLBACK)(
    _In_opt_ PVOID Context,
    _In_ FSVOLNOTIFY FormatStatus,
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2);

BOOLEAN
FsVolCommitOpsQueue(
    _In_ PPARTLIST PartitionList,
    _In_ PVOLENTRY SystemVolume,
    _In_ PVOLENTRY InstallVolume,
    _In_opt_ PFSVOL_CALLBACK FsVolCallback,
    _In_opt_ PVOID Context);

/* EOF */
