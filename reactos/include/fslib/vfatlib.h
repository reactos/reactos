/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        include/fslib/vfatlib.h
 * PURPOSE:     Public definitions for vfat filesystem library
 */
#ifndef __VFATLIB_H
#define __VFATLIB_H

#include <fmifs.h>


NTSTATUS
VfatInitialize();

NTSTATUS
VfatCleanup();

NTSTATUS
VfatFormat(
	PUNICODE_STRING  DriveRoot,
	DWORD  MediaFlag,
	PUNICODE_STRING  Label,
	BOOL  QuickFormat,
	DWORD  ClusterSize,
	PFMIFSCALLBACK  Callback);

#endif /*__VFATLIB_H */
