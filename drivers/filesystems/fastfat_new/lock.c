/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GNU GPLv3 as published by the Free Software Foundation
 * FILE:            drivers/filesystems/fastfat/lock.c
 * PURPOSE:         Lock support routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
FatiLockControl(PFAT_IRP_CONTEXT IrpContext, PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;
    NTSTATUS Status;

    /* Get IRP stack location */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Determine type of open */
    TypeOfOpen = FatDecodeFileObject(IrpSp->FileObject, &Vcb, &Fcb, &Ccb);

    /* Only user file open is allowed */
    if (TypeOfOpen != UserFileOpen)
    {
        FatCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    /* Acquire shared FCB lock */
    if (!FatAcquireSharedFcb(IrpContext, Fcb))
    {
        UNIMPLEMENTED;
        //Status = FatFsdPostRequest(IrpContext, Irp);
        Status = STATUS_NOT_IMPLEMENTED;
        return Status;
    }

    /* Check oplock state */
    Status = FsRtlCheckOplock(&Fcb->Fcb.Oplock,
                              Irp,
                              IrpContext,
                              FatOplockComplete,
                              NULL);

    if (Status != STATUS_SUCCESS)
    {
        /* Release FCB lock */
        FatReleaseFcb(IrpContext, Fcb);

        return Status;
    }

    /* Process the lock */
    Status = FsRtlProcessFileLock(&Fcb->Fcb.Lock, Irp, NULL);

    /* Update Fast I/O state */
    Fcb->Header.IsFastIoPossible = FatIsFastIoPossible(Fcb);

    /* Complete the request */
    FatCompleteRequest(IrpContext, NULL, 0);

    /* Release FCB lock */
    FatReleaseFcb(IrpContext, Fcb);

    return Status;
}

NTSTATUS
NTAPI
FatLockControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PFAT_IRP_CONTEXT IrpContext;
    NTSTATUS Status;
    BOOLEAN TopLevel;

    DPRINT1("FatLockControl()\n");

    /* Enter FsRtl critical region */
    FsRtlEnterFileSystem();

    /* Set Top Level IRP if not set */
    TopLevel = FatIsTopLevelIrp(Irp);

    /* Build an irp context */
    IrpContext = FatBuildIrpContext(Irp, IoIsOperationSynchronous(Irp));

    /* Call internal function */
    Status = FatiLockControl(IrpContext, Irp);

    /* Reset Top Level IRP */
    if (TopLevel) IoSetTopLevelIrp(NULL);

    /* Leave FsRtl critical region */
    FsRtlExitFileSystem();

    return Status;
}

VOID
NTAPI
FatOplockComplete(IN PVOID Context,
                  IN PIRP Irp)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
FatPrePostIrp(IN PVOID Context,
              IN PIRP Irp)
{
    UNIMPLEMENTED;
}

/* EOF */
