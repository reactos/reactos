/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Xbox specific disk access routines
 * COPYRIGHT:   Copyright 2004 Gé van Geldorp (gvg@reactos.com)
 *              Copyright 2019-2025 Dmitry Borisov (di.sean@protonmail.com)
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

/* DISK IO ERROR SUPPORT *****************************************************/

static LONG lReportError = 0; // >= 0: display errors; < 0: hide errors.

LONG DiskReportError(BOOLEAN bShowError)
{
    /* Set the reference count */
    if (bShowError) ++lReportError;
    else            --lReportError;
    return lReportError;
}

#if 0 // TODO: ATA/IDE error code descriptions.
static PCSTR DiskGetErrorCodeString(ULONG ErrorCode)
{
    switch (ErrorCode)
    {
    default: return "unknown error code";
    }
}
#endif

static VOID DiskError(PCSTR ErrorString, ULONG ErrorCode)
{
    CHAR ErrorCodeString[200];

    if (lReportError < 0)
        return;

#if 0 // TODO: ATA/IDE error code descriptions.
    sprintf(ErrorCodeString, "%s\n\nError Code: 0x%lx\nError: %s",
            ErrorString, ErrorCode, DiskGetErrorCodeString(ErrorCode));
#else
    UNREFERENCED_PARAMETER(ErrorCode);
    sprintf(ErrorCodeString, "%s", ErrorString);
#endif

    ERR("%s\n", ErrorCodeString);
    UiMessageBox(ErrorCodeString);
}

/* FUNCTIONS ******************************************************************/

static
VOID
XboxDiskInit(VOID)
{
    UCHAR DetectedCount;
    UCHAR UnitNumber;
    PDEVICE_UNIT DeviceUnit = NULL;

    ASSERT(!AtaInitialized);

    AtaInitialized = TRUE;

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
}

static inline
PDEVICE_UNIT
XboxDiskDriveNumberToDeviceUnit(UCHAR DriveNumber)
{
    /* Xbox has only 1 IDE controller and no floppy */
    if (DriveNumber < 0x80 || (DriveNumber & 0x0F) >= 2)
        return NULL;

    if (!AtaInitialized)
        XboxDiskInit();

    /* HDD */
    if ((DriveNumber == 0x80) && HardDrive)
        return HardDrive;

    /* CD */
    if (((DriveNumber & 0xF0) > 0x80) && CdDrive)
        return CdDrive;

    return NULL;
}

BOOLEAN DiskResetController(UCHAR DriveNumber)
{
    WARN("DiskResetController(0x%x) DISK OPERATION FAILED -- RESETTING CONTROLLER\n", DriveNumber);
    /* No-op on XBOX */
    return TRUE;
}

CONFIGURATION_TYPE
DiskGetConfigType(
    _In_ UCHAR DriveNumber)
{
    PDEVICE_UNIT DeviceUnit;

    DeviceUnit = XboxDiskDriveNumberToDeviceUnit(DriveNumber);
    if (!DeviceUnit)
        return -1; // MaximumType;

    if (DeviceUnit == CdDrive) // (DeviceUnit->Flags & ATA_DEVICE_ATAPI)
        return CdromController;
    else // if (DeviceUnit == HardDrive)
        return DiskPeripheral;
}

// FIXME: Dummy for entry.S/linux.S
VOID __cdecl DiskStopFloppyMotor(VOID)
{
    /* No-op on XBOX */
}

BOOLEAN
XboxDiskReadLogicalSectors(
    IN UCHAR DriveNumber,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer)
{
    PDEVICE_UNIT DeviceUnit;
    BOOLEAN Success;

    TRACE("XboxDiskReadLogicalSectors() DriveNumber: 0x%x SectorNumber: %I64u SectorCount: %u Buffer: 0x%x\n",
          DriveNumber, SectorNumber, SectorCount, Buffer);

    DeviceUnit = XboxDiskDriveNumberToDeviceUnit(DriveNumber);
    if (!DeviceUnit)
        return FALSE;

    Success = AtaReadLogicalSectors(DeviceUnit, SectorNumber, SectorCount, Buffer);
    if (!Success)
        DiskError("Disk Read Failed", -1);
    return Success;
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
    Geometry->SectorsPerTrack = DeviceUnit->SectorsPerTrack;
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
        return DeviceUnit->SectorsPerTrack;
}

/* EOF */
