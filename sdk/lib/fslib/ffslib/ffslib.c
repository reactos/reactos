/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS FFS library
 * FILE:        lib/fslib/ffslib/ffslib.c
 * PURPOSE:     FFS lib
 * PROGRAMMERS: Pierre Schweitzer
 */

#define NTOS_MODE_USER
#include <ndk/umtypes.h>
#include <fmifs/fmifs.h>

#define NDEBUG
#include <debug.h>

BOOLEAN
NTAPI
FfsFormat(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN QuickFormat,
    IN BOOLEAN BackwardCompatible,
    IN MEDIA_TYPE MediaType,
    IN PUNICODE_STRING Label,
    IN ULONG ClusterSize)
{
    UNIMPLEMENTED;
    return TRUE;
}

BOOLEAN
NTAPI
FfsChkdsk(
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
