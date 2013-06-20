/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GNU GPLv3 as published by the Free Software Foundation
 * FILE:            drivers/filesystems/fastfat/fat.c
 * PURPOSE:         FAT support routines
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* PROTOTYPES ***************************************************************/

BOOLEAN
NTAPI
FatValidBpb(IN PBIOS_PARAMETER_BLOCK Bpb);

/* VARIABLES ****************************************************************/

BOOLEAN
NTAPI
FatValidBpb(IN PBIOS_PARAMETER_BLOCK Bpb)
{
    return (FatValidBytesPerSector(Bpb->BytesPerSector)
        && FatValidSectorsPerCluster(Bpb->SectorsPerCluster)
        && Bpb->ReservedSectors > 0
        && Bpb->Fats > 0
        && (Bpb->Sectors > 0 || Bpb->LargeSectors > 0)
        && (Bpb->SectorsPerFat > 0
            || (Bpb->LargeSectorsPerFat > 0 && Bpb->FsVersion == 0))
        && (Bpb->Media == 0xf0
            || Bpb->Media == 0xf8
            || Bpb->Media == 0xf9
            || Bpb->Media == 0xfb
            || Bpb->Media == 0xfc
            || Bpb->Media == 0xfd
            || Bpb->Media == 0xfe
            || Bpb->Media == 0xff)
        && (Bpb->SectorsPerFat == 0 || Bpb->RootEntries > 0)
        && (Bpb->SectorsPerFat > 0 || !Bpb->MirrorDisabled));
}

/**
 * Determines the index of the set bit.
 *
 * @param Number
 * Number having a single bit set.
 *
 * @return
 * Index of the set bit.
 */
FORCEINLINE
ULONG
FatPowerOfTwo(
    ULONG Number)
{
    ULONG Temp;
    Temp = Number
        - ((Number >> 1) & 033333333333)
        - ((Number >> 2) & 011111111111);
    return (((Temp + (Temp >> 3)) & 030707070707) % 63);
}

VOID
NTAPI
FatiInitializeVcb(PVCB Vcb)
{
    ULONG ClustersCapacity;

    /* Various characteristics needed for navigation in FAT */
    if ((Vcb->Sectors = Vcb->Bpb.Sectors) == 0)
        Vcb->Sectors = Vcb->Bpb.LargeSectors;
    if ((Vcb->SectorsPerFat = Vcb->Bpb.SectorsPerFat) == 0)
        Vcb->SectorsPerFat = Vcb->Bpb.LargeSectorsPerFat;
    Vcb->RootDirent = Vcb->Bpb.ReservedSectors + Vcb->Bpb.Fats * Vcb->SectorsPerFat;
    Vcb->RootDirentSectors = BytesToSectors(Vcb,
        Vcb->Bpb.RootEntries * sizeof(DIR_ENTRY));
    Vcb->DataArea = Vcb->RootDirent + Vcb->RootDirentSectors;
    Vcb->Clusters = (Vcb->Sectors - Vcb->Bpb.ReservedSectors
        - Vcb->Bpb.Fats * Vcb->SectorsPerFat
        - Vcb->RootDirentSectors) / Vcb->Bpb.SectorsPerCluster;
    if (Vcb->BytesPerClusterLog < 4087)
    {
        Vcb->IndexDepth = 0x0c;
        //Vcb->Methods = Fat12Methods;
    }
    else
    {
        Vcb->IndexDepth = 0x10;
        //Vcb->Methods = Fat16Methods;
    }
    /* Large Sectors are used for FAT32 */
    if (Vcb->Bpb.Sectors == 0) {
        Vcb->IndexDepth = 0x20;
        //Vcb->Methods = Fat32Methods;
    }
    ClustersCapacity = (SectorsToBytes(Vcb, Vcb->Sectors) * 0x8 / Vcb->IndexDepth) - 1;
    if (Vcb->Clusters > ClustersCapacity)
        Vcb->Clusters = ClustersCapacity;
    Vcb->BytesPerCluster = SectorsToBytes(Vcb, Vcb->Bpb.SectorsPerCluster);
    Vcb->BytesPerClusterLog = FatPowerOfTwo(Vcb->BytesPerCluster);
    Vcb->BeyondLastClusterInFat = ((LONGLONG) Vcb->Clusters) * Vcb->IndexDepth / 0x8;

    /* Update real volume size with the real value. */
    Vcb->Header.FileSize.QuadPart =
    Vcb->Header.AllocationSize.QuadPart = SectorsToBytes(Vcb, Vcb->Sectors);
}

