/* $Id: disk.h,v 1.10 2002/09/07 15:12:20 chorns Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/disk.h
 * PURPOSE:      Disk related definitions used by all the parts of the system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * UPDATE HISTORY: 
 *               27/06/00: Created
 */

#ifndef __INCLUDE_DISK_H
#define __INCLUDE_DISK_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#include <ddk/ntdddisk.h>

#define IOCTL_DISK_LOGGING              CTL_CODE(FILE_DEVICE_DISK, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_HISTOGRAM_STRUCTURE  CTL_CODE(FILE_DEVICE_DISK, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_HISTOGRAM_DATA       CTL_CODE(FILE_DEVICE_DISK, 13, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_HISTOGRAM_RESET      CTL_CODE(FILE_DEVICE_DISK, 14, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_REQUEST_STRUCTURE    CTL_CODE(FILE_DEVICE_DISK, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_REQUEST_DATA         CTL_CODE(FILE_DEVICE_DISK, 16, METHOD_BUFFERED, FILE_ANY_ACCESS)


#define PARTITION_ENTRY_UNUSED          0x00
#define PARTITION_FAT_12                0x01
#define PARTITION_XENIX_1               0x02
#define PARTITION_XENIX_2               0x03
#define PARTITION_FAT_16                0x04
#define PARTITION_EXTENDED              0x05
#define PARTITION_HUGE                  0x06
#define PARTITION_IFS                   0x07
#define PARTITION_FAT32                 0x0B
#define PARTITION_FAT32_XINT13          0x0C
#define PARTITION_XINT13                0x0E
#define PARTITION_XINT13_EXTENDED       0x0F
#define PARTITION_PREP                  0x41
#define PARTITION_LDM                   0x42
#define PARTITION_UNIX                  0x63

#define PARTITION_NTFT                  0x80
#define VALID_NTFT                      0xC0

typedef struct _DRIVE_LAYOUT_INFORMATION {
	DWORD PartitionCount;
	DWORD Signature;
	PARTITION_INFORMATION PartitionEntry[1];
} DRIVE_LAYOUT_INFORMATION, *PDRIVE_LAYOUT_INFORMATION;

#if 0
typedef struct _SET_PARTITION_INFORMATION
{
  ULONG PartitionType;
} SET_PARTITION_INFORMATION, *PSET_PARTITION_INFORMATION;

typedef struct _DISK_GEOMETRY
{
  LARGE_INTEGER Cylinders;
  MEDIA_TYPE MediaType;
  DWORD TracksPerCylinder;
  DWORD SectorsPerTrack;
  DWORD BytesPerSector;
} DISK_GEOMETRY, *PDISK_GEOMETRY;
#endif

#endif /* __INCLUDE_DISK_H */

/* EOF */
