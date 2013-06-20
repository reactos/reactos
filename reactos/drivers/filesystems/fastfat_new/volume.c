/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GNU GPLv3 as published by the Free Software Foundation
 * FILE:            drivers/filesystems/fastfat/volume.c
 * PURPOSE:         Volume information
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
FatiQueryFsVolumeInfo(PVCB Vcb,
                      PFILE_FS_VOLUME_INFORMATION Buffer,
                      PLONG Length)
{
    ULONG ByteSize;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Deduct the minimum written length */
    *Length -= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel[0]);

    /* Zero it */
    RtlZeroMemory(Buffer, sizeof(FILE_FS_VOLUME_INFORMATION));

    DPRINT("Serial number 0x%x, label length %d\n",
        Vcb->Vpb->SerialNumber, Vcb->Vpb->VolumeLabelLength);

    /* Save serial number */
    Buffer->VolumeSerialNumber = Vcb->Vpb->SerialNumber;

    /* Set max byte size */
    ByteSize = Vcb->Vpb->VolumeLabelLength;

    /* Check buffer length and reduce byte size if needed */
    if (*Length < Vcb->Vpb->VolumeLabelLength)
    {
        /* Copy only up to what buffer size was provided */
        ByteSize = *Length;
        Status = STATUS_BUFFER_OVERFLOW;
    }

    /* Copy volume label */
    Buffer->VolumeLabelLength = Vcb->Vpb->VolumeLabelLength;
    RtlCopyMemory(Buffer->VolumeLabel, Vcb->Vpb->VolumeLabel, ByteSize);
    *Length -= ByteSize;

    return Status;
}

NTSTATUS
NTAPI
FatiQueryFsSizeInfo(PVCB Vcb,
                    PFILE_FS_SIZE_INFORMATION Buffer,
                    PLONG Length)
{
    FF_PARTITION *Partition;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Deduct the minimum written length */
    *Length -= sizeof(FILE_FS_SIZE_INFORMATION);

    /* Zero it */
    RtlZeroMemory(Buffer, sizeof(FILE_FS_SIZE_INFORMATION));

    /* Reference FullFAT's partition */
    Partition = Vcb->Ioman->pPartition;

    /* Set values */
    Buffer->AvailableAllocationUnits.LowPart = Partition->FreeClusterCount;
    Buffer->TotalAllocationUnits.LowPart = Partition->NumClusters;
    Buffer->SectorsPerAllocationUnit = Vcb->Bpb.SectorsPerCluster;
    Buffer->BytesPerSector = Vcb->Bpb.BytesPerSector;

    DPRINT1("Total %d, free %d, SPC %d, BPS %d\n", Partition->NumClusters,
        Partition->FreeClusterCount, Vcb->Bpb.SectorsPerCluster, Vcb->Bpb.BytesPerSector);

    return Status;
}

