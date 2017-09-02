/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS ReiserFS library
 * FILE:        lib/fslib/reiserfslib/reiserfslib.c
 * PURPOSE:     ReiserFS lib
 * PROGRAMMERS: Pierre Schweitzer
 */
#include "reiserfslib.h"

#define NDEBUG
#include <debug.h>

NTSTATUS NTAPI
ReiserfsFormat(IN PUNICODE_STRING DriveRoot,
               IN FMIFS_MEDIA_FLAG MediaFlag,
               IN PUNICODE_STRING Label,
               IN BOOLEAN QuickFormat,
               IN ULONG ClusterSize,
               IN PFMIFSCALLBACK Callback)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}


NTSTATUS NTAPI
ReiserfsChkdsk(IN PUNICODE_STRING DriveRoot,
               IN BOOLEAN FixErrors,
               IN BOOLEAN Verbose,
               IN BOOLEAN CheckOnlyIfDirty,
               IN BOOLEAN ScanDrive,
               IN PFMIFSCALLBACK Callback)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}
