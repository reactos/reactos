/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        vfatxlib.h
 */
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <fmifs/fmifs.h>

typedef struct _FATX_BOOT_SECTOR
{
   unsigned char SysType[4];        // 0
   unsigned long VolumeID;          // 4
   unsigned long SectorsPerCluster; // 8
   unsigned short FATCount;         // 12
   unsigned long Unknown;           // 14
   unsigned char Unused[4078];      // 18
} __attribute__((packed)) FATX_BOOT_SECTOR, *PFATX_BOOT_SECTOR;


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

/* EOF */
