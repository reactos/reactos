/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS CDFS filesystem library
 * FILE:        include/reactos/libs/fslib/cdfslib.h
 * PURPOSE:     Public definitions for CDFS filesystem library
 */

#ifndef __CDFSLIB_H
#define __CDFSLIB_H

#include <fmifs/fmifs.h>

BOOLEAN
NTAPI
CdfsChkdsk(
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
CdfsFormat(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN QuickFormat,
    IN BOOLEAN BackwardCompatible,
    IN MEDIA_TYPE MediaType,
    IN PUNICODE_STRING Label,
    IN ULONG ClusterSize);

#endif /* __CDFSLIB_H */
