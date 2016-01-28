/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             memory.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2AllocateFcb)
#pragma alloc_text(PAGE, Ext2FreeFcb)
#pragma alloc_text(PAGE, Ext2AllocateInode)
#pragma alloc_text(PAGE, Ext2DestroyInode)
#pragma alloc_text(PAGE, Ext2CheckBitmapConsistency)
#pragma alloc_text(PAGE, Ext2CheckSetBlock)
#pragma alloc_text(PAGE, Ext2InitializeVcb)
#pragma alloc_text(PAGE, Ext2FreeCcb)
#pragma alloc_text(PAGE, Ext2AllocateCcb)
#pragma alloc_text(PAGE, Ext2TearDownStream)
#pragma alloc_text(PAGE, Ext2DestroyVcb)
#pragma alloc_text(PAGE, Ext2SyncUninitializeCacheMap)
#pragma alloc_text(PAGE, Ext2ReaperThread)
#pragma alloc_text(PAGE, Ext2StartReaperThread)
#endif

PEXT2_IRP_CONTEXT
Ext2AllocateIrpContext (IN PDEVICE_OBJECT   DeviceObject,
                        IN PIRP             Irp )
{
    PIO_STACK_LOCATION   irpSp;
    PEXT2_IRP_CONTEXT    IrpContext;

    ASSERT(DeviceObject != NULL);
    ASSERT(Irp != NULL);

    irpSp = IoGetCurrentIrpStackLocation(Irp);

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
    IrpContext->MajorFunction = irpSp->MajorFunction;
    IrpContext->MinorFunction = irpSp->MinorFunction;
    IrpContext->DeviceObject = DeviceObject;
    IrpContext->FileObject = irpSp->FileObject;
    if (NULL != IrpContext->FileObject) {
        IrpContext->Fcb = (PEXT2_FCB)IrpContext->FileObject->FsContext;
        IrpContext->Ccb = (PEXT2_CCB)IrpContext->FileObject->FsContext2;
    }

    if (IrpContext->FileObject != NULL) {
        IrpContext->RealDevice = IrpContext->FileObject->DeviceObject;
    } else if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) {
        if (irpSp->Parameters.MountVolume.Vpb) {
            IrpContext->RealDevice = irpSp->Parameters.MountVolume.Vpb->RealDevice;
        }
    }

    if (IsFlagOn(irpSp->Flags, SL_WRITE_THROUGH)) {
        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH);
    }

    if (IsFlagOn(irpSp->Flags, SL_OVERRIDE_VERIFY_VOLUME)) {
        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_VERIFY_READ);
    }

    if (IrpContext->MajorFunction == IRP_MJ_CLEANUP ||
        IrpContext->MajorFunction == IRP_MJ_CLOSE ||
        IrpContext->MajorFunction == IRP_MJ_SHUTDOWN ||
        IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL ||
        IrpContext->MajorFunction == IRP_MJ_PNP ) {

        if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL ||
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

    ASSERT(!IsMcbSymLink(Mcb));

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
    Fcb->Inode = &Mcb->Inode;

    ASSERT(Mcb->Fcb == NULL);
    Ext2ReferMcb(Mcb);
    Fcb->Mcb = Mcb;
    Mcb->Fcb = Fcb;

    DEBUG(DL_RES, ("Ext2AllocateFcb: Fcb %p created: %wZ.\n",
                   Fcb, &Fcb->Mcb->FullName));

    RtlZeroMemory(&Fcb->Header, sizeof(FSRTL_COMMON_FCB_HEADER));
    Fcb->Header.NodeTypeCode = (USHORT) EXT2FCB;
    Fcb->Header.NodeByteSize = sizeof(EXT2_FCB);
    Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;
    Fcb->Header.Resource = &(Fcb->MainResource);
    Fcb->Header.PagingIoResource = &(Fcb->PagingIoResource);

    Fcb->Header.FileSize.QuadPart = Mcb->Inode.i_size;
    Fcb->Header.ValidDataLength.QuadPart = Mcb->Inode.i_size;
    Fcb->Header.AllocationSize.QuadPart = CEILING_ALIGNED(ULONGLONG,
                                          Fcb->Header.FileSize.QuadPart, (ULONGLONG)Vcb->BlockSize);

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
    PEXT2_VCB   Vcb = Fcb->Vcb;

    ASSERT((Fcb != NULL) && (Fcb->Identifier.Type == EXT2FCB) &&
           (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
    ASSERT((Fcb->Mcb->Identifier.Type == EXT2MCB) &&
           (Fcb->Mcb->Identifier.Size == sizeof(EXT2_MCB)));

#ifndef _WIN2K_TARGET_
    FsRtlTeardownPerStreamContexts(&Fcb->Header);
#endif

    if ((Fcb->Mcb->Identifier.Type == EXT2MCB) &&
            (Fcb->Mcb->Identifier.Size == sizeof(EXT2_MCB))) {

        ASSERT (Fcb->Mcb->Fcb == Fcb);
        if (IsMcbSpecialFile(Fcb->Mcb) || IsFileDeleted(Fcb->Mcb)) {

            ASSERT(!IsRoot(Fcb));
            Ext2RemoveMcb(Fcb->Vcb, Fcb->Mcb);
            Fcb->Mcb->Fcb = NULL;

            Ext2UnlinkMcb(Vcb, Fcb->Mcb);
            Ext2DerefMcb(Fcb->Mcb);
            Ext2LinkHeadMcb(Vcb, Fcb->Mcb);

        } else {

            Fcb->Mcb->Fcb = NULL;
            Ext2DerefMcb(Fcb->Mcb);
        }

    } else {
        DbgBreak();
    }

    Ext2RemoveFcb(Fcb->Vcb, Fcb);

    FsRtlUninitializeFileLock(&Fcb->FileLockAnchor);
    FsRtlUninitializeOplock(&Fcb->Oplock);
    ExDeleteResourceLite(&Fcb->MainResource);
    ExDeleteResourceLite(&Fcb->PagingIoResource);

    DEBUG(DL_RES, ( "Ext2FreeFcb: Fcb (%p) is being released: %wZ.\n",
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

    Ccb = (PEXT2_CCB) (ExAllocateFromNPagedLookasideList(
                           &(Ext2Global->Ext2CcbLookasideList)));
    if (!Ccb) {
        return NULL;
    }

    DEBUG(DL_RES, ( "ExtAllocateCcb: Ccb created: %ph.\n", Ccb));

    RtlZeroMemory(Ccb, sizeof(EXT2_CCB));

    Ccb->Identifier.Type = EXT2CCB;
    Ccb->Identifier.Size = sizeof(EXT2_CCB);

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
Ext2FreeCcb (IN PEXT2_VCB Vcb, IN PEXT2_CCB Ccb)
{
    ASSERT(Ccb != NULL);

    ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
           (Ccb->Identifier.Size == sizeof(EXT2_CCB)));

    DEBUG(DL_RES, ( "Ext2FreeCcb: Ccb = %ph.\n", Ccb));

    if (Ccb->SymLink) {
        DEBUG(DL_INF, ( "Ext2FreeCcb: Ccb SymLink: %wZ.\n",
                        &Ccb->SymLink->FullName));
        if (IsFileDeleted(Ccb->SymLink->Target)) {
            Ext2UnlinkMcb(Vcb, Ccb->SymLink);
            Ext2DerefMcb(Ccb->SymLink);
            Ext2LinkHeadMcb(Vcb, Ccb->SymLink);
        } else {
            Ext2DerefMcb(Ccb->SymLink);
        }
    }

    if (Ccb->DirectorySearchPattern.Buffer != NULL) {
        DEC_MEM_COUNT(PS_DIR_PATTERN, Ccb->DirectorySearchPattern.Buffer,
                      Ccb->DirectorySearchPattern.MaximumLength );
        Ext2FreePool(Ccb->DirectorySearchPattern.Buffer, EXT2_DIRSP_MAGIC);
    }

    ExFreeToNPagedLookasideList(&(Ext2Global->Ext2CcbLookasideList), Ccb);
    DEC_MEM_COUNT(PS_CCB, Ccb, sizeof(EXT2_CCB));
}

PEXT2_INODE
Ext2AllocateInode (PEXT2_VCB  Vcb)
{
    PVOID inode = NULL;

    inode = ExAllocateFromNPagedLookasideList(
                &(Vcb->InodeLookasideList));
    if (!inode) {
        return NULL;
    }

    RtlZeroMemory(inode, INODE_SIZE);

    DEBUG(DL_INF, ("ExtAllocateInode: Inode created: %ph.\n", inode));
    INC_MEM_COUNT(PS_EXT2_INODE, inode, INODE_SIZE);

    return inode;
}

VOID
Ext2DestroyInode (IN PEXT2_VCB Vcb, IN PEXT2_INODE inode)
{
    ASSERT(inode != NULL);

    DEBUG(DL_INF, ("Ext2FreeInode: Inode = %ph.\n", inode));

    ExFreeToNPagedLookasideList(&(Vcb->InodeLookasideList), inode);
    DEC_MEM_COUNT(PS_EXT2_INODE, inode, INODE_SIZE);
}

struct dentry * Ext2AllocateEntry()
{
    struct dentry *de;

    de = (struct dentry *)ExAllocateFromNPagedLookasideList(
             &(Ext2Global->Ext2DentryLookasideList));
    if (!de) {
        return NULL;
    }

    RtlZeroMemory(de, sizeof(struct dentry));
    INC_MEM_COUNT(PS_DENTRY, de, sizeof(struct dentry));

    return de;
}

VOID Ext2FreeEntry (IN struct dentry *de)
{
    ASSERT(de != NULL);

    if (de->d_name.name)
        ExFreePool(de->d_name.name);

    ExFreeToNPagedLookasideList(&(Ext2Global->Ext2DentryLookasideList), de);
    DEC_MEM_COUNT(PS_DENTRY, de, sizeof(struct dentry));
}


struct dentry *Ext2BuildEntry(PEXT2_VCB Vcb, PEXT2_MCB Dcb, PUNICODE_STRING FileName)
{
    OEM_STRING      Oem = { 0 };
    struct dentry  *de = NULL;
    NTSTATUS        Status = STATUS_INSUFFICIENT_RESOURCES;

    _SEH2_TRY {

        de = Ext2AllocateEntry();
        if (!de) {
            DEBUG(DL_ERR, ("Ext2BuildEntry: failed to allocate dentry.\n"));
            _SEH2_LEAVE;
        }
        de->d_sb = &Vcb->sb;
        if (Dcb)
            de->d_parent = Dcb->de;

        Oem.MaximumLength = (USHORT)Ext2UnicodeToOEMSize(Vcb, FileName) + 1;
        Oem.Buffer = ExAllocatePool(PagedPool, Oem.MaximumLength);
        if (!Oem.Buffer) {
            DEBUG(DL_ERR, ( "Ex2BuildEntry: failed to allocate OEM name.\n"));
            _SEH2_LEAVE;
        }
        de->d_name.name = Oem.Buffer;
        RtlZeroMemory(Oem.Buffer, Oem.MaximumLength);
        Status = Ext2UnicodeToOEM(Vcb, &Oem, FileName);
        if (!NT_SUCCESS(Status)) {
            DEBUG(DL_CP, ("Ext2BuildEntry: failed to convert %S to OEM.\n", FileName->Buffer));
            _SEH2_LEAVE;
        }
        de->d_name.len  = Oem.Length;

    } _SEH2_FINALLY {

        if (!NT_SUCCESS(Status)) {
            if (de)
                Ext2FreeEntry(de);
        }
    } _SEH2_END;

    return de;
}

PEXT2_EXTENT
Ext2AllocateExtent ()
{
    PEXT2_EXTENT Extent;

    Extent = (PEXT2_EXTENT)ExAllocateFromNPagedLookasideList(
                 &(Ext2Global->Ext2ExtLookasideList));
    if (!Extent) {
        return NULL;
    }

    RtlZeroMemory(Extent, sizeof(EXT2_EXTENT));
    INC_MEM_COUNT(PS_EXTENT, Extent, sizeof(EXT2_EXTENT));

    return Extent;
}

VOID
Ext2FreeExtent (IN PEXT2_EXTENT Extent)
{
    ASSERT(Extent != NULL);
    ExFreeToNPagedLookasideList(&(Ext2Global->Ext2ExtLookasideList), Extent);
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
#ifndef __REACTOS__
    ULONG        count = 0;
#endif
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
    if (FsRtlNumberOfRunsInLargeMcb(Extents) != 0) {

        LONGLONG            DirtyVba;
        LONGLONG            DirtyLba;
        LONGLONG            DirtyLength;
        int                 i, n = 0;

        for (i = 0; FsRtlGetNextLargeMcbEntry(
                    Extents, i, &DirtyVba,
                    &DirtyLba, &DirtyLength); i++)  {
            if (DirtyVba > 0 && DirtyLba != -1) {
                DEBUG(DL_EXT, ("Vba:%I64xh Lba:%I64xh Len:%I64xh.\n", DirtyVba, DirtyLba, DirtyLength));
                n++;
            }
        }

        return n ? TRUE : FALSE;
    }

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
    _SEH2_TRY {
        FsRtlTruncateLargeMcb(Zone, (LONGLONG)0);
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        DbgBreak();
    } _SEH2_END;
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
    BOOLEAN     rc = FALSE;

    Offset = Vbn & (~(Vcb->IoUnitSize - 1));
    Length = (Vbn - Offset + Length + Vcb->IoUnitSize - 1) &
             ~(Vcb->IoUnitSize - 1);

    ASSERT ((Offset & (Vcb->IoUnitSize - 1)) == 0);
    ASSERT ((Length & (Vcb->IoUnitSize - 1)) == 0);

    Offset = (Offset >> Vcb->IoUnitBits) + 1;
    Length = (Length >> Vcb->IoUnitBits);

Again:

    _SEH2_TRY {
        rc = FsRtlAddLargeMcbEntry(
                &Vcb->Extents,
                Offset,
                Offset,
                Length
                );
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        DbgBreak();
        rc = FALSE;
    } _SEH2_END;

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
    BOOLEAN     rc = TRUE;

    Offset =  Vbn & (~(Vcb->IoUnitSize - 1));
    Length = (Length + Vbn - Offset + Vcb->IoUnitSize - 1) & (~(Vcb->IoUnitSize - 1));

    ASSERT ((Offset & (Vcb->IoUnitSize - 1)) == 0);
    ASSERT ((Length & (Vcb->IoUnitSize - 1)) == 0);

    Offset = (Offset >> Vcb->IoUnitBits) + 1;
    Length = (Length >> Vcb->IoUnitBits);

Again:

    _SEH2_TRY {
        FsRtlRemoveLargeMcbEntry(
            &Vcb->Extents,
            Offset,
            Length
        );
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        DbgBreak();
        rc = FALSE;
    } _SEH2_END;

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

    offset = Vbn & (~(Vcb->IoUnitSize - 1));
    ASSERT ((offset & (Vcb->IoUnitSize - 1)) == 0);
    offset = (offset >> Vcb->IoUnitBits) + 1;

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
            (*Lbn) = (((*Lbn) - 1) << Vcb->IoUnitBits);
            (*Lbn) += ((Vbn) & (Vcb->IoUnitSize - 1));
        }

        if (Length && *Length) {
            (*Length) <<= Vcb->IoUnitBits;
            (*Length)  -= ((Vbn) & (Vcb->IoUnitSize - 1));
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

    _SEH2_TRY {

        rc = FsRtlAddLargeMcbEntry(
            &Mcb->Extents,
            Vbn,
            Lbn,
            Length
        );

    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {

        DbgBreak();
        rc = FALSE;
    } _SEH2_END;

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

    _SEH2_TRY {
        FsRtlRemoveLargeMcbEntry(
            &Mcb->Extents,
            Vbn,
            Length
        );
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        DbgBreak();
        rc = FALSE;
    } _SEH2_END;

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
Ext2AddMcbMetaExts (
    IN PEXT2_VCB Vcb,
    IN PEXT2_MCB Mcb,
    IN ULONG     Block,
    IN ULONG     Length
)
{
    ULONG       TriedTimes = 0;
    LONGLONG    Lbn = Block + 1;
    BOOLEAN     rc = TRUE;

Again:

    _SEH2_TRY {

        rc = FsRtlAddLargeMcbEntry(
                &Mcb->MetaExts,
                Lbn,
                Lbn,
                Length
             );

    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {

        DbgBreak();
        rc = FALSE;
    } _SEH2_END;

    if (!rc && ++TriedTimes < 10) {
        Ext2Sleep(TriedTimes * 100);
        goto Again;
    }

    DEBUG(DL_EXT, ("Ext2AddMcbMetaExts: Block: %xh-%xh rc=%d Runs=%u\n", Block,
                   Length, rc, FsRtlNumberOfRunsInLargeMcb(&Mcb->MetaExts)));

    if (rc) {
        Ext2CheckExtent(&Mcb->MetaExts, Lbn, Lbn, Length, TRUE);
    }

    return rc;
}

BOOLEAN
Ext2RemoveMcbMetaExts (
    IN PEXT2_VCB Vcb,
    IN PEXT2_MCB Mcb,
    IN ULONG     Block,
    IN ULONG     Length
)
{
    ULONG       TriedTimes = 0;
    LONGLONG    Lbn = Block + 1;
    BOOLEAN     rc = TRUE;

Again:

    _SEH2_TRY {

        FsRtlRemoveLargeMcbEntry(
            &Mcb->MetaExts,
            Lbn,
            Length
        );

    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        DbgBreak();
        rc = FALSE;
    } _SEH2_END;

    if (!rc && ++TriedTimes < 10) {
        Ext2Sleep(TriedTimes * 100);
        goto Again;
    }

    DEBUG(DL_EXT, ("Ext2RemoveMcbMetaExts: Block: %xh-%xhh Runs=%u\n", Block,
                    Length, FsRtlNumberOfRunsInLargeMcb(&Mcb->MetaExts)));
    if (rc) {
        Ext2CheckExtent(&Mcb->MetaExts, Lbn, 0, Length, FALSE);
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

    Ext2ClearAllExtents(&Mcb->Extents);
    Ext2ClearAllExtents(&Mcb->MetaExts);

    ASSERT(Mcb != NULL);
    End = (ULONG)((Mcb->Inode.i_size + BLOCK_SIZE - 1) >> BLOCK_BITS);

    while (Start < End) {

        Block = Mapped = 0;

        /* mapping file offset to ext2 block */
        if (INODE_HAS_EXTENT(&Mcb->Inode)) {
            Status = Ext2MapExtent(
                         IrpContext,
                         Vcb,
                         Mcb,
                         Start,
                         FALSE,
                         &Block,
                         &Mapped
                     );
        } else {
            Status = Ext2MapIndirect(
                         IrpContext,
                         Vcb,
                         Mcb,
                         Start,
                         FALSE,
                         &Block,
                         &Mapped
                     );
        }

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
                ClearFlag(Mcb->Flags, MCB_ZONE_INITED);
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

    /* set mcb zone as initialized */
    SetLongFlag(Mcb->Flags, MCB_ZONE_INITED);

errorout:

    if (!IsZoneInited(Mcb)) {
        Ext2ClearAllExtents(&Mcb->Extents);
        Ext2ClearAllExtents(&Mcb->MetaExts);
    }

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

    if (!IsZoneInited(Mcb)) {
        Status = Ext2InitializeZone(IrpContext, Vcb, Mcb);
        if (!NT_SUCCESS(Status)) {
            DbgBreak();
        }
    }

    if ((IrpContext && IrpContext->Irp) &&
            ((IrpContext->Irp->Flags & IRP_NOCACHE) ||
             (IrpContext->Irp->Flags & IRP_PAGING_IO))) {
        Size = (Size + SECTOR_SIZE - 1) & (~(SECTOR_SIZE - 1));
    }

    Start = (ULONG)(Offset >> BLOCK_BITS);
    End = (ULONG)((Size + Offset + BLOCK_SIZE - 1) >> BLOCK_BITS);

    if (End > (ULONG)((Mcb->Inode.i_size + BLOCK_SIZE - 1) >> BLOCK_BITS) ) {
        End = (ULONG)((Mcb->Inode.i_size + BLOCK_SIZE - 1) >> BLOCK_BITS);
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
                /* we likely get a sparse file here */
                Mapped = 1;
                Block = 0;
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

            /* skip wrong blocks, in case wrongly treating symlink
               target names as blocks, silly  */
            if (Block >= TOTAL_BLOCKS) {
                Block = 0;
            }

            /* add new allocated blocks to Mcb zone */
            if (IsZoneInited(Mcb) && Block) {
                if (!Ext2AddBlockExtent(Vcb, Mcb, Start, Block, Mapped)) {
                    DbgBreak();
                    ClearFlag(Mcb->Flags, MCB_ZONE_INITED);
                    Ext2ClearAllExtents(&Mcb->Extents);
                }
            }
        }

        /* calculate i/o extent */
        Lba = ((LONGLONG)Block << BLOCK_BITS) + Offset - ((LONGLONG)Start << BLOCK_BITS);
        Length = (ULONG)(((LONGLONG)(Start + Mapped) << BLOCK_BITS) - Offset);
        if (Length > Size) {
            Length = Size;
        }

        Start += Mapped;
        Offset = (ULONGLONG)Start << BLOCK_BITS;

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
        Ext2FreePool(Target->Buffer, EXT2_FNAME_MAGIC);
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

    Target->Buffer = Ext2AllocatePool(
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
    Mcb = (PEXT2_MCB) (ExAllocateFromNPagedLookasideList(
                           &(Ext2Global->Ext2McbLookasideList)));

    if (Mcb == NULL) {
        return NULL;
    }

    /* initialize Mcb header */
    RtlZeroMemory(Mcb, sizeof(EXT2_MCB));
    Mcb->Identifier.Type = EXT2MCB;
    Mcb->Identifier.Size = sizeof(EXT2_MCB);
    Mcb->FileAttr = FileAttr;

    Mcb->Inode.i_priv = (PVOID)Mcb;
    Mcb->Inode.i_sb = &Vcb->sb;

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
    _SEH2_TRY {
        FsRtlInitializeLargeMcb(&(Mcb->Extents), NonPagedPool);
        FsRtlInitializeLargeMcb(&(Mcb->MetaExts), NonPagedPool);
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DbgBreak();
    } _SEH2_END;

    if (!NT_SUCCESS(Status)) {
        goto errorout;
    }

    INC_MEM_COUNT(PS_MCB, Mcb, sizeof(EXT2_MCB));
    DEBUG(DL_INF, ( "Ext2AllocateMcb: Mcb %wZ created.\n", &Mcb->FullName));

    return Mcb;

errorout:

    if (Mcb) {

        if (Mcb->ShortName.Buffer) {
            DEC_MEM_COUNT(PS_MCB_NAME, Mcb->ShortName.Buffer,
                          Mcb->ShortName.MaximumLength);
            Ext2FreePool(Mcb->ShortName.Buffer, EXT2_FNAME_MAGIC);
        }

        if (Mcb->FullName.Buffer) {
            DEC_MEM_COUNT(PS_MCB_NAME, Mcb->FullName.Buffer,
                          Mcb->FullName.MaximumLength);
            Ext2FreePool(Mcb->FullName.Buffer, EXT2_FNAME_MAGIC);
        }

        ExFreeToNPagedLookasideList(&(Ext2Global->Ext2McbLookasideList), Mcb);
    }

    return NULL;
}

VOID
Ext2FreeMcb (IN PEXT2_VCB Vcb, IN PEXT2_MCB Mcb)
{
#ifndef __REACTOS__
    PEXT2_MCB   Parent = Mcb->Parent;
#endif
    ASSERT(Mcb != NULL);

    ASSERT((Mcb->Identifier.Type == EXT2MCB) &&
           (Mcb->Identifier.Size == sizeof(EXT2_MCB)));

    if ((Mcb->Identifier.Type != EXT2MCB) ||
            (Mcb->Identifier.Size != sizeof(EXT2_MCB))) {
        return;
    }

    DEBUG(DL_INF, ( "Ext2FreeMcb: Mcb %wZ will be freed.\n", &Mcb->FullName));

    if (IsMcbSymLink(Mcb) && Mcb->Target) {
        Ext2DerefMcb(Mcb->Target);
    }

    if (FsRtlNumberOfRunsInLargeMcb(&Mcb->Extents)) {
        DEBUG(DL_EXT, ("List data extents for: %wZ\n", &Mcb->FullName));
        Ext2ListExtents(&Mcb->Extents);
    }
    FsRtlUninitializeLargeMcb(&(Mcb->Extents));
    if (FsRtlNumberOfRunsInLargeMcb(&Mcb->MetaExts)) {
        DEBUG(DL_EXT, ("List meta extents for: %wZ\n", &Mcb->FullName));
        Ext2ListExtents(&Mcb->MetaExts);
    }
    FsRtlUninitializeLargeMcb(&(Mcb->MetaExts));
    ClearLongFlag(Mcb->Flags, MCB_ZONE_INITED);

    if (Mcb->ShortName.Buffer) {
        DEC_MEM_COUNT(PS_MCB_NAME, Mcb->ShortName.Buffer,
                      Mcb->ShortName.MaximumLength);
        Ext2FreePool(Mcb->ShortName.Buffer, EXT2_FNAME_MAGIC);
    }

    if (Mcb->FullName.Buffer) {
        DEC_MEM_COUNT(PS_MCB_NAME, Mcb->FullName.Buffer,
                      Mcb->FullName.MaximumLength);
        Ext2FreePool(Mcb->FullName.Buffer, EXT2_FNAME_MAGIC);
    }

    /* free dentry */
    if (Mcb->de) {
        Ext2FreeEntry(Mcb->de);
    }

    Mcb->Identifier.Type = 0;
    Mcb->Identifier.Size = 0;

    ExFreeToNPagedLookasideList(&(Ext2Global->Ext2McbLookasideList), Mcb);
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

    _SEH2_TRY {
        ExAcquireResourceSharedLite(&Vcb->McbLock, TRUE);
        LockAcquired = TRUE;
        Mcb = Ext2SearchMcbWithoutLock(Parent, FileName);
    } _SEH2_FINALLY {
        if (LockAcquired) {
            ExReleaseResourceLite(&Vcb->McbLock);
        }
    } _SEH2_END;

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

    _SEH2_TRY {

        Ext2ReferMcb(Parent);

        if (Ext2IsDot(FileName)) {
            TmpMcb = Parent;
            Ext2ReferMcb(Parent);
            _SEH2_LEAVE;
        }

        if (Ext2IsDotDot(FileName)) {
            if (IsMcbRoot(Parent)) {
                TmpMcb = Parent;
            } else {
                TmpMcb = Parent->Parent;
            }
            if (TmpMcb) {
                Ext2ReferMcb(TmpMcb);
            }
            _SEH2_LEAVE;
        }

        if (IsMcbSymLink(Parent)) {
            if (Parent->Target) {
                TmpMcb = Parent->Target->Child;
                ASSERT(!IsMcbSymLink(Parent->Target));
            } else {
                TmpMcb = NULL;
                _SEH2_LEAVE;
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

    } _SEH2_FINALLY {

        Ext2DerefMcb(Parent);
    } _SEH2_END;

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

    _SEH2_TRY {

        ExAcquireResourceExclusiveLite(
            &Vcb->McbLock,
            TRUE );
        LockAcquired = TRUE;

        /* use it's target if it's a symlink */
        if (IsMcbSymLink(Parent)) {
            Parent = Parent->Target;
            ASSERT(!IsMcbSymLink(Parent));
        }

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
            Child->de->d_parent = Parent->de;
            Ext2ReferMcb(Parent);
            SetLongFlag(Child->Flags, MCB_ENTRY_TREE);
        }

    } _SEH2_FINALLY {

        if (LockAcquired) {
            ExReleaseResourceLite(&Vcb->McbLock);
        }
    } _SEH2_END;
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

    _SEH2_TRY {

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
                    DEBUG(DL_RES, ("Mcb %p %wZ removed from Mcb %p %wZ\n", Mcb,
                                   &Mcb->FullName, Mcb->Parent, &Mcb->Parent->FullName));
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
            Mcb->de->d_parent = NULL;
        }

    } _SEH2_FINALLY {

        if (LockAcquired) {
            ExReleaseResourceLite(&Vcb->McbLock);
        }
    } _SEH2_END;

    return TRUE;
}

VOID
Ext2CleanupAllMcbs(PEXT2_VCB Vcb)
{
    BOOLEAN   LockAcquired = FALSE;
    PEXT2_MCB Mcb = NULL;

    _SEH2_TRY {

        ExAcquireResourceExclusiveLite(
            &Vcb->McbLock,
            TRUE );
        LockAcquired = TRUE;

        while ((Mcb = Ext2FirstUnusedMcb(Vcb, TRUE, Vcb->NumOfMcb)) != 0) {
            while (Mcb) {
                PEXT2_MCB Next = Mcb->Next;
                if (IsMcbSymLink(Mcb)) {
                    Mcb->Target = NULL;
                }
                Ext2FreeMcb(Vcb, Mcb);
                Mcb = Next;
            }
        }
        Ext2FreeMcb(Vcb, Vcb->McbTree);
        Vcb->McbTree = NULL;

    } _SEH2_FINALLY {

        if (LockAcquired) {
            ExReleaseResourceLite(&Vcb->McbLock);
        }
    } _SEH2_END;
}

BOOLEAN
Ext2CheckSetBlock(PEXT2_IRP_CONTEXT IrpContext, PEXT2_VCB Vcb, LONGLONG Block)
{
    PEXT2_GROUP_DESC gd;
    ULONG           Group, dwBlk, Length;

    RTL_BITMAP      BlockBitmap;
    PVOID           BitmapCache;
    PBCB            BitmapBcb;

    LARGE_INTEGER   Offset;

    BOOLEAN         bModified = FALSE;


    Group = (ULONG)(Block - EXT2_FIRST_DATA_BLOCK) / BLOCKS_PER_GROUP;
    dwBlk = (ULONG)(Block - EXT2_FIRST_DATA_BLOCK) % BLOCKS_PER_GROUP;

    gd = ext4_get_group_desc(&Vcb->sb, Group, NULL);
    if (!gd) {
        return FALSE;
    }
    Offset.QuadPart = ext4_block_bitmap(&Vcb->sb, gd);
    Offset.QuadPart <<= BLOCK_BITS;

    if (Group == Vcb->sbi.s_groups_count - 1) {
        Length = (ULONG)(TOTAL_BLOCKS % BLOCKS_PER_GROUP);

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
                    PIN_WAIT,
                    &BitmapBcb,
                    &BitmapCache ) ) {

        DEBUG(DL_ERR, ( "Ext2CheckSetBlock: Failed to PinLock block %xh ...\n",
                        ext4_block_bitmap(&Vcb->sb, gd)));
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

    for (i = 0; i < Vcb->sbi.s_groups_count; i++) {

        PEXT2_GROUP_DESC    gd;

        gd = ext4_get_group_desc(&Vcb->sb, i, NULL);
        if (!gd)
            continue;
        Ext2CheckSetBlock(IrpContext, Vcb, ext4_block_bitmap(&Vcb->sb, gd));
        Ext2CheckSetBlock(IrpContext, Vcb, ext4_inode_bitmap(&Vcb->sb, gd));


        if (i == Vcb->sbi.s_groups_count - 1) {
            InodeBlocks = ((INODES_COUNT % INODES_PER_GROUP) *
                           Vcb->InodeSize + Vcb->BlockSize - 1) /
                          (Vcb->BlockSize);
        } else {
            InodeBlocks = (INODES_PER_GROUP * Vcb->InodeSize +
                           Vcb->BlockSize - 1) / (Vcb->BlockSize);
        }

        for (j = 0; j < InodeBlocks; j++ )
            Ext2CheckSetBlock(IrpContext, Vcb, ext4_inode_table(&Vcb->sb, gd) + j);
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
    InitializeListHead(&Vcb->Next);
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
    UniBuffer = Ext2AllocatePool(PagedPool, 1024, EXT2_PARAM_MAGIC);
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
            Ext2FreePool(UniBuffer, EXT2_PARAM_MAGIC);
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
#ifdef __REACTOS__
                        ParamPattern[i].uValue[k] = Params->Buffer[j + k];
                        k++;
#else
                        ParamPattern[i].uValue[k] = Params->Buffer[j + k++];
#endif
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

NTSTATUS
Ext2PerformRegistryVolumeParams(IN PEXT2_VCB Vcb)
{
    NTSTATUS        Status;
    UNICODE_STRING  VolumeParams;

    Status = Ext2QueryVolumeParams(Vcb, &VolumeParams);
    if (NT_SUCCESS(Status)) {

        /* set Vcb settings from registery */
        EXT2_VOLUME_PROPERTY2  Property;
        Ext2ParseRegistryVolumeParams(&VolumeParams, &Property);
        Ext2ProcessVolumeProperty(Vcb, &Property, sizeof(Property));

    } else {

        /* don't support auto mount */
        if (IsFlagOn(Ext2Global->Flags, EXT2_AUTO_MOUNT)) {
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_UNSUCCESSFUL;
            goto errorout;
        }

        /* set Vcb settings from Ext2Global */
        if (IsFlagOn(Ext2Global->Flags, EXT2_SUPPORT_WRITING)) {
            if (Vcb->IsExt3fs) {
                if (IsFlagOn(Ext2Global->Flags, EXT3_FORCE_WRITING)) {
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

        if ((Vcb->bHidingPrefix = Ext2Global->bHidingPrefix) != 0) {
            RtlCopyMemory( Vcb->sHidingPrefix,
                           Ext2Global->sHidingPrefix,
                           HIDINGPAT_LEN);
        } else {
            RtlZeroMemory( Vcb->sHidingPrefix,
                           HIDINGPAT_LEN);
        }

        if ((Vcb->bHidingSuffix = Ext2Global->bHidingSuffix) != 0) {
            RtlCopyMemory( Vcb->sHidingSuffix,
                           Ext2Global->sHidingSuffix,
                           HIDINGPAT_LEN);
        } else {
            RtlZeroMemory( Vcb->sHidingSuffix,
                           HIDINGPAT_LEN);
        }
    }

errorout:

    if (VolumeParams.Buffer) {
        Ext2FreePool(VolumeParams.Buffer, EXT2_PARAM_MAGIC);
    }

    return Status;
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
    while (  (Length > 0) &&
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

#define is_power_of_2(x)        ((x) != 0 && (((x) & ((x) - 1)) == 0))

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
    ULONG                       ChangeCount = 0, features;
    CC_FILE_SIZES               FileSizes;
    int                         i, has_huge_files;

    BOOLEAN                     VcbResourceInitialized = FALSE;
    BOOLEAN                     NotifySyncInitialized = FALSE;
    BOOLEAN                     ExtentsInitialized = FALSE;
    BOOLEAN                     InodeLookasideInitialized = FALSE;

    _SEH2_TRY {

        if (Vpb == NULL) {
            Status = STATUS_DEVICE_NOT_READY;
            _SEH2_LEAVE;
        }

        /* checking in/compat features */
        if (IsFlagOn(sb->s_feature_compat, EXT3_FEATURE_COMPAT_HAS_JOURNAL)) {
            Vcb->IsExt3fs = TRUE;
        }

        /* don't mount any volumes with external journal devices */
        if (IsFlagOn(sb->s_feature_incompat, EXT3_FEATURE_INCOMPAT_JOURNAL_DEV)) {
            Status = STATUS_UNRECOGNIZED_VOLUME;
            _SEH2_LEAVE;
        }

        /* check block size */
        Vcb->BlockSize  = (EXT2_MIN_BLOCK_SIZE << sb->s_log_block_size);
        /* we cannot handle volume with block size bigger than 64k */
        if (Vcb->BlockSize > EXT2_MAX_USER_BLKSIZE) {
            Status = STATUS_UNRECOGNIZED_VOLUME;
            _SEH2_LEAVE;
        }

        if (Vcb->BlockSize >= PAGE_SIZE) {
            Vcb->IoUnitBits = PAGE_SHIFT;
            Vcb->IoUnitSize = PAGE_SIZE;
        } else {
            Vcb->IoUnitSize = Vcb->BlockSize;
            Vcb->IoUnitBits = Ext2Log2(Vcb->BlockSize);
        }

        /* initialize vcb header members ... */
        Vcb->Header.IsFastIoPossible = FastIoIsNotPossible;
        Vcb->Header.Resource = &(Vcb->MainResource);
        Vcb->Header.PagingIoResource = &(Vcb->PagingIoResource);
        Vcb->OpenVolumeCount = 0;
        Vcb->OpenHandleCount = 0;
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
        if (Vcb->InodeSize == 0) {
            Vcb->InodeSize = EXT2_GOOD_OLD_INODE_SIZE;
        }

        /* initialize inode lookaside list */
        ExInitializeNPagedLookasideList(&(Vcb->InodeLookasideList),
                                        NULL, NULL, 0, sizeof(EXT2_INODE),
                                        'SNIE', 0);

        InodeLookasideInitialized = TRUE;

        /* initialize label in Vpb */
        Status = Ext2InitializeLabel(Vcb, sb);
        if (!NT_SUCCESS(Status)) {
            DbgBreak();
        }

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

        /* initialize UUID and serial number */
        if (Ext2IsNullUuid(sb->s_uuid)) {
            ExUuidCreate((UUID *)sb->s_uuid);
        } else {
            /* query parameters from registry */
            if (!NT_SUCCESS(Ext2PerformRegistryVolumeParams(Vcb))) {
                /* don't mount this volume */
                Status = STATUS_UNRECOGNIZED_VOLUME;
                _SEH2_LEAVE;
            }
        }

        Vpb->SerialNumber = ((ULONG*)sb->s_uuid)[0] +
                            ((ULONG*)sb->s_uuid)[1] +
                            ((ULONG*)sb->s_uuid)[2] +
                            ((ULONG*)sb->s_uuid)[3];

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
            _SEH2_LEAVE;
        }
        Vcb->ChangeCount = ChangeCount;

        /* create the stream object for ext2 volume */
        Vcb->Volume = IoCreateStreamFileObject(NULL, Vcb->Vpb->RealDevice);
        if (!Vcb->Volume) {
            Status = STATUS_UNRECOGNIZED_VOLUME;
            _SEH2_LEAVE;
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
        _SEH2_TRY {
            FsRtlInitializeLargeMcb(&(Vcb->Extents), PagedPool);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DbgBreak();
        } _SEH2_END;
        if (!NT_SUCCESS(Status)) {
            _SEH2_LEAVE;
        }
        ExtentsInitialized = TRUE;

        /* set block device */
        Vcb->bd.bd_dev = Vcb->RealDevice;
        Vcb->bd.bd_geo = Vcb->DiskGeometry;
        Vcb->bd.bd_part = Vcb->PartitionInformation;
        Vcb->bd.bd_volume = Vcb->Volume;
        Vcb->bd.bd_priv = (void *) Vcb;
        memset(&Vcb->bd.bd_bh_root, 0, sizeof(struct rb_root));
        spin_lock_init(&Vcb->bd.bd_bh_lock);
        Vcb->bd.bd_bh_cache = kmem_cache_create("bd_bh_buffer",
                                                Vcb->BlockSize, 0, 0, NULL);
        if (!Vcb->bd.bd_bh_cache) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

        Vcb->SectorBits = Ext2Log2(SECTOR_SIZE);
        Vcb->sb.s_magic = sb->s_magic;
        Vcb->sb.s_bdev = &Vcb->bd;
        Vcb->sb.s_blocksize = BLOCK_SIZE;
        Vcb->sb.s_blocksize_bits = BLOCK_BITS;
        Vcb->sb.s_priv = (void *) Vcb;
        Vcb->sb.s_fs_info = &Vcb->sbi;

        Vcb->sbi.s_es = sb;
        Vcb->sbi.s_blocks_per_group = sb->s_blocks_per_group;
        Vcb->sbi.s_first_ino = sb->s_first_ino;
        Vcb->sbi.s_desc_size = sb->s_desc_size;

        if (EXT3_HAS_INCOMPAT_FEATURE(&Vcb->sb, EXT4_FEATURE_INCOMPAT_64BIT)) {
            if (Vcb->sbi.s_desc_size < EXT4_MIN_DESC_SIZE_64BIT ||
                    Vcb->sbi.s_desc_size > EXT4_MAX_DESC_SIZE ||
                    !is_power_of_2(Vcb->sbi.s_desc_size)) {
                DEBUG(DL_ERR, ("EXT4-fs: unsupported descriptor size %lu\n", Vcb->sbi.s_desc_size));
                Status = STATUS_DISK_CORRUPT_ERROR;
                _SEH2_LEAVE;
            }
        } else {
            Vcb->sbi.s_desc_size = EXT4_MIN_DESC_SIZE;
        }

        Vcb->sbi.s_blocks_per_group = sb->s_blocks_per_group;
        Vcb->sbi.s_inodes_per_group = sb->s_inodes_per_group;
        if (EXT3_INODES_PER_GROUP(&Vcb->sb) == 0) {
            Status = STATUS_DISK_CORRUPT_ERROR;
            _SEH2_LEAVE;
        }
        Vcb->sbi.s_inodes_per_block = BLOCK_SIZE / Vcb->InodeSize;
        if (Vcb->sbi.s_inodes_per_block == 0) {
            Status = STATUS_DISK_CORRUPT_ERROR;
            _SEH2_LEAVE;
        }
        Vcb->sbi.s_itb_per_group = Vcb->sbi.s_inodes_per_group /
                                   Vcb->sbi.s_inodes_per_block;


        Vcb->sbi.s_desc_per_block = BLOCK_SIZE / GROUP_DESC_SIZE;
        Vcb->sbi.s_desc_per_block_bits = ilog2(Vcb->sbi.s_desc_per_block);

        for (i=0; i < 4; i++) {
            Vcb->sbi.s_hash_seed[i] = sb->s_hash_seed[i];
        }
        Vcb->sbi.s_def_hash_version = sb->s_def_hash_version;

        if (le32_to_cpu(sb->s_rev_level) == EXT3_GOOD_OLD_REV &&
                (EXT3_HAS_COMPAT_FEATURE(&Vcb->sb, ~0U) ||
                 EXT3_HAS_RO_COMPAT_FEATURE(&Vcb->sb, ~0U) ||
                 EXT3_HAS_INCOMPAT_FEATURE(&Vcb->sb, ~0U))) {
            printk(KERN_WARNING
                   "EXT3-fs warning: feature flags set on rev 0 fs, "
                   "running e2fsck is recommended\n");
        }

        /*
         * Check feature flags regardless of the revision level, since we
         * previously didn't change the revision level when setting the flags,
         * so there is a chance incompat flags are set on a rev 0 filesystem.
         */
        features = EXT3_HAS_INCOMPAT_FEATURE(&Vcb->sb, ~EXT4_FEATURE_INCOMPAT_SUPP);
        if (features & EXT4_FEATURE_INCOMPAT_DIRDATA) {
            SetLongFlag(Vcb->Flags, VCB_READ_ONLY);
            ClearFlag(features, EXT4_FEATURE_INCOMPAT_DIRDATA);
        }
        if (features) {
            printk(KERN_ERR "EXT3-fs: %s: couldn't mount because of "
                   "unsupported optional features (%x).\n",
                   Vcb->sb.s_id, le32_to_cpu(features));
            Status = STATUS_UNRECOGNIZED_VOLUME;
            _SEH2_LEAVE;
        }

        features = EXT3_HAS_RO_COMPAT_FEATURE(&Vcb->sb, ~EXT4_FEATURE_RO_COMPAT_SUPP);
        if (features) {
            printk(KERN_ERR "EXT3-fs: %s: unsupported optional features in this volume: (%x).\n",
                   Vcb->sb.s_id, le32_to_cpu(features));
            if (CanIWrite(Vcb)) {
            } else {
                SetLongFlag(Vcb->Flags, VCB_READ_ONLY);
            }
        }

        has_huge_files = EXT3_HAS_RO_COMPAT_FEATURE(&Vcb->sb, EXT4_FEATURE_RO_COMPAT_HUGE_FILE);

        Vcb->sb.s_maxbytes = ext3_max_size(BLOCK_BITS, has_huge_files);
        Vcb->max_bitmap_bytes = ext3_max_bitmap_size(BLOCK_BITS,
                                has_huge_files);
        Vcb->max_bytes = ext3_max_size(BLOCK_BITS, has_huge_files);

        /* calculate maximum file bocks ... */
        {
            ULONG  dwData[EXT2_BLOCK_TYPES] = {EXT2_NDIR_BLOCKS, 1, 1, 1};
            ULONG  i;

            ASSERT(BLOCK_BITS == Ext2Log2(BLOCK_SIZE));

            Vcb->sbi.s_groups_count = (ULONG)(ext3_blocks_count(sb) - sb->s_first_data_block +
                                              sb->s_blocks_per_group - 1) / sb->s_blocks_per_group;

            Vcb->max_data_blocks = 0;
            for (i = 0; i < EXT2_BLOCK_TYPES; i++) {
                if (BLOCK_BITS >= 12 && i == (EXT2_BLOCK_TYPES - 1)) {
                    dwData[i] = 0x40000000;
                } else {
                    dwData[i] = dwData[i] << ((BLOCK_BITS - 2) * i);
                }
                Vcb->max_blocks_per_layer[i] = dwData[i];
                Vcb->max_data_blocks += Vcb->max_blocks_per_layer[i];
            }
        }

        Vcb->sbi.s_gdb_count = (Vcb->sbi.s_groups_count + Vcb->sbi.s_desc_per_block - 1) /
                               Vcb->sbi.s_desc_per_block;
        /* load all gorup desc */
        if (!Ext2LoadGroup(Vcb)) {
            Status = STATUS_UNSUCCESSFUL;
            _SEH2_LEAVE;
        }

        /* recovery journal since it's ext3 */
        if (Vcb->IsExt3fs) {
            Ext2RecoverJournal(IrpContext, Vcb);
            if (IsFlagOn(Vcb->Flags, VCB_JOURNAL_RECOVER)) {
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
            DbgBreak();
            Status = STATUS_UNSUCCESSFUL;
            _SEH2_LEAVE;
        }

        Vcb->sb.s_root = Ext2BuildEntry(Vcb, NULL, &RootNode);
        if (!Vcb->sb.s_root) {
            DbgBreak();
            Status = STATUS_UNSUCCESSFUL;
            _SEH2_LEAVE;
        }
        Vcb->sb.s_root->d_sb = &Vcb->sb;
        Vcb->sb.s_root->d_inode = &Vcb->McbTree->Inode;
        Vcb->McbTree->de = Vcb->sb.s_root;

        /* load root inode */
        Vcb->McbTree->Inode.i_ino = EXT2_ROOT_INO;
        Vcb->McbTree->Inode.i_sb = &Vcb->sb;
        if (!Ext2LoadInode(Vcb, &Vcb->McbTree->Inode)) {
            DbgBreak();
            Status = STATUS_CANT_WAIT;
            _SEH2_LEAVE;
        }

        /* initializeroot node */
        Vcb->McbTree->CreationTime = Ext2NtTime(Vcb->McbTree->Inode.i_ctime);
        Vcb->McbTree->LastAccessTime = Ext2NtTime(Vcb->McbTree->Inode.i_atime);
        Vcb->McbTree->LastWriteTime = Ext2NtTime(Vcb->McbTree->Inode.i_mtime);
        Vcb->McbTree->ChangeTime = Ext2NtTime(Vcb->McbTree->Inode.i_mtime);

        /* check bitmap if user specifies it */
        if (IsFlagOn(Ext2Global->Flags, EXT2_CHECKING_BITMAP)) {
            Ext2CheckBitmapConsistency(IrpContext, Vcb);
        }

        /* get anything doen, then refer target device */
        ObReferenceObject(Vcb->TargetDeviceObject);
        SetLongFlag(Vcb->Flags, VCB_INITIALIZED);

    } _SEH2_FINALLY {

        if (!NT_SUCCESS(Status)) {

            if (Vcb->McbTree) {
                Ext2FreeMcb(Vcb, Vcb->McbTree);
            }

            if (InodeLookasideInitialized) {
                ExDeleteNPagedLookasideList(&(Vcb->InodeLookasideList));
            }

            if (ExtentsInitialized) {
                Ext2PutGroup(Vcb);
                if (Vcb->bd.bd_bh_cache)
                    kmem_cache_destroy(Vcb->bd.bd_bh_cache);
                FsRtlUninitializeLargeMcb(&(Vcb->Extents));
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

            if (VcbResourceInitialized) {
                ExDeleteResourceLite(&Vcb->McbLock);
                ExDeleteResourceLite(&Vcb->MetaLock);
                ExDeleteResourceLite(&Vcb->MainResource);
                ExDeleteResourceLite(&Vcb->PagingIoResource);
            }
        }
    } _SEH2_END;

    return Status;
}


VOID
Ext2TearDownStream(IN PEXT2_VCB Vcb)
{
    PFILE_OBJECT    Stream = Vcb->Volume;
    IO_STATUS_BLOCK IoStatus;

    ASSERT(Vcb != NULL);
    ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
           (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

    if (Stream) {

        Vcb->Volume = NULL;

        if (IsFlagOn(Stream->Flags, FO_FILE_MODIFIED)) {
            CcFlushCache(&(Vcb->SectionObject), NULL, 0, &IoStatus);
            ClearFlag(Stream->Flags, FO_FILE_MODIFIED);
        }

        if (Stream->PrivateCacheMap) {
            Ext2SyncUninitializeCacheMap(Stream);
        }

        ObDereferenceObject(Stream);
    }
}

VOID
Ext2DestroyVcb (IN PEXT2_VCB Vcb)
{
    ASSERT(Vcb != NULL);
    ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
           (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

    DEBUG(DL_FUN, ("Ext2DestroyVcb ...\n"));

    if (Vcb->Volume) {
        Ext2TearDownStream(Vcb);
    }
    ASSERT(NULL == Vcb->Volume);

    FsRtlNotifyUninitializeSync(&Vcb->NotifySync);
    Ext2ListExtents(&Vcb->Extents);
    FsRtlUninitializeLargeMcb(&(Vcb->Extents));

    Ext2CleanupAllMcbs(Vcb);

    Ext2PutGroup(Vcb);

    if (Vcb->bd.bd_bh_cache)
        kmem_cache_destroy(Vcb->bd.bd_bh_cache);

    if (Vcb->SuperBlock) {
        Ext2FreePool(Vcb->SuperBlock, EXT2_SB_MAGIC);
        Vcb->SuperBlock = NULL;
    }

    if (IsFlagOn(Vcb->Flags, VCB_NEW_VPB)) {
        ASSERT(Vcb->Vpb2 != NULL);
        Ext2FreePool(Vcb->Vpb2, TAG_VPB);
        DEC_MEM_COUNT(PS_VPB, Vcb->Vpb2, sizeof(VPB));
        Vcb->Vpb2 = NULL;
    }

    ObDereferenceObject(Vcb->TargetDeviceObject);

    ExDeleteNPagedLookasideList(&(Vcb->InodeLookasideList));
    ExDeleteResourceLite(&Vcb->McbLock);
    ExDeleteResourceLite(&Vcb->MetaLock);
    ExDeleteResourceLite(&Vcb->PagingIoResource);
    ExDeleteResourceLite(&Vcb->MainResource);

    DEBUG(DL_DBG, ("Ext2DestroyVcb: DevObject=%p Vcb=%p\n", Vcb->DeviceObject, Vcb));
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
    if (Mcb->Inode.i_ino == EXT2_ROOT_INO) {
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
    if (Mcb->Inode.i_ino == EXT2_ROOT_INO) {
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
    if (Mcb->Inode.i_ino == EXT2_ROOT_INO) {
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
                        Mcb->Refercount == 0 &&
                        (Mcb->Child == NULL || IsMcbSymLink(Mcb))) {

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
VOID NTAPI
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

    _SEH2_TRY {

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
                Timeout.QuadPart = (LONGLONG)-4*1000*1000*10; /* 4 seconds */
                NumOfMcbs = Ext2Global->MaxDepth / 8;
            } else if (DidNothing) {
                Timeout.QuadPart = (LONGLONG)-8*1000*1000*10; /* 8 seconds */
                if (LastState) {
                    Timeout.QuadPart *= 2;
                }
                NumOfMcbs = Ext2Global->MaxDepth / 16;
            } else {
                Timeout.QuadPart = (LONGLONG)-5*1000*1000*10; /* 5 seconds */
                if (LastState) {
                    Timeout.QuadPart *= 2;
                }
                NumOfMcbs = Ext2Global->MaxDepth / 32;
            }

            if (NumOfMcbs == 0)
                NumOfMcbs = 1;

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
                    DEBUG(DL_RES, ( "Ext2ReaperThread: releasing Mcb (%p): %wZ"
                                    " Total: %xh\n", Mcb, &Mcb->FullName,
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

    } _SEH2_FINALLY {

        if (GlobalAcquired) {
            ExReleaseResourceLite(&Ext2Global->Resource);
        }
    } _SEH2_END;

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
