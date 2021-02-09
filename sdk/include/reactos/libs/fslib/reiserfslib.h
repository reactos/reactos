/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS ReiserFS filesystem library
 * FILE:        include/reactos/libs/fslib/reiserfslib.h
 * PURPOSE:     Public definitions for ReiserFS filesystem library
 */

#ifndef __REISERFSLIB_H
#define __REISERFSLIB_H

#include <fmifs/fmifs.h>

BOOLEAN
NTAPI
ReiserfsChkdsk(
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
ReiserfsFormat(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN QuickFormat,
    IN BOOLEAN BackwardCompatible,
    IN MEDIA_TYPE MediaType,
    IN PUNICODE_STRING Label,
    IN ULONG ClusterSize);

#endif /* __REISERFSLIB_H */
