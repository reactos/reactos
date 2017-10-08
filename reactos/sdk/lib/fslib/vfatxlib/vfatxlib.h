/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        vfatxlib.h
 */

#ifndef _VFATXLIB_H_
#define _VFATXLIB_H_

#define NTOS_MODE_USER
#include <ndk/umtypes.h>
#include <ndk/pstypes.h>
#include <ndk/ldrtypes.h>
#include <ndk/iofuncs.h>
#include <fmifs/fmifs.h>

#include <pshpack1.h>
typedef struct _FATX_BOOT_SECTOR
{
   unsigned char SysType[4];        // 0
   unsigned long VolumeID;          // 4
   unsigned long SectorsPerCluster; // 8
   unsigned short FATCount;         // 12
   unsigned long Unknown;           // 14
   unsigned char Unused[4078];      // 18
} FATX_BOOT_SECTOR, *PFATX_BOOT_SECTOR;
#include <poppack.h>

typedef struct _FORMAT_CONTEXT
{
  PFMIFSCALLBACK Callback;
  ULONG TotalSectorCount;
  ULONG CurrentSectorCount;
  BOOLEAN Success;
  ULONG Percent;
} FORMAT_CONTEXT, *PFORMAT_CONTEXT;



NTSTATUS
FatxFormat (HANDLE FileHandle,
	    PPARTITION_INFORMATION PartitionInfo,
	    PDISK_GEOMETRY DiskGeometry,
	    BOOLEAN QuickFormat,
	    PFORMAT_CONTEXT Context);

VOID
VfatxUpdateProgress (PFORMAT_CONTEXT Context,
		     ULONG Increment);

#endif /* _VFATXLIB_H_ */

/* EOF */
