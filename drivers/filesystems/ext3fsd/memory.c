/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             memory.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"
#include <linux\ext3_fs.h>

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2AllocateIrpContext)
#pragma alloc_text(PAGE, Ext2FreeIrpContext)
#pragma alloc_text(PAGE, Ext2AllocateFcb)
#pragma alloc_text(PAGE, Ext2FreeFcb)
#pragma alloc_text(PAGE, Ext2AllocateInode)
#pragma alloc_text(PAGE, Ext2DestroyInode)
#pragma alloc_text(PAGE, Ext2AllocateExtent)
#pragma alloc_text(PAGE, Ext2FreeExtent)
#pragma alloc_text(PAGE, Ext2CountExtents)
#pragma alloc_text(PAGE, Ext2JointExtents)
#pragma alloc_text(PAGE, Ext2DestroyExtentChain)
#pragma alloc_text(PAGE, Ext2BuildExtents)
#pragma alloc_text(PAGE, Ext2ListExtents)
#pragma alloc_text(PAGE, Ext2CheckExtent)
#pragma alloc_text(PAGE, Ext2AddVcbExtent)
#pragma alloc_text(PAGE, Ext2RemoveVcbExtent)
#pragma alloc_text(PAGE, Ext2LookupVcbExtent)
#pragma alloc_text(PAGE, Ext2ClearAllExtents)
#pragma alloc_text(PAGE, Ext2AddMcbExtent)
#pragma alloc_text(PAGE, Ext2RemoveMcbExtent)
#pragma alloc_text(PAGE, Ext2LookupMcbExtent)
#pragma alloc_text(PAGE, Ext2AddBlockExtent)
#pragma alloc_text(PAGE, Ext2LookupBlockExtent)
#pragma alloc_text(PAGE, Ext2RemoveBlockExtent)
#pragma alloc_text(PAGE, Ext2BuildName)
#pragma alloc_text(PAGE, Ext2AllocateMcb)
#pragma alloc_text(PAGE, Ext2SearchMcb)
#pragma alloc_text(PAGE, Ext2InsertMcb)
#pragma alloc_text(PAGE, Ext2SearchMcbWithoutLock)
#pragma alloc_text(PAGE, Ext2RemoveMcb)
#pragma alloc_text(PAGE, Ext2CleanupAllMcbs)
#pragma alloc_text(PAGE, Ext2CheckBitmapConsistency)
#pragma alloc_text(PAGE, Ext2CheckSetBlock)
#pragma alloc_text(PAGE, Ext2InitializeVcb)
#pragma alloc_text(PAGE, Ext2FreeCcb)
#pragma alloc_text(PAGE, Ext2AllocateCcb)
#pragma alloc_text(PAGE, Ext2DestroyVcb)
#pragma alloc_text(PAGE, Ext2SyncUninitializeCacheMap)
#pragma alloc_text(PAGE, Ext2LinkHeadMcb)
#pragma alloc_text(PAGE, Ext2LinkTailMcb)
#pragma alloc_text(PAGE, Ext2UnlinkMcb)
#pragma alloc_text(PAGE, Ext2FirstUnusedMcb)
#pragma alloc_text(PAGE, Ext2ReaperThread)
#pragma alloc_text(PAGE, Ext2StartReaperThread)
#endif

PEXT2_IRP_CONTEXT
Ext2AllocateIrpContext (IN PDEVICE_OBJECT   DeviceObject,
                        IN PIRP             Irp )
{
    PIO_STACK_LOCATION   IoStackLocation;
    PEXT2_IRP_CONTEXT    IrpContext;
    
    ASSERT(DeviceObject != NULL);
    ASSERT(Irp != NULL);
    
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    
    IrpContext = (PEXT2_IRP_CONTEXT) (
        ExAllocateFromNPagedLookasideList(
            &(Ext2Global->Ext2IrpContextLookasideList)));

    if (IrpContext == NULL) {
        return NULL;
    }

    RtlZeroMemory(IrpContext, sizeof(EXT2_IRP_CONTEXT) );

    IrpContext->Identifier.Type = EXT2ICX;
    IrpContext->Identifier.Size = sizeof(EXT2_IRP_CONTEXT);

    IrpContext->Irp = Irp;
    IrpContext->MajorFunction = IoStackLocation->MajorFunction;
    IrpContext->MinorFunction = IoStackLocation->MinorFunction;
    IrpContext->DeviceObject = DeviceObject;
    IrpContext->FileObject = IoStackLocation->FileObject;
    if (NULL != IrpContext->FileObject) {
        IrpContext->Fcb = (PEXT2_FCB)IrpContext->FileObject->FsContext;
        IrpContext->Ccb = (PEXT2_CCB)IrpContext->FileObject->FsContext2;
    }

    if (IrpContext->FileObject != NULL) {
        IrpContext->RealDevice = IrpContext->FileObject->DeviceObject;
    } else if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) {
        if (IoStackLocation->Parameters.MountVolume.Vpb) {
            IrpContext->RealDevice = 
                IoStackLocation->Parameters.MountVolume.Vpb->RealDevice;
        }
    }

    if ( IrpContext->MajorFunction == IRP_MJ_CLEANUP ||
         IrpContext->MajorFunction == IRP_MJ_CLOSE ||
         IrpContext->MajorFunction == IRP_MJ_SHUTDOWN ||
         IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL ||
         IrpContext->MajorFunction == IRP_MJ_PNP ) {

        if ( IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL ||
             IrpContext->MajorFunction == IRP_MJ_PNP) {
            if (IoGetCurrentIrpStackLocation(Irp)->FileObject == NULL || 
                IoIsOperationSynchronous(Irp)) {
                SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
            }
        } else {
            SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
        }

    } else if (IoIsOperationSynchronous(Irp)) {

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
    }

    IrpContext->IsTopLevel = (IoGetTopLevelIrp() == Irp);
    IrpContext->ExceptionInProgress = FALSE;
    INC_IRP_COUNT(IrpContext);    

    return IrpContext;
}

VOID
Ext2FreeIrpContext (IN PEXT2_IRP_CONTEXT IrpContext)
{
    ASSERT(IrpContext != NULL);
    
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
        (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

    /* free the IrpContext to NonPagedList */
    IrpContext->Identifier.Type = 0;
    IrpContext->Identifier.Size = 0;

    DEC_IRP_COUNT(IrpContext);
    ExFreeToNPagedLookasideList(&(Ext2Global->Ext2IrpContextLookasideList), IrpContext);
}


PEXT2_FCB
Ext2AllocateFcb (
    IN PEXT2_VCB   Vcb,
    IN PEXT2_MCB   Mcb
    )
{
    PEXT2_FCB Fcb;

    ASSERT(!IsMcbSymlink(Mcb));

    Fcb = (PEXT2_FCB) ExAllocateFromNPagedLookasideList(
                            &(Ext2Global->Ext2FcbLookasideList));

    if (!Fcb) {
        return NULL;
    }

    RtlZeroMemory(Fcb, sizeof(EXT2_FCB));
    Fcb->Identifier.Type = EXT2FCB;
    Fcb->Identifier.Size = sizeof(EXT2_FCB);

#ifndef _WIN2K_TARGET_
    ExInitializeFastMutex(&Fcb->Mutex);
    FsRtlSetupAdvancedHeader(&Fcb->Header,  &Fcb->Mutex);
#endif

    FsRtlInitializeOplock(&Fcb->Oplock);
    FsRtlInitializeFileLock (
        &Fcb->FileLockAnchor,
        NULL,
        NULL );
    
    Fcb->OpenHandleCount = 0;
    Fcb->ReferenceCount = 0;
    Fcb->Vcb = Vcb;
    Fcb->Inode = Mcb->Inode;

    ASSERT(Mcb->Fcb == NULL);
    Ext2ReferMcb(Mcb);
    Fcb->Mcb = Mcb;
    Mcb->Fcb = Fcb;

    DEBUG(DL_INF, ("Ext2AllocateFcb: Fcb %p created: %wZ.\n",
                   Fcb, &Fcb->Mcb->FullName));

    RtlZeroMemory(&Fcb->Header, sizeof(FSRTL_COMMON_FCB_HEADER));
    Fcb->Header.NodeTypeCode = (USHORT) EXT2FCB;
    Fcb->Header.NodeByteSize = sizeof(EXT2_FCB);
    Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;
    Fcb->Header.Resource = &(Fcb->MainResource);
    Fcb->Header.PagingIoResource = &(Fcb->PagingIoResource);

    Fcb->Header.FileSize.QuadPart = Mcb->FileSize.QuadPart;
    Fcb->Header.AllocationSize.QuadPart = CEILING_ALIGNED(ULONGLONG, 
          Fcb->Header.FileSize.QuadPart, (ULONGLONG)Vcb->BlockSize);

    Fcb->Header.ValidDataLength.QuadPart = Mcb->FileSize.QuadPart;
    
    Fcb->SectionObject.DataSectionObject = NULL;
    Fcb->SectionObject.SharedCacheMap = NULL;
    Fcb->SectionObject.ImageSectionObject = NULL;

    ExInitializeResourceLite(&(Fcb->MainResource));
    ExInitializeResourceLite(&(Fcb->PagingIoResource));

    Ext2InsertFcb(Vcb, Fcb);

    INC_MEM_COUNT(PS_FCB, Fcb, sizeof(EXT2_FCB));
    
    return Fcb;
}

VOID
Ext2FreeFcb (IN PEXT2_FCB Fcb)
{
    ASSERT((Fcb != NULL) && (Fcb->Identifier.Type == EXT2FCB) &&
           (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
    ASSERT((Fcb->Mcb->Identifier.Type == EXT2MCB) &&
           (Fcb->Mcb->Identifier.Size == sizeof(EXT2_MCB)));

    if ((Fcb->Mcb->Identifier.Type == EXT2MCB) &&
        (Fcb->Mcb->Identifier.Size == sizeof(EXT2_MCB))) {

        if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED)) {
            ASSERT(!IsRoot(Fcb));
            Ext2RemoveMcb(Fcb->Vcb, Fcb->Mcb);
        }

        ASSERT (Fcb->Mcb->Fcb == Fcb);
        Fcb->Mcb->Fcb = NULL;
        Ext2DerefMcb(Fcb->Mcb);

    } else {
        DbgBreak();
    }

    Ext2RemoveFcb(Fcb->Vcb, Fcb);

    FsRtlUninitializeFileLock(&Fcb->FileLockAnchor);
    FsRtlUninitializeOplock(&Fcb->Oplock);
    ExDeleteResourceLite(&Fcb->MainResource);
    ExDeleteResourceLite(&Fcb->PagingIoResource);

    DEBUG(DL_INF, ( "Ext2FreeFcb: Fcb (%p) is being released: %wZ.\n",
                     Fcb, &Fcb->Mcb->FullName));

    Fcb->Identifier.Type = 0;
    Fcb->Identifier.Size = 0;

    ExFreeToNPagedLookasideList(&(Ext2Global->Ext2FcbLookasideList), Fcb);
    DEC_MEM_COUNT(PS_FCB, Fcb, sizeof(EXT2_FCB));
}

/* Insert Fcb to Vcb->FcbList queue */

VOID
Ext2InsertFcb(PEXT2_VCB Vcb, PEXT2_FCB Fcb)
{
    ExInterlockedInsertTailList(&Vcb->FcbList, &Fcb->Next, &Vcb->FcbLock);
    Ext2ReferXcb(&Vcb->FcbCount);
}

/* Remove Fcb from Vcb->FcbList queue */

VOID
Ext2RemoveFcb(PEXT2_VCB Vcb, PEXT2_FCB Fcb)
{
    KIRQL   irql;

    KeAcquireSpinLock(&Vcb->FcbLock, &irql);
    RemoveEntryList(&(Fcb->Next));
    if (Vcb->FcbCount > 0) {
        Ext2DerefXcb(&Vcb->FcbCount);
    } else {
        DbgBreak();
    }
    KeReleaseSpinLock(&Vcb->FcbLock, irql);
}

PEXT2_CCB
Ext2AllocateCcb (PEXT2_MCB  SymLink)
{
    PEXT2_CCB Ccb;

    Ccb = (PEXT2_CCB) (ExAllocateFromPagedLookasideList(
                        &(Ext2Global->Ext2CcbLookasideList)));
    if (!Ccb) {
        return NULL;
    }

    DEBUG(DL_INF, ( "ExtAllocateCcb: Ccb created: %ph.\n", Ccb));

    RtlZeroMemory(Ccb, sizeof(EXT2_CCB));
   
    Ccb->Identifier.Type = EXT2CCB;
    Ccb->Identifier.Size = sizeof(EXT2_CCB);
    
    Ccb->CurrentByteOffset = 0;
    Ccb->SymLink = SymLink;
    if (SymLink) {
        ASSERT(SymLink->Refercount > 0);
        Ext2ReferMcb(SymLink);
        DEBUG(DL_INF, ( "ExtAllocateCcb: Ccb SymLink: %wZ.\n",
                             &Ccb->SymLink->FullName));
    }
    
    Ccb->DirectorySearchPattern.Length = 0;
    Ccb->DirectorySearchPattern.MaximumLength = 0;
    Ccb->DirectorySearchPattern.Buffer = 0;

    INC_MEM_COUNT(PS_CCB, Ccb, sizeof(EXT2_CCB));

    return Ccb;
}

VOID
Ext2FreeCcb (IN PEXT2_CCB Ccb)
{
    ASSERT(Ccb != NULL);
    
    ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
        (Ccb->Identifier.Size == sizeof(EXT2_CCB)));

    DEBUG(DL_INF, ( "Ext2FreeCcb: Ccb = %ph.\n", Ccb));

    if (Ccb->SymLink) {
        DEBUG(DL_INF, ( "Ext2FreeCcb: Ccb SymLink: %wZ.\n",
                             &Ccb->SymLink->FullName));
        Ext2DerefMcb(Ccb->SymLink);
    }

    if (Ccb->DirectorySearchPattern.Buffer != NULL) {
        DEC_MEM_COUNT(PS_DIR_PATTERN, Ccb->DirectorySearchPattern.Buffer,
                      Ccb->DirectorySearchPattern.MaximumLength );
        ExFreePoolWithTag(Ccb->DirectorySearchPattern.Buffer, EXT2_DIRSP_MAGIC);
    }

    ExFreeToPagedLookasideList(&(Ext2Global->Ext2CcbLookasideList), Ccb);
    DEC_MEM_COUNT(PS_CCB, Ccb, sizeof(EXT2_CCB));
}

