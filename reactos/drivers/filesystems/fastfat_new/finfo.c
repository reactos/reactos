/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/finfo.c
 * PURPOSE:         File Information support routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
FatiQueryInformation(IN PFAT_IRP_CONTEXT IrpContext,
                     IN PIRP Irp)
{
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION IrpSp;
    FILE_INFORMATION_CLASS InfoClass;
    TYPE_OF_OPEN FileType;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    /* Get IRP stack location */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Get the file object */
    FileObject = IrpSp->FileObject;

    InfoClass = IrpSp->Parameters.QueryFile.FileInformationClass;

    DPRINT1("FatCommonQueryInformation\n", 0);
    DPRINT1("\tIrp                  = %08lx\n", Irp);
    DPRINT1("\tLength               = %08lx\n", IrpSp->Parameters.QueryFile.Length);
    DPRINT1("\tFileInformationClass = %08lx\n", InfoClass);
    DPRINT1("\tBuffer               = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    FileType = FatDecodeFileObject(FileObject, &Vcb, &Fcb, &Ccb);

    DPRINT1("Vcb %p, Fcb %p, Ccb %p, open type %d\n", Vcb, Fcb, Ccb, FileType);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
FatQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp)
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
    if (IoGetTopLevelIrp() == NULL)
    {
        IoSetTopLevelIrp(Irp);
        TopLevel = TRUE;
    }

    /* Build an irp context */
    IrpContext = FatBuildIrpContext(Irp, CanWait);

    /* Perform the actual read */
    Status = FatiQueryInformation(IrpContext, Irp);

    /* Restore top level Irp */
    if (TopLevel) IoSetTopLevelIrp(NULL);

    /* Leave FsRtl critical region */
    FsRtlExitFileSystem();

    return Status;
}

NTSTATUS
NTAPI
FatSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT1("FatSetInformation()\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
