/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS FFS filesystem library
 * FILE:        include/reactos/libs/fslib/ffslib.h
 * PURPOSE:     Public definitions for FFS filesystem library
 */

#ifndef __FFSLIB_H
#define __FFSLIB_H

#include <fmifs/fmifs.h>

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
    IN PULONG ExitStatus);

BOOLEAN
NTAPI
FfsFormat(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN QuickFormat,
    IN BOOLEAN BackwardCompatible,
    IN MEDIA_TYPE MediaType,
    IN PUNICODE_STRING Label,
    IN ULONG ClusterSize);

#endif /* __FFSLIB_H */
