/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS EXT2 filesystem library
 * FILE:        include/reactos/libs/fslib/ext2lib.h
 * PURPOSE:     Public definitions for ext2 filesystem library
 */

#ifndef __EXT2LIB_H
#define __EXT2LIB_H

#include <fmifs/fmifs.h>

BOOLEAN
NTAPI
Ext2Chkdsk(
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
Ext2Format(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN QuickFormat,
    IN BOOLEAN BackwardCompatible,
    IN MEDIA_TYPE MediaType,
    IN PUNICODE_STRING Label,
    IN ULONG ClusterSize);

#endif /* __EXT2LIB_H */
