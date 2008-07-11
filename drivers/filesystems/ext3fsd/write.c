/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             write.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
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

VOID
Ext2FloppyFlush(IN PVOID Parameter);

VOID
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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2FloppyFlush)
#pragma alloc_text(PAGE, Ext2StartFloppyFlushDpc)
#pragma alloc_text(PAGE, Ext2ZeroHoles)
#pragma alloc_text(PAGE, Ext2Write)
#pragma alloc_text(PAGE, Ext2WriteVolume)
#pragma alloc_text(PAGE, Ext2WriteInode)
#pragma alloc_text(PAGE, Ext2WriteFile)
#pragma alloc_text(PAGE, Ext2WriteComplete)
#endif

/* FUNCTIONS *************************************************************/

VOID
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
    ExFreePoolWithTag(Parameter, EXT2_FLPFLUSH_MAGIC);
}

VOID
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

    Context = ExAllocatePoolWithTag(
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
Ext2ZeroHoles (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN LONGLONG Offset,
    IN LONGLONG Count
    )
{
    LARGE_INTEGER StartAddr = {0,0};
    LARGE_INTEGER EndAddr = {0,0};

    StartAddr.QuadPart = (Offset + (SECTOR_SIZE - 1)) &
                         ~((LONGLONG)SECTOR_SIZE - 1);

    if (Offset != 0 && StartAddr.QuadPart == 0) {
        return TRUE;
    }

    EndAddr.QuadPart = (Offset + Count + (SECTOR_SIZE - 1)) &
                       ~((LONGLONG)SECTOR_SIZE - 1);

    if (StartAddr.QuadPart < EndAddr.QuadPart) {
        return CcZeroData( FileObject,
                       &StartAddr,
                       &EndAddr,
                       IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );
    }

    return TRUE;
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

    BOOLEAN             PagingIo;
    BOOLEAN             Nocache;
    BOOLEAN             SynchronousIo;
    BOOLEAN             MainResourceAcquired = FALSE;

    BOOLEAN             bDeferred = FALSE;

    PUCHAR              Buffer = NULL;
    PEXT2_EXTENT        Chain = NULL;
    EXT2_EXTENT         BlockArray;

    __try {

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
            __leave;
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

        DEBUG(DL_INF, ( "Ext2WriteVolume: Off=%I64xh Len=%xh Paging=%xh Nocache=%xh\n",
                             ByteOffset.QuadPart, Length, PagingIo, Nocache));

        if (Length == 0) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            __leave;
        }

        if (Nocache &&
           (ByteOffset.LowPart & (SECTOR_SIZE - 1) ||
            Length & (SECTOR_SIZE - 1))) {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC)) {
            ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
            Status = STATUS_PENDING;
            __leave;
        }
        
        if (ByteOffset.QuadPart >=
            Vcb->PartitionInformation.PartitionLength.QuadPart  ) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_END_OF_FILE;
            __leave;
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

                    __leave;
                }
            }
        }

        /*
         * User direct volume access
         */
    
        if (Ccb != NULL && !PagingIo) {

            ExAcquireResourceExclusiveLite(&Vcb->MainResource, TRUE);
            MainResourceAcquired = TRUE;

            if (!FlagOn(Ccb->Flags, CCB_VOLUME_DASD_PURGE)) {

                if (!IsFlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
                    Status = Ext2PurgeVolume( Vcb, TRUE);
                }

                SetFlag(Ccb->Flags, CCB_VOLUME_DASD_PURGE);
            }

            if (!IsFlagOn(Ccb->Flags, CCB_ALLOW_EXTENDED_DASD_IO)) {
                if (ByteOffset.QuadPart + Length > Vcb->Header.FileSize.QuadPart) {
                    Length = (ULONG)(Vcb->Header.FileSize.QuadPart - ByteOffset.QuadPart);
                }
            }

            ExReleaseResourceLite(&Vcb->MainResource);
            MainResourceAcquired = FALSE;

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
                __leave;
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
                    __leave;
                }

                if (!CcCopyWrite( Vcb->Volume,
                                  (PLARGE_INTEGER)(&ByteOffset),
                                  Length,
                                  TRUE,
                                  Buffer )) {
                    Status = STATUS_PENDING;
                    __leave;
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
                __leave;
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
                        __leave;
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
                                    Length,
                                    FALSE   );
                Irp = IrpContext->Irp;

                if (NT_SUCCESS(Status)) {
                    for (Extent = Chain; Extent != NULL; Extent = Extent->Next) {
                        Ext2RemoveVcbExtent(Vcb, Extent->Lba, Extent->Length);
                    }
                }

                if (!Irp) {
                    __leave;
                }

            } else {

                Irp->IoStatus.Information = Length;
                Status = STATUS_SUCCESS;
                __leave;
            }

        } else {

            Length &= ~((ULONG)SECTOR_SIZE - 1);

            Status = Ext2LockUserBuffer(
                IrpContext->Irp,
                Length,
                IoWriteAccess );
                
            if (!NT_SUCCESS(Status)) {
                __leave;
            }

            BlockArray.Irp = NULL;
            BlockArray.Lba = ByteOffset.QuadPart;
            BlockArray.Offset = 0;
            BlockArray.Length = Length;
            BlockArray.Next = NULL;

            Status = Ext2ReadWriteBlocks(IrpContext,
                                Vcb,
                                &BlockArray,
                                Length,
                                FALSE   );

            if (NT_SUCCESS(Status)) {
                Irp->IoStatus.Information = Length;
            }

            Irp = IrpContext->Irp;
            if (!Irp) {
                __leave;
            }
        }

    } __finally {

        if (MainResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {
            if (Irp) {
                if (Status == STATUS_PENDING) {
                    if(!bDeferred) {
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
    }

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
    OUT PULONG              dwRet
    )
{
    PEXT2_EXTENT    Chain = NULL;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;

    BOOLEAN         bAlloc = FALSE;

    __try {

        if (dwRet) {
            *dwRet = 0;
        }

        /* For file/non pagingio, we support the allocation on writing. */
 
        if (!IsFlagOn(Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)) {
            if (!(IrpContext->Irp->Flags  & IRP_PAGING_IO)) {
                bAlloc = TRUE;
            }
        }

        Status = Ext2BuildExtents (
                    IrpContext,
                    Vcb,
                    Mcb,
                    Offset,
                    Size,
                    bAlloc,
                    &Chain
                    );

        if (!NT_SUCCESS(Status)) {
            __leave;
        }

        if (Chain == NULL) {
            Status = STATUS_SUCCESS;
            __leave;
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
                        Size,
                        FALSE
                        );

        } else {

            PEXT2_EXTENT Extent;
            for (Extent = Chain; Extent != NULL; Extent = Extent->Next) {

                if( !Ext2SaveBuffer(
                        IrpContext,
                        Vcb,
                        Extent->Lba,
                        Extent->Length,
                        (PVOID)((PUCHAR)Buffer + Extent->Offset)
                       )) {
                    __leave;
                }
            }

            if (IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK)) {

                DEBUG(DL_FLP, ("Ext2WriteInode is starting FlushingDpc...\n"));
                Ext2StartFloppyFlushDpc(Vcb, NULL, NULL);
            }

            Status = STATUS_SUCCESS;
        }

    } __finally {

        if (Chain) {
            Ext2DestroyExtentChain(Chain);
        }

        if (NT_SUCCESS(Status)) {
            if (dwRet) *dwRet = Size;
        }
    }

    return Status;
}

NTSTATUS
Ext2WriteFile(IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PEXT2_VCB           Vcb;
    PEXT2_FCB           Fcb;
    PEXT2_CCB           Ccb;
    PFILE_OBJECT        FileObject;
    PFILE_OBJECT        CacheObject;

    PDEVICE_OBJECT      DeviceObject;

    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;

    ULONG               Length;
    LARGE_INTEGER       ByteOffset;
    ULONG               ReturnedLength = 0;

    BOOLEAN             OpPostIrp = FALSE;
    BOOLEAN             PagingIo;
    BOOLEAN             Nocache;
    BOOLEAN             SynchronousIo;

    BOOLEAN             RecursiveWriteThrough = FALSE;
    BOOLEAN             MainResourceAcquired = FALSE;
    BOOLEAN             PagingIoResourceAcquired = FALSE;

    BOOLEAN             bDeferred = FALSE;

    PUCHAR              Buffer;

    __try {

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

        DEBUG(DL_INF, ( "Ext2WriteFile: %wZ Offset=%I64xh Length=%xh Paging=%xh Nocache=%xh\n",
                             &Fcb->Mcb->ShortName, ByteOffset.QuadPart, Length, PagingIo, Nocache));

        if (Length == 0) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            __leave;
        }

        if (Nocache && (ByteOffset.LowPart & (SECTOR_SIZE - 1))) {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC)) {
            ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
            Status = STATUS_PENDING;
            __leave;
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
                    __leave;
                }
            }
        }

        if (IsEndOfFile(ByteOffset)) {
            ByteOffset.QuadPart = Fcb->Header.FileSize.QuadPart;
        }

        if (IsDirectory(Fcb) && !PagingIo) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }

        if (IsFlagOn(Irp->Flags, IRP_SYNCHRONOUS_PAGING_IO) && !IrpContext->IsTopLevel) {

            PIRP TopIrp;

            TopIrp = IoGetTopLevelIrp();

            if ( (ULONG_PTR)TopIrp > FSRTL_MAX_TOP_LEVEL_IRP_FLAG &&
                 NodeType(TopIrp) == IO_TYPE_IRP) {

                PIO_STACK_LOCATION IrpStack;

                IrpStack = IoGetCurrentIrpStackLocation(TopIrp);

                if ((IrpStack->MajorFunction == IRP_MJ_WRITE) &&
                    (IrpStack->FileObject->FsContext == FileObject->FsContext)) {

                    RecursiveWriteThrough = TRUE;
                }
            }
        }

        //
        //  Do flushing for such cases
        //
        if (Nocache && !PagingIo && Ccb != NULL && 
            (Fcb->SectionObject.DataSectionObject != NULL))  {

            MainResourceAcquired =
                ExAcquireResourceExclusiveLite( &Fcb->MainResource, 
                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT));

            ExAcquireSharedStarveExclusive( &Fcb->PagingIoResource, TRUE);
            ExReleaseResourceLite(&Fcb->PagingIoResource);

            CcFlushCache( &(Fcb->SectionObject),
                          &ByteOffset,
                          CEILING_ALIGNED(ULONG, Length, BLOCK_SIZE),
                          &(Irp->IoStatus));
            ClearLongFlag(Fcb->Flags, FCB_FILE_MODIFIED);

            if (!NT_SUCCESS(Irp->IoStatus.Status)) {
                Status = Irp->IoStatus.Status;
                __leave;
            }

            ExAcquireSharedStarveExclusive( &Fcb->PagingIoResource, TRUE);
            ExReleaseResourceLite(&Fcb->PagingIoResource);

            CcPurgeCacheSection( &(Fcb->SectionObject),
                                 (PLARGE_INTEGER)&(ByteOffset),
                                 CEILING_ALIGNED(ULONG, Length, BLOCK_SIZE),
                                 FALSE );

            if (MainResourceAcquired) {
                ExReleaseResourceLite(&Fcb->MainResource);
                MainResourceAcquired = FALSE;
            }
        }
        
        if (!PagingIo) {

            if (!ExAcquireResourceExclusiveLite(
                &Fcb->MainResource,
                IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
                Status = STATUS_PENDING;
                __leave;
            }
            
            MainResourceAcquired = TRUE;

            if (!FsRtlCheckLockForWriteAccess(
                &Fcb->FileLockAnchor,
                Irp         )) {
                Status = STATUS_FILE_LOCK_CONFLICT;
                __leave;
            }

        } else {

            if (!ExAcquireResourceSharedLite(
                 &Fcb->PagingIoResource,
                 IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
                Status = STATUS_PENDING;
                __leave;
            }

            PagingIoResourceAcquired = TRUE;
        }
        
        if (!IsDirectory(Fcb) && Ccb != NULL) {

            Status = FsRtlCheckOplock( &Fcb->Oplock,
                                       Irp,
                                       IrpContext,
                                       Ext2OplockComplete,
                                       Ext2LockIrp );

            if (Status != STATUS_SUCCESS) {
                OpPostIrp = TRUE;
                __leave;
            }

            //
            //  Set the flag indicating if Fast I/O is possible
            //

            Fcb->Header.IsFastIoPossible = Ext2IsFastIoPossible(Fcb);
        }

        if (PagingIo) {

            if ( (ByteOffset.QuadPart + Length) > Fcb->Header.FileSize.QuadPart) {

                if ( ByteOffset.QuadPart >= Fcb->Header.FileSize.QuadPart) {

                    Status = STATUS_SUCCESS;
                    Irp->IoStatus.Information = 0;
                    __leave;

                } else {

                    Length = (ULONG)(Fcb->Header.FileSize.QuadPart - ByteOffset.QuadPart);
                }
            }

        } else {

            if (IsDirectory(Fcb)) {
                __leave;
            }

            //
            //  Extend the inode size when the i/o is beyond the file end ?
            //

            if ((ByteOffset.QuadPart + Length) > (Fcb->Header.FileSize.QuadPart)) {

                LARGE_INTEGER   ExtendSize, AllocationSize;

                ExtendSize.QuadPart = (LONGLONG)(ByteOffset.QuadPart + Length);
                AllocationSize.QuadPart = CEILING_ALIGNED(ULONGLONG, 
                                            (ULONGLONG)ExtendSize.QuadPart,
                                            (ULONGLONG)BLOCK_SIZE);
                if (AllocationSize.QuadPart > Fcb->Header.AllocationSize.QuadPart) {
                    Status = Ext2ExpandFile(IrpContext, Vcb, Fcb->Mcb, &AllocationSize);
                    if (NT_SUCCESS(Status)) {
                        Fcb->Header.AllocationSize.QuadPart = AllocationSize.QuadPart;
                        if (CcIsFileCached(FileObject)) {
                            CcSetFileSizes(FileObject, 
                                (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
                        }
                        SetLongFlag(Fcb->Flags, FCB_ALLOC_IN_WRITE);
                        ClearLongFlag(Fcb->Flags, FCB_ALLOC_IN_CREATE);
                    } else {
                        __leave;
                    }
                }

                Fcb->Header.FileSize.QuadPart =
                Fcb->Mcb->FileSize.QuadPart = ExtendSize.QuadPart;

                Fcb->Inode->i_size = ExtendSize.LowPart;
                if (S_ISREG(Fcb->Inode->i_mode)) {
                    Fcb->Inode->i_size_high = (ULONG) ExtendSize.HighPart;
                }
                Ext2SaveInode(IrpContext, Vcb, Fcb->Mcb->iNo, Fcb->Inode);

                if (Fcb->Header.FileSize.QuadPart >= 0x80000000 && 
                    !IsFlagOn(SUPER_BLOCK->s_feature_ro_compat, EXT2_FEATURE_RO_COMPAT_LARGE_FILE)) {
                    SetFlag(SUPER_BLOCK->s_feature_ro_compat, EXT2_FEATURE_RO_COMPAT_LARGE_FILE);
                    Ext2SaveSuper(IrpContext, Vcb);
                }

                if (CcIsFileCached(FileObject)) {
                    CcSetFileSizes(FileObject, 
                        (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
                }

                Ext2NotifyReportChange(
                        IrpContext,
                        Vcb,
                        Fcb->Mcb,
                        FILE_NOTIFY_CHANGE_SIZE,
                        FILE_ACTION_MODIFIED );

                DEBUG(DL_INF, ( "Ext2WriteFile: expanding %wZ to %I64xh\n", &Fcb->Mcb->ShortName, Fcb->Header.FileSize.QuadPart));
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

                CcSetFileSizes(
                        FileObject, 
                        (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
            }

            CacheObject = FileObject;

            if (ByteOffset.QuadPart > Fcb->Header.ValidDataLength.QuadPart) {
                Ext2ZeroHoles( IrpContext, Vcb, FileObject, Fcb->Header.ValidDataLength.QuadPart,
                               ByteOffset.QuadPart - Fcb->Header.ValidDataLength.QuadPart );
            }

            if (FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                CcPrepareMdlWrite(
                    CacheObject,
                    (&ByteOffset),
                    Length,
                    &Irp->MdlAddress,
                    &Irp->IoStatus );
                
                Status = Irp->IoStatus.Status;

            } else {

                Buffer = Ext2GetUserBuffer(Irp);
                
                if (Buffer == NULL) {
                    DbgBreak();
                    Status = STATUS_INVALID_USER_BUFFER;
                    __leave;
                }
                
                if (!CcCopyWrite(
                        CacheObject,
                        (PLARGE_INTEGER)&ByteOffset,
                        Length,
                        IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                        Buffer  )) {
                    Status = STATUS_PENDING;
                    DbgBreak();
                    __leave;
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

            Length = (Length + SECTOR_SIZE - 1) & (~(SECTOR_SIZE - 1));
            if (CcIsFileCached(FileObject) && !RecursiveWriteThrough && 
                Fcb->LazyWriterThread != PsGetCurrentThread() &&
                ByteOffset.QuadPart > Fcb->Header.ValidDataLength.QuadPart) {

                Ext2ZeroHoles( IrpContext, Vcb, FileObject, Fcb->Header.ValidDataLength.QuadPart,
                               ByteOffset.QuadPart - Fcb->Header.ValidDataLength.QuadPart );
            }

            Status = Ext2LockUserBuffer(
                IrpContext->Irp,
                Length,
                IoReadAccess );
                
            if (!NT_SUCCESS(Status)) {
                __leave;
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
        }

         /* Update files's ValiDateLength */
         if ( NT_SUCCESS(Status) && !RecursiveWriteThrough &&
              Fcb->LazyWriterThread != PsGetCurrentThread() &&
              (ByteOffset.QuadPart + ReturnedLength) > (Fcb->Header.ValidDataLength.QuadPart)) {
            if (Fcb->Header.FileSize.QuadPart > ByteOffset.QuadPart + ReturnedLength) {
                Fcb->Header.ValidDataLength.QuadPart = ByteOffset.QuadPart + ReturnedLength;
            } else {
                Fcb->Header.ValidDataLength.QuadPart = Fcb->Header.FileSize.QuadPart;
            }
         }

         DEBUG(DL_INF, ( "Ext2WriteFile: Writing %wZ at Offset=%I64xh Length=%xh RetLen=%xh VDL=%I64xh done! (%xh)\n",
                  &Fcb->Mcb->ShortName, ByteOffset, Length, ReturnedLength, 
                  Fcb->Header.ValidDataLength.QuadPart, Status));

    } __finally {

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

                    if (NT_SUCCESS(Status)) {

                        if (SynchronousIo && !PagingIo) {
                            FileObject->CurrentByteOffset.QuadPart =
                                ByteOffset.QuadPart + Irp->IoStatus.Information;
                        }
                    
                        if (!PagingIo) {
                            SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
                            SetLongFlag(Fcb->Flags, FCB_FILE_MODIFIED);
                        }
                    }

                    Ext2CompleteIrpContext(IrpContext, Status);
                }
            } else {
                Ext2FreeIrpContext(IrpContext);
            }
        }
    }
    
    return Status;
}

NTSTATUS
Ext2WriteComplete (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT    FileObject;
    PIRP            Irp;
    PIO_STACK_LOCATION IrpSp;
    
    __try {

        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        FileObject = IrpContext->FileObject;
        
        Irp = IrpContext->Irp;
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        
        CcMdlWriteComplete(FileObject, &(IrpSp->Parameters.Write.ByteOffset), Irp->MdlAddress);
        
        Irp->MdlAddress = NULL;
        
        Status = STATUS_SUCCESS;

    } __finally {

        if (!IrpContext->ExceptionInProgress) {
            Ext2CompleteIrpContext(IrpContext, Status);
        }
    }
    
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

    __try {

        if (IsFlagOn(IrpContext->MinorFunction, IRP_MN_COMPLETE)) {

            Status =  Ext2WriteComplete(IrpContext);
            bCompleteRequest = FALSE;

        } else {

            DeviceObject = IrpContext->DeviceObject;

            if (IsExt2FsDevice(DeviceObject)) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                __leave;
            }

            Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;

            if (Vcb->Identifier.Type != EXT2VCB ||
                Vcb->Identifier.Size != sizeof(EXT2_VCB) ) {
                 Status = STATUS_INVALID_PARAMETER;
                __leave;
            }

            if (IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {
                Status = STATUS_TOO_LATE;
                __leave;
            }

            if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                Status = STATUS_MEDIA_WRITE_PROTECTED;
                __leave;
            }

            FileObject = IrpContext->FileObject;
            
            FcbOrVcb = (PEXT2_FCBVCB) FileObject->FsContext;

            if (FcbOrVcb->Identifier.Type == EXT2VCB) {

                Status = Ext2WriteVolume(IrpContext);

                if (!NT_SUCCESS(Status)) {
                    DbgBreak();
                }

                bCompleteRequest = FALSE;
            } else if (FcbOrVcb->Identifier.Type == EXT2FCB) {
                Status = Ext2WriteFile(IrpContext);

                if (!NT_SUCCESS(Status)) {
                    DbgBreak();
                }

                bCompleteRequest = FALSE;
            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
        }

    } __finally {

        if (bCompleteRequest) {
            Ext2CompleteIrpContext(IrpContext, Status);
        }
    }
    
    return Status;
}
