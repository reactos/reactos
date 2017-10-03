/*
 * PROJECT:     ReactOS CDFS library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Checking and Formatting CDFS volumes
 * COPYRIGHT:   Copyright 2017 Colin Finck <colin@reactos.org>
 */

#define NTOS_MODE_USER
#include <ndk/umtypes.h>
#include <fmifs/fmifs.h>

#define NDEBUG
#include <debug.h>

NTSTATUS NTAPI
CdfsChkdsk(IN PUNICODE_STRING DriveRoot,
    IN BOOLEAN FixErrors,
    IN BOOLEAN Verbose,
    IN BOOLEAN CheckOnlyIfDirty,
    IN BOOLEAN ScanDrive,
    IN PFMIFSCALLBACK Callback)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
CdfsFormat(IN PUNICODE_STRING DriveRoot,
    IN FMIFS_MEDIA_FLAG MediaFlag,
    IN PUNICODE_STRING Label,
    IN BOOLEAN QuickFormat,
    IN ULONG ClusterSize,
    IN PFMIFSCALLBACK Callback)
{
    // Not possible for CDFS (ISO-9660).
    return STATUS_NOT_SUPPORTED;
}
