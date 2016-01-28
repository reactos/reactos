/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             read.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

NTSTATUS
Ext2ReadComplete (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2ReadFile (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2ReadVolume (IN PEXT2_IRP_CONTEXT IrpContext);

/* FUNCTIONS *************************************************************/

NTSTATUS
Ext2CompleteIrpContext (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN NTSTATUS Status )
{
    PIRP    Irp = NULL;
    BOOLEAN bPrint;

    Irp = IrpContext->Irp;

    if (Irp != NULL) {

        if (NT_ERROR(Status)) {
            Irp->IoStatus.Information = 0;
        }

        Irp->IoStatus.Status = Status;
        bPrint = !IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED);

        Ext2CompleteRequest(
            Irp, bPrint, (CCHAR)(NT_SUCCESS(Status)?
                                 IO_DISK_INCREMENT : IO_NO_INCREMENT) );

        IrpContext->Irp = NULL;
    }

    Ext2FreeIrpContext(IrpContext);

    return Status;
}


NTSTATUS
Ext2ReadVolume (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PEXT2_VCB           Vcb;
    PEXT2_CCB           Ccb;
    PEXT2_FCBVCB        FcbOrVcb;
    PFILE_OBJECT        FileObject;

    PDEVICE_OBJECT      DeviceObject;

    PIRP                Irp = NULL;
    PIO_STACK_LOCATION  IoStackLocation;

    ULONG               Length;
    LARGE_INTEGER       ByteOffset;

    BOOLEAN             PagingIo;
    BOOLEAN             Nocache;
    BOOLEAN             SynchronousIo;
    BOOLEAN             MainResourceAcquired = FALSE;

    PUCHAR              Buffer = NULL;
    EXT2_EXTENT         BlockArray;

    _SEH2_TRY {

        ASSERT(IrpContext);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        FileObject = IrpContext->FileObject;
        FcbOrVcb = (PEXT2_FCBVCB) FileObject->FsContext;
        ASSERT(FcbOrVcb);

        if (!(FcbOrVcb->Identifier.Type == EXT2VCB && (PVOID)FcbOrVcb == (PVOID)Vcb)) {

            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        Irp = IrpContext->Irp;
        Irp->IoStatus.Information = 0;
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

        Length = IoStackLocation->Parameters.Read.Length;
        ByteOffset = IoStackLocation->Parameters.Read.ByteOffset;

        PagingIo = IsFlagOn(Irp->Flags, IRP_PAGING_IO);
        Nocache = IsFlagOn(Irp->Flags, IRP_NOCACHE) || (Ccb != NULL);
        SynchronousIo = IsFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO);

        if (PagingIo) {
            ASSERT(Nocache);
        }

        if (Length == 0) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC)) {
            ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }

        if (ByteOffset.QuadPart >=
                Vcb->PartitionInformation.PartitionLength.QuadPart  ) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_END_OF_FILE;
            _SEH2_LEAVE;
        }

        if (ByteOffset.QuadPart + Length > Vcb->Header.FileSize.QuadPart) {
            Length = (ULONG)(Vcb->Header.FileSize.QuadPart - ByteOffset.QuadPart);
        }

        /*
         *  User direct volume access
         */

        if (Ccb != NULL && !PagingIo) {

            if (!ExAcquireResourceExclusiveLite(
                        &Vcb->MainResource,
                        IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            MainResourceAcquired = TRUE;

            if (!FlagOn(Ccb->Flags, CCB_VOLUME_DASD_PURGE)) {

                if (!FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
                    Ext2FlushVolume(IrpContext, Vcb, FALSE);
                }

                SetFlag(Ccb->Flags, CCB_VOLUME_DASD_PURGE);
            }

            ExReleaseResourceLite(&Vcb->MainResource);
            MainResourceAcquired = FALSE;

            /* will do Nocache i/o */
        }

        /*
         *  I/O to volume StreamObject
         */

        if (!Nocache) {

            if (IsFlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                CcMdlRead(
                    Vcb->Volume,
                    &ByteOffset,
                    Length,
                    &Irp->MdlAddress,
                    &Irp->IoStatus );

                Status = Irp->IoStatus.Status;

            } else {

                Buffer = Ext2GetUserBuffer(Irp);
                if (Buffer == NULL) {
                    DbgBreak();
                    Status = STATUS_INVALID_USER_BUFFER;
                    _SEH2_LEAVE;
                }

                if (!CcCopyRead(
                            Vcb->Volume,
                            &ByteOffset,
                            Length,
                            Ext2CanIWait(),
                            Buffer,
                            &Irp->IoStatus )) {
                    Status = STATUS_PENDING;
                    _SEH2_LEAVE;
                }

                Status = Irp->IoStatus.Status;
            }

        } else {

            Length &= ~((ULONG)SECTOR_SIZE - 1);
            Status = Ext2LockUserBuffer(
                         IrpContext->Irp,
                         Length,
                         IoWriteAccess );

            if (!NT_SUCCESS(Status)) {
                _SEH2_LEAVE;
            }

            BlockArray.Irp = NULL;
            BlockArray.Lba = ByteOffset.QuadPart;
            BlockArray.Offset = 0;
            BlockArray.Length = Length;
            BlockArray.Next = NULL;

            Status = Ext2ReadWriteBlocks(IrpContext,
                                         Vcb,
                                         &BlockArray,
                                         Length );

            Irp = IrpContext->Irp;
            if (!Irp) {
                _SEH2_LEAVE;
            }
        }

    } _SEH2_FINALLY {

        if (MainResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {

            if (Irp) {

                if (Status == STATUS_PENDING &&
                    !IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED)) {

                    Status = Ext2LockUserBuffer(
                                 IrpContext->Irp,
                                 Length,
                                 IoWriteAccess );

                    if (NT_SUCCESS(Status)) {
                        Status = Ext2QueueRequest(IrpContext);
                    } else {
                        Ext2CompleteIrpContext(IrpContext, Status);
                    }

                } else {

                    if (NT_SUCCESS(Status)) {

                        if (!PagingIo) {

                            if (SynchronousIo) {

                                FileObject->CurrentByteOffset.QuadPart =
                                    ByteOffset.QuadPart + Irp->IoStatus.Information;
                            }

                            FileObject->Flags |= FO_FILE_FAST_IO_READ;
                        }
                    }

                    Ext2CompleteIrpContext(IrpContext, Status);;
                }

            } else {
                Ext2FreeIrpContext(IrpContext);
            }
        }
    } _SEH2_END;

    return Status;
}


