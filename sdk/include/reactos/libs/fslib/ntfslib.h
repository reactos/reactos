/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NTFS filesystem library
 * FILE:        include/reactos/libs/fslib/ntfslib.h
 * PURPOSE:     Public definitions for NTFS filesystem library
 */
#ifndef __NTFSLIB_H
#define __NTFSLIB_H

#include <fmifs/fmifs.h>

NTSTATUS NTAPI
NtfsChkdsk(
	IN PUNICODE_STRING DriveRoot,
	IN BOOLEAN FixErrors,
	IN BOOLEAN Verbose,
	IN BOOLEAN CheckOnlyIfDirty,
	IN BOOLEAN ScanDrive,
	IN PFMIFSCALLBACK Callback);

NTSTATUS NTAPI
NtfsFormat(
	IN PUNICODE_STRING DriveRoot,
	IN FMIFS_MEDIA_FLAG MediaFlag,
	IN PUNICODE_STRING Label,
	IN BOOLEAN QuickFormat,
	IN ULONG ClusterSize,
	IN PFMIFSCALLBACK Callback);

#endif /*__NTFSLIB_H */
