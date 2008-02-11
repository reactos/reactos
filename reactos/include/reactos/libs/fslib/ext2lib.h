/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS EXT2 filesystem library
 * FILE:        include/fslib/ext2lib.h
 * PURPOSE:     Public definitions for ext2 filesystem library
 */
#ifndef __EXT2LIB_H
#define __EXT2LIB_H

#include <fmifs.h>

NTSTATUS
Ext2Initialize (VOID);

NTSTATUS
Ext2Cleanup (VOID);

NTSTATUS
Ext2Format (PUNICODE_STRING DriveRoot,
	    ULONG MediaFlag,
	    PUNICODE_STRING Label,
	    BOOLEAN QuickFormat,
	    ULONG ClusterSize,
	    PFMIFSCALLBACK Callback);

#endif /*__EXT2LIB_H */