PEXT2_INODE
Ext2AllocateInode (PEXT2_VCB  Vcb)
{
    PEXT2_INODE inode = NULL;

    inode = (PEXT2_INODE) (ExAllocateFromNPagedLookasideList(
                            &(Vcb->InodeLookasideList)));
    if (!inode) {
        return NULL;
    }

    RtlZeroMemory(inode, INODE_SIZE);

    DEBUG(DL_INF, ("ExtAllocateInode: Inode created: %ph.\n", inode));
    // INC_MEM_COUNT(PS_EXT2_INODE, inode, INODE_SIZE);

    return inode;
}

VOID
Ext2DestroyInode (IN PEXT2_VCB Vcb, IN PEXT2_INODE inode)
{
    ASSERT(inode != NULL);
    
    DEBUG(DL_INF, ("Ext2FreeInode: Inode = %ph.\n", inode));

    ExFreeToNPagedLookasideList(&(Vcb->InodeLookasideList), inode);
    //DEC_MEM_COUNT(PS_EXT2_INODE, inode, INODE_SIZE);
}

PEXT2_EXTENT
Ext2AllocateExtent ()
{
    PEXT2_EXTENT Extent;

    Extent = (PEXT2_EXTENT)ExAllocateFromPagedLookasideList(
                            &(Ext2Global->Ext2ExtLookasideList));
    if (!Extent) {
        return NULL;
    }

    RtlZeroMemory(Extent, sizeof(EXT2_EXTENT));
    INC_MEM_COUNT(PS_EXTENT, Extent, sizeof(EXT2_EXTENT));

#if DBG
    if (Ext2Global->PerfStat.Current.Slot[PS_EXTENT] >= 500) {
        DbgBreak();
    }
#endif

    return Extent;
}

VOID
Ext2FreeExtent (IN PEXT2_EXTENT Extent)
{
    ASSERT(Extent != NULL);
    ExFreeToPagedLookasideList(&(Ext2Global->Ext2ExtLookasideList), Extent);
    DEC_MEM_COUNT(PS_EXTENT, Extent, sizeof(EXT2_EXTENT));
}

ULONG
Ext2CountExtents(IN PEXT2_EXTENT Chain)
{
    ULONG        count = 0;   
    PEXT2_EXTENT List = Chain;

    while (List) {
        count += 1;
        List = List->Next;
    }

    return count;
}

VOID
Ext2JointExtents(
    IN PEXT2_EXTENT Chain,
    IN PEXT2_EXTENT Extent
    )
{
    ULONG        count = 0;   
    PEXT2_EXTENT List = Chain;

    while (List->Next) {
        List = List->Next;
    }

    List->Next = Extent;
}


VOID
Ext2DestroyExtentChain(IN PEXT2_EXTENT Chain)
{
    PEXT2_EXTENT Extent = NULL, List = Chain;

    while (List) {
        Extent = List->Next;
        Ext2FreeExtent(List);
        List = Extent;
    }
}

BOOLEAN
Ext2ListExtents(PLARGE_MCB  Extents)
{
#if EXT2_DEBUG
    if (FsRtlNumberOfRunsInLargeMcb(Extents) != 0) {
 
        LONGLONG            DirtyVba;
        LONGLONG            DirtyLba;
        LONGLONG            DirtyLength;
        int                 i;

        for (i = 0; FsRtlGetNextLargeMcbEntry(
                        Extents, i, &DirtyVba,
                        &DirtyLba, &DirtyLength); i++)  {
            DEBUG(DL_EXT, ( "DirtyVba = %I64xh\n", DirtyVba));
            DEBUG(DL_EXT, ( "DirtyLba = %I64xh\n", DirtyLba));
            DEBUG(DL_EXT, ( "DirtyLen = %I64xh\n\n", DirtyLength));
        }

        return TRUE;
   }
#endif

    return FALSE;
}

VOID
Ext2CheckExtent(
    PLARGE_MCB  Zone,
    LONGLONG    Vbn,
    LONGLONG    Lbn,
    LONGLONG    Length,
    BOOLEAN     bAdded 
    )
{
#if EXT2_DEBUG
    LONGLONG    DirtyLbn;
    LONGLONG    DirtyLen;
    LONGLONG    RunStart;
    LONGLONG    RunLength;
    ULONG       Index;
    BOOLEAN     bFound = FALSE;

    bFound = FsRtlLookupLargeMcbEntry(
                        Zone,
                        Vbn,
                        &DirtyLbn,
                        &DirtyLen,
                        &RunStart,
                        &RunLength,
                        &Index );

    if (!bAdded && (!bFound || DirtyLbn == -1)) {
        return;
    }

    if ( !bFound || (DirtyLbn == -1) ||
         (DirtyLbn != Lbn) ||
         (DirtyLen < Length)) {

         DbgBreak();

         for (Index = 0; TRUE; Index++) {

            if (!FsRtlGetNextLargeMcbEntry(
                        Zone,
                        Index,
                        &Vbn,
                        &Lbn,
                        &Length)) {
                break;
            }

            DEBUG(DL_EXT, ("Index = %xh Vbn = %I64xh Lbn = %I64xh Len = %I64xh\n",
                            Index, Vbn, Lbn, Length ));
        }
    }
#endif
}

