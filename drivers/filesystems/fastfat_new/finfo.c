/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GNU GPLv3 as published by the Free Software Foundation
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
FatiQueryBasicInformation(IN PFAT_IRP_CONTEXT IrpContext,
                          IN PFCB Fcb,
                          IN PFILE_OBJECT FileObject,
                          IN OUT PFILE_BASIC_INFORMATION Buffer,
                          IN OUT PLONG Length)
{
    /* Zero the buffer */
    RtlZeroMemory(Buffer, sizeof(FILE_BASIC_INFORMATION));

    /* Deduct the written length */
    *Length -= sizeof(FILE_BASIC_INFORMATION);

    /* Check if it's a dir or a file */
    if (FatNodeType(Fcb) == FAT_NTC_FCB)
    {
        // FIXME: Read dirent and get times from there
        Buffer->LastAccessTime.QuadPart = 0;
        Buffer->CreationTime.QuadPart = 0;
        Buffer->LastWriteTime.QuadPart = 0;
    }
    else
    {
        // FIXME: May not be really correct
        Buffer->FileAttributes = 0;
        DPRINT1("Basic info of a directory '%wZ' is requested!\n", &Fcb->FullFileName);
    }


    /* If attribute is 0, set normal */
    if (Buffer->FileAttributes == 0)
        Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
}

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
        DPRINT("Filesize %d, chain length %d\n", Fcb->FatHandle->Filesize, Fcb->FatHandle->iChainLength);
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

    DPRINT("FullFileName %wZ\n", &Fcb->FullFileName);

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

    DPRINT("FatiQueryInformation\n", 0);
    DPRINT("\tIrp                  = %08lx\n", Irp);
    DPRINT("\tLength               = %08lx\n", Length);
    DPRINT("\tFileInformationClass = %08lx\n", InfoClass);
    DPRINT("\tBuffer               = %08lx\n", Buffer);

    FileType = FatDecodeFileObject(FileObject, &Vcb, &Fcb, &Ccb);

    DPRINT("Vcb %p, Fcb %p, Ccb %p, open type %d\n", Vcb, Fcb, Ccb, FileType);

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
    case FileBasicInformation:
        FatiQueryBasicInformation(IrpContext, Fcb, FileObject, Buffer, &Length);
        break;
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
    TopLevel = FatIsTopLevelIrp(Irp);

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
FatSetEndOfFileInfo(IN PFAT_IRP_CONTEXT IrpContext,
                    IN PIRP Irp,
                    IN PFILE_OBJECT FileObject,
                    IN PVCB Vcb,
                    IN PFCB Fcb)
{
    PFILE_END_OF_FILE_INFORMATION Buffer;
    ULONG NewFileSize;
    ULONG InitialFileSize;
    ULONG InitialValidDataLength;
    //ULONG InitialValidDataToDisk;
    BOOLEAN CacheMapInitialized = FALSE;
    BOOLEAN ResourceAcquired = FALSE;
    NTSTATUS Status;

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    if (FatNodeType(Fcb) != FAT_NTC_FCB)
    {
        /* Trying to change size of a dir */
        Status = STATUS_INVALID_DEVICE_REQUEST;
        return Status;
    }

    /* Validate new size */
    if (!FatIsIoRangeValid(Buffer->EndOfFile, 0))
    {
        Status = STATUS_DISK_FULL;
        return Status;
    }

    NewFileSize = Buffer->EndOfFile.LowPart;

    /* Lookup allocation size if needed */
    if (Fcb->Header.AllocationSize.QuadPart == (LONGLONG)-1)
        UNIMPLEMENTED;//FatLookupFileAllocationSize(IrpContext, Fcb);

    /* Cache the file if there is a data section */
    if (FileObject->SectionObjectPointer->DataSectionObject &&
        (FileObject->SectionObjectPointer->SharedCacheMap == NULL) &&
        !FlagOn(Irp->Flags, IRP_PAGING_IO))
    {
        if (FlagOn(FileObject->Flags, FO_CLEANUP_COMPLETE))
        {
            /* This is a really weird condition */
            UNIMPLEMENTED;
            //Raise(STATUS_FILE_CLOSED);
        }

        /*  Initialize the cache map */
        CcInitializeCacheMap(FileObject,
                             (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                             FALSE,
                             &FatGlobalData.CacheMgrCallbacks,
                             Fcb);

        CacheMapInitialized = TRUE;
    }

    /* Lazy write case */
    if (IoGetCurrentIrpStackLocation(Irp)->Parameters.SetFile.AdvanceOnly)
    {
        if (!IsFileDeleted(Fcb) &&
            (Fcb->Condition == FcbGood))
        {
            /* Clamp the new file size */
            if (NewFileSize >= Fcb->Header.FileSize.LowPart)
                NewFileSize = Fcb->Header.FileSize.LowPart;

            ASSERT(NewFileSize <= Fcb->Header.AllocationSize.LowPart);

            /* Never reduce the file size here! */

            // TODO: Actually change file size
            DPRINT1("Actually changing file size is missing\n");

            /* Notify about file size change */
            FatNotifyReportChange(IrpContext,
                                  Vcb,
                                  Fcb,
                                  FILE_NOTIFY_CHANGE_SIZE,
                                  FILE_ACTION_MODIFIED);
        }
        else
        {
            DPRINT1("Cannot set size on deleted file\n");
        }

        Status = STATUS_SUCCESS;
        return Status;
    }

    if ( NewFileSize > Fcb->Header.AllocationSize.LowPart )
    {
        // TODO: Increase file size
        DPRINT1("Actually changing file size is missing\n");
    }

    if (Fcb->Header.FileSize.LowPart != NewFileSize)
    {
        if (NewFileSize < Fcb->Header.FileSize.LowPart)
        {
            if (!MmCanFileBeTruncated(FileObject->SectionObjectPointer,
                                      &Buffer->EndOfFile))
            {
                Status = STATUS_USER_MAPPED_FILE;

                /* Free up resources if necessary */
                if (CacheMapInitialized)
                    CcUninitializeCacheMap(FileObject, NULL, NULL);

                return Status;
            }

            ResourceAcquired = ExAcquireResourceExclusiveLite(Fcb->Header.PagingIoResource, TRUE);
        }

        /* Set new file sizes */
        InitialFileSize = Fcb->Header.FileSize.LowPart;
        InitialValidDataLength = Fcb->Header.ValidDataLength.LowPart;
        //InitialValidDataToDisk = Fcb->ValidDataToDisk;

        Fcb->Header.FileSize.LowPart = NewFileSize;

        /* Adjust valid data length if new size is less than that */
        if (Fcb->Header.ValidDataLength.LowPart > NewFileSize)
            Fcb->Header.ValidDataLength.LowPart = NewFileSize;

        //if (Fcb->ValidDataToDisk > NewFileSize)
        //    Fcb->ValidDataToDisk = NewFileSize;

        DPRINT1("New file size is 0x%08lx\n", NewFileSize);

        /* Update cache mapping */
        CcSetFileSizes(FileObject,
                       (PCC_FILE_SIZES)&Fcb->Header.AllocationSize);

        /* Notify about size change */
        FatNotifyReportChange(IrpContext,
                              Vcb,
                              Fcb,
                              FILE_NOTIFY_CHANGE_SIZE,
                              FILE_ACTION_MODIFIED);

        /* Set truncate on close flag */
        SetFlag(Fcb->State, FCB_STATE_TRUNCATE_ON_CLOSE);
    }

    /* Set modified flag */
    FileObject->Flags |= FO_FILE_MODIFIED;

    /* Free up resources if necessary */
    if (CacheMapInitialized)
        CcUninitializeCacheMap(FileObject, NULL, NULL);

    if (ResourceAcquired)
        ExReleaseResourceLite(Fcb->Header.PagingIoResource);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
FatiSetInformation(IN PFAT_IRP_CONTEXT IrpContext,
                   IN PIRP Irp)
{
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION IrpSp;
    FILE_INFORMATION_CLASS InfoClass;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;
    LONG Length;
    PVOID Buffer;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN VcbAcquired = FALSE, FcbAcquired = FALSE;

    /* Get IRP stack location */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Get the file object */
    FileObject = IrpSp->FileObject;

    /* Copy variables to something with shorter names */
    InfoClass = IrpSp->Parameters.SetFile.FileInformationClass;
    Length = IrpSp->Parameters.SetFile.Length;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    DPRINT("FatiSetInformation\n", 0);
    DPRINT("\tIrp                  = %08lx\n", Irp);
    DPRINT("\tLength               = %08lx\n", Length);
    DPRINT("\tFileInformationClass = %08lx\n", InfoClass);
    DPRINT("\tFileObject           = %08lx\n", IrpSp->Parameters.SetFile.FileObject);
    DPRINT("\tBuffer               = %08lx\n", Buffer);

    TypeOfOpen = FatDecodeFileObject(FileObject, &Vcb, &Fcb, &Ccb);

    DPRINT("Vcb %p, Fcb %p, Ccb %p, open type %d\n", Vcb, Fcb, Ccb, TypeOfOpen);

    switch (TypeOfOpen)
    {
    case UserVolumeOpen:
        Status = STATUS_INVALID_PARAMETER;
        /* Complete request and return status */
        FatCompleteRequest(IrpContext, Irp, Status);
        return Status;
    case UserFileOpen:
        /* Check oplocks */
        if (!FlagOn(Fcb->State, FCB_STATE_PAGEFILE) &&
            ((InfoClass == FileEndOfFileInformation) ||
            (InfoClass == FileAllocationInformation)))
        {
            Status = FsRtlCheckOplock(&Fcb->Fcb.Oplock,
                                      Irp,
                                      IrpContext,
                                      NULL,
                                      NULL);

            if (Status != STATUS_SUCCESS)
            {
                /* Complete request and return status */
                FatCompleteRequest(IrpContext, Irp, Status);
                return Status;
            }

            /* Update Fast IO flag */
            Fcb->Header.IsFastIoPossible = FatIsFastIoPossible(Fcb);
        }
        break;

    case UserDirectoryOpen:
        break;

    default:
        Status = STATUS_INVALID_PARAMETER;
        /* Complete request and return status */
        FatCompleteRequest(IrpContext, Irp, Status);
        return Status;
    }

    /* If it's a root DCB - fail */
    if (FatNodeType(Fcb) == FAT_NTC_ROOT_DCB)
    {
        if (InfoClass == FileDispositionInformation)
            Status = STATUS_CANNOT_DELETE;
        else
            Status = STATUS_INVALID_PARAMETER;

        /* Complete request and return status */
        FatCompleteRequest(IrpContext, Irp, Status);
        return Status;
    }

    /* Acquire the volume lock if needed */
    if (InfoClass == FileDispositionInformation ||
        InfoClass == FileRenameInformation)
    {
        if (!FatAcquireExclusiveVcb(IrpContext, Vcb))
        {
            UNIMPLEMENTED;
        }

        VcbAcquired = TRUE;
    }

    /* Acquire FCB lock */
    if (!FlagOn(Fcb->State, FCB_STATE_PAGEFILE))
    {
        if (!FatAcquireExclusiveFcb(IrpContext, Fcb))
        {
            UNIMPLEMENTED;
        }

        FcbAcquired = TRUE;
    }

    // TODO: VerifyFcb

    switch (InfoClass)
    {
    case FileBasicInformation:
        //Status = FatSetBasicInfo(IrpContext, Irp, Fcb, Ccb);
        DPRINT1("FileBasicInformation\n");
        break;

    case FileDispositionInformation:
        if (FlagOn(Vcb->State, VCB_STATE_FLAG_DEFERRED_FLUSH) &&
            !FlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT))
        {
            UNIMPLEMENTED;
        }
        else
        {
            //Status = FatSetDispositionInfo(IrpContext, Irp, FileObject, Fcb);
            DPRINT1("FileDispositionInformation\n");
        }

        break;

    case FileRenameInformation:
        if (!FlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT))
        {
            UNIMPLEMENTED;
        }
        else
        {
            //Status = FatSetRenameInfo(IrpContext, Irp, Vcb, Fcb, Ccb);
            DPRINT1("FileRenameInformation\n");

            /* NOTE: Request must not be completed here!
            That's why Irp/IrpContext are set to NULL */
            if (Status == STATUS_PENDING)
            {
                Irp = NULL;
                IrpContext = NULL;
            }
        }
        break;

    case FilePositionInformation:
        //Status = FatSetPositionInfo(IrpContext, Irp, FileObject);
        DPRINT1("FilePositionInformation\n");
        break;

    case FileLinkInformation:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case FileAllocationInformation:
        //Status = FatSetAllocationInfo(IrpContext, Irp, Fcb, FileObject);
        DPRINT1("FileAllocationInformation\n");
        break;

    case FileEndOfFileInformation:
        Status = FatSetEndOfFileInfo(IrpContext, Irp, FileObject, Vcb, Fcb);
        break;

    default:
        Status = STATUS_INVALID_PARAMETER;
    }

    /* Release locks */
    if (FcbAcquired) FatReleaseFcb(IrpContext, Fcb);
    if (VcbAcquired) FatReleaseVcb(IrpContext, Vcb);

    /* Complete request and return status */
    FatCompleteRequest(IrpContext, Irp, Status);
    return Status;
}

NTSTATUS
NTAPI
FatSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp)
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

    /* Perform the actual read */
    Status = FatiSetInformation(IrpContext, Irp);

    /* Restore top level Irp */
    if (TopLevel) IoSetTopLevelIrp(NULL);

    /* Leave FsRtl critical region */
    FsRtlExitFileSystem();

    return Status;
}

/* EOF */
