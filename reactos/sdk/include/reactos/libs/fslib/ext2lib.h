/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS EXT2 filesystem library
 * FILE:        include/reactos/libs/fslib/ext2lib.h
 * PURPOSE:     Public definitions for ext2 filesystem library
 */
#ifndef __EXT2LIB_H
#define __EXT2LIB_H

#include <fmifs/fmifs.h>

NTSTATUS NTAPI
Ext2Chkdsk(
	IN PUNICODE_STRING DriveRoot,
	IN BOOLEAN FixErrors,
	IN BOOLEAN Verbose,
	IN BOOLEAN CheckOnlyIfDirty,
	IN BOOLEAN ScanDrive,
	IN PFMIFSCALLBACK Callback);

NTSTATUS NTAPI
Ext2Format(
	IN PUNICODE_STRING DriveRoot,
	IN FMIFS_MEDIA_FLAG MediaFlag,
	IN PUNICODE_STRING Label,
	IN BOOLEAN QuickFormat,
	IN ULONG ClusterSize,
	IN PFMIFSCALLBACK Callback);

#endif /*__EXT2LIB_H */
