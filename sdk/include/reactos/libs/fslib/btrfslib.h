/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS BtrFS filesystem library
 * FILE:        include/reactos/libs/fslib/btrfslib.h
 * PURPOSE:     Public definitions for BtrFS filesystem library
 */
#ifndef __BTRFSLIB_H
#define __BTRFSLIB_H

#include <fmifs/fmifs.h>

NTSTATUS NTAPI
BtrfsChkdskEx(
	IN PUNICODE_STRING DriveRoot,
	IN BOOLEAN FixErrors,
	IN BOOLEAN Verbose,
	IN BOOLEAN CheckOnlyIfDirty,
	IN BOOLEAN ScanDrive,
	IN PFMIFSCALLBACK Callback);

NTSTATUS NTAPI
BtrfsFormatEx(
	IN PUNICODE_STRING DriveRoot,
	IN FMIFS_MEDIA_FLAG MediaFlag,
	IN PUNICODE_STRING Label,
	IN BOOLEAN QuickFormat,
	IN ULONG ClusterSize,
	IN PFMIFSCALLBACK Callback);

#endif /*__BTRFSLIB_H */
