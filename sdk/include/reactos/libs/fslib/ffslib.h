/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS FFS filesystem library
 * FILE:        include/reactos/libs/fslib/ffslib.h
 * PURPOSE:     Public definitions for FFS filesystem library
 */
#ifndef __FFSLIB_H
#define __FFSLIB_H

#include <fmifs/fmifs.h>

NTSTATUS NTAPI
FfsChkdsk(
	IN PUNICODE_STRING DriveRoot,
	IN BOOLEAN FixErrors,
	IN BOOLEAN Verbose,
	IN BOOLEAN CheckOnlyIfDirty,
	IN BOOLEAN ScanDrive,
	IN PFMIFSCALLBACK Callback);

NTSTATUS NTAPI
FfsFormat(
	IN PUNICODE_STRING DriveRoot,
	IN FMIFS_MEDIA_FLAG MediaFlag,
	IN PUNICODE_STRING Label,
	IN BOOLEAN QuickFormat,
	IN ULONG ClusterSize,
	IN PFMIFSCALLBACK Callback);

#endif /*__FFSLIB_H */
