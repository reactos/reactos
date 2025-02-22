/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS BtrFS filesystem library
 * FILE:        include/reactos/libs/fslib/btrfslib.h
 * PURPOSE:     Public definitions for BtrFS filesystem library
 */

#ifndef __BTRFSLIB_H
#define __BTRFSLIB_H

#include <fmifs/fmifs.h>

BOOLEAN
NTAPI
BtrfsChkdsk(
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
    IN PULONG ExitStatus);

BOOLEAN
NTAPI
BtrfsFormat(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN QuickFormat,
    IN BOOLEAN BackwardCompatible,
    IN MEDIA_TYPE MediaType,
    IN PUNICODE_STRING Label,
    IN ULONG ClusterSize);

#endif /* __BTRFSLIB_H */
