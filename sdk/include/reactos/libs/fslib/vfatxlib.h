/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        include/reactos/libs/fslib/vfatxlib.h
 * PURPOSE:     Public definitions for vfat filesystem library
 */

#ifndef __VFATXLIB_H
#define __VFATXLIB_H

#include <fmifs/fmifs.h>

BOOLEAN
NTAPI
VfatxChkdsk(
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
VfatxFormat(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN QuickFormat,
    IN BOOLEAN BackwardCompatible,
    IN MEDIA_TYPE MediaType,
    IN PUNICODE_STRING Label,
    IN ULONG ClusterSize);

#endif /* __VFATXLIB_H */
