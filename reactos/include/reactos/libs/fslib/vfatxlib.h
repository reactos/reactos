/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        include/fslib/vfatlib.h
 * PURPOSE:     Public definitions for vfat filesystem library
 */
#ifndef __VFATXLIB_H
#define __VFATXLIB_H

#include <fmifs/fmifs.h>

NTSTATUS NTAPI
VfatxFormat (PUNICODE_STRING DriveRoot,
	    FMIFS_MEDIA_FLAG MediaFlag,
	    PUNICODE_STRING Label,
	    BOOLEAN QuickFormat,
	    ULONG ClusterSize,
	    PFMIFSCALLBACK Callback);

#endif /*__VFATLIB_H */