VOID
Ext2ClearAllExtents(PLARGE_MCB  Zone)
{
    __try {
        FsRtlTruncateLargeMcb(Zone, (LONGLONG)0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        DbgBreak();
    }
}


BOOLEAN
Ext2AddVcbExtent (
    IN PEXT2_VCB Vcb,
    IN LONGLONG  Vbn,
    IN LONGLONG  Length
    )
{
    ULONG       TriedTimes = 0;

    LONGLONG    Offset = 0;
    LONGLONG    Base = 0;
    UCHAR       Bits = 0;
    BOOLEAN     rc = FALSE;

    Base = (LONGLONG)SECTOR_SIZE;
    Bits = (UCHAR)SECTOR_BITS;
    Offset = Vbn & (~(Base - 1));
    Length = (Length + Vbn - Offset + Base - 1) & (~(Base - 1));

    ASSERT ((Offset & (Base - 1)) == 0);
    ASSERT ((Length & (Base - 1)) == 0);

    Offset =  (Offset >> Bits) + 1;
    Length = (Length >> Bits);

Again:

    __try {
        rc = FsRtlAddLargeMcbEntry(
                &Vcb->Extents,
                Offset,
                Offset,
                Length
                );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        DbgBreak();
        rc = FALSE;
    }

    if (!rc && ++TriedTimes < 10) {
        Ext2Sleep(TriedTimes * 100);
        goto Again;
    }

    DEBUG(DL_EXT, ("Ext2AddVcbExtent: Vbn=%I64xh Length=%I64xh,"
                    " rc=%d Runs=%u\n", Offset, Length, rc,
                    FsRtlNumberOfRunsInLargeMcb(&Vcb->Extents)));

    if (rc) {
        Ext2CheckExtent(&Vcb->Extents, Offset, Offset, Length, TRUE);
    }

    return rc;
}

BOOLEAN
Ext2RemoveVcbExtent (
    IN PEXT2_VCB Vcb,
    IN LONGLONG  Vbn,
    IN LONGLONG  Length
    )
{
    ULONG       TriedTimes = 0;
    LONGLONG    Offset = 0;
    LONGLONG    Base = 0;
    UCHAR       Bits = 0;
    BOOLEAN     rc = TRUE;

    Base = (LONGLONG)SECTOR_SIZE;
    Bits = (UCHAR)SECTOR_BITS;
    Offset =  Vbn & (~(Base - 1));
    Length = (Length + Vbn - Offset + Base - 1) & (~(Base - 1));

    ASSERT ((Offset & (Base - 1)) == 0);
    ASSERT ((Length & (Base - 1)) == 0);

    Offset = (Offset >> Bits) + 1;
    Length = (Length >> Bits);

Again:

    __try {
        FsRtlRemoveLargeMcbEntry(
                &Vcb->Extents,
                Offset,
                Length
                );
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        DbgBreak();
        rc = FALSE;
    }

    if (!rc && ++TriedTimes < 10) {
        Ext2Sleep(TriedTimes * 100);
        goto Again;
    }

    DEBUG(DL_EXT, ("Ext2RemoveVcbExtent: Vbn=%I64xh Length=%I64xh Runs=%u\n",
                    Offset, Length, FsRtlNumberOfRunsInLargeMcb(&Vcb->Extents)));
    if (rc) {
        Ext2CheckExtent(&Vcb->Extents, Offset, 0, Length, FALSE);
    }

    return rc;
}

BOOLEAN
Ext2LookupVcbExtent (
    IN PEXT2_VCB    Vcb,
    IN LONGLONG     Vbn,
    OUT PLONGLONG   Lbn,
    OUT PLONGLONG   Length
    )
{
    LONGLONG    offset;
    BOOLEAN     rc;

    offset = Vbn & (~((LONGLONG)SECTOR_SIZE - 1));
    ASSERT ((offset & (SECTOR_SIZE - 1)) == 0);
    offset = (offset >> SECTOR_BITS) + 1;

    rc = FsRtlLookupLargeMcbEntry(
                &(Vcb->Extents),
                offset,
                Lbn,
                Length,
                NULL,
                NULL,
                NULL
            );

    if (rc) {

        if (Lbn && ((*Lbn) != -1)) {
            ASSERT((*Lbn) > 0);
            (*Lbn) = (((*Lbn) - 1) << SECTOR_BITS);
            (*Lbn) += ((Vbn) & ((LONGLONG)SECTOR_SIZE - 1));
        }

        if (Length && *Length) {
            (*Length) <<= SECTOR_BITS;
            (*Length)  -= ((Vbn) & ((LONGLONG)SECTOR_SIZE - 1));
        }
    }

    return rc;
}


BOOLEAN
Ext2AddMcbExtent (
    IN PEXT2_VCB Vcb,
    IN PEXT2_MCB Mcb,
    IN LONGLONG  Vbn,
    IN LONGLONG  Lbn,
    IN LONGLONG  Length
    )
{
    ULONG       TriedTimes = 0;
    LONGLONG    Base = 0;
    UCHAR       Bits = 0;
    BOOLEAN     rc = FALSE;

    Base = (LONGLONG)BLOCK_SIZE;
    Bits = (UCHAR)BLOCK_BITS;

    ASSERT ((Vbn & (Base - 1)) == 0);
    ASSERT ((Lbn & (Base - 1)) == 0);
    ASSERT ((Length & (Base - 1)) == 0);

    Vbn = (Vbn >> Bits) + 1;
    Lbn =  (Lbn >> Bits) + 1;
    Length = (Length >> Bits);

Again:

    __try {

        rc = FsRtlAddLargeMcbEntry(
                &Mcb->Extents,
                Vbn,
                Lbn,
                Length
                );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        DbgBreak();
        rc = FALSE;
    }

    if (!rc && ++TriedTimes < 10) {
        Ext2Sleep(TriedTimes * 100);
        goto Again;
    }

    DEBUG(DL_EXT, ("Ext2AddMcbExtent: Vbn=%I64xh Lbn=%I64xh Length=%I64xh,"
                    " rc=%d Runs=%u\n", Vbn, Lbn, Length, rc,
                    FsRtlNumberOfRunsInLargeMcb(&Mcb->Extents)));

    if (rc) {
        Ext2CheckExtent(&Mcb->Extents, Vbn, Lbn, Length, TRUE);
    }

    return rc;
}

BOOLEAN
Ext2RemoveMcbExtent (
    IN PEXT2_VCB Vcb,
    IN PEXT2_MCB Mcb,
    IN LONGLONG  Vbn,
    IN LONGLONG  Length
    )
{
    ULONG       TriedTimes = 0;
    LONGLONG    Base = 0;
    UCHAR       Bits = 0;
    BOOLEAN     rc = TRUE;

    Base = (LONGLONG)BLOCK_SIZE;
    Bits = (UCHAR)BLOCK_BITS;

    ASSERT ((Vbn & (Base - 1)) == 0);
    ASSERT ((Length & (Base - 1)) == 0);

    Vbn = (Vbn >> Bits) + 1;
    Length = (Length >> Bits);

Again:

    __try {
        FsRtlRemoveLargeMcbEntry(
                &Mcb->Extents,
                Vbn,
                Length
                );
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        DbgBreak();
        rc = FALSE;
    }

    if (!rc && ++TriedTimes < 10) {
        Ext2Sleep(TriedTimes * 100);
        goto Again;
    }

    DEBUG(DL_EXT, ("Ext2RemoveMcbExtent: Vbn=%I64xh Length=%I64xh Runs=%u\n",
                    Vbn, Length, FsRtlNumberOfRunsInLargeMcb(&Mcb->Extents)));
    if (rc) {
        Ext2CheckExtent(&Mcb->Extents, Vbn, 0, Length, FALSE);
    }

    return rc;
}

BOOLEAN
Ext2LookupMcbExtent (
    IN PEXT2_VCB    Vcb,
    IN PEXT2_MCB    Mcb,
    IN LONGLONG     Vbn,
    OUT PLONGLONG   Lbn,
    OUT PLONGLONG   Length
    )
{
    LONGLONG    offset;
    BOOLEAN     rc;

    offset = Vbn & (~((LONGLONG)BLOCK_SIZE - 1));
    ASSERT ((offset & (BLOCK_SIZE - 1)) == 0);
    offset = (offset >> BLOCK_BITS) + 1;

    rc = FsRtlLookupLargeMcbEntry(
                &(Mcb->Extents),
                offset,
                Lbn,
                Length,
                NULL,
                NULL,
                NULL
            );

    if (rc) {

        if (Lbn && ((*Lbn) != -1)) {
            ASSERT((*Lbn) > 0);
            (*Lbn) = (((*Lbn) - 1) << BLOCK_BITS);
            (*Lbn) += ((Vbn) & ((LONGLONG)BLOCK_SIZE - 1));
        }

        if (Length && *Length) {
            (*Length) <<= BLOCK_BITS;
            (*Length)  -= ((Vbn) & ((LONGLONG)BLOCK_SIZE - 1));
        }
    }

    return rc;
}

BOOLEAN
Ext2AddBlockExtent(
    IN PEXT2_VCB    Vcb,
    IN PEXT2_MCB    Mcb,
    IN ULONG        Start,
    IN ULONG        Block,
    IN ULONG        Number 
    )
{
    LONGLONG    Vbn = 0;
    LONGLONG    Lbn = 0;
    LONGLONG    Length = 0;

    Vbn = ((LONGLONG) Start) << BLOCK_BITS;
    Lbn = ((LONGLONG) Block) << BLOCK_BITS;
    Length = ((LONGLONG)Number << BLOCK_BITS);

    if (Mcb) {
#if EXT2_DEBUG
        ULONG   _block = 0, _mapped = 0;
        BOOLEAN _rc = Ext2LookupBlockExtent(Vcb, Mcb, Start, &_block, &_mapped);
        if (_rc && _block != 0 && (_block != Block)) {
            DbgBreak();
        }
#endif
        return Ext2AddMcbExtent(Vcb, Mcb, Vbn, Lbn, Length);

    }

    ASSERT(Start == Block);
    return Ext2AddVcbExtent(Vcb, Vbn, Length);
}

BOOLEAN
Ext2LookupBlockExtent(
    IN PEXT2_VCB    Vcb,
    IN PEXT2_MCB    Mcb,
    IN ULONG        Start,
    IN PULONG       Block,
    IN PULONG       Mapped
    )
{
    LONGLONG    Vbn = 0;
    LONGLONG    Lbn = 0;
    LONGLONG    Length = 0;

    BOOLEAN     rc = FALSE;

    Vbn = ((LONGLONG) Start) << BLOCK_BITS;

    if (Mcb) {
        rc = Ext2LookupMcbExtent(Vcb, Mcb, Vbn, &Lbn, &Length);
    } else {
        rc = Ext2LookupVcbExtent(Vcb, Vbn, &Lbn, &Length);
    }

    if (rc) {
        *Mapped = (ULONG)(Length >> BLOCK_BITS);
        if (Lbn != -1 && Length > 0) {
            *Block = (ULONG)(Lbn >> BLOCK_BITS);
        } else {
            *Block = 0;
        }
    }

    return rc;
}


BOOLEAN
Ext2RemoveBlockExtent(
    IN PEXT2_VCB    Vcb,
    IN PEXT2_MCB    Mcb,
    IN ULONG        Start,
    IN ULONG        Number 
    )
{
    LONGLONG    Vbn = 0;
    LONGLONG    Length = 0;
    BOOLEAN     rc;

    Vbn = ((LONGLONG) Start) << BLOCK_BITS;
    Length = ((LONGLONG)Number << BLOCK_BITS);

    if (Mcb) {
        rc = Ext2RemoveMcbExtent(Vcb, Mcb, Vbn, Length);
    } else {
        rc = Ext2RemoveVcbExtent(Vcb, Vbn, Length);
    }

    return rc;
}

NTSTATUS
Ext2InitializeZone( 
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    ULONG       Start = 0;
    ULONG       End;
    ULONG       Block;
    ULONG       Mapped;

    End = (ULONG)((Mcb->FileSize.QuadPart + BLOCK_SIZE - 1) >> BLOCK_BITS);

    while (Start < End) {

        Block = Mapped = 0;

        /* mapping file offset to ext2 block */
        Status = Ext2BlockMap(
                        IrpContext,
                        Vcb,
                        Mcb,
                        Start,
                        FALSE,
                        &Block,
                        &Mapped
                        );

        if (!NT_SUCCESS(Status)) {
            goto errorout;
        }

        /* skip wrong blocks, in case wrongly treating symlink
           target names as blocks, silly  */
        if (Block >= TOTAL_BLOCKS) {
            Block = 0;
        }

        if (Block) {
            if (!Ext2AddBlockExtent(Vcb, Mcb, Start, Block, Mapped)) {
                DbgBreak();
                ClearFlag(Mcb->Flags, MCB_ZONE_INIT);
                Ext2ClearAllExtents(&Mcb->Extents);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto errorout;
            }
            DEBUG(DL_MAP, ("Ext2InitializeZone %wZ: Block = %xh Mapped = %xh\n",
                            &Mcb->FullName, Block, Mapped));
        }
  
        /* Mapped is total number of continous blocks or NULL blocks */
        Start += Mapped;
    }
 
    /* set the initialized flag */
    SetLongFlag(Mcb->Flags, MCB_ZONE_INIT);

errorout:

    return Status;
}

NTSTATUS
Ext2BuildExtents( 
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONGLONG            Offset, 
    IN ULONG                Size, 
    IN BOOLEAN              bAlloc,
    OUT PEXT2_EXTENT *      Chain
    )
{
    ULONG       Start, End;
    ULONG       Total = 0;

    LONGLONG    Lba = 0;
    NTSTATUS    Status = STATUS_SUCCESS;

    PEXT2_EXTENT  Extent = NULL;
    PEXT2_EXTENT  List = *Chain;

    BOOLEAN     IsFirstBlock = TRUE;

    if (!IsZoneInited(Mcb)) {
        Status = Ext2InitializeZone(IrpContext, Vcb, Mcb);
        if (!NT_SUCCESS(Status)) {
            DbgBreak();
            ClearLongFlag(Mcb->Flags, MCB_ZONE_INIT);
            goto errorout;
        }
    }

    if ((IrpContext && IrpContext->Irp) && 
        ((IrpContext->Irp->Flags & IRP_NOCACHE) || 
         (IrpContext->Irp->Flags & IRP_PAGING_IO))) {
        Size = (Size + SECTOR_SIZE - 1) & (~(SECTOR_SIZE - 1));
    }

    Start = (ULONG)(Offset >> BLOCK_BITS);
    End = (ULONG)((Size + Offset + BLOCK_SIZE - 1) >> BLOCK_BITS);

    if (End > (ULONG)((Mcb->FileSize.QuadPart + BLOCK_SIZE - 1) >> BLOCK_BITS) ) {
        End = (ULONG)((Mcb->FileSize.QuadPart + BLOCK_SIZE - 1) >> BLOCK_BITS);
    }

    while (Size > 0 && Start < End) {

        ULONG   Mapped = 0;
        ULONG   Length = 0;
        ULONG   Block = 0;

        BOOLEAN rc = FALSE;

        /* try to map file offset to ext2 block upon Extents cache */
        if (IsZoneInited(Mcb)) {
            rc = Ext2LookupBlockExtent(
                        Vcb,    
                        Mcb,
                        Start,
                        &Block,
                        &Mapped);

            if (!rc) {
                DbgBreak();
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto errorout;
            }
        }

        /* try to BlockMap in case failed to access Extents cache */
        if (!IsZoneInited(Mcb) || (bAlloc && Block == 0)) {
            Status = Ext2BlockMap(
                        IrpContext,
                        Vcb,
                        Mcb,
                        Start,
                        bAlloc,
                        &Block,
                        &Mapped
                        );

            if (!NT_SUCCESS(Status)) {
                goto errorout;
            }
        }

        /* Mapped is total number of continous blocks or NULL blocks */
        Start += Mapped;
        Length = (Mapped << BLOCK_BITS);

        /* the first request's offset might not be BLOCK_SIZE aligned */
        if (IsFirstBlock) {
            Lba = ((ULONG)Offset & (BLOCK_SIZE - 1));
            IsFirstBlock = FALSE;
        } else {
            Lba = 0;
        }
        Length -= (ULONG)Lba;

        Lba += ((LONGLONG) Block) << BLOCK_BITS;;
        if (Size < Length) {
            Length = Size;
        }

        /* skip wrong blocks, in case wrongly treating symlink
           target names as blocks, silly  */
        if (Block >= TOTAL_BLOCKS) {
            Block = 0;
        }

        if (Block != 0) {

            if (List && List->Lba + List->Length == Lba) {
                /* it's continuous upon previous Extent */
                List->Length += Length;
            } else {
                /* have to allocate a new Extent */
                Extent = Ext2AllocateExtent();
                if (!Extent) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto errorout;
                }

                Extent->Lba = Lba;
                Extent->Length = Length;
                Extent->Offset = Total;

                /* insert new Extent to chain */
                if (List) {
                    List->Next = Extent;
                    List = Extent;
                } else {
                    *Chain = List = Extent;
                }
            }
        }

        Total += Length;
        Size  -= Length;
    }
 
errorout:

    return Status;
}


