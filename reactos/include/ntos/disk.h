/* $Id: disk.h,v 1.13 2003/06/22 16:33:44 ekohl Exp $
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

#define IOCTL_DISK_BASE                   FILE_DEVICE_DISK

#define IOCTL_DISK_GET_DRIVE_GEOMETRY     CTL_CODE(IOCTL_DISK_BASE, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_GET_PARTITION_INFO     CTL_CODE(IOCTL_DISK_BASE, 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_SET_PARTITION_INFO     CTL_CODE(IOCTL_DISK_BASE, 0x0002, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_GET_DRIVE_LAYOUT       CTL_CODE(IOCTL_DISK_BASE, 0x0003, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_SET_DRIVE_LAYOUT       CTL_CODE(IOCTL_DISK_BASE, 0x0004, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_VERIFY                 CTL_CODE(IOCTL_DISK_BASE, 0x0005, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_FORMAT_TRACKS          CTL_CODE(IOCTL_DISK_BASE, 0x0006, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_REASSIGN_BLOCKS        CTL_CODE(IOCTL_DISK_BASE, 0x0007, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_PERFORMANCE            CTL_CODE(IOCTL_DISK_BASE, 0x0008, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_IS_WRITABLE            CTL_CODE(IOCTL_DISK_BASE, 0x0009, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_LOGGING                CTL_CODE(IOCTL_DISK_BASE, 0x000A, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_FORMAT_TRACKS_EX       CTL_CODE(IOCTL_DISK_BASE, 0x000B, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_HISTOGRAM_STRUCTURE    CTL_CODE(IOCTL_DISK_BASE, 0x000C, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_HISTOGRAM_DATA         CTL_CODE(IOCTL_DISK_BASE, 0x000D, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_HISTOGRAM_RESET        CTL_CODE(IOCTL_DISK_BASE, 0x000E, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_REQUEST_STRUCTURE      CTL_CODE(IOCTL_DISK_BASE, 0x000F, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_REQUEST_DATA           CTL_CODE(IOCTL_DISK_BASE, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_CONTRIOLLER_NUMBER     CTL_CODE(IOCTL_DISK_BASE, 0x0011, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define SMART_GET_VERSION                 CTL_CODE(IOCTL_DISK_BASE, 0x0020, METHOD_BUFFERED, FILE_READ_ACCESS)
#define SMART_SEND_DRIVE_COMMAND          CTL_CODE(IOCTL_DISK_BASE, 0x0021, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define SMART_RCV_DRIVE_DATA              CTL_CODE(IOCTL_DISK_BASE, 0x0022, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_DISK_INTERNAL_SET_VERIFY    CTL_CODE(IOCTL_DISK_BASE, 0x0100, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_CLEAR_VERIFY  CTL_CODE(IOCTL_DISK_BASE, 0x0101, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_DISK_CHECK_VERIFY           CTL_CODE(IOCTL_DISK_BASE, 0x0200, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_MEDIA_REMOVAL          CTL_CODE(IOCTL_DISK_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_EJECT_MEDIA            CTL_CODE(IOCTL_DISK_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_LOAD_MEDIA             CTL_CODE(IOCTL_DISK_BASE, 0x0203, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_RESERVE                CTL_CODE(IOCTL_DISK_BASE, 0x0204, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_RELEASE                CTL_CODE(IOCTL_DISK_BASE, 0x0205, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_FIND_NEW_DEVICES       CTL_CODE(IOCTL_DISK_BASE, 0x0206, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_GET_MEDIA_TYPES        CTL_CODE(IOCTL_DISK_BASE, 0x0300, METHOD_BUFFERED, FILE_ANY_ACCESS)


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

#if 0
#define PTEmpty                         0x00
#define PTDOS3xPrimary                  0x01
#define PTXENIXRoot                     0x02
#define PTXENIXUsr                      0x03
#define PTOLDDOS16Bit                   0x04
#define PTDosExtended                   0x05
#define PTDos5xPrimary                  0x06
#define PTIfs                           0x07	// e.g.: HPFS, NTFS, etc
#define PTAIX                           0x08
#define PTAIXBootable                   0x09
#define PTOS2BootMgr                    0x0A
#define PTWin95FAT32                    0x0B
#define PTWin95FAT32LBA                 0x0C
#define PTWin95FAT16LBA                 0x0E
#define PTWin95ExtendedLBA              0x0F
#define PTVenix286                      0x40
#define PTNovell                        0x51
#define PTMicroport                     0x52
#define PTGnuHurd                       0x63
#define PTNetware286                    0x64
#define PTNetware386                    0x65
#define PTPCIX                          0x75
#define PTOldMinix                      0x80
#define PTMinix                         0x81
#define PTLinuxSwap                     0x82
#define PTLinuxExt2                     0x83
#define PTAmoeba                        0x93
#define PTAmoebaBBT                     0x94
#define PTBSD                           0xA5
#define PTBSDIFS                        0xB7
#define PTBSDISwap                      0xB8
#define PTSyrinx                        0xC7
#define PTCPM                           0xDB
#define PTDOSAccess                     0xE1
#define PTDOSRO                         0xE3
#define PTDOSSecondary                  0xF2
#define PTBBT                           0xFF
#endif

#define IsRecognizedPartition(P)  \
    ((P) == PARTITION_FAT_12       || \
     (P) == PARTITION_FAT_16       || \
     (P) == PARTITION_HUGE         || \
     (P) == PARTITION_IFS          || \
     (P) == PARTITION_FAT32        || \
     (P) == PARTITION_FAT32_XINT13 || \
     (P) == PARTITION_XINT13)

#define IsContainerPartition(P)  \
    ((P) == PARTITION_EXTENDED || \
     (P) == PARTITION_XINT13_EXTENDED)


typedef enum _MEDIA_TYPE
{
  Unknown,
  F5_1Pt2_512,
  F3_1Pt44_512,
  F3_2Pt88_512,
  F3_20Pt8_512,
  F3_720_512,
  F5_360_512,
  F5_320_512,
  F5_320_1024,
  F5_180_512,
  F5_160_512,
  RemovableMedia,
  FixedMedia
} MEDIA_TYPE;

typedef struct _PARTITION_INFORMATION
{
  LARGE_INTEGER StartingOffset;
  LARGE_INTEGER PartitionLength;
  DWORD HiddenSectors;
  DWORD PartitionNumber;
  BYTE PartitionType;
  BOOLEAN BootIndicator;
  BOOLEAN RecognizedPartition;
  BOOLEAN RewritePartition;
} PARTITION_INFORMATION, *PPARTITION_INFORMATION;

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

#ifndef __USE_W32API

typedef struct _DRIVE_LAYOUT_INFORMATION
{
  DWORD PartitionCount;
  DWORD Signature;
  PARTITION_INFORMATION PartitionEntry[1];
} DRIVE_LAYOUT_INFORMATION, *PDRIVE_LAYOUT_INFORMATION;

#endif /* !__USE_W32API */

#endif /* __INCLUDE_DISK_H */

/* EOF */
