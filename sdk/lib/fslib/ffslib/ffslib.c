/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS FFS library
 * FILE:        lib/fslib/ffslib/ffslib.c
 * PURPOSE:     FFS lib
 * PROGRAMMERS: Pierre Schweitzer
 */
#include "ffslib.h"

#define NDEBUG
#include <debug.h>

NTSTATUS NTAPI
FfsFormat(IN PUNICODE_STRING DriveRoot,
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
FfsChkdsk(IN PUNICODE_STRING DriveRoot,
          IN BOOLEAN FixErrors,
          IN BOOLEAN Verbose,
          IN BOOLEAN CheckOnlyIfDirty,
          IN BOOLEAN ScanDrive,
          IN PFMIFSCALLBACK Callback)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}