BOOLEAN
Ext2BuildName(
    IN OUT PUNICODE_STRING  Target,
    IN PUNICODE_STRING      File,
    IN PUNICODE_STRING      Parent
    )
{
    USHORT  Length = 0;
    USHORT  ParentLen = 0;
    BOOLEAN bBackslash = TRUE;

    /* free the original buffer */
    if (Target->Buffer) {
        DEC_MEM_COUNT(PS_MCB_NAME, Target->Buffer, Target->MaximumLength);
        ExFreePoolWithTag(Target->Buffer, EXT2_FNAME_MAGIC);
        Target->Length = Target->MaximumLength = 0;
    }

    /* check the parent directory's name and backslash */
    if (Parent && Parent->Buffer && Parent->Length > 0) {
        ParentLen = Parent->Length / sizeof(WCHAR);
        if (Parent->Buffer[ParentLen - 1] == L'\\') {
            bBackslash = FALSE;
        }
    }

    if (Parent == NULL || File->Buffer[0] == L'\\') {
        /* must be root inode */
        ASSERT(ParentLen == 0);
        bBackslash = FALSE;
    }

    /* allocate and initialize new name buffer */
    Length  = File->Length;
    Length += (ParentLen + (bBackslash ? 1 : 0)) * sizeof(WCHAR);

    Target->Buffer = ExAllocatePoolWithTag(
                         PagedPool,
                         Length + 2,
                         EXT2_FNAME_MAGIC
                         );

    if (!Target->Buffer) {
        DEBUG(DL_ERR, ( "Ex2BuildName: failed to allocate name bufer.\n"));
        return FALSE;
    }
    RtlZeroMemory(Target->Buffer, Length + 2);

    if (ParentLen) {
        RtlCopyMemory(&Target->Buffer[0], 
                       Parent->Buffer,
                       ParentLen * sizeof(WCHAR));
    }

    if (bBackslash) {
        Target->Buffer[ParentLen++] = L'\\';
    }

    RtlCopyMemory( &Target->Buffer[ParentLen],
                    File->Buffer,
                    File->Length);

    INC_MEM_COUNT(PS_MCB_NAME, Target->Buffer, Length + 2);
    Target->Length = Length;
    Target->MaximumLength = Length + 2;

    return TRUE;
}

PEXT2_MCB
Ext2AllocateMcb (
    IN PEXT2_VCB        Vcb,
    IN PUNICODE_STRING  FileName,
    IN PUNICODE_STRING  Parent,
    IN ULONG            FileAttr
    )
{
    PEXT2_MCB   Mcb = NULL;
    NTSTATUS    Status = STATUS_SUCCESS;

    /* need wake the reaper thread if there are many Mcb allocated */
    if (Ext2Global->PerfStat.Current.Mcb > (((ULONG)Ext2Global->MaxDepth) * 4)) {
        KeSetEvent(&Ext2Global->Reaper.Wait, 0, FALSE);
    }

    /* allocate Mcb from LookasideList */
    Mcb = (PEXT2_MCB) (ExAllocateFromPagedLookasideList(
                         &(Ext2Global->Ext2McbLookasideList)));
  
    if (Mcb == NULL) {
        return NULL;
    }

    /* initialize Mcb header */
    RtlZeroMemory(Mcb, sizeof(EXT2_MCB));
    Mcb->Identifier.Type = EXT2MCB;
    Mcb->Identifier.Size = sizeof(EXT2_MCB);
    Mcb->FileAttr = FileAttr;

    /* allocate inode structure */
    Mcb->Inode = Ext2AllocateInode(Vcb);
    if (Mcb->Inode == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorout;
    }

    /* initialize Mcb names */
    if (FileName) {

#if EXT2_DEBUG
        if ( FileName->Length == 2 &&
             FileName->Buffer[0] == L'\\') {
            DEBUG(DL_RES, ( "Ext2AllocateMcb: Root Mcb is to be created !\n"));
        }

        if ( FileName->Length == 2 &&
             FileName->Buffer[0] == L'.') {
            DbgBreak();
        }

        if ( FileName->Length == 4 &&
             FileName->Buffer[0] == L'.' &&
             FileName->Buffer[1] == L'.' ) {
            DbgBreak();
        }
#endif

        if (( FileName->Length >= 4 && FileName->Buffer[0] == L'.') &&
            ((FileName->Length == 4 && FileName->Buffer[1] != L'.') ||
              FileName->Length >= 6 )) {
            SetFlag(Mcb->FileAttr, FILE_ATTRIBUTE_HIDDEN);
        }

        if (!Ext2BuildName(&Mcb->ShortName, FileName, NULL)) {
            goto errorout;
        }
        if (!Ext2BuildName(&Mcb->FullName, FileName, Parent)) {
            goto errorout;
        }
    }

    /* initialize Mcb Extents, it will raise an expcetion if failed */
    __try {
        FsRtlInitializeLargeMcb(&(Mcb->Extents), PagedPool);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DbgBreak();
    }

    if (!NT_SUCCESS(Status)) {
        goto errorout;
    }

    INC_MEM_COUNT(PS_MCB, Mcb, sizeof(EXT2_MCB));
    DEBUG(DL_INF, ( "Ext2AllocateMcb: Mcb %wZ created.\n", &Mcb->FullName));

    return Mcb;

errorout:

    if (Mcb) {

        if (Mcb->Inode) {
            Ext2DestroyInode(Vcb, Mcb->Inode);
        }

        if (Mcb->ShortName.Buffer) {
            DEC_MEM_COUNT(PS_MCB_NAME, Mcb->ShortName.Buffer,
                          Mcb->ShortName.MaximumLength);
            ExFreePoolWithTag(Mcb->ShortName.Buffer, EXT2_FNAME_MAGIC);
        }

        if (Mcb->FullName.Buffer) {
            DEC_MEM_COUNT(PS_MCB_NAME, Mcb->FullName.Buffer,
                          Mcb->FullName.MaximumLength);
            ExFreePoolWithTag(Mcb->FullName.Buffer, EXT2_FNAME_MAGIC);
        }

        ExFreeToPagedLookasideList(&(Ext2Global->Ext2McbLookasideList), Mcb);
    }

    return NULL;
}

VOID
Ext2FreeMcb (IN PEXT2_VCB Vcb, IN PEXT2_MCB Mcb)
{
    PEXT2_MCB   Parent = Mcb->Parent;

    ASSERT(Mcb != NULL);
    
    ASSERT((Mcb->Identifier.Type == EXT2MCB) &&
        (Mcb->Identifier.Size == sizeof(EXT2_MCB)));

    if ((Mcb->Identifier.Type != EXT2MCB) ||
        (Mcb->Identifier.Size != sizeof(EXT2_MCB))) {
        return;
    }

    DEBUG(DL_INF, ( "Ext2FreeMcb: Mcb %wZ will be freed.\n", &Mcb->FullName));

    if (IsMcbSymlink(Mcb) && Mcb->Target) {
        Ext2DerefMcb(Mcb->Target);
    }

    DEBUG(DL_EXT, ("Ext2FreeMcb ...: %wZ\n", &Mcb->FullName));
    Ext2ListExtents(&Mcb->Extents);
    FsRtlUninitializeLargeMcb(&(Mcb->Extents));
    ClearLongFlag(Mcb->Flags, MCB_ZONE_INIT);

    if (Mcb->Inode) {
        Ext2DestroyInode(Vcb, Mcb->Inode);
    }

    if (Mcb->ShortName.Buffer) {
        DEC_MEM_COUNT(PS_MCB_NAME, Mcb->ShortName.Buffer,
                      Mcb->ShortName.MaximumLength);
        ExFreePoolWithTag(Mcb->ShortName.Buffer, EXT2_FNAME_MAGIC);
    }

    if (Mcb->FullName.Buffer) {
        DEC_MEM_COUNT(PS_MCB_NAME, Mcb->FullName.Buffer,
                      Mcb->FullName.MaximumLength);
        ExFreePoolWithTag(Mcb->FullName.Buffer, EXT2_FNAME_MAGIC);
    }

    Mcb->Identifier.Type = 0;
    Mcb->Identifier.Size = 0;

    ExFreeToPagedLookasideList(&(Ext2Global->Ext2McbLookasideList), Mcb);
    DEC_MEM_COUNT(PS_MCB, Mcb, sizeof(EXT2_MCB));
}


PEXT2_MCB
Ext2SearchMcb(
    PEXT2_VCB           Vcb,
    PEXT2_MCB           Parent,
    PUNICODE_STRING     FileName
    )
{
    BOOLEAN   LockAcquired = FALSE;
    PEXT2_MCB Mcb = NULL;

    __try {
        ExAcquireResourceSharedLite(&Vcb->McbLock, TRUE);
        LockAcquired = TRUE;
        Mcb = Ext2SearchMcbWithoutLock(Parent, FileName);
    } __finally {
        if (LockAcquired) {
            ExReleaseResourceLite(&Vcb->McbLock);
        }
    }

    return Mcb;
}


