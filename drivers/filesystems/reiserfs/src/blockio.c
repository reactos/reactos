/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             blockio.c
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

typedef struct _RFSD_RW_CONTEXT {
        PIRP        MasterIrp;
        KEVENT      Event;
        BOOLEAN     Wait;
        ULONG       Blocks;
        ULONG       Length;
} RFSD_RW_CONTEXT, *PRFSD_RW_CONTEXT;

#ifdef _PREFAST_
IO_COMPLETION_ROUTINE RfsdReadWriteBlockSyncCompletionRoutine;
IO_COMPLETION_ROUTINE RfsdReadWriteBlockAsyncCompletionRoutine;
IO_COMPLETION_ROUTINE RfsdMediaEjectControlCompletion;
#endif // _PREFAST_

NTSTATUS NTAPI
RfsdReadWriteBlockSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context    );

NTSTATUS NTAPI
RfsdReadWriteBlockAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context    );

NTSTATUS NTAPI
RfsdMediaEjectControlCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt     );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RfsdLockUserBuffer)
#pragma alloc_text(PAGE, RfsdGetUserBuffer)
#pragma alloc_text(PAGE, RfsdReadSync)
#pragma alloc_text(PAGE, RfsdReadDisk)
#pragma alloc_text(PAGE, RfsdDiskIoControl)
#pragma alloc_text(PAGE, RfsdReadWriteBlocks)
#pragma alloc_text(PAGE, RfsdMediaEjectControl)
#pragma alloc_text(PAGE, RfsdDiskShutDown)
#endif

/* FUNCTIONS ***************************************************************/

