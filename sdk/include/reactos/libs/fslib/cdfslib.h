/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS CDFS filesystem library
 * FILE:        include/reactos/libs/fslib/cdfslib.h
 * PURPOSE:     Public definitions for CDFS filesystem library
 */
#ifndef __CDFSLIB_H
#define __CDFSLIB_H

#include <fmifs/fmifs.h>

NTSTATUS NTAPI
CdfsChkdsk(
	IN PUNICODE_STRING DriveRoot,
	IN BOOLEAN FixErrors,
	IN BOOLEAN Verbose,
	IN BOOLEAN CheckOnlyIfDirty,
	IN BOOLEAN ScanDrive,
	IN PFMIFSCALLBACK Callback);

NTSTATUS NTAPI
CdfsFormat(
	IN PUNICODE_STRING DriveRoot,
	IN FMIFS_MEDIA_FLAG MediaFlag,
	IN PUNICODE_STRING Label,
	IN BOOLEAN QuickFormat,
	IN ULONG ClusterSize,
	IN PFMIFSCALLBACK Callback);

#endif /*__CDFSLIB_H */
