/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             write.c
 * PURPOSE:          
 * PROGRAMMER:       Mark Piper, Matt Wu, Bo Brantén.
 * HOMEPAGE:         
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsd.h"

/* GLOBALS ***************************************************************/

extern PRFSD_GLOBAL RfsdGlobal;

/* DEFINITIONS *************************************************************/

typedef struct _RFSD_FLPFLUSH_CONTEXT {

    PRFSD_VCB    Vcb;
    PRFSD_FCB    Fcb;
    PFILE_OBJECT FileObject;

    KDPC        Dpc;
    KTIMER      Timer;
    WORK_QUEUE_ITEM Item;

} RFSD_FLPFLUSH_CONTEXT, *PRFSD_FLPFLUSH_CONTEXT;

#ifdef _PREFAST_
WORKER_THREAD_ROUTINE RfsdFloppyFlush;
#endif // _PREFAST_

#ifndef __REACTOS__
VOID
RfsdFloppyFlush(IN PVOID Parameter);
#else
VOID
NTAPI
RfsdFloppyFlush(_In_ PVOID Parameter);
#endif

#ifdef _PREFAST_
KDEFERRED_ROUTINE RfsdFloppyFlushDpc;
#endif // _PREFAST_

#ifndef __REACTOS__
VOID
RfsdFloppyFlushDpc (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2);
#else
VOID
NTAPI
RfsdFloppyFlushDpc (
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2);
#endif

