/* $Id$
 *
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          hal/halx86/xbox/part_xbox.c
 * PURPOSE:       Xbox specific handling of partition tables
 * PROGRAMMER:    Ge van Geldorp (gvg@reactos.com)
 * UPDATE HISTORY:
 *             2004/12/04: Created
 */

/* INCLUDES *****************************************************************/

#include "halxbox.h"

#define NDEBUG
#include <debug.h>

#define XBOX_SIGNATURE_SECTOR 3
#define XBOX_SIGNATURE        ('B' | ('R' << 8) | ('F' << 16) | ('R' << 24))
#define PARTITION_SIGNATURE   0xaa55

/* VARIABLES ***************************************************************/

static pHalExamineMBR NtoskrnlExamineMBR;
static pHalIoReadPartitionTable NtoskrnlIoReadPartitionTable;
static pHalIoSetPartitionInformation NtoskrnlIoSetPartitionInformation;
static pHalIoWritePartitionTable NtoskrnlIoWritePartitionTable;

static struct
{
    ULONG SectorStart;
    ULONG SectorCount;
    CHAR PartitionType;
} XboxPartitions[] =
{
    /* This is in the \Device\Harddisk0\Partition.. order used by the Xbox kernel */
    { 0x0055F400, 0x0098f800, PARTITION_FAT32  }, /* Store, E: */
    { 0x00465400, 0x000FA000, PARTITION_FAT_16 }, /* System, C: */
    { 0x00000400, 0x00177000, PARTITION_FAT_16 }, /* Cache1, X: */
    { 0x00177400, 0x00177000, PARTITION_FAT_16 }, /* Cache2, Y: */
    { 0x002EE400, 0x00177000, PARTITION_FAT_16 }  /* Cache3, Z: */
};

#define XBOX_PARTITION_COUNT (sizeof(XboxPartitions) / sizeof(XboxPartitions[0]))

/* FUNCTIONS ***************************************************************/


static NTSTATUS
HalpXboxReadSector(IN PDEVICE_OBJECT DeviceObject,
                   IN ULONG SectorSize,
                   IN PLARGE_INTEGER SectorOffset,
                   OUT PVOID Sector)
{
    IO_STATUS_BLOCK StatusBlock;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;

    DPRINT("HalpXboxReadSector(%p %lu 0x%08x%08x %p)\n",
           DeviceObject, SectorSize, SectorOffset->u.HighPart, SectorOffset->u.LowPart, Sector);

    ASSERT(DeviceObject);
    ASSERT(Sector);

    KeInitializeEvent(&Event,
                      NotificationEvent,
                      FALSE);

    /* Read the sector */
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       DeviceObject,
                                       Sector,
                                       SectorSize,
                                       SectorOffset,
                                       &Event,
                                       &StatusBlock);

    Status = IoCallDriver(DeviceObject,
                          Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = StatusBlock.Status;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Reading sector failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    return Status;
}

static NTSTATUS FASTCALL
HalpXboxDeviceHasXboxPartitioning(IN PDEVICE_OBJECT DeviceObject,
                                  IN ULONG SectorSize,
                                  OUT BOOLEAN *HasXboxPartitioning)
{
    PVOID SectorData;
    LARGE_INTEGER Offset;
    NTSTATUS Status;

    DPRINT("HalpXboxDeviceHasXboxPartitioning(%p %lu %p)\n",
           DeviceObject,
           SectorSize,
           HasXboxPartitioning);

    SectorData = ExAllocatePool(PagedPool, SectorSize);
    if (!SectorData)
    {
        return STATUS_NO_MEMORY;
    }

    Offset.QuadPart = XBOX_SIGNATURE_SECTOR * SectorSize;
    Status = HalpXboxReadSector(DeviceObject, SectorSize, &Offset, SectorData);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    DPRINT("Signature 0x%02x 0x%02x 0x%02x 0x%02x\n",
           *((UCHAR *) SectorData), *((UCHAR *) SectorData + 1), *((UCHAR *) SectorData + 2), *((UCHAR *) SectorData + 3));
    *HasXboxPartitioning = (XBOX_SIGNATURE == *((ULONG *) SectorData));
    ExFreePool(SectorData);
    DPRINT("%s partitioning found\n", *HasXboxPartitioning ? "Xbox" : "MBR");

    return STATUS_SUCCESS;
}

static VOID FASTCALL
HalpXboxExamineMBR(IN PDEVICE_OBJECT DeviceObject,
                   IN ULONG SectorSize,
                   IN ULONG MBRTypeIdentifier,
                   OUT PVOID *Buffer)
{
    BOOLEAN HasXboxPartitioning;
    NTSTATUS Status;

    DPRINT("HalpXboxExamineMBR(%p %lu %lx %p)\n",
           DeviceObject,
           SectorSize,
           MBRTypeIdentifier,
           Buffer);

    *Buffer = NULL;

    Status = HalpXboxDeviceHasXboxPartitioning(DeviceObject, SectorSize, &HasXboxPartitioning);
    if (! NT_SUCCESS(Status))
    {
        return;
    }

    if (! HasXboxPartitioning)
    {
        DPRINT("Delegating to standard MBR code\n");
        NtoskrnlExamineMBR(DeviceObject, SectorSize, MBRTypeIdentifier, Buffer);
        return;
    }

    /* Buffer already set to NULL */
    return;
}

