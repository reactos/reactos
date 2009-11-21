/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             cmcb.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2AcquireForLazyWrite)
#pragma alloc_text(PAGE, Ext2ReleaseFromLazyWrite)
#pragma alloc_text(PAGE, Ext2AcquireForReadAhead)
#pragma alloc_text(PAGE, Ext2ReleaseFromReadAhead)
#pragma alloc_text(PAGE, Ext2NoOpAcquire)
#pragma alloc_text(PAGE, Ext2NoOpRelease)
#pragma alloc_text(PAGE, Ext2AcquireForCreateSection)
#pragma alloc_text(PAGE, Ext2ReleaseForCreateSection)
#pragma alloc_text(PAGE, Ext2AcquireFileForModWrite)
#pragma alloc_text(PAGE, Ext2ReleaseFileForModWrite)
#pragma alloc_text(PAGE, Ext2AcquireFileForCcFlush)
#pragma alloc_text(PAGE, Ext2ReleaseFileForCcFlush)
#endif

#define CMCB_DEBUG_LEVEL DL_NVR

BOOLEAN
Ext2AcquireForLazyWrite (
        IN PVOID    Context,
        IN BOOLEAN  Wait)
{
    //
    // On a readonly filesystem this function still has to exist but it
    // doesn't need to do anything.
    
    PEXT2_FCB    Fcb;
    
    Fcb = (PEXT2_FCB) Context;
    ASSERT(Fcb != NULL);
    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
        (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

    DEBUG(CMCB_DEBUG_LEVEL, ("Ext2AcquireForLazyWrite: %s %s Fcb=%p\n",
          Ext2GetCurrentProcessName(), "ACQUIRE_FOR_LAZY_WRITE", Fcb));

    if(!ExAcquireResourceSharedLite(
            &Fcb->PagingIoResource, Wait)) {
        return FALSE;
    }

    ASSERT(Fcb->LazyWriterThread == NULL);
    Fcb->LazyWriterThread = PsGetCurrentThread();

    ASSERT(IoGetTopLevelIrp() == NULL);

    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}

VOID
Ext2ReleaseFromLazyWrite (IN PVOID Context)
{
    //
    // On a readonly filesystem this function still has to exist but it
    // doesn't need to do anything.
    PEXT2_FCB Fcb;
    
    Fcb = (PEXT2_FCB) Context;
    
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
        (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

    DEBUG(CMCB_DEBUG_LEVEL, ( "Ext2ReleaseFromLazyWrite: %s %s Fcb=%p\n",
          Ext2GetCurrentProcessName(), "RELEASE_FROM_LAZY_WRITE", Fcb));

    ASSERT(Fcb->LazyWriterThread == PsGetCurrentThread());
    Fcb->LazyWriterThread = NULL;

    ExReleaseResourceLite(&Fcb->PagingIoResource);
 
    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    IoSetTopLevelIrp( NULL );
}

BOOLEAN
Ext2AcquireForReadAhead (IN PVOID    Context,
             IN BOOLEAN  Wait)
{
    PEXT2_FCB    Fcb;
    
    Fcb = (PEXT2_FCB) Context;
    
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
        (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

    DEBUG(CMCB_DEBUG_LEVEL, ("Ext2AcquireForReadAhead: i=%xh Fcb=%p\n", 
          Fcb->Mcb->iNo, Fcb));

    if (!ExAcquireResourceSharedLite(
        &Fcb->MainResource, Wait  ))
        return FALSE;

    ASSERT(IoGetTopLevelIrp() == NULL);

    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}

VOID
Ext2ReleaseFromReadAhead (IN PVOID Context)
{
    PEXT2_FCB Fcb;
    
    Fcb = (PEXT2_FCB) Context;
    
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
        (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

    DEBUG(CMCB_DEBUG_LEVEL, ("Ext2ReleaseFromReadAhead: i=%xh Fcb=%p\n", 
          Fcb->Mcb->iNo, Fcb));

    IoSetTopLevelIrp( NULL );

    ExReleaseResourceLite(&Fcb->MainResource);
}

BOOLEAN
Ext2NoOpAcquire (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )
{
    ASSERT(IoGetTopLevelIrp() == NULL);
    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    return TRUE;
}

VOID
Ext2NoOpRelease (
    IN PVOID Fcb
    )
{
    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    IoSetTopLevelIrp( NULL );

    return;
}


VOID
Ext2AcquireForCreateSection (
    IN PFILE_OBJECT FileObject
    )

{
    PEXT2_FCB Fcb = FileObject->FsContext;

    if (Fcb->Header.PagingIoResource != NULL) {
        ExAcquireResourceExclusiveLite(Fcb->Header.PagingIoResource, TRUE);
    }

    DEBUG(CMCB_DEBUG_LEVEL, ("Ext2AcquireForCreateSection:  Fcb=%p\n", Fcb));
}

VOID
Ext2ReleaseForCreateSection (
    IN PFILE_OBJECT FileObject
    )

{
    PEXT2_FCB Fcb = FileObject->FsContext;

    DEBUG(CMCB_DEBUG_LEVEL, ("Ext2ReleaseForCreateSection:  Fcb=%p\n", Fcb));

    if (Fcb->Header.PagingIoResource != NULL) {
        ExReleaseResourceLite(Fcb->Header.PagingIoResource);
    }
}


NTSTATUS
Ext2AcquireFileForModWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER EndingOffset,
    OUT PERESOURCE *ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject
    )

{
    BOOLEAN ResourceAcquired = FALSE;

    PEXT2_FCB Fcb = FileObject->FsContext;

    if (Fcb->Header.PagingIoResource != NULL) {
        *ResourceToRelease = Fcb->Header.PagingIoResource;
    } else {
        *ResourceToRelease = Fcb->Header.Resource;
    }

    ResourceAcquired = ExAcquireResourceSharedLite(*ResourceToRelease, FALSE);
    if (!ResourceAcquired) {
        *ResourceToRelease = NULL;
    }

    DEBUG(CMCB_DEBUG_LEVEL, ("Ext2AcquireFileForModWrite:  Fcb=%p Acquired=%d\n",
                              Fcb, ResourceAcquired));

    return (ResourceAcquired ? STATUS_SUCCESS : STATUS_CANT_WAIT);
}

NTSTATUS
Ext2ReleaseFileForModWrite (
    IN PFILE_OBJECT FileObject,
    IN PERESOURCE ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PEXT2_FCB Fcb = FileObject->FsContext;

    DEBUG(CMCB_DEBUG_LEVEL, ("Ext2ReleaseFileForModWrite: Fcb=%p\n", Fcb));

    if (ResourceToRelease != NULL) {
        ASSERT(ResourceToRelease == Fcb->Header.PagingIoResource ||
               ResourceToRelease == Fcb->Header.Resource);
        ExReleaseResourceLite(ResourceToRelease);
    } else {
        DbgBreak();
    }

    return STATUS_SUCCESS;
}

NTSTATUS
Ext2AcquireFileForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PEXT2_FCB Fcb = FileObject->FsContext;

    if (Fcb->Header.PagingIoResource != NULL) {
        ExAcquireResourceSharedLite(Fcb->Header.PagingIoResource, TRUE);
    }

    DEBUG(CMCB_DEBUG_LEVEL, ("Ext2AcquireFileForCcFlush: Fcb=%p\n", Fcb));

    return STATUS_SUCCESS;
}

NTSTATUS
Ext2ReleaseFileForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PEXT2_FCB Fcb = FileObject->FsContext;

    DEBUG(CMCB_DEBUG_LEVEL, ("Ext2ReleaseFileForCcFlush: Fcb=%p\n", Fcb));

    if (Fcb->Header.PagingIoResource != NULL) {
        ExReleaseResourceLite(Fcb->Header.PagingIoResource);
    }

    return STATUS_SUCCESS;
}

