/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             memory.c
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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RfsdAllocateIrpContext)
#pragma alloc_text(PAGE, RfsdFreeIrpContext)
#pragma alloc_text(PAGE, RfsdAllocateFcb)
#pragma alloc_text(PAGE, RfsdFreeFcb)
#pragma alloc_text(PAGE, RfsdAllocateMcb)
#pragma alloc_text(PAGE, RfsdSearchMcbTree)
#pragma alloc_text(PAGE, RfsdSearchMcb)
#pragma alloc_text(PAGE, RfsdGetFullFileName)
#pragma alloc_text(PAGE, RfsdRefreshMcb)
#pragma alloc_text(PAGE, RfsdAddMcbNode)
#pragma alloc_text(PAGE, RfsdDeleteMcbNode)
#pragma alloc_text(PAGE, RfsdFreeMcbTree)
#if !RFSD_READ_ONLY
#pragma alloc_text(PAGE, RfsdCheckBitmapConsistency)
#pragma alloc_text(PAGE, RfsdCheckSetBlock)
#endif // !RFSD_READ_ONLY
#pragma alloc_text(PAGE, RfsdInitializeVcb)
#pragma alloc_text(PAGE, RfsdFreeCcb)
#pragma alloc_text(PAGE, RfsdAllocateCcb)
#pragma alloc_text(PAGE, RfsdFreeVcb)
#pragma alloc_text(PAGE, RfsdCreateFcbFromMcb)
#pragma alloc_text(PAGE, RfsdSyncUninitializeCacheMap)
#endif

__drv_mustHoldCriticalRegion
PRFSD_IRP_CONTEXT
RfsdAllocateIrpContext (IN PDEVICE_OBJECT   DeviceObject,
                        IN PIRP             Irp )
{
    PIO_STACK_LOCATION   IoStackLocation;
    PRFSD_IRP_CONTEXT    IrpContext;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);
    ASSERT(Irp != NULL);
    
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    
    ExAcquireResourceExclusiveLite(
            &RfsdGlobal->LAResource,
            TRUE );

    IrpContext = (PRFSD_IRP_CONTEXT) (
        ExAllocateFromNPagedLookasideList(
            &(RfsdGlobal->RfsdIrpContextLookasideList)));

    ExReleaseResourceForThreadLite(
            &RfsdGlobal->LAResource,
            ExGetCurrentResourceThread() );

    if (IrpContext == NULL) {

        IrpContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(RFSD_IRP_CONTEXT), RFSD_POOL_TAG);

        //
        //  Zero out the irp context and indicate that it is from pool and
        //  not region allocated
        //

        RtlZeroMemory(IrpContext, sizeof(RFSD_IRP_CONTEXT));

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_FROM_POOL);

    } else {

        //
        //  Zero out the irp context and indicate that it is from zone and
        //  not pool allocated
        //

        RtlZeroMemory(IrpContext, sizeof(RFSD_IRP_CONTEXT) );
    }
    
    if (!IrpContext) {
        return NULL;
    }
    
    IrpContext->Identifier.Type = RFSDICX;
    IrpContext->Identifier.Size = sizeof(RFSD_IRP_CONTEXT);
    
    IrpContext->Irp = Irp;
    
    IrpContext->MajorFunction = IoStackLocation->MajorFunction;
    IrpContext->MinorFunction = IoStackLocation->MinorFunction;
    
    IrpContext->DeviceObject = DeviceObject;
    
    IrpContext->FileObject = IoStackLocation->FileObject;

    if (IrpContext->FileObject != NULL) {
        IrpContext->RealDevice = IrpContext->FileObject->DeviceObject;
    } else if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) {
        if (IoStackLocation->Parameters.MountVolume.Vpb) {
            IrpContext->RealDevice = 
                IoStackLocation->Parameters.MountVolume.Vpb->RealDevice;
        }
    }

    if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL ||
        IrpContext->MajorFunction == IRP_MJ_DEVICE_CONTROL ||
        IrpContext->MajorFunction == IRP_MJ_SHUTDOWN) {
        IrpContext->IsSynchronous = TRUE;
    } else if (IrpContext->MajorFunction == IRP_MJ_CLEANUP ||
        IrpContext->MajorFunction == IRP_MJ_CLOSE) {
        IrpContext->IsSynchronous = FALSE;
    }
#if (_WIN32_WINNT >= 0x0500)
    else if (IrpContext->MajorFunction == IRP_MJ_PNP) {
        if (IoGetCurrentIrpStackLocation(Irp)->FileObject == NULL) {
            IrpContext->IsSynchronous = TRUE;
        } else {
            IrpContext->IsSynchronous = IoIsOperationSynchronous(Irp);
        }
    }
#endif //(_WIN32_WINNT >= 0x0500)
    else {
        IrpContext->IsSynchronous = IoIsOperationSynchronous(Irp);
    }

#if 0    
    //
    // Temporary workaround for a bug in close that makes it reference a
    // fileobject when it is no longer valid.
    //
    if (IrpContext->MajorFunction == IRP_MJ_CLOSE) {
        IrpContext->IsSynchronous = TRUE;
    }
#endif
    
    IrpContext->IsTopLevel = (IoGetTopLevelIrp() == Irp);
    
    IrpContext->ExceptionInProgress = FALSE;
    
    return IrpContext;
}

NTSTATUS
RfsdCompleteIrpContext (
    IN PRFSD_IRP_CONTEXT IrpContext,
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

        RfsdCompleteRequest(
            Irp, bPrint, (CCHAR)(NT_SUCCESS(Status)?
            IO_DISK_INCREMENT : IO_NO_INCREMENT) );

        IrpContext->Irp = NULL;               
    }

    RfsdFreeIrpContext(IrpContext);

    return Status;
}

__drv_mustHoldCriticalRegion
VOID
RfsdFreeIrpContext (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PAGED_CODE();

    ASSERT(IrpContext != NULL);
    
    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
        (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));

    RfsdUnpinRepinnedBcbs(IrpContext);

    //  Return the Irp context record to the region or to pool depending on
    //  its flag

    if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FROM_POOL)) {

        IrpContext->Identifier.Type = 0;
        IrpContext->Identifier.Size = 0;

        ExFreePool( IrpContext );

    } else {

        IrpContext->Identifier.Type = 0;
        IrpContext->Identifier.Size = 0;

        ExAcquireResourceExclusiveLite(
                &RfsdGlobal->LAResource,
                TRUE );

        ExFreeToNPagedLookasideList(&(RfsdGlobal->RfsdIrpContextLookasideList), IrpContext);

        ExReleaseResourceForThreadLite(
                &RfsdGlobal->LAResource,
                ExGetCurrentResourceThread() );

    }
}

