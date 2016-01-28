/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             write.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

#define DL_FLP  DL_DBG

/* DEFINITIONS *************************************************************/

#define EXT2_FLPFLUSH_MAGIC 'FF2E'

typedef struct _EXT2_FLPFLUSH_CONTEXT {

    PEXT2_VCB           Vcb;
    PEXT2_FCB           Fcb;
    PFILE_OBJECT        FileObject;

    KDPC                Dpc;
    KTIMER              Timer;
    WORK_QUEUE_ITEM     Item;

} EXT2_FLPFLUSH_CONTEXT, *PEXT2_FLPFLUSH_CONTEXT;

VOID NTAPI
Ext2FloppyFlush(IN PVOID Parameter);

VOID NTAPI
Ext2FloppyFlushDpc (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2);


NTSTATUS
Ext2WriteComplete (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2WriteFile (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2WriteVolume (IN PEXT2_IRP_CONTEXT IrpContext);

VOID
Ext2DeferWrite(IN PEXT2_IRP_CONTEXT, PIRP Irp);


/* FUNCTIONS *************************************************************/

VOID NTAPI
Ext2FloppyFlush(IN PVOID Parameter)
{
    PEXT2_FLPFLUSH_CONTEXT Context;
    PFILE_OBJECT           FileObject;
    PEXT2_FCB Fcb;
    PEXT2_VCB Vcb;

    Context = (PEXT2_FLPFLUSH_CONTEXT) Parameter;
    FileObject = Context->FileObject;
    Fcb = Context->Fcb;
    Vcb = Context->Vcb;

    DEBUG(DL_FLP, ("Ext2FloppyFlushing ...\n"));

    IoSetTopLevelIrp((PIRP)FSRTL_FSP_TOP_LEVEL_IRP);

    if (FileObject) {
        ASSERT(Fcb == (PEXT2_FCB)FileObject->FsContext);

        ExAcquireSharedStarveExclusive(&Fcb->PagingIoResource, TRUE);
        ExReleaseResourceLite(&Fcb->PagingIoResource);

        CcFlushCache(&(Fcb->SectionObject), NULL, 0, NULL);

        ObDereferenceObject(FileObject);
    }

    if (Vcb) {
        ExAcquireSharedStarveExclusive(&Vcb->PagingIoResource, TRUE);
        ExReleaseResourceLite(&Vcb->PagingIoResource);

        CcFlushCache(&(Vcb->SectionObject), NULL, 0, NULL);
    }

    IoSetTopLevelIrp(NULL);
    Ext2FreePool(Parameter, EXT2_FLPFLUSH_MAGIC);
}

VOID NTAPI
Ext2FloppyFlushDpc (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
)
{
    PEXT2_FLPFLUSH_CONTEXT Context;

    Context = (PEXT2_FLPFLUSH_CONTEXT) DeferredContext;

    DEBUG(DL_FLP, ("Ext2FloppyFlushDpc is to be started...\n"));

    ExQueueWorkItem(&Context->Item, CriticalWorkQueue);
}

VOID
Ext2StartFloppyFlushDpc (
    PEXT2_VCB   Vcb,
    PEXT2_FCB   Fcb,
    PFILE_OBJECT FileObject )
{
    LARGE_INTEGER          OneSecond;
    PEXT2_FLPFLUSH_CONTEXT Context;

    ASSERT(IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK));

    Context = Ext2AllocatePool(
                  NonPagedPool,
                  sizeof(EXT2_FLPFLUSH_CONTEXT),
                  EXT2_FLPFLUSH_MAGIC
              );

    if (!Context) {
        DEBUG(DL_ERR, ( "Ex2StartFloppy...: failed to allocate Context\n"));
        DbgBreak();
        return;
    }

    KeInitializeTimer(&Context->Timer);

    KeInitializeDpc( &Context->Dpc,
                     Ext2FloppyFlushDpc,
                     Context );

    ExInitializeWorkItem( &Context->Item,
                          Ext2FloppyFlush,
                          Context );

    Context->Vcb = Vcb;
    Context->Fcb = Fcb;
    Context->FileObject = FileObject;

    if (FileObject) {
        ObReferenceObject(FileObject);
    }

    OneSecond.QuadPart = (LONGLONG)-1*1000*1000*10;
    KeSetTimer( &Context->Timer,
                OneSecond,
                &Context->Dpc );
}

BOOLEAN
Ext2ZeroData (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB         Vcb,
    IN PFILE_OBJECT      FileObject,
    IN PLARGE_INTEGER    Start,
    IN PLARGE_INTEGER    End
    )
{
    PEXT2_FCB       Fcb;
    PBCB            Bcb;
    PVOID           Ptr;
    ULONG           Size;
#ifndef __REACTOS__
    BOOLEAN         rc = TRUE;
#endif
    ASSERT (End && Start && End->QuadPart > Start->QuadPart);
    Fcb = (PEXT2_FCB) FileObject->FsContext;

    /* skip data zero if we've already tracked unwritten part */
    if (0 == (  End->LowPart & (BLOCK_SIZE -1)) &&
        0 == (Start->LowPart & (BLOCK_SIZE -1))) {

        if (INODE_HAS_EXTENT(Fcb->Inode)) {
            return TRUE;
        } else {
#if !EXT2_PRE_ALLOCATION_SUPPORT
            return TRUE;
#endif
        }
    }

    /* clear data in range [Start, End) */
    return CcZeroData(FileObject, Start, End, Ext2CanIWait());
}

VOID
Ext2DeferWrite(IN PEXT2_IRP_CONTEXT IrpContext, PIRP Irp)
{
    ASSERT(IrpContext->Irp == Irp);

    Ext2QueueRequest(IrpContext);
}


NTSTATUS
Ext2WriteVolume (IN PEXT2_IRP_CONTEXT IrpContext)
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

    BOOLEAN             PagingIo = FALSE;
    BOOLEAN             Nocache = FALSE;
    BOOLEAN             SynchronousIo = FALSE;
    BOOLEAN             MainResourceAcquired = FALSE;

    BOOLEAN             bDeferred = FALSE;

    PUCHAR              Buffer = NULL;
    PEXT2_EXTENT        Chain = NULL;
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
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

        Length = IoStackLocation->Parameters.Write.Length;
        ByteOffset = IoStackLocation->Parameters.Write.ByteOffset;

        PagingIo = IsFlagOn(Irp->Flags, IRP_PAGING_IO);
        Nocache = IsFlagOn(Irp->Flags, IRP_NOCACHE) || (Ccb != NULL);
        SynchronousIo = IsFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO);

        if (PagingIo) {
            ASSERT(Nocache);
        }

        DEBUG(DL_INF, ("Ext2WriteVolume: Off=%I64xh Len=%xh Paging=%xh Nocache=%xh\n",
                       ByteOffset.QuadPart, Length, PagingIo, Nocache));

        if (Length == 0) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        if (Nocache &&
                (ByteOffset.LowPart & (SECTOR_SIZE - 1) ||
                 Length & (SECTOR_SIZE - 1))) {
            Status = STATUS_INVALID_PARAMETER;
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

        if (!Nocache) {

            BOOLEAN bAgain = IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED);
            BOOLEAN bWait  = IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
            BOOLEAN bQueue = IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED);

            if ( !CcCanIWrite(
                        FileObject,
                        Length,
                        (bWait && bQueue),
                        bAgain ) ) {

                Status = Ext2LockUserBuffer(
                             IrpContext->Irp,
                             Length,
                             IoReadAccess);
                if (NT_SUCCESS(Status)) {
                    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED);
                    CcDeferWrite( FileObject,
                                  (PCC_POST_DEFERRED_WRITE)Ext2DeferWrite,
                                  IrpContext,
                                  Irp,
                                  Length,
                                  bAgain );

                    bDeferred = TRUE;
                    Status = STATUS_PENDING;

                    _SEH2_LEAVE;
                }
            }
        }

        /*
         * User direct volume access
         */

        if (Ccb != NULL && !PagingIo) {

            if (!FlagOn(Ccb->Flags, CCB_VOLUME_DASD_PURGE)) {

                if (!FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
                    Status = Ext2PurgeVolume( Vcb, TRUE);
                }

                SetFlag(Ccb->Flags, CCB_VOLUME_DASD_PURGE);
            }

            if (!IsFlagOn(Ccb->Flags, CCB_ALLOW_EXTENDED_DASD_IO)) {
                if (ByteOffset.QuadPart + Length > Vcb->Header.FileSize.QuadPart) {
                    Length = (ULONG)(Vcb->Header.FileSize.QuadPart - ByteOffset.QuadPart);
                }
            }

        } else if (Nocache && !PagingIo && (Vcb->SectionObject.DataSectionObject != NULL))  {

            ExAcquireResourceExclusiveLite(&Vcb->MainResource, TRUE);
            MainResourceAcquired = TRUE;

            ExAcquireSharedStarveExclusive(&Vcb->PagingIoResource, TRUE);
            ExReleaseResourceLite(&Vcb->PagingIoResource);

            CcFlushCache( &(Vcb->SectionObject),
                          &ByteOffset,
                          Length,
                          &(Irp->IoStatus));

            if (!NT_SUCCESS(Irp->IoStatus.Status))  {
                Status = Irp->IoStatus.Status;
                _SEH2_LEAVE;
            }

            ExAcquireSharedStarveExclusive(&Vcb->PagingIoResource, TRUE);
            ExReleaseResourceLite(&Vcb->PagingIoResource);

            CcPurgeCacheSection( &(Vcb->SectionObject),
                                 (PLARGE_INTEGER)&(ByteOffset),
                                 Length,
                                 FALSE );

            ExReleaseResourceLite(&Vcb->MainResource);
            MainResourceAcquired = FALSE;
        }

        if ( (ByteOffset.QuadPart + Length) > Vcb->Header.FileSize.QuadPart) {
            Length = (ULONG)(Vcb->Header.FileSize.QuadPart - ByteOffset.QuadPart);
        }

        if (!Nocache) {

            if (FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                CcPrepareMdlWrite (
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

                if (!CcCopyWrite( Vcb->Volume,
                                  (PLARGE_INTEGER)(&ByteOffset),
                                  Length,
                                  TRUE,
                                  Buffer )) {
                    Status = STATUS_PENDING;
                    _SEH2_LEAVE;
                }

                Status = Irp->IoStatus.Status;
                Ext2AddVcbExtent(Vcb, ByteOffset.QuadPart, (LONGLONG)Length);
            }

            if (NT_SUCCESS(Status)) {
                Irp->IoStatus.Information = Length;
            }

        } else if (PagingIo) {

            LONGLONG            DirtyStart;
            LONGLONG            DirtyLba;
            LONGLONG            DirtyLength;
            LONGLONG            RemainLength;

            PEXT2_EXTENT        Extent = NULL;
            PEXT2_EXTENT        List = NULL;

            Length &= ~((ULONG)SECTOR_SIZE - 1);

            Status = Ext2LockUserBuffer(IrpContext->Irp, Length, IoReadAccess);
            if (!NT_SUCCESS(Status)) {
                _SEH2_LEAVE;
            }

            DirtyLba = ByteOffset.QuadPart;
            RemainLength = (LONGLONG) Length;

            ASSERT(Length >= SECTOR_SIZE);

            while (RemainLength > 0) {

                DirtyStart = DirtyLba;
                ASSERT(DirtyStart >= ByteOffset.QuadPart);
                ASSERT(DirtyStart <= ByteOffset.QuadPart + Length);

                if (Ext2LookupVcbExtent(Vcb, DirtyStart, &DirtyLba, &DirtyLength)) {

                    if (DirtyLba == -1) {

                        DirtyLba = DirtyStart + DirtyLength;
                        if (ByteOffset.QuadPart + Length > DirtyLba) {
                            RemainLength = ByteOffset.QuadPart + Length - DirtyLba;
                            ASSERT(DirtyStart >= ByteOffset.QuadPart);
                            ASSERT(DirtyStart <= ByteOffset.QuadPart + Length);
                        } else {
                            RemainLength = 0;
                        }
                        continue;
                    }

                    ASSERT(DirtyLba <= DirtyStart);
                    Extent = Ext2AllocateExtent();

                    if (!Extent) {
                        DEBUG(DL_ERR, ( "Ex2WriteVolume: failed to allocate Extent\n"));
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        _SEH2_LEAVE;
                    }

                    Extent->Irp = NULL;
                    Extent->Lba = DirtyLba;
                    Extent->Offset = (ULONG)( DirtyStart + Length -
                                              RemainLength - DirtyLba );
                    ASSERT(Extent->Offset <= Length);

                    if (DirtyLba + DirtyLength >= DirtyStart + RemainLength) {
                        Extent->Length = (ULONG)( DirtyLba +
                                                  RemainLength -
                                                  DirtyStart );
                        ASSERT(Extent->Length <= Length);
                        RemainLength = 0;
                    } else {
                        Extent->Length = (ULONG)(DirtyLength + DirtyLba - DirtyStart);
                        RemainLength =  (DirtyStart + RemainLength) -
                                        (DirtyLba + DirtyLength);
                        ASSERT(RemainLength <= (LONGLONG)Length);
                        ASSERT(Extent->Length <= Length);
                    }

                    ASSERT(Extent->Length >= SECTOR_SIZE);
                    DirtyLba = DirtyStart + DirtyLength;

                    if (List) {
                        List->Next = Extent;
                        List = Extent;
                    } else {
                        Chain = List = Extent;
                    }

                } else {

                    if (RemainLength > SECTOR_SIZE) {
                        DirtyLba = DirtyStart + SECTOR_SIZE;
                        RemainLength -= SECTOR_SIZE;
                    } else {
                        RemainLength = 0;
                    }
                }
            }

            if (Chain) {
                Status = Ext2ReadWriteBlocks(IrpContext,
                                             Vcb,
                                             Chain,
                                             Length );
                Irp = IrpContext->Irp;

                if (NT_SUCCESS(Status)) {
                    for (Extent = Chain; Extent != NULL; Extent = Extent->Next) {
                        Ext2RemoveVcbExtent(Vcb, Extent->Lba, Extent->Length);
                    }
                }

                if (!Irp) {
                    _SEH2_LEAVE;
                }

            } else {

                Irp->IoStatus.Information = Length;
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
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

            if (NT_SUCCESS(Status)) {
                Irp->IoStatus.Information = Length;
            }

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

                if (Status == STATUS_PENDING) {

                    if (!bDeferred) {
                        Status = Ext2LockUserBuffer(
                                     IrpContext->Irp,
                                     Length,
                                     IoReadAccess );

                        if (NT_SUCCESS(Status)) {
                            Status = Ext2QueueRequest(IrpContext);
                        } else {
                            Ext2CompleteIrpContext(IrpContext, Status);
                        }
                    }

                } else {

                    if (NT_SUCCESS(Status)) {

                        if (SynchronousIo && !PagingIo) {
                            FileObject->CurrentByteOffset.QuadPart =
                                ByteOffset.QuadPart + Irp->IoStatus.Information;
                        }

                        if (!PagingIo) {
                            SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
                        }
                    }

                    Ext2CompleteIrpContext(IrpContext, Status);
                }

            } else {

                Ext2FreeIrpContext(IrpContext);
            }
        }

        if (Chain) {
            Ext2DestroyExtentChain(Chain);
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2WriteInode (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONGLONG            Offset,
    IN PVOID                Buffer,
    IN ULONG                Size,
    IN BOOLEAN              bDirectIo,
    OUT PULONG              BytesWritten
)
{
    PEXT2_EXTENT    Chain = NULL;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;

    _SEH2_TRY {

        if (BytesWritten) {
            *BytesWritten = 0;
        }

        Status = Ext2BuildExtents (
                     IrpContext,
                     Vcb,
                     Mcb,
                     Offset,
                     Size,
                     IsMcbDirectory(Mcb) ? FALSE : TRUE,
                     &Chain
                 );

        if (!NT_SUCCESS(Status)) {
            _SEH2_LEAVE;
        }

        if (Chain == NULL) {
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        if (bDirectIo) {

            ASSERT(IrpContext != NULL);

            //
            // We assume the offset is aligned.
            //

            Status = Ext2ReadWriteBlocks(
                         IrpContext,
                         Vcb,
                         Chain,
                         Size
                     );

        } else {

            PEXT2_EXTENT Extent;
            for (Extent = Chain; Extent != NULL; Extent = Extent->Next) {

                if ( !Ext2SaveBuffer(
                            IrpContext,
                            Vcb,
                            Extent->Lba,
                            Extent->Length,
                            (PVOID)((PUCHAR)Buffer + Extent->Offset)
                        )) {
                    _SEH2_LEAVE;
                }
            }

            if (IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK)) {

                DEBUG(DL_FLP, ("Ext2WriteInode is starting FlushingDpc...\n"));
                Ext2StartFloppyFlushDpc(Vcb, NULL, NULL);
            }

            Status = STATUS_SUCCESS;
        }

    } _SEH2_FINALLY {

        if (Chain) {
            Ext2DestroyExtentChain(Chain);
        }

        if (NT_SUCCESS(Status) && BytesWritten) {
            *BytesWritten = Size;
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2WriteFile(IN PEXT2_IRP_CONTEXT IrpContext)
{
    PEXT2_VCB           Vcb;
    PEXT2_FCB           Fcb;
    PEXT2_CCB           Ccb;
    PFILE_OBJECT        FileObject;

    PDEVICE_OBJECT      DeviceObject;

    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;
    PUCHAR              Buffer;

    LARGE_INTEGER       ByteOffset;
    ULONG               ReturnedLength = 0;
    ULONG               Length;

    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    BOOLEAN             OpPostIrp = FALSE;
    BOOLEAN             PagingIo = FALSE;
    BOOLEAN             Nocache = FALSE;
    BOOLEAN             SynchronousIo = FALSE;

    BOOLEAN             RecursiveWriteThrough = FALSE;
    BOOLEAN             MainResourceAcquired = FALSE;
    BOOLEAN             PagingIoResourceAcquired = FALSE;

    BOOLEAN             bDeferred = FALSE;
#ifndef __REACTOS__
    BOOLEAN             UpdateFileValidSize = FALSE;
#endif
    BOOLEAN             FileSizesChanged = FALSE;
    BOOLEAN             rc;


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
        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        ASSERT(Fcb);
        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        Irp = IrpContext->Irp;
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

        Length = IoStackLocation->Parameters.Write.Length;
        ByteOffset = IoStackLocation->Parameters.Write.ByteOffset;

        PagingIo = IsFlagOn(Irp->Flags, IRP_PAGING_IO);
        Nocache = IsFlagOn(Irp->Flags, IRP_NOCACHE);
        SynchronousIo = IsFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO);

        if (PagingIo) {
            ASSERT(Nocache);
        }

        DEBUG(DL_INF, ("Ext2WriteFile: %wZ Offset=%I64xh Length=%xh Paging=%xh Nocache=%xh\n",
                       &Fcb->Mcb->ShortName, ByteOffset.QuadPart, Length, PagingIo, Nocache));

        if (IsSpecialFile(Fcb)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        if (IsFileDeleted(Fcb->Mcb) ||
            (IsSymLink(Fcb) && IsFileDeleted(Fcb->Mcb->Target)) ) {
            Status = STATUS_FILE_DELETED;
            _SEH2_LEAVE;
        }

        if (Length == 0) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        if (Nocache && ( (ByteOffset.LowPart & (SECTOR_SIZE - 1)) ||
                         (Length & (SECTOR_SIZE - 1))) ) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC)) {
            ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }

        if (!Nocache) {

            BOOLEAN bAgain = IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED);
            BOOLEAN bWait  = IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
            BOOLEAN bQueue = IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED);

            if ( !CcCanIWrite(
                        FileObject,
                        Length,
                        (bWait && bQueue),
                        bAgain ) ) {

                Status = Ext2LockUserBuffer(
                             IrpContext->Irp,
                             Length,
                             IoReadAccess);

                if (NT_SUCCESS(Status)) {
                    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED);
                    CcDeferWrite( FileObject,
                                  (PCC_POST_DEFERRED_WRITE)Ext2DeferWrite,
                                  IrpContext,
                                  Irp,
                                  Length,
                                  bAgain );
                    bDeferred = TRUE;
                    Status = STATUS_PENDING;
                    _SEH2_LEAVE;
                }
            }
        }

        if (IsWritingToEof(ByteOffset)) {
            ByteOffset.QuadPart = Fcb->Header.FileSize.QuadPart;
        }

        if (IsDirectory(Fcb) && !PagingIo) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        if (IsFlagOn(Irp->Flags, IRP_SYNCHRONOUS_PAGING_IO) && !IrpContext->IsTopLevel) {

            PIRP TopIrp;

            TopIrp = IoGetTopLevelIrp();

            if ( (ULONG_PTR)TopIrp > FSRTL_MAX_TOP_LEVEL_IRP_FLAG &&
                    NodeType(TopIrp) == IO_TYPE_IRP) {

                PIO_STACK_LOCATION IrpStack;

                IrpStack = IoGetCurrentIrpStackLocation(TopIrp);

                if ((IrpStack->MajorFunction == IRP_MJ_WRITE) &&
                    (IrpStack->FileObject->FsContext == FileObject->FsContext) &&
                    !FlagOn(TopIrp->Flags, IRP_NOCACHE) ) {

                    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH);
                    RecursiveWriteThrough = TRUE;
                }
            }
        }

        if (PagingIo) {

            if (!ExAcquireResourceSharedLite(&Fcb->PagingIoResource, TRUE)) {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            PagingIoResourceAcquired = TRUE;

            if ( (ByteOffset.QuadPart + Length) > Fcb->Header.AllocationSize.QuadPart) {

                if ( ByteOffset.QuadPart >= Fcb->Header.AllocationSize.QuadPart) {

                    Status = STATUS_END_OF_FILE;
                    Irp->IoStatus.Information = 0;
                    _SEH2_LEAVE;

                } else {

                    Length = (ULONG)(Fcb->Header.AllocationSize.QuadPart - ByteOffset.QuadPart);
                }
            }

        } else {

            if (IsDirectory(Fcb)) {
                _SEH2_LEAVE;
            }

            if (!ExAcquireResourceExclusiveLite(&Fcb->MainResource, TRUE)) {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            MainResourceAcquired = TRUE;

            //
            //  Do flushing for such cases
            //
            if (Nocache && Ccb != NULL && Fcb->SectionObject.DataSectionObject != NULL)  {

                ExAcquireSharedStarveExclusive( &Fcb->PagingIoResource, TRUE);
                ExReleaseResourceLite(&Fcb->PagingIoResource);

                CcFlushCache( &(Fcb->SectionObject),
                              &ByteOffset,
                              CEILING_ALIGNED(ULONG, Length, BLOCK_SIZE),
                              &(Irp->IoStatus));
                ClearLongFlag(Fcb->Flags, FCB_FILE_MODIFIED);

                if (!NT_SUCCESS(Irp->IoStatus.Status)) {
                    Status = Irp->IoStatus.Status;
                    _SEH2_LEAVE;
                }

                ExAcquireSharedStarveExclusive( &Fcb->PagingIoResource, TRUE);
                ExReleaseResourceLite(&Fcb->PagingIoResource);

                CcPurgeCacheSection( &(Fcb->SectionObject),
                                     &(ByteOffset),
                                     CEILING_ALIGNED(ULONG, Length, BLOCK_SIZE),
                                     FALSE );
            }

            if (!FsRtlCheckLockForWriteAccess(&Fcb->FileLockAnchor, Irp)) {
                Status = STATUS_FILE_LOCK_CONFLICT;
                _SEH2_LEAVE;
            }

            if (Ccb != NULL) {
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

            //
            //  Extend the inode size when the i/o is beyond the file end ?
            //

            if ((ByteOffset.QuadPart + Length) > Fcb->Header.FileSize.QuadPart) {

                LARGE_INTEGER AllocationSize, Last;

                if (!ExAcquireResourceExclusiveLite(&Fcb->PagingIoResource, TRUE)) {
                    Status = STATUS_PENDING;
                    _SEH2_LEAVE;
                }
                PagingIoResourceAcquired = TRUE;

                /* let this irp wait, since it has to be synchronous */
                SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

                Last.QuadPart = Fcb->Header.AllocationSize.QuadPart;
                AllocationSize.QuadPart = (LONGLONG)(ByteOffset.QuadPart + Length);
                AllocationSize.QuadPart = CEILING_ALIGNED(ULONGLONG,
                                          (ULONGLONG)AllocationSize.QuadPart,
                                          (ULONGLONG)BLOCK_SIZE);

                /* tell Ext2ExpandFile to allocate unwritten extent or NULL blocks
                   for indirect files, otherwise we might get gabage data in holes */
                IrpContext->MajorFunction += IRP_MJ_MAXIMUM_FUNCTION;
                Status = Ext2ExpandFile(IrpContext, Vcb, Fcb->Mcb, &AllocationSize);
                IrpContext->MajorFunction -= IRP_MJ_MAXIMUM_FUNCTION;
                if (AllocationSize.QuadPart > Last.QuadPart) {
                    Fcb->Header.AllocationSize.QuadPart = AllocationSize.QuadPart;
                    SetLongFlag(Fcb->Flags, FCB_ALLOC_IN_WRITE);
                }
                ExReleaseResourceLite(&Fcb->PagingIoResource);
                PagingIoResourceAcquired = FALSE;

                if (ByteOffset.QuadPart >= Fcb->Header.AllocationSize.QuadPart) {
                    if (NT_SUCCESS(Status)) {
                        DbgBreak();
                        Status = STATUS_UNSUCCESSFUL;
                    }
                    _SEH2_LEAVE;
                }

                if (ByteOffset.QuadPart + Length > Fcb->Header.AllocationSize.QuadPart) {
                    Length = (ULONG)(Fcb->Header.AllocationSize.QuadPart - ByteOffset.QuadPart);
                }

                Fcb->Header.FileSize.QuadPart = Fcb->Inode->i_size = ByteOffset.QuadPart + Length;
                Ext2SaveInode(IrpContext, Vcb, Fcb->Inode);

                if (CcIsFileCached(FileObject)) {
                    CcSetFileSizes(FileObject, (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
                }

                FileObject->Flags |= FO_FILE_SIZE_CHANGED | FO_FILE_MODIFIED;
                FileSizesChanged = TRUE;

                if (Fcb->Header.FileSize.QuadPart >= 0x80000000 &&
                        !IsFlagOn(SUPER_BLOCK->s_feature_ro_compat, EXT2_FEATURE_RO_COMPAT_LARGE_FILE)) {
                    SetFlag(SUPER_BLOCK->s_feature_ro_compat, EXT2_FEATURE_RO_COMPAT_LARGE_FILE);
                    Ext2SaveSuper(IrpContext, Vcb);
                }

                DEBUG(DL_IO, ("Ext2WriteFile: expanding %wZ to FS: %I64xh FA: %I64xh\n",
                              &Fcb->Mcb->ShortName, Fcb->Header.FileSize.QuadPart,
                              Fcb->Header.AllocationSize.QuadPart));
            }
        }

        ReturnedLength = Length;

        if (!Nocache) {

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

                CcPrepareMdlWrite(
                    FileObject,
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

                if (ByteOffset.QuadPart > Fcb->Header.ValidDataLength.QuadPart) {

                    /* let this irp wait, since it has to be synchronous */
                    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

                    rc = Ext2ZeroData(IrpContext, Vcb, FileObject,
                                      &Fcb->Header.ValidDataLength, &ByteOffset);
                    if (!rc) {
                        Status = STATUS_PENDING;
                        DbgBreak();
                        _SEH2_LEAVE;
                    }
                }

                if (!CcCopyWrite(FileObject, &ByteOffset, Length, Ext2CanIWait(), Buffer)) {
                    if (Ext2CanIWait() || 
                        !CcCopyWrite(FileObject,  &ByteOffset, Length, TRUE, Buffer)) {
                        Status = STATUS_PENDING;
                        DbgBreak();
                        _SEH2_LEAVE;
                    }
                }

                if (ByteOffset.QuadPart + Length > Fcb->Header.ValidDataLength.QuadPart ) {

                    if (Fcb->Header.FileSize.QuadPart < ByteOffset.QuadPart + Length) {
                        Fcb->Header.ValidDataLength.QuadPart = Fcb->Header.FileSize.QuadPart;
                    } else {
                        if (Fcb->Header.ValidDataLength.QuadPart < ByteOffset.QuadPart + Length)
                            Fcb->Header.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                    }

                    CcSetFileSizes(FileObject, (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
                    FileSizesChanged = TRUE;
                }

                Status = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(Status)) {
                Irp->IoStatus.Information = Length;
                if (IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK)) {
                    DEBUG(DL_FLP, ("Ext2WriteFile is starting FlushingDpc...\n"));
                    Ext2StartFloppyFlushDpc(Vcb, Fcb, FileObject);
                }
            }

        } else {

            if (!PagingIo && !RecursiveWriteThrough && !IsLazyWriter(Fcb)) {
                if (ByteOffset.QuadPart + Length > Fcb->Header.ValidDataLength.QuadPart ) {
                    if (ByteOffset.QuadPart > Fcb->Header.ValidDataLength.QuadPart) {

                        /* let this irp wait, since it has to be synchronous */
                        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
                        rc = Ext2ZeroData(IrpContext, Vcb, FileObject,
                                          &Fcb->Header.ValidDataLength,
                                          &ByteOffset);
                        if (!rc) {
                            Status = STATUS_PENDING;
                            DbgBreak();
                            _SEH2_LEAVE;
                        }
                    }
                }
            }

            Status = Ext2LockUserBuffer(
                         IrpContext->Irp,
                         Length,
                         IoReadAccess );

            if (!NT_SUCCESS(Status)) {
                _SEH2_LEAVE;
            }

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = ReturnedLength;

            Status = Ext2WriteInode(
                         IrpContext,
                         Vcb,
                         Fcb->Mcb,
                         (ULONGLONG)(ByteOffset.QuadPart),
                         NULL,
                         ReturnedLength,
                         TRUE,
                         &Length
                     );

            Irp = IrpContext->Irp;

            if (NT_SUCCESS(Status) && !PagingIo && !RecursiveWriteThrough && !IsLazyWriter(Fcb)) {

                if (ByteOffset.QuadPart + Length > Fcb->Header.ValidDataLength.QuadPart ) {

                    FileSizesChanged = TRUE;

                    if (Fcb->Header.FileSize.QuadPart < ByteOffset.QuadPart + Length) {
                        Fcb->Header.ValidDataLength.QuadPart = Fcb->Header.FileSize.QuadPart;
                    } else {
                        if (Fcb->Header.ValidDataLength.QuadPart < ByteOffset.QuadPart + Length)
                            Fcb->Header.ValidDataLength.QuadPart = ByteOffset.QuadPart + Length;
                    }

                    if (CcIsFileCached(FileObject)) {
                        CcSetFileSizes(FileObject, (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
                    }

                    DEBUG(DL_IO, ("Ext2WriteFile: %wZ written FS: %I64xh FA: %I64xh BO: %I64xh LEN: %u\n",
                                  &Fcb->Mcb->ShortName, Fcb->Header.FileSize.QuadPart,
                                   Fcb->Header.AllocationSize.QuadPart, ByteOffset.QuadPart, Length));
                }
            }
        }

        if (FileSizesChanged) {
            FileObject->Flags |= FO_FILE_SIZE_CHANGED | FO_FILE_MODIFIED;
            Ext2NotifyReportChange( IrpContext,  Vcb, Fcb->Mcb,
                                    FILE_NOTIFY_CHANGE_SIZE,
                                    FILE_ACTION_MODIFIED );
        }

    } _SEH2_FINALLY {

        /*
         *  in case we got excpetions, we need revert MajorFunction
         *  back to IRP_MJ_WRITE. The reason we do this, if to tell
         *  Ext2ExpandFile to allocate unwritten extent or don't add
         *  new blocks for indirect files.
         */
        if (IrpContext->MajorFunction > IRP_MJ_MAXIMUM_FUNCTION)
            IrpContext->MajorFunction -= IRP_MJ_MAXIMUM_FUNCTION;

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

                if (Status == STATUS_PENDING ||
                        Status == STATUS_CANT_WAIT ) {

                    if (!bDeferred) {
                        Status = Ext2QueueRequest(IrpContext);
                    }

                } else {

                    if (NT_SUCCESS(Status) && !PagingIo) {

                        if (SynchronousIo) {
                            FileObject->CurrentByteOffset.QuadPart =
                                ByteOffset.QuadPart + Irp->IoStatus.Information;
                        }

                        SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
                        SetLongFlag(Fcb->Flags, FCB_FILE_MODIFIED);
                    }

                    Ext2CompleteIrpContext(IrpContext, Status);
                }
            } else {
                Ext2FreeIrpContext(IrpContext);
            }
        }
    } _SEH2_END;

    DEBUG(DL_IO, ("Ext2WriteFile: %wZ written at Offset=%I64xh Length=%xh PagingIo=%d Nocache=%d "
                  "RetLen=%xh VDL=%I64xh FileSize=%I64xh i_size=%I64xh Status=%xh\n",
                  &Fcb->Mcb->ShortName, ByteOffset, Length, PagingIo, Nocache, ReturnedLength,
                  Fcb->Header.ValidDataLength.QuadPart,Fcb->Header.FileSize.QuadPart,
                  Fcb->Inode->i_size, Status));

    return Status;
}

NTSTATUS
Ext2WriteComplete (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT    FileObject;
    PIRP            Irp;
    PIO_STACK_LOCATION IrpSp;

    _SEH2_TRY {

        ASSERT(IrpContext);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        FileObject = IrpContext->FileObject;

        Irp = IrpContext->Irp;
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        CcMdlWriteComplete(FileObject, &(IrpSp->Parameters.Write.ByteOffset), Irp->MdlAddress);
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
Ext2Write (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status;
    PEXT2_FCBVCB        FcbOrVcb;
    PDEVICE_OBJECT      DeviceObject;
    PFILE_OBJECT        FileObject;
    PEXT2_VCB           Vcb;
    BOOLEAN             bCompleteRequest = TRUE;

    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
           (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

    _SEH2_TRY {

        if (IsFlagOn(IrpContext->MinorFunction, IRP_MN_COMPLETE)) {

            Status =  Ext2WriteComplete(IrpContext);
            bCompleteRequest = FALSE;

        } else {

            DeviceObject = IrpContext->DeviceObject;
            if (IsExt2FsDevice(DeviceObject)) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                _SEH2_LEAVE;
            }
            FileObject = IrpContext->FileObject;

            Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;

            if (Vcb->Identifier.Type != EXT2VCB ||
                    Vcb->Identifier.Size != sizeof(EXT2_VCB) ) {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

            if (IsVcbReadOnly(Vcb)) {
                Status = STATUS_MEDIA_WRITE_PROTECTED;
                _SEH2_LEAVE;
            }

            if (FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED) &&
                Vcb->LockFile != FileObject ) {
                Status = STATUS_ACCESS_DENIED;
                _SEH2_LEAVE;
            }

            FcbOrVcb = (PEXT2_FCBVCB) FileObject->FsContext;

            if (FcbOrVcb->Identifier.Type == EXT2VCB) {

                Status = Ext2WriteVolume(IrpContext);
                if (!NT_SUCCESS(Status)) {
                    DbgBreak();
                }
                bCompleteRequest = FALSE;

            } else if (FcbOrVcb->Identifier.Type == EXT2FCB) {

                if (IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {
                    Status = STATUS_TOO_LATE;
                    _SEH2_LEAVE;
                }

                Status = Ext2WriteFile(IrpContext);
                if (!NT_SUCCESS(Status)) {
                    DbgBreak();
                }

                bCompleteRequest = FALSE;
            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
        }

    } _SEH2_FINALLY {

        if (bCompleteRequest) {
            Ext2CompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;

    return Status;
}