#define SafeZeroMemory(AT,BYTE_COUNT) {                                 \
    _SEH2_TRY {                                                         \
        if (AT)                                                         \
            RtlZeroMemory((AT), (BYTE_COUNT));                          \
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {                         \
         Ext2RaiseStatus( IrpContext, STATUS_INVALID_USER_BUFFER );     \
    } _SEH2_END;                                                        \
}

NTSTATUS
Ext2ReadInode (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONGLONG            Offset,
    IN PVOID                Buffer,
    IN ULONG                Size,
    IN BOOLEAN              bDirectIo,
    OUT PULONG              BytesRead
)
{
    PEXT2_EXTENT    Chain = NULL;
    PEXT2_EXTENT    Extent = NULL, Prev = NULL;

    IO_STATUS_BLOCK IoStatus;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    ULONG           RealSize ;

    if (BytesRead) {
        *BytesRead = 0;
    }

    _SEH2_TRY {

        Ext2ReferMcb(Mcb);

        ASSERT((Mcb->Identifier.Type == EXT2MCB) &&
               (Mcb->Identifier.Size == sizeof(EXT2_MCB)));

        if ((Mcb->Identifier.Type != EXT2MCB) ||
                (Mcb->Identifier.Size != sizeof(EXT2_MCB))) {
            _SEH2_LEAVE;
        }

        if (Buffer == NULL && IrpContext != NULL)
            Buffer = Ext2GetUserBuffer(IrpContext->Irp);


        /* handle fast symlinks */
        if (S_ISLNK(Mcb->Inode.i_mode) &&
                Mcb->Inode.i_size < EXT2_LINKLEN_IN_INODE) {

            PUCHAR Data = (PUCHAR) (&Mcb->Inode.i_block[0]);

            if (!Buffer) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH2_LEAVE;
            }

            if (Offset < EXT2_LINKLEN_IN_INODE) {
                if ((ULONG)Offset + Size >= EXT2_LINKLEN_IN_INODE)
                    Size = EXT2_LINKLEN_IN_INODE - (ULONG)Offset - 1;
                RtlCopyMemory(Buffer, Data + (ULONG)Offset, Size);
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_END_OF_FILE;
            }
            _SEH2_LEAVE;
        }

        //
        // Build the scatterred block ranges to be read
        //

        if (bDirectIo) {
            RealSize = CEILING_ALIGNED(ULONG, Size, SECTOR_SIZE - 1);
        } else {
            RealSize = Size;
        }

        Status = Ext2BuildExtents(
                     IrpContext,
                     Vcb,
                     Mcb,
                     Offset,
                     RealSize,
                     FALSE,
                     &Chain
                 );

        if (!NT_SUCCESS(Status)) {
            _SEH2_LEAVE;
        }

        if (Chain == NULL) {
            SafeZeroMemory((PCHAR)Buffer, Size);
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        /* for sparse file, we need zero the gaps */
        for (Extent = Chain; Buffer != NULL && Extent != NULL; Extent = Extent->Next) {
            if (NULL == Prev) {
                ASSERT(Extent == Chain);
                if (Extent->Offset) {
                    SafeZeroMemory((PCHAR)Buffer, Extent->Offset);
                }
            } else if (Extent->Offset > (Prev->Offset + Prev->Length)) {
                SafeZeroMemory((PCHAR)Buffer + Prev->Offset + Prev->Length,
                               Extent->Offset - Prev->Offset - Prev->Length);
            }
            if (NULL == Extent->Next) {
                if (Extent->Offset + Extent->Length < Size) {
                    SafeZeroMemory((PCHAR)Buffer + Extent->Offset + Extent->Length,
                                   Size - Extent->Offset - Extent->Length);
                }
            }
            Prev = Extent;
        }

        if (bDirectIo) {

            ASSERT(IrpContext != NULL);

            // Offset should be SECTOR_SIZE aligned ...
            Status = Ext2ReadWriteBlocks(
                         IrpContext,
                         Vcb,
                         Chain,
                         Size
                     );
        } else {

            for (Extent = Chain; Extent != NULL; Extent = Extent->Next) {

                if (!CcCopyRead(
                            Vcb->Volume,
                            (PLARGE_INTEGER)(&(Extent->Lba)),
                            Extent->Length,
                            PIN_WAIT,
                            (PVOID)((PUCHAR)Buffer + Extent->Offset),
                            &IoStatus
                        )) {
                    Status = STATUS_CANT_WAIT;
                } else {
                    Status = IoStatus.Status;
                }

                if (!NT_SUCCESS(Status)) {
                    break;
                }
            }
        }

    } _SEH2_FINALLY {

        if (Chain) {
            Ext2DestroyExtentChain(Chain);
        }

        Ext2DerefMcb(Mcb);
    } _SEH2_END;

    if (NT_SUCCESS(Status)) {
        if (BytesRead)
            *BytesRead = Size;
    }

    return Status;
}