PEXT2_MCB
Ext2SearchMcbWithoutLock(
    PEXT2_MCB       Parent,
    PUNICODE_STRING FileName
    )
{
    PEXT2_MCB TmpMcb = NULL;

    DEBUG(DL_RES, ("Ext2SearchMcb: %wZ\n", FileName));

    __try {

        Ext2ReferMcb(Parent);

        if ( FileName->Length == 2 &&
             FileName->Buffer[0] == L'.') {
            TmpMcb = Parent;
            Ext2ReferMcb(Parent);
            __leave;
        }

        if ( FileName->Length == 4 &&
             FileName->Buffer[0] == L'.' &&
             FileName->Buffer[1] == L'.' ) {
            if (IsMcbRoot(Parent)) {
                TmpMcb = Parent;
            } else {
                TmpMcb = Parent->Parent;
            }
            if (TmpMcb) {
                Ext2ReferMcb(TmpMcb);
            }
            __leave;
        }

        if (IsMcbSymlink(Parent)) {
            if (Parent->Target) {
                TmpMcb = Parent->Target->Child;
                ASSERT(!IsMcbSymlink(Parent->Target));
            } else {
                TmpMcb = NULL;
                __leave;
            }
        } else {
            TmpMcb = Parent->Child;
        }

        while (TmpMcb) {

            if (!RtlCompareUnicodeString(
                        &(TmpMcb->ShortName),
                        FileName, TRUE )) {
                Ext2ReferMcb(TmpMcb);
                break;
            }

            TmpMcb = TmpMcb->Next;
        }

    } __finally {

        Ext2DerefMcb(Parent);
    }

    return TmpMcb;
}

VOID
Ext2InsertMcb (
    PEXT2_VCB Vcb,
    PEXT2_MCB Parent,
    PEXT2_MCB Child
    )
{
    BOOLEAN     LockAcquired = FALSE;
    PEXT2_MCB   Mcb = NULL;

    __try {

        ExAcquireResourceExclusiveLite(
            &Vcb->McbLock,
            TRUE );
        LockAcquired = TRUE;

        /* use it's target if it's a symlink */
        if (IsMcbSymlink(Parent)) {
            Parent = Parent->Target;
        }
        ASSERT(!IsMcbSymlink(Parent));

        Mcb = Parent->Child;
        while (Mcb) {
            if (Mcb == Child) {
                break;
            }
            Mcb = Mcb->Next;
        }

        if (Mcb) {
            /* already attached in the list */
            DEBUG(DL_ERR, ( "Ext2InsertMcb: Child Mcb is alreay attached.\n"));
            if (!IsFlagOn(Mcb->Flags, MCB_ENTRY_TREE)) {
                SetLongFlag(Child->Flags, MCB_ENTRY_TREE);
                DEBUG(DL_ERR, ( "Ext2InsertMcb: Child Mcb's flag isn't set.\n"));
            }

            DbgBreak();

        } else {

            /* insert this Mcb into the head */
            Child->Next = Parent->Child;
            Parent->Child = Child;
            Child->Parent = Parent;
            Ext2ReferMcb(Parent);
            SetLongFlag(Child->Flags, MCB_ENTRY_TREE);
        }

    } __finally {

        if (LockAcquired) {
            ExReleaseResourceLite(&Vcb->McbLock);
        }
    }
}

BOOLEAN
Ext2RemoveMcb (
    PEXT2_VCB Vcb,
    PEXT2_MCB Mcb
    )
{
    PEXT2_MCB   TmpMcb = NULL;
    BOOLEAN     LockAcquired = FALSE;
    BOOLEAN     bLinked = FALSE;

    __try {

        ExAcquireResourceExclusiveLite(&Vcb->McbLock, TRUE);
        LockAcquired = TRUE;

        if (Mcb->Parent) {

            if (Mcb->Parent->Child == Mcb) {
                Mcb->Parent->Child = Mcb->Next;
                bLinked = TRUE;
            } else {
                TmpMcb = Mcb->Parent->Child;

                while (TmpMcb && TmpMcb->Next != Mcb) {
                    TmpMcb = TmpMcb->Next;
                }

                if (TmpMcb) {
                    TmpMcb->Next = Mcb->Next;
                    bLinked = TRUE;
                } else {
                    /* we got errors: link broken */
                }
            }

            if (bLinked) {
                if (IsFlagOn(Mcb->Flags, MCB_ENTRY_TREE)) {
                    Ext2DerefMcb(Mcb->Parent);
                    ClearLongFlag(Mcb->Flags, MCB_ENTRY_TREE);
                } else {
                    DbgBreak();
                }
            } else {
                if (IsFlagOn(Mcb->Flags, MCB_ENTRY_TREE)) {
                    ClearLongFlag(Mcb->Flags, MCB_ENTRY_TREE);
                }
                DbgBreak();
            }
            Mcb->Parent = NULL;
        }

    } __finally {

        if (LockAcquired) {
            ExReleaseResourceLite(&Vcb->McbLock);
        }
    }

    return TRUE;
}

VOID
Ext2CleanupAllMcbs(PEXT2_VCB Vcb)
{
    BOOLEAN   LockAcquired = FALSE;
    PEXT2_MCB Mcb = NULL;

    __try {

        ExAcquireResourceExclusiveLite(
            &Vcb->McbLock,
            TRUE );
        LockAcquired = TRUE;

        while (Mcb = Ext2FirstUnusedMcb(Vcb, TRUE, Vcb->NumOfMcb)) {
            while (Mcb) {
                PEXT2_MCB Next = Mcb->Next;
                if (IsMcbSymlink(Mcb)) {
                    Mcb->Target = NULL;
                }
                Ext2FreeMcb(Vcb, Mcb);
                Mcb = Next;
            }
        }
        Ext2FreeMcb(Vcb, Vcb->McbTree);
        Vcb->McbTree = NULL;

    } __finally {

        if (LockAcquired) {
            ExReleaseResourceLite(&Vcb->McbLock);
        }
    }
}

BOOLEAN
Ext2CheckSetBlock(PEXT2_IRP_CONTEXT IrpContext, PEXT2_VCB Vcb, ULONG Block)
{
    ULONG           Group, dwBlk, Length;

    RTL_BITMAP      BlockBitmap;
    PVOID           BitmapCache;
    PBCB            BitmapBcb;

    LARGE_INTEGER   Offset;

    BOOLEAN         bModified = FALSE;


    Group = (Block - EXT2_FIRST_DATA_BLOCK) / BLOCKS_PER_GROUP;

    dwBlk = (Block - EXT2_FIRST_DATA_BLOCK) % BLOCKS_PER_GROUP;


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
        
    if (!CcPinRead( Vcb->Volume,
                        &Offset,
                        Vcb->BlockSize,
                        Ext2CanIWait(),
                        &BitmapBcb,
                        &BitmapCache ) ) {

        DEBUG(DL_ERR, ( "Ext2CheckSetBlock: Failed to PinLock block %xh ...\n",
                         Vcb->GroupDesc[Group].bg_block_bitmap));
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
        Ext2AddVcbExtent(Vcb, Offset.QuadPart, (LONGLONG)BLOCK_SIZE);
    }

    {
        CcUnpinData(BitmapBcb);
        BitmapBcb = NULL;
        BitmapCache = NULL;

        RtlZeroMemory(&BlockBitmap, sizeof(RTL_BITMAP));
    }
      
    return (!bModified);
}

BOOLEAN
Ext2CheckBitmapConsistency(PEXT2_IRP_CONTEXT IrpContext, PEXT2_VCB Vcb)
{
    ULONG i, j, InodeBlocks;

    for (i = 0; i < Vcb->NumOfGroups; i++) {

        Ext2CheckSetBlock(IrpContext, Vcb, Vcb->GroupDesc[i].bg_block_bitmap);
        Ext2CheckSetBlock(IrpContext, Vcb, Vcb->GroupDesc[i].bg_inode_bitmap);
        
        
        if (i == Vcb->NumOfGroups - 1) {
            InodeBlocks = ((INODES_COUNT % INODES_PER_GROUP) * 
                            INODE_SIZE + Vcb->BlockSize - 1) /
                            (Vcb->BlockSize);
        } else {
            InodeBlocks = (INODES_PER_GROUP * INODE_SIZE + 
                           Vcb->BlockSize - 1) / (Vcb->BlockSize);
        }

        for (j = 0; j < InodeBlocks; j++ )
            Ext2CheckSetBlock(IrpContext, Vcb, Vcb->GroupDesc[i].bg_inode_table + j);
    }

    return TRUE;
}

/* Ext2Global->Resource should be already acquired */
VOID
Ext2InsertVcb(PEXT2_VCB Vcb)
{
    InsertTailList(&(Ext2Global->VcbList), &Vcb->Next);
}


/* Ext2Global->Resource should be already acquired */
VOID
Ext2RemoveVcb(PEXT2_VCB Vcb)
{
    RemoveEntryList(&Vcb->Next);
}

NTSTATUS
Ext2QueryVolumeParams(IN PEXT2_VCB Vcb, IN PUNICODE_STRING Params)
{
    NTSTATUS                    Status;
    RTL_QUERY_REGISTRY_TABLE    QueryTable[2];

    UNICODE_STRING              UniName;
    PUSHORT                     UniBuffer = NULL;

    USHORT                      UUID[50];

    int                         i;
    int                         len = 0;

    /* zero params */
    RtlZeroMemory(Params, sizeof(UNICODE_STRING));

    /* constructing volume UUID name */
    memset(UUID, 0, sizeof(USHORT) * 50);
    for (i=0; i < 16; i++) {
        if (i == 0) {
            swprintf((wchar_t *)&UUID[len], L"{%2.2X",Vcb->SuperBlock->s_uuid[i]);
            len += 3;
        } else if (i == 15) {
            swprintf((wchar_t *)&UUID[len], L"-%2.2X}", Vcb->SuperBlock->s_uuid[i]);
            len +=4;
        } else {
            swprintf((wchar_t *)&UUID[len], L"-%2.2X", Vcb->SuperBlock->s_uuid[i]);
            len += 3;
        }
    }

    /* allocating memory for UniBuffer */
    UniBuffer = ExAllocatePoolWithTag(PagedPool, 1024, EXT2_PARAM_MAGIC);
    if (NULL == UniBuffer) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorout;
    }
    RtlZeroMemory(UniBuffer, 1024);

    /* querying volume parameter string */
    RtlZeroMemory(&QueryTable[0], sizeof(RTL_QUERY_REGISTRY_TABLE) * 2);
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = UUID;
    QueryTable[0].EntryContext = &(UniName);
    UniName.MaximumLength = 1024;
    UniName.Length = 0;
    UniName.Buffer = UniBuffer;

    Status = RtlQueryRegistryValues(
                RTL_REGISTRY_ABSOLUTE,
                Ext2Global->RegistryPath.Buffer,
                &QueryTable[0],
                NULL,
                NULL
                );

    if (!NT_SUCCESS(Status)) {
        goto errorout;
    }

 errorout:

    if (NT_SUCCESS(Status)) {
        *Params = UniName;
    } else {
        if (UniBuffer) {
            ExFreePoolWithTag(UniBuffer, EXT2_PARAM_MAGIC);
        }
    }

    return Status;
}

