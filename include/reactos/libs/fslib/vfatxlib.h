/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        include/fslib/vfatlib.h
 * PURPOSE:     Public definitions for vfat filesystem library
 */
#ifndef __VFATXLIB_H
#define __VFATXLIB_H

#include <fmifs/fmifs.h>

NTSTATUS
VfatxInitialize (VOID);

NTSTATUS
VfatxCleanup (VOID);

NTSTATUS
VfatxFormat (PUNICODE_STRING DriveRoot,
	    ULONG MediaFlag,
	    BOOLEAN QuickFormat,
	    PFMIFSCALLBACK Callback);

#endif /*__VFATLIB_H */