VOID
RfsdRepinBcb (
    IN PRFSD_IRP_CONTEXT IrpContext,
    IN PBCB Bcb
    )
{
    PRFSD_REPINNED_BCBS Repinned;

    Repinned = &IrpContext->Repinned;

    return;

#if 0

    ULONG i;

    while (Repinned)  {

        for (i = 0; i < RFSD_REPINNED_BCBS_ARRAY_SIZE; i += 1) {
            if (Repinned->Bcb[i] == Bcb) {
                return;
            }
        }

        Repinned = Repinned->Next;
    }

    while (TRUE) {

        for (i = 0; i < RFSD_REPINNED_BCBS_ARRAY_SIZE; i += 1) {
            if (Repinned->Bcb[i] == Bcb) {
                return;
            }

            if (Repinned->Bcb[i] == NULL) {
                Repinned->Bcb[i] = Bcb;
                CcRepinBcb( Bcb );

                return;
            }
        }

        if (Repinned->Next == NULL) {
            Repinned->Next = ExAllocatePoolWithTag(PagedPool, sizeof(RFSD_REPINNED_BCBS), RFSD_POOL_TAG);
            RtlZeroMemory( Repinned->Next, sizeof(RFSD_REPINNED_BCBS) );
        }

        Repinned = Repinned->Next;
    }

#endif

}

VOID
RfsdUnpinRepinnedBcbs (
    IN PRFSD_IRP_CONTEXT IrpContext
    )
{
    IO_STATUS_BLOCK RaiseIosb;
    PRFSD_REPINNED_BCBS Repinned;
    BOOLEAN WriteThroughToDisk;
    PFILE_OBJECT FileObject = NULL;
    BOOLEAN ForceVerify = FALSE;
    ULONG i;

    Repinned = &IrpContext->Repinned;
    RaiseIosb.Status = STATUS_SUCCESS;

    WriteThroughToDisk = (BOOLEAN) (IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH) ||
                                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FLOPPY));

    while (Repinned != NULL) {
        for (i = 0; i < RFSD_REPINNED_BCBS_ARRAY_SIZE; i += 1) {
            if (Repinned->Bcb[i] != NULL) {
                IO_STATUS_BLOCK Iosb;

                ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

                if ( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FLOPPY) ) {
                    FileObject = CcGetFileObjectFromBcb( Repinned->Bcb[i] );
                }

                ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

                CcUnpinRepinnedBcb( Repinned->Bcb[i],
                                    WriteThroughToDisk,
                                    &Iosb );

                ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

                if ( !NT_SUCCESS(Iosb.Status) ) {
                    if (RaiseIosb.Status == STATUS_SUCCESS) {
                        RaiseIosb = Iosb;
                    }

                    if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FLOPPY) &&
                        (IrpContext->MajorFunction != IRP_MJ_CLEANUP) &&
                        (IrpContext->MajorFunction != IRP_MJ_FLUSH_BUFFERS) &&
                        (IrpContext->MajorFunction != IRP_MJ_SET_INFORMATION)) {

                        CcPurgeCacheSection( FileObject->SectionObjectPointer,
                                             NULL,
                                             0,
                                             FALSE );

                        ForceVerify = TRUE;
                    }
                }

                Repinned->Bcb[i] = NULL;

            } else {

                break;
            }
        }

        if (Repinned != &IrpContext->Repinned) 
{
            PRFSD_REPINNED_BCBS Saved;

            Saved = Repinned->Next;
            ExFreePool( Repinned );
            Repinned = Saved;

        } else {

            Repinned = Repinned->Next;
            IrpContext->Repinned.Next = NULL;
        }
    }

    if (!NT_SUCCESS(RaiseIosb.Status)) {

        DbgBreak();

        if (ForceVerify && FileObject) {
            SetFlag(FileObject->DeviceObject->Flags, DO_VERIFY_VOLUME);
            IoSetHardErrorOrVerifyDevice( IrpContext->Irp,
                                          FileObject->DeviceObject );
        }

        IrpContext->Irp->IoStatus = RaiseIosb;
        RfsdNormalizeAndRaiseStatus(IrpContext, RaiseIosb.Status );
    }

    return;
}

__drv_mustHoldCriticalRegion
PRFSD_FCB
RfsdAllocateFcb (IN PRFSD_VCB   Vcb,
         IN PRFSD_MCB           RfsdMcb,
         IN PRFSD_INODE         Inode )
{
    PRFSD_FCB Fcb;

    PAGED_CODE();

    ExAcquireResourceExclusiveLite(
            &RfsdGlobal->LAResource,
            TRUE );

    Fcb = (PRFSD_FCB) ExAllocateFromNPagedLookasideList(
                            &(RfsdGlobal->RfsdFcbLookasideList));

    ExReleaseResourceForThreadLite(
            &RfsdGlobal->LAResource,
            ExGetCurrentResourceThread() );

    if (Fcb == NULL) {
        Fcb = (PRFSD_FCB) ExAllocatePoolWithTag(NonPagedPool, sizeof(RFSD_FCB), RFSD_POOL_TAG);

        RtlZeroMemory(Fcb, sizeof(RFSD_FCB));

        SetFlag(Fcb->Flags, FCB_FROM_POOL);
    } else {
        RtlZeroMemory(Fcb, sizeof(RFSD_FCB));
    }

    if (!Fcb) {
        return NULL;
    }
    
    Fcb->Identifier.Type = RFSDFCB;
    Fcb->Identifier.Size = sizeof(RFSD_FCB);

    FsRtlInitializeFileLock (
        &Fcb->FileLockAnchor,
        NULL,
        NULL );
    
    Fcb->OpenHandleCount = 0;
    Fcb->ReferenceCount = 0;
    
    Fcb->Vcb = Vcb;

#if DBG    

    Fcb->AnsiFileName.MaximumLength = (USHORT)
        RfsdUnicodeToOEMSize(&(RfsdMcb->ShortName)) + 1;

    Fcb->AnsiFileName.Buffer = (PUCHAR) 
        ExAllocatePoolWithTag(PagedPool, Fcb->AnsiFileName.MaximumLength, RFSD_POOL_TAG);

    if (!Fcb->AnsiFileName.Buffer) {
        goto errorout;
    }

    RtlZeroMemory(Fcb->AnsiFileName.Buffer, Fcb->AnsiFileName.MaximumLength);

    RfsdUnicodeToOEM( &(Fcb->AnsiFileName),
                     &(RfsdMcb->ShortName));

#endif

    RfsdMcb->FileAttr = FILE_ATTRIBUTE_NORMAL;
    
    if (S_ISDIR(Inode->i_mode)) {
        SetFlag(RfsdMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY);
    }

    if ( IsFlagOn(Vcb->Flags, VCB_READ_ONLY) || 
         RfsdIsReadOnly(Inode->i_mode)) {
        SetFlag(RfsdMcb->FileAttr, FILE_ATTRIBUTE_READONLY);
    }
    
    Fcb->Inode = Inode;

    Fcb->RfsdMcb = RfsdMcb;
    RfsdMcb->RfsdFcb = Fcb;
    
    RtlZeroMemory(&Fcb->Header, sizeof(FSRTL_COMMON_FCB_HEADER));
    
    Fcb->Header.NodeTypeCode = (USHORT) RFSDFCB;
    Fcb->Header.NodeByteSize = sizeof(RFSD_FCB);
    Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;
    Fcb->Header.Resource = &(Fcb->MainResource);
    Fcb->Header.PagingIoResource = &(Fcb->PagingIoResource);

    // NOTE: In EXT2, the low part was stored in i_size (a 32-bit value); the high part would be stored in the acl field...
	// However, on ReiserFS, the i_size is a 64-bit value.
	Fcb->Header.FileSize.QuadPart = Fcb->Inode->i_size;
    

    Fcb->Header.AllocationSize.QuadPart = 
        CEILING_ALIGNED(Fcb->Header.FileSize.QuadPart, (ULONGLONG)Vcb->BlockSize);

	Fcb->Header.ValidDataLength.QuadPart = (LONGLONG)(0x7fffffffffffffff);
    
    Fcb->SectionObject.DataSectionObject = NULL;
    Fcb->SectionObject.SharedCacheMap = NULL;
    Fcb->SectionObject.ImageSectionObject = NULL;

    ExInitializeResourceLite(&(Fcb->MainResource));
    ExInitializeResourceLite(&(Fcb->PagingIoResource));

    InsertTailList(&Vcb->FcbList, &Fcb->Next);

#if DBG

    ExAcquireResourceExclusiveLite(
        &RfsdGlobal->CountResource,
        TRUE );

    RfsdGlobal->FcbAllocated++;

    ExReleaseResourceForThreadLite(
        &RfsdGlobal->CountResource,
        ExGetCurrentResourceThread() );
#endif
    
    return Fcb;

#if DBG
errorout:
#endif

    if (Fcb) {

#if DBG
        if (Fcb->AnsiFileName.Buffer)
            ExFreePool(Fcb->AnsiFileName.Buffer);
#endif
        
        if (FlagOn(Fcb->Flags, FCB_FROM_POOL)) {
            
            ExFreePool( Fcb );
            
        } else {

            ExAcquireResourceExclusiveLite(
                    &RfsdGlobal->LAResource,
                    TRUE );

            ExFreeToNPagedLookasideList(&(RfsdGlobal->RfsdFcbLookasideList), Fcb);

            ExReleaseResourceForThreadLite(
                    &RfsdGlobal->LAResource,
                    ExGetCurrentResourceThread() );
        }
        
    }

    return NULL;
}