VOID
Ext2ParseRegistryVolumeParams(
    IN  PUNICODE_STRING         Params,
    OUT PEXT2_VOLUME_PROPERTY2  Property
    )
{
    WCHAR       Codepage[CODEPAGE_MAXLEN];
    WCHAR       Prefix[HIDINGPAT_LEN];
    WCHAR       Suffix[HIDINGPAT_LEN];
    USHORT      MountPoint[4];
    UCHAR       DrvLetter[4];

    BOOLEAN     bWriteSupport = FALSE,
                bCheckBitmap = FALSE,
                bCodeName = FALSE,
                bMountPoint = FALSE;
    struct {
        PWCHAR   Name;      /* parameters name */
        PBOOLEAN bExist;    /* is it contained in params */
        USHORT   Length;    /* parameter value length */
        PWCHAR   uValue;    /* value buffer in unicode */
        PCHAR    aValue;    /* value buffer in ansi */
    } ParamPattern[] = {
        /* writing support */
        {READING_ONLY, &Property->bReadonly, 0, NULL, NULL},
        {WRITING_SUPPORT, &bWriteSupport, 0, NULL, NULL},
        {EXT3_FORCEWRITING, &Property->bExt3Writable, 0, NULL, NULL},

        /* need check bitmap */
        {CHECKING_BITMAP, &bCheckBitmap, 0, NULL, NULL},
        /* codepage */
        {CODEPAGE_NAME, &bCodeName, CODEPAGE_MAXLEN, 
         &Codepage[0], Property->Codepage},
        /* filter prefix and suffix */
        {HIDING_PREFIX, &Property->bHidingPrefix, HIDINGPAT_LEN,
         &Prefix[0], Property->sHidingPrefix},
        {HIDING_SUFFIX, &Property->bHidingSuffix, HIDINGPAT_LEN,
         &Suffix[0], Property->sHidingSuffix},
        {MOUNT_POINT, &bMountPoint, 4,
         &MountPoint[0], &DrvLetter[0]},

        /* end */
        {NULL, NULL, 0, NULL}
    };

    USHORT i, j, k;

    RtlZeroMemory(Codepage, CODEPAGE_MAXLEN);
    RtlZeroMemory(Prefix, HIDINGPAT_LEN);
    RtlZeroMemory(Suffix, HIDINGPAT_LEN);
    RtlZeroMemory(MountPoint, sizeof(USHORT) * 4);
    RtlZeroMemory(DrvLetter, sizeof(CHAR) * 4);

    RtlZeroMemory(Property, sizeof(EXT2_VOLUME_PROPERTY2));
    Property->Magic = EXT2_VOLUME_PROPERTY_MAGIC;
    Property->Command = APP_CMD_SET_PROPERTY2;

    for (i=0; ParamPattern[i].Name != NULL; i++) {

        UNICODE_STRING  Name1=*Params, Name2;
        RtlInitUnicodeString(&Name2, ParamPattern[i].Name);
        *ParamPattern[i].bExist = FALSE;

        for (j=0; j * sizeof(WCHAR) + Name2.Length <= Params->Length ; j++) {

            Name1.MaximumLength = Params->Length - j * sizeof(WCHAR);
            Name1.Length = Name2.Length;
            Name1.Buffer = &Params->Buffer[j];

            if (!RtlCompareUnicodeString(&Name1, &Name2, TRUE)) {
                if (j * sizeof(WCHAR) + Name2.Length == Params->Length ||
                    Name1.Buffer[Name2.Length/sizeof(WCHAR)] == L';' ||
                    Name1.Buffer[Name2.Length/sizeof(WCHAR)] == L','  ) {
                    *(ParamPattern[i].bExist) = TRUE;
                } else if ((j * 2 + Name2.Length < Params->Length + 2) ||
                           (Name1.Buffer[Name2.Length/sizeof(WCHAR)] == L'=' )) {
                    j += Name2.Length/sizeof(WCHAR) + 1;
                    k = 0;
                    while ( j + k < Params->Length/2 &&
                            k < ParamPattern[i].Length &&
                            Params->Buffer[j+k] != L';' &&
                            Params->Buffer[j+k] != L',' ) {
                        ParamPattern[i].uValue[k] = Params->Buffer[j + k++];
                    }
                    if (k) {
                        NTSTATUS status;
                        ANSI_STRING AnsiName;
                        AnsiName.Length = 0;
                        AnsiName.MaximumLength =ParamPattern[i].Length;
                        AnsiName.Buffer = ParamPattern[i].aValue;

                        Name2.Buffer = ParamPattern[i].uValue;
                        Name2.MaximumLength = Name2.Length = k * sizeof(WCHAR);
                        status = RtlUnicodeStringToAnsiString(
                                    &AnsiName, &Name2, FALSE);
                        if (NT_SUCCESS(status)) {
                            *(ParamPattern[i].bExist) = TRUE;
                        } else {
                            *ParamPattern[i].bExist = FALSE;
                        }
                    }
                }
                break;
            }
        }
    }

    if (bMountPoint) {
        Property->DrvLetter = DrvLetter[0];
        Property->DrvLetter |= 0x80;
    }
}

VOID
Ext2PerformRegistryVolumeParams(IN PEXT2_VCB Vcb)
{
    NTSTATUS        Status;
    UNICODE_STRING  VolumeParams;

    Status = Ext2QueryVolumeParams(Vcb, &VolumeParams);

    if (NT_SUCCESS(Status)) {

        /* set Vcb settings from registery */
        EXT2_VOLUME_PROPERTY2  Property;
        Ext2ParseRegistryVolumeParams(&VolumeParams, &Property);
        Ext2ProcessVolumeProperty(Vcb, &Property);

    } else {

        /* set Vcb settings from Ext2Global */
        if (IsFlagOn(Ext2Global->Flags, EXT2_SUPPORT_WRITING)) {
            if (Vcb->IsExt3fs) {
                if(IsFlagOn(Ext2Global->Flags, EXT3_FORCE_WRITING)) {
                    ClearLongFlag(Vcb->Flags, VCB_READ_ONLY);
                } else {
                    SetLongFlag(Vcb->Flags, VCB_READ_ONLY);
                }
            } else {
                ClearLongFlag(Vcb->Flags, VCB_READ_ONLY);
            }
        } else {
            SetLongFlag(Vcb->Flags, VCB_READ_ONLY);
        }

        /* set the default codepage */
        Vcb->Codepage.PageTable = Ext2Global->Codepage.PageTable;
        memcpy(Vcb->Codepage.AnsiName, Ext2Global->Codepage.AnsiName, CODEPAGE_MAXLEN);
        Vcb->Codepage.PageTable = Ext2Global->Codepage.PageTable;

        if (Vcb->bHidingPrefix = Ext2Global->bHidingPrefix) {
            RtlCopyMemory( Vcb->sHidingPrefix,
                           Ext2Global->sHidingPrefix,
                           HIDINGPAT_LEN);
        } else {
            RtlZeroMemory( Vcb->sHidingPrefix,
                           HIDINGPAT_LEN);
        }

        if (Vcb->bHidingSuffix = Ext2Global->bHidingSuffix) {
            RtlCopyMemory( Vcb->sHidingSuffix,
                           Ext2Global->sHidingSuffix,
                           HIDINGPAT_LEN);
        } else {
            RtlZeroMemory( Vcb->sHidingSuffix,
                           HIDINGPAT_LEN);
        }        
    }

    if (VolumeParams.Buffer) {
        ExFreePoolWithTag(VolumeParams.Buffer, EXT2_PARAM_MAGIC);
    }
}

NTSTATUS
Ext2InitializeLabel(
    IN PEXT2_VCB            Vcb,
    IN PEXT2_SUPER_BLOCK    Sb
    )
{
    NTSTATUS            status;

    USHORT              Length;
    UNICODE_STRING      Label;
    OEM_STRING          OemName;

    Label.MaximumLength = 16 * sizeof(WCHAR);
    Label.Length    = 0;
    Label.Buffer    = Vcb->Vpb->VolumeLabel;
    Vcb->Vpb->VolumeLabelLength = 0;
    RtlZeroMemory(Label.Buffer, Label.MaximumLength);

    Length = 16;
    while(  (Length > 0) &&
            ((Sb->s_volume_name[Length -1]  == 0x00) ||
             (Sb->s_volume_name[Length - 1] == 0x20)  )
         ) {
        Length--;
    }

    if (Length == 0) {
        return STATUS_SUCCESS;
    }

    OemName.Buffer =  Sb->s_volume_name;
    OemName.MaximumLength = 16;
    OemName.Length = Length;

    status = Ext2OEMToUnicode(Vcb, &Label, &OemName);
    if (NT_SUCCESS(status)) {
        Vcb->Vpb->VolumeLabelLength = Label.Length;
    }

    return status;
}

static __inline BOOLEAN Ext2IsNullUuid (__u8 * uuid)
{
    int i;
    for (i = 0; i < 16; i++) {
        if (uuid[i]) {
            break;
        }
    }

    return (i >= 16);
}

