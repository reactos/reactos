/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS ReiserFS filesystem library
 * FILE:        include/reactos/libs/fslib/reiserfslib.h
 * PURPOSE:     Public definitions for ReiserFS filesystem library
 */
#ifndef __REISERFSLIB_H
#define __REISERFSLIB_H

#include <fmifs/fmifs.h>

NTSTATUS NTAPI
ReiserfsChkdsk(
	IN PUNICODE_STRING DriveRoot,
	IN BOOLEAN FixErrors,
	IN BOOLEAN Verbose,
	IN BOOLEAN CheckOnlyIfDirty,
	IN BOOLEAN ScanDrive,
	IN PFMIFSCALLBACK Callback);

NTSTATUS NTAPI
ReiserfsFormat(
	IN PUNICODE_STRING DriveRoot,
	IN FMIFS_MEDIA_FLAG MediaFlag,
	IN PUNICODE_STRING Label,
	IN BOOLEAN QuickFormat,
	IN ULONG ClusterSize,
	IN PFMIFSCALLBACK Callback);

#endif /*__REISERFSLIB_H */