__drv_mustHoldCriticalRegion
VOID
RfsdFreeFcb (IN PRFSD_FCB Fcb)
{
    PRFSD_VCB       Vcb;

    PAGED_CODE();

    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
        (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

    Vcb = Fcb->Vcb;

    FsRtlUninitializeFileLock(&Fcb->FileLockAnchor);

    ExDeleteResourceLite(&Fcb->MainResource);
    
    ExDeleteResourceLite(&Fcb->PagingIoResource);
    
    Fcb->RfsdMcb->RfsdFcb = NULL;

    if(IsFlagOn(Fcb->Flags, FCB_FILE_DELETED)) {
        if (Fcb->RfsdMcb) {
            RfsdDeleteMcbNode(Vcb, Fcb->RfsdMcb->Parent, Fcb->RfsdMcb);
            RfsdFreeMcb(Fcb->RfsdMcb);
        }
    }

    if (Fcb->LongName.Buffer) {
        ExFreePool(Fcb->LongName.Buffer);
        Fcb->LongName.Buffer = NULL;
    }

#if DBG    
    ExFreePool(Fcb->AnsiFileName.Buffer);
#endif

    ExFreePool(Fcb->Inode);

    Fcb->Header.NodeTypeCode = (SHORT)0xCCCC;
    Fcb->Header.NodeByteSize = (SHORT)0xC0C0;

    if (FlagOn(Fcb->Flags, FCB_FROM_POOL)) {

        ExFreePool( Fcb );

    } else {

        ExAcquireResourceExclusiveLite(
                    &RfsdGlobal->LAResource,
                    TRUE );

        ExFreeToNPagedLookasideList(&(RfsdGlobal->RfsdFcbLookasideList), Fcb);

        ExReleaseResourceForThreadLite(
                    &RfsdGlobal->LAResource,
                    ExGetCurrentResourceThread() );
    }

#if DBG

    ExAcquireResourceExclusiveLite(
        &RfsdGlobal->CountResource,
        TRUE );

    RfsdGlobal->FcbAllocated--;

    ExReleaseResourceForThreadLite(
        &RfsdGlobal->CountResource,
        ExGetCurrentResourceThread() );
#endif

}

__drv_mustHoldCriticalRegion
PRFSD_CCB
RfsdAllocateCcb (VOID)
{
    PRFSD_CCB Ccb;

    PAGED_CODE();

    ExAcquireResourceExclusiveLite(
            &RfsdGlobal->LAResource,
            TRUE );

    Ccb = (PRFSD_CCB) (ExAllocateFromNPagedLookasideList( &(RfsdGlobal->RfsdCcbLookasideList)));

    ExReleaseResourceForThreadLite(
            &RfsdGlobal->LAResource,
            ExGetCurrentResourceThread() );
    
    if (Ccb == NULL) {
        Ccb = (PRFSD_CCB) ExAllocatePoolWithTag(NonPagedPool, sizeof(RFSD_CCB), RFSD_POOL_TAG);

        RtlZeroMemory(Ccb, sizeof(RFSD_CCB));

        SetFlag(Ccb->Flags, CCB_FROM_POOL);
    } else {
        RtlZeroMemory(Ccb, sizeof(RFSD_CCB));
    }

    if (!Ccb) {
        return NULL;
    }
    
    Ccb->Identifier.Type = RFSDCCB;
    Ccb->Identifier.Size = sizeof(RFSD_CCB);
    
    Ccb->CurrentByteOffset = 0;
    
    Ccb->DirectorySearchPattern.Length = 0;
    Ccb->DirectorySearchPattern.MaximumLength = 0;
    Ccb->DirectorySearchPattern.Buffer = 0;
    
    return Ccb;
}

__drv_mustHoldCriticalRegion
VOID
RfsdFreeCcb (IN PRFSD_CCB Ccb)
{
    PAGED_CODE();

    ASSERT(Ccb != NULL);
    
    ASSERT((Ccb->Identifier.Type == RFSDCCB) &&
        (Ccb->Identifier.Size == sizeof(RFSD_CCB)));
    
    if (Ccb->DirectorySearchPattern.Buffer != NULL) {
        ExFreePool(Ccb->DirectorySearchPattern.Buffer);
    }

    if (FlagOn(Ccb->Flags, CCB_FROM_POOL)) {

        ExFreePool( Ccb );

    } else {

        ExAcquireResourceExclusiveLite(
                &RfsdGlobal->LAResource,
                TRUE );

        ExFreeToNPagedLookasideList(&(RfsdGlobal->RfsdCcbLookasideList), Ccb);

        ExReleaseResourceForThreadLite(
                &RfsdGlobal->LAResource,
                ExGetCurrentResourceThread() );
    }
}

__drv_mustHoldCriticalRegion
PRFSD_MCB
RfsdAllocateMcb (PRFSD_VCB Vcb, PUNICODE_STRING FileName, ULONG FileAttr)
{
    PRFSD_MCB   Mcb = NULL;
    PLIST_ENTRY List = NULL;
    ULONG       Extra = 0;

    PAGED_CODE();

#define MCB_NUM_SHIFT   0x04

    if (RfsdGlobal->McbAllocated > (RfsdGlobal->MaxDepth <<  MCB_NUM_SHIFT))
        Extra = RfsdGlobal->McbAllocated - 
                (RfsdGlobal->MaxDepth << MCB_NUM_SHIFT) +
                RfsdGlobal->MaxDepth;

    RfsdPrint((DBG_INFO,
            "RfsdAllocateMcb: CurrDepth=%xh/%xh/%xh FileName=%S\n", 
            RfsdGlobal->McbAllocated,
            RfsdGlobal->MaxDepth << MCB_NUM_SHIFT,
            RfsdGlobal->FcbAllocated,
            FileName->Buffer));

    List = Vcb->McbList.Flink;

    while ((List != &(Vcb->McbList)) && (Extra > 0)) {
        Mcb = CONTAINING_RECORD(List, RFSD_MCB, Link);
        List = List->Flink;

        if ((!RFSD_IS_ROOT_KEY(Mcb->Key)) && (Mcb->Child == NULL) &&
            (Mcb->RfsdFcb == NULL) && (!IsMcbUsed(Mcb))) {
            RfsdPrint((DBG_INFO, "RfsdAllocateMcb: Mcb %S will be freed.\n",
                                 Mcb->ShortName.Buffer));

            if (RfsdDeleteMcbNode(Vcb, Vcb->McbTree, Mcb)) {
                RfsdFreeMcb(Mcb);

                Extra--;
            }
        }
    }

    ExAcquireResourceExclusiveLite(
            &RfsdGlobal->LAResource,
            TRUE );

    Mcb = (PRFSD_MCB) (ExAllocateFromPagedLookasideList(
                         &(RfsdGlobal->RfsdMcbLookasideList)));
  
    ExReleaseResourceForThreadLite(
            &RfsdGlobal->LAResource,
            ExGetCurrentResourceThread() );
    
    if (Mcb == NULL) {
        Mcb = (PRFSD_MCB) ExAllocatePoolWithTag(PagedPool, sizeof(RFSD_MCB), RFSD_POOL_TAG);
        
        RtlZeroMemory(Mcb, sizeof(RFSD_MCB));
        
        SetFlag(Mcb->Flags, MCB_FROM_POOL);
    } else {
        RtlZeroMemory(Mcb, sizeof(RFSD_MCB));
    }
    
    if (!Mcb) {
        return NULL;
    }

    Mcb->Identifier.Type = RFSDMCB;
    Mcb->Identifier.Size = sizeof(RFSD_MCB);

    if (FileName && FileName->Length) {

        Mcb->ShortName.Length = FileName->Length;
        Mcb->ShortName.MaximumLength = Mcb->ShortName.Length + 2;

        Mcb->ShortName.Buffer = ExAllocatePoolWithTag(PagedPool, Mcb->ShortName.MaximumLength, RFSD_POOL_TAG);

        if (!Mcb->ShortName.Buffer)
            goto errorout;

        RtlZeroMemory(Mcb->ShortName.Buffer, Mcb->ShortName.MaximumLength);
        RtlCopyMemory(Mcb->ShortName.Buffer, FileName->Buffer, Mcb->ShortName.Length);
    } 

    Mcb->FileAttr = FileAttr;

    ExAcquireResourceExclusiveLite(
        &RfsdGlobal->CountResource,
        TRUE );

    RfsdGlobal->McbAllocated++;

    ExReleaseResourceForThreadLite(
        &RfsdGlobal->CountResource,
        ExGetCurrentResourceThread() );

    return Mcb;

errorout:

    if (Mcb) {

        if (Mcb->ShortName.Buffer)
            ExFreePool(Mcb->ShortName.Buffer);

        if (FlagOn(Mcb->Flags, MCB_FROM_POOL)) {

            ExFreePool( Mcb );

        } else {

            ExAcquireResourceExclusiveLite(
                    &RfsdGlobal->LAResource,
                    TRUE );

            ExFreeToPagedLookasideList(&(RfsdGlobal->RfsdMcbLookasideList), Mcb);

            ExReleaseResourceForThreadLite(
                    &RfsdGlobal->LAResource,
                    ExGetCurrentResourceThread() );
        }
    }

    return NULL;
}

__drv_mustHoldCriticalRegion
VOID
RfsdFreeMcb (IN PRFSD_MCB Mcb)
{
#ifndef __REACTOS__
    PRFSD_MCB   Parent = Mcb->Parent;
#endif

    ASSERT(Mcb != NULL);
    
    ASSERT((Mcb->Identifier.Type == RFSDMCB) &&
        (Mcb->Identifier.Size == sizeof(RFSD_MCB)));

    RfsdPrint((DBG_INFO, "RfsdFreeMcb: Mcb %S will be freed.\n", Mcb->ShortName.Buffer));

    if (Mcb->ShortName.Buffer)
        ExFreePool(Mcb->ShortName.Buffer);
    
    if (FlagOn(Mcb->Flags, MCB_FROM_POOL)) {

        ExFreePool( Mcb );

    } else {

        ExAcquireResourceExclusiveLite(
                &RfsdGlobal->LAResource,
                TRUE );

        ExFreeToPagedLookasideList(&(RfsdGlobal->RfsdMcbLookasideList), Mcb);

        ExReleaseResourceForThreadLite(
                &RfsdGlobal->LAResource,
                ExGetCurrentResourceThread() );
    }

    ExAcquireResourceExclusiveLite(
        &RfsdGlobal->CountResource,
        TRUE );

    RfsdGlobal->McbAllocated--;

    ExReleaseResourceForThreadLite(
        &RfsdGlobal->CountResource,
        ExGetCurrentResourceThread() );
}

__drv_mustHoldCriticalRegion
PRFSD_FCB
RfsdCreateFcbFromMcb(PRFSD_VCB Vcb, PRFSD_MCB Mcb)
{
    PRFSD_FCB       Fcb = NULL;
    RFSD_INODE      RfsdIno;

    PAGED_CODE();

    if (Mcb->RfsdFcb)
        return Mcb->RfsdFcb;

    if (RfsdLoadInode(Vcb, &(Mcb->Key), &RfsdIno)) {
        PRFSD_INODE pTmpInode = ExAllocatePoolWithTag(PagedPool, sizeof(RFSD_INODE), RFSD_POOL_TAG);
        if (!pTmpInode) {
            goto errorout;
        }

        RtlCopyMemory(pTmpInode, &RfsdIno, sizeof(RFSD_INODE));
        Fcb = RfsdAllocateFcb(Vcb, Mcb, pTmpInode);
        if (!Fcb) {
            ExFreePool(pTmpInode);
        }
    }

errorout:

    return Fcb;
}

BOOLEAN
RfsdGetFullFileName(PRFSD_MCB Mcb, PUNICODE_STRING FileName)
{
    USHORT          Length = 0;
    PRFSD_MCB       TmpMcb = Mcb;
    PUNICODE_STRING FileNames[256];
    SHORT           Count = 0 , i = 0, j = 0;

    PAGED_CODE();

    while(TmpMcb && Count < 256) {
        if (RFSD_IS_ROOT_KEY(TmpMcb->Key))
            break;

        FileNames[Count++] = &TmpMcb->ShortName;
        Length += (2 + TmpMcb->ShortName.Length);

        TmpMcb = TmpMcb->Parent;
    }

    if (Count >= 256)
        return FALSE;

    if (Count ==0)
        Length = 2;
    
    FileName->Length = Length;
    FileName->MaximumLength = Length + 2;
    FileName->Buffer = ExAllocatePoolWithTag(PagedPool, Length + 2, RFSD_POOL_TAG);

    if (!FileName->Buffer) {
        return FALSE;
    }

    RtlZeroMemory(FileName->Buffer, FileName->MaximumLength);

    if (Count == 0) {
        FileName->Buffer[0] = L'\\';
        return  TRUE;
    }

    for (i = Count - 1; i >= 0 && j < (SHORT)(FileName->MaximumLength); i--) {
        FileName->Buffer[j++] = L'\\';

        RtlCopyMemory( &(FileName->Buffer[j]),
                       FileNames[i]->Buffer,
                       FileNames[i]->Length );

        j += FileNames[i]->Length / 2;
    }
    
    return TRUE;
}

PRFSD_MCB
RfsdSearchMcbTree(  PRFSD_VCB Vcb,
                    PRFSD_MCB RfsdMcb,
                    PRFSD_KEY_IN_MEMORY Key)
{
    PRFSD_MCB   Mcb = NULL;
    PLIST_ENTRY List = NULL;
    BOOLEAN     bFind = FALSE;

    PAGED_CODE();

    List = Vcb->McbList.Flink;

    while ((!bFind) && (List != &(Vcb->McbList))) {
        Mcb = CONTAINING_RECORD(List, RFSD_MCB, Link);
        List = List->Flink;

		if (CompareShortKeys(&(Mcb->Key), Key) == RFSD_KEYS_MATCH) {
            bFind = TRUE;
            break;
        }
    }

    if (bFind) {
        ASSERT(Mcb != NULL);
        RfsdRefreshMcb(Vcb, Mcb);
    } else {
        Mcb = NULL;
    }

    return Mcb;
}

/** 
 Returns NULL is the parent has no child, or if we search through the child's brothers and hit a NULL
 Otherwise, returns the MCB of one of the the parent's children, which matches FileName.
*/
PRFSD_MCB
RfsdSearchMcb(  PRFSD_VCB Vcb,
                PRFSD_MCB Parent,
                PUNICODE_STRING FileName )
{
    PRFSD_MCB TmpMcb = Parent->Child;

    PAGED_CODE();

    while (TmpMcb) {
        if (!RtlCompareUnicodeString(
                    &(TmpMcb->ShortName),
                    FileName, TRUE ))
            break;

        TmpMcb = TmpMcb->Next;
    }

    if (TmpMcb) {
        RfsdRefreshMcb(Vcb, TmpMcb);
    }

    return TmpMcb;
}

VOID
RfsdRefreshMcb(PRFSD_VCB Vcb, PRFSD_MCB Mcb)
{
    PAGED_CODE();

    ASSERT (IsFlagOn(Mcb->Flags, MCB_IN_TREE));

    RemoveEntryList(&(Mcb->Link));
    InsertTailList(&(Vcb->McbList), &(Mcb->Link));
}

VOID
RfsdAddMcbNode(PRFSD_VCB Vcb, PRFSD_MCB Parent, PRFSD_MCB Child)
{
    PRFSD_MCB TmpMcb = Parent->Child;

    PAGED_CODE();

    if(IsFlagOn(Child->Flags, MCB_IN_TREE)) {
        DbgBreak();
        RfsdPrint((DBG_ERROR, "RfsdAddMcbNode: Child Mcb is alreay in the tree.\n"));
        return;
    }

    if (TmpMcb) {
        ASSERT(TmpMcb->Parent == Parent);

        while (TmpMcb->Next) {
            TmpMcb = TmpMcb->Next;
            ASSERT(TmpMcb->Parent == Parent);
        }

        TmpMcb->Next = Child;
        Child->Parent = Parent;
        Child->Next = NULL;
    } else {
        Parent->Child = Child;
        Child->Parent = Parent;
        Child->Next = NULL;
    }

    InsertTailList(&(Vcb->McbList), &(Child->Link));
    SetFlag(Child->Flags, MCB_IN_TREE);
}

BOOLEAN
RfsdDeleteMcbNode(PRFSD_VCB Vcb, PRFSD_MCB McbTree, PRFSD_MCB RfsdMcb)
{
    PRFSD_MCB   TmpMcb;

    PAGED_CODE();

    if(!IsFlagOn(RfsdMcb->Flags, MCB_IN_TREE)) {
        return TRUE;
    }

    if (RfsdMcb->Parent) {
        if (RfsdMcb->Parent->Child == RfsdMcb) {
            RfsdMcb->Parent->Child = RfsdMcb->Next;
        } else {
            TmpMcb = RfsdMcb->Parent->Child;

            while (TmpMcb && TmpMcb->Next != RfsdMcb)
                TmpMcb = TmpMcb->Next;

            if (TmpMcb) {
                TmpMcb->Next = RfsdMcb->Next;
            } else {
                // error
                return FALSE;
            }
        }
    } else if (RfsdMcb->Child) {
        return FALSE;
    }

    RemoveEntryList(&(RfsdMcb->Link));
    ClearFlag(RfsdMcb->Flags, MCB_IN_TREE);

    return TRUE;
}

__drv_mustHoldCriticalRegion
VOID RfsdFreeMcbTree(PRFSD_MCB McbTree)
{
    PAGED_CODE();

    if (!McbTree)
        return;

    if (McbTree->Child) {
        RfsdFreeMcbTree(McbTree->Child);
    }

    if (McbTree->Next) {

        PRFSD_MCB   Current;
        PRFSD_MCB   Next;

        Current = McbTree->Next;

        while (Current) {

            Next = Current->Next;

            if (Current->Child) {
                RfsdFreeMcbTree(Current->Child);
            }

            RfsdFreeMcb(Current);
            Current = Next;
        }
    }

    RfsdFreeMcb(McbTree);
}

#if !RFSD_READ_ONLY

BOOLEAN
RfsdCheckSetBlock(PRFSD_IRP_CONTEXT IrpContext, PRFSD_VCB Vcb, ULONG Block)
{
    ULONG           Group, dwBlk, Length;

    RTL_BITMAP      BlockBitmap;
    PVOID           BitmapCache;
    PBCB            BitmapBcb;

    LARGE_INTEGER   Offset;

    BOOLEAN         bModified = FALSE;

    PAGED_CODE();
#if 0
    Group = (Block - RFSD_FIRST_DATA_BLOCK) / BLOCKS_PER_GROUP;

    dwBlk = (Block - RFSD_FIRST_DATA_BLOCK) % BLOCKS_PER_GROUP;


    Offset.QuadPart = (LONGLONG) Vcb->BlockSize;
    Offset.QuadPart = Offset.QuadPart * Vcb->GroupDesc[Group].bg_block_bitmap;

    if (Group == Vcb->NumOfGroups - 1) {
        Length = TOTAL_BLOCKS % BLOCKS_PER_GROUP;

        /* s_blocks_count is integer multiple of s_blocks_per_group */
        if (Length == 0)
            Length = BLOCKS_PER_GROUP;
    } else {
        Length = BLOCKS_PER_GROUP;
    }

    if (dwBlk >= Length)
        return FALSE;
        
    if (!CcPinRead( Vcb->StreamObj,
                        &Offset,
                        Vcb->BlockSize,
                        PIN_WAIT,
                        &BitmapBcb,
                        &BitmapCache ) ) {

        RfsdPrint((DBG_ERROR, "RfsdDeleteBlock: PinReading error ...\n"));
        return FALSE;
    }

    RtlInitializeBitMap( &BlockBitmap,
                         BitmapCache,
                         Length );

    if (RtlCheckBit(&BlockBitmap, dwBlk) == 0) {
        DbgBreak();
        RtlSetBits(&BlockBitmap, dwBlk, 1);
        bModified = TRUE;
    }

    if (bModified) {

        CcSetDirtyPinnedData(BitmapBcb, NULL );

        RfsdRepinBcb(IrpContext, BitmapBcb);

        RfsdAddMcbEntry(Vcb, Offset.QuadPart, (LONGLONG)Vcb->BlockSize);
    }

    {
        CcUnpinData(BitmapBcb);
        BitmapBcb = NULL;
        BitmapCache = NULL;

        RtlZeroMemory(&BlockBitmap, sizeof(RTL_BITMAP));
    }
#endif // 0
    return (!bModified);
}

BOOLEAN
RfsdCheckBitmapConsistency(PRFSD_IRP_CONTEXT IrpContext, PRFSD_VCB Vcb)
{
    ULONG i, j, InodeBlocks;

    PAGED_CODE();
#if 0
    for (i = 0; i < Vcb->NumOfGroups; i++) {

        RfsdCheckSetBlock(IrpContext, Vcb, Vcb->GroupDesc[i].bg_block_bitmap);
        RfsdCheckSetBlock(IrpContext, Vcb, Vcb->GroupDesc[i].bg_inode_bitmap);
        
        
        if (i == Vcb->NumOfGroups - 1) {
            InodeBlocks = ((INODES_COUNT % INODES_PER_GROUP) * 
                            sizeof(RFSD_INODE) + Vcb->BlockSize - 1) /
                            (Vcb->BlockSize);
        } else {
            InodeBlocks = (INODES_PER_GROUP * sizeof(RFSD_INODE) + Vcb->BlockSize - 1) / (Vcb->BlockSize);
        }

        for (j = 0; j < InodeBlocks; j++ )
            RfsdCheckSetBlock(IrpContext, Vcb, Vcb->GroupDesc[i].bg_inode_table + j);
    }
#endif // 0
    return TRUE;
}

#endif // !RFSD_READ_ONLY

VOID
RfsdInsertVcb(PRFSD_VCB Vcb)
{
    InsertTailList(&(RfsdGlobal->VcbList), &Vcb->Next);
}

VOID
RfsdRemoveVcb(PRFSD_VCB Vcb)
{
    RemoveEntryList(&Vcb->Next);
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdInitializeVcb( IN PRFSD_IRP_CONTEXT IrpContext, 
                   IN PRFSD_VCB Vcb, 
                   IN PRFSD_SUPER_BLOCK RfsdSb,
                   IN PDEVICE_OBJECT TargetDevice,
                   IN PDEVICE_OBJECT VolumeDevice,
                   IN PVPB Vpb )
{
    BOOLEAN                     VcbResourceInitialized = FALSE;
    USHORT                      VolumeLabelLength;
    ULONG                       IoctlSize;
    BOOLEAN                     NotifySyncInitialized = FALSE;
    LONGLONG                    DiskSize;
    LONGLONG                    PartSize;
    NTSTATUS                    Status = STATUS_UNSUCCESSFUL;
    UNICODE_STRING              RootNode;
    USHORT                      Buffer[2];
    ULONG                       ChangeCount;
	char	s_volume_name[16] = "ReiserFS\0"; /* volume name */		// FUTURE: pull in from v2 superblock, if available
	int templen;

    PAGED_CODE();

    _SEH2_TRY {

        if (!Vpb) {

            Status = STATUS_DEVICE_NOT_READY;
            _SEH2_LEAVE;
        }

        RfsdPrint((DBG_ERROR, "RfsdInitializeVcb: Flink = %xh McbList = %xh\n",
                              Vcb->McbList.Flink, &(Vcb->McbList.Flink)));

#if RFSD_READ_ONLY
        SetFlag(Vcb->Flags, VCB_READ_ONLY);
#endif //READ_ONLY

        if (IsFlagOn(Vpb->RealDevice->Characteristics, FILE_REMOVABLE_MEDIA)) {
            SetFlag(Vcb->Flags, VCB_REMOVABLE_MEDIA);
        }

        if (IsFlagOn(Vpb->RealDevice->Characteristics, FILE_FLOPPY_DISKETTE)) {
            SetFlag(Vcb->Flags, VCB_FLOPPY_DISK);
        }

        /*if (IsFlagOn(RfsdGlobal->Flags, RFSD_SUPPORT_WRITING)) {
            if ((IsFlagOn(RfsdSb->s_feature_incompat, EXT3_FEATURE_INCOMPAT_JOURNAL_DEV) ||
                  IsFlagOn(RfsdSb->s_feature_incompat, EXT3_FEATURE_INCOMPAT_RECOVER) ||
                  IsFlagOn(RfsdSb->s_feature_compat, EXT3_FEATURE_COMPAT_HAS_JOURNAL)) ) {
                if(IsFlagOn(RfsdGlobal->Flags, EXT3_FORCE_WRITING)) {
                    ClearFlag(Vcb->Flags, VCB_READ_ONLY);
                } else {
                    SetFlag(Vcb->Flags, VCB_READ_ONLY);
                }
            } else {
                ClearFlag(Vcb->Flags, VCB_READ_ONLY);
            }
        } else {
            SetFlag(Vcb->Flags, VCB_READ_ONLY);
        }*/
		  // RFSD will be read only.
		  SetFlag(Vcb->Flags, VCB_READ_ONLY);

        ExInitializeResourceLite(&Vcb->MainResource);
        ExInitializeResourceLite(&Vcb->PagingIoResource);

        ExInitializeResourceLite(&Vcb->McbResource);
        
        VcbResourceInitialized = TRUE;

        Vcb->Vpb = Vpb;

        Vcb->RealDevice = Vpb->RealDevice;
        Vpb->DeviceObject = VolumeDevice;

        {
            UNICODE_STRING      LabelName;
            OEM_STRING          OemName;

				

            LabelName.MaximumLength = 16 * 2;
            LabelName.Length    = 0;
            LabelName.Buffer    = Vcb->Vpb->VolumeLabel;

            RtlZeroMemory(LabelName.Buffer, LabelName.MaximumLength);

            VolumeLabelLength = 16;

            while( (VolumeLabelLength > 0) &&
                   ((s_volume_name[VolumeLabelLength-1] == '\0') ||
                    (s_volume_name[VolumeLabelLength-1] == ' ')) ) {
                VolumeLabelLength--;
            }

            OemName.Buffer =  s_volume_name;
            OemName.MaximumLength = 16;
            OemName.Length = VolumeLabelLength;

				templen = RtlOemStringToUnicodeSize(&OemName);

            Status = RfsdOEMToUnicode( &LabelName,
                                       &OemName );

            if (!NT_SUCCESS(Status)) {
                _SEH2_LEAVE;
            }

            Vpb->VolumeLabelLength = LabelName.Length;
        }

#if DISABLED
        Vpb->SerialNumber = ((ULONG*)RfsdSb->s_uuid)[0] + ((ULONG*)RfsdSb->s_uuid)[1] +
                            ((ULONG*)RfsdSb->s_uuid)[2] + ((ULONG*)RfsdSb->s_uuid)[3];
#endif

        Vcb->StreamObj = IoCreateStreamFileObject( NULL, Vcb->Vpb->RealDevice);

        if (Vcb->StreamObj) {

            Vcb->StreamObj->SectionObjectPointer = &(Vcb->SectionObject);
            Vcb->StreamObj->Vpb = Vcb->Vpb;
            Vcb->StreamObj->ReadAccess = TRUE;
            if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
            {
                Vcb->StreamObj->WriteAccess = TRUE;
                Vcb->StreamObj->DeleteAccess = TRUE;
            }
            else
            {
                Vcb->StreamObj->WriteAccess = TRUE;
                Vcb->StreamObj->DeleteAccess = TRUE;
            }
            Vcb->StreamObj->FsContext = (PVOID) Vcb;
            Vcb->StreamObj->FsContext2 = NULL;
            Vcb->StreamObj->Vpb = Vcb->Vpb;

            SetFlag(Vcb->StreamObj->Flags, FO_NO_INTERMEDIATE_BUFFERING);
        } else {
            _SEH2_LEAVE;
        }

        InitializeListHead(&Vcb->FcbList);

        InitializeListHead(&Vcb->NotifyList);

        FsRtlNotifyInitializeSync(&Vcb->NotifySync);

        NotifySyncInitialized = TRUE;

        Vcb->DeviceObject = VolumeDevice;
        
        Vcb->TargetDeviceObject = TargetDevice;
        
        Vcb->OpenFileHandleCount = 0;
        
        Vcb->ReferenceCount = 0;
        
        Vcb->SuperBlock = RfsdSb;
        
        Vcb->Header.NodeTypeCode = (USHORT) RFSDVCB;
        Vcb->Header.NodeByteSize = sizeof(RFSD_VCB);
        Vcb->Header.IsFastIoPossible = FastIoIsNotPossible;
        Vcb->Header.Resource = &(Vcb->MainResource);
        Vcb->Header.PagingIoResource = &(Vcb->PagingIoResource);

        Vcb->Vpb->SerialNumber = 'MATT';

        DiskSize =
            Vcb->DiskGeometry.Cylinders.QuadPart *
            Vcb->DiskGeometry.TracksPerCylinder *
            Vcb->DiskGeometry.SectorsPerTrack *
            Vcb->DiskGeometry.BytesPerSector;

        IoctlSize = sizeof(PARTITION_INFORMATION);
        
        Status = RfsdDiskIoControl(
            TargetDevice,
            IOCTL_DISK_GET_PARTITION_INFO,
            NULL,
            0,
            &Vcb->PartitionInformation,
            &IoctlSize );

        PartSize = Vcb->PartitionInformation.PartitionLength.QuadPart;
        
        if (!NT_SUCCESS(Status)) {
            Vcb->PartitionInformation.StartingOffset.QuadPart = 0;
            
            Vcb->PartitionInformation.PartitionLength.QuadPart =
                DiskSize;

            PartSize = DiskSize;
            
            Status = STATUS_SUCCESS;
        }

        IoctlSize = sizeof(ULONG);
        Status = RfsdDiskIoControl(
                TargetDevice,
                IOCTL_DISK_CHECK_VERIFY,
                NULL,
                0,
                &ChangeCount,
                &IoctlSize );

        if (!NT_SUCCESS(Status)) {
            _SEH2_LEAVE;
        }

        Vcb->ChangeCount = ChangeCount;

        Vcb->Header.AllocationSize.QuadPart =
        Vcb->Header.FileSize.QuadPart = PartSize;

        Vcb->Header.ValidDataLength.QuadPart = 
            (LONGLONG)(0x7fffffffffffffff);
/*
        Vcb->Header.AllocationSize.QuadPart = (LONGLONG)(rfsd_super_block->s_blocks_count - rfsd_super_block->s_free_blocks_count)
            * (RFSD_MIN_BLOCK << rfsd_super_block->s_log_block_size);
        Vcb->Header.FileSize.QuadPart = Vcb->Header.AllocationSize.QuadPart;
        Vcb->Header.ValidDataLength.QuadPart = Vcb->Header.AllocationSize.QuadPart;
*/
        {
            CC_FILE_SIZES FileSizes;

            FileSizes.AllocationSize.QuadPart =
            FileSizes.FileSize.QuadPart =
                Vcb->Header.AllocationSize.QuadPart;

            FileSizes.ValidDataLength.QuadPart= (LONGLONG)(0x7fffffffffffffff);

            CcInitializeCacheMap( Vcb->StreamObj,
                                  &FileSizes,
                                  TRUE,
                                  &(RfsdGlobal->CacheManagerNoOpCallbacks),
                                  Vcb );
        }
#if DISABLED	// IN FFFS TOO
        if (!RfsdLoadGroup(Vcb)) {
            Status = STATUS_UNSUCCESSFUL;
            _SEH2_LEAVE;
        }
#endif

        FsRtlInitializeLargeMcb(&(Vcb->DirtyMcbs), PagedPool);
        InitializeListHead(&(Vcb->McbList));


        //
        // Now allocating the mcb for root ...
        //

        Buffer[0] = L'\\';
        Buffer[1] = 0;

        RootNode.Buffer = Buffer;
        RootNode.MaximumLength = RootNode.Length = 2;

        Vcb->McbTree = RfsdAllocateMcb( Vcb, &RootNode,
                            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_NORMAL);

        if (!Vcb->McbTree) {
            _SEH2_LEAVE;
        }

		// Set the root of the filesystem to the root directory's inode / stat data structure
		Vcb->McbTree->Key.k_dir_id = RFSD_ROOT_PARENT_ID;
		Vcb->McbTree->Key.k_objectid = RFSD_ROOT_OBJECT_ID;
		Vcb->McbTree->Key.k_offset = 0;
		Vcb->McbTree->Key.k_type   = RFSD_KEY_TYPE_v1_STAT_DATA;		

#ifdef DISABLED
        if (IsFlagOn(RfsdGlobal->Flags, RFSD_CHECKING_BITMAP)) {
            RfsdCheckBitmapConsistency(IrpContext, Vcb);
        }

        {
			ULONG   dwData[RFSD_BLOCK_TYPES] = {RFSD_NDIR_BLOCKS, 1, 1, 1};
            ULONG   dwMeta[RFSD_BLOCK_TYPES] = {0, 0, 0, 0};
            ULONG   i;

				KdPrint(("Reminder: " __FUNCTION__ ", dwData, dwMeta??\n"));

            for (i = 0; i < RFSD_BLOCK_TYPES; i++) {
                dwData[i] = dwData[i] << ((BLOCK_BITS - 2) * i);

                if (i > 0) {
                    dwMeta[i] = 1 + (dwMeta[i - 1] << (BLOCK_BITS - 2));
                }

                Vcb->dwData[i] = dwData[i];
                Vcb->dwMeta[i] = dwMeta[i];
            }
        }
#endif

        SetFlag(Vcb->Flags, VCB_INITIALIZED);

    } _SEH2_FINALLY {

        if (!NT_SUCCESS(Status)) {

            if (NotifySyncInitialized) {
                FsRtlNotifyUninitializeSync(&Vcb->NotifySync);
            }

            if (Vcb->GroupDesc) {
                ExFreePool(Vcb->GroupDesc);
                // CcUnpinData(Vcb->GroupDescBcb);
                Vcb->GroupDesc = NULL;
            }

            if (Vcb->SuperBlock) {
                ExFreePool(Vcb->SuperBlock);
                Vcb->SuperBlock = NULL;
            }

            if (VcbResourceInitialized) {
                ExDeleteResourceLite(&Vcb->MainResource);
                ExDeleteResourceLite(&Vcb->PagingIoResource);
            }
        }
    } _SEH2_END;

    return Status;
}

__drv_mustHoldCriticalRegion
VOID
RfsdFreeVcb (IN PRFSD_VCB Vcb )
{
    PAGED_CODE();

    ASSERT(Vcb != NULL);
    
    ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
        (Vcb->Identifier.Size == sizeof(RFSD_VCB)));
    
    FsRtlNotifyUninitializeSync(&Vcb->NotifySync);

    if (Vcb->StreamObj) {
        if (IsFlagOn(Vcb->StreamObj->Flags, FO_FILE_MODIFIED)) {
            IO_STATUS_BLOCK    IoStatus;

            CcFlushCache(&(Vcb->SectionObject), NULL, 0, &IoStatus);
            ClearFlag(Vcb->StreamObj->Flags, FO_FILE_MODIFIED);
        }

        if (Vcb->StreamObj->PrivateCacheMap)
            RfsdSyncUninitializeCacheMap(Vcb->StreamObj);

        ObDereferenceObject(Vcb->StreamObj);
        Vcb->StreamObj = NULL;
    }

#if DBG
    if (FsRtlNumberOfRunsInLargeMcb(&(Vcb->DirtyMcbs)) != 0) {
        LONGLONG            DirtyVba;
        LONGLONG            DirtyLba;
        LONGLONG            DirtyLength;
        int                 i;

        for (i = 0; FsRtlGetNextLargeMcbEntry (&(Vcb->DirtyMcbs), i, &DirtyVba, &DirtyLba, &DirtyLength); i++)
        {
            RfsdPrint((DBG_INFO, "DirtyVba = %I64xh\n", DirtyVba));
            RfsdPrint((DBG_INFO, "DirtyLba = %I64xh\n", DirtyLba));
            RfsdPrint((DBG_INFO, "DirtyLen = %I64xh\n\n", DirtyLength));
        }

        DbgBreak();
   }
#endif

    FsRtlUninitializeLargeMcb(&(Vcb->DirtyMcbs));

    RfsdFreeMcbTree(Vcb->McbTree);

    if (Vcb->GroupDesc) {
        ExFreePool(Vcb->GroupDesc);
        // CcUnpinData(Vcb->GroupDescBcb);
        Vcb->GroupDesc = NULL;
    }
    
    if (Vcb->SuperBlock) {
        ExFreePool(Vcb->SuperBlock);
        Vcb->SuperBlock = NULL;
    }

    ExDeleteResourceLite(&Vcb->McbResource);

    ExDeleteResourceLite(&Vcb->PagingIoResource);

    ExDeleteResourceLite(&Vcb->MainResource);

    IoDeleteDevice(Vcb->DeviceObject);
}

VOID
RfsdSyncUninitializeCacheMap (
    IN PFILE_OBJECT FileObject
    )
{
    CACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent;
    NTSTATUS WaitStatus;
    LARGE_INTEGER RfsdLargeZero = {0,0};

    PAGED_CODE();

    KeInitializeEvent( &UninitializeCompleteEvent.Event,
                       SynchronizationEvent,
                       FALSE);

    CcUninitializeCacheMap( FileObject,
                            &RfsdLargeZero,
                            &UninitializeCompleteEvent );

    WaitStatus = KeWaitForSingleObject( &UninitializeCompleteEvent.Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

    ASSERT (NT_SUCCESS(WaitStatus));
}
