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
    _In_ PUNICODE_STRING DriveRoot,
    _In_ PFMIFSCALLBACK Callback,
    _In_ BOOLEAN FixErrors,
    _In_ BOOLEAN Verbose,
    _In_ BOOLEAN CheckOnlyIfDirty,
    _In_ BOOLEAN ScanDrive,
    _In_opt_ PVOID pUnknown1,
    _In_opt_ PVOID pUnknown2,
    _In_opt_ PVOID pUnknown3,
    _In_opt_ PVOID pUnknown4,
    _Out_ PULONG ExitStatus);

BOOLEAN
NTAPI
VfatFormat(
    _In_ PUNICODE_STRING DriveRoot,
    _In_ PFMIFSCALLBACK Callback,
    _In_ BOOLEAN QuickFormat,
    _In_ BOOLEAN BackwardCompatible,
    _In_ MEDIA_TYPE MediaType,
    _In_ PUNICODE_STRING Label,
    _In_ ULONG ClusterSize);

#endif /* __VFATLIB_H */
