/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Disk devices helpers
 * COPYRIGHT:   Copyright 2025-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(DISK);

#define FIRST_PARTITION 1

/* DISK IO ERROR SUPPORT *****************************************************/

static LONG lReportError = 0; // >= 0: display errors; < 0: hide errors.

LONG
DiskReportError(
    _In_ BOOLEAN bShowError)
{
    /* Set the reference count */
    if (bShowError) ++lReportError;
    else            --lReportError;
    return lReportError;
}

VOID
DiskError(
    _In_ PCSTR ErrorString,
    _In_ ULONG ErrorCode)
{
    PCSTR ErrorDescription;
    CHAR ErrorCodeString[200];

    if (lReportError < 0)
        return;

    ErrorDescription = DiskGetErrorCodeString(ErrorCode);
    if (ErrorDescription)
    {
        RtlStringCbPrintfA(ErrorCodeString, sizeof(ErrorCodeString),
                           "%s\n\nError Code: 0x%lx\nError: %s",
                           ErrorString, ErrorCode, ErrorDescription);
    }
    else
    {
        RtlStringCbCopyA(ErrorCodeString, sizeof(ErrorCodeString), ErrorString);
    }

    ERR("%s\n", ErrorCodeString);
    UiMessageBox(ErrorCodeString);
}


/* FUNCTIONS *****************************************************************/

ARC_STATUS
DiskInitialize(
    _In_ UCHAR DriveNumber, // FIXME: Arch-specific
    _In_ PCSTR DeviceName,
    _In_ CONFIGURATION_TYPE DeviceType,
    _In_ const DEVVTBL* FuncTable,
    _Out_opt_ PULONG pChecksum,
    _Out_opt_ PULONG pSignature,
    _Out_opt_ PBOOLEAN pValidPartitionTable)
{
    PMASTER_BOOT_RECORD Mbr;
    PULONG Buffer;
    ULONGLONG SectorStart;
    ULONG SectorSize;
    ULONG i;
    ULONG Checksum, Signature;
    BOOLEAN ValidPartitionTable;
    BOOLEAN IsCdRom;
    PARTITION_TABLE_ENTRY PartitionTableEntry;
    CHAR ArcName[MAX_PATH];
    NTSTATUS NtStatus;

    IsCdRom = (DeviceType == CdromController);
    if (IsCdRom)
    {
        SectorStart = 16ULL;
        SectorSize = 2048;
    }
    else // (DeviceType == FloppyDiskPeripheral || DiskPeripheral)
    {
        SectorStart = 0ULL;
        SectorSize = 512;
    }

    /* Read the MBR */
    if (!MachDiskReadLogicalSectors(DriveNumber, SectorStart, 1, DiskReadBuffer))
    {
        ERR("Reading MBR failed\n");
        return EIO;
    }

    Buffer = (PULONG)DiskReadBuffer;
    Mbr = (PMASTER_BOOT_RECORD)DiskReadBuffer;

    Signature = Mbr->Signature;
    TRACE("Signature: %x\n", Signature);

    /* Calculate the MBR checksum */
    Checksum = 0;
    for (i = 0; i < SectorSize / sizeof(ULONG); i++)
    {
        Checksum += Buffer[i];
    }
    Checksum = ~Checksum + 1;
    TRACE("Checksum: %x\n", Checksum);

    ValidPartitionTable = (IsCdRom || (Mbr->MasterBootRecordMagic == 0xAA55));
    TRACE("IsPartitionValid: %s\n", ValidPartitionTable ? "TRUE" : "FALSE");

    /* Register the device */
    // NOTE: For now, only register the direct device if it's not a
    // "rigid" disk, i.e. peripherals not of the form 'xxx(y)rdisk(z)'.
    // Rigid disks are registered with the 'partition(0)' suffix below.
    if (DeviceType != DiskPeripheral)
    {
        if (!FsRegisterDevice(DeviceName, FuncTable))
            return ENOMEM;
    }

    /* Fill out the ARC disk block */
    AddReactOSArcDiskInfo(DeviceName, Signature, Checksum, ValidPartitionTable);

    if (pChecksum)
        *pChecksum = Checksum;
    if (pSignature)
        *pSignature = Signature;
    if (pValidPartitionTable)
        *pValidPartitionTable = ValidPartitionTable;

    /* Don't search for partitions in non-"rigid" disks (floppies, CD-ROMs) */
    if (DeviceType != DiskPeripheral)
        return ESUCCESS;

    /* Register the device with the 'partition(0)' suffix */
    NtStatus = RtlStringCbPrintfA(ArcName, sizeof(ArcName), "%spartition(0)", DeviceName);
    if (!NT_SUCCESS(NtStatus))
        return ENAMETOOLONG;
    if (!FsRegisterDevice(ArcName, FuncTable))
        return ENOMEM;

    /* Detect disk partition type */
    DiskDetectPartitionType(DriveNumber);

    /* Add partitions */
    i = FIRST_PARTITION;
    while (DiskGetPartitionEntry(DriveNumber, i, &PartitionTableEntry))
    {
        if (PartitionTableEntry.SystemIndicator != PARTITION_ENTRY_UNUSED)
        {
            NtStatus = RtlStringCbPrintfA(ArcName, sizeof(ArcName),
                                          "%spartition(%lu)", DeviceName, i);
            if (!NT_SUCCESS(NtStatus))
                return ENAMETOOLONG;
            if (!FsRegisterDevice(ArcName, FuncTable))
                return ENOMEM;
        }
        i++;
    }

    return ESUCCESS;
}
