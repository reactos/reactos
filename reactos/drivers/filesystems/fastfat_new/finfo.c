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

VOID
NTAPI
FatiQueryStandardInformation(IN PFAT_IRP_CONTEXT IrpContext,
                             IN PFCB Fcb,
                             IN PFILE_OBJECT FileObject,
                             IN OUT PFILE_STANDARD_INFORMATION Buffer,
                             IN OUT PLONG Length)
{
    /* Zero the buffer */
    RtlZeroMemory(Buffer, sizeof(FILE_STANDARD_INFORMATION));

    /* Deduct the written length */
    *Length -= sizeof(FILE_STANDARD_INFORMATION);

    Buffer->NumberOfLinks = 1;
    Buffer->DeletePending = FALSE; // FIXME

    /* Check if it's a dir or a file */
    if (FatNodeType(Fcb) == FAT_NTC_FCB)
    {
        Buffer->Directory = FALSE;

        Buffer->EndOfFile.LowPart = Fcb->FatHandle->Filesize;
        Buffer->AllocationSize = Buffer->EndOfFile;
        DPRINT1("Filesize %d, chain length %d\n", Fcb->FatHandle->Filesize, Fcb->FatHandle->iChainLength);
    }
    else
    {
        Buffer->Directory = TRUE;
    }
}

VOID
NTAPI
FatiQueryInternalInformation(IN PFAT_IRP_CONTEXT IrpContext,
                             IN PFCB Fcb,
                             IN PFILE_OBJECT FileObject,
                             IN OUT PFILE_INTERNAL_INFORMATION Buffer,
                             IN OUT PLONG Length)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
FatiQueryNameInformation(IN PFAT_IRP_CONTEXT IrpContext,
                         IN PFCB Fcb,
                         IN PFILE_OBJECT FileObject,
                         IN OUT PFILE_NAME_INFORMATION Buffer,
                         IN OUT PLONG Length)
{
    ULONG ByteSize;
    ULONG Trim = 0;
    BOOLEAN Overflow = FALSE;

    /* Deduct the minimum written length */
    *Length -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]);

    /* Build full name if needed */
    if (!Fcb->FullFileName.Buffer)
    {
        FatSetFullFileNameInFcb(IrpContext, Fcb);
    }

    DPRINT1("FullFileName %wZ\n", &Fcb->FullFileName);

    if (*Length < Fcb->FullFileName.Length - Trim)
    {
        /* Buffer can't fit all data */
        ByteSize = *Length;
        Overflow = TRUE;
    }
    else
    {
        /* Deduct the amount of bytes we are going to write */
        ByteSize = Fcb->FullFileName.Length - Trim;
        *Length -= ByteSize;
    }

    /* Copy the name */
    RtlCopyMemory(Buffer->FileName,
                  Fcb->FullFileName.Buffer,
                  ByteSize);

    /* Set the length */
    Buffer->FileNameLength = Fcb->FullFileName.Length - Trim;

    /* Is this a shortname query? */
    if (Trim)
    {
        /* Yes, not supported atm */
        ASSERT(FALSE);
    }

    /* Indicate overflow by passing -1 as the length */
    if (Overflow) *Length = -1;
}

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
    LONG Length;
    PVOID Buffer;
    BOOLEAN VcbLocked = FALSE, FcbLocked = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Get IRP stack location */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Get the file object */
    FileObject = IrpSp->FileObject;

    /* Copy variables to something with shorter names */
    InfoClass = IrpSp->Parameters.QueryFile.FileInformationClass;
    Length = IrpSp->Parameters.QueryFile.Length;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    DPRINT1("FatiQueryInformation\n", 0);
    DPRINT1("\tIrp                  = %08lx\n", Irp);
    DPRINT1("\tLength               = %08lx\n", Length);
    DPRINT1("\tFileInformationClass = %08lx\n", InfoClass);
    DPRINT1("\tBuffer               = %08lx\n", Buffer);

    FileType = FatDecodeFileObject(FileObject, &Vcb, &Fcb, &Ccb);

    DPRINT1("Vcb %p, Fcb %p, Ccb %p, open type %d\n", Vcb, Fcb, Ccb, FileType);

    /* Acquire VCB lock */
    if (InfoClass == FileNameInformation ||
        InfoClass == FileAllInformation)
    {
        if (!FatAcquireExclusiveVcb(IrpContext, Vcb))
        {
            ASSERT(FALSE);
        }

        /* Remember we locked the VCB */
        VcbLocked = TRUE;
    }

    /* Acquire FCB lock */
    // FIXME: If not paging file
    if (!FatAcquireSharedFcb(IrpContext, Fcb))
    {
        ASSERT(FALSE);
    }
    FcbLocked = TRUE;

    switch (InfoClass)
    {
    case FileStandardInformation:
        FatiQueryStandardInformation(IrpContext, Fcb, FileObject, Buffer, &Length);
        break;
    case FileInternalInformation:
        FatiQueryInternalInformation(IrpContext, Fcb, FileObject, Buffer, &Length);
        break;
    case FileNameInformation:
        FatiQueryNameInformation(IrpContext, Fcb, FileObject, Buffer, &Length);
        break;
    default:
        DPRINT1("Unimplemented information class %d requested\n", InfoClass);
        Status = STATUS_INVALID_PARAMETER;
    }

    /* Check for buffer overflow */
    if (Length < 0)
    {
        Status = STATUS_BUFFER_OVERFLOW;
        Length = 0;
    }

    /* Set IoStatus.Information to amount of filled bytes */
    Irp->IoStatus.Information = IrpSp->Parameters.QueryFile.Length - Length;

    /* Release FCB locks */
    if (FcbLocked) FatReleaseFcb(IrpContext, Fcb);
    if (VcbLocked) FatReleaseVcb(IrpContext, Vcb);

    /* Complete request and return status */
    FatCompleteRequest(IrpContext, Irp, Status);
    return Status;
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