NTSTATUS
Ext2InitializeVcb( IN PEXT2_IRP_CONTEXT IrpContext, 
                   IN PEXT2_VCB         Vcb, 
                   IN PEXT2_SUPER_BLOCK sb,
                   IN PDEVICE_OBJECT TargetDevice,
                   IN PDEVICE_OBJECT VolumeDevice,
                   IN PVPB Vpb )
{
    NTSTATUS                    Status = STATUS_UNRECOGNIZED_VOLUME;
    ULONG                       IoctlSize;
    LONGLONG                    DiskSize;
    LONGLONG                    PartSize;
    UNICODE_STRING              RootNode;
    USHORT                      Buffer[2];
    ULONG                       ChangeCount = 0;
    CC_FILE_SIZES               FileSizes;

    BOOLEAN                     VcbResourceInitialized = FALSE;
    BOOLEAN                     NotifySyncInitialized = FALSE;
    BOOLEAN                     ExtentsInitialized = FALSE;
    BOOLEAN                     InodeLookasideInitialized = FALSE;

    __try {

        if (Vpb == NULL) {
            Status = STATUS_DEVICE_NOT_READY;
            __leave;
        }

        /* checking in/compat features */
        if (IsFlagOn(sb->s_feature_compat, EXT3_FEATURE_COMPAT_HAS_JOURNAL)) {
            Vcb->IsExt3fs = TRUE;
        }

        /* don't mount an journal device */
        if (IsFlagOn(sb->s_feature_incompat, EXT3_FEATURE_INCOMPAT_JOURNAL_DEV)) {
            __leave;
        }

        /* check block size */
        Vcb->BlockSize  = (EXT2_MIN_BLOCK_SIZE << sb->s_log_block_size);
        /* we cannot handle volume with block size bigger than 64k */
        if (Vcb->BlockSize > EXT2_MAX_USER_BLKSIZE) {
            Status = STATUS_UNRECOGNIZED_VOLUME;
            __leave;
        }

        /* initialize vcb header members ... */
        Vcb->Header.IsFastIoPossible = FastIoIsNotPossible;
        Vcb->Header.Resource = &(Vcb->MainResource);
        Vcb->Header.PagingIoResource = &(Vcb->PagingIoResource);
        Vcb->OpenFileHandleCount = 0;
        Vcb->ReferenceCount = 0;

        /* initialize eresources */
        ExInitializeResourceLite(&Vcb->MainResource);
        ExInitializeResourceLite(&Vcb->PagingIoResource);
        ExInitializeResourceLite(&Vcb->MetaLock);
        ExInitializeResourceLite(&Vcb->McbLock);
#ifndef _WIN2K_TARGET_
        ExInitializeFastMutex(&Vcb->Mutex);
        FsRtlSetupAdvancedHeader(&Vcb->Header,  &Vcb->Mutex);
#endif
        VcbResourceInitialized = TRUE;

        /* initialize Fcb list head */
        InitializeListHead(&Vcb->FcbList);
        KeInitializeSpinLock(&Vcb->FcbLock);

        /* initialize Mcb list head  */
        InitializeListHead(&(Vcb->McbList));

        /* initialize directory notify list */
        InitializeListHead(&Vcb->NotifyList);
        FsRtlNotifyInitializeSync(&Vcb->NotifySync);
        NotifySyncInitialized = TRUE;

        /* superblock checking */
        Vcb->SuperBlock = sb;

        /* initialize Vpb and Label */
        Vcb->DeviceObject = VolumeDevice;
        Vcb->TargetDeviceObject = TargetDevice;
        Vcb->Vpb = Vpb;
        Vcb->RealDevice = Vpb->RealDevice;
        Vpb->DeviceObject = VolumeDevice;

        /* set inode size */
        Vcb->InodeSize = (ULONG)sb->s_inode_size;
        if (INODE_SIZE == 0) {
            DbgBreak();
            Vcb->InodeSize = EXT2_GOOD_OLD_INODE_SIZE;
        }

        /* initialize inode lookaside list */
        ExInitializeNPagedLookasideList(&(Vcb->InodeLookasideList),
                                        NULL, NULL, 0, INODE_SIZE,
                                        'SNIE', 0);

        InodeLookasideInitialized = TRUE;

        /* initialize label in Vpb */
        Status = Ext2InitializeLabel(Vcb, sb);
        if (!NT_SUCCESS(Status)) {
            DbgBreak();
        }

        /* initialize UUID and serial number */
        if (Ext2IsNullUuid(sb->s_uuid)) {
            ExUuidCreate((UUID *)sb->s_uuid);
        }

        Vpb->SerialNumber = ((ULONG*)sb->s_uuid)[0] + 
                            ((ULONG*)sb->s_uuid)[1] +
                            ((ULONG*)sb->s_uuid)[2] +
                            ((ULONG*)sb->s_uuid)[3];

        /* check device characteristics flags */
        if (IsFlagOn(Vpb->RealDevice->Characteristics, FILE_REMOVABLE_MEDIA)) {
            SetLongFlag(Vcb->Flags, VCB_REMOVABLE_MEDIA);
        }

        if (IsFlagOn(Vpb->RealDevice->Characteristics, FILE_FLOPPY_DISKETTE)) {
            SetLongFlag(Vcb->Flags, VCB_FLOPPY_DISK);
        }

        if (IsFlagOn(Vpb->RealDevice->Characteristics, FILE_READ_ONLY_DEVICE)) {
            SetLongFlag(Vcb->Flags, VCB_WRITE_PROTECTED);
        }

        if (IsFlagOn(TargetDevice->Characteristics, FILE_READ_ONLY_DEVICE)) {
            SetLongFlag(Vcb->Flags, VCB_WRITE_PROTECTED);
        }

        /* verify device is writable ? */
        if (Ext2IsMediaWriteProtected(IrpContext, TargetDevice)) {
            SetFlag(Vcb->Flags, VCB_WRITE_PROTECTED);
        }

        /* query partition size and disk geometry parameters */
        DiskSize =  Vcb->DiskGeometry.Cylinders.QuadPart *
                    Vcb->DiskGeometry.TracksPerCylinder *
                    Vcb->DiskGeometry.SectorsPerTrack *
                    Vcb->DiskGeometry.BytesPerSector;

        IoctlSize = sizeof(PARTITION_INFORMATION);
        Status = Ext2DiskIoControl(
            TargetDevice,
            IOCTL_DISK_GET_PARTITION_INFO,
            NULL,
            0,
            &Vcb->PartitionInformation,
            &IoctlSize );
        if (NT_SUCCESS(Status)) {
            PartSize = Vcb->PartitionInformation.PartitionLength.QuadPart;
        } else {
            Vcb->PartitionInformation.StartingOffset.QuadPart = 0;
            Vcb->PartitionInformation.PartitionLength.QuadPart = DiskSize;
            PartSize = DiskSize;
            Status = STATUS_SUCCESS;
        }
        Vcb->Header.AllocationSize.QuadPart =
        Vcb->Header.FileSize.QuadPart = PartSize;

        Vcb->Header.ValidDataLength.QuadPart = 
            Vcb->Header.FileSize.QuadPart;

        /* verify count */
        IoctlSize = sizeof(ULONG);
        Status = Ext2DiskIoControl(
                    TargetDevice,
                    IOCTL_DISK_CHECK_VERIFY,
                    NULL,
                    0,
                    &ChangeCount,
                    &IoctlSize );

        if (!NT_SUCCESS(Status)) {
            __leave;
        }
        Vcb->ChangeCount = ChangeCount;

        /* query parameters from registry */
        Ext2PerformRegistryVolumeParams(Vcb);

        /* create the stream object for ext2 volume */
        Vcb->Volume = IoCreateStreamFileObject(NULL, Vcb->Vpb->RealDevice);
        if (!Vcb->Volume) {
            __leave;
        }

        /* initialize streaming object file */
        Vcb->Volume->SectionObjectPointer = &(Vcb->SectionObject);
        Vcb->Volume->ReadAccess = TRUE;
        Vcb->Volume->WriteAccess = TRUE;
        Vcb->Volume->DeleteAccess = TRUE;
        Vcb->Volume->FsContext = (PVOID) Vcb;
        Vcb->Volume->FsContext2 = NULL;
        Vcb->Volume->Vpb = Vcb->Vpb;

        FileSizes.AllocationSize.QuadPart =
        FileSizes.FileSize.QuadPart =
        FileSizes.ValidDataLength.QuadPart =
            Vcb->Header.AllocationSize.QuadPart;

        CcInitializeCacheMap( Vcb->Volume,
                              &FileSizes,
                              TRUE,
                              &(Ext2Global->CacheManagerNoOpCallbacks),
                              Vcb );
 
        /* initialize disk block LargetMcb and entry Mcb, 
           it will raise an expcetion if failed */
        __try {
            FsRtlInitializeLargeMcb(&(Vcb->Extents), PagedPool);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DbgBreak();
        }
        if (!NT_SUCCESS(Status)) {
            __leave;
        }
        ExtentsInitialized = TRUE;

        /* calculate maximum file bocks ... */
        {
            ULONG   dwData[EXT2_BLOCK_TYPES] = {EXT2_NDIR_BLOCKS, 1, 1, 1};
            ULONG   i;
            ULONGLONG MaxBlocks = 0x100000000;

            Vcb->SectorBits = Ext2Log2(SECTOR_SIZE);
            ASSERT(BLOCK_BITS == Ext2Log2(BLOCK_SIZE));

            Vcb->NumOfGroups = (sb->s_blocks_count - sb->s_first_data_block +
                sb->s_blocks_per_group - 1) / sb->s_blocks_per_group;

            MaxBlocks = (MaxBlocks >> (BLOCK_BITS - SECTOR_BITS));
            if (MaxBlocks > (ULONGLONG)0xFFFFFFF0) {
                MaxBlocks = (ULONGLONG)0xFFFFFFF0;
            }

            for (i = 0; i < EXT2_BLOCK_TYPES; i++) {

                dwData[i] = dwData[i] << ((BLOCK_BITS - 2) * i);
                if ( (((BLOCK_BITS - 2) * i) >= 32) || 
                     (ULONGLONG)Vcb->MaxInodeBlocks + dwData[i] > MaxBlocks) {
                    dwData[i] = (ULONG)(MaxBlocks - Vcb->MaxInodeBlocks);
                    Vcb->MaxInodeBlocks = (ULONG)MaxBlocks;
                } else {
                    Vcb->MaxInodeBlocks += dwData[i];
                }
                Vcb->NumBlocks[i] = dwData[i];
            }
        }

        /* load all gorup desc */
        if (!Ext2LoadGroup(Vcb)) {
            Status = STATUS_UNSUCCESSFUL;
            __leave;
        }

        /* recovery journal since it's ext3 */
        if (Vcb->IsExt3fs) {
            Ext2RecoverJournal(IrpContext, Vcb);
            if (IsFlagOn(Vcb->Flags, VCB_JOURNAL_RECOVER) || 
                IsFlagOn(sb->s_feature_incompat, EXT3_FEATURE_INCOMPAT_META_BG)) {
                SetLongFlag(Vcb->Flags, VCB_READ_ONLY);
            }
        }

        /* Now allocating the mcb for root ... */
        Buffer[0] = L'\\';
        Buffer[1] = 0;
        RootNode.Buffer = Buffer;
        RootNode.MaximumLength = RootNode.Length = 2;
        Vcb->McbTree = Ext2AllocateMcb(
                            Vcb, &RootNode, NULL,
                            FILE_ATTRIBUTE_DIRECTORY
                            );
        if (!Vcb->McbTree) {
            Status = STATUS_UNSUCCESSFUL;
            __leave;
        }

        /* load root inode */
        Vcb->McbTree->iNo = EXT2_ROOT_INO;
        if (!Ext2LoadInode(Vcb, EXT2_ROOT_INO, Vcb->McbTree->Inode)) {
            DbgBreak();
            Status = STATUS_CANT_WAIT;
            __leave;
        }

        /* initializeroot node */
        Vcb->McbTree->FileSize.LowPart = Vcb->McbTree->Inode->i_size;
        Vcb->McbTree->FileSize.HighPart = 0;

        /* check bitmap if user specifies it */
        if (IsFlagOn(Ext2Global->Flags, EXT2_CHECKING_BITMAP)) {
            Ext2CheckBitmapConsistency(IrpContext, Vcb);
        }

        /* get anything doen, then refer target device */
        ObReferenceObject(Vcb->TargetDeviceObject);
        SetLongFlag(Vcb->Flags, VCB_INITIALIZED);

    } __finally {

        if (!NT_SUCCESS(Status)) {

            if (InodeLookasideInitialized) {
                ExDeleteNPagedLookasideList(&(Vcb->InodeLookasideList));
            }

            if (ExtentsInitialized) {
                FsRtlUninitializeLargeMcb(&(Vcb->Extents));
            }

            if (Vcb->McbTree) {
                Ext2FreeMcb(Vcb, Vcb->McbTree);
            }

            if (Vcb->Volume) {
                if (Vcb->Volume->PrivateCacheMap) {
                    Ext2SyncUninitializeCacheMap(Vcb->Volume);
                }
                ObDereferenceObject(Vcb->Volume);
            }

            if (NotifySyncInitialized) {
                FsRtlNotifyUninitializeSync(&Vcb->NotifySync);
            }

            if (Vcb->GroupDesc) {
                ExFreePoolWithTag(Vcb->GroupDesc, EXT2_GD_MAGIC);
                Vcb->GroupDesc = NULL;
            }

            if (VcbResourceInitialized) {
                ExDeleteResourceLite(&Vcb->McbLock);
                ExDeleteResourceLite(&Vcb->MetaLock);
                ExDeleteResourceLite(&Vcb->MainResource);
                ExDeleteResourceLite(&Vcb->PagingIoResource);
            }
        }
    }

    return Status;
}