NTSTATUS
NTAPI
FatiQueryFsDeviceInfo(PVCB Vcb,
                      PFILE_FS_DEVICE_INFORMATION Buffer,
                      PLONG Length)
{
    /* Deduct the minimum written length */
    *Length -= sizeof(FILE_FS_DEVICE_INFORMATION);

    /* Zero it */
    RtlZeroMemory(Buffer, sizeof(FILE_FS_DEVICE_INFORMATION));

    /* Set values */
    Buffer->DeviceType = FILE_DEVICE_DISK;
    Buffer->Characteristics = Vcb->TargetDeviceObject->Characteristics;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
FatiQueryVolumeInfo(PFAT_IRP_CONTEXT IrpContext, PIRP Irp)
{
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION IrpSp;
    FILE_INFORMATION_CLASS InfoClass;
    TYPE_OF_OPEN FileType;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;
    LONG Length;
    PVOID Buffer;
    BOOLEAN VcbLocked = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Get IRP stack location */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Get the file object */
    FileObject = IrpSp->FileObject;

    /* Copy variables to something with shorter names */
    InfoClass = IrpSp->Parameters.QueryVolume.FsInformationClass;
    Length = IrpSp->Parameters.QueryVolume.Length;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    DPRINT("FatiQueryVolumeInfo\n", 0);
    DPRINT("\tIrp                  = %08lx\n", Irp);
    DPRINT("\tLength               = %08lx\n", Length);
    DPRINT("\tFsInformationClass   = %08lx\n", InfoClass);
    DPRINT("\tBuffer               = %08lx\n", Buffer);

    FileType = FatDecodeFileObject(FileObject, &Vcb, &Fcb, &Ccb);

    DPRINT("Vcb %p, Fcb %p, Ccb %p, open type %d\n", Vcb, Fcb, Ccb, FileType);

    switch (InfoClass)
    {
    case FileFsVolumeInformation:
        /* Acquired the shared VCB lock */
        if (!FatAcquireSharedVcb(IrpContext, Vcb))
        {
            ASSERT(FALSE);
        }

        /* Remember we locked it */
        VcbLocked = TRUE;

        /* Call FsVolumeInfo handler */
        Status = FatiQueryFsVolumeInfo(Vcb, Buffer, &Length);
        break;

    case FileFsSizeInformation:
        /* Call FsVolumeInfo handler */
        Status = FatiQueryFsSizeInfo(Vcb, Buffer, &Length);
        break;

    case FileFsDeviceInformation:
        Status = FatiQueryFsDeviceInfo(Vcb, Buffer, &Length);
        break;

    case FileFsAttributeInformation:
        UNIMPLEMENTED;
        //Status = FatiQueryFsAttributeInfo(IrpContext, Vcb, Buffer, &Length);
        break;

    case FileFsFullSizeInformation:
        UNIMPLEMENTED;
        //Status = FatiQueryFsFullSizeInfo(IrpContext, Vcb, Buffer, &Length);
        break;

    default:
        Status = STATUS_INVALID_PARAMETER;
    }

    /* Set IoStatus.Information to amount of filled bytes */
    Irp->IoStatus.Information = IrpSp->Parameters.QueryVolume.Length - Length;

    /* Release VCB lock */
    if (VcbLocked) FatReleaseVcb(IrpContext, Vcb);

    /* Complete request and return status */
    FatCompleteRequest(IrpContext, Irp, Status);
    return Status;
}

NTSTATUS
NTAPI
FatQueryVolumeInfo(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS Status;
    BOOLEAN TopLevel, CanWait;
    PFAT_IRP_CONTEXT IrpContext;

    CanWait = TRUE;
    TopLevel = FALSE;
    Status = STATUS_INVALID_DEVICE_REQUEST;

    /* Get CanWait flag */
    if (IoGetCurrentIrpStackLocation(Irp)->FileObject != NULL)
        CanWait = IoIsOperationSynchronous(Irp);

    /* Enter FsRtl critical region */
    FsRtlEnterFileSystem();

    /* Set Top Level IRP if not set */
    TopLevel = FatIsTopLevelIrp(Irp);

    /* Build an irp context */
    IrpContext = FatBuildIrpContext(Irp, CanWait);

    /* Call the request handler */
    Status = FatiQueryVolumeInfo(IrpContext, Irp);

    /* Restore top level Irp */
    if (TopLevel)
        IoSetTopLevelIrp(NULL);

    /* Leave FsRtl critical region */
    FsRtlExitFileSystem();

    return Status;
}

NTSTATUS
NTAPI
FatSetVolumeInfo(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT1("FatSetVolumeInfo()\n");
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
FatReadStreamFile(PVCB Vcb,
                  ULONGLONG ByteOffset,
                  ULONG ByteSize,
                  PBCB *Bcb,
                  PVOID *Buffer)
{
    LARGE_INTEGER Offset;

    Offset.QuadPart = ByteOffset;

    if (!CcMapData(Vcb->StreamFileObject,
                   &Offset,
                   ByteSize,
                   TRUE, // FIXME: CanWait
                   Bcb,
                   Buffer))
    {
        ASSERT(FALSE);
    }
}

BOOLEAN
NTAPI
FatCheckForDismount(IN PFAT_IRP_CONTEXT IrpContext,
                    PVCB Vcb,
                    IN BOOLEAN Force)
{
    /* We never allow deletion of a volume for now */
    return FALSE;
}

/* EOF */
