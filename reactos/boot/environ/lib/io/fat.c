/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/io/fat.c
 * PURPOSE:         Boot Library FAT File System Management Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"
#include "../drivers/filesystems/fs_rec/fs_rec.h"

/* DATA VARIABLES ************************************************************/

PVOID* FatDeviceTable;
ULONG FatDeviceTableEntries;
PWCHAR FatpLongFileName;

/* FUNCTIONS *****************************************************************/

NTSTATUS
FatMount (
    _In_ ULONG DeviceId,
    _In_ ULONG Unknown,
    _Out_ PBL_FILE_ENTRY* FileEntry
    )
{
    BL_DEVICE_INFORMATION DeviceInformation;
    ULONG UnknownFlag;
    NTSTATUS Status;
    PACKED_BOOT_SECTOR FatBootSector;
    BIOS_PARAMETER_BLOCK BiosBlock;

    EfiPrintf(L"FAT Mount on Device %d\r\n", DeviceId);

    /* Capture thing */
    BlDeviceGetInformation(DeviceId, &DeviceInformation);
    UnknownFlag = DeviceInformation.BlockDeviceInfo.Unknown;

    /* Set thing to 1 */
    DeviceInformation.BlockDeviceInfo.Unknown |= 1;
    BlDeviceSetInformation(DeviceId, &DeviceInformation);

    /* Read the boot sector */
    EfiPrintf(L"Reading fat boot sector...\r\n");
    Status = BlDeviceReadAtOffset(DeviceId,
                                  sizeof(FatBootSector),
                                  0,
                                  &FatBootSector,
                                  NULL);

    /* Restore thing back */
    DeviceInformation.BlockDeviceInfo.Unknown = UnknownFlag;
    BlDeviceSetInformation(DeviceId, &DeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Failed reading drive: %lx\r\n", Status);
        return Status;
    }

    FatUnpackBios(&BiosBlock, &FatBootSector.PackedBpb);

    EfiPrintf(L"Drive read\r\n");

    EfiPrintf(L"Jump: %lx Bytes Per Sector: %d Sectors Per Cluster: %d Reserved: %d Fats: %d Sectors: %d Large Sectors: %d Media: %lx RootEntries: %d\r\n",
        FatBootSector.Jump[0],
        BiosBlock.BytesPerSector,
        BiosBlock.SectorsPerCluster,
        BiosBlock.ReservedSectors,
        BiosBlock.Fats,
        BiosBlock.Sectors,
        BiosBlock.LargeSectors,
        BiosBlock.Media,
        BiosBlock.RootEntries);

    EfiStall(3000000);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
FatInitialize (
    VOID
    )
{
    NTSTATUS Status;

    /* Allocate the device table with 2 entries*/
    FatDeviceTableEntries = 2;
    FatDeviceTable = BlMmAllocateHeap(sizeof(PBL_FILE_ENTRY) *
                                      FatDeviceTableEntries);
    if (FatDeviceTable)
    {
        /* Zero it out */
        RtlZeroMemory(FatDeviceTable,
                      sizeof(PBL_FILE_ENTRY) * FatDeviceTableEntries);

        /* Allocate a 512 byte buffer for long file name conversion */
        FatpLongFileName = BlMmAllocateHeap(512);
        Status = FatpLongFileName != NULL ? STATUS_SUCCESS : STATUS_NO_MEMORY;
    }
    else
    {
        /* No memory, fail */
        Status = STATUS_NO_MEMORY;
    }

    /* Return back to caller */
    return Status;
}

