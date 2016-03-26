/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             cmcb.c
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
#pragma alloc_text(PAGE, RfsdAcquireForLazyWrite)
#pragma alloc_text(PAGE, RfsdReleaseFromLazyWrite)
#pragma alloc_text(PAGE, RfsdAcquireForReadAhead)
#pragma alloc_text(PAGE, RfsdReleaseFromReadAhead)
#pragma alloc_text(PAGE, RfsdNoOpAcquire)
#pragma alloc_text(PAGE, RfsdNoOpRelease)
#endif

__drv_mustHoldCriticalRegion
BOOLEAN NTAPI
RfsdAcquireForLazyWrite (
        IN PVOID    Context,
        IN BOOLEAN  Wait)
{
    PRFSD_FCB    Fcb;

    PAGED_CODE();

    Fcb = (PRFSD_FCB) Context;
    
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
        (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

    RfsdPrint((DBG_INFO, "RfsdAcquireForLazyWrite: %s %s %s\n",
        RfsdGetCurrentProcessName(),
        "ACQUIRE_FOR_LAZY_WRITE",
        Fcb->AnsiFileName.Buffer        ));

    if (!IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY)) {
        RfsdPrint(( DBG_INFO, "RfsdAcquireForLazyWrite: Key=%x,%xh %S\n", 
			Fcb->RfsdMcb->Key.k_dir_id, Fcb->RfsdMcb->Key.k_objectid, Fcb->RfsdMcb->ShortName.Buffer ));

        if(!ExAcquireResourceSharedLite(
            &Fcb->PagingIoResource, Wait)) {
            return FALSE;
        }
    }

    ASSERT(IoGetTopLevelIrp() == NULL);

    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}

__drv_mustHoldCriticalRegion
VOID NTAPI
RfsdReleaseFromLazyWrite (IN PVOID Context)
{
    PRFSD_FCB Fcb;

    PAGED_CODE();

    Fcb = (PRFSD_FCB) Context;
    
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
        (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

    RfsdPrint((DBG_INFO, "RfsdReleaseFromLazyWrite: %s %s %s\n",
        RfsdGetCurrentProcessName(),
        "RELEASE_FROM_LAZY_WRITE",
        Fcb->AnsiFileName.Buffer
        ));

    if (!IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY)) {
        RfsdPrint(( DBG_INFO, "RfsdReleaseFromLazyWrite: Inode=%x%xh %S\n", 
			Fcb->RfsdMcb->Key.k_dir_id, Fcb->RfsdMcb->Key.k_objectid, Fcb->RfsdMcb->ShortName.Buffer ));

        ExReleaseResourceLite(&Fcb->PagingIoResource);
    }

    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    IoSetTopLevelIrp( NULL );
}

__drv_mustHoldCriticalRegion
BOOLEAN NTAPI
RfsdAcquireForReadAhead (IN PVOID    Context,
             IN BOOLEAN  Wait)
{
    PRFSD_FCB    Fcb;

    PAGED_CODE();

    Fcb = (PRFSD_FCB) Context;
    
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
        (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

    RfsdPrint(( DBG_INFO, "RfsdAcquireForReadAhead: Inode=%x,%xh %S\n", 
			Fcb->RfsdMcb->Key.k_dir_id, Fcb->RfsdMcb->Key.k_objectid, Fcb->RfsdMcb->ShortName.Buffer ));

    if (!ExAcquireResourceSharedLite(
        &Fcb->MainResource, Wait  ))
        return FALSE;

    ASSERT(IoGetTopLevelIrp() == NULL);

    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}

__drv_mustHoldCriticalRegion
VOID NTAPI
RfsdReleaseFromReadAhead (IN PVOID Context)
{
    PRFSD_FCB Fcb;

    PAGED_CODE();

    Fcb = (PRFSD_FCB) Context;
    
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
        (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

    RfsdPrint(( DBG_INFO, "RfsdReleaseFromReadAhead: Inode=%x,%xh %S\n", 
			Fcb->RfsdMcb->Key.k_dir_id, Fcb->RfsdMcb->Key.k_objectid, Fcb->RfsdMcb->ShortName.Buffer ));

    IoSetTopLevelIrp( NULL );

    ExReleaseResourceLite(&Fcb->MainResource);
}

BOOLEAN NTAPI
RfsdNoOpAcquire (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )
{
    UNREFERENCED_PARAMETER( Fcb );
    UNREFERENCED_PARAMETER( Wait );

    PAGED_CODE();

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

VOID NTAPI
RfsdNoOpRelease (
    IN PVOID Fcb
    )
{
    PAGED_CODE();

    //
    //  Clear the kludge at this point.
    //

    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    IoSetTopLevelIrp( NULL );

    UNREFERENCED_PARAMETER( Fcb );

    return;
}