NTSTATUS
RfsdWriteComplete (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdWriteFile (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdWriteVolume (IN PRFSD_IRP_CONTEXT IrpContext);

VOID
RfsdDeferWrite(IN PRFSD_IRP_CONTEXT, PIRP Irp);

#ifdef ALLOC_PRAGMA
#if !RFSD_READ_ONLY
#pragma alloc_text(PAGE, RfsdFloppyFlush)
#pragma alloc_text(PAGE, RfsdStartFloppyFlushDpc)
#pragma alloc_text(PAGE, RfsdZeroHoles)
#pragma alloc_text(PAGE, RfsdWrite)
#pragma alloc_text(PAGE, RfsdWriteVolume)
#pragma alloc_text(PAGE, RfsdWriteInode)
#pragma alloc_text(PAGE, RfsdWriteFile)
#pragma alloc_text(PAGE, RfsdWriteComplete)
#endif // !RFSD_READ_ONLY
#endif

/* FUNCTIONS *************************************************************/

#if !RFSD_READ_ONLY

VOID
RfsdFloppyFlush(IN PVOID Parameter)
{
    PRFSD_FLPFLUSH_CONTEXT Context;
    PFILE_OBJECT           FileObject;
    PRFSD_FCB Fcb;
    PRFSD_VCB Vcb;

    PAGED_CODE();

    Context = (PRFSD_FLPFLUSH_CONTEXT) Parameter;
    FileObject = Context->FileObject;
    Fcb = Context->Fcb;
    Vcb = Context->Vcb;

    RfsdPrint((DBG_USER, "RfsdFloppyFlushing ...\n"));

    FsRtlEnterFileSystem();

    IoSetTopLevelIrp((PIRP)FSRTL_FSP_TOP_LEVEL_IRP);

    if (Vcb) {
        ExAcquireSharedStarveExclusive(&Vcb->PagingIoResource, TRUE);
        ExReleaseResourceLite(&Vcb->PagingIoResource);

        CcFlushCache(&(Vcb->SectionObject), NULL, 0, NULL);
    }

    if (FileObject) {
        ASSERT(Fcb == (PRFSD_FCB)FileObject->FsContext);

        ExAcquireSharedStarveExclusive(&Fcb->PagingIoResource, TRUE);
        ExReleaseResourceLite(&Fcb->PagingIoResource);

        CcFlushCache(&(Fcb->SectionObject), NULL, 0, NULL);

        ObDereferenceObject(FileObject);
    }

    IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    ExFreePool(Parameter);
}

VOID
RfsdFloppyFlushDpc (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    PRFSD_FLPFLUSH_CONTEXT Context;

    Context = (PRFSD_FLPFLUSH_CONTEXT) DeferredContext;

    RfsdPrint((DBG_USER, "RfsdFloppyFlushDpc is to be started...\n"));

    ExInitializeWorkItem( &Context->Item,
                          RfsdFloppyFlush,
                          Context );

    ExQueueWorkItem(&Context->Item, CriticalWorkQueue);
}

VOID
RfsdStartFloppyFlushDpc (
    PRFSD_VCB   Vcb,
    PRFSD_FCB   Fcb,
    PFILE_OBJECT FileObject )
{
    LARGE_INTEGER          OneSecond;
    PRFSD_FLPFLUSH_CONTEXT Context;

    PAGED_CODE();

    ASSERT(IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK));

    Context = ExAllocatePoolWithTag(NonPagedPool, sizeof(PRFSD_FLPFLUSH_CONTEXT), RFSD_POOL_TAG);

    if (!Context) {
        DbgBreak();
        return;
    }

    KeInitializeTimer(&Context->Timer);

    KeInitializeDpc( &Context->Dpc,
                     RfsdFloppyFlushDpc,
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
RfsdZeroHoles (
    IN PRFSD_IRP_CONTEXT IrpContext,
    IN PRFSD_VCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN LONGLONG Offset,
    IN LONGLONG Count
    )
{
    LARGE_INTEGER StartAddr = {0,0};
    LARGE_INTEGER EndAddr = {0,0};

    PAGED_CODE();

    StartAddr.QuadPart = (Offset + (SECTOR_SIZE - 1)) &
                         ~((LONGLONG)SECTOR_SIZE - 1);

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
RfsdDeferWrite(IN PRFSD_IRP_CONTEXT IrpContext, PIRP Irp)
{
    ASSERT(IrpContext->Irp == Irp);

    RfsdQueueRequest(IrpContext);
}

NTSTATUS
RfsdWriteVolume (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PRFSD_VCB           Vcb;
    PRFSD_CCB           Ccb;
    PRFSD_FCBVCB        FcbOrVcb;
    PFILE_OBJECT        FileObject;

    PDEVICE_OBJECT      DeviceObject;

    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;

    ULONG               Length;
    LARGE_INTEGER       ByteOffset;

    BOOLEAN             PagingIo;
    BOOLEAN             Nocache;
    BOOLEAN             SynchronousIo;
    BOOLEAN             MainResourceAcquired = FALSE;
    BOOLEAN             PagingIoResourceAcquired = FALSE;

    BOOLEAN             bDeferred = FALSE;

    PUCHAR              Buffer;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
    
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));
        
        FileObject = IrpContext->FileObject;

        FcbOrVcb = (PRFSD_FCBVCB) FileObject->FsContext;
        
        ASSERT(FcbOrVcb);
        
        if (!(FcbOrVcb->Identifier.Type == RFSDVCB && (PVOID)FcbOrVcb == (PVOID)Vcb)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        Ccb = (PRFSD_CCB) FileObject->FsContext2;

        Irp = IrpContext->Irp;
            
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
            
        Length = IoStackLocation->Parameters.Write.Length;
        ByteOffset = IoStackLocation->Parameters.Write.ByteOffset;
            
        PagingIo = (Irp->Flags & IRP_PAGING_IO ? TRUE : FALSE);
        Nocache = (Irp->Flags & IRP_NOCACHE ? TRUE : FALSE);
        SynchronousIo = (FileObject->Flags & FO_SYNCHRONOUS_IO ? TRUE : FALSE);

        RfsdPrint((DBG_INFO, "RfsdWriteVolume: Off=%I64xh Len=%xh Paging=%xh Nocache=%xh\n",
                             ByteOffset.QuadPart, Length, PagingIo, Nocache));

        if (Length == 0) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }
      
        // For the case of "Direct Access Storage Device", we
        // need flush/purge the cache
    
        if (Ccb != NULL) {

            ExAcquireResourceExclusiveLite(&Vcb->MainResource, TRUE);
            MainResourceAcquired = TRUE;

            Status = RfsdPurgeVolume( Vcb, TRUE);

            ExReleaseResourceLite(&Vcb->MainResource);
            MainResourceAcquired = FALSE;

            if(!IsFlagOn(Ccb->Flags, CCB_ALLOW_EXTENDED_DASD_IO)) {
                if (ByteOffset.QuadPart + Length > Vcb->Header.FileSize.QuadPart) {
                    Length = (ULONG)(Vcb->Header.FileSize.QuadPart - ByteOffset.QuadPart);
                }
            }

            {
                RFSD_BDL BlockArray;

                if ((ByteOffset.LowPart & (SECTOR_SIZE - 1)) ||
                   (Length & (SECTOR_SIZE - 1)) ) {
                    Status = STATUS_INVALID_PARAMETER;
                    _SEH2_LEAVE;
                }

                Status = RfsdLockUserBuffer(
                    IrpContext->Irp,
                    Length,
                    IoReadAccess );
                
                if (!NT_SUCCESS(Status)) {
                    _SEH2_LEAVE;
                }

                BlockArray.Irp = NULL;
                BlockArray.Lba = ByteOffset.QuadPart;;
                BlockArray.Offset = 0;
                BlockArray.Length = Length;

                Status = RfsdReadWriteBlocks(IrpContext,
                                    Vcb,
                                    &BlockArray,
                                    Length,
                                    1,
                                    FALSE   );
                Irp = IrpContext->Irp;

                _SEH2_LEAVE;
            }
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

#if FALSE

        if (!Nocache) {

            BOOLEAN bAgain = IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED);
            BOOLEAN bWait  = IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
            BOOLEAN bQueue = IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED);

            if ( !CcCanIWrite( 
                      FileObject,
                      Length,
                      (bWait && bQueue),
                      bAgain ) ) {

                SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED);

                CcDeferWrite( FileObject,
                              (PCC_POST_DEFERRED_WRITE)RfsdDeferWrite,
                              IrpContext,
                              Irp,
                              Length,
                              bAgain );

                bDeferred = TRUE;

                DbgBreak();

                Status = STATUS_PENDING;

                _SEH2_LEAVE;
            }
        }

#endif

        if (Nocache && !PagingIo && (Vcb->SectionObject.DataSectionObject != NULL))  {

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

        if (!PagingIo) {

#pragma prefast( suppress: 28137, "by design" )
            if (!ExAcquireResourceExclusiveLite(
                &Vcb->MainResource,
                IrpContext->IsSynchronous )) {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            
            MainResourceAcquired = TRUE;

        } else {

/*
            ULONG ResShCnt, ResExCnt; 
            ResShCnt = ExIsResourceAcquiredSharedLite(&Vcb->PagingIoResource);
            ResExCnt = ExIsResourceAcquiredExclusiveLite(&Vcb->PagingIoResource);

            RfsdPrint((DBG_USER, "PagingIoRes: %xh:%xh Synchronous=%xh\n", ResShCnt, ResExCnt, IrpContext->IsSynchronous));
*/

            if (Ccb) {

                if (!ExAcquireResourceSharedLite(
                    &Vcb->PagingIoResource,
                    IrpContext->IsSynchronous )) {
                    Status = STATUS_PENDING;
                    _SEH2_LEAVE;
                }
            
                PagingIoResourceAcquired = TRUE;
            }
        }
        
        if (!Nocache) {

            if ( (ByteOffset.QuadPart + Length) >
                 Vcb->PartitionInformation.PartitionLength.QuadPart ){
                Length = (ULONG) (
                    Vcb->PartitionInformation.PartitionLength.QuadPart -
                    ByteOffset.QuadPart);

                Length &= ~((ULONG)SECTOR_SIZE - 1);
            }

            if (FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                CcPrepareMdlWrite (
                    Vcb->StreamObj,
                    &ByteOffset,
                    Length,
                    &Irp->MdlAddress,
                    &Irp->IoStatus );
                
                Status = Irp->IoStatus.Status;

            } else {

                Buffer = RfsdGetUserBuffer(Irp);
                    
                if (Buffer == NULL) {
                    DbgBreak();

                    Status = STATUS_INVALID_USER_BUFFER;
                    _SEH2_LEAVE;
                }

                if (!CcCopyWrite( Vcb->StreamObj,
                                  (PLARGE_INTEGER)(&ByteOffset),
                                  Length,
                                  TRUE,
                                  Buffer )) {
                    Status = STATUS_PENDING;
                    _SEH2_LEAVE;
                }

                Status = Irp->IoStatus.Status;
                RfsdAddMcbEntry(Vcb, ByteOffset.QuadPart, (LONGLONG)Length);
            }

            if (NT_SUCCESS(Status)) {
                Irp->IoStatus.Information = Length;
            }

        } else {

            PRFSD_BDL           rfsd_bdl = NULL;
            ULONG               Blocks = 0;

            LONGLONG            DirtyStart;
            LONGLONG            DirtyLba;
            LONGLONG            DirtyLength;
            LONGLONG            RemainLength;

            if ((ByteOffset.QuadPart + Length) >
                    Vcb->PartitionInformation.PartitionLength.QuadPart ) {
                Length = (ULONG) (
                    Vcb->PartitionInformation.PartitionLength.QuadPart -
                    ByteOffset.QuadPart);

                Length &= ~((ULONG)SECTOR_SIZE - 1);
            }

            Status = RfsdLockUserBuffer(
                IrpContext->Irp,
                Length,
                IoReadAccess );
                
            if (!NT_SUCCESS(Status)) {
                _SEH2_LEAVE;
            }

            rfsd_bdl = ExAllocatePoolWithTag(PagedPool, 
                                       (Length / Vcb->BlockSize) *
                                       sizeof(RFSD_BDL), RFSD_POOL_TAG);

            if (!rfsd_bdl) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH2_LEAVE;
            }

            DirtyLba = ByteOffset.QuadPart;
            RemainLength = (LONGLONG) Length;

            while (RemainLength > 0) {

                DirtyStart = DirtyLba;

                if (RfsdLookupMcbEntry( Vcb, 
                                        DirtyStart,
                                        &DirtyLba,
                                        &DirtyLength,
                                        (PLONGLONG)NULL,
                                        (PLONGLONG)NULL,
                                        (PULONG)NULL) ) {

                    if (DirtyLba == -1) {
                        DirtyLba = DirtyStart + DirtyLength;

                        RemainLength = ByteOffset.QuadPart + 
                                       (LONGLONG)Length -
                                       DirtyLba;
                        continue;
                    }
            
                    rfsd_bdl[Blocks].Irp = NULL;
                    rfsd_bdl[Blocks].Lba = DirtyLba;
                    rfsd_bdl[Blocks].Offset = (ULONG)( (LONGLONG)Length +
                                                       DirtyStart -
                                                       RemainLength - 
                                                       DirtyLba );

                    if (DirtyLba + DirtyLength > DirtyStart + RemainLength) {
                        rfsd_bdl[Blocks].Length = (ULONG)( DirtyStart +
                                                           RemainLength -
                                                           DirtyLba );
                        RemainLength = 0;
                    } else {
                        rfsd_bdl[Blocks].Length = (ULONG)DirtyLength;
                        RemainLength =  (DirtyStart + RemainLength) -
                                        (DirtyLba + DirtyLength);
                    }

                    DirtyLba = DirtyStart + DirtyLength;
                    Blocks++;

                } else {

                    if (Blocks == 0) {

                        if (rfsd_bdl)
                            ExFreePool(rfsd_bdl);

                        //
                        // Lookup fails at the first time, ie. 
                        // no dirty blocks in the run
                        //

                        DbgBreak();

                        if (RemainLength == (LONGLONG)Length)
                            Status = STATUS_SUCCESS;
                        else
                            Status = STATUS_UNSUCCESSFUL;

                        _SEH2_LEAVE;

                    } else {
                        break;
                    }
                }
            }

            if (Blocks > 0) {

                Status = RfsdReadWriteBlocks(IrpContext,
                                    Vcb,
                                    rfsd_bdl,
                                    Length,
                                    Blocks,
                                    FALSE   );
                Irp = IrpContext->Irp;

                if (NT_SUCCESS(Status)) {
                    ULONG   i;

                    for (i=0; i<Blocks;i++) {
                        RfsdRemoveMcbEntry( Vcb,
                                            rfsd_bdl[i].Lba,
                                            rfsd_bdl[i].Length );
                    }
                }

                if (rfsd_bdl)
                    ExFreePool(rfsd_bdl);

                if (!Irp)
                    _SEH2_LEAVE;

            } else {

                if (rfsd_bdl)
                    ExFreePool(rfsd_bdl);

                Irp->IoStatus.Information = Length;
    
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }
        }
    } _SEH2_FINALLY {

        if (PagingIoResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->PagingIoResource,
                ExGetCurrentResourceThread());
        }
        
        if (MainResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread());
        }

        if (!IrpContext->ExceptionInProgress) {
            if (Irp) {
                if (Status == STATUS_PENDING) {
                    if(!bDeferred) {
                        Status = RfsdLockUserBuffer(
                            IrpContext->Irp,
                            Length,
                            IoReadAccess );
                    
                        if (NT_SUCCESS(Status)) {
                            Status = RfsdQueueRequest(IrpContext);
                        } else {
                            RfsdCompleteIrpContext(IrpContext, Status);
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

                    RfsdCompleteIrpContext(IrpContext, Status);
                }
            } else {
                RfsdFreeIrpContext(IrpContext);
            }
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
RfsdWriteInode (
    IN PRFSD_IRP_CONTEXT    IrpContext,
    IN PRFSD_VCB            Vcb,
    IN ULONG                InodeNo,
    IN PRFSD_INODE          Inode,
    IN ULONGLONG            Offset,
    IN PVOID                Buffer,
    IN ULONG                Size,
    IN BOOLEAN              bWriteToDisk,
    OUT PULONG              dwRet
    )
{
    PRFSD_BDL       rfsd_bdl = NULL;
    ULONG           blocks, i;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;

    ULONGLONG       FileSize;
    ULONGLONG       AllocSize;

    BOOLEAN         bAlloc = FALSE;

    PAGED_CODE();
#if 0
    if (dwRet) {
        *dwRet = 0;
    }

    //
    // For file/non pagingio, we support the allocation on writing.
    //

    if (S_ISREG(Inode->i_mode)) {

        if (!(IrpContext->Irp->Flags  & IRP_PAGING_IO)) {
            bAlloc = TRUE;
        }
    }

    //
    // Initialize the FileSize / AllocationSize ...
    //

    FileSize  = (ULONGLONG) Inode->i_size;
    if (S_ISREG(Inode->i_mode))
        FileSize |= ((ULONGLONG)(Inode->i_size_high) << 32);
    AllocSize = CEILING_ALIGNED(FileSize, (ULONGLONG)Vcb->BlockSize);


    //
    // Check the inputed parameters ...
    //

    if (!bAlloc) {

        if (Offset >= AllocSize) {
            RfsdPrint((DBG_ERROR, "RfsdWritenode: beyond the file range.\n"));
            return STATUS_SUCCESS;
        }

        if (Offset + Size > AllocSize) {
            Size = (ULONG)(AllocSize - Offset);
        }
    }

    Status = RfsdBuildBDL (
                IrpContext,
                Vcb,
                InodeNo,
                Inode,
                Offset,
                Size,
                bAlloc,
                &rfsd_bdl,
                &blocks
                );

    if (blocks <= 0) {
        Status = STATUS_SUCCESS;
        goto errorout;
    }

    if (bWriteToDisk) {

        //
        // We assume the offset is aligned.
        //

        Status = RfsdReadWriteBlocks(
                    IrpContext,
                    Vcb,
                    rfsd_bdl,
                    Size,
                    blocks,
                    FALSE
                    );

    } else {

        for(i = 0; i < blocks; i++) {

            if( !RfsdSaveBuffer(
                    IrpContext,
                    Vcb,
                    rfsd_bdl[i].Lba,
                    rfsd_bdl[i].Length,
                    (PVOID)((PUCHAR)Buffer + rfsd_bdl[i].Offset)
                   )) {
                goto errorout;
            }
        }

        if (IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK)) {

            RfsdPrint((DBG_USER, "RfsdWriteInode is starting FlushingDpc...\n"));
            RfsdStartFloppyFlushDpc(Vcb, NULL, NULL);
        }

        Status = STATUS_SUCCESS;
    }

errorout:

    if (rfsd_bdl)
        ExFreePool(rfsd_bdl);

    if (NT_SUCCESS(Status)) {
        if (dwRet) *dwRet = Size;
    }
#endif // 0
    return Status;
}

NTSTATUS
RfsdWriteFile(IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PRFSD_VCB           Vcb;
    PRFSD_FCB           Fcb;
    PRFSD_CCB           Ccb;
    PFILE_OBJECT        FileObject;
    PFILE_OBJECT        CacheObject;

    PDEVICE_OBJECT      DeviceObject;

    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;

    ULONG               Length;
    ULONG               ReturnedLength;
    LARGE_INTEGER       ByteOffset;

    BOOLEAN             PagingIo;
    BOOLEAN             Nocache;
    BOOLEAN             SynchronousIo;
    BOOLEAN             MainResourceAcquired = FALSE;
    BOOLEAN             PagingIoResourceAcquired = FALSE;

    BOOLEAN             bNeedExtending = FALSE;
    BOOLEAN             bAppendFile = FALSE;

    BOOLEAN             bDeferred = FALSE;

    PUCHAR              Buffer;

    PAGED_CODE();
#if 0
    _SEH2_TRY {

        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
    
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));
        
        FileObject = IrpContext->FileObject;
        
        Fcb = (PRFSD_FCB) FileObject->FsContext;
        
        ASSERT(Fcb);
    
        ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
            (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

        Ccb = (PRFSD_CCB) FileObject->FsContext2;

        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        Length = IoStackLocation->Parameters.Write.Length;
        ByteOffset = IoStackLocation->Parameters.Write.ByteOffset;
        
        PagingIo = (Irp->Flags & IRP_PAGING_IO ? TRUE : FALSE);
        Nocache = (Irp->Flags & IRP_NOCACHE ? TRUE : FALSE);
        SynchronousIo = (FileObject->Flags & FO_SYNCHRONOUS_IO ? TRUE : FALSE);

        RfsdPrint((DBG_INFO, "RfsdWriteFile: Off=%I64xh Len=%xh Paging=%xh Nocache=%xh\n",
                             ByteOffset.QuadPart, Length, PagingIo, Nocache));

/*
        if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED))
        {
            Status = STATUS_FILE_DELETED;
            _SEH2_LEAVE;
        }

        if (IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING))
        {
            Status = STATUS_DELETE_PENDING;
            _SEH2_LEAVE;
        }
*/
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

#if FALSE
        if (!Nocache) {

            BOOLEAN bAgain = IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED);
            BOOLEAN bWait  = IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
            BOOLEAN bQueue = IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED);

            if ( !CcCanIWrite( 
                      FileObject,
                      Length,
                      (bWait && bQueue),
                      bAgain ) ) {
                SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED);

                CcDeferWrite( FileObject,
                              (PCC_POST_DEFERRED_WRITE)RfsdDeferWrite,
                              IrpContext,
                              Irp,
                              Length,
                              bAgain );

                bDeferred = TRUE;

                DbgBreak();

                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
        }

#endif

        if (IsEndOfFile(ByteOffset)) {
            bAppendFile = TRUE;
            ByteOffset.QuadPart = Fcb->Header.FileSize.QuadPart;
        }

        if ( FlagOn(Fcb->RfsdMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY) && !PagingIo) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        //
        //  Do flushing for such cases
        //
        if (Nocache && !PagingIo && (Fcb->SectionObject.DataSectionObject != NULL))  {

            ExAcquireResourceExclusiveLite( &Fcb->MainResource, 
                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT));

            MainResourceAcquired = TRUE;

            ExAcquireSharedStarveExclusive( &Fcb->PagingIoResource, TRUE);
            ExReleaseResourceLite(&Fcb->PagingIoResource);

            CcFlushCache( &(Fcb->SectionObject),
                          &ByteOffset,
                          Length,
                          &(Irp->IoStatus));
            ClearFlag(Fcb->Flags, FCB_FILE_MODIFIED);

            if (!NT_SUCCESS(Irp->IoStatus.Status)) 
            {
                Status = Irp->IoStatus.Status;
                _SEH2_LEAVE;
            }

            ExAcquireSharedStarveExclusive( &Fcb->PagingIoResource, TRUE);
            ExReleaseResourceLite(&Fcb->PagingIoResource);

            CcPurgeCacheSection( &(Fcb->SectionObject),
                                 (PLARGE_INTEGER)&(ByteOffset),
                                 Length,
                                 FALSE );

            ExReleaseResourceLite(&Fcb->MainResource);
            MainResourceAcquired = FALSE;
        }
        
        if (!PagingIo) {

            if (!ExAcquireResourceExclusiveLite(
                &Fcb->MainResource,
                IrpContext->IsSynchronous )) {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            
            MainResourceAcquired = TRUE;

        } else {

/*
            ULONG ResShCnt, ResExCnt; 
            ResShCnt = ExIsResourceAcquiredSharedLite(&Fcb->PagingIoResource);
            ResExCnt = ExIsResourceAcquiredExclusiveLite(&Fcb->PagingIoResource);

            RfsdPrint((DBG_USER, "RfsdWriteFile: Inode=%xh %S PagingIo: %xh:%xh Synchronous=%xh\n",
                       Fcb->RfsdMcb->Inode, Fcb->RfsdMcb->ShortName.Buffer, ResShCnt, ResExCnt, IrpContext->IsSynchronous));
*/
            if (!ExAcquireResourceSharedLite(
                 &Fcb->PagingIoResource,
                 IrpContext->IsSynchronous )) {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }

            PagingIoResourceAcquired = TRUE;
        }
        
        if (!PagingIo) {
            if (!FsRtlCheckLockForWriteAccess(
                &Fcb->FileLockAnchor,
                Irp         )) {
                Status = STATUS_FILE_LOCK_CONFLICT;
                _SEH2_LEAVE;
            }
        }

        if (Nocache) {

            if ( (ByteOffset.QuadPart + Length) >
                 Fcb->Header.AllocationSize.QuadPart) {

                if ( ByteOffset.QuadPart >= 
                     Fcb->Header.AllocationSize.QuadPart) {

                    Status = STATUS_SUCCESS;
                    Irp->IoStatus.Information = 0;
                    _SEH2_LEAVE;

                } else {

                    if (Length > (ULONG)(Fcb->Header.AllocationSize.QuadPart
                                 - ByteOffset.QuadPart)) {
                        Length = (ULONG)(Fcb->Header.AllocationSize.QuadPart
                                 - ByteOffset.QuadPart);
                    }
                }
            }
        }

        if (!Nocache) {

            if (FlagOn(Fcb->RfsdMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)) {
                _SEH2_LEAVE;
            }

            if (FileObject->PrivateCacheMap == NULL) {

                CcInitializeCacheMap(
                        FileObject,
                        (PCC_FILE_SIZES)(&Fcb->Header.AllocationSize),
                        FALSE,
                        &RfsdGlobal->CacheManagerCallbacks,
                        Fcb );

                CcSetReadAheadGranularity(
                         FileObject,
                         READ_AHEAD_GRANULARITY );

                CcSetFileSizes(
                        FileObject, 
                        (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
            }

            CacheObject = FileObject;

            //
            //  Need extending the size of inode ?
            //
            if ( (bAppendFile) || ((ByteOffset.QuadPart + Length) >
                 (Fcb->Header.FileSize.QuadPart)) ) {

                LARGE_INTEGER   ExtendSize;
                LARGE_INTEGER   FileSize;

                bNeedExtending = TRUE;
                FileSize = Fcb->Header.FileSize;
                ExtendSize.QuadPart = (LONGLONG)(ByteOffset.QuadPart + Length);

                if (ExtendSize.QuadPart > Fcb->Header.AllocationSize.QuadPart) {
                    Status = RfsdExpandFile(IrpContext, Vcb, Fcb, &ExtendSize);
                    if (!NT_SUCCESS(Status)) {
                        _SEH2_LEAVE;
                    }
                }

                {
                    Fcb->Header.FileSize.QuadPart = ExtendSize.QuadPart;
                    Fcb->Inode->i_size = ExtendSize.LowPart;
                    Fcb->Inode->i_size_high = (ULONG) ExtendSize.HighPart;
                }

                if (FileObject->PrivateCacheMap) {

                    CcSetFileSizes(FileObject, (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));

                    if (ByteOffset.QuadPart > FileSize.QuadPart) {
                        RfsdZeroHoles( IrpContext, Vcb, FileObject, FileSize.QuadPart, 
                                       ByteOffset.QuadPart - FileSize.QuadPart);
                    }

                    if (Fcb->Header.AllocationSize.QuadPart > ExtendSize.QuadPart) {
                        RfsdZeroHoles(IrpContext, Vcb, FileObject, ExtendSize.QuadPart, 
                        Fcb->Header.AllocationSize.QuadPart - ExtendSize.QuadPart);
                    }
                }

                if (RfsdSaveInode(IrpContext, Vcb, Fcb->RfsdMcb->Inode, Fcb->Inode)) {
                    Status = STATUS_SUCCESS;
                }

                RfsdNotifyReportChange(
                        IrpContext,
                        Vcb,
                        Fcb,
                        FILE_NOTIFY_CHANGE_SIZE,
                        FILE_ACTION_MODIFIED );
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

                Buffer = RfsdGetUserBuffer(Irp);
                
                if (Buffer == NULL) {
                    DbgBreak();
                    Status = STATUS_INVALID_USER_BUFFER;
                    _SEH2_LEAVE;
                }
                
                if (!CcCopyWrite(
                        CacheObject,
                        (PLARGE_INTEGER)&ByteOffset,
                        Length,
                        IrpContext->IsSynchronous,
                        Buffer  )) {
                    Status = STATUS_PENDING;
                    _SEH2_LEAVE;
                }
                
                Status = Irp->IoStatus.Status;
            }

            if (NT_SUCCESS(Status)) {

                Irp->IoStatus.Information = Length;

                if (IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK)) {
                    RfsdPrint((DBG_USER, "RfsdWriteFile is starting FlushingDpc...\n"));
                    RfsdStartFloppyFlushDpc(Vcb, Fcb, FileObject);
                }
            }

        } else {

            ReturnedLength = Length;

            Status = RfsdLockUserBuffer(
                IrpContext->Irp,
                Length,
                IoReadAccess );
                
            if (!NT_SUCCESS(Status)) {
                _SEH2_LEAVE;
            }

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = Length;
            
            Status = RfsdWriteInode(
                        IrpContext,
                        Vcb,
                        Fcb->RfsdMcb->Inode,
                        Fcb->Inode,
                        (ULONGLONG)(ByteOffset.QuadPart),
                        NULL,
                        Length,
                        TRUE,
                        &ReturnedLength
                        );

            Irp = IrpContext->Irp;

        }

    } _SEH2_FINALLY {

        if (PagingIoResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->PagingIoResource,
                ExGetCurrentResourceThread());
        }
        
        if (MainResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread());
        }
        
        if (!IrpContext->ExceptionInProgress) {
            if (Irp) {
                if (Status == STATUS_PENDING) {
                    if (!bDeferred) {
                        Status = RfsdLockUserBuffer(
                            IrpContext->Irp,
                            Length,
                            IoReadAccess );
                    
                        if (NT_SUCCESS(Status)) {
                            Status = RfsdQueueRequest(IrpContext);
                        } else {
                            RfsdCompleteIrpContext(IrpContext, Status);
                        }
                    }
                } else {
                    if (NT_SUCCESS(Status)) {
                        if (SynchronousIo && !PagingIo) {
                            FileObject->CurrentByteOffset.QuadPart =
                                ByteOffset.QuadPart + Irp->IoStatus.Information;
                        }
                    
                        if (!PagingIo)
                        {
                            SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
                            SetFlag(Fcb->Flags, FCB_FILE_MODIFIED);
                        }
                    }

                    RfsdCompleteIrpContext(IrpContext, Status);
                }
            } else {
                RfsdFreeIrpContext(IrpContext);
            }
        }
    } _SEH2_END;