NTSTATUS
Ext2ReadFile(IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PEXT2_VCB           Vcb;
    PEXT2_FCB           Fcb;
    PEXT2_CCB           Ccb;
    PFILE_OBJECT        FileObject;

    PDEVICE_OBJECT      DeviceObject;

    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;

    ULONG               Length;
    ULONG               ReturnedLength = 0;
    LARGE_INTEGER       ByteOffset;

    BOOLEAN             OpPostIrp = FALSE;
    BOOLEAN             PagingIo;
    BOOLEAN             Nocache;
    BOOLEAN             SynchronousIo;
    BOOLEAN             MainResourceAcquired = FALSE;
    BOOLEAN             PagingIoResourceAcquired = FALSE;

    PUCHAR              Buffer;

    _SEH2_TRY {

        ASSERT(IrpContext);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        FileObject = IrpContext->FileObject;
        Fcb = (PEXT2_FCB) FileObject->FsContext;
        ASSERT(Fcb);
        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        Ccb = (PEXT2_CCB) FileObject->FsContext2;

        Irp = IrpContext->Irp;
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

        Length = IoStackLocation->Parameters.Read.Length;
        ByteOffset = IoStackLocation->Parameters.Read.ByteOffset;

        PagingIo = IsFlagOn(Irp->Flags, IRP_PAGING_IO);
        Nocache = IsFlagOn(Irp->Flags, IRP_NOCACHE);
        SynchronousIo = IsFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO);

        if (PagingIo) {
            ASSERT(Nocache);
        }

        DEBUG(DL_INF, ("Ext2ReadFile: reading %wZ Off=%I64xh Len=%xh Paging=%xh Nocache=%xh\n",
                       &Fcb->Mcb->ShortName, ByteOffset.QuadPart, Length, PagingIo, Nocache));

        if ((IsSymLink(Fcb) && IsFileDeleted(Fcb->Mcb->Target)) ||
            IsFileDeleted(Fcb->Mcb)) {
            Status = STATUS_FILE_DELETED;
            _SEH2_LEAVE;
        }

        if (Length == 0) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        if (Nocache &&
                (ByteOffset.LowPart & (SECTOR_SIZE - 1) ||
                 Length & (SECTOR_SIZE - 1))) {
            Status = STATUS_INVALID_PARAMETER;
            DbgBreak();
            _SEH2_LEAVE;
        }

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC)) {
            ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
            Status = STATUS_PENDING;
            DbgBreak();
            _SEH2_LEAVE;
        }

        if (!PagingIo && Nocache && (FileObject->SectionObjectPointer->DataSectionObject != NULL)) {
            CcFlushCache( FileObject->SectionObjectPointer,
                          &ByteOffset,
                          Length,
                          &Irp->IoStatus );

            if (!NT_SUCCESS(Irp->IoStatus.Status)) {
                _SEH2_LEAVE;
            }
        }

        ReturnedLength = Length;

        if (PagingIo) {

            if (!ExAcquireResourceSharedLite(
                        &Fcb->PagingIoResource,
                        IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            PagingIoResourceAcquired = TRUE;

        } else {

            if (Nocache) {

                if (!ExAcquireResourceExclusiveLite(
                            &Fcb->MainResource,
                            IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
                    Status = STATUS_PENDING;
                    _SEH2_LEAVE;
                }
                MainResourceAcquired = TRUE;

            } else {

                if (!ExAcquireResourceSharedLite(
                            &Fcb->MainResource,
                            IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
                    Status = STATUS_PENDING;
                    _SEH2_LEAVE;
                }
                MainResourceAcquired = TRUE;
            }

            if (!FsRtlCheckLockForReadAccess(
                        &Fcb->FileLockAnchor,
                        Irp         )) {
                Status = STATUS_FILE_LOCK_CONFLICT;
                _SEH2_LEAVE;
            }
        }

        if ((ByteOffset.QuadPart + (LONGLONG)Length) > Fcb->Header.FileSize.QuadPart) {
            if (ByteOffset.QuadPart >= Fcb->Header.FileSize.QuadPart) {
                Irp->IoStatus.Information = 0;
                Status = STATUS_END_OF_FILE;
                _SEH2_LEAVE;
            }
            ReturnedLength = (ULONG)(Fcb->Header.FileSize.QuadPart - ByteOffset.QuadPart);
        }


        if (!IsDirectory(Fcb) && Ccb != NULL) {
            Status = FsRtlCheckOplock( &Fcb->Oplock,
                                       Irp,
                                       IrpContext,
                                       Ext2OplockComplete,
                                       Ext2LockIrp );

            if (Status != STATUS_SUCCESS) {
                OpPostIrp = TRUE;
                _SEH2_LEAVE;
            }

            //
            //  Set the flag indicating if Fast I/O is possible
            //

            Fcb->Header.IsFastIoPossible = Ext2IsFastIoPossible(Fcb);
        }

        if (!Nocache) {

            if (IsDirectory(Fcb)) {
                _SEH2_LEAVE;
            }

            if (FileObject->PrivateCacheMap == NULL) {
                CcInitializeCacheMap(
                        FileObject,
                        (PCC_FILE_SIZES)(&Fcb->Header.AllocationSize),
                        FALSE,
                        &Ext2Global->CacheManagerCallbacks,
                        Fcb );
                CcSetReadAheadGranularity(
                        FileObject,
                        READ_AHEAD_GRANULARITY );
            }

            if (FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {
                CcMdlRead(
                    FileObject,
                    (&ByteOffset),
                    ReturnedLength,
                    &Irp->MdlAddress,
                    &Irp->IoStatus );

                Status = Irp->IoStatus.Status;

            } else {

                Buffer = Ext2GetUserBuffer(Irp);
                if (Buffer == NULL) {
                    Status = STATUS_INVALID_USER_BUFFER;
                    DbgBreak();
                    _SEH2_LEAVE;
                }

                if (!CcCopyRead(FileObject, &ByteOffset, ReturnedLength,
                                Ext2CanIWait(), Buffer, &Irp->IoStatus)) {

                    if (Ext2CanIWait() || !CcCopyRead(FileObject, &ByteOffset,
                                                      ReturnedLength, TRUE,
                                                      Buffer, &Irp->IoStatus)) {
                        Status = STATUS_PENDING;
                        DbgBreak();
                        _SEH2_LEAVE;
                    }
                }
                Status = Irp->IoStatus.Status;
            }

        } else {

            ULONG   BytesRead = ReturnedLength;
            PUCHAR  SystemVA  = Ext2GetUserBuffer(IrpContext->Irp);

            if (ByteOffset.QuadPart + BytesRead > Fcb->Header.ValidDataLength.QuadPart) {

                if (ByteOffset.QuadPart >= Fcb->Header.ValidDataLength.QuadPart) {
                    if (SystemVA) {
                        SafeZeroMemory(SystemVA, Length);
                    }
                    Irp->IoStatus.Information = ReturnedLength;
                    Status = STATUS_SUCCESS;
                    _SEH2_LEAVE;
                } else {
                    BytesRead = (ULONG)(Fcb->Header.ValidDataLength.QuadPart - ByteOffset.QuadPart);
                    if (SystemVA) {
                        SafeZeroMemory(SystemVA + BytesRead, Length - BytesRead);
                    }
                }
            }

            Status = Ext2LockUserBuffer(
                         IrpContext->Irp,
                         BytesRead,
                         IoReadAccess );

            if (!NT_SUCCESS(Status)) {
                _SEH2_LEAVE;
            }

            Status = Ext2ReadInode(
                         IrpContext,
                         Vcb,
                         Fcb->Mcb,
                         ByteOffset.QuadPart,
                         NULL,
                         BytesRead,
                         TRUE,
                         NULL );

            /* we need re-queue this request in case STATUS_CANT_WAIT
               and fail it in other failure cases  */
            if (!NT_SUCCESS(Status)) {
                _SEH2_LEAVE;
            }

            /* pended by low level device */
            if (Status == STATUS_PENDING) {
                IrpContext->Irp = Irp = NULL;
                _SEH2_LEAVE;
            }

            Irp = IrpContext->Irp;
            ASSERT(Irp);
            Status = Irp->IoStatus.Status;

            if (!NT_SUCCESS(Status)) {
                Ext2NormalizeAndRaiseStatus(IrpContext, Status);
            }
        }

        Irp->IoStatus.Information = ReturnedLength;

    } _SEH2_FINALLY {

        if (Irp) {
            if (PagingIoResourceAcquired) {
                ExReleaseResourceLite(&Fcb->PagingIoResource);
            }

            if (MainResourceAcquired) {
                ExReleaseResourceLite(&Fcb->MainResource);
            }
        }

        if (!OpPostIrp && !IrpContext->ExceptionInProgress) {

            if (Irp) {
                if ( Status == STATUS_PENDING ||
                     Status == STATUS_CANT_WAIT) {

                    Status = Ext2LockUserBuffer(
                                 IrpContext->Irp,
                                 Length,
                                 IoWriteAccess );

                    if (NT_SUCCESS(Status)) {
                        Status = Ext2QueueRequest(IrpContext);
                    } else {
                        Ext2CompleteIrpContext(IrpContext, Status);
                    }
                } else {
                    if (NT_SUCCESS(Status)) {
                        if (!PagingIo) {
                            if (SynchronousIo) {
                                FileObject->CurrentByteOffset.QuadPart =
                                    ByteOffset.QuadPart + Irp->IoStatus.Information;
                            }
                            FileObject->Flags |= FO_FILE_FAST_IO_READ;
                        }
                    }

                    Ext2CompleteIrpContext(IrpContext, Status);
                }

            } else {

                Ext2FreeIrpContext(IrpContext);
            }
        }
    } _SEH2_END;

    DEBUG(DL_IO, ("Ext2ReadFile: %wZ fetch at Off=%I64xh Len=%xh Paging=%xh Nocache=%xh Returned=%xh Status=%xh\n",
                  &Fcb->Mcb->ShortName, ByteOffset.QuadPart, Length, PagingIo, Nocache, ReturnedLength, Status));
    return Status;

}

NTSTATUS
Ext2ReadComplete (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT    FileObject;
    PIRP            Irp;

    _SEH2_TRY {

        ASSERT(IrpContext);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        FileObject = IrpContext->FileObject;
        Irp = IrpContext->Irp;

        CcMdlReadComplete(FileObject, Irp->MdlAddress);
        Irp->MdlAddress = NULL;
        Status = STATUS_SUCCESS;

    } _SEH2_FINALLY {

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;

    return Status;
}


NTSTATUS
Ext2Read (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status;
    PEXT2_VCB           Vcb;
    PEXT2_FCBVCB        FcbOrVcb;
    PDEVICE_OBJECT      DeviceObject;
    PFILE_OBJECT        FileObject;
    BOOLEAN             bCompleteRequest;

    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
           (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

    _SEH2_TRY {

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_COMPLETE)) {

            Status =  Ext2ReadComplete(IrpContext);
            bCompleteRequest = FALSE;

        } else {

            DeviceObject = IrpContext->DeviceObject;

            if (IsExt2FsDevice(DeviceObject)) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                bCompleteRequest = TRUE;
                _SEH2_LEAVE;
            }

            Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
            if (Vcb->Identifier.Type != EXT2VCB ||
                    Vcb->Identifier.Size != sizeof(EXT2_VCB) ) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                bCompleteRequest = TRUE;

                _SEH2_LEAVE;
            }

            FileObject = IrpContext->FileObject;

            if (FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED) &&
                Vcb->LockFile != FileObject ) {
                Status = STATUS_ACCESS_DENIED;
                _SEH2_LEAVE;
            }

            FcbOrVcb = (PEXT2_FCBVCB) FileObject->FsContext;

            if (FcbOrVcb->Identifier.Type == EXT2VCB) {

                Status = Ext2ReadVolume(IrpContext);
                bCompleteRequest = FALSE;

            } else if (FcbOrVcb->Identifier.Type == EXT2FCB) {

                if (IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {
                    Status = STATUS_TOO_LATE;
                    bCompleteRequest = TRUE;
                    _SEH2_LEAVE;
                }

                Status = Ext2ReadFile(IrpContext);
                bCompleteRequest = FALSE;
            } else {
                DEBUG(DL_ERR, ( "Ext2Read: Inavlid FileObject (Vcb or Fcb corrupted)\n"));
                DbgBreak();

                Status = STATUS_INVALID_PARAMETER;
                bCompleteRequest = TRUE;
            }
        }

    } _SEH2_FINALLY {
        if (bCompleteRequest) {
            Ext2CompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;

    return Status;
}
