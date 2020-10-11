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
// Formatting routines
//

struct _PARTENTRY; // Defined in partlist.h

BOOLEAN
PreparePartitionForFormatting(
    IN struct _PARTENTRY* PartEntry,
    IN PCWSTR FileSystemName);

/* EOF */
