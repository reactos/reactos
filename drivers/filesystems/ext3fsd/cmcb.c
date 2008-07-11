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
#pragma alloc_text(PAGE, Ext2NoOpRelease)
#pragma alloc_text(PAGE, Ext2NoOpRelease)
#endif


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

    DEBUG(DL_INF, ( "Ext2AcquireForLazyWrite: %s %s %wZ\n",
        Ext2GetCurrentProcessName(),
        "ACQUIRE_FOR_LAZY_WRITE",
        &Fcb->Mcb->FullName        ));

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

    DEBUG(DL_INF, ( "Ext2ReleaseFromLazyWrite: %s %s %wZ\n",
        Ext2GetCurrentProcessName(),
        "RELEASE_FROM_LAZY_WRITE",
        &Fcb->Mcb->FullName
        ));

    ASSERT(Fcb->LazyWriterThread == PsGetCurrentThread());
    Fcb->LazyWriterThread = NULL;

    DEBUG(DL_INF, ("Ext2ReleaseFromLazyWrite: Inode=%xh %S\n", 
               Fcb->Mcb->iNo, Fcb->Mcb->ShortName.Buffer ));

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

    DEBUG(DL_INF, ("Ext2AcquireForReadAhead: Inode=%xh %S\n", 
                Fcb->Mcb->iNo, Fcb->Mcb->ShortName.Buffer ));

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

    DEBUG(DL_INF, ("Ext2ReleaseFromReadAhead: Inode=%xh %S\n", 
                Fcb->Mcb->iNo, Fcb->Mcb->ShortName.Buffer ));

    IoSetTopLevelIrp( NULL );

    ExReleaseResourceLite(&Fcb->MainResource);
}

BOOLEAN
Ext2NoOpAcquire (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )
{
    UNREFERENCED_PARAMETER( Fcb );
    UNREFERENCED_PARAMETER( Wait );

    //
    //  This is a kludge because Cc is really the top level.  We it
    //  enters the file system, we will think it is a resursive call
    //  and complete the request with hard errors or verify.  It will
    //  have to deal with them, somehow....
    //

    ASSERT(IoGetTopLevelIrp() == NULL);

    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}

VOID
Ext2NoOpRelease (
    IN PVOID Fcb
    )
{
    //
    //  Clear the kludge at this point.
    //

    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    IoSetTopLevelIrp( NULL );

    UNREFERENCED_PARAMETER( Fcb );

    return;
}
