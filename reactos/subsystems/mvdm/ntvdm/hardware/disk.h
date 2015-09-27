/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/hardware/disk.h
 * PURPOSE:         Generic Disk Controller (Floppy, Hard Disk, ...)
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _DISK_H_
#define _DISK_H_

/* DEFINES ********************************************************************/

// https://msdn.microsoft.com/en-us/library/windows/desktop/aa363972(v=vs.85).aspx
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa363976(v=vs.85).aspx
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa363969(v=vs.85).aspx
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365231(v=vs.85).aspx

typedef struct _DISK_INFO
{
    WORD Cylinders;             // DWORD
    BYTE Heads;                 // DWORD
    BYTE Sectors;               // QWORD
    // SectorPerTrack; ???      // DWORD
    WORD SectorSize;
} DISK_INFO, *PDISK_INFO;

typedef struct _DISK_IMAGE
{
    DISK_INFO DiskInfo;
    BYTE DiskType; // Type to return from BIOS & CMOS

    BYTE LastOperationStatus;
    // CurrentPos;

    HANDLE hDisk;
    BOOLEAN ReadOnly;
    // WCHAR ImageFile[MAX_PATH];

} DISK_IMAGE, *PDISK_IMAGE;

typedef enum _DISK_TYPE
{
    FLOPPY_DISK,
    HARD_DISK,
    MAX_DISK_TYPE
} DISK_TYPE;

/* FUNCTIONS ******************************************************************/

BOOLEAN
IsDiskPresent(IN PDISK_IMAGE DiskImage);

BYTE
SeekDisk(IN PDISK_IMAGE DiskImage,
         IN WORD Cylinder,
         IN BYTE Head,
         IN BYTE Sector);

BYTE
ReadDisk(IN PDISK_IMAGE DiskImage,
         IN WORD Cylinder,
         IN BYTE Head,
         IN BYTE Sector,
         IN BYTE NumSectors);

BYTE
WriteDisk(IN PDISK_IMAGE DiskImage,
          IN WORD Cylinder,
          IN BYTE Head,
          IN BYTE Sector,
          IN BYTE NumSectors);

PDISK_IMAGE
RetrieveDisk(IN DISK_TYPE DiskType,
             IN ULONG DiskNumber);

BOOLEAN
MountDisk(IN DISK_TYPE DiskType,
          IN ULONG DiskNumber,
          IN PCSTR FileName,
          IN BOOLEAN ReadOnly);

BOOLEAN
UnmountDisk(IN DISK_TYPE DiskType,
            IN ULONG DiskNumber);

BOOLEAN DiskCtrlInitialize(VOID);
VOID DiskCtrlCleanup(VOID);

#endif // _DISK_H_

/* EOF */