static NTSTATUS FASTCALL
HalpXboxIoReadPartitionTable(IN PDEVICE_OBJECT DeviceObject,
                             IN ULONG SectorSize,
                             IN BOOLEAN ReturnRecognizedPartitions,
                             OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer)
{
    BOOLEAN HasXboxPartitioning;
    NTSTATUS Status;
    ULONG Part;
    PPARTITION_INFORMATION PartInfo;

    DPRINT("HalpXboxIoReadPartitionTable(%p %lu %x %p)\n",
           DeviceObject,
           SectorSize,
           ReturnRecognizedPartitions,
           PartitionBuffer);

    Status = HalpXboxDeviceHasXboxPartitioning(DeviceObject, SectorSize, &HasXboxPartitioning);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    if (! HasXboxPartitioning)
    {
        DPRINT("Delegating to standard MBR code\n");
        return NtoskrnlIoReadPartitionTable(DeviceObject, SectorSize,
                                            ReturnRecognizedPartitions, PartitionBuffer);
    }

    *PartitionBuffer = (PDRIVE_LAYOUT_INFORMATION)
                       ExAllocatePool(PagedPool,
                                      sizeof(DRIVE_LAYOUT_INFORMATION) +
                                      XBOX_PARTITION_COUNT * sizeof(PARTITION_INFORMATION));
    if (NULL == *PartitionBuffer)
    {
        return STATUS_NO_MEMORY;
    }
    (*PartitionBuffer)->PartitionCount = XBOX_PARTITION_COUNT;
    (*PartitionBuffer)->Signature = PARTITION_SIGNATURE;
    for (Part = 0; Part < XBOX_PARTITION_COUNT; Part++)
    {
        PartInfo = (*PartitionBuffer)->PartitionEntry + Part;
        PartInfo->StartingOffset.QuadPart = (ULONGLONG) XboxPartitions[Part].SectorStart *
                                            (ULONGLONG) SectorSize;
        PartInfo->PartitionLength.QuadPart = (ULONGLONG) XboxPartitions[Part].SectorCount *
                                             (ULONGLONG) SectorSize;
        PartInfo->HiddenSectors = 0;
        PartInfo->PartitionNumber = Part + 1;
        PartInfo->PartitionType = XboxPartitions[Part].PartitionType;
        PartInfo->BootIndicator = FALSE;
        PartInfo->RecognizedPartition = TRUE;
        PartInfo->RewritePartition = FALSE;
        DPRINT(" %ld: nr: %d boot: %1x type: %x start: 0x%I64x count: 0x%I64x rec: %d\n",
               Part,
               PartInfo->PartitionNumber,
               PartInfo->BootIndicator,
               PartInfo->PartitionType,
               PartInfo->StartingOffset.QuadPart,
               PartInfo->PartitionLength.QuadPart,
               PartInfo->RecognizedPartition);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS FASTCALL
HalpXboxIoSetPartitionInformation(IN PDEVICE_OBJECT DeviceObject,
                                  IN ULONG SectorSize,
                                  IN ULONG PartitionNumber,
                                  IN ULONG PartitionType)
{
    BOOLEAN HasXboxPartitioning;
    NTSTATUS Status;

    DPRINT("HalpXboxIoSetPartitionInformation(%p %lu %lu %lu)\n",
           DeviceObject,
           SectorSize,
           PartitionNumber,
           PartitionType);

    Status = HalpXboxDeviceHasXboxPartitioning(DeviceObject, SectorSize, &HasXboxPartitioning);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    if (!HasXboxPartitioning)
    {
        DPRINT("Delegating to standard MBR code\n");
        return NtoskrnlIoSetPartitionInformation(DeviceObject, SectorSize,
                                                 PartitionNumber, PartitionType);
    }

    /* Can't change the partitioning */
    DPRINT1("Xbox partitions are fixed, can't change them\n");
    return STATUS_ACCESS_DENIED;
}

static NTSTATUS FASTCALL
HalpXboxIoWritePartitionTable(IN PDEVICE_OBJECT DeviceObject,
                              IN ULONG SectorSize,
                              IN ULONG SectorsPerTrack,
                              IN ULONG NumberOfHeads,
                              IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer)
{
    BOOLEAN HasXboxPartitioning;
    NTSTATUS Status;

    DPRINT("HalpXboxIoWritePartitionTable(%p %lu %lu %lu %p)\n",
           DeviceObject,
           SectorSize,
           SectorsPerTrack,
           NumberOfHeads,
           PartitionBuffer);

    Status = HalpXboxDeviceHasXboxPartitioning(DeviceObject, SectorSize, &HasXboxPartitioning);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    if (!HasXboxPartitioning)
    {
        DPRINT("Delegating to standard MBR code\n");
        return NtoskrnlIoWritePartitionTable(DeviceObject, SectorSize,
                                             SectorsPerTrack, NumberOfHeads,
                                             PartitionBuffer);
    }

    /* Can't change the partitioning */
    DPRINT1("Xbox partitions are fixed, can't change them\n");
    return STATUS_ACCESS_DENIED;
}

#define HalExamineMBR HALDISPATCH->HalExamineMBR

void
HalpXboxInitPartIo(void)
{
    NtoskrnlExamineMBR = HalExamineMBR;
    HalExamineMBR = HalpXboxExamineMBR;
    NtoskrnlIoReadPartitionTable = HalIoReadPartitionTable;
    HalIoReadPartitionTable = HalpXboxIoReadPartitionTable;
    NtoskrnlIoSetPartitionInformation = HalIoSetPartitionInformation;
    HalIoSetPartitionInformation = HalpXboxIoSetPartitionInformation;
    NtoskrnlIoWritePartitionTable = HalIoWritePartitionTable;
    HalIoWritePartitionTable = HalpXboxIoWritePartitionTable;
}

/* EOF */
