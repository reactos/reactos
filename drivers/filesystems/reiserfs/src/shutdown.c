/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             shutdown.c
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
#pragma alloc_text(PAGE, RfsdShutDown)
#endif

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdShutDown (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS                Status;
#ifndef __REACTOS__
    PKEVENT                 Event;
#endif
    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;
    PRFSD_VCB               Vcb;
    PLIST_ENTRY             ListEntry;
    BOOLEAN                 GlobalResourceAcquired = FALSE;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext);
    
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));

        Status = STATUS_SUCCESS;

        Irp = IrpContext->Irp;
    
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "by design" )
#endif
        if (!ExAcquireResourceExclusiveLite(
                &RfsdGlobal->Resource,
                IrpContext->IsSynchronous )) {
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }
            
        GlobalResourceAcquired = TRUE;

#ifndef __REACTOS__
        Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), RFSD_POOL_TAG);
        KeInitializeEvent(Event, NotificationEvent, FALSE );
#endif

        for (ListEntry = RfsdGlobal->VcbList.Flink;
             ListEntry != &(RfsdGlobal->VcbList);
             ListEntry = ListEntry->Flink ) {

            Vcb = CONTAINING_RECORD(ListEntry, RFSD_VCB, Next);

            if (ExAcquireResourceExclusiveLite(
                &Vcb->MainResource,
                TRUE )) {

                Status = RfsdFlushFiles(Vcb, TRUE);
                if(!NT_SUCCESS(Status)) {
                    DbgBreak();
                }

                Status = RfsdFlushVolume(Vcb, TRUE);

                if(!NT_SUCCESS(Status)) {
                    DbgBreak();
                }

                RfsdDiskShutDown(Vcb);

                ExReleaseResourceForThreadLite(
                    &Vcb->MainResource,
                    ExGetCurrentResourceThread());
            }
        }

/*
        IoUnregisterFileSystem(RfsdGlobal->DeviceObject);
*/
    } _SEH2_FINALLY {

        if (GlobalResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &RfsdGlobal->Resource,
                ExGetCurrentResourceThread() );
        }

        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING) {
                RfsdQueueRequest(IrpContext);
            } else {
                RfsdCompleteIrpContext(IrpContext, Status);
            }
        }
    } _SEH2_END;

    return Status;
}