VOID
Ext2DestroyVcb (IN PEXT2_VCB Vcb )
{
    ASSERT(Vcb != NULL);
    
    ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
        (Vcb->Identifier.Size == sizeof(EXT2_VCB)));
    
    FsRtlNotifyUninitializeSync(&Vcb->NotifySync);

    if (Vcb->Volume) {

        if (IsFlagOn(Vcb->Volume->Flags, FO_FILE_MODIFIED)) {
            IO_STATUS_BLOCK    IoStatus;
            CcFlushCache(&(Vcb->SectionObject), NULL, 0, &IoStatus);
            ClearFlag(Vcb->Volume->Flags, FO_FILE_MODIFIED);
        }

        if (Vcb->Volume->PrivateCacheMap) {
            Ext2SyncUninitializeCacheMap(Vcb->Volume);
        }

        ObDereferenceObject(Vcb->Volume);
        Vcb->Volume = NULL;
    }

    DEBUG(DL_EXT, ("Ext2DestroyVcb ...\n"));
    Ext2ListExtents(&Vcb->Extents);
    FsRtlUninitializeLargeMcb(&(Vcb->Extents));

    Ext2CleanupAllMcbs(Vcb);

    if (Vcb->GroupDesc) {
        ExFreePoolWithTag(Vcb->GroupDesc, EXT2_GD_MAGIC);
        Vcb->GroupDesc = NULL;
    }
    
    if (Vcb->SuperBlock) {
        ExFreePoolWithTag(Vcb->SuperBlock, EXT2_SB_MAGIC);
        Vcb->SuperBlock = NULL;
    }

    ObDereferenceObject(Vcb->TargetDeviceObject);

    if (IsFlagOn(Vcb->Flags, VCB_NEW_VPB)) {
        ExFreePoolWithTag(Vcb->Vpb, TAG_VPB);
        DEC_MEM_COUNT(PS_VPB, Vcb->Vpb, sizeof(VPB));
    }

    ExDeleteNPagedLookasideList(&(Vcb->InodeLookasideList));

    ExDeleteResourceLite(&Vcb->McbLock);
    ExDeleteResourceLite(&Vcb->MetaLock);
    ExDeleteResourceLite(&Vcb->PagingIoResource);
    ExDeleteResourceLite(&Vcb->MainResource);

    IoDeleteDevice(Vcb->DeviceObject);
    DEC_MEM_COUNT(PS_VCB, Vcb->DeviceObject, sizeof(EXT2_VCB));
}


/* uninitialize cache map */

VOID
Ext2SyncUninitializeCacheMap (
    IN PFILE_OBJECT FileObject
    )
{
    CACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent;
    NTSTATUS WaitStatus;
    LARGE_INTEGER Ext2LargeZero = {0,0};


    KeInitializeEvent( &UninitializeCompleteEvent.Event,
                       SynchronizationEvent,
                       FALSE);

    CcUninitializeCacheMap( FileObject,
                            &Ext2LargeZero,
                            &UninitializeCompleteEvent );

    WaitStatus = KeWaitForSingleObject( &UninitializeCompleteEvent.Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

    ASSERT (NT_SUCCESS(WaitStatus));
}

/* Link Mcb to tail of Vcb->McbList queue */

VOID
Ext2LinkTailMcb(PEXT2_VCB Vcb, PEXT2_MCB Mcb)
{
    if (Mcb->iNo == EXT2_ROOT_INO) {
        return;
    }

    ExAcquireResourceExclusiveLite(&Vcb->McbLock, TRUE);

    if (IsFlagOn(Mcb->Flags, MCB_VCB_LINK)) {
        DEBUG(DL_RES, ( "Ext2LinkTailMcb: %wZ already linked.\n",
                    &Mcb->FullName));
    } else {
        InsertTailList(&Vcb->McbList, &Mcb->Link);
        SetLongFlag(Mcb->Flags, MCB_VCB_LINK);
        Ext2ReferXcb(&Vcb->NumOfMcb);
    }

    ExReleaseResourceLite(&Vcb->McbLock);
}

/* Link Mcb to head of Vcb->McbList queue */

VOID
Ext2LinkHeadMcb(PEXT2_VCB Vcb, PEXT2_MCB Mcb)
{
    if (Mcb->iNo == EXT2_ROOT_INO) {
        return;
    }

    ExAcquireResourceExclusiveLite(&Vcb->McbLock, TRUE);

    if (!IsFlagOn(Mcb->Flags, MCB_VCB_LINK)) {
        InsertHeadList(&Vcb->McbList, &Mcb->Link);
        SetLongFlag(Mcb->Flags, MCB_VCB_LINK);
        Ext2ReferXcb(&Vcb->NumOfMcb);
    } else {
        DEBUG(DL_RES, ( "Ext2LinkHeadMcb: %wZ already linked.\n",
                              &Mcb->FullName));
    }
    ExReleaseResourceLite(&Vcb->McbLock);
}

/* Unlink Mcb from Vcb->McbList queue */

VOID
Ext2UnlinkMcb(PEXT2_VCB Vcb, PEXT2_MCB Mcb)
{
    if (Mcb->iNo == EXT2_ROOT_INO) {
        return;
    }

    ExAcquireResourceExclusiveLite(&Vcb->McbLock, TRUE);

    if (IsFlagOn(Mcb->Flags, MCB_VCB_LINK)) {
        RemoveEntryList(&(Mcb->Link));
        ClearLongFlag(Mcb->Flags, MCB_VCB_LINK);
        Ext2DerefXcb(&Vcb->NumOfMcb);
    } else {
        DEBUG(DL_RES, ( "Ext2UnlinkMcb: %wZ already unlinked.\n",
                              &Mcb->FullName));
    }
    ExReleaseResourceLite(&Vcb->McbLock);
}

/* get the first Mcb record in Vcb->McbList */

PEXT2_MCB
Ext2FirstUnusedMcb(PEXT2_VCB Vcb, BOOLEAN Wait, ULONG Number)
{
    PEXT2_MCB   Head = NULL;
    PEXT2_MCB   Tail = NULL;
    PEXT2_MCB   Mcb = NULL;
    PLIST_ENTRY List = NULL;
    ULONG       i = 0;

    if (!ExAcquireResourceExclusiveLite(&Vcb->McbLock, Wait)) {
        return NULL;
    }

    while (Number--) {

        if (!IsListEmpty(&Vcb->McbList)) {

            while (i++ < Vcb->NumOfMcb) {

                List = RemoveHeadList(&Vcb->McbList);
                Mcb = CONTAINING_RECORD(List, EXT2_MCB, Link);
                ASSERT(IsFlagOn(Mcb->Flags, MCB_VCB_LINK));

                if (Mcb->Fcb == NULL && !IsMcbRoot(Mcb) && 
                    (IsMcbSymlink(Mcb) || Mcb->Child == NULL) &&
                    Mcb->Refercount == 0 ) {

                    Ext2RemoveMcb(Vcb, Mcb);
                    ClearLongFlag(Mcb->Flags, MCB_VCB_LINK);
                    Ext2DerefXcb(&Vcb->NumOfMcb);

                    /* attach all Mcb into a chain*/
                    if (Head) {
                        ASSERT(Tail != NULL);
                        Tail->Next = Mcb;
                        Tail = Mcb;
                    } else {
                        Head = Tail = Mcb;
                    }
                    Tail->Next = NULL;

                } else {

                    InsertTailList(&Vcb->McbList, &Mcb->Link);
                    Mcb = NULL;
                }
            }
        }
    }
    ExReleaseResourceLite(&Vcb->McbLock);

    return Head;
}


/* Reaper thread to release unused Mcb blocks */
VOID
Ext2ReaperThread(
    PVOID   Context
    )
{
    BOOLEAN         GlobalAcquired = FALSE;

    BOOLEAN         DidNothing = TRUE;
    BOOLEAN         LastState  = TRUE;
    BOOLEAN         WaitLock;

    PLIST_ENTRY     List = NULL;
    LARGE_INTEGER   Timeout;

    PEXT2_VCB       Vcb = NULL;
    PEXT2_MCB       Mcb  = NULL;

    ULONG           i, NumOfMcbs;
    
    __try {

        /* wake up DirverEntry */
        KeSetEvent(&Ext2Global->Reaper.Engine, 0, FALSE);

        /* now process looping */
        while (TRUE) {

            WaitLock = FALSE;

            /* calculate how long we need wait */
            if (Ext2Global->PerfStat.Current.Mcb > (((ULONG)Ext2Global->MaxDepth) * 128)) {
                Timeout.QuadPart = (LONGLONG)-1000*1000; /* 0.1 second */
                NumOfMcbs = Ext2Global->MaxDepth * 4;
                WaitLock = TRUE;
            } else if (Ext2Global->PerfStat.Current.Mcb > (((ULONG)Ext2Global->MaxDepth) * 32)) {
                Timeout.QuadPart = (LONGLONG)-1000*1000*5; /* 0.5 second */
                NumOfMcbs = Ext2Global->MaxDepth * 2;
                WaitLock = TRUE;
            } else if (Ext2Global->PerfStat.Current.Mcb > (((ULONG)Ext2Global->MaxDepth) * 8)) {
                Timeout.QuadPart = (LONGLONG)-1000*1000*10; /* 1 second */
                NumOfMcbs = Ext2Global->MaxDepth;
            } else if (Ext2Global->PerfStat.Current.Mcb > (((ULONG)Ext2Global->MaxDepth) * 2)) {
                Timeout.QuadPart = (LONGLONG)-2*1000*1000*10; /* 2 second */
                NumOfMcbs = Ext2Global->MaxDepth / 4;
            } else if (Ext2Global->PerfStat.Current.Mcb > (ULONG)Ext2Global->MaxDepth) {
                Timeout.QuadPart = (LONGLONG)-10*1000*1000*10; /* 10 seconds */
                NumOfMcbs = Ext2Global->MaxDepth / 8;
            } else if (DidNothing) {
                Timeout.QuadPart = (LONGLONG)-3*60*1000*1000*10; /* 3 mins */
                if (LastState) {
                    Timeout.QuadPart *= 2;
                }
                NumOfMcbs = Ext2Global->MaxDepth / 16;
            } else {
                Timeout.QuadPart = (LONGLONG)-15*1000*1000*10; /* 15 seconds */
                if (LastState) {
                    Timeout.QuadPart *= 2;
                }
                NumOfMcbs = Ext2Global->MaxDepth / 32;
            }

            LastState = DidNothing;

            /* wait until it is waken or it times out */
            KeWaitForSingleObject(
                &(Ext2Global->Reaper.Wait),
                Executive,
                KernelMode,
                FALSE,
                &Timeout
                );

            DidNothing = TRUE;

            /* acquire global exclusive lock */
            if (!ExAcquireResourceSharedLite(&Ext2Global->Resource, WaitLock)) {
                continue;
            }
            GlobalAcquired = TRUE;

            /* search all Vcb to get unused resources freed to system */
            for (List = Ext2Global->VcbList.Flink;
                 List != &(Ext2Global->VcbList);
                 List = List->Flink ) {

                Vcb = CONTAINING_RECORD(List, EXT2_VCB, Next);

                Mcb = Ext2FirstUnusedMcb(Vcb, WaitLock, NumOfMcbs);
                while (Mcb) {
                    PEXT2_MCB   Next = Mcb->Next;
                    DEBUG(DL_RES, ( "Ext2ReaperThread: releasing Mcb: %wZ"
                                         " Total: %xh\n", &Mcb->FullName,
                                         Ext2Global->PerfStat.Current.Mcb));
                    Ext2FreeMcb(Vcb, Mcb);
                    Mcb = Next;
                    LastState = DidNothing = FALSE;
                }
            }

            if (GlobalAcquired) {
                ExReleaseResourceLite(&Ext2Global->Resource);
                GlobalAcquired = FALSE;
            }
        }

    } __finally {

        if (GlobalAcquired) {
            ExReleaseResourceLite(&Ext2Global->Resource);
        }
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}


NTSTATUS
Ext2StartReaperThread()
{
    NTSTATUS status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES  oa;
    HANDLE   handle = 0;

    /* initialize wait event */
    KeInitializeEvent(
        &Ext2Global->Reaper.Wait,
        SynchronizationEvent, FALSE
        );

    /* initialize oa */
    InitializeObjectAttributes(
        &oa,
        NULL,
        OBJ_CASE_INSENSITIVE |
        OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    /* start a new system thread */
    status = PsCreateSystemThread(
        &handle,
        0,
        &oa,
        NULL,
        NULL,
        Ext2ReaperThread,
        NULL
        );

    if (NT_SUCCESS(status)) {
        ZwClose(handle);
    }

    return status;
}