NTSTATUS
NTAPI
FatInitializeVcb(IN PFAT_IRP_CONTEXT IrpContext,
                 IN PVCB Vcb,
                 IN PDEVICE_OBJECT TargetDeviceObject,
                 IN PVPB Vpb)
{
    NTSTATUS Status;
    PBCB Bcb;
    PVOID Buffer;
    LARGE_INTEGER Offset;

    RtlZeroMemory(Vcb, sizeof(*Vcb));

    /* Initialize list head, so that it will
     * not fail in cleanup.
     */
    InitializeListHead(&Vcb->VcbLinks);

    /* Setup FCB Header */
    Vcb->Header.NodeTypeCode = FAT_NTC_VCB;
    Vcb->Header.NodeByteSize = sizeof(*Vcb);

    /* Setup Vcb fields */
    Vcb->TargetDeviceObject = TargetDeviceObject;
    ObReferenceObject(TargetDeviceObject);
    Vcb->Vpb = Vpb;

    /* Setup FCB Header */
    ExInitializeFastMutex(&Vcb->HeaderMutex);
    FsRtlSetupAdvancedHeader(&Vcb->Header, &Vcb->HeaderMutex);

    /* Create Volume File Object */
    Vcb->StreamFileObject = IoCreateStreamFileObject(NULL,
                                                     Vcb->TargetDeviceObject);

    /* We have to setup all FCB fields needed for CC */
    Vcb->StreamFileObject->FsContext = Vcb;
    Vcb->StreamFileObject->SectionObjectPointer = &Vcb->SectionObjectPointers;

    /* At least full boot sector should be available */
    //Vcb->Header.FileSize.QuadPart = sizeof(PACKED_BOOT_SECTOR);
    //Vcb->Header.AllocationSize.QuadPart = sizeof(PACKED_BOOT_SECTOR);
    Vcb->Header.ValidDataLength.HighPart = MAXLONG;
    Vcb->Header.ValidDataLength.LowPart = MAXULONG;

    Vcb->Header.AllocationSize.QuadPart = Int32x32To64(5*1024, 1024*1024); //HACK: 5 Gb
    Vcb->Header.FileSize.QuadPart = Vcb->Header.AllocationSize.QuadPart;

    /* Set VCB to a good condition */
    Vcb->Condition = VcbGood;

    /* Initialize VCB's resource */
    ExInitializeResourceLite(&Vcb->Resource);

    /* Initialize close queue lists */
    InitializeListHead(&Vcb->AsyncCloseList);
    InitializeListHead(&Vcb->DelayedCloseList);

    /* Initialize CC */
    CcInitializeCacheMap(Vcb->StreamFileObject,
                         (PCC_FILE_SIZES)&Vcb->Header.AllocationSize,
                         FALSE,
                         &FatGlobalData.CacheMgrNoopCallbacks,
                         Vcb);

    /* Read boot sector */
    Offset.QuadPart = 0;
    Bcb = NULL;

    /* Note: Volume Read path does not require
     * any of the parameters set further
     * in this routine.
     */
    if (CcMapData(Vcb->StreamFileObject,
                  &Offset,
                  sizeof(PACKED_BOOT_SECTOR),
                  TRUE,
                  &Bcb,
                  &Buffer))
    {
        PPACKED_BOOT_SECTOR BootSector = (PPACKED_BOOT_SECTOR) Buffer;
        FatUnpackBios(&Vcb->Bpb, &BootSector->PackedBpb);
        if (!(FatBootSectorJumpValid(BootSector->Jump) &&
            FatValidBpb(&Vcb->Bpb)))
        {
            Status = STATUS_UNRECOGNIZED_VOLUME;
        }
        CopyUchar4(&Vcb->Vpb->SerialNumber, BootSector->Id);
        CcUnpinData(Bcb);
    }
    else
    {
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto FatInitializeVcbCleanup;
    }

    /* Increase internal / residual open counter */
    InterlockedIncrement((PLONG)&(Vcb->InternalOpenCount));
    InterlockedIncrement((PLONG)&(Vcb->ResidualOpenCount));

    /* Set up notifications */
    FsRtlNotifyInitializeSync(&Vcb->NotifySync);
    InitializeListHead(&Vcb->NotifyList);

    /* Call helper function */
    FatiInitializeVcb(Vcb);

    /* Add this Vcb to global Vcb list */
    (VOID)FatAcquireExclusiveGlobal(IrpContext);
    InsertTailList(&FatGlobalData.VcbListHead, &Vcb->VcbLinks);
    FatReleaseGlobal(IrpContext);

    return STATUS_SUCCESS;

FatInitializeVcbCleanup:

    /* Unwind the routine actions */
    FatUninitializeVcb(Vcb);
    return Status;
}

VOID
NTAPI
FatUninitializeVcb(IN PVCB Vcb)
{
    LARGE_INTEGER ZeroSize;

    ZeroSize.QuadPart = 0LL;

    /* Close volume file */
    if (Vcb->StreamFileObject != NULL)
    {
        /* Uninitialize CC. */
        CcUninitializeCacheMap(Vcb->StreamFileObject, &ZeroSize, NULL);
        ObDereferenceObject(Vcb->StreamFileObject);
        Vcb->StreamFileObject = NULL;
    }

    /* Free ContextClose if it's not freed up already */
    if (Vcb->CloseContext) ExFreePool(Vcb->CloseContext);

    /* Free notifications stuff */
    FsRtlNotifyUninitializeSync(&Vcb->NotifySync);

    /* Unlink from global Vcb list. */
    RemoveEntryList(&Vcb->VcbLinks);

    /* Release Target Device */
    ObDereferenceObject(Vcb->TargetDeviceObject);
    Vcb->TargetDeviceObject = NULL;
}

/* EOF */


