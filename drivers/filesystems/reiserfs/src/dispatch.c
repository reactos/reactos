/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             dispatch.c
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
#pragma alloc_text(PAGE, RfsdQueueRequest)
#pragma alloc_text(PAGE, RfsdDeQueueRequest)
#pragma alloc_text(PAGE, RfsdDispatchRequest)
#pragma alloc_text(PAGE, RfsdBuildRequest)
#endif

NTSTATUS
RfsdQueueRequest (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PAGED_CODE();

    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
        (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
    
    // IsSynchronous means we can block (so we don't requeue it)
    IrpContext->IsSynchronous = TRUE;

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED);
    
    IoMarkIrpPending(IrpContext->Irp);
    
    ExInitializeWorkItem(
        &IrpContext->WorkQueueItem,
        RfsdDeQueueRequest,
        IrpContext );
    
    ExQueueWorkItem(&IrpContext->WorkQueueItem, CriticalWorkQueue);
    
    return STATUS_PENDING;
}

VOID NTAPI
RfsdDeQueueRequest (IN PVOID Context)
{
    PRFSD_IRP_CONTEXT IrpContext;

    PAGED_CODE();

    IrpContext = (PRFSD_IRP_CONTEXT) Context;

    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
           (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));

    _SEH2_TRY {

        _SEH2_TRY {

            FsRtlEnterFileSystem();

            if (!IrpContext->IsTopLevel) {

                IoSetTopLevelIrp((PIRP) FSRTL_FSP_TOP_LEVEL_IRP);
            }

            RfsdDispatchRequest(IrpContext);

        } _SEH2_EXCEPT (RfsdExceptionFilter(IrpContext, _SEH2_GetExceptionInformation())) {

            RfsdExceptionHandler(IrpContext);
        } _SEH2_END;

    } _SEH2_FINALLY {

        IoSetTopLevelIrp(NULL);

        FsRtlExitFileSystem();
    } _SEH2_END;
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdDispatchRequest (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PAGED_CODE();

    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
        (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
    
    switch (IrpContext->MajorFunction) {

        case IRP_MJ_CREATE:
            return RfsdCreate(IrpContext);
        
        case IRP_MJ_CLOSE:
            return RfsdClose(IrpContext);
        
        case IRP_MJ_READ:
            return RfsdRead(IrpContext);

#if !RFSD_READ_ONLY
        case IRP_MJ_WRITE:
            return RfsdWrite(IrpContext);
#endif // !RFSD_READ_ONLY

        case IRP_MJ_FLUSH_BUFFERS:
            return RfsdFlush(IrpContext);

        case IRP_MJ_QUERY_INFORMATION:
            return RfsdQueryInformation(IrpContext);

        case IRP_MJ_SET_INFORMATION:
            return RfsdSetInformation(IrpContext);

        case IRP_MJ_QUERY_VOLUME_INFORMATION:
            return RfsdQueryVolumeInformation(IrpContext);

#if !RFSD_READ_ONLY
        case IRP_MJ_SET_VOLUME_INFORMATION:
            return RfsdSetVolumeInformation(IrpContext);
#endif // !RFSD_READ_ONLY

        case IRP_MJ_DIRECTORY_CONTROL:
            return RfsdDirectoryControl(IrpContext);
        
        case IRP_MJ_FILE_SYSTEM_CONTROL:
            return RfsdFileSystemControl(IrpContext);

        case IRP_MJ_DEVICE_CONTROL:
            return RfsdDeviceControl(IrpContext);
        
        case IRP_MJ_LOCK_CONTROL:
            return RfsdLockControl(IrpContext);
        
        case IRP_MJ_CLEANUP:
            return RfsdCleanup(IrpContext);

        case IRP_MJ_SHUTDOWN:
            return RfsdShutDown(IrpContext);

#if (_WIN32_WINNT >= 0x0500)
        case IRP_MJ_PNP:
            return RfsdPnp(IrpContext);
#endif //(_WIN32_WINNT >= 0x0500)        
        default:
			{
				NTSTATUS DefaultRC = STATUS_INVALID_DEVICE_REQUEST;

				RfsdPrint((DBG_ERROR, "RfsdDispatchRequest: Unexpected major function: %xh\n",
                       IrpContext->MajorFunction));

				RfsdCompleteIrpContext(IrpContext, DefaultRC);

				return DefaultRC;
			}
    }
}

NTSTATUS NTAPI
RfsdBuildRequest (PDEVICE_OBJECT   DeviceObject, PIRP Irp)
{
    BOOLEAN             AtIrqlPassiveLevel = FALSE;
    BOOLEAN             IsTopLevelIrp = FALSE;
    PRFSD_IRP_CONTEXT   IrpContext = NULL;
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();

    _SEH2_TRY {

        _SEH2_TRY {

#if DBG
            RfsdDbgPrintCall(DeviceObject, Irp);
#endif
            
            AtIrqlPassiveLevel = (KeGetCurrentIrql() == PASSIVE_LEVEL);
            
            if (AtIrqlPassiveLevel) {

                FsRtlEnterFileSystem();
            }
            
            if (!IoGetTopLevelIrp()) {

                IsTopLevelIrp = TRUE;
                IoSetTopLevelIrp(Irp);
            }
            
            IrpContext = RfsdAllocateIrpContext(DeviceObject, Irp);
            
            if (!IrpContext) {

                Status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Status = Status;

                RfsdCompleteRequest(Irp, TRUE, IO_NO_INCREMENT);

            } else {

                if ((IrpContext->MajorFunction == IRP_MJ_CREATE) &&
                    !AtIrqlPassiveLevel) {

                    DbgBreak();
                }

                Status = RfsdDispatchRequest(IrpContext);
            }
        } _SEH2_EXCEPT (RfsdExceptionFilter(IrpContext, _SEH2_GetExceptionInformation())) {

            Status = RfsdExceptionHandler(IrpContext);
        } _SEH2_END;

    } _SEH2_FINALLY  {

        if (IsTopLevelIrp) {
            IoSetTopLevelIrp(NULL);
        }
        
        if (AtIrqlPassiveLevel) {
            FsRtlExitFileSystem();
        }       
    } _SEH2_END;
    
    return Status;
}
