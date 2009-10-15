/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/fsctl.c
 * PURPOSE:         Filesystem control
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
FatUserFsCtrl(PFAT_IRP_CONTEXT IrpContext, PIRP Irp)
{
    DPRINT1("FatUserFsCtrl()\n");
    FatCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS
NTAPI
FatVerifyVolume(PFAT_IRP_CONTEXT IrpContext, PIRP Irp)
{
    DPRINT1("FatVerifyVolume()\n");
    FatCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
    return STATUS_INVALID_DEVICE_REQUEST;
}

VOID
NTAPI
FatiCleanVcbs(PFAT_IRP_CONTEXT IrpContext)
{
    /* Make sure this IRP is waitable */
    ASSERT(IrpContext->Flags & IRPCONTEXT_CANWAIT);

    /* Acquire global resource */
    ExAcquireResourceExclusiveLite(&FatGlobalData.Resource, TRUE);

    /* TODO: Go through all VCBs and delete unmounted ones */

    /* Release global resource */
    ExReleaseResourceLite(&FatGlobalData.Resource);
}

VOID
NTAPI
FatiUnpackBpb(PBIOS_PARAMETER_BLOCK Bpb, PPACKED_BIOS_PARAMETER_BLOCK PackedBpb)
{
    CopyUchar2(&Bpb->BytesPerSector,     &PackedBpb->BytesPerSector[0]);
    CopyUchar1(&Bpb->SectorsPerCluster,  &PackedBpb->SectorsPerCluster[0]);
    CopyUchar2(&Bpb->ReservedSectors,    &PackedBpb->ReservedSectors[0]);
    CopyUchar1(&Bpb->Fats,               &PackedBpb->Fats[0]);
    CopyUchar2(&Bpb->RootEntries,        &PackedBpb->RootEntries[0]);
    CopyUchar2(&Bpb->Sectors,            &PackedBpb->Sectors[0]);
    CopyUchar1(&Bpb->Media,              &PackedBpb->Media[0]);
    CopyUchar2(&Bpb->SectorsPerFat,      &PackedBpb->SectorsPerFat[0]);
    CopyUchar2(&Bpb->SectorsPerTrack,    &PackedBpb->SectorsPerTrack[0]);
    CopyUchar2(&Bpb->Heads,              &PackedBpb->Heads[0]);
    CopyUchar4(&Bpb->HiddenSectors,      &PackedBpb->HiddenSectors[0]);
    CopyUchar4(&Bpb->LargeSectors,       &PackedBpb->LargeSectors[0]);
    CopyUchar4(&Bpb->LargeSectorsPerFat, &((PPACKED_BIOS_PARAMETER_BLOCK_EX)PackedBpb)->LargeSectorsPerFat[0]);
    CopyUchar2(&Bpb->ExtendedFlags,      &((PPACKED_BIOS_PARAMETER_BLOCK_EX)PackedBpb)->ExtendedFlags[0]);
    CopyUchar2(&Bpb->FsVersion,          &((PPACKED_BIOS_PARAMETER_BLOCK_EX)PackedBpb)->FsVersion[0]);
    CopyUchar4(&Bpb->RootDirFirstCluster,&((PPACKED_BIOS_PARAMETER_BLOCK_EX)PackedBpb)->RootDirFirstCluster[0]);
    CopyUchar2(&Bpb->FsInfoSector,       &((PPACKED_BIOS_PARAMETER_BLOCK_EX)PackedBpb)->FsInfoSector[0]);
    CopyUchar2(&Bpb->BackupBootSector,   &((PPACKED_BIOS_PARAMETER_BLOCK_EX)PackedBpb)->BackupBootSector[0]);
}

BOOLEAN
NTAPI
FatiBpbFat32(PPACKED_BIOS_PARAMETER_BLOCK PackedBpb)
{
    return (*(USHORT *)(&PackedBpb->SectorsPerFat) == 0);
}

NTSTATUS
NTAPI
FatMountVolume(PFAT_IRP_CONTEXT IrpContext,
               PDEVICE_OBJECT TargetDeviceObject,
               PVPB Vpb,
               PDEVICE_OBJECT FsDeviceObject)
{
    NTSTATUS Status;
    DISK_GEOMETRY DiskGeometry;
    ULONG MediaChangeCount = 0;
    PVOLUME_DEVICE_OBJECT VolumeDevice;
    VCB *Vcb;
    FF_ERROR Error;
    PBCB BootBcb;
    PPACKED_BOOT_SECTOR BootSector;

    DPRINT1("FatMountVolume()\n");

    /* Make sure this IRP is waitable */
    ASSERT(IrpContext->Flags & IRPCONTEXT_CANWAIT);

    /* Request media changes count, mostly usefull for removable devices */
    Status = FatPerformDevIoCtrl(TargetDeviceObject,
                                 IOCTL_STORAGE_CHECK_VERIFY,
                                 NULL,
                                 0,
                                 &MediaChangeCount,
                                 sizeof(ULONG),
                                 TRUE);

    if (!NT_SUCCESS(Status)) return Status;

    /* TODO: Check if data-track present in case of a CD drive */
    /* TODO: IOCTL_DISK_GET_PARTITION_INFO_EX */

    /* Remove unmounted VCBs */
    FatiCleanVcbs(IrpContext);

    /* Acquire the global exclusive lock */
    FatAcquireExclusiveGlobal(IrpContext);

    /* Create a new volume device object */
    Status = IoCreateDevice(FatGlobalData.DriverObject,
                            sizeof(VOLUME_DEVICE_OBJECT) - sizeof(DEVICE_OBJECT),
                            NULL,
                            FILE_DEVICE_DISK_FILE_SYSTEM,
                            0,
                            FALSE,
                            (PDEVICE_OBJECT *)&VolumeDevice);

    if (!NT_SUCCESS(Status))
    {
        /* Release the global lock */
        FatReleaseGlobal(IrpContext);

        return Status;
    }

    /* Match alignment requirements */
    if (TargetDeviceObject->AlignmentRequirement > VolumeDevice->DeviceObject.AlignmentRequirement)
    {
        VolumeDevice->DeviceObject.AlignmentRequirement = TargetDeviceObject->AlignmentRequirement;
    }

    /* Init stack size */
    VolumeDevice->DeviceObject.StackSize = TargetDeviceObject->StackSize + 1;

    /* Get sector size */
    Status = FatPerformDevIoCtrl(TargetDeviceObject,
                                 IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                 NULL,
                                 0,
                                 &DiskGeometry,
                                 sizeof(DISK_GEOMETRY),
                                 TRUE);

    if (!NT_SUCCESS(Status)) goto FatMountVolumeCleanup;

    VolumeDevice->DeviceObject.SectorSize = (USHORT) DiskGeometry.BytesPerSector;

    /* Signal we're done with initializing */
    VolumeDevice->DeviceObject.Flags &= ~DO_DEVICE_INITIALIZING;

    /* Save device object in a VPB */
    Vpb->DeviceObject = (PDEVICE_OBJECT)VolumeDevice;

    /* Initialize VCB for this volume */
    Status = FatInitializeVcb(IrpContext, &VolumeDevice->Vcb, TargetDeviceObject, Vpb);
    if (!NT_SUCCESS(Status)) goto FatMountVolumeCleanup;

    Vcb = &VolumeDevice->Vcb;

    /* Initialize FullFAT library */
    Vcb->Ioman = FF_CreateIOMAN(NULL,
                                8192,
                                VolumeDevice->DeviceObject.SectorSize,
                                &Error);

    ASSERT(Vcb->Ioman);

    /* Register block device read/write functions */
    Error = FF_RegisterBlkDevice(Vcb->Ioman,
                                 VolumeDevice->DeviceObject.SectorSize,
                                 (FF_WRITE_BLOCKS)FatWriteBlocks,
                                 (FF_READ_BLOCKS)FatReadBlocks,
                                 Vcb);

    if (Error)
    {
        DPRINT1("Registering block device with FullFAT failed with error %d\n", Error);
        FF_DestroyIOMAN(Vcb->Ioman);
        goto FatMountVolumeCleanup;
    }

    /* Mount the volume using FullFAT */
    if(FF_MountPartition(Vcb->Ioman, 0))
    {
        DPRINT1("Partition mounting failed\n");
        FF_DestroyIOMAN(Vcb->Ioman);
        goto FatMountVolumeCleanup;
    }

    /* Read the boot sector */
    FatReadStreamFile(Vcb, 0, sizeof(PACKED_BOOT_SECTOR), &BootBcb, (PVOID)&BootSector);

    /* Check if it's successful */
    if (!BootBcb)
    {
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto FatMountVolumeCleanup;
    }

    /* Unpack data */
    FatiUnpackBpb(&Vcb->Bpb, &BootSector->PackedBpb);

    /* Verify if sector size matches */
    if (DiskGeometry.BytesPerSector != Vcb->Bpb.BytesPerSector)
    {
        DPRINT1("Disk geometry BPS %d and bios BPS %d don't match!\n",
            DiskGeometry.BytesPerSector, Vcb->Bpb.BytesPerSector);

        /* Fail */
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto FatMountVolumeCleanup;
    }

    /* If Sectors value is set, discard the LargeSectors value */
    if (Vcb->Bpb.Sectors) Vcb->Bpb.LargeSectors = 0;

    /* Copy serial number */
    if (FatiBpbFat32(&BootSector->PackedBpb))
    {
        CopyUchar4(&Vpb->SerialNumber, ((PPACKED_BOOT_SECTOR_EX)BootSector)->Id);
    }
    else
    {
        /* This is FAT12/16 */
        CopyUchar4(&Vpb->SerialNumber, BootSector->Id);
    }

    /* Unpin the BCB */
    CcUnpinData(BootBcb);

    /* Create root DCB for it */
    FatCreateRootDcb(IrpContext, &VolumeDevice->Vcb);

    /* Keep trace of media changes */
    VolumeDevice->Vcb.MediaChangeCount = MediaChangeCount;

    //ObDereferenceObject(TargetDeviceObject);

    /* Release the global lock */
    FatReleaseGlobal(IrpContext);

    /* Notify about volume mount */
    //FsRtlNotifyVolumeEvent(VolumeDevice->Vcb.StreamFileObject, FSRTL_VOLUME_MOUNT);

    /* Return success */
    return STATUS_SUCCESS;


FatMountVolumeCleanup:

    /* Unwind the routine actions */
    IoDeleteDevice((PDEVICE_OBJECT)VolumeDevice);

    /* Release the global lock */
    FatReleaseGlobal(IrpContext);

    return Status;
}



NTSTATUS
NTAPI
FatiFileSystemControl(PFAT_IRP_CONTEXT IrpContext, PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    /* Get current IRP stack location */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Dispatch depending on the minor function */
    switch (IrpSp->MinorFunction)
    {
    case IRP_MN_USER_FS_REQUEST:
        Status = FatUserFsCtrl(IrpContext, Irp);
        break;

    case IRP_MN_MOUNT_VOLUME:
        Status = FatMountVolume(IrpContext,
                                IrpSp->Parameters.MountVolume.DeviceObject,
                                IrpSp->Parameters.MountVolume.Vpb,
                                IrpSp->DeviceObject);

        FatCompleteRequest(IrpContext, Irp, Status);

        break;

    case IRP_MN_VERIFY_VOLUME:
        Status = FatVerifyVolume(IrpContext, Irp);
        break;

    default:
        DPRINT1("Unhandled FSCTL minor 0x%x\n", IrpSp->MinorFunction);
        FatCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    return Status;
}


NTSTATUS
NTAPI
FatFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFAT_IRP_CONTEXT IrpContext;
    BOOLEAN CanWait = TRUE;

    DPRINT("FatFileSystemControl(DeviceObject %p, Irp %p)\n", DeviceObject, Irp);

    /* Get CanWait flag */
    if (IoGetCurrentIrpStackLocation(Irp)->FileObject)
    {
        CanWait = IoIsOperationSynchronous(Irp);
    }

    /* Enter FsRtl critical region */
    FsRtlEnterFileSystem();

    /* Build an irp context */
    IrpContext = FatBuildIrpContext(Irp, CanWait);

    /* Call internal function */
    Status = FatiFileSystemControl(IrpContext, Irp);

    /* Leave FsRtl critical region */
    FsRtlExitFileSystem();

    return Status;
}
