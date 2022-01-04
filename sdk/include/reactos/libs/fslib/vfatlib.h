/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        include/reactos/libs/fslib/vfatlib.h
 * PURPOSE:     Public definitions for vfat filesystem library
 */

#ifndef __VFATLIB_H
#define __VFATLIB_H

#include <fmifs/fmifs.h>

BOOLEAN
NTAPI
VfatChkdsk(
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
VfatFormat(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN QuickFormat,
    IN BOOLEAN BackwardCompatible,
    IN MEDIA_TYPE MediaType,
    IN PUNICODE_STRING Label,
    IN ULONG ClusterSize);

#endif /* __VFATLIB_H */
