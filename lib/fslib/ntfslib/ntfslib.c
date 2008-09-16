/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NTFS FS library
 * FILE:        lib/fslib/ntfslib/ntfslib.c
 * PURPOSE:     NTFS lib
 * PROGRAMMERS: Pierre Schweitzer
 */
#include "ntfslib.h"

#define NDEBUG
#include <debug.h>

NTSTATUS NTAPI
NtfsFormat(PUNICODE_STRING DriveRoot,
           FMIFS_MEDIA_FLAG MediaFlag,
           PUNICODE_STRING Label,
           BOOLEAN QuickFormat,
           ULONG ClusterSize,
           PFMIFSCALLBACK Callback)
{
  UNIMPLEMENTED;
  return STATUS_SUCCESS;
}


NTSTATUS WINAPI
NtfsChkdsk(IN PUNICODE_STRING DriveRoot,
           IN BOOLEAN FixErrors,
           IN BOOLEAN Verbose,
           IN BOOLEAN CheckOnlyIfDirty,
           IN BOOLEAN ScanDrive,
           IN PFMIFSCALLBACK Callback)
{
  UNIMPLEMENTED;
  return STATUS_SUCCESS;
}