NTSTATUS
RfsdLockUserBuffer (IN PIRP     Irp,
            IN ULONG            Length,
            IN LOCK_OPERATION   Operation)
{
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(Irp != NULL);
    
    if (Irp->MdlAddress != NULL) {

        return STATUS_SUCCESS;
    }
    
    IoAllocateMdl(Irp->UserBuffer, Length, FALSE, FALSE, Irp);
    
    if (Irp->MdlAddress == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    _SEH2_TRY {

        MmProbeAndLockPages(Irp->MdlAddress, Irp->RequestorMode, Operation);
        
        Status = STATUS_SUCCESS;
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {

        IoFreeMdl(Irp->MdlAddress);
        
        Irp->MdlAddress = NULL;

        DbgBreak();
        
        Status = STATUS_INVALID_USER_BUFFER;
    } _SEH2_END;
    
    return Status;
}

PVOID
RfsdGetUserBuffer (IN PIRP Irp )
{
    PAGED_CODE();

    ASSERT(Irp != NULL);
    
    if (Irp->MdlAddress) {

#if (_WIN32_WINNT >= 0x0500)
        return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
#else
        return MmGetSystemAddressForMdl(Irp->MdlAddress);
#endif
    } else {

        return Irp->UserBuffer;
    }
}

NTSTATUS NTAPI
RfsdReadWriteBlockSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context    )
{
    PRFSD_RW_CONTEXT pContext = (PRFSD_RW_CONTEXT)Context;

    if (!NT_SUCCESS( Irp->IoStatus.Status )) {
		DbgBreak(); // [mark]
        pContext->MasterIrp->IoStatus = Irp->IoStatus;
    }

    IoFreeMdl( Irp->MdlAddress );
    IoFreeIrp( Irp );

    if (InterlockedDecrement(&pContext->Blocks) == 0) {

        pContext->MasterIrp->IoStatus.Information = 0;

        if (NT_SUCCESS(pContext->MasterIrp->IoStatus.Status)) {

            pContext->MasterIrp->IoStatus.Information =
                pContext->Length;
        }

        KeSetEvent( &pContext->Event, 0, FALSE );
    }

    UNREFERENCED_PARAMETER( DeviceObject );

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS NTAPI
RfsdReadWriteBlockAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PRFSD_RW_CONTEXT pContext = (PRFSD_RW_CONTEXT)Context;

    if (!NT_SUCCESS( Irp->IoStatus.Status )) {
		DbgBreak(); // [mark]
        pContext->MasterIrp->IoStatus = Irp->IoStatus;
    }

    if (InterlockedDecrement(&pContext->Blocks) == 0) {

        pContext->MasterIrp->IoStatus.Information = 0;

        if (NT_SUCCESS(pContext->MasterIrp->IoStatus.Status)) {

            pContext->MasterIrp->IoStatus.Information =
                pContext->Length;
        }

        IoMarkIrpPending( pContext->MasterIrp );

        ExFreePool(pContext);
    }

    UNREFERENCED_PARAMETER( DeviceObject );

    return STATUS_SUCCESS;
}

// Looks like this is really just getting an MDL (memory descriptor list) from the MM and doing its business...
// ... but IoCallDriver is what will actually read from the disk device to furnish non-chached or paging IO operations!
// NOTE: It is vital that IoCallDriver use sector-aligned offset and length (otherwise, 0xc000000d = STATUS_INVALID_PARAMETER) will result!
NTSTATUS
RfsdReadWriteBlocks(
    IN PRFSD_IRP_CONTEXT IrpContext,
    IN PRFSD_VCB        Vcb,
    IN PRFSD_BDL        RfsdBDL,		// The block-description list
    IN ULONG            Length,			// Length of data to read
    IN ULONG            Count,			// Count of blocks inside the BDL
    IN BOOLEAN          bVerify )
{
    PMDL                Mdl;
    PIRP                Irp;
    PIRP                MasterIrp = IrpContext->Irp;
    PIO_STACK_LOCATION  IrpSp;
    NTSTATUS            Status = STATUS_SUCCESS;
    PRFSD_RW_CONTEXT    pContext = NULL;
    ULONG               i;
    BOOLEAN             bBugCheck = FALSE;

    PAGED_CODE();

    ASSERT(MasterIrp);

    _SEH2_TRY {

        pContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(RFSD_RW_CONTEXT), RFSD_POOL_TAG);

        if (!pContext) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

        RtlZeroMemory(pContext, sizeof(RFSD_RW_CONTEXT));

        pContext->Wait = IrpContext->IsSynchronous;
        pContext->MasterIrp = MasterIrp;
        pContext->Blocks = Count;
        pContext->Length = 0;

        if (pContext->Wait) {
            KeInitializeEvent(&(pContext->Event), NotificationEvent, FALSE);
        }

        for (i = 0; i < Count; i++) {

            Irp = IoMakeAssociatedIrp( 
                        MasterIrp,
                        (CCHAR)(Vcb->TargetDeviceObject->StackSize + 1) );

            if (!Irp) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
#ifdef __REACTOS__
                ExFreePool(pContext);
                pContext = NULL;
#endif
                _SEH2_LEAVE;
            }

            Mdl = IoAllocateMdl( (PCHAR)MasterIrp->UserBuffer +
                    RfsdBDL[i].Offset,
                    RfsdBDL[i].Length,
                    FALSE,
                    FALSE,
                    Irp );

            if (!Mdl)  {
                Status = STATUS_INSUFFICIENT_RESOURCES;
#ifdef __REACTOS__
                ExFreePool(pContext);
                pContext = NULL;
#endif
                _SEH2_LEAVE;
            }
            
            IoBuildPartialMdl( MasterIrp->MdlAddress,
                        Mdl,
                        (PCHAR)MasterIrp->UserBuffer + RfsdBDL[i].Offset,
                        RfsdBDL[i].Length );
                
            IoSetNextIrpStackLocation( Irp );
            IrpSp = IoGetCurrentIrpStackLocation( Irp );
                
            
            IrpSp->MajorFunction = IrpContext->MajorFunction;
            IrpSp->Parameters.Read.Length = RfsdBDL[i].Length;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = RfsdBDL[i].Lba;

            IoSetCompletionRoutine(
                    Irp,
                    ((IrpContext->IsSynchronous) ?
						(&RfsdReadWriteBlockSyncCompletionRoutine) :
						(&RfsdReadWriteBlockAsyncCompletionRoutine)),
                    (PVOID) pContext,
                    TRUE,
                    TRUE,
                    TRUE );

            IrpSp = IoGetNextIrpStackLocation( Irp );

            IrpSp->MajorFunction = IrpContext->MajorFunction;
            IrpSp->Parameters.Read.Length = RfsdBDL[i].Length;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = RfsdBDL[i].Lba;

            if (bVerify) {
                SetFlag( IrpSp->Flags, SL_OVERRIDE_VERIFY_VOLUME );
            }

            RfsdBDL[i].Irp = Irp;
        }

        MasterIrp->AssociatedIrp.IrpCount = Count;

        if (IrpContext->IsSynchronous) {
            MasterIrp->AssociatedIrp.IrpCount += 1;
        }

        pContext->Length = Length;

        bBugCheck = TRUE;

        for (i = 0; i < Count; i++) {
            Status = IoCallDriver ( Vcb->TargetDeviceObject,
                                    RfsdBDL[i].Irp);
        }

        if (IrpContext->IsSynchronous) {
            KeWaitForSingleObject( &(pContext->Event),
                                   Executive, KernelMode, FALSE, NULL );

            KeClearEvent( &(pContext->Event) );
        }

    } _SEH2_FINALLY {

        if (IrpContext->IsSynchronous) {
            if (MasterIrp)
                Status = MasterIrp->IoStatus.Status;

            if (pContext)
                ExFreePool(pContext);

        } else {
            IrpContext->Irp = NULL;
            Status = STATUS_PENDING;
        }

        if (_SEH2_AbnormalTermination()) {
            if (bBugCheck) {
                RfsdBugCheck(RFSD_BUGCHK_BLOCK, 0, 0, 0);
            }

            for (i = 0; i < Count; i++)  {
                if (RfsdBDL[i].Irp != NULL ) {
                    if ( RfsdBDL[i].Irp->MdlAddress != NULL ) {
                        IoFreeMdl( RfsdBDL[i].Irp->MdlAddress );
                    }

                    IoFreeIrp( RfsdBDL[i].Irp );
                }
            }
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
RfsdReadSync(
    IN PRFSD_VCB        Vcb,
    IN ULONGLONG        Offset,
    IN ULONG            Length,
    OUT PVOID           Buffer,
    BOOLEAN             bVerify
    )
{
    PKEVENT         Event = NULL;
    PIRP            Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS        Status;

    PAGED_CODE();

    ASSERT(Vcb != NULL);
    ASSERT(Vcb->TargetDeviceObject != NULL);
    ASSERT(Buffer != NULL);

    _SEH2_TRY {
    
        Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), RFSD_POOL_TAG);

        if (NULL == Event) {
            _SEH2_LEAVE;
        }

        KeInitializeEvent(Event, NotificationEvent, FALSE);

        Irp = IoBuildSynchronousFsdRequest(
            IRP_MJ_READ,
            Vcb->TargetDeviceObject,
            Buffer,
            Length,
            (PLARGE_INTEGER)(&Offset),
            Event,
            &IoStatus
            );

        if (!Irp) {            
			Status = STATUS_INSUFFICIENT_RESOURCES;
			_SEH2_LEAVE;
        }

        if (bVerify) {
            SetFlag( IoGetNextIrpStackLocation(Irp)->Flags, 
                     SL_OVERRIDE_VERIFY_VOLUME );
        }

        Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject(
                Event,
                Suspended,
                KernelMode,
                FALSE,
                NULL
                );

            Status = IoStatus.Status;
        }

    } _SEH2_FINALLY {

        if (Event) {
            ExFreePool(Event);
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
RfsdReadDisk(
         IN PRFSD_VCB   Vcb,
         IN ULONGLONG   Offset,			// Byte offset (relative to disk) to read from (need not be sector-aligned!)
         IN ULONG       Size,
         IN OUT	PVOID   Buffer,			
         IN BOOLEAN     bVerify )		// True if the volume should be verified before reading
{
    NTSTATUS    Status;
    PUCHAR      Buf;
    ULONG       Length;
    ULONGLONG   Lba;

    PAGED_CODE();

	// Align the offset and length to the sector boundaries
    Lba = Offset & (~((ULONGLONG)SECTOR_SIZE - 1));
    Length = (ULONG)(Size + Offset + SECTOR_SIZE - 1 - Lba) &
             (~((ULONG)SECTOR_SIZE - 1));

	// Allocate a temporary buffer to read the sector-aligned data into
    Buf = ExAllocatePoolWithTag(PagedPool, Length, RFSD_POOL_TAG);
    if (!Buf) {
        RfsdPrint((DBG_ERROR, "RfsdReadDisk: no enough memory.\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;

        goto errorout;
    }

	// Read the data
    Status = RfsdReadSync(  Vcb, 
                            Lba,
                            Length,
                            Buf,
                            FALSE );

    if (!NT_SUCCESS(Status)) {
        RfsdPrint((DBG_ERROR, "RfsdReadDisk: Read Block Device error.\n"));

        goto errorout;
    }

	// Copy the requested data into the user's buffer
    RtlCopyMemory(Buffer, &Buf[Offset - Lba], Size);

errorout:

    if (Buf)
        ExFreePool(Buf);

    return Status;
}

NTSTATUS 
RfsdDiskIoControl (
           IN PDEVICE_OBJECT   DeviceObject,
           IN ULONG            IoctlCode,
           IN PVOID            InputBuffer,
           IN ULONG            InputBufferSize,
           IN OUT PVOID        OutputBuffer,
           IN OUT PULONG       OutputBufferSize)
{
    ULONG           OutBufferSize = 0;
    KEVENT          Event;
    PIRP            Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS        Status;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);
    
    if (OutputBufferSize)
    {
        OutBufferSize = *OutputBufferSize;
    }
    
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    
    Irp = IoBuildDeviceIoControlRequest(
        IoctlCode,
        DeviceObject,
        InputBuffer,
        InputBufferSize,
        OutputBuffer,
        OutBufferSize,
        FALSE,
        &Event,
        &IoStatus
        );
    
    if (Irp == NULL) {
        RfsdPrint((DBG_ERROR, "RfsdDiskIoControl: Building IRQ error!\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = IoCallDriver(DeviceObject, Irp);
    
    if (Status == STATUS_PENDING)  {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }
    
    if (OutputBufferSize) {
        *OutputBufferSize = (ULONG) IoStatus.Information;
    }
    
    return Status;
}

NTSTATUS NTAPI
RfsdMediaEjectControlCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )
{
    PKEVENT Event = (PKEVENT)Contxt;

    KeSetEvent( Event, 0, FALSE );

    UNREFERENCED_PARAMETER( DeviceObject );

    return STATUS_SUCCESS;
}

__drv_mustHoldCriticalRegion
VOID
RfsdMediaEjectControl (
    IN PRFSD_IRP_CONTEXT IrpContext,
    IN PRFSD_VCB Vcb,
    IN BOOLEAN bPrevent
    )
{
    PIRP                    Irp;
    KEVENT                  Event;
    NTSTATUS                Status;
    PREVENT_MEDIA_REMOVAL   Prevent;
    IO_STATUS_BLOCK         IoStatus;

    PAGED_CODE();

    ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            TRUE            );

    if (bPrevent != IsFlagOn(Vcb->Flags, VCB_REMOVAL_PREVENTED)) {
        if (bPrevent) {
            SetFlag(Vcb->Flags, VCB_REMOVAL_PREVENTED);
        } else {
            ClearFlag(Vcb->Flags, VCB_REMOVAL_PREVENTED);
        }
    }

    ExReleaseResourceForThreadLite(
            &Vcb->MainResource,
            ExGetCurrentResourceThread());

    Prevent.PreventMediaRemoval = bPrevent;

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    Irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_MEDIA_REMOVAL,
                                         Vcb->TargetDeviceObject,
                                         &Prevent,
                                         sizeof(PREVENT_MEDIA_REMOVAL),
                                         NULL,
                                         0,
                                         FALSE,
                                         NULL,
                                         &IoStatus );

    if (Irp != NULL) {
        IoSetCompletionRoutine( Irp,
                                RfsdMediaEjectControlCompletion,
                                &Event,
                                TRUE,
                                TRUE,
                                TRUE );

        Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

        if (Status == STATUS_PENDING) {
            Status = KeWaitForSingleObject( &Event,
                                            Executive,
                                            KernelMode,
                                            FALSE,
                                            NULL );
        }
    }
}

NTSTATUS
RfsdDiskShutDown(PRFSD_VCB Vcb)
{
    PIRP                Irp;
    KEVENT              Event;
    NTSTATUS            Status;
    IO_STATUS_BLOCK     IoStatus;

    PAGED_CODE();

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_SHUTDOWN,
                                       Vcb->TargetDeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);

    if (Irp) {
        Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            Status = IoStatus.Status;
        }
    } else  {
        Status = IoStatus.Status;
    }

    return Status;
}