#endif // 0
    return Status;
}

NTSTATUS
RfsdWriteComplete (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT    FileObject;
    PIRP            Irp;
    PIO_STACK_LOCATION IrpSp;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        FileObject = IrpContext->FileObject;
        
        Irp = IrpContext->Irp;
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        
        CcMdlWriteComplete(FileObject, &(IrpSp->Parameters.Write.ByteOffset), Irp->MdlAddress);
        
        Irp->MdlAddress = NULL;
        
        Status = STATUS_SUCCESS;

    } _SEH2_FINALLY {

        if (!IrpContext->ExceptionInProgress) {
            RfsdCompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;
    
    return Status;
}

NTSTATUS
RfsdWrite (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status;
    PRFSD_FCBVCB        FcbOrVcb;
    PDEVICE_OBJECT      DeviceObject;
    PFILE_OBJECT        FileObject;
    PRFSD_VCB           Vcb;
    BOOLEAN             bCompleteRequest = TRUE;

    PAGED_CODE();

    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
        (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));

    _SEH2_TRY {

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_COMPLETE)) {

            Status =  RfsdWriteComplete(IrpContext);
            bCompleteRequest = FALSE;

        } else {

            DeviceObject = IrpContext->DeviceObject;

            if (DeviceObject == RfsdGlobal->DeviceObject) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                _SEH2_LEAVE;
            }

            Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;

            if (Vcb->Identifier.Type != RFSDVCB ||
                Vcb->Identifier.Size != sizeof(RFSD_VCB) ) {
                 Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

            ASSERT(IsMounted(Vcb));

            if (IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {
                Status = STATUS_TOO_LATE;
                _SEH2_LEAVE;
            }

            if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
                Status = STATUS_MEDIA_WRITE_PROTECTED;
                _SEH2_LEAVE;
            }

            FileObject = IrpContext->FileObject;
            
            FcbOrVcb = (PRFSD_FCBVCB) FileObject->FsContext;

            if (FcbOrVcb->Identifier.Type == RFSDVCB) {

                Status = RfsdWriteVolume(IrpContext);

                if (!NT_SUCCESS(Status)) {
                    DbgBreak();
                }

                bCompleteRequest = FALSE;
            } else if (FcbOrVcb->Identifier.Type == RFSDFCB) {
                Status = RfsdWriteFile(IrpContext);

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
            RfsdCompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;
    
    return Status;
}

#endif // !RFSD_READ_ONLY
