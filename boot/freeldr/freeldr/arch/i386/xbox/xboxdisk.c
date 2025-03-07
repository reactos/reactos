/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Xbox specific disk access routines
 * COPYRIGHT:   Copyright 2004 Gé van Geldorp (gvg@reactos.com)
 *              Copyright 2019 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <hwide.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(DISK);

/* GLOBALS ********************************************************************/

static PDEVICE_UNIT HardDrive = NULL;
static PDEVICE_UNIT CdDrive = NULL;
static BOOLEAN AtaInitialized = FALSE;

/* FUNCTIONS ******************************************************************/

VOID
XboxDiskInit(BOOLEAN Init)
{
    UCHAR DetectedCount;
    UCHAR UnitNumber;
    PDEVICE_UNIT DeviceUnit = NULL;

    if (Init & !AtaInitialized)
    {
        /* Find first HDD and CD */
        AtaInit(&DetectedCount);
        for (UnitNumber = 0; UnitNumber <= DetectedCount; UnitNumber++)
        {
            DeviceUnit = AtaGetDevice(UnitNumber);
            if (DeviceUnit)
            {
                if (DeviceUnit->Flags & ATA_DEVICE_ATAPI)
                {
                    if (!CdDrive)
                        CdDrive = DeviceUnit;
                }
                else
                {
                    if (!HardDrive)
                        HardDrive = DeviceUnit;
                }
            }
        }
        AtaInitialized = TRUE;
    }
    else
    {
        AtaFree();
    }
}

static inline
PDEVICE_UNIT
XboxDiskDriveNumberToDeviceUnit(UCHAR DriveNumber)
{
    /* Xbox has only 1 IDE controller and no floppy */
    if (DriveNumber < 0x80 || (DriveNumber & 0x0F) >= 2)
        return NULL;

    if (!AtaInitialized)
        XboxDiskInit(TRUE);

    /* HDD */
    if ((DriveNumber == 0x80) && HardDrive)
        return HardDrive;

    /* CD */
    if (((DriveNumber & 0xF0) > 0x80) && CdDrive)
        return CdDrive;

    return NULL;
}

BOOLEAN
XboxDiskReadLogicalSectors(
    IN UCHAR DriveNumber,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer)
{
    PDEVICE_UNIT DeviceUnit;

    TRACE("XboxDiskReadLogicalSectors() DriveNumber: 0x%x SectorNumber: %I64d SectorCount: %d Buffer: 0x%x\n",
          DriveNumber, SectorNumber, SectorCount, Buffer);

    DeviceUnit = XboxDiskDriveNumberToDeviceUnit(DriveNumber);
    if (!DeviceUnit)
        return FALSE;

    return AtaAtapiReadLogicalSectorsLBA(DeviceUnit, SectorNumber, SectorCount, Buffer);
}

BOOLEAN
XboxDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY Geometry)
{
    PDEVICE_UNIT DeviceUnit;

    TRACE("XboxDiskGetDriveGeometry(0x%x)\n", DriveNumber);

    DeviceUnit = XboxDiskDriveNumberToDeviceUnit(DriveNumber);
    if (!DeviceUnit)
        return FALSE;

    Geometry->Cylinders = DeviceUnit->Cylinders;
    Geometry->Heads = DeviceUnit->Heads;
    Geometry->SectorsPerTrack = DeviceUnit->Sectors;
    Geometry->BytesPerSector = DeviceUnit->SectorSize;
    Geometry->Sectors = DeviceUnit->TotalSectors;

    return TRUE;
}

ULONG
XboxDiskGetCacheableBlockCount(UCHAR DriveNumber)
{
    PDEVICE_UNIT DeviceUnit;

    DeviceUnit = XboxDiskDriveNumberToDeviceUnit(DriveNumber);
    if (!DeviceUnit)
        return 1; // Unknown count.

    /*
     * If LBA is supported then the block size will be 64 sectors (32k).
     * If not then the block size is the size of one track.
     */
    if (DeviceUnit->Flags & ATA_DEVICE_LBA)
        return 64;
    else
        return DeviceUnit->Sectors;
}

/* EOF */
