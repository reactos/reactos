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

BOOLEAN
NTAPI
CdfsFormat(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN QuickFormat,
    IN BOOLEAN BackwardCompatible,
    IN MEDIA_TYPE MediaType,
    IN PUNICODE_STRING Label,
    IN ULONG ClusterSize)
{
    // Not possible for CDFS (ISO-9660).
    return FALSE;
}

BOOLEAN
NTAPI
CdfsChkdsk(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN FixErrors,
    IN BOOLEAN Verbose,
    IN BOOLEAN CheckOnlyIfDirty,
    IN BOOLEAN ScanDrive,
    IN PVOID pUnknown1,
    IN PVOID pUnknown2,
    IN PVOID pUnknown3,
    IN PVOID pUnknown4,
    IN PULONG ExitStatus)
{
    UNIMPLEMENTED;
    *ExitStatus = (ULONG)STATUS_SUCCESS;
    return TRUE;
}
