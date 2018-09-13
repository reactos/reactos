/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    iosubs.c

Abstract:

    This module contains the subroutines for the I/O system.

Author:

    Darryl E. Havens (darrylh) 16-Apr-1989

Environment:

    Kernel mode

Revision History:


--*/

#include "iop.h"

//
// This is the overall system device configuration record.
//

static CONFIGURATION_INFORMATION ConfigurationInformation = {
    0,                                 // DiskCount
    0,                                 // FloppyCount
    0,                                 // CdRomCount
    0,                                 // TapeCount
    0,                                 // ScsiPortCount
    0,                                 // SerialCount
    0,                                 // ParallelCount
    FALSE,                             // Primary ATDISK IO address claimed
    FALSE,                             // Secondary ATDISK IO address claimed
    sizeof(CONFIGURATION_INFORMATION), // Version
    0                                  // MediumChangerCount
};

//
// This value may be overridden by the registry.
//

LOGICAL IoCountOperations = TRUE;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IoAttachDevice)
#pragma alloc_text(PAGE, IoCancelThreadIo)
#pragma alloc_text(PAGE, IoCheckDesiredAccess)
#pragma alloc_text(PAGE, IoCheckEaBufferValidity)
#pragma alloc_text(PAGE, IoCheckFunctionAccess)
#pragma alloc_text(PAGE, IoCheckQuotaBufferValidity)
#pragma alloc_text(PAGE, IoCheckShareAccess)
#pragma alloc_text(PAGE, IoConnectInterrupt)
#pragma alloc_text(PAGE, IoCreateController)
#pragma alloc_text(PAGE, IoCreateDevice)
#pragma alloc_text(PAGE, IoCreateFile)
#pragma alloc_text(PAGE, IoCreateNotificationEvent)
#pragma alloc_text(PAGE, IoCreateStreamFileObject)
#pragma alloc_text(PAGE, IoCreateSymbolicLink)
#pragma alloc_text(PAGE, IoCreateSynchronizationEvent)
#pragma alloc_text(PAGE, IoCreateUnprotectedSymbolicLink)
#pragma alloc_text(PAGE, IoDeleteController)
#pragma alloc_text(PAGE, IoDeleteSymbolicLink)
#pragma alloc_text(PAGE, IoDisconnectInterrupt)
#pragma alloc_text(PAGE, IoEnqueueIrp)
#pragma alloc_text(PAGE, IoFastQueryNetworkAttributes)
#pragma alloc_text(PAGE, IoGetConfigurationInformation)
#pragma alloc_text(PAGE, IoGetDeviceObjectPointer)
#pragma alloc_text(PAGE, IoInitializeTimer)
#pragma alloc_text(PAGE, IoQueryFileInformation)
#pragma alloc_text(PAGE, IoQueryVolumeInformation)
#pragma alloc_text(PAGE, IoPageFileCreated)
#pragma alloc_text(PAGE, IoRegisterBootDriverReinitialization)
#pragma alloc_text(PAGE, IoRegisterDriverReinitialization)
#pragma alloc_text(PAGE, IoRegisterFileSystem)
#pragma alloc_text(PAGE, IoRegisterFsRegistrationChange)
#pragma alloc_text(PAGE, IoRegisterLastChanceShutdownNotification)
#pragma alloc_text(PAGE, IoRegisterShutdownNotification)
#pragma alloc_text(PAGE, IoRemoveShareAccess)
#pragma alloc_text(PAGE, IoSetInformation)
#pragma alloc_text(PAGE, IoSetShareAccess)
#pragma alloc_text(PAGE, IoUnregisterFileSystem)
#pragma alloc_text(PAGE, IoUnregisterFsRegistrationChange)
#pragma alloc_text(PAGE, IoUpdateShareAccess)
#pragma alloc_text(PAGE, IoVerifyVolume)
#pragma alloc_text(PAGE, IopCreateVpb)
#pragma alloc_text(PAGE, IoCancelFileOpen)
#pragma alloc_text(PAGELK, IoShutdownSystem)
#pragma alloc_text(PAGELK, IoUnregisterShutdownNotification)
#endif

VOID
IoAcquireCancelSpinLock(
    OUT PKIRQL Irql
    )

/*++

Routine Description:

    This routine is invoked to acquire the cancel spin lock.  This spin lock
    must be acquired before setting the address of a cancel routine in an
    IRP.

Arguments:

    Irql - Address of a variable to receive the old IRQL.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;

    //
    // Simply acquire the cancel spin lock and return.
    //

    ExAcquireSpinLock( &IopCancelSpinLock, &oldIrql );
    *Irql = oldIrql;
}

VOID
IoAcquireVpbSpinLock(
    OUT PKIRQL Irql
    )

/*++

Routine Description:

    This routine is invoked to acquire the Volume Parameter Block (VPB) spin
    lock.  This spin lock must be acquired before accessing the mount flag,
    reference count, and device object fields of a VPB.

Arguments:

    Irql - Address of a variable to receive the old IRQL.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;

    //
    // Simply acquire the IopLoadFileSystemDriverVPB spin lock and return.
    //

    ExAcquireSpinLock( &IopVpbSpinLock, &oldIrql );
    *Irql = oldIrql;
}


NTSTATUS
IoAllocateAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine allocates the adapter channel specified by the adapter object.
    This is accomplished by calling HalAllocateAdapterChannel which does all of
    the work.

Arguments:

    AdapterObject - Pointer to the adapter control object to allocate to the
        driver.

    DeviceObject - Pointer to the driver's device object that represents the
        device allocating the adapter.

    NumberOfMapRegisters - The number of map registers that are to be allocated
        from the channel, if any.

    ExecutionRoutine - The address of the driver's execution routine that is
        invoked once the adapter channel (and possibly map registers) have been
        allocated.

    Context - An untyped longword context parameter passed to the driver's
        execution routine.

Return Value:

    Returns STATUS_SUCESS unless too many map registers are requested.

Notes:

    Note that this routine MUST be invoked at DISPATCH_LEVEL or above.

--*/

{
#if !defined(NO_LEGACY_DRIVERS)
    PWAIT_CONTEXT_BLOCK wcb;

    wcb = &DeviceObject->Queue.Wcb;

    wcb->DeviceObject = DeviceObject;
    wcb->CurrentIrp = DeviceObject->CurrentIrp;
    wcb->DeviceContext = Context;

    return( HalAllocateAdapterChannel( AdapterObject,
                                       wcb,
                                       NumberOfMapRegisters,
                                       ExecutionRoutine ) );
#else
    return( (*((PDMA_ADAPTER)AdapterObject)->DmaOperations->
             AllocateAdapterChannel)( (PDMA_ADAPTER)AdapterObject,
                                      DeviceObject,
                                      NumberOfMapRegisters,
                                      ExecutionRoutine,
                                      Context) );

#endif // NO_LEGACY_DRIVERS
}


VOID
IoAllocateController(
    IN PCONTROLLER_OBJECT ControllerObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine allocates the controller specified by the controller object.
    This is accomplished by placing the device object of the driver that wants
    to allocate the controller on the controller's queue.  If the queue is
    already "busy", then the controller has already been allocated, so the
    device object is simply placed onto the queue and waits until the controller
    becomes free.

    Once the controller becomes free (or if it already is), then the driver's
    execution routine is invoked.

Arguments:

    ControllerObject - Pointer to the controller object to allocate to the
        driver.

    DeviceObject - Pointer to the driver's device object that represents the
        device allocating the controller.

    ExecutionRoutine - The address of the driver's execution routine that is
        invoked once the controller has been allocated.

    Context - An untyped longword context parameter passed to the driver's
        execution routine.

Return Value:

    None.

Notes:

    Note that this routine MUST be invoked at DISPATCH_LEVEL or above.

--*/

{
    IO_ALLOCATION_ACTION action;

    //
    // Initialize the device object's wait context block in case this device
    // must wait before being able to allocate the controller.
    //

    DeviceObject->Queue.Wcb.DeviceRoutine = ExecutionRoutine;
    DeviceObject->Queue.Wcb.DeviceContext = Context;

    //
    // Allocate the controller object for this particular device.  If the
    // controller cannot be allocated because it has already been allocated
    // to another device, then return to the caller now;  otherwise,
    // continue.
    //

    if (!KeInsertDeviceQueue( &ControllerObject->DeviceWaitQueue,
                              &DeviceObject->Queue.Wcb.WaitQueueEntry )) {

        //
        // The controller was not busy so it has been allocated.  Simply
        // invoke the driver's execution routine now.
        //

        action = ExecutionRoutine( DeviceObject,
                                   DeviceObject->CurrentIrp,
                                   0,
                                   Context );

        //
        // If the driver would like to have the controller deallocated,
        // then deallocate it now.
        //

        if (action == DeallocateObject) {
            IoFreeController( ControllerObject );
        }
    }
}

NTSTATUS
IoAllocateDriverObjectExtension(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID ClientIdentificationAddress,
    IN ULONG DriverObjectExtensionSize,
    OUT PVOID *DriverObjectExtension
    )

/*++

Routine Description:
    This routine allocates per driver storage for helper or class drivers
    which may support several different mini-drivers.  The storage is tagged
    with a client identification address which is used to retrieve a pointer
    to the storage.  The client id must be unique.

    The allocated storage is freed when the driver object is deleted.

Arguments:

    DriverObject - The driver object to which the extension is to be
        associated.

    ClientIdentificationAddress - Unique identifier used to retrieve the
        extension.

    DriverObjectExtensionSize - Specifies the size in bytes of the extension.

    DriverObjectExtension - Returns a pointer to the allocated extension.

Return Value:

    Returns the status of the operation.  Failure cases are
    STATUS_INSUFFICIENT_RESOURCES and STATUS_OBJECT_NAME_COLLISION.

--*/

{
    KIRQL irql;
    BOOLEAN inserted = FALSE;
    PIO_CLIENT_EXTENSION extension;
    PIO_CLIENT_EXTENSION newExtension;

    *DriverObjectExtension = NULL;

    newExtension = ExAllocatePoolWithTag( NonPagedPool,
                                          DriverObjectExtensionSize +
                                          sizeof( IO_CLIENT_EXTENSION ),
                                          'virD');

    if (newExtension == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlZeroMemory( newExtension,
                    DriverObjectExtensionSize +
                    sizeof( IO_CLIENT_EXTENSION )
                    );

    newExtension->ClientIdentificationAddress = ClientIdentificationAddress;

    ExAcquireFastLock( &IopDatabaseLock, &irql );
    extension = DriverObject->DriverExtension->ClientDriverExtension;
    while (extension != NULL) {

        if (extension->ClientIdentificationAddress == ClientIdentificationAddress) {
            break;
        }

        extension = extension->NextExtension;
    }

    if (extension == NULL) {

        //
        // The client id does not exist.  Insert the new extension in the
        // list.
        //

        newExtension->NextExtension =
            DriverObject->DriverExtension->ClientDriverExtension;
        DriverObject->DriverExtension->ClientDriverExtension = newExtension;
        inserted = TRUE;
    }

    ExReleaseFastLock( &IopDatabaseLock, irql );

    if (!inserted) {
        ExFreePool( newExtension );
        return(STATUS_OBJECT_NAME_COLLISION);
    }

    //
    // Return a pointer to the client's data area.
    //

    *DriverObjectExtension = newExtension + 1;
    return(STATUS_SUCCESS);
}

PVOID
IoAllocateErrorLogEntry(
    IN PVOID IoObject,
    IN UCHAR EntrySize
    )

/*++

Routine Description:

    This routine allocates and initializes an error log entry buffer and returns
    a pointer to the data entry portion of the buffer.

Arguments:

    IoObject - Pointer to driver's device object or driver object.

    EntrySize - Size of entry to be allocated, in bytes.  The maximum size is
        specified by ERROR_LOG_MAXIMUM_SIZE.

Return Value:

    Pointer to the body of the allocated error log entry, or NULL, if there are
    no free entries in the system.

Note:

    This routine assumes that the caller wants an error log entry within the
    bounds of the maximum size.

--*/

{
    PDEVICE_OBJECT deviceObject;
    PDRIVER_OBJECT driverObject;

    //
    // Make sure that a I/O object pointer was passed in.
    //

    if (IoObject == NULL) {
        return(NULL);
    }

    //
    // Assume for a moment this is a device object.
    //

    deviceObject = IoObject;

    //
    // Determine if this is a driver object or device object or if we
    // are allocating a generic error log entry.   This is determined
    // from the Type field of the object passed in.
    //

    if (deviceObject->Type == IO_TYPE_DEVICE) {

        driverObject = deviceObject->DriverObject;

    } else if (deviceObject->Type == IO_TYPE_DRIVER) {

        driverObject = (PDRIVER_OBJECT) IoObject;
        deviceObject = NULL;

    } else {

        return(NULL);

    }

    return (IopAllocateErrorLogEntry(
                deviceObject,
                driverObject,
                EntrySize));

}

PVOID
IoAllocateGenericErrorLogEntry(
    IN  UCHAR   EntrySize
    )

/*++

Routine Description:

    This routine allocates and initializes an error log entry buffer and returns
    a pointer to the data entry portion of the buffer. It's expected to be
    called from inside the kernel where there may not be a driver object
    or a device object.

Arguments:


    EntrySize - Size of entry to be allocated, in bytes.  The maximum size is
        specified by ERROR_LOG_MAXIMUM_SIZE.

Return Value:

    Pointer to the body of the allocated error log entry, or NULL, if there are
    no free entries in the system.

Note:

    This routine assumes that the caller wants an error log entry within the
    bounds of the maximum size.

--*/

{
    return(IopAllocateErrorLogEntry(NULL, NULL, EntrySize));
}

PVOID
IopAllocateErrorLogEntry(
    IN PDEVICE_OBJECT deviceObject,
    IN PDRIVER_OBJECT driverObject,
    IN UCHAR EntrySize
    )
{
    PERROR_LOG_ENTRY elEntry;
    PVOID returnValue;
    NTSTATUS status;
    KIRQL oldIrql;
    ULONG size;

    //
    // Make sure the packet is large enough but not too large.
    //

    if (EntrySize < sizeof(IO_ERROR_LOG_PACKET) ||
        EntrySize > ERROR_LOG_MAXIMUM_SIZE) {

        return(NULL);
    }

    //
    // Round entry size to a PVOID size boundary.
    //

    EntrySize = (UCHAR) ((EntrySize + sizeof(PVOID) - 1) & ~(sizeof(PVOID) - 1));

    //
    // Calculate the size of the entry needed.
    //

    size = sizeof(ERROR_LOG_ENTRY) + EntrySize;

    //
    // Make sure that there are not too many outstanding packets.
    //

    ExAcquireSpinLock(&IopErrorLogAllocationLock, &oldIrql);

    try{

        if (IopErrorLogAllocation > IOP_MAXIMUM_LOG_ALLOCATION) {

            //
            // Fail the request.
            //

            return(NULL);
        }

        //
        // Increase the outstanding allocation.
        //

        IopErrorLogAllocation += size;

        //
        // Allocate the packet.
        //

        elEntry = ExAllocatePoolWithTag( NonPagedPool, size, 'rEoI' );

        if (elEntry == NULL) {

            //
            // Drop the allocation and return.
            //

            IopErrorLogAllocation -= size;

            return(NULL);
        }

        //
        // Reference the device object and driver object. So they don't
        // go away before the name gets pulled out.
        //

        if (deviceObject != NULL) {

            ObReferenceObject( deviceObject );
        }

        if (driverObject != NULL) {

            ObReferenceObject( driverObject );
        }

        //
        // Initialize the fields.
        //

        RtlZeroMemory(elEntry, size);

        elEntry->Type = IO_TYPE_ERROR_LOG;
        elEntry->Size = (USHORT) size;
        elEntry->DeviceObject = deviceObject;
        elEntry->DriverObject = driverObject;

        returnValue = elEntry+1;

    } finally {
        ExReleaseSpinLock(&IopErrorLogAllocationLock, oldIrql);
    }                   

    return returnValue;
}

PIRP
IoAllocateIrp(
    IN CCHAR StackSize,
    IN BOOLEAN ChargeQuota
    )
{
    return (pIoAllocateIrp(StackSize, ChargeQuota));
}


PIRP
IopAllocateIrpPrivate(
    IN CCHAR StackSize,
    IN BOOLEAN ChargeQuota
    )

/*++

Routine Description:

    This routine allocates an I/O Request Packet from the system nonpaged pool.
    The packet will be allocated to contain StackSize stack locations.  The IRP
    will also be initialized.

Arguments:

    StackSize - Specifies the maximum number of stack locations required.

    ChargeQuota - Specifies whether quota should be charged against thread.

Return Value:

    The function value is the address of the allocated/initialized IRP,
    or NULL if one could not be allocated.

--*/

{
    USHORT allocateSize;
    UCHAR fixedSize;
    PIRP irp;
    UCHAR lookasideAllocation;
    PNPAGED_LOOKASIDE_LIST lookasideList;
    UCHAR mustSucceed;
    PP_NPAGED_LOOKASIDE_NUMBER number;
    USHORT packetSize;
    PKPRCB prcb;

    //
    // If the size of the packet required is less than or equal to those on
    // the lookaside lists, then attempt to allocate the packet from the
    // lookaside lists.
    //

    irp = NULL;
    fixedSize = 0;
    mustSucceed = 0;
    packetSize = IoSizeOfIrp(StackSize);
    allocateSize = packetSize;
    if ((StackSize <= (CCHAR)IopLargeIrpStackLocations) &&
        ((ChargeQuota == FALSE) || (IopLookasideIrpFloat < IopLookasideIrpLimit))) {
        fixedSize = IRP_ALLOCATED_FIXED_SIZE;
        number = LookasideSmallIrpList;
        if (StackSize != 1) {
            allocateSize = IoSizeOfIrp((CCHAR)IopLargeIrpStackLocations);
            number = LookasideLargeIrpList;
        }

        prcb = KeGetCurrentPrcb();
        lookasideList = prcb->PPLookasideList[number].P;
        lookasideList->L.TotalAllocates += 1;
        irp = (PIRP)ExInterlockedPopEntrySList(&lookasideList->L.ListHead,
                                               &lookasideList->Lock);

        if (irp == NULL) {
            lookasideList->L.AllocateMisses += 1;
            lookasideList = prcb->PPLookasideList[number].L;
            lookasideList->L.TotalAllocates += 1;
            irp = (PIRP)ExInterlockedPopEntrySList(&lookasideList->L.ListHead,
                                                   &lookasideList->Lock);
        }
    }

    //
    // If an IRP was not allocated from the lookaside list, then allocate
    // the packet from nonpaged pool and charge quota if requested.
    //

    lookasideAllocation = 0;
    if (!irp) {
        if (fixedSize != 0) {
            lookasideList->L.AllocateMisses += 1;
        }

        //
        // There are no free packets on the lookaside list, or the packet is
        // too large to be allocated from one of the lists, so it must be
        // allocated from nonpaged pool. If quota is to be charged, charge it
        // against the current process. Otherwise, allocate the pool normally.
        //

        if (ChargeQuota) {
            try {
                irp = ExAllocatePoolWithQuotaTag(NonPagedPool, allocateSize,' prI');

            } except(EXCEPTION_EXECUTE_HANDLER) {
                NOTHING;
            }

        } else {

            //
            // Attempt to allocate the pool from non-paged pool.  If this
            // fails, and the caller's previous mode was kernel then allocate
            // the pool as must succeed.
            //

            irp = ExAllocatePoolWithTag(NonPagedPool, allocateSize, ' prI');
            if (!irp) {
                mustSucceed = IRP_ALLOCATED_MUST_SUCCEED;
                if (KeGetPreviousMode() == KernelMode ) {
                    irp = ExAllocatePoolWithTag(NonPagedPoolMustSucceed,
                                                allocateSize,
                                                ' prI');
                }
            }
        }

        if (!irp) {
            return NULL;
        }

    } else {
        if (ChargeQuota != FALSE) {
            lookasideAllocation = IRP_LOOKASIDE_ALLOCATION;
            InterlockedIncrement( &IopLookasideIrpFloat );
        }
        ChargeQuota = FALSE;
    }

    //
    // Initialize the packet.
    //

    IopInitializeIrp(irp, packetSize, StackSize);
    irp->AllocationFlags = (fixedSize | lookasideAllocation | mustSucceed);
    if (ChargeQuota) {
        irp->AllocationFlags |= IRP_QUOTA_CHARGED;
    }

    return irp;
}

PMDL
IoAllocateMdl(
    IN PVOID VirtualAddress,
    IN ULONG Length,
    IN BOOLEAN SecondaryBuffer,
    IN BOOLEAN ChargeQuota,
    IN OUT PIRP Irp OPTIONAL
    )

/*++

Routine Description:

    This routine allocates a Memory Descriptor List (MDL) large enough to map
    the buffer specified by the VirtualAddress and Length parameters.  If the
    routine is given a pointer to an Irp, then it will chain the MDL to the
    IRP in the appropriate way.

    If this routine is not given a pointer to an Irp it is up to the caller to
    set the MDL address in the IRP that the MDL is being allocated for.

    Note that the header information of the MDL will also be initialized.

Arguments:

    VirtualAddress - Starting virtual address of the buffer to be mapped.

    Length - Length, in bytes, of the buffer to be mapped.

    SecondaryBuffer - Indicates whether this is a chained buffer.

    ChargeQuota - Indicates whether quota should be charged if MDL allocated.

        N.B. This parameter is ignored.

    Irp - Optional pointer to IRP that MDL is being allocated for.

Return Value:

    A pointer to the allocated MDL, or NULL if one could not be allocated.
    Note that if no MDL could be allocated because there was not enough quota,
    then it is up to the caller to catch the raised exception.

--*/

{
    ULONG allocateSize;
    USHORT fixedSize;
    PMDL mdl;
    USHORT mustSucceed;
    ULONG size;
    PMDL tmpMdlPtr;

    ASSERT(Length);

    //
    // If the requested length is greater than 2Gb, then we're not going
    // to be able to map the memory, so fail the request.
    //

    if (Length & 0x80000000) {
        return NULL;
    }

    //
    // Allocate an MDL from the lookaside list or pool as appropriate.
    //

    mdl = NULL;
    fixedSize = 0;
    mustSucceed = 0;
    size = COMPUTE_PAGES_SPANNED(VirtualAddress, Length);
    if (size > IOP_FIXED_SIZE_MDL_PFNS) {
        allocateSize = sizeof(MDL) + (sizeof(PFN_NUMBER) * size);
        if (allocateSize > MAXUSHORT) {
            return NULL;
        }

    } else {
        fixedSize = MDL_ALLOCATED_FIXED_SIZE;
        allocateSize =  sizeof(MDL) + (sizeof(PFN_NUMBER) * IOP_FIXED_SIZE_MDL_PFNS);
        mdl = (PMDL)ExAllocateFromPPNPagedLookasideList(LookasideMdlList);
    }

    if (!mdl) {
        mdl = ExAllocatePoolWithTag(NonPagedPool, allocateSize, ' ldM');
        if (!mdl) {
            if (KeGetPreviousMode() == KernelMode) {
                mustSucceed = MDL_ALLOCATED_MUST_SUCCEED;
                mdl = ExAllocatePoolWithTag(NonPagedPoolMustSucceed,
                                            allocateSize,
                                            ' ldM' );
            }

            if (!mdl) {
                return NULL;
            }
        }
    }

    //
    // Now fill in the header of the MDL.
    //

    MmInitializeMdl(mdl, VirtualAddress, Length);
    mdl->MdlFlags |= (fixedSize | mustSucceed);

    //
    // Finally, if an IRP was specified, store the address of the MDL
    // based on whether or not this is a secondary buffer.
    //

    if (Irp) {
        if (!SecondaryBuffer) {
            Irp->MdlAddress = mdl;

        } else {
            tmpMdlPtr = Irp->MdlAddress;
            while (tmpMdlPtr->Next != NULL) {
                tmpMdlPtr = tmpMdlPtr->Next;
            }

            tmpMdlPtr->Next = mdl;
        }
    }

    return mdl;
}

NTSTATUS
IoAsynchronousPageWrite(
    IN PFILE_OBJECT FileObject,
    IN PMDL MemoryDescriptorList,
    IN PLARGE_INTEGER StartingOffset,
    IN PIO_APC_ROUTINE ApcRoutine,
    IN PVOID ApcContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PIRP *Irp OPTIONAL
    )

/*++

Routine Description:

    This routine provides a special, fast interface for the Modified Page Writer
    (MPW) to write pages to the disk quickly and with very little overhead.  All
    of the special handling for this request is recognized by setting the
    IRP_PAGING_IO flag in the IRP flags word.

Arguments:

    FileObject - A pointer to a referenced file object describing which file
        the write should be performed on.

    MemoryDescriptorList - An MDL which describes the physical pages that the
        pages should be written to the disk.  All of the pages have been locked
        in memory.  The MDL also describes the length of the write operation.

    StartingOffset - Pointer to the offset in the file from which the write
        should take place.

    ApcRoutine - The address of a kernel APC routine which should be executed
        after the write operation has completed.

    ApcContext - A context parameter which should be supplied to the kernel APC
        routine when it executes.

    IoStatusBlock - A pointer to the I/O status block in which the final status
        and information should be stored.

    Irp - If specified, allows the caller to squirrel away a pointer to the Irp.

Return Value:

    The function value is the final status of the queue request to the I/O
    system subcomponents.


--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT deviceObject;
    NTSTATUS status;

    //
    // Begin by getting a pointer to the device object that the file resides
    // on.
    //

    deviceObject = IoGetRelatedDeviceObject( FileObject );

    //
    // Allocate an I/O Request Packet (IRP) for this out-page operation.
    //

    irp = IoAllocateIrp( deviceObject->StackSize, FALSE );
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  If specified, let the caller know what Irp is responsible for this
    //  transfer.  While this is mainly for debugging purposes, it is
    //  absolutely essential to debug certain types of problems, and is
    //  very cheap, thus is included in the FREE build as well.
    //

    if (ARGUMENT_PRESENT(Irp)) {
        *Irp = irp;
    }

    //
    // Get a pointer to the first stack location in the packet.  This location
    // will be used to pass the function codes and parameters to the first
    // driver.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    //
    // Fill in the IRP according to this request.
    //

    irp->MdlAddress = MemoryDescriptorList;
    irp->Flags = IRP_PAGING_IO | IRP_NOCACHE;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->UserBuffer = (PVOID) ((PCHAR) MemoryDescriptorList->StartVa + MemoryDescriptorList->ByteOffset);
    irp->RequestorMode = KernelMode;
    irp->UserIosb = IoStatusBlock;
    irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

    //
    // Fill in the normal write parameters.
    //

    irpSp->MajorFunction = IRP_MJ_WRITE;
    irpSp->Parameters.Write.Length = MemoryDescriptorList->ByteCount;
    irpSp->Parameters.Write.ByteOffset = *StartingOffset;
    irpSp->FileObject = FileObject;

    //
    // Queue the packet to the appropriate driver based on whether or not there
    // is a VPB associated with the device.
    //

    status = IoCallDriver( deviceObject, irp );

    if (NT_ERROR( status )) {
        IoStatusBlock->Status = status;
        IoStatusBlock->Information = 0;
        ApcRoutine( ApcContext, IoStatusBlock, 0 );
        status = STATUS_PENDING;
    }

    return status;
}


NTSTATUS
IoAttachDevice(
    IN PDEVICE_OBJECT SourceDevice,
    IN PUNICODE_STRING TargetDevice,
    OUT PDEVICE_OBJECT *AttachedDevice
    )

/*++

Routine Description:

    This routine "attaches" a device to another device.  That is, it associates
    the source device to a target device which enables the I/O system to ensure
    that the target device a) exists, and b) cannot be unloaded until the source
    device has detached.  Also, requests bound for the target device are given
    to the source device first, where applicable.

Arguments:

    SourceDevice - Pointer to device object to be attached to the target.

    TargetDevice - Supplies the name of the target device to which the attach
        is to occur.

    AttachedDevice - Returns a pointer to the device to which the attach
        occurred.  This is the device object that the source driver should
        use to communicate with the target driver.

Return Value:

    The function value is the final status of the operation.

--*/

{
    NTSTATUS status;
    PDEVICE_OBJECT targetDevice;
    PFILE_OBJECT fileObject;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatus;

    PAGED_CODE();

    //
    // Attempt to open the target device for attach access.  This ensures that
    // the device itself will be opened, with all of the special considerations
    // thereof.
    //

    InitializeObjectAttributes( &objectAttributes,
                                TargetDevice,
                                0,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ZwOpenFile( &fileHandle,
                         FILE_READ_ATTRIBUTES,
                         &objectAttributes,
                         &ioStatus,
                         0,
                         FILE_NON_DIRECTORY_FILE | IO_ATTACH_DEVICE_API );

    if (NT_SUCCESS( status )) {

        //
        // The open operation was successful.  Dereference the file handle
        // and obtain a pointer to the device object for the handle.
        //

        status = ObReferenceObjectByHandle( fileHandle,
                                            0,
                                            IoFileObjectType,
                                            KernelMode,
                                            (PVOID *) &fileObject,
                                            NULL );
        if (NT_SUCCESS( status )) {

            //
            // Get a pointer to the device object for this file, and close
            // the handle.
            //

            targetDevice = IoGetRelatedDeviceObject( fileObject );
            (VOID) ZwClose( fileHandle );

        } else {

            return status;
        }

    } else {

        return status;

    }

    //
    // Set the attached device pointer so that the driver being attached to
    // cannot unload until the detach occurs, and so that attempts to open the
    // device object go through the attached driver.  Note that the reference
    // count is not incremented since exclusive drivers can only be opened once
    // and this would count as an open.  At that point, both device objects
    // would become useless.
    //

    *AttachedDevice = IoAttachDeviceToDeviceStack( SourceDevice, targetDevice );
    if(!*AttachedDevice) {
        status = STATUS_NO_SUCH_DEVICE;
    }

    //
    // Finally, dereference the file object.  This decrements the reference
    // count for the target device so that when the detach occurs the device
    // can go away if necessary.
    //

    ObDereferenceObject( fileObject );

    //
    // Return the final status of the operation.
    //

    return status;
}

NTSTATUS
IoAttachDeviceByPointer(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
    )

/*++

Routine Description:

    This routine attaches the source device object to the target device
    object.

Arguments:

    SourceDevice - Specifies the device object that is to be attached to
        the target device.

    TargetDevice - Specifies the device object to which the attachment is
        to take place.

Return Value:

    The function value is the final status of the attach operation.

Note:

    THIS FUNCTION IS OBSOLETE!!! see IoAttachDeviceToDeviceStack

--*/

{
    PDEVICE_OBJECT deviceObject;
    NTSTATUS status;

    //
    // Get a pointer to the topmost device object in the stack of devices,
    // beginning with the TargetDevice.
    //

    deviceObject = IoAttachDeviceToDeviceStack( SourceDevice, TargetDevice );
    if( deviceObject == NULL ){
        status = STATUS_NO_SUCH_DEVICE;
    } else {
        status = STATUS_SUCCESS;
    }

    return status;
}

PDEVICE_OBJECT
IoAttachDeviceToDeviceStack(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
    )

/*++

Routine Description:

    This routine attaches the source device object to the target device
    object and returns a pointer to the actual device attached to, if
    successful.

Arguments:

    SourceDevice - Specifies the device object that is to be attached to
        the target device.

    TargetDevice - Specifies the device object to which the attachment is
        to occur.

Return Value:

    If successful, this function returns a pointer to the device object to
    which the attachment actually occurred.

    If unsuccessful, this function returns NULL.  (This could happen if the
    device currently at the top of the attachment chain is being unloaded,
    deleted or initialized.)

--*/

{
    PDEVICE_OBJECT deviceObject;
    PDEVOBJ_EXTENSION sourceExtension;
    KIRQL irql;

    //
    // Retrieve a pointer to the source device object's extension outside
    // of the IopDatabaseLock, since it isn't protected by that.
    //

    sourceExtension = SourceDevice->DeviceObjectExtension;

    //
    // Get a pointer to the topmost device object in the stack of devices,
    // beginning with the TargetDevice, and attach to it.
    //

    ExAcquireFastLock( &IopDatabaseLock, &irql );

    //
    // Tell the Special IRP code the stack has changed. Code that will reexamine
    // the stack takes the database lock, so we can place the call here. This
    // also allows us to assert correct behavoir *before* the stack is built up.
    //

    IOV_ATTACH_DEVICE_TO_DEVICE_STACK(SourceDevice, TargetDevice);

    deviceObject = IoGetAttachedDevice( TargetDevice );

    //
    // Make sure that the SourceDevice object isn't already attached to
    // something else, this is now illegal.
    //

    ASSERT( sourceExtension->AttachedTo == NULL );

    //
    // Now attach to the device, provided that it is not being unloaded,
    // deleted or initializing.
    //

    if (deviceObject->Flags & DO_DEVICE_INITIALIZING ||
        deviceObject->DeviceObjectExtension->ExtensionFlags &
        (DOE_UNLOAD_PENDING | DOE_DELETE_PENDING | DOE_REMOVE_PENDING | DOE_REMOVE_PROCESSED)) {

        //
        // The device currently at the top of the attachment chain is being
        // unloaded, deleted or initialized.
        //

        deviceObject = (PDEVICE_OBJECT) NULL;

    } else {

        //
        // Perform the attachment.  First update the device previously at the
        // top of the attachment chain.
        //
        deviceObject->AttachedDevice = SourceDevice;
        deviceObject->Spare1++;

        //
        // Now update the new top-of-attachment-chain.
        //

        SourceDevice->StackSize = (UCHAR) (deviceObject->StackSize + 1);
        SourceDevice->AlignmentRequirement = deviceObject->AlignmentRequirement;
        SourceDevice->SectorSize = deviceObject->SectorSize;

        if (deviceObject->DeviceObjectExtension->ExtensionFlags & DOE_START_PENDING)  {
            SourceDevice->DeviceObjectExtension->ExtensionFlags |= DOE_START_PENDING;
        }

        //
        // Attachment chain is doubly-linked.
        //

        sourceExtension->AttachedTo = deviceObject;
    }
    ExReleaseFastLock( &IopDatabaseLock, irql );

    return deviceObject;
}

PIRP
IoBuildAsynchronousFsdRequest(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PIO_STATUS_BLOCK IoStatusBlock OPTIONAL
    )

/*++

Routine Description:

    This routine builds an I/O Request Packet (IRP) suitable for a File System
    Driver (FSD) to use in requesting an I/O operation from a device driver.
    The request must be one of the following request codes:

        IRP_MJ_READ
        IRP_MJ_WRITE
        IRP_MJ_FLUSH_BUFFERS
        IRP_MJ_SHUTDOWN
        IRP_MJ_POWER

    This routine provides a simple, fast interface to the device driver w/o
    having to put the knowledge of how to build an IRP into all of the FSDs
    (and device drivers) in the system.

Arguments:

    MajorFunction - Function to be performed;  see previous list.

    DeviceObject - Pointer to device object on which the I/O will be performed.

    Buffer - Pointer to buffer to get data from or write data into.  This
        parameter is required for read/write, but not for flush or shutdown
        functions.

    Length - Length of buffer in bytes.  This parameter is required for
        read/write, but not for flush or shutdown functions.

    StartingOffset - Pointer to the offset on the disk to read/write from/to.
        This parameter is required for read/write, but not for flush or
        shutdown functions.

    IoStatusBlock - Pointer to the I/O status block for completion status
        information.  This parameter is optional since most asynchronous FSD
        requests will be synchronized by using completion routines, and so the
        I/O status block will not be written.

Return Value:

    The function value is a pointer to the IRP representing the specified
    request.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;

    //
    // Begin by allocating the IRP for this request.  Do not charge quota to
    // the current process for this IRP.
    //

    irp = IoAllocateIrp( DeviceObject->StackSize, FALSE );
    if (!irp) {
        return irp;
    }

    //
    // Set current thread for IoSetHardErrorOrVerifyDevice.
    //

    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and the parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    //
    // Set the major function code.
    //

    irpSp->MajorFunction = (UCHAR) MajorFunction;

    if (MajorFunction != IRP_MJ_FLUSH_BUFFERS &&
        MajorFunction != IRP_MJ_SHUTDOWN &&
        MajorFunction != IRP_MJ_PNP &&
        MajorFunction != IRP_MJ_POWER) {

        //
        // Now allocate a buffer or lock the pages of the caller's buffer into
        // memory based on whether the target device performs direct or buffered
        // I/O operations.
        //

        if (DeviceObject->Flags & DO_BUFFERED_IO) {

            //
            // The target device supports buffered I/O operations.  Allocate a
            // system buffer and, if this is a write, fill it in.  Otherwise,
            // the copy will be done into the caller's buffer in the completion
            // code.  Also note that the system buffer should be deallocated on
            // completion.  Also, set the parameters based on whether this is a
            // read or a write operation.
            //

            irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag( NonPagedPoolCacheAligned,
                                                                     Length,
                                                                     '  oI' );
            if (irp->AssociatedIrp.SystemBuffer == NULL) {
                IoFreeIrp( irp );
                return (PIRP) NULL;
            }

            if (MajorFunction == IRP_MJ_WRITE) {
                RtlCopyMemory( irp->AssociatedIrp.SystemBuffer, Buffer, Length );
                irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
            } else {
                irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_INPUT_OPERATION;
                irp->UserBuffer = Buffer;
            }

        } else if (DeviceObject->Flags & DO_DIRECT_IO) {

            //
            // The target device supports direct I/O operations.  Allocate
            // an MDL large enough to map the buffer and lock the pages into
            // memory.
            //

            irp->MdlAddress = IoAllocateMdl( Buffer,
                                             Length,
                                             FALSE,
                                             FALSE,
                                             (PIRP) NULL );
            if (irp->MdlAddress == NULL) {
                IoFreeIrp( irp );
                return (PIRP) NULL;
            }

            try {
                MmProbeAndLockPages( irp->MdlAddress,
                                     KernelMode,
                                     (LOCK_OPERATION) (MajorFunction == IRP_MJ_READ ? IoWriteAccess : IoReadAccess) );
            } except(EXCEPTION_EXECUTE_HANDLER) {
                  if (irp->MdlAddress != NULL) {
                      IoFreeMdl( irp->MdlAddress );
                  }
                  IoFreeIrp( irp );
                  return (PIRP) NULL;
            }

        } else {

            //
            // The operation is neither buffered nor direct.  Simply pass the
            // address of the buffer in the packet to the driver.
            //

            irp->UserBuffer = Buffer;
        }

        //
        // Set the parameters according to whether this is a read or a write
        // operation.  Notice that these parameters must be set even if the
        // driver has not specified buffered or direct I/O.
        //

        if (MajorFunction == IRP_MJ_WRITE) {
            irpSp->Parameters.Write.Length = Length;
            irpSp->Parameters.Write.ByteOffset = *StartingOffset;
        } else {
            irpSp->Parameters.Read.Length = Length;
            irpSp->Parameters.Read.ByteOffset = *StartingOffset;
        }
    }

    //
    // Finally, set the address of the I/O status block and return a pointer
    // to the IRP.
    //

    irp->UserIosb = IoStatusBlock;
    return irp;
}

PIRP
IoBuildDeviceIoControlRequest(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    IN PKEVENT Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )

/*++

Routine Description:

    This routine builds an I/O Request Packet (IRP) that can be used to
    perform a synchronous internal or normal device I/O control function.

Arguments:

    IoControlCode - Specifies the device I/O control code that is to be
        performed by the target device driver.

    DeviceObject - Specifies the target device on which the I/O control
        function is to be performed.

    InputBuffer - Optional pointer to an input buffer that is to be passed
        to the device driver.

    InputBufferLength - Length of the InputBuffer in bytes.  If the Input-
        Buffer parameter is not passed, this parameter must be zero.

    OutputBuffer - Optional pointer to an output buffer that is to be passed
        to the device driver.

    OutputBufferLength - Length of the OutputBuffer in bytes.  If the
        OutputBuffer parameter is not passed, this parameter must be zero.

    InternalDeviceIoControl - A BOOLEAN parameter that specifies whether
        the packet that gets generated should have a major function code
        of IRP_MJ_INTERNAL_DEVICE_CONTROL (the parameter is TRUE), or
        IRP_MJ_DEVICE_CONTROL (the parameter is FALSE).

    Event - Supplies a pointer to a kernel event that is to be set to the
        Signaled state when the I/O operation is complete.  Note that the
        Event must already be set to the Not-Signaled state.

    IoStatusBlock - Supplies a pointer to an I/O status block that is to
        be filled in with the final status of the operation once it
        completes.

Return Value:

    The function value is a pointer to the generated IRP suitable for calling
    the target device driver.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    ULONG method;

    //
    // Begin by allocating the IRP for this request.  Do not charge quota to
    // the current process for this IRP.
    //

    irp = IoAllocateIrp( DeviceObject->StackSize, FALSE );
    if (!irp) {
        return irp;
    }

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and the parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    //
    // Set the major function code based on the type of device I/O control
    // function the caller has specified.
    //

    if (InternalDeviceIoControl) {
        irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    } else {
        irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    }

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP for those parameters that are the same for all four methods.
    //

    irpSp->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    irpSp->Parameters.DeviceIoControl.IoControlCode = IoControlCode;

    //
    // Get the method bits from the I/O control code to determine how the
    // buffers are to be passed to the driver.
    //

    method = IoControlCode & 3;

    //
    // Based on the method that the buffers are being passed, either allocate
    // buffers or build MDLs or do nothing.
    //

    switch ( method ) {

    case 0:

        //
        // For this case, allocate a buffer that is large enough to contain
        // both the input and the output buffers.  Copy the input buffer
        // to the allocated buffer and set the appropriate IRP fields.
        //

        if (InputBufferLength != 0 || OutputBufferLength != 0) {
            irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag( NonPagedPoolCacheAligned,
                                                                     InputBufferLength > OutputBufferLength ? InputBufferLength : OutputBufferLength,
                                                                     '  oI' );
            if (irp->AssociatedIrp.SystemBuffer == NULL) {
                IoFreeIrp( irp );
                return (PIRP) NULL;
            }
            if (ARGUMENT_PRESENT( InputBuffer )) {
                RtlCopyMemory( irp->AssociatedIrp.SystemBuffer,
                               InputBuffer,
                               InputBufferLength );
            }
            irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
            irp->UserBuffer = OutputBuffer;
            if (ARGUMENT_PRESENT( OutputBuffer )) {
                irp->Flags |= IRP_INPUT_OPERATION;
            }
        } else {
            irp->Flags = 0;
            irp->UserBuffer = (PVOID) NULL;
        }

        break;

    case 1:
    case 2:

        //
        // For these two cases, allocate a buffer that is large enough to
        // contain the input buffer, if any, and copy the information to
        // the allocated buffer.  Then build an MDL for either read or write
        // access, depending on the method, for the output buffer.  Note
        // that an output buffer must have been specified.
        //

        if (ARGUMENT_PRESENT( InputBuffer )) {
            irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag( NonPagedPoolCacheAligned,
                                                                     InputBufferLength,
                                                                     '  oI' );
            if (irp->AssociatedIrp.SystemBuffer == NULL) {
                IoFreeIrp( irp );
                return (PIRP) NULL;
            }
            RtlCopyMemory( irp->AssociatedIrp.SystemBuffer,
                           InputBuffer,
                           InputBufferLength );
            irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
        } else {
            irp->Flags = 0;
        }

        if (ARGUMENT_PRESENT( OutputBuffer )) {
            irp->MdlAddress = IoAllocateMdl( OutputBuffer,
                                             OutputBufferLength,
                                             FALSE,
                                             FALSE,
                                             (PIRP) NULL );
            if (irp->MdlAddress == NULL) {
                if (ARGUMENT_PRESENT( InputBuffer )) {
                    ExFreePool( irp->AssociatedIrp.SystemBuffer );
                }
                IoFreeIrp( irp );
                return (PIRP) NULL;
            }

            try {

                MmProbeAndLockPages( irp->MdlAddress,
                                     KernelMode,
                                     (LOCK_OPERATION) ((method == 1) ? IoReadAccess : IoWriteAccess) );

            } except (EXCEPTION_EXECUTE_HANDLER) {

                  if (irp->MdlAddress != NULL) {
                      IoFreeMdl( irp->MdlAddress );
                  }

                  if (ARGUMENT_PRESENT( InputBuffer )) {
                      ExFreePool( irp->AssociatedIrp.SystemBuffer );
                  }

                  IoFreeIrp( irp );
                  return (PIRP) NULL;
            }
        }

        break;

    case 3:

        //
        // For this case, do nothing.  Everything is up to the driver.
        // Simply give the driver a copy of the caller's parameters and
        // let the driver do everything itself.
        //

        irp->UserBuffer = OutputBuffer;
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer = InputBuffer;
    }

    //
    // Finally, set the address of the I/O status block and the address of
    // the kernel event object.  Note that I/O completion will not attempt
    // to dereference the event since there is no file object associated
    // with this operation.
    //

    irp->UserIosb = IoStatusBlock;
    irp->UserEvent = Event;

    //
    // Also set the address of the current thread in the packet so the
    // completion code will have a context to execute in.  The IRP also
    // needs to be queued to the thread since the caller is going to set
    // the file object pointer.
    //

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    IopQueueThreadIrp( irp );

    //
    // Simply return a pointer to the packet.
    //

    return irp;
}

VOID
IoBuildPartialMdl(
    IN PMDL SourceMdl,
    IN OUT PMDL TargetMdl,
    IN PVOID VirtualAddress,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine maps a portion of a buffer as described by an MDL.  The
    portion of the buffer to be mapped is specified via a virtual address
    and an optional length.  If the length is not supplied, then the
    remainder of the buffer is mapped.

Arguments:

    SourceMdl - MDL for the current buffer.

    TargetMdl - MDL to map the specified portion of the buffer.

    VirtualAddress - Base of the buffer to begin mapping.

    Length - Length of buffer to be mapped;  if zero, remainder.

Return Value:

    None.

Notes:

    This routine assumes that the target MDL is large enough to map the
    desired portion of the buffer.  If the target is not large enough
    then an exception will be raised.

    It is also assumed that the remaining length of the buffer to be mapped
    is non-zero.

--*/

{
    ULONG_PTR baseVa;
    ULONG offset;
    ULONG newLength;
    ULONG pageOffset;
    PPFN_NUMBER basePointer;
    PPFN_NUMBER copyPointer;

    //
    // Calculate the base address of the buffer that the source Mdl maps.
    // Then, determine the length of the buffer to be mapped, if not
    // specified.
    //

    baseVa = (ULONG_PTR) MmGetMdlBaseVa( SourceMdl );
    offset = (ULONG) ((ULONG_PTR)VirtualAddress - baseVa) - MmGetMdlByteOffset(SourceMdl);

    if (Length == 0) {
        newLength = MmGetMdlByteCount( SourceMdl ) - offset;
    } else {
        newLength = Length;
        //if (newLength > (MmGetMdlByteCount(SourceMdl) - offset)) {
        //    KeBugCheck( TARGET_MDL_TOO_SMALL );
        //}
    }

    //
    // Initialize the target MDL header.  Note that the original size of
    // the MDL structure itself is left unchanged.
    //

    //ASSERT ((SourceMdl->MdlFlags & MDL_PARTIAL) == 0);
    TargetMdl->Process = SourceMdl->Process;

    TargetMdl->StartVa = (PVOID) PAGE_ALIGN( VirtualAddress );
    pageOffset = ((ULONG)((ULONG_PTR) TargetMdl->StartVa - (ULONG_PTR) SourceMdl->StartVa)) >> PAGE_SHIFT;


    TargetMdl->ByteCount = newLength;
    TargetMdl->ByteOffset = BYTE_OFFSET( VirtualAddress );
    newLength = COMPUTE_PAGES_SPANNED( VirtualAddress, newLength );
    if (((TargetMdl->Size - sizeof( MDL )) / sizeof (PFN_NUMBER)) < newLength ) {
        KeBugCheck( TARGET_MDL_TOO_SMALL );
    }

    //
    // Set the MdlFlags in the target MDL.  Clear all flags but
    // carry across the allocation information, page read and the
    // system mapped info.
    //

    TargetMdl->MdlFlags &= (MDL_ALLOCATED_FIXED_SIZE | MDL_ALLOCATED_MUST_SUCCEED);
    TargetMdl->MdlFlags |= SourceMdl->MdlFlags & (MDL_SOURCE_IS_NONPAGED_POOL |
                                                  MDL_MAPPED_TO_SYSTEM_VA |
                                                  MDL_IO_PAGE_READ);
    TargetMdl->MdlFlags |= MDL_PARTIAL;

#if DBG
    if (TargetMdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {
        TargetMdl->MdlFlags |= MDL_PARENT_MAPPED_SYSTEM_VA;
    }
#endif //DBG

    //
    // Preserved the mapped system address.
    //

    TargetMdl->MappedSystemVa = (PUCHAR)SourceMdl->MappedSystemVa + offset;

    //
    // Determine the base address of the first PFN in the source MDL that
    // needs to be copied to the target.  Then, copy as many PFNs as are
    // needed.
    //

    basePointer = MmGetMdlPfnArray(SourceMdl);
    basePointer += pageOffset;
    copyPointer = MmGetMdlPfnArray(TargetMdl);

    while (newLength > 0) {
        *copyPointer = *basePointer;
        copyPointer++;
        basePointer++;
        newLength--;
    }
}

PIRP
IoBuildSynchronousFsdRequest(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PKEVENT Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )

/*++

Routine Description:

    This routine builds an I/O Request Packet (IRP) suitable for a File System
    Driver (FSD) to use in requesting an I/O operation from a device driver.
    The request must be one of the following request codes:

        IRP_MJ_READ
        IRP_MJ_WRITE
        IRP_MJ_FLUSH_BUFFERS
        IRP_MJ_SHUTDOWN

    This routine provides a simple, fast interface to the device driver w/o
    having to put the knowledge of how to build an IRP into all of the FSDs
    (and device drivers) in the system.

    The IRP created by this function causes the I/O system to complete the
    request by setting the specified event to the Signaled state.

Arguments:

    MajorFunction - Function to be performed;  see previous list.

    DeviceObject - Pointer to device object on which the I/O will be performed.

    Buffer - Pointer to buffer to get data from or write data into.  This
        parameter is required for read/write, but not for flush or shutdown
        functions.

    Length - Length of buffer in bytes.  This parameter is required for
        read/write, but not for flush or shutdown functions.

    StartingOffset - Pointer to the offset on the disk to read/write from/to.
        This parameter is required for read/write, but not for flush or
        shutdown functions.

    Event - Pointer to a kernel event structure for synchronization.  The event
        will be set to the Signaled state when the I/O has completed.

    IoStatusBlock - Pointer to I/O status block for completion status info.

Return Value:

    The function value is a pointer to the IRP representing the specified
    request.

--*/

{
    PIRP irp;

    //
    // Do all of the real work in real IRP build routine.
    //

    irp = IoBuildAsynchronousFsdRequest( MajorFunction,
                                         DeviceObject,
                                         Buffer,
                                         Length,
                                         StartingOffset,
                                         IoStatusBlock );
    if (irp == NULL) {
        return irp;
    }

    //
    // Now fill in the event to the completion code will do the right thing.
    // Notice that because there is no FileObject, the I/O completion code
    // will not attempt to dereference the event.
    //

    irp->UserEvent = Event;

    //
    // There will be a file object associated w/this packet, so it must be
    // queued to the thread.
    //

    IopQueueThreadIrp( irp );
    return irp;
}

#if DBG && PNP_IO_TRACE
PCHAR pnpmf[] = {
        "START_DEVICE",
        "QUERY_REMOVE_DEVICE",
        "REMOVE_DEVICE",
        "CANCEL_REMOVE_DEVICE",
        "STOP_DEVICE",
        "QUERY_STOP_DEVICE",
        "CANCEL_STOP_DEVICE",
        "QUERY_DEVICE_RELATIONS",
        "QUERY_INTERFACE",
        "QUERY_CAPABILITIES",
        "QUERY_RESOURCES",
        "QUERY_RESOURCE_REQUIREMENTS",
        "QUERY_DEVICE_TEXT",
        "FILTER_RESOURCE_REQUIREMENTS",
        "obsolete minor function!!!",
        "READ_CONFIG",
        "WRITE_CONFIG",
        "EJECT",
        "SET_LOCK",
        "QUERY_ID",
        "QUERY_PNP_DEVICE_STATE",
        "QUERY_BUS_INFORMATION",
        "USAGE_NOTIFICATION"
        };
PCHAR pnpUsageMf[] = {
    "*** Undefined ***",
    "Paging",
    "Hibernation",
    "DumpFile"
};
#endif


NTSTATUS
FASTCALL
IopfCallDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine is invoked to pass an I/O Request Packet (IRP) to another
    driver at its dispatch routine.

Arguments:

    DeviceObject - Pointer to device object to which the IRP should be passed.

    Irp - Pointer to IRP for request.

Return Value:

    Return status from driver's dispatch routine.

--*/

{
    PIO_STACK_LOCATION irpSp;
    PDRIVER_OBJECT driverObject;
    NTSTATUS status;

    //
    // Ensure that this is really an I/O Request Packet.
    //

    ASSERT( Irp->Type == IO_TYPE_IRP );

    //
    // Update the IRP stack to point to the next location.
    //
    Irp->CurrentLocation--;

    if (Irp->CurrentLocation <= 0) {
        KeBugCheckEx( NO_MORE_IRP_STACK_LOCATIONS, (ULONG_PTR) Irp, 0, 0, 0 );
    }

    irpSp = IoGetNextIrpStackLocation( Irp );
    Irp->Tail.Overlay.CurrentStackLocation = irpSp;

    //
    // Save a pointer to the device object for this request so that it can
    // be used later in completion.
    //

    irpSp->DeviceObject = DeviceObject;

    //
    // Invoke the driver at its dispatch routine entry point.
    //

    driverObject = DeviceObject->DriverObject;

    PERFINFO_DRIVER_MAJORFUNCTION_CALL(Irp, irpSp, driverObject);

    status = driverObject->MajorFunction[irpSp->MajorFunction]( DeviceObject,
                                                              Irp );

    PERFINFO_DRIVER_MAJORFUNCTION_RETURN(Irp, irpSp, driverObject);

    return status;
}

NTSTATUS
FASTCALL
IofCallDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    //
    // This routine will either jump immediately to IopfCallDriver, or rather
    // IovCallDriver.
    //
    return pIofCallDriver(DeviceObject, Irp);
}


BOOLEAN
IoCancelIrp(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is invoked to cancel an individual I/O Request Packet.
    It acquires the cancel spin lock, sets the cancel flag in the IRP, and
    then invokes the cancel routine specified by the appropriate field in
    the IRP, if a routine was specified.  It is expected that the cancel
    routine will relaease the cancel spinlock.  If there is no cancel routine,
    then the cancel spin lock is released.

Arguments:

    Irp - Supplies a pointer to the IRP to be cancelled.

Return Value:

    The function value is TRUE if the IRP was in a cancelable state (it
    had a cancel routine), else FALSE is returned.

Notes:

    It is assumed that the caller has taken the necessary action to ensure
    that the packet cannot be fully completed before invoking this routine.

--*/

{
    PDRIVER_CANCEL cancelRoutine;
    KIRQL irql;
    BOOLEAN returnValue;


    ASSERT( Irp->Type == IO_TYPE_IRP );

    if (IopVerifierOn) {
        if (IOV_CANCEL_IRP(Irp, &returnValue)) {
            return returnValue;
        }
    }

    //
    // Acquire the cancel spin lock.
    //

    IoAcquireCancelSpinLock( &irql );

    //
    // Set the cancel flag in the IRP.
    //

    Irp->Cancel = TRUE;

    //
    // Obtain the address of the cancel routine, and if one was specified,
    // invoke it.
    //

    cancelRoutine = (PDRIVER_CANCEL) InterlockedExchangePointer( (PVOID *) &Irp->CancelRoutine,
                                                                 NULL );

    if (cancelRoutine) {
        if (Irp->CurrentLocation > (CCHAR) (Irp->StackCount + 1)) {
            KeBugCheckEx( CANCEL_STATE_IN_COMPLETED_IRP, (ULONG_PTR) Irp, 0, 0, 0 );
        }
        Irp->CancelIrql = irql;

        cancelRoutine( Irp->Tail.Overlay.CurrentStackLocation->DeviceObject,
                       Irp );
        //
        // The cancel spinlock should have been released by the cancel routine.
        //

        return(TRUE);

    } else {

        //
        // There was no cancel routine, so release the cancel spinlock and
        // return indicating the Irp was not currently cancelable.
        //

        IoReleaseCancelSpinLock( irql );

        return(FALSE);
    }
}

VOID
IoCancelThreadIo(
    IN PETHREAD Thread
    )

/*++

Routine Description:

    This routine cancels all of the I/O operations for the specified thread.
    This is accomplished by walking the list of IRPs in the thread IRP list
    and canceling each one individually.  No other I/O operations can be
    started for the thread since this routine has control of the thread itself.

Arguments:

    Tcb - Pointer to the Thread Control Block for the thread.

Return Value:

    None.

--*/

{
    PLIST_ENTRY header;
    PLIST_ENTRY entry;
    KIRQL irql;
    PETHREAD thread;
    PIRP irp;
    ULONG count;
    LARGE_INTEGER interval;

    PAGED_CODE();

    DBG_UNREFERENCED_PARAMETER( Thread );

    thread = PsGetCurrentThread();

    //
    // Raise the IRQL so that the IrpList cannot be modified by a completion
    // APC.
    //

    KeRaiseIrql( APC_LEVEL, &irql );

    header = &thread->IrpList;
    entry = thread->IrpList.Flink;

    //
    // Walk the list of pending IRPs, canceling each of them.
    //

    while (header != entry) {
        irp = CONTAINING_RECORD( entry, IRP, ThreadListEntry );
        IoCancelIrp( irp );
        entry = entry->Flink;
    }

    //
    // Wait for the requests to complete.  Note that waiting may eventually
    // timeout, in which case more work must be done.
    //

    count = 0;
    interval.QuadPart = -10 * 1000 * 100;

    while (!IsListEmpty( &Thread->IrpList )) {

        //
        // Lower the IRQL so that the thread APC can fire which will complete
        // the requests.  Delay execution for a time and let the request
        // finish.  The delay time is 100ms.
        //

        KeLowerIrql( irql );
        KeDelayExecutionThread( KernelMode, FALSE, &interval );

        if (count++ > 3000) {

            //
            // This I/O request has timed out, as it has not been completed
            // for a full 5 minutes. Attempt to remove the packet's association
            // with this thread.  Note that by not resetting the count, the
            // next time through the loop the next packet, if there is one,
            // which has also timed out, will be dealt with, although it
            // will be given another 100ms to complete.
            //

            IopDisassociateThreadIrp();
        }

        KeRaiseIrql( APC_LEVEL, &irql );
    }

    KeLowerIrql( irql );
}

NTSTATUS
IoCheckDesiredAccess(
    IN OUT PACCESS_MASK DesiredAccess,
    IN ACCESS_MASK GrantedAccess
    )

/*++

Routine Description:

    This routine is invoked to determine whether or not the granted access
    to a file allows the access specified by a desired access.

Arguments:

    DesiredAccess - Pointer to a variable containing the access desired to
        the file.

    GrantedAccess - Access currently granted to the file.

Return Value:

    The final status of the access check is the function value.  If the
    accessor has the access to the file, then STATUS_SUCCESS is returned;
    otherwise, STATUS_ACCESS_DENIED is returned.

    Also, the DesiredAccess is returned with no generic mapping.

--*/

{
    PAGED_CODE();

    //
    // Convert the desired access to a non-generic access mask.
    //

    RtlMapGenericMask( DesiredAccess,
                       &IoFileObjectType->TypeInfo.GenericMapping );

    //
    // Determine whether the desired access to the file is allowed, given
    // the current granted access.
    //

    if (!SeComputeDeniedAccesses( GrantedAccess, *DesiredAccess )) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_ACCESS_DENIED;
    }
}

NTSTATUS
IoCheckEaBufferValidity(
    IN PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG EaLength,
    OUT PULONG ErrorOffset
    )

/*++

Routine Description:

    This routine checks the validity of the specified EA buffer to guarantee
    that its format is proper, no fields hang over, that it is not recursive,
    etc.

Arguments:

    EaBuffer - Pointer to the buffer containing the EAs to be checked.

    EaLength - Specifies the length of EaBuffer.

    ErrorOffset - A variable to receive the offset of the offending entry
        in the EA buffer if an error is incurred.  This variable is only
        valid if an error occurs.

Return Value:

    The function value is STATUS_SUCCESS if the EA buffer contains a valid,
    properly formed list, otherwise STATUS_EA_LIST_INCONSISTENT.

--*/

#define ALIGN_LONG( Address ) ( (ULONG) ((Address + 3) & ~3) )

#define GET_OFFSET_LENGTH( CurrentEa, EaBase ) (    \
    (ULONG) ((PCHAR) CurrentEa - (PCHAR) EaBase) )

{
    LONG tempLength;
    ULONG entrySize;
    PFILE_FULL_EA_INFORMATION eas;

    PAGED_CODE();

    //
    // Walk the buffer and ensure that its format is valid.  That is, ensure
    // that it does not walk off the end of the buffer, is not recursive,
    // etc.
    //

    eas = EaBuffer;
    tempLength = EaLength;

    for (;;) {

        //
        // Get the size of the current entry in the buffer.  The minimum
        // size of the entry is the fixed size part of the structure plus
        // the length of the name, a single termination character byte which
        // must be present (a 0), plus the length of the value.  If this
        // is not the last entry, then there will also be pad bytes to get
        // to the next longword boundary.
        //

        //
        // Start by checking that the fixed size lies within the stated length.
        //

        if (tempLength < FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0])) {

            *ErrorOffset = GET_OFFSET_LENGTH( eas, EaBuffer );
            return STATUS_EA_LIST_INCONSISTENT;
        }

        entrySize = FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +
                        eas->EaNameLength + 1 + eas->EaValueLength;

        //
        // Confirm that the full length lies within the stated buffer length.
        //

        if ((ULONG) tempLength < entrySize) {

            *ErrorOffset = GET_OFFSET_LENGTH( eas, EaBuffer );
            return STATUS_EA_LIST_INCONSISTENT;
        }

        //
        // Confirm that there is a NULL terminator after the name.
        //

        if (eas->EaName[eas->EaNameLength] != '\0') {

            *ErrorOffset = GET_OFFSET_LENGTH( eas, EaBuffer );
            return STATUS_EA_LIST_INCONSISTENT;
        }

        if (eas->NextEntryOffset) {

            //
            // There is another entry in the buffer and it must be longword
            // aligned.  Ensure that the offset indicates that it is.  If it
            // isn't, return invalid parameter.
            //

            if (ALIGN_LONG( entrySize ) != eas->NextEntryOffset ||
                (LONG) eas->NextEntryOffset < 0) {
                *ErrorOffset = GET_OFFSET_LENGTH( eas, EaBuffer );
                return STATUS_EA_LIST_INCONSISTENT;

            } else {

                //
                // There is another entry in the buffer, so account for the
                // size of the current entry in the length and get a pointer
                // to the next entry.
                //

                tempLength -= eas->NextEntryOffset;
                if (tempLength < 0) {
                    *ErrorOffset = GET_OFFSET_LENGTH( eas, EaBuffer );
                    return STATUS_EA_LIST_INCONSISTENT;
                }
                eas = (PFILE_FULL_EA_INFORMATION) ((PCHAR) eas + eas->NextEntryOffset);

            }

        } else {

            //
            // There are no other entries in the buffer.  Simply account for
            // the overall buffer length according to the size of the current
            // entry and exit the loop.
            //

            tempLength -= entrySize;
            break;

        }
    }

    //
    // All of the entries in the buffer have been processed.  Check to see
    // whether the overall buffer length went negative.  If so, return an
    // error.
    //

    if (tempLength < 0) {
        *ErrorOffset = GET_OFFSET_LENGTH( eas, EaBuffer );
        return STATUS_EA_LIST_INCONSISTENT;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IoCheckFunctionAccess(
    IN ACCESS_MASK GrantedAccess,
    IN UCHAR MajorFunction,
    IN UCHAR MinorFunction,
    IN ULONG IoControlCode,
    IN PVOID Arg1 OPTIONAL,
    IN PVOID Arg2 OPTIONAL
    )

/*++

Routine Description:

    This routine checks the parameters and access for the function and
    parameters specified by the input parameters against the current access
    to the file as described by the GrantedAccess mask parameter.  If the
    caller has the access to the file, then a successful status code is
    returned.  Otherwise, an error status code is returned as the function
    value.

Arguments:

    GrantedAccess - Access granted to the file for the caller.

    MajorFunction - Major function code for the operation being performed.

    MinorFunction - Minor function code for the operation being performed.

    IoControlCode - I/O function control code for a device or file system I/O
        code.  Used only for those two function types.

    Arg1 - Optional argument that depends on the major function. Its
         FileInformationClass if the major function code indicates a query or set
         file information function is being performed. It points to Security Info
         if major function code is IRP_MJ_*_SECURITY.

    Arg2 - Optional second argument that depends on the major function. Currently its
        FsInformationClass.This parameter MUST be supplied if the major function
        code indicates that a query or set file system information function is
        being performed.

Return Value:

    The final status of the access check is the function value.  If the
    accessor has the access to the file, then STATUS_SUCCESS is returned;
    otherwise, STATUS_ACCESS_DENIED is returned.

Note:

    The GrantedAccess mask may not contain any generic mappings.  That is,
    the IoCheckDesiredAccess function must have been previously invoked to
    return a full mask.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PFILE_INFORMATION_CLASS FileInformationClass;
    PFS_INFORMATION_CLASS FsInformationClass;
    SECURITY_INFORMATION SecurityInformation;
    ACCESS_MASK DesiredAccess;

    UNREFERENCED_PARAMETER( MinorFunction );

    PAGED_CODE();

    //
    // Determine the major function being performed.  If the function code
    // is invalid, then return an error.
    //

    FileInformationClass = (PFILE_INFORMATION_CLASS)Arg1;
    FsInformationClass = (PFS_INFORMATION_CLASS)Arg2;

    switch( MajorFunction ) {

    case IRP_MJ_CREATE:
    case IRP_MJ_CLOSE:

        break;

    case IRP_MJ_READ:

        if (SeComputeDeniedAccesses( GrantedAccess, FILE_READ_DATA )) {
            status = STATUS_ACCESS_DENIED;
        }
        break;

    case IRP_MJ_WRITE:

        if (!SeComputeGrantedAccesses( GrantedAccess, FILE_WRITE_DATA | FILE_APPEND_DATA )) {
            status = STATUS_ACCESS_DENIED;
        }
        break;

    case IRP_MJ_QUERY_INFORMATION:

        if (IopQueryOperationAccess[*FileInformationClass] != 0) {
            if (SeComputeDeniedAccesses( GrantedAccess, IopQueryOperationAccess[*FileInformationClass] )) {
                status = STATUS_ACCESS_DENIED;
            }
        }
        break;

    case IRP_MJ_SET_INFORMATION:

        if (IopSetOperationAccess[*FileInformationClass] != 0) {
            if (SeComputeDeniedAccesses( GrantedAccess, IopSetOperationAccess[*FileInformationClass] )) {
                status = STATUS_ACCESS_DENIED;
            }
        }
        break;

    case IRP_MJ_QUERY_EA:

        if (SeComputeDeniedAccesses( GrantedAccess, FILE_READ_EA )) {
            status = STATUS_ACCESS_DENIED;
        }
        break;

    case IRP_MJ_SET_EA:

        if (SeComputeDeniedAccesses( GrantedAccess, FILE_WRITE_EA )) {
            status = STATUS_ACCESS_DENIED;
        }
        break;

    case IRP_MJ_FLUSH_BUFFERS:

        if (SeComputeDeniedAccesses( GrantedAccess, FILE_WRITE_DATA )) {
            status = STATUS_ACCESS_DENIED;
        }
        break;

    case IRP_MJ_QUERY_VOLUME_INFORMATION:

        if (SeComputeDeniedAccesses( GrantedAccess, IopQueryFsOperationAccess[*FsInformationClass] )) {
            status = STATUS_ACCESS_DENIED;
        }
        break;

    case IRP_MJ_SET_VOLUME_INFORMATION:

        if (SeComputeDeniedAccesses( GrantedAccess, IopSetFsOperationAccess[*FsInformationClass] )) {
            status = STATUS_ACCESS_DENIED;
        }
        break;

    case IRP_MJ_DIRECTORY_CONTROL:

        if (SeComputeDeniedAccesses( GrantedAccess, FILE_LIST_DIRECTORY )) {
            status = STATUS_ACCESS_DENIED;
        }
        break;

    case IRP_MJ_FILE_SYSTEM_CONTROL:
    case IRP_MJ_DEVICE_CONTROL:
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:

        {
            ULONG accessMode = (IoControlCode >> 14) & 3;

            if (accessMode != FILE_ANY_ACCESS) {

                //
                // This I/O control requires that the caller have read, write,
                // or read/write access to the object.  If this is not the case,
                // then cleanup and return an appropriate error status code.
                //

                if (!(SeComputeGrantedAccesses( GrantedAccess, accessMode ))) {
                    status = STATUS_ACCESS_DENIED;
                }
            }

        }
        break;

    case IRP_MJ_LOCK_CONTROL:

        if (!SeComputeGrantedAccesses( GrantedAccess, FILE_READ_DATA | FILE_WRITE_DATA )) {
            status = STATUS_ACCESS_DENIED;
        }
        break;

    case IRP_MJ_SET_SECURITY:

        SecurityInformation = *((PSECURITY_INFORMATION)Arg1);
        SeSetSecurityAccessMask(SecurityInformation, &DesiredAccess);

        if (SeComputeDeniedAccesses( GrantedAccess, DesiredAccess )) {
            status = STATUS_ACCESS_DENIED;
        }
        break;

    case IRP_MJ_QUERY_SECURITY:

        SecurityInformation = *((PSECURITY_INFORMATION)Arg1);
        SeQuerySecurityAccessMask(SecurityInformation, &DesiredAccess);

        if (SeComputeDeniedAccesses( GrantedAccess, DesiredAccess )) {
            status = STATUS_ACCESS_DENIED;
        }
        break;
    default:

        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    return status;
}

NTKERNELAPI
NTSTATUS
IoCheckQuerySetFileInformation(
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN ULONG Length,
    IN BOOLEAN SetOperation
    )

/*++

Routine Description:

    This routine checks the validity of the parameters for either a query or a
    set file information operation.  It is used primarily by network servers
    running in kernel mode since no such parameter validity checking is done
    in the normal path.

Arguments:

    FileInformationClass - Specifies the information class to check checked.

    Length - Specifies the length of the buffer supplied.

    SetOperation - Specifies that the operation was a set file information as
        opposed to a query operation.

Return Value:

    The function value is STATUS_SUCCESS if the parameters were valid,
    otherwise an appropriate error is returned.

--*/

{
    PCHAR operationLength;

    //
    // The file information class itself must be w/in the valid range of file
    // information classes, otherwise this is an invalid information class.
    //

    if ((ULONG) FileInformationClass >= FileMaximumInformation) {
        return STATUS_INVALID_INFO_CLASS;
    }

    //
    // Determine whether this is a query or a set operation and act accordingly.
    //

    operationLength = SetOperation ? IopSetOperationLength : IopQueryOperationLength;

    if (!operationLength[FileInformationClass]) {
        return STATUS_INVALID_INFO_CLASS;
    }
    if (Length < (ULONG) operationLength[FileInformationClass]) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    return STATUS_SUCCESS;
}
NTKERNELAPI
NTSTATUS
IoCheckQuerySetVolumeInformation(
    IN FS_INFORMATION_CLASS FsInformationClass,
    IN ULONG Length,
    IN BOOLEAN SetOperation
    )

/*++

Routine Description:

    This routine checks the validity of the parameters for either a query or a
    set volume information operation.  It is used primarily by network servers
    running in kernel mode since no such parameter validity checking is done
    in the normal path.

Arguments:

    FsInformationClass - Specifies the information class to check.

    Length - Specifies the length of the buffer supplied.

    SetOperation - Specifies that the operation was a set volume information as
        opposed to a query operation.

Return Value:

    The function value is STATUS_SUCCESS if the parameters were valid,
    otherwise an appropriate error is returned.

--*/

{
    PCHAR operationLength;

    operationLength = SetOperation ? IopSetFsOperationLength : IopQueryFsOperationLength;

    //
    // The volume information class itself must be w/in the valid range of file
    // information classes, otherwise this is an invalid information class.
    //
    if ((ULONG) FsInformationClass >= FileFsMaximumInformation ||
        operationLength[ FsInformationClass ] == 0 ) {

        return STATUS_INVALID_INFO_CLASS;
    }

    if (Length < (ULONG) operationLength[FsInformationClass]) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IoCheckQuotaBufferValidity(
    IN PFILE_QUOTA_INFORMATION QuotaBuffer,
    IN ULONG QuotaLength,
    OUT PULONG ErrorOffset
    )

/*++

Routine Description:

    This routine checks the validity of the specified quota buffer to guarantee
    that its format is proper, no fields hang over, that it is not recursive,
    etc.

Arguments:

    QuotaBuffer - Pointer to the buffer containing the quota entries to be
        checked.

    QuotaLength - Specifies the length of the QuotaBuffer.

    ErrorOffset - A variable to receive the offset of the offending entry in
        the quota buffer if an error is incurred.  This variable is only valid
        if an error occurs.

Return Value:

    The function value is STATUS_SUCCESS if the quota buffer contains a valid,
    properly formed list, otherwise STATUS_QUOTA_LIST_INCONSISTENT.

--*/

#if defined(_X86_)
#define REQUIRED_QUOTA_ALIGNMENT sizeof( ULONG )
#else
#define REQUIRED_QUOTA_ALIGNMENT sizeof( ULONGLONG )
#endif

#define ALIGN_QUAD( Address ) ( (ULONG) ((Address + 7) & ~7) )

#define GET_OFFSET_LENGTH( CurrentEntry, QuotaBase ) (\
    (ULONG) ((PCHAR) CurrentEntry - (PCHAR) QuotaBase) )

{
    LONG tempLength;
    ULONG entrySize;
    PFILE_QUOTA_INFORMATION quotas;

    PAGED_CODE();

    //
    // Walk the buffer and ensure that its format is valid.  That is, ensure
    // that it does not walk off the end of the buffer, is not recursive,
    // etc.
    //

    quotas = QuotaBuffer;
    tempLength = QuotaLength;

    //
    // Ensure the buffer has the correct alignment.
    //

    if ((ULONG_PTR) quotas & (REQUIRED_QUOTA_ALIGNMENT - 1)) {
        *ErrorOffset = 0;
        return STATUS_DATATYPE_MISALIGNMENT;
    }

    for (;;) {

        ULONG sidLength;

        //
        // Get the size of the current entry in the buffer.  The minimum size
        // of the entry is the fixed size part of the structure plus the actual
        // length of the SID.  If this is not the last entry, then there will
        // also be pad bytes to get to the next longword boundary.  Likewise,
        // ensure that the SID itself is valid.
        //

        if (tempLength < FIELD_OFFSET( FILE_QUOTA_INFORMATION, Sid ) ||
            !RtlValidSid( &quotas->Sid )) {
            goto error_exit;
        }

        sidLength = RtlLengthSid( (&quotas->Sid) );
        entrySize = FIELD_OFFSET( FILE_QUOTA_INFORMATION, Sid ) + sidLength;

        //
        // Confirm that the full length lies within the stated buffer length.
        //

        if ((ULONG) tempLength < entrySize ||
            quotas->SidLength != sidLength) {
            goto error_exit;
        }

        if (quotas->NextEntryOffset) {

            //
            // There is another entry in the buffer and it must be longword
            // aligned.  Ensure that the offset indicates that it is.  If it
            // is not, return error status code.
            //

            if (entrySize > quotas->NextEntryOffset ||
                quotas->NextEntryOffset & (REQUIRED_QUOTA_ALIGNMENT - 1) ||
                (LONG) quotas->NextEntryOffset < 0) {
                goto error_exit;

            } else {

                //
                // There is another entry in the buffer, so account for the size
                // of the current entry in the length and get a pointer to the
                // next entry.
                //

                tempLength -= quotas->NextEntryOffset;
                if (tempLength < 0) {
                    goto error_exit;
                }
                quotas = (PFILE_QUOTA_INFORMATION) ((PCHAR) quotas + quotas->NextEntryOffset);
            }

        } else {

            //
            // There are no more entries in the buffer.  Simply account for the
            // overall buffer length according to the size of the current
            // entry and exit the loop.
            //

            tempLength -= entrySize;
            break;
        }
    }

    //
    // All of the entries in the buffer have been processed.  Check to see
    // whether the overall buffer length went negative.  If so, return an
    // error.
    //

    if (tempLength < 0) {
        goto error_exit;
    }

    return STATUS_SUCCESS;

error_exit:

    *ErrorOffset = GET_OFFSET_LENGTH( quotas, QuotaBuffer );
    return STATUS_QUOTA_LIST_INCONSISTENT;

}

NTSTATUS
IoCheckShareAccess(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN BOOLEAN Update
    )

/*++

Routine Description:

    This routine is invoked to determine whether or not a new accessor to
    a file actually has shared access to it.  The check is made according
    to:

        1)  How the file is currently opened.

        2)  What types of shared accesses are currently specified.

        3)  The desired and shared accesses that the new open is requesting.

    If the open should succeed, then the access information about how the
    file is currently opened is updated, according to the Update parameter.

Arguments:

    DesiredAccess - Desired access of current open request.

    DesiredShareAccess - Shared access requested by current open request.

    FileObject - Pointer to the file object of the current open request.

    ShareAccess - Pointer to the share access structure that describes how
        the file is currently being accessed.

    Update - Specifies whether or not the share access information for the
        file is to be updated.

Return Value:

    The final status of the access check is the function value.  If the
    accessor has access to the file, STATUS_SUCCESS is returned.  Otherwise,
    STATUS_SHARING_VIOLATION is returned.

Note:

    Note that the ShareAccess parameter must be locked against other accesses
    from other threads while this routine is executing.  Otherwise the counts
    will be out-of-synch.

--*/

{
    ULONG ocount;

    PAGED_CODE();

    //
    // Set the access type in the file object for the current accessor.
    // Note that reading and writing attributes are not included in the
    // access check.
    //

    FileObject->ReadAccess = (BOOLEAN) ((DesiredAccess & (FILE_EXECUTE
        | FILE_READ_DATA)) != 0);
    FileObject->WriteAccess = (BOOLEAN) ((DesiredAccess & (FILE_WRITE_DATA
        | FILE_APPEND_DATA)) != 0);
    FileObject->DeleteAccess = (BOOLEAN) ((DesiredAccess & DELETE) != 0);

    //
    // There is no more work to do unless the user specified one of the
    // sharing modes above.
    //

    if (FileObject->ReadAccess ||
        FileObject->WriteAccess ||
        FileObject->DeleteAccess) {

        FileObject->SharedRead = (BOOLEAN) ((DesiredShareAccess & FILE_SHARE_READ) != 0);
        FileObject->SharedWrite = (BOOLEAN) ((DesiredShareAccess & FILE_SHARE_WRITE) != 0);
        FileObject->SharedDelete = (BOOLEAN) ((DesiredShareAccess & FILE_SHARE_DELETE) != 0);

        //
        // Now check to see whether or not the desired accesses are compatible
        // with the way that the file is currently open.
        //

        ocount = ShareAccess->OpenCount;

        if ( (FileObject->ReadAccess && (ShareAccess->SharedRead < ocount))
             ||
             (FileObject->WriteAccess && (ShareAccess->SharedWrite < ocount))
             ||
             (FileObject->DeleteAccess && (ShareAccess->SharedDelete < ocount))
             ||
             ((ShareAccess->Readers != 0) && !FileObject->SharedRead)
             ||
             ((ShareAccess->Writers != 0) && !FileObject->SharedWrite)
             ||
             ((ShareAccess->Deleters != 0) && !FileObject->SharedDelete)
           ) {

            //
            // The check failed.  Simply return to the caller indicating that the
            // current open cannot access the file.
            //

            return STATUS_SHARING_VIOLATION;

        //
        // The check was successful.  Update the counter information in the
        // shared access structure for this open request if the caller
        // specified that it should be updated.
        //

        } else if (Update) {

            ShareAccess->OpenCount++;

            ShareAccess->Readers += FileObject->ReadAccess;
            ShareAccess->Writers += FileObject->WriteAccess;
            ShareAccess->Deleters += FileObject->DeleteAccess;

            ShareAccess->SharedRead += FileObject->SharedRead;
            ShareAccess->SharedWrite += FileObject->SharedWrite;
            ShareAccess->SharedDelete += FileObject->SharedDelete;
        }
    }
    return STATUS_SUCCESS;
}

VOID
FASTCALL
IofCompleteRequest(
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    )
{
    //
    // This routine will either jump immediately to IopfCompleteRequest, or
    // rather IovCompleteRequest.
    //
    pIofCompleteRequest(Irp, PriorityBoost);
}

VOID
FASTCALL
IopfCompleteRequest(
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    )

/*++

Routine Description:

    This routine is invoked to complete an I/O request.  It is invoked by the
    driver in its DPC routine to perform the final completion of the IRP.  The
    functions performed by this routine are as follows.

        1.  A check is made to determine whether the packet's stack locations
            have been exhausted.  If not, then the stack location pointer is set
            to the next location and if there is a routine to be invoked, then
            it will be invoked.  This continues until there are either no more
            routines which are interested or the packet runs out of stack.

            If a routine is invoked to complete the packet for a specific driver
            which needs to perform work a lot of work or the work needs to be
            performed in the context of another process, then the routine will
            return an alternate success code of STATUS_MORE_PROCESSING_REQUIRED.
            This indicates that this completion routine should simply return to
            its caller because the operation will be "completed" by this routine
            again sometime in the future.

        2.  A check is made to determine whether this IRP is an associated IRP.
            If it is, then the count on the master IRP is decremented.  If the
            count for the master becomes zero, then the master IRP will be
            completed according to the steps below taken for a normal IRP being
            completed.  If the count is still non-zero, then this IRP (the one
            being completed) will simply be deallocated.

        3.  If this is paging I/O or a close operation, then simply write the
            I/O status block and set the event to the signaled state, and
            dereference the event.  If this is paging I/O, deallocate the IRP
            as well.

        4.  Unlock the pages, if any, specified by the MDL by calling
            MmUnlockPages.

        5.  A check is made to determine whether or not completion of the
            request can be deferred until later.  If it can be, then this
            routine simply exits and leaves it up to the originator of the
            request to fully complete the IRP.  By not initializing and queueing
            the special kernel APC to the calling thread (which is the current
            thread by definition), a lot of interrupt and queueing processing
            can be avoided.


        6.  The final rundown routine is invoked to queue the request packet to
            the target (requesting) thread as a special kernel mode APC.

Arguments:

    Irp - Pointer to the I/O Request Packet to complete.

    PriorityBoost - Supplies the amount of priority boost that is to be given
        to the target thread when the special kernel APC is queued.

Return Value:

    None.

--*/

#define ZeroIrpStackLocation( IrpSp ) {         \
    (IrpSp)->MinorFunction = 0;                 \
    (IrpSp)->Flags = 0;                         \
    (IrpSp)->Control = 0 ;                      \
    (IrpSp)->Parameters.Others.Argument1 = 0;   \
    (IrpSp)->Parameters.Others.Argument2 = 0;   \
    (IrpSp)->Parameters.Others.Argument3 = 0;   \
    (IrpSp)->Parameters.Others.Argument4 = 0;   \
    (IrpSp)->FileObject = (PFILE_OBJECT) NULL; }

{
    PIRP masterIrp;
    NTSTATUS status;
    PIO_STACK_LOCATION stackPointer;
    PMDL mdl;
    PETHREAD thread;
    PFILE_OBJECT fileObject;
    KIRQL irql;
    PVOID saveAuxiliaryPointer = NULL;

    //
    // Begin by ensuring that this packet has not already been completed
    // by someone.
    //

    if (Irp->CurrentLocation > (CCHAR) (Irp->StackCount + 1) ||
        Irp->Type != IO_TYPE_IRP) {
        KeBugCheckEx( MULTIPLE_IRP_COMPLETE_REQUESTS, (ULONG_PTR) Irp, __LINE__, 0, 0 );
    }

    //
    // Ensure that the packet being completed really is still an IRP.
    //

    ASSERT( Irp->Type == IO_TYPE_IRP );

    //
    // Ensure that no one believes that this request is still in a cancelable
    // state.
    //

    ASSERT( !Irp->CancelRoutine );

    //
    // Ensure that the packet is not being completed with a thoroughly
    // confusing status code.  Actually completing a packet with a pending
    // status probably means that someone forgot to set the real status in
    // the packet.
    //

    ASSERT( Irp->IoStatus.Status != STATUS_PENDING );

    //
    // Ensure that the packet is not being completed with a minus one.  This
    // is apparently a common problem in some drivers, and has no meaning
    // as a status code.
    //

    ASSERT( Irp->IoStatus.Status != 0xffffffff );

    //
    // Ensure that if this is a paging I/O operation, and it failed, that the
    // reason for the failure isn't because quota was exceeded.
    //

    ASSERT( !(Irp->Flags & IRP_PAGING_IO && Irp->IoStatus.Status == STATUS_QUOTA_EXCEEDED ) );

    //
    // Now check to see whether this is the last driver that needs to be
    // invoked for this packet.  If not, then bump the stack and check to
    // see whether the driver wishes to see the completion.  As each stack
    // location is examined, invoke any routine which needs to be invoked.
    // If the routine returns STATUS_MORE_PROCESSING_REQUIRED, then stop the
    // processing of this packet.
    //

    for (stackPointer = IoGetCurrentIrpStackLocation( Irp ),
         Irp->CurrentLocation++,
         Irp->Tail.Overlay.CurrentStackLocation++;
         Irp->CurrentLocation <= (CCHAR) (Irp->StackCount + 1);
         stackPointer++,
         Irp->CurrentLocation++,
         Irp->Tail.Overlay.CurrentStackLocation++) {

        //
        // A stack location was located.  Check to see whether or not it
        // has a completion routine and if so, whether or not it should be
        // invoked.
        //
        // Begin by saving the pending returned flag in the current stack
        // location in the fixed part of the IRP.
        //

        Irp->PendingReturned = stackPointer->Control & SL_PENDING_RETURNED;

        if ( (NT_SUCCESS( Irp->IoStatus.Status ) &&
             stackPointer->Control & SL_INVOKE_ON_SUCCESS) ||
             (!NT_SUCCESS( Irp->IoStatus.Status ) &&
             stackPointer->Control & SL_INVOKE_ON_ERROR) ||
             (Irp->Cancel &&
             stackPointer->Control & SL_INVOKE_ON_CANCEL)
           ) {

            //
            // This driver has specified a completion routine.  Invoke the
            // routine passing it a pointer to its device object and the
            // IRP that is being completed.
            //

            ZeroIrpStackLocation( stackPointer );

            PERFINFO_DRIVER_COMPLETIONROUTINE_CALL(Irp, stackPointer);

            status = stackPointer->CompletionRoutine( (PDEVICE_OBJECT) (Irp->CurrentLocation == (CCHAR) (Irp->StackCount + 1) ?
                                                      (PDEVICE_OBJECT) NULL :
                                                      IoGetCurrentIrpStackLocation( Irp )->DeviceObject),
                                                      Irp,
                                                      stackPointer->Context );

            PERFINFO_DRIVER_COMPLETIONROUTINE_RETURN(Irp, stackPointer);

            if (status == STATUS_MORE_PROCESSING_REQUIRED) {

                //
                // Note:  Notice that if the driver has returned the above
                //        status value, it may have already DEALLOCATED the
                //        packet!  Therefore, do NOT touch any part of the
                //        IRP in the following code.
                //

                return;
            }

        } else {
            if (Irp->PendingReturned && Irp->CurrentLocation <= Irp->StackCount) {
                IoMarkIrpPending( Irp );
            }
            ZeroIrpStackLocation( stackPointer );
        }
    }

    //
    // Check to see whether this is an associated IRP.  If so, then decrement
    // the count in the master IRP.  If the count is decremented to zero,
    // then complete the master packet as well.
    //

    if (Irp->Flags & IRP_ASSOCIATED_IRP) {
        ULONG count;
        masterIrp = Irp->AssociatedIrp.MasterIrp;
        count = ExInterlockedAddUlong( (PULONG) &masterIrp->AssociatedIrp.IrpCount,
                                       0xffffffff,
                                       &IopDatabaseLock );

        //
        // Deallocate this packet and any MDLs that are associated with it
        // by either doing direct deallocations if they were allocated from
        // a zone or by queueing the packet to a thread to perform the
        // deallocation.
        //
        // Also, check the count of the master IRP to determine whether or not
        // the count has gone to zero.  If not, then simply get out of here.
        // Otherwise, complete the master packet.
        //

        Irp->Tail.Overlay.Thread = masterIrp->Tail.Overlay.Thread;
        IopFreeIrpAndMdls( Irp );
        if (count == 1) {
            IoCompleteRequest( masterIrp, PriorityBoost );
        }
        return;
    }

    //
    // Check to see if we have a name junction. If so set the stage to
    // transmogrify the reparse point data in IopCompleteRequest.
    //

    if ((Irp->IoStatus.Status == STATUS_REPARSE )  &&
        (Irp->IoStatus.Information > IO_REPARSE_TAG_RESERVED_RANGE)) {

        if (Irp->IoStatus.Information == IO_REPARSE_TAG_MOUNT_POINT) {

            //
            // For name junctions, we save the pointer to the auxiliary
            // buffer and use it below.
            //

            ASSERT( Irp->Tail.Overlay.AuxiliaryBuffer != NULL );

            saveAuxiliaryPointer = (PVOID) Irp->Tail.Overlay.AuxiliaryBuffer;

            //
            // We NULL the entry to avoid its de-allocation at this time.
            // This buffer get deallocated in IopDoNameTransmogrify
            //

            Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
        } else {

            //
            // Fail the request. A driver needed to act on this IRP prior
            // to getting to this point.
            //

            Irp->IoStatus.Status = STATUS_IO_REPARSE_TAG_NOT_HANDLED;
        }
    }

    //
    // Check the auxiliary buffer pointer in the packet and if a buffer was
    // allocated, deallocate it now.  Note that this buffer must be freed
    // here since the pointer is overlayed with the APC that will be used
    // to get to the requesting thread's context.
    //

    if (Irp->Tail.Overlay.AuxiliaryBuffer) {
        ExFreePool( Irp->Tail.Overlay.AuxiliaryBuffer );
        Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
    }

    //
    // Check to see if this is paging I/O or a close operation.  If either,
    // then special processing must be performed.  The reasons that special
    // processing must be performed is different based on the type of
    // operation being performed.  The biggest reasons for special processing
    // on paging operations are that using a special kernel APC for an in-
    // page operation cannot work since the special kernel APC can incur
    // another pagefault.  Likewise, all paging I/O uses MDLs that belong
    // to the memory manager, not the I/O system.
    //
    // Close operations are special because the close may have been invoked
    // because of a special kernel APC (some IRP was completed which caused
    // the reference count on the object to become zero while in the I/O
    // system's special kernel APC routine).  Therefore, a special kernel APC
    // cannot be used since it cannot execute until the close APC finishes.
    //
    // The special steps are as follows for a synchronous paging operation
    // and close are:
    //
    //     1.  Copy the I/O status block (it is in SVAS, nonpaged).
    //     2.  Signal the event
    //     3.  If paging I/O, deallocate the IRP
    //
    // The special steps taken for asynchronous paging operations (out-pages)
    // are as follows:
    //
    //     1.  Initialize a special kernel APC just for page writes.
    //     1.  Queue the special kernel APC.
    //
    // It should also be noted that the logic for completing a Mount request
    // operation is exactly the same as a Page Read.  No assumptions should be
    // made here about this being a Page Read operation w/o carefully checking
    // to ensure that they are also true for a Mount.  That is:
    //
    //     IRP_PAGING_IO  and  IRP_MOUNT_COMPLETION
    //
    // are the same flag in the IRP.
    //
    // Also note that the last time the IRP is touched for a close operation
    // must be just before the event is set to the signaled state.  Once this
    // occurs, the IRP can be deallocated by the thread waiting for the event.
    //

    if (Irp->Flags & (IRP_PAGING_IO | IRP_CLOSE_OPERATION)) {
        if (Irp->Flags & (IRP_SYNCHRONOUS_PAGING_IO | IRP_CLOSE_OPERATION)) {
            ULONG flags;

            flags = Irp->Flags & IRP_SYNCHRONOUS_PAGING_IO;
            *Irp->UserIosb = Irp->IoStatus;
            (VOID) KeSetEvent( Irp->UserEvent, PriorityBoost, FALSE );
            if (flags) {
                IoFreeIrp( Irp );
            }
        } else {
            thread = Irp->Tail.Overlay.Thread;
            KeInitializeApc( &Irp->Tail.Apc,
                             &thread->Tcb,
                             Irp->ApcEnvironment,
                             IopCompletePageWrite,
                             (PKRUNDOWN_ROUTINE) NULL,
                             (PKNORMAL_ROUTINE) NULL,
                             KernelMode,
                             (PVOID) NULL );
            (VOID) KeInsertQueueApc( &Irp->Tail.Apc,
                                     (PVOID) NULL,
                                     (PVOID) NULL,
                                     PriorityBoost );
        }
        return;
    }

    //
    // Check to see whether any pages need to be unlocked.
    //

    if (Irp->MdlAddress != NULL) {

        //
        // Unlock any pages that may be described by MDLs.
        //

        mdl = Irp->MdlAddress;
        while (mdl != NULL) {
            MmUnlockPages( mdl );
            mdl = mdl->Next;
        }
    }

    //
    // Make a final check here to determine whether or not this is a
    // synchronous I/O operation that is being completed in the context
    // of the original requestor.  If so, then an optimal path through
    // I/O completion can be taken.
    //

    if (Irp->Flags & IRP_DEFER_IO_COMPLETION && !Irp->PendingReturned) {

        if ((Irp->IoStatus.Status == STATUS_REPARSE )  &&
            (Irp->IoStatus.Information == IO_REPARSE_TAG_MOUNT_POINT)) {

            //
            // For name junctions we reinstate the address of the appropriate
            // buffer. It is freed in parse.c
            //

            Irp->Tail.Overlay.AuxiliaryBuffer = saveAuxiliaryPointer;
        }

        return;
    }

    //
    // Finally, initialize the IRP as an APC structure and queue the special
    // kernel APC to the target thread.
    //

    thread = Irp->Tail.Overlay.Thread;
    fileObject = Irp->Tail.Overlay.OriginalFileObject;

    if (!Irp->Cancel) {

        KeInitializeApc( &Irp->Tail.Apc,
                         &thread->Tcb,
                         Irp->ApcEnvironment,
                         IopCompleteRequest,
                         IopAbortRequest,
                         (PKNORMAL_ROUTINE) NULL,
                         KernelMode,
                         (PVOID) NULL );

        (VOID) KeInsertQueueApc( &Irp->Tail.Apc,
                                 fileObject,
                                 (PVOID) saveAuxiliaryPointer,
                                 PriorityBoost );
    } else {

        //
        // This request has been cancelled.  Ensure that access to the thread
        // is synchronized, otherwise it may go away while attempting to get
        // through the remainder of completion for this request.  This happens
        // when the thread times out waiting for the request to be completed
        // once it has been cancelled.
        //
        // Note that it is safe to capture the thread pointer above, w/o having
        // the lock because the cancel flag was not set at that point, and
        // the code that disassociates IRPs must set the flag before looking to
        // see whether or not the packet has been completed, and this packet
        // will appear to be completed because it no longer belongs to a driver.
        //

        ExAcquireSpinLock( &IopCompletionLock, &irql );

        thread = Irp->Tail.Overlay.Thread;

        if (thread) {

            KeInitializeApc( &Irp->Tail.Apc,
                             &thread->Tcb,
                             Irp->ApcEnvironment,
                             IopCompleteRequest,
                             IopAbortRequest,
                             (PKNORMAL_ROUTINE) NULL,
                             KernelMode,
                             (PVOID) NULL );

            (VOID) KeInsertQueueApc( &Irp->Tail.Apc,
                                     fileObject,
                                     (PVOID) saveAuxiliaryPointer,
                                     PriorityBoost );

            ExReleaseSpinLock( &IopCompletionLock, irql );

        } else {

            //
            // This request has been aborted from completing in the caller's
            // thread.  This can only occur if the packet was cancelled, and
            // the driver did not complete the request, so it was timed out.
            // Attempt to drop things on the floor, since the originating thread
            // has probably exited at this point.
            //

            ExReleaseSpinLock( &IopCompletionLock, irql );

            ASSERT( Irp->Cancel );

            //
            // Drop the IRP on the floor.
            //

            IopDropIrp( Irp, fileObject );

        }
    }
}

NTSTATUS
IoConnectInterrupt(
    OUT PKINTERRUPT *InterruptObject,
    IN PKSERVICE_ROUTINE ServiceRoutine,
    IN PVOID ServiceContext,
    IN PKSPIN_LOCK SpinLock OPTIONAL,
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KIRQL SynchronizeIrql,
    IN KINTERRUPT_MODE InterruptMode,
    IN BOOLEAN ShareVector,
    IN KAFFINITY ProcessorEnableMask,
    IN BOOLEAN FloatingSave
    )

/*++

Routine Description:

    This routine allocates, initializes, and connects interrupt objects for
    all of the processors specified in the processor enable mask.

Arguments:

    InterruptObject - Address of a variable to receive a pointer to the first
        interrupt object allocated and initialized.

    ServiceRoutine - Address of the interrupt service routine (ISR) that should
        be executed when the interrupt occurs.

    ServiceContext - Supplies a pointer to the context information required
        by the ISR.

    SpinLock - Supplies a pointer to a spin lock to be used when synchronizing
        with the ISR.

    Vector - Supplies the vector upon which the interrupt occurs.

    Irql - Supplies the IRQL upon which the interrupt occurs.

    SynchronizeIrql - The request priority that the interrupt should be
        synchronized with.

    InterruptMode - Specifies the interrupt mode of the device.

    ShareVector - Supplies a boolean value that specifies whether the
        vector can be shared with other interrupt objects or not.  If FALSE
        then the vector may not be shared, if TRUE it may be.
        Latched.

    ProcessorEnableMask - Specifies a bit-vector for each processor on which
        the interrupt is to be connected.  A value of one in the bit position
        cooresponding to the processor number indicates that the interrupt
        should be allowed on that processor.  At least one bit must be set.

    FloatingSave - A BOOLEAN that specifies whether or not the machine's
        floating point state should be saved before invoking the ISR.

Return Value:

    The function value is the final function status.  The three status values
    that this routine can itself return are:

        STATUS_SUCCESS - Everything worked successfully.
        STATUS_INVALID_PARAMETER - No processors were specified.
        STATUS_INSUFFICIENT_RESOURCES - There was not enough nonpaged pool.

--*/

{
    CCHAR count;
    BOOLEAN builtinUsed;
    PKINTERRUPT interruptObject;
    ULONG processorMask;
    NTSTATUS status;
    PIO_INTERRUPT_STRUCTURE interruptStructure;
    PKSPIN_LOCK spinLock;

    PAGED_CODE();

    //
    // Initialize the return pointer and assume success.
    //

    *InterruptObject = (PKINTERRUPT) NULL;
    status = STATUS_SUCCESS;

    //
    // Determine how much memory is to be allocated based on how many
    // processors this system may have and how many bits are set in the
    // processor enable mask.
    //

    processorMask = ProcessorEnableMask & KeActiveProcessors;
    count = 0;

    while (processorMask) {
        if (processorMask & 1) {
            count++;
        }
        processorMask >>= 1;
    }

    //
    // If any interrupt objects are to be allocated and initialized, allocate
    // the memory now.
    //

    if (count) {
        interruptStructure = ExAllocatePoolWithTag( NonPagedPool,
                                                    ((count - 1) * sizeof( KINTERRUPT )) +
                                                    sizeof( IO_INTERRUPT_STRUCTURE ),
                                                    'nioI' );
        if (interruptStructure == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        status = STATUS_INVALID_PARAMETER;
    }

    //
    // If the caller specified a spin lock to be used for the interrupt object,
    // use it.  Otherwise, provide one by using the one in the structure that
    // was just allocated.
    //

    if (ARGUMENT_PRESENT( SpinLock )) {
        spinLock = SpinLock;
    } else {
        spinLock = &interruptStructure->SpinLock;
    }

    //
    // If the pool allocation was successful, initialize and connect the
    // interrupt objects to the appropriate processors.
    //

    if (NT_SUCCESS( status )) {

        //
        // Return the address of the first interrupt object in case an
        // interrupt is pending for the device when it is initially connected
        // and the driver must synchronize its execution with the ISR.
        //

        *InterruptObject = &interruptStructure->InterruptObject;

        //
        // Begin by getting a pointer to the start of the memory to be used
        // for interrupt objects other than the builtin object.
        //

        interruptObject = (PKINTERRUPT) (interruptStructure + 1);
        builtinUsed = FALSE;
        processorMask = ProcessorEnableMask & KeActiveProcessors;

        //
        // Now zero the interrupt structure itself so that if something goes
        // wrong it can be backed out.
        //

        RtlZeroMemory( interruptStructure, sizeof( IO_INTERRUPT_STRUCTURE ) );

        //
        // For each entry in the processor enable mask that is set, connect
        // and initialize an interrupt object.  The first bit that is set
        // uses the builtin interrupt object, and all others use the pointers
        // that follow it.
        //

        for (count = 0; processorMask; count++, processorMask >>= 1) {
            if (processorMask & 1) {
                KeInitializeInterrupt( builtinUsed ?
                                       interruptObject :
                                       &interruptStructure->InterruptObject,
                                       ServiceRoutine,
                                       ServiceContext,
                                       spinLock,
                                       Vector,
                                       Irql,
                                       SynchronizeIrql,
                                       InterruptMode,
                                       ShareVector,
                                       count,
                                       FloatingSave );

                if (!KeConnectInterrupt( builtinUsed ?
                                         interruptObject :
                                         &interruptStructure->InterruptObject )) {

                    //
                    // An error occurred while attempting to connect the
                    // interrupt.  This means that the driver either specified
                    // the wrong type of interrupt mode, or attempted to connect
                    // to some processor that didn't exist, or whatever.  In
                    // any case, the problem turns out to be an invalid
                    // parameter was specified.  Simply back everything out
                    // and return an error status.
                    //
                    // Note that if the builtin entry was used, then the entire
                    // structure needs to be walked as there are entries that
                    // were successfully connected.  Otherwise, the first
                    // attempt to connect failed, so simply free everything
                    // in-line.
                    //

                    if (builtinUsed) {
                        IoDisconnectInterrupt( &interruptStructure->InterruptObject );
                    } else {
                        ExFreePool( interruptStructure );
                    }
                    status = STATUS_INVALID_PARAMETER;
                    break;
                }


                //
                // If the builtin entry has been used, then the interrupt
                // object just connected was one of the pointers, so fill
                // it in with the address of the memory actually used.
                //

                if (builtinUsed) {
                    interruptStructure->InterruptArray[count] = interruptObject++;

                } else {

                    //
                    // Otherwise, the builtin entry was used, so indicate
                    // that it is no longer valid to use and start using
                    // the pointers instead.
                    //

                    builtinUsed = TRUE;
                }

            }
        }
    }

    //
    // Finally, reset the address of the interrupt object if the function
    // failed and return the final status of the operation.
    //

    if (!NT_SUCCESS( status )) {
        *InterruptObject = (PKINTERRUPT) NULL;
    }

    return status;
}

PCONTROLLER_OBJECT
IoCreateController(
    IN ULONG Size
    )

/*++

Routine Description:

    This routine creates a controller object that can be used to synchronize
    access to a physical device controller from two or more devices.

Arguments:

    Size - Size of the adapter extension in bytes.

Return Value:

    A pointer to the controller object that was created or a NULL pointer.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    PCONTROLLER_OBJECT controllerObject;
    HANDLE handle;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Initialize the object attributes structure in preparation for creating
    // the controller object.
    //

    InitializeObjectAttributes( &objectAttributes,
                                (PUNICODE_STRING) NULL,
                                0,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    //
    // Create the controller object itself.
    //

    status = ObCreateObject( KernelMode,
                             IoControllerObjectType,
                             &objectAttributes,
                             KernelMode,
                             (PVOID) NULL,
                             (ULONG) sizeof( CONTROLLER_OBJECT ) + Size,
                             0,
                             0,
                             (PVOID *) &controllerObject );
    if (NT_SUCCESS( status )) {

        //
        // Insert the controller object into the table.
        //

        status = ObInsertObject( controllerObject,
                                 NULL,
                                 FILE_READ_DATA | FILE_WRITE_DATA,
                                 1,
                                 (PVOID *) &controllerObject,
                                 &handle );

        //
        // If the insert operation fails, set return value to NULL.
        //

        if (!NT_SUCCESS( status )) {
            controllerObject = (PCONTROLLER_OBJECT) NULL;
        } else {

            //
            // The insert completed successfully.  Close the handle so that if
            // the driver is unloaded, the controller object can go away.
            //

            (VOID) NtClose( handle );

            //
            // Zero the memory for the controller object.
            //

            RtlZeroMemory( controllerObject, sizeof( CONTROLLER_OBJECT ) + Size );

            //
            // Set the type and size of this controller object.
            //

            controllerObject->Type = IO_TYPE_CONTROLLER;
            controllerObject->Size = (USHORT) (sizeof( CONTROLLER_OBJECT ) + Size);
            controllerObject->ControllerExtension = (PVOID) (controllerObject + 1);

            //
            // Finally, initialize the controller's device queue.
            //

            KeInitializeDeviceQueue( &controllerObject->DeviceWaitQueue );
        }
    } else {
        controllerObject = (PCONTROLLER_OBJECT) NULL;
    }

    return controllerObject;
}

VOID
IopInsertRemoveDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Insert
    )

{
    KIRQL irql;

    ExAcquireSpinLock( &IopDatabaseLock, &irql );
    if (Insert) {
        DeviceObject->NextDevice = DriverObject->DeviceObject;
        DriverObject->DeviceObject = DeviceObject;
        }
    else {
        PDEVICE_OBJECT *prevPoint;

        prevPoint = &DeviceObject->DriverObject->DeviceObject;
        while (*prevPoint != DeviceObject) {
            prevPoint = &(*prevPoint)->NextDevice;
        }
        *prevPoint = DeviceObject->NextDevice;
    }
    ExReleaseSpinLock( &IopDatabaseLock, irql );
}

VOID
IopCreateVpb (
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PVPB Vpb;

    Vpb = ExAllocatePoolWithTag(
                NonPagedPoolMustSucceed,
                sizeof( VPB ),
                ' bpV'
                );

    RtlZeroMemory (Vpb, sizeof(VPB));
    Vpb->Type = IO_TYPE_VPB;
    Vpb->Size = sizeof( VPB );
    Vpb->RealDevice = DeviceObject;
    DeviceObject->Vpb = Vpb;
}

NTSTATUS
IoCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG DeviceExtensionSize,
    IN PUNICODE_STRING DeviceName OPTIONAL,
    IN DEVICE_TYPE DeviceType,
    IN ULONG DeviceCharacteristics,
    IN BOOLEAN Exclusive,
    OUT PDEVICE_OBJECT *DeviceObject
    )

/*++

Routine Description:

    This routine creates a device object and links it into the I/O database.

Arguments:

    DriverObject - A pointer to the driver object for this device.

    DeviceExtensionSize - Size, in bytes, of extension to device object;
        i.e., the size of the driver-specific data for this device object.

    DeviceName - Optional name that should be associated with this device.
        If the DeviceCharacteristics has the FILE_AUTOGENERATED_DEVICE_NAME
        flag set, this parameter is ignored.

    DeviceType - The type of device that the device object should represent.

    DeviceCharacteristics - The characteristics for the device.

    Exclusive - Indicates that the device object should be created with using
        the exclusive object attribute.

        NOTE: This flag should not be used for WDM drivers.  Since only the
        PDO is named, it is the only device object in a devnode attachment
        stack that is openable.  However, since this device object is created
        by the underlying bus driver (which has no knowledge about what type
        of device this is), there is no way to know whether this flag should
        be set.  Therefore, this parameter should always be FALSE for WDM
        drivers.  Drivers attached to the PDO (e.g., the function driver) must
        enforce any exclusivity rules.

    DeviceObject - Pointer to the device object pointer we will return.

Return Value:

    The function value is the final status of the operation.


--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    PDEVICE_OBJECT deviceObject;
    PDEVOBJ_EXTENSION deviceObjectExt;
    HANDLE handle;
    BOOLEAN deviceHasName;
    CHAR localSecurityDescriptor[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR securityDescriptor;
    PACL acl;
    PACE_HEADER ace;
    PSID sid;
    PACCESS_MASK mask;
    ULONG i;
    ULONG RoundedSize;
    NTSTATUS status;
    USHORT sectorSize = 0;
    LONG nextUniqueDeviceObjectNumber;
    UNICODE_STRING autoGeneratedDeviceName;
    BOOLEAN retryWithNewName = FALSE;
    WCHAR deviceNameBuffer[17];             // "\Device\xxxxxxxx"

    PAGED_CODE();

    acl = NULL;

    //
    // Enclose the creation of the device object in a do/while, in the rare
    // event where we have to retry because some other driver is using our
    // naming scheme for auto-generated device object names.
    //

    do {

        if (DeviceCharacteristics & FILE_AUTOGENERATED_DEVICE_NAME) {

            //
            // The caller has requested that we automatically generate a device
            // object name.  Retrieve the next available number to use for this
            // purpose, and create a name of the form "\Device\<n>", where <n>
            // is the (8 hexadecimal digit) character representation of the unique
            // number we retrieve.
            //

            nextUniqueDeviceObjectNumber = InterlockedIncrement( &IopUniqueDeviceObjectNumber );
            swprintf( deviceNameBuffer, L"\\Device\\%08lx", nextUniqueDeviceObjectNumber );

            if (retryWithNewName) {

                //
                // We've already done this once (hence, the unicode device name string
                // is all set up, as is all the security information).  Thus, we can
                // skip down to where we re-attempt the creation of the device object
                // using our new name.
                //

                retryWithNewName = FALSE;
                goto attemptDeviceObjectCreation;

            } else {

                //
                // Set the DeviceName parameter to point to our unicode string, just as
                // if the caller had specified it (note, we explicitly ignore anything
                // the caller passes us for device name if the FILE_AUTOGENERATED_DEVICE_NAME
                // characteristic is specified.
                //

                RtlInitUnicodeString( &autoGeneratedDeviceName, deviceNameBuffer );
                DeviceName = &autoGeneratedDeviceName;
            }
        }

        //
        // Remember whether or not this device was created with a name so that
        // it can be deallocated later.
        //

        deviceHasName = (BOOLEAN) (ARGUMENT_PRESENT( DeviceName ) ? TRUE : FALSE);

        //
        // Detmermine whether or not this device needs to have a security descriptor
        // placed on it that allows read/write access, or whether the system default
        // should be used.  Disks, virtual disks, and file systems simply use the
        // system default descriptor.  All others allow read/write access.
        //
        // NOTE: This routine assumes that it is in the system's security context.
        //       In particular, it assumes that the Default DACL is the system's
        //       Default DACL.  If this assumption changes in future releases,
        //       then use of the Default DACL below should be reviewed for
        //       appropriateness.
        //

        //
        // If the device is a pnp device then wait until it registers a device
        // class before doing the default setup.
        //

        securityDescriptor = IopCreateDefaultDeviceSecurityDescriptor(
                                DeviceType,
                                DeviceCharacteristics,
                                deviceHasName,
                                localSecurityDescriptor,
                                &acl,
                                NULL);

        switch ( DeviceType ) {

        case FILE_DEVICE_DISK_FILE_SYSTEM:

            sectorSize = 512;
            break;

        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:

            sectorSize = 2048;
            break;

        case FILE_DEVICE_DISK:
        case FILE_DEVICE_VIRTUAL_DISK:

            sectorSize = 512;
            break;
        }

attemptDeviceObjectCreation:
        //
        // Initialize the object attributes structure in preparation for creating
        // device object.  Note that the device may be created as an exclusive
        // device so that only one open can be done to it at a time.  This saves
        // single user devices from having drivers that implement special code to
        // make sure that only one connection is ever valid at any given time.
        //

        InitializeObjectAttributes( &objectAttributes,
                                    DeviceName,
                                    0,
                                    (HANDLE) NULL,
                                    securityDescriptor );


        if (Exclusive) {
            objectAttributes.Attributes |= OBJ_EXCLUSIVE;
        } else {
            objectAttributes.Attributes |= 0;
        }

        if (deviceHasName) {
            objectAttributes.Attributes |= OBJ_PERMANENT;
        }

        //
        // Create the device object itself.
        //

        RoundedSize = (sizeof( DEVICE_OBJECT ) + DeviceExtensionSize)
                       % sizeof (LONGLONG);
        if (RoundedSize) {
            RoundedSize = sizeof (LONGLONG) - RoundedSize;
        }

        RoundedSize += DeviceExtensionSize;

        status = ObCreateObject( KernelMode,
                                 IoDeviceObjectType,
                                 &objectAttributes,
                                 KernelMode,
                                 (PVOID) NULL,
                                 (ULONG) sizeof( DEVICE_OBJECT ) + sizeof ( DEVOBJ_EXTENSION ) +
                                         RoundedSize,
                                 0,
                                 0,
                                 (PVOID *) &deviceObject );

        if ((status == STATUS_OBJECT_NAME_COLLISION) &&
            (DeviceCharacteristics & FILE_AUTOGENERATED_DEVICE_NAME)) {

            //
            // Some other driver is using our naming scheme, and we've picked a
            // device name already in use.  Try again, with a new number.
            //

            retryWithNewName = TRUE;
        }

    } while (retryWithNewName);

    if (!NT_SUCCESS( status )) {

        //
        // Creating the device object was not successful.  Clean everything
        // up and indicate that the object was not created.
        //

        deviceObject = (PDEVICE_OBJECT) NULL;

    } else {

        //
        // The device was successfully created.  Initialize the object so
        // that it can be inserted into the object table.  Begin by zeroing
        // the memory for the device object.
        //

        RtlZeroMemory( deviceObject,
                       sizeof( DEVICE_OBJECT ) + sizeof ( DEVOBJ_EXTENSION ) +
                       RoundedSize );

        //
        // Fill in deviceObject & deviceObjectExtension cross pointers
        //

        deviceObjectExt = (PDEVOBJ_EXTENSION)  (((PCHAR) deviceObject) +
                            sizeof (DEVICE_OBJECT) + RoundedSize);

        deviceObjectExt->DeviceObject = deviceObject;
        deviceObject->DeviceObjectExtension = deviceObjectExt;

        //
        // Initialize deviceObjectExt
        // Note: the size of a Device Object Extension is initialized specifically
        // to ZERO so no driver will depend on it.
        //

        deviceObjectExt->Type = IO_TYPE_DEVICE_OBJECT_EXTENSION;
        deviceObjectExt->Size = 0;

        PoInitializeDeviceObject(deviceObjectExt);

        //
        // Set the type and size of this device object.
        //

        deviceObject->Type = IO_TYPE_DEVICE;
        deviceObject->Size = (USHORT) (sizeof( DEVICE_OBJECT ) + DeviceExtensionSize);

        //
        // Set the device type field in the object so that later code can
        // check the type.  Likewise, set the device characteristics.
        //

        deviceObject->DeviceType = DeviceType;
        deviceObject->Characteristics = DeviceCharacteristics;

        //
        // If this device is either a tape or a disk, allocate a Volume
        // Parameter Block (VPB) which indicates that the volume has
        // never been mounted, and set the device object's VPB pointer to
        // it.
        //

        if ((DeviceType == FILE_DEVICE_DISK) ||
            (DeviceType == FILE_DEVICE_TAPE) ||
            (DeviceType == FILE_DEVICE_CD_ROM) ||
            (DeviceType == FILE_DEVICE_VIRTUAL_DISK)) {

            IopCreateVpb (deviceObject);
            KeInitializeEvent( &deviceObject->DeviceLock,
                               SynchronizationEvent,
                               TRUE );
        }

        //
        // Initialize the remainder of the device object.
        //
        deviceObject->AlignmentRequirement = HalGetDmaAlignmentRequirement() - 1;
        deviceObject->SectorSize = sectorSize;
        deviceObject->Flags = DO_DEVICE_INITIALIZING;

        if (Exclusive) {
            deviceObject->Flags |= DO_EXCLUSIVE;
        }
        if (deviceHasName) {
            deviceObject->Flags |= DO_DEVICE_HAS_NAME;
        }

        if(DeviceExtensionSize) {
            deviceObject->DeviceExtension = deviceObject + 1;
        } else {
            deviceObject->DeviceExtension = NULL;
        }

        deviceObject->StackSize = 1;
        switch ( DeviceType ) {

        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
        case FILE_DEVICE_DISK_FILE_SYSTEM:
        case FILE_DEVICE_FILE_SYSTEM:
        case FILE_DEVICE_NETWORK_FILE_SYSTEM:
        case FILE_DEVICE_TAPE_FILE_SYSTEM:

            //
            // This device represents a file system of some sort.  Simply
            // initialize the queue list head in the device object.
            //

            InitializeListHead( &deviceObject->Queue.ListEntry );
            break;

        default:

            //
            // This is a real device of some sort.  Allocate a spin lock
            // and initialize the device queue object in the device object.
            //

            KeInitializeDeviceQueue( &deviceObject->DeviceQueue );
            break;
        }

        //
        // Insert the device object into the table.
        //

        status = ObInsertObject( deviceObject,
                                 NULL,
                                 FILE_READ_DATA | FILE_WRITE_DATA,
                                 1,
                                 (PVOID *) &deviceObject,
                                 &handle );

        if (NT_SUCCESS( status )) {

            //
            // Reference the driver object.   When the device object goes
            // away the reference will be removed.  This prevents the
            // driver object and driver image from going away while the
            // device object is in the pending delete state.
            //

            ObReferenceObject( DriverObject );

            ASSERT((DriverObject->Flags & DRVO_UNLOAD_INVOKED) == 0);

            //
            // The insert completed successfully.  Link the device object
            // and driver objects together.  Close the handle so that if
            // the driver is unloaded, the device object can go away.
            //

            deviceObject->DriverObject = DriverObject;

            IopInsertRemoveDevice( DriverObject, deviceObject, TRUE );
            if (deviceObject->Vpb) {
                PoVolumeDevice(deviceObject);
            }

            (VOID) NtClose( handle );

        } else {

            //
            // The insert operation failed.  Fortunately it dropped the
            // reference count on the device - since that was the last one
            // all the cleanup should be done for us.
            //

            //
            // indicate that no device object was created.
            //

            deviceObject = (PDEVICE_OBJECT) NULL;
        }
    }

    //
    // Free the DACL if we allocated it...
    //

    if (acl != NULL) {
        ExFreePool( acl );
    }

    *DeviceObject = deviceObject;
    return status;
}

NTSTATUS
IoCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG Disposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    IN CREATE_FILE_TYPE CreateFileType,
    IN PVOID ExtraCreateParameters OPTIONAL,
    IN ULONG Options
    )

/*++

Routine Description:

    This is the common routine for both NtCreateFile and NtOpenFile to allow
    a user to create or open a file.  This procedure is also used internally
    by kernel mode components, such as the network server, to perform the
    same type of operation, but allows kernel mode code to force checking
    arguments and access to the file, rather than bypassing these checks
    because the code is running in kernel mode.

Arguments:

    FileHandle - A pointer to a variable to receive the handle to the open
        file.

    DesiredAccess - Supplies the types of access that the caller would like
        to the file.

    ObjectAttributes - Supplies the attributes to be used for the file object
        (name, SECURITY_DESCRIPTOR, etc.)

    IoStatusBlock - Specifies the address of the caller's I/O status block.

    AllocationSize - Initial size that should be allocated to the file.
        This parameter only has an affect if the file is created.  Further,
        if not specified, then it is taken to mean zero.

    FileAttributes - Specifies the attributes that should be set on the file,
        if it is created.

    ShareAccess - Supplies the types of share access that the caller would
        like to the file.

    Disposition - Supplies the method for handling the create/open.

    CreateOptions - Caller options for how to perform the create/open.

    EaBuffer - Optionally specifies a set of EAs to be applied to the file
        if it is created.

    EaLength - Supplies the length of the EaBuffer.

    CreateFileType - The type of file to create.

    ExtraCreateParameters - Optionally specifies a pointer to extra create
        parameters.  The format of the parameters depends on the value of
        CreateFileType.

    Options - Specifies the options that are to be used during generation
        of the create IRP.

Return Value:

    The function value is the final status of the create/open operation.

Warning:

    If a pointer to ExtraCreateParameters is passed the data must be
    readable from kernel mode.


--*/

{
    KPROCESSOR_MODE requestorMode;
    NTSTATUS status;
    HANDLE handle;
    OPEN_PACKET openPacket;
    BOOLEAN SuccessfulIoParse;
    LARGE_INTEGER initialAllocationSize;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    if (Options & IO_NO_PARAMETER_CHECKING) {
        requestorMode = KernelMode;
    }

    if (requestorMode != KernelMode || Options & IO_CHECK_CREATE_PARAMETERS) {

        //
        // Check for any invalid parameters.
        //

        if (

            //
            // Check that no invalid file attributes flags were specified.
            //

//          (FileAttributes & ~FILE_ATTRIBUTE_VALID_SET_FLAGS)
            (FileAttributes & ~FILE_ATTRIBUTE_VALID_FLAGS)

            ||

            //
            // Check that no invalid share access flags were specified.
            //

            (ShareAccess & ~FILE_SHARE_VALID_FLAGS)

            ||

            //
            // Ensure that the disposition value is in range.
            //

            (Disposition > FILE_MAXIMUM_DISPOSITION)

            ||

            //
            // Check that no invalid create options were specified.
            //

            (CreateOptions & ~FILE_VALID_OPTION_FLAGS)

            ||

            //
            // If the caller specified synchronous I/O, then ensure that
            // (s)he also asked for synchronize desired access to the
            // file.
            //

            (CreateOptions & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT) &&
            (!(DesiredAccess & SYNCHRONIZE)))

            ||

            //
            // Also, if the caller specified that the file is to be deleted
            // on close, then ensure that delete is specified as one of the
            // desired accesses requested.
            //

            (CreateOptions & FILE_DELETE_ON_CLOSE &&
            (!(DesiredAccess & DELETE)))

            ||

            //
            // Likewise, ensure that if one of the synchronous I/O modes
            // is specified that the other one is not specified as well.
            //

            ((CreateOptions & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT)) ==
                              (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT))

            ||

            //
            // If this create or open is for a directory operation, check
            // that all of the other flags, dispositions, and desired
            // access parameters were also specified correctly.
            //
            // These are as follows:
            //
            //     o  No other flags other than the synchronous I/O flags,
            //        write-through, or open by file ID are set.
            //
            //     o  The disposition value is one of create, open, or
            //        open-if.
            //
            //     o  No non-directory accesses have been specified.
            //

            ((CreateOptions & FILE_DIRECTORY_FILE)
             && !(CreateOptions & FILE_NON_DIRECTORY_FILE)
             && ((CreateOptions & ~(FILE_DIRECTORY_FILE |
                                    FILE_SYNCHRONOUS_IO_ALERT |
                                    FILE_SYNCHRONOUS_IO_NONALERT |
                                    FILE_WRITE_THROUGH |
                                    FILE_COMPLETE_IF_OPLOCKED |
                                    FILE_OPEN_FOR_BACKUP_INTENT |
                                    FILE_DELETE_ON_CLOSE |
                                    FILE_OPEN_FOR_FREE_SPACE_QUERY |
                                    FILE_OPEN_BY_FILE_ID |
                                    FILE_OPEN_REPARSE_POINT))
                 || ((Disposition != FILE_CREATE)
                     && (Disposition != FILE_OPEN)
                     && (Disposition != FILE_OPEN_IF))
                )
            )

            ||

            //
            //  FILE_COMPLETE_IF_OPLOCK and FILE_RESERVE_OPFILTER are
            //  incompatible options.
            //

            ((CreateOptions & FILE_COMPLETE_IF_OPLOCKED) &&
             (CreateOptions & FILE_RESERVE_OPFILTER))

            ||

            //
            // Finally, if the no intermediate buffering option was
            // specified, ensure that the caller did not also request
            // append access to the file.
            //

            (CreateOptions & FILE_NO_INTERMEDIATE_BUFFERING &&
            (DesiredAccess & FILE_APPEND_DATA)) ) {

            return STATUS_INVALID_PARAMETER;
        }

        //
        // Check the file type specific creation parameters.
        //

        if (CreateFileType == CreateFileTypeNone) {

            NOTHING;

        } else if (CreateFileType == CreateFileTypeNamedPipe) {

            if (!ARGUMENT_PRESENT( ExtraCreateParameters ) ) {

                return STATUS_INVALID_PARAMETER;

            } else {

                PNAMED_PIPE_CREATE_PARAMETERS NamedPipeCreateParameters;

                NamedPipeCreateParameters = ExtraCreateParameters;

                //
                // Check the parameters for creating a named pipe to
                // ensure that no invalid parameters were passed.
                //

                if (

                    //
                    // Check the NamedPipeType field to ensure that it
                    // is within range.
                    //

                    (NamedPipeCreateParameters->NamedPipeType >
                        FILE_PIPE_MESSAGE_TYPE)

                    ||

                    //
                    // Check the ReadMode field to ensure that it is
                    // within range.
                    //

                    (NamedPipeCreateParameters->ReadMode >
                        FILE_PIPE_MESSAGE_MODE)

                    ||

                    //
                    // Check the CompletionMode field to ensure that
                    // it is within range.
                    //

                    (NamedPipeCreateParameters->CompletionMode >
                        FILE_PIPE_COMPLETE_OPERATION)

                    ||

                    //
                    // Check the ShareAccess parameter to ensure that
                    // it does not specify shared delete access.  The
                    // Named Pipe File System itself will need to ensure
                    // that at least one of SHARE_READ or SHARE_WRITE
                    // is specified if the first instance of the pipe
                    // is being created.
                    //

                    (ShareAccess & FILE_SHARE_DELETE)

                    ||

                    //
                    // Check the Disposition parameter to ensure that
                    // is does not specify anything other than create,
                    // open, or open if.
                    //

                    (Disposition < FILE_OPEN || Disposition > FILE_OPEN_IF)

                    ||

                    //
                    // Finally, check the CreateOptions parameter to
                    // ensure that it does not contain any invalid
                    // options for named pipes.
                    //

                    (CreateOptions & ~FILE_VALID_PIPE_OPTION_FLAGS)) {
                    return STATUS_INVALID_PARAMETER;
                }

            }

        } else if (CreateFileType == CreateFileTypeMailslot) {

            if (!ARGUMENT_PRESENT( ExtraCreateParameters ) ) {

                return STATUS_INVALID_PARAMETER;

            } else {

                PMAILSLOT_CREATE_PARAMETERS mailslotCreateParameters;

                mailslotCreateParameters = ExtraCreateParameters;

                //
                // Check the parameters for creating a mailslot to ensure
                // that no invalid parameters were passed.
                //

                if (

                    //
                    // Check the ShareAccess parameter to ensure that
                    // it does not specify shared delete access.
                    //

                    (ShareAccess & FILE_SHARE_DELETE)

                    ||

                    //
                    // Check the ShareAccess parameter to ensure that
                    // shared write access is specified.
                    //

                    !(ShareAccess & ~FILE_SHARE_WRITE)

                    ||

                    //
                    // Check the Disposition parameter to ensure that
                    // it specifies that the file is to be created.
                    //

                    (Disposition != FILE_CREATE)

                    ||

                    //
                    // Check the CreateOptions parameter to ensure that
                    // it does not contain any options that are invalid
                    // for mailslots.
                    //

                    (CreateOptions & ~FILE_VALID_MAILSLOT_OPTION_FLAGS)) {
                    return STATUS_INVALID_PARAMETER;
                }
            }
        }
    }

    if (requestorMode != KernelMode) {

        //
        // The caller's access mode is not kernel so probe each of the
        // arguments and capture them as necessary.  If any failures occur,
        // the condition handler will be invoked to handle them.  It will
        // simply cleanup and return an access violation status code back
        // to the system service dispatcher.
        //

        openPacket.EaBuffer = (PFILE_FULL_EA_INFORMATION) NULL;

        try {

            //
            // The FileHandle parameter must be writeable by the caller.
            // Probe it for a write operation.
            //

            ProbeAndWriteHandle( FileHandle, 0L );

            //
            // The IoStatusBlock parameter must be writeable by the caller.
            //

            ProbeForWriteIoStatus( IoStatusBlock );

            //
            // The AllocationSize parameter must be readable by the caller
            // if it is present.  If so, probe and capture it.
            //

            if (ARGUMENT_PRESENT( AllocationSize )) {
                ProbeForRead( AllocationSize,
                              sizeof( LARGE_INTEGER ),
                              sizeof( ULONG ) );
                initialAllocationSize = *AllocationSize;
            } else {
                initialAllocationSize.QuadPart = 0;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred while attempting to access the
            // caller's parameters.  Simply return the reason for the
            // exception as the service status.
            //

            return GetExceptionCode();
        }

        //
        // Finally, if an EaBuffer was specified, ensure that it is readable
        // from the caller's mode and capture it.
        //

        if (ARGUMENT_PRESENT( EaBuffer ) && EaLength) {

            ULONG errorOffset;

            try {

                ProbeForRead( EaBuffer, EaLength, sizeof( ULONG ) );
                openPacket.EaBuffer = ExAllocatePoolWithQuotaTag( NonPagedPool,
                                                                  EaLength,
                                                                  'aEoI' );
                openPacket.EaLength = EaLength;
                RtlCopyMemory( openPacket.EaBuffer, EaBuffer, EaLength );

                //
                // Walk the buffer and ensure that its format is valid.  Note
                // that has been probed.
                //

                status = IoCheckEaBufferValidity( openPacket.EaBuffer,
                                                  EaLength,
                                                  &errorOffset );

                if (!NT_SUCCESS( status )) {
                    IoStatusBlock->Status = status;
                    IoStatusBlock->Information = errorOffset;
                    ExRaiseStatus( status );
                }

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception was incurred while attempting to access the
                // caller's parameters.  Check to see whether or not an EA
                // buffer was allocated and deallocate if so.
                //

                if (openPacket.EaBuffer != NULL) {
                    ExFreePool( openPacket.EaBuffer );
                }

                return GetExceptionCode();

            }

        } else {

            //
            // No EAs were specified.
            //

            openPacket.EaBuffer = (PVOID) NULL;
            openPacket.EaLength = 0L;
        }

    } else {

        //
        // The caller's mode is kernel.  Copy the input parameters to their
        // expected locations for later use.  Also, put move attach device
        // flag where it belongs.
        //

        if (CreateOptions & IO_ATTACH_DEVICE_API) {
            Options |= IO_ATTACH_DEVICE;
            CreateOptions &= ~IO_ATTACH_DEVICE_API;

        }

        if (ARGUMENT_PRESENT( AllocationSize )) {
            initialAllocationSize = *AllocationSize;
        } else {
            initialAllocationSize.QuadPart = 0;
        }

        if (ARGUMENT_PRESENT( EaBuffer ) && EaLength) {

            ULONG errorOffset;

            openPacket.EaBuffer = ExAllocatePoolWithTag( NonPagedPool,
                                                         EaLength,
                                                         'aEoI' );
            if (!openPacket.EaBuffer) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            openPacket.EaLength = EaLength;
            RtlCopyMemory( openPacket.EaBuffer, EaBuffer, EaLength );

            //
            // Walk the buffer and ensure that its format is valid.  Note
            // that has been probed.
            //

            status = IoCheckEaBufferValidity( openPacket.EaBuffer,
                                              EaLength,
                                              &errorOffset );

            if (!NT_SUCCESS( status )) {
                ExFreePool(openPacket.EaBuffer);
                IoStatusBlock->Status = status;
                IoStatusBlock->Information = errorOffset;
                return status;
            }

        } else {
            openPacket.EaBuffer = (PVOID) NULL;
            openPacket.EaLength = 0L;
        }
    }

    //
    // Now fill in an Open Packet (OP) to be used in calling the device object
    // parse routine.  This packet will allow information to be passed between
    // this routine and the parse routine so that a common context may be kept.
    // For most services this would be done with an I/O Request Packet (IRP),
    // but this cannot be done here because the number of stack entries which
    // need to be allocated in the IRP is not yet known.
    //

    openPacket.Type = IO_TYPE_OPEN_PACKET;
    openPacket.Size = sizeof( OPEN_PACKET );
    openPacket.ParseCheck = 0L;
    openPacket.AllocationSize = initialAllocationSize;
    openPacket.CreateOptions = CreateOptions;
    openPacket.FileAttributes = (USHORT) FileAttributes;
    openPacket.ShareAccess = (USHORT) ShareAccess;
    openPacket.Disposition = Disposition;
    openPacket.Override = FALSE;
    openPacket.QueryOnly = FALSE;
    openPacket.DeleteOnly = FALSE;
    openPacket.Options = Options;
    openPacket.RelatedFileObject = (PFILE_OBJECT) NULL;
    openPacket.CreateFileType = CreateFileType;
    openPacket.ExtraCreateParameters = ExtraCreateParameters;

    //
    // Assume that the operation is going to be successful.
    //

    openPacket.FinalStatus = STATUS_SUCCESS;

    //
    // Zero the file object field in the OP so the parse routine knows that
    // this is the first time through.  For reparse operations it will continue
    // to use the same file object that it allocated the first time.
    //

    openPacket.FileObject = (PFILE_OBJECT) NULL;

    //
    // Update the open count for this process.
    //

    IopUpdateOtherOperationCount();

    //
    // Attempt to open the file object by name.  This will yield the handle
    // that the user is to use as his handle to the file in all subsequent
    // calls, if it works.
    //
    // This call performs a whole lot of the work for actually getting every-
    // thing set up for the I/O system.  The object manager will take the name
    // of the file and will translate it until it reaches a device object (or
    // it fails).  If the former, then it will invoke the parse routine set up
    // by the I/O system for device objects.  This routine will actually end
    // up creating the file object, allocating an IRP, filling it in, and then
    // invoking the driver's dispatch routine with the packet.
    //

    status = ObOpenObjectByName( ObjectAttributes,
                                 (POBJECT_TYPE) NULL,
                                 requestorMode,
                                 NULL,
                                 DesiredAccess,
                                 &openPacket,
                                 &handle );

    //
    // If an EA buffer was allocated, deallocate it now before attempting to
    // determine whether or not the operation was successful so that it can be
    // done in one place rather than in two places.
    //

    if (openPacket.EaBuffer) {
        ExFreePool( openPacket.EaBuffer );
    }

    //
    // Check the status of the open.  If it was not successful, cleanup and
    // get out.  Notice that it is also possible, because this code does not
    // explicitly request that a particular type of object (because the Object
    // Manager does not check when a parse routine is present and because the
    // name first refers to a device object and then a file object), a check
    // must be made here to ensure that what was returned was really a file
    // object.  The check is to see whether the device object parse routine
    // believes that it successfully returned a pointer to a file object.  If
    // it does, then OK;  otherwise, something went wrong somewhere.
    //

    SuccessfulIoParse = (BOOLEAN) (openPacket.ParseCheck == OPEN_PACKET_PATTERN);

    if (!NT_SUCCESS( status ) || !SuccessfulIoParse) {

        if (NT_SUCCESS( status )) {

            //
            // The operation was successful as far as the object system is
            // concerned, but the I/O system device parse routine was never
            // successfully completed so this operation has actually completed
            // with an error because of an object mismatch.  Therefore, this is
            // the wrong type of object so dereference whatever was actually
            // referenced by closing the handle that was created for it.
            // We have to do a ZwClose as this handle can be a kernel handle if
            // IoCreateFile was called by a driver.
            //

            ZwClose( handle );
            status = STATUS_OBJECT_TYPE_MISMATCH;
        }

        //
        // If the final status according to the device parse routine
        // indicates that the operation was not successful, then use that
        // routine's final status because it is more descriptive than the
        // status which was returned by the object manager.
        //

        if (!NT_SUCCESS( openPacket.FinalStatus )) {
            status = openPacket.FinalStatus;

            if (NT_WARNING( status )) {

                try {

                    IoStatusBlock->Status = openPacket.FinalStatus;
                    IoStatusBlock->Information = openPacket.Information;

                } except(EXCEPTION_EXECUTE_HANDLER) {

                    status = GetExceptionCode();

                }

            }

        } else if (openPacket.FileObject != NULL && !SuccessfulIoParse) {

            //
            // Otherwise, one of two things occurred:
            //
            //     1)  The parse routine was invoked at least once and a
            //         reparse was performed but the parse routine did not
            //         actually complete.
            //
            //     2)  The parse routine was successful so everything worked
            //         but the object manager incurred an error after the
            //         parse routine completed.
            //
            // For case #1, there is an outstanding file object that still
            // exists.  This must be cleaned up.
            //
            // For case #2, nothing must be done as the object manager has
            // already dereferenced the file object.  Note that this code is
            // not invoked if the parse routine completed with a successful
            // status return (SuccessfulIoParse is TRUE).
            //

            if (openPacket.FileObject->FileName.Length != 0) {
                ExFreePool( openPacket.FileObject->FileName.Buffer );
            }
            openPacket.FileObject->DeviceObject = (PDEVICE_OBJECT) NULL;
            ObDereferenceObject( openPacket.FileObject );
        }

        //
        // When an NTFS file junction or an NTFS directory junction is traversed
        // OBJ_MAX_REPARSE_ATTEMPTS namy times, the object manager gives up and
        // returns the code STATUS_OBJECT_NAME_NOT_FOUND.
        //
        // This can happen in the following cases:
        //
        //      1) One encounters a legal chain of directory junctions that happen
        //         to be longer than the value of the above constant.
        //
        //      2) One encounters a self-referential file or directory junction that
        //         is, in effect, a tight name cycle.
        //
        //      3) One encounters a name cycle composed of several NTFS junctions.
        //
        // To improve on this return code see if  openPacket.Information  is
        // the trace of an NTFS name junction.
        //

        if ((status == STATUS_OBJECT_NAME_NOT_FOUND) &&
            (openPacket.Information == IO_REPARSE_TAG_MOUNT_POINT)) {

            status = STATUS_REPARSE_POINT_NOT_RESOLVED;
        }

    } else {

        //
        // At this point, the open/create operation has been successfully
        // completed.  There is a handle to the file object, which has been
        // created, and the file object has been signaled.
        //
        // The remaining work to be done is to complete the operation.  This is
        // performed as follows:
        //
        //    1.  The file object has been signaled, so no work needs to be done
        //        for it.
        //
        //    2.  The file handle is returned to the user.
        //
        //    3.  The I/O status block is written with the final status.
        //

        openPacket.FileObject->Flags |= FO_HANDLE_CREATED;

        ASSERT( openPacket.FileObject->Type == IO_TYPE_FILE );

        try {

            //
            // Return the file handle.
            //

            *FileHandle = handle;

            //
            // Write the I/O status into the caller's buffer.
            //

            IoStatusBlock->Information = openPacket.Information;
            IoStatusBlock->Status = openPacket.FinalStatus;
            status = openPacket.FinalStatus;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            status = GetExceptionCode();

        }

    }

    //
    // If the parse routine successfully created a file object then
    // derefence it here.
    //

    if (SuccessfulIoParse && openPacket.FileObject != NULL) {
        ObDereferenceObject( openPacket.FileObject );
    }

    return status;
}

PKEVENT
IoCreateNotificationEvent(
    IN PUNICODE_STRING EventName,
    OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    This routine creates a named notification event for use in notifying
    different system components or drivers that an event occurred.

Arguments:

    EventName - Supplies the full name of the event.

    EventHandle - Supplies a location to return a handle to the event.

Return Value:

    The function value is a pointer to the created/opened event, or NULL if
    the event could not be created/opened.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS status;
    HANDLE eventHandle;
    PKEVENT eventObject;

    PAGED_CODE();

    //
    // Begin by initializing the object attributes.
    //

    InitializeObjectAttributes( &objectAttributes,
                                EventName,
                                OBJ_OPENIF,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    //
    // Now create or open the event.
    //

    status = ZwCreateEvent( &eventHandle,
                            EVENT_ALL_ACCESS,
                            &objectAttributes,
                            NotificationEvent,
                            TRUE );
    if (!NT_SUCCESS( status )) {
        return (PKEVENT) NULL;
    }

    //
    // Reference the object by its handle to get a pointer that can be returned
    // to the caller.
    //

    (VOID) ObReferenceObjectByHandle( eventHandle,
                                      0,
                                      ExEventObjectType,
                                      KernelMode,
                                      (PVOID *) &eventObject,
                                      NULL );
    ObDereferenceObject( eventObject );

    //
    // Return the handle and the pointer to the event.
    //

    *EventHandle = eventHandle;

    return eventObject;
}

PFILE_OBJECT
IoCreateStreamFileObject(
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked to create a new file object that represents an
    alternate data stream for an existing file object.  The input file object
    represents the file object that already exists for a file, and the newly
    created stream file object is used to access other parts of the file
    other than the data.  Some uses of stream file objects are the EAs or
    the SECURITY_DESCRIPTORs on the file.  The stream file object allows
    the file system to cache these parts of the file just as if they were
    an entire to themselves.

    It is also possible to use stream file objects to represent virtual
    volume files.  This allows various parts of the on-disk structure to
    be viewed as a virtual file and therefore be cached using the same logic
    in the file system.  For this case, the device object pointer is used
    to create the file object.

Arguments:

    FileObject - Pointer to the file object to which the new stream file
        is related.  This pointer is optional.

    DeviceObject - Pointer to the device object on which the stream file
        is to be opened.  This pointer is not optional if the FileObject
        pointer is not specified.

Return Value:

    The function value is a pointer to the newly created stream file object.

--*/

{
    PFILE_OBJECT newFileObject;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE handle;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Begin by getting the device object from either the file object or
    // the device object parameter.
    //

    if (ARGUMENT_PRESENT( FileObject )) {
        DeviceObject = FileObject->DeviceObject;
    }

    //
    // Increment the reference count for the target device object.  Note
    // that no check is made to determine whether or not the device driver
    // is attempting to unload since this is an implicit open of a pseudo-
    // file that is being made, not a real file open request.  In essence,
    // no new file is really being opened.
    //

    ExInterlockedAddUlong( &DeviceObject->ReferenceCount, 1, &IopDatabaseLock );

    //
    // Initialize the object attributes that will be used to create the file
    // object.
    //

    InitializeObjectAttributes( &objectAttributes,
                                (PUNICODE_STRING) NULL,
                                0,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    //
    // Create the new file object.
    //

    status = ObCreateObject( KernelMode,
                             IoFileObjectType,
                             &objectAttributes,
                             0,
                             (PVOID) NULL,
                             (ULONG) sizeof( FILE_OBJECT ),
                             (ULONG) sizeof( FILE_OBJECT ),
                             0,
                             (PVOID *) &newFileObject );

    if (!NT_SUCCESS( status )) {
        IopDecrementDeviceObjectRef( DeviceObject, FALSE );
        ExRaiseStatus( status );
    }

    //
    // Initialize the common fields of the file object.
    //

    RtlZeroMemory( newFileObject, sizeof( FILE_OBJECT ) );
    newFileObject->Type = IO_TYPE_FILE;
    newFileObject->Size = sizeof( FILE_OBJECT );
    newFileObject->DeviceObject = DeviceObject;
    newFileObject->Flags = FO_STREAM_FILE;
    KeInitializeEvent( &newFileObject->Event, SynchronizationEvent, FALSE );

    //
    // Insert the device object into the table.  Note that this is done w/a
    // pointer bias so that the object cannot go away if some random user
    // application closes the handle before this code is finished w/it.
    //

    status = ObInsertObject( newFileObject,
                             NULL,
                             FILE_READ_DATA,
                             1,
                             (PVOID *) &newFileObject,
                             &handle );

    if (!NT_SUCCESS( status )) {
        ExRaiseStatus( status );
    }

    //
    // The insert completed successfully.  Update the bookkeeping so that the
    // fact that there is a handle is reflected.
    //

    newFileObject->Flags |= FO_HANDLE_CREATED;
    ASSERT( newFileObject->Type == IO_TYPE_FILE );

    //
    // Synchronize here with the file system to make sure that
    // volumes don't go away while en route to the FS.
    //

    if (DeviceObject->Vpb) {

        ExInterlockedAddUlong( &DeviceObject->Vpb->ReferenceCount,
                               1,
                               &IopVpbSpinLock );
    }

    //
    // Finally, close the handle to the file. and clear the forward cluster
    //

    status = NtClose( handle );

    ASSERT( NT_SUCCESS( status ) );

    return newFileObject;
}


PFILE_OBJECT
IoCreateStreamFileObjectLite(
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked to create a new file object that represents an
    alternate data stream for an existing file object.  The input file object
    represents the file object that already exists for a file, and the newly
    created stream file object is used to access other parts of the file
    other than the data.  Some uses of stream file objects are the EAs or
    the SECURITY_DESCRIPTORs on the file.  The stream file object allows
    the file system to cache these parts of the file just as if they were
    an entire to themselves.

    It is also possible to use stream file objects to represent virtual
    volume files.  This allows various parts of the on-disk structure to
    be viewed as a virtual file and therefore be cached using the same logic
    in the file system.  For this case, the device object pointer is used
    to create the file object.

    This call differs from IoCreateStreamFileObject in that it performs no
    handle management and does not result in a call to the file system
    cleanup entry.

Arguments:

    FileObject - Pointer to the file object to which the new stream file
        is related.  This pointer is optional.

    DeviceObject - Pointer to the device object on which the stream file
        is to be opened.  This pointer is not optional if the FileObject
        pointer is not specified.

Return Value:

    The function value is a pointer to the newly created stream file object.

--*/

{
    PFILE_OBJECT newFileObject;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE handle;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Begin by getting the device object from either the file object or
    // the device object parameter.
    //

    if (ARGUMENT_PRESENT( FileObject )) {
        DeviceObject = FileObject->DeviceObject;
    }

    //
    // if the driver has been marked for an unload or deleted operation, and
    // the reference count goes to zero, then the driver may need to be
    // unloaded or deleted at this point.
    // file that is being made, not a real file open request.  In essence,
    // no new file is really being opened.
    //

    ExInterlockedAddUlong( &DeviceObject->ReferenceCount, 1, &IopDatabaseLock );

    //
    // Initialize the object attributes that will be used to create the file
    // object.
    //

    InitializeObjectAttributes( &objectAttributes,
                                (PUNICODE_STRING) NULL,
                                0,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    //
    // Create the new file object.
    //

    status = ObCreateObject( KernelMode,
                             IoFileObjectType,
                             &objectAttributes,
                             0,
                             (PVOID) NULL,
                             (ULONG) sizeof( FILE_OBJECT ),
                             (ULONG) sizeof( FILE_OBJECT ),
                             0,
                             (PVOID *) &newFileObject );

    if (!NT_SUCCESS( status )) {
        IopDecrementDeviceObjectRef( DeviceObject, FALSE );
        ExRaiseStatus( status );
    }

    //
    // Initialize the common fields of the file object.
    //

    RtlZeroMemory( newFileObject, sizeof( FILE_OBJECT ) );
    newFileObject->Type = IO_TYPE_FILE;
    newFileObject->Size = sizeof( FILE_OBJECT );
    newFileObject->DeviceObject = DeviceObject;
    newFileObject->Flags = FO_STREAM_FILE;
    KeInitializeEvent( &newFileObject->Event, SynchronizationEvent, FALSE );

    //
    //  Clean up from the creation.
    //

    ObFreeObjectCreateInfoBuffer(OBJECT_TO_OBJECT_HEADER(newFileObject)->ObjectCreateInfo);
    OBJECT_TO_OBJECT_HEADER(newFileObject)->ObjectCreateInfo = NULL;

    newFileObject->Flags |= FO_HANDLE_CREATED;
    ASSERT( newFileObject->Type == IO_TYPE_FILE );

    //
    // Synchronize here with the file system to make sure that
    // volumes don't go away while en route to the FS.
    //

    if (DeviceObject->Vpb) {

        ExInterlockedAddUlong( &DeviceObject->Vpb->ReferenceCount,
                               1,
                               &IopVpbSpinLock );
    }

    return newFileObject;
}




NTSTATUS
IoCreateSymbolicLink(
    IN PUNICODE_STRING SymbolicLinkName,
    IN PUNICODE_STRING DeviceName
    )

/*++

Routine Description:

    This routine is invoked to assign a symbolic link name to a device.

Arguments:

    SymbolicLinkName - Supplies the symbolic link name as a Unicode string.

    DeviceName - Supplies the name to which the symbolic link name refers.

Return Value:

    The function value is the final status of the operation.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE linkHandle;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Begin by initializing the object attributes for the symbolic link.
    //

    InitializeObjectAttributes( &objectAttributes,
                                SymbolicLinkName,
                                OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                SePublicDefaultUnrestrictedSd );

    //
    // Note that the following assignment can fail (because it is not system
    // initialization time and therefore the \ARCname directory does not
    // exist - if this is really a call to IoAssignArcName), but that is fine.
    //

    status = ZwCreateSymbolicLinkObject( &linkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &objectAttributes,
                                         DeviceName );
    if (NT_SUCCESS( status )) {
        ZwClose( linkHandle );
    }

    return status;
}

PKEVENT
IoCreateSynchronizationEvent(
    IN PUNICODE_STRING EventName,
    OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    This routine creates a named synchronization event for use in serialization
    of access to hardware between two otherwise non-related drivers.  The event
    is created if it does not already exist, otherwise it is simply opened.

Arguments:

    EventName - Supplies the full name of the event.

    EventHandle - Supplies a location to return a handle to the event.

Return Value:

    The function value is a pointer to the created/opened event, or NULL if
    the event could not be created/opened.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS status;
    HANDLE eventHandle;
    PKEVENT eventObject;

    PAGED_CODE();

    //
    // Begin by initializing the object attributes.
    //

    InitializeObjectAttributes( &objectAttributes,
                                EventName,
                                OBJ_OPENIF,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    //
    // Now create or open the event.
    //

    status = ZwCreateEvent( &eventHandle,
                            EVENT_ALL_ACCESS,
                            &objectAttributes,
                            SynchronizationEvent,
                            TRUE );
    if (!NT_SUCCESS( status )) {
        return (PKEVENT) NULL;
    }

    //
    // Reference the object by its handle to get a pointer that can be returned
    // to the caller.
    //

    (VOID) ObReferenceObjectByHandle( eventHandle,
                                      0,
                                      ExEventObjectType,
                                      KernelMode,
                                      (PVOID *) &eventObject,
                                      NULL );
    ObDereferenceObject( eventObject );

    //
    // Return the handle and the pointer to the event.
    //

    *EventHandle = eventHandle;

    return eventObject;
}

NTSTATUS
IoCreateUnprotectedSymbolicLink(
    IN PUNICODE_STRING SymbolicLinkName,
    IN PUNICODE_STRING DeviceName
    )

/*++

Routine Description:

    This routine is invoked to assign an unprotected symbolic link name to
    a device.  That is, a symbolic link that can be dynamically reassigned
    without any special authorization.


    NOTE:  This routine will NOT over-ride inheritable protection that
           the symbolic link might pick up.  It simply prevents the caller's
            default token protection from being assigned.


Arguments:

    SymbolicLinkName - Supplies the symbolic link name as a Unicode string.

    DeviceName - Supplies the name to which the symbolic link name refers.

Return Value:

    The function value is the final status of the operation.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE linkHandle;
    NTSTATUS status;
    SECURITY_DESCRIPTOR securityDescriptor;

    PAGED_CODE();

    //
    // Create a security descriptor that has all access.
    //

    status = RtlCreateSecurityDescriptor( &securityDescriptor,
                                          SECURITY_DESCRIPTOR_REVISION1 );
    if (!NT_SUCCESS( status )) {
        return status;
    }

    status = RtlSetDaclSecurityDescriptor ( &securityDescriptor,
                                            TRUE,
                                            NULL,
                                            TRUE );     //does not over-ride inheritable protection
    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // Initialize the object attributes for the symbolic link.
    //

        InitializeObjectAttributes( &objectAttributes,
                                SymbolicLinkName,
                                OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                &securityDescriptor );

    //
    // Note that the following assignment can fail (because it is not system
    // initialization time and therefore the \ARCname directory does not
    // exist - if this is really a call to IoAssignArcName), but that is fine.
    //

    status = ZwCreateSymbolicLinkObject( &linkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &objectAttributes,
                                         DeviceName );
    if (NT_SUCCESS( status )) {
        ZwClose( linkHandle );
    }

    return status;
}

VOID
IoDeleteController(
    IN PCONTROLLER_OBJECT ControllerObject
    )

/*++

Routine Description:

    This routine deletes the specified controller object from the system
    so that it may no longer be referenced from a driver.  It is invoked
    when either the driver is being unloaded from the system, or the driver's
    initialization routine failed to properly initialize the device or a
    fatal driver initialization error was encountered.

Arguments:

    ControllerObject - Pointer to the controller object that is to be
        deleted.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    // The controller was created as a temporary object, and all of the
    // handles for the object have already been closed.  At this point,
    // simply dereferencing the object will cause it to be deleted.
    //

    ObDereferenceObject( ControllerObject );
}

VOID
IopRemoveTimerFromTimerList(
    IN PIO_TIMER timer
    )
{
    KIRQL irql;

    ExAcquireFastLock( &IopTimerLock, &irql );
    RemoveEntryList( &timer->TimerList );
    if (timer->TimerFlag) {
        IopTimerCount--;
    }
    ExReleaseFastLock( &IopTimerLock, irql );
}

VOID
IoDeleteDevice(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine deletes the specified device object from the system so that
    it may no longer be referenced.  It is invoked when either the device
    driver is being unloaded from the system, or the driver's initialization
    routine failed to properly initialize the device or a fatal driver
    initialization error was encountered, or when the device is being removed
    from the system.

Arguments:

    DeviceObject - Pointer to the device object that is to be deleted.

Return Value:

    None.

--*/

{
    KIRQL irql;

    IOV_DELETE_DEVICE(DeviceObject);

    //
    // Check to see whether or not the device has registered a shutdown
    // handler if necessary, and if so, unregister it.
    //

    if (DeviceObject->Flags & DO_SHUTDOWN_REGISTERED) {
        IoUnregisterShutdownNotification( DeviceObject );
    }

    //
    // Release the pool that was allocated to contain the timer dispatch
    // routine and its associated context if there was one.
    //

    if (DeviceObject->Timer) {
        PIO_TIMER timer;

        timer = DeviceObject->Timer;
        IopRemoveTimerFromTimerList(timer);
        ExFreePool( timer );
    }

    //
    // If this device has a name, then mark the
    // object as temporary so that when it is dereferenced it will be
    // deleted.
    //

    if (DeviceObject->Flags & DO_DEVICE_HAS_NAME) {
        ObMakeTemporaryObject( DeviceObject );
    }

    //
    // PoRunDownDeviceObject will clean up any power management
    // structures attached to the device object.
    //

    PoRunDownDeviceObject(DeviceObject);

    //
    // Mark the device object as deleted.
    //

    ExAcquireSpinLock( &IopDatabaseLock, &irql );

    DeviceObject->DeviceObjectExtension->ExtensionFlags |= DOE_DELETE_PENDING;

    if (!DeviceObject->ReferenceCount) {
        IopCompleteUnloadOrDelete( DeviceObject, irql );
    } else {
        ExReleaseSpinLock( &IopDatabaseLock, irql );
    }
}


NTSTATUS
IopDeleteSessionSymLinks(
    IN PUNICODE_STRING LinkName
    )
/*++

Routine Description:

    This routine is called from IoDeleteSymbolic Link. It enumerates all the
    Terminal Server session specific object directories and deletes the specified
    symbolic link from the DosDevices object directory of each sesssion. This
    routine is only called when Terminal Services is enabled.

Arguments:

    SymbolicLinkName - Provides the Unicode name string to be deassigned.

Return Values:

    None.

--*/

{

    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING UnicodeString;
    UNICODE_STRING SymbolicLinkName;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE DirectoryHandle;
    HANDLE linkHandle;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    BOOLEAN RestartScan;
    ULONG Context = 0;
    ULONG ReturnedLength;
    PWCHAR NameBuf;
    PUCHAR DirInfoBuffer;
    ULONG Size;
    WCHAR Prefix[13]; // sizeof L"\\DosDevices\\"



    //
    // Only delete links that start with \DosDevices\
    //

    if (LinkName->Length < (sizeof(L"\\DosDevices\\"))) {
        return STATUS_SUCCESS;
    }
    RtlInitUnicodeString( &UnicodeString, L"\\DosDevices\\" );

    wcsncpy(Prefix,LinkName->Buffer,(sizeof(L"\\DosDevices\\")/sizeof(WCHAR)) - 1);
    RtlInitUnicodeString( &SymbolicLinkName, Prefix);

    if (RtlCompareUnicodeString(&UnicodeString, &SymbolicLinkName,TRUE)) {

        return STATUS_SUCCESS;

    }


    //
    // Open the root Sessions Directory.
    //
    RtlInitUnicodeString( &UnicodeString, L"\\Sessions" );

    InitializeObjectAttributes( &Attributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = ZwOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_QUERY,
                                    &Attributes
                                  );
    if (NT_SUCCESS( Status )) {


        //
        // Since SessionId is a ULONG , the prefix (\\Sessions\\<SessionId>\\DosDevices)
        // cannot be more that 128 characters in length
        //
        Size = (LinkName->Length + 128) * sizeof(WCHAR);
        NameBuf = (PWCHAR)ExAllocatePoolWithTag(PagedPool, Size, ' oI');

        if (NameBuf == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        SymbolicLinkName.Buffer = (PWSTR)NameBuf;
        SymbolicLinkName.Length = (USHORT)Size;
        SymbolicLinkName.MaximumLength = (USHORT)Size;


        //
        // 4k should be more than enough to query a directory object entry
        //
        Size = 4096;
        DirInfoBuffer = (PUCHAR)ExAllocatePoolWithTag(PagedPool, Size, ' oI');

        if (DirInfoBuffer == NULL) {
            ExFreePool(NameBuf);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RestartScan = TRUE;
        DirInfo = (POBJECT_DIRECTORY_INFORMATION)DirInfoBuffer;


        while (TRUE) {

            Status = ZwQueryDirectoryObject( DirectoryHandle,
                                             (PVOID)DirInfo,
                                             Size,
                                             TRUE,
                                             RestartScan,
                                             &Context,
                                             &ReturnedLength
                                           );

            RestartScan = FALSE;

            //
            //  Check the status of the operation.
            //

            if (!NT_SUCCESS( Status )) {
                if (Status == STATUS_NO_MORE_ENTRIES) {
                    Status = STATUS_SUCCESS;
                    }

                break;
                }


            //
            // This generates session specific symbolic link path
            // \Sessions\<id>\DosDevices\<LinkName>
            //
            RtlInitUnicodeString( &UnicodeString, L"\\Sessions\\" );
            RtlCopyUnicodeString( &SymbolicLinkName, &UnicodeString );
            RtlAppendUnicodeStringToString( &SymbolicLinkName, &(DirInfo->Name) );
            RtlAppendUnicodeStringToString( &SymbolicLinkName, LinkName );
            //
            // Begin by initializing the object attributes for the symbolic link.
            //

            InitializeObjectAttributes( &Attributes,
                                        &SymbolicLinkName,
                                        OBJ_CASE_INSENSITIVE,
                                        NULL,
                                        NULL );

            //
            // Open the symbolic link itself so that it can be marked temporary and
            // closed.
            //

            Status = ZwOpenSymbolicLinkObject( &linkHandle,
                                               DELETE,
                                               &Attributes );
            if (NT_SUCCESS( Status )) {

                //
                // The symbolic link was successfully opened.  Attempt to make it a
                // temporary object, and then close the handle.  This will cause the
                // object to go away.
                //

                Status = ZwMakeTemporaryObject( linkHandle );
                if (NT_SUCCESS( Status )) {
                    ZwClose( linkHandle );
                }
            }



         }

         ZwClose(DirectoryHandle);
         ExFreePool(NameBuf);
         ExFreePool(DirInfoBuffer);
    }

     return Status;
}


NTSTATUS
IoDeleteSymbolicLink(
    IN PUNICODE_STRING SymbolicLinkName
    )

/*++

Routine Description:

    This routine is invoked to remove a symbolic link from the system.  This
    generally occurs whenever a driver that has assigned a symbolic link needs
    to exit.  It can also be used when a driver no longer needs to redirect
    a name.

Arguments:

    SymbolicLinkName - Provides the Unicode name string to be deassigned.

Return Values:

    None.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE linkHandle;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Begin by initializing the object attributes for the symbolic link.
    //

    InitializeObjectAttributes( &objectAttributes,
                                SymbolicLinkName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    //
    // Open the symbolic link itself so that it can be marked temporary and
    // closed.
    //

    status = ZwOpenSymbolicLinkObject( &linkHandle,
                                       DELETE,
                                       &objectAttributes );
    if (NT_SUCCESS( status )) {

        //
        // The symbolic link was successfully opened.  Attempt to make it a
        // temporary object, and then close the handle.  This will cause the
        // object to go away.
        //

        status = ZwMakeTemporaryObject( linkHandle );
        if (NT_SUCCESS( status )) {
            ZwClose( linkHandle );
        }

        if (ExVerifySuite(TerminalServer) == TRUE) {
            IopDeleteSessionSymLinks( SymbolicLinkName );
        }
    }


    return status;
}

VOID
IoDetachDevice(
    IN OUT PDEVICE_OBJECT TargetDevice
    )

/*++

Routine Description:

    This routine detaches the device object which is currently attached to the
    target device.

Arguments:

    TargetDevice - Pointer to device object to be detached from.

Return Value:

    None.


--*/

{
    KIRQL irql;
    PDEVICE_OBJECT detachingDevice;
    PDEVOBJ_EXTENSION detachingExtension;

    //
    // Detach the device object attached to the target device.  This also
    // includes decrementing the reference count for the device.  Note that
    // if the driver has been marked for an unload operation, and the
    // reference count goes to zero, then the driver may need to be unloaded
    // at this point.
    //

    ExAcquireSpinLock( &IopDatabaseLock, &irql );

    //
    // Tell the Special IRP code the stack has changed. Code that will reexamine
    // the stack takes the database lock, so we can place the call here. This
    // also allows us to assert correct behavoir *before* the stack is torn
    // down.
    //
    IOV_DETACH_DEVICE(TargetDevice);

    detachingDevice = TargetDevice->AttachedDevice;
    detachingExtension = detachingDevice->DeviceObjectExtension;
    ASSERT( detachingExtension->AttachedTo == TargetDevice );

    //
    // Unlink the device from the doubly-linked attachment chain.
    //

    detachingExtension->AttachedTo = NULL;
    TargetDevice->AttachedDevice = NULL;

    if (TargetDevice->DeviceObjectExtension->ExtensionFlags &
        (DOE_UNLOAD_PENDING | DOE_DELETE_PENDING | DOE_REMOVE_PENDING) &&
        !TargetDevice->ReferenceCount) {
        IopCompleteUnloadOrDelete( TargetDevice, irql );
    } else {
        ExReleaseSpinLock( &IopDatabaseLock, irql );
    }
}

VOID
IoDisconnectInterrupt(
    IN PKINTERRUPT InterruptObject
    )

/*++

Routine Description:

    This routine disconnects all of the interrupt objects that were
    initialized and connected by the IoConnectInterrupt routine.  Note
    that no interrupt objects directly connected using the kernel
    services may be input to this routine.

Arguments:

    InterruptObject - Supplies a pointer to the interrupt object allocated
        by the IoConnectInterrupt routine.

Return Value:

    None.

--*/

{
    PIO_INTERRUPT_STRUCTURE interruptStructure;
    ULONG i;

    PAGED_CODE();

    //
    // Obtain a pointer to the builtin interrupt object in the I/O interrupt
    // structure.
    //

    interruptStructure = CONTAINING_RECORD( InterruptObject,
                                            IO_INTERRUPT_STRUCTURE,
                                            InterruptObject );

    //
    // The builtin interrupt object is always used, so simply disconnect
    // it.
    //

    KeDisconnectInterrupt( &interruptStructure->InterruptObject );

    //
    // Now loop through each of the interrupt objects pointed to by the
    // structure and disconnect each.
    //

    for (i = 0; i < MAXIMUM_PROCESSORS; i++) {
        if (interruptStructure->InterruptArray[i] != NULL) {
            KeDisconnectInterrupt( interruptStructure->InterruptArray[i] );
        }
    }

    //
    // Finally, deallocate the memory associated with the entire structure.
    //

    ExFreePool( interruptStructure );
}

VOID
IoEnqueueIrp(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine enqueues the specified I/O Request Packet (IRP) to the thread's
    IRP pending queue.  The thread that the IRP is queued to is specified by
    the IRP's Thread field.

Arguments:

    Irp - Supplies a pointer to the IRP to be enqueued.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    // Simply enqueue the IRP to the thread's IRP queue.
    //

    IopQueueThreadIrp( Irp );
    return;
}

BOOLEAN
IoFastQueryNetworkAttributes(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG OpenOptions,
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer
    )

/*++

Routine Description:

    This routine attempts to perform a fast I/O call to obtain the network
    attributes for a file.  This involves a specialized interface between
    this function and the I/O system's device parse method.  This allows the
    parse method to have the file system pseudo-open the file, obtain the
    appropriate attributes for the file, and return them to the caller w/as
    little overhead as possbile from either the Object Manager or the I/O
    system itself.

Arguments:

    ObjectAttributes - Supplies the attributes to be used for opening the
        file (e.g., the file's name, etc).

    DesiredAccess - Supplies the type(s) of access that the caller would like
        to the file.

    OpenOptions - Supplies standard NtOpenFile open options.

    IoStatus - Supplies a pointer to a variable to receive the final status
        of the operation.

    Buffer - Supplies an output buffer to receive the network attributes for
        the specified file.

Return Value:

    The final function value indicates whether or not the fast path could
    be taken successfully.

--*/

{
    HANDLE handle;
    NTSTATUS status;
    OPEN_PACKET openPacket;
    DUMMY_FILE_OBJECT localFileObject;

    //
    // Build a parse open packet that tells the parse method to open the
    // file and query its network attributes using the fast path, if it
    // exists for this file.
    //

    RtlZeroMemory( &openPacket, sizeof( OPEN_PACKET ) );

    openPacket.Type = IO_TYPE_OPEN_PACKET;
    openPacket.Size = sizeof( OPEN_PACKET );
    openPacket.ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    openPacket.Disposition = FILE_OPEN;
    openPacket.CreateOptions = OpenOptions | FILE_OPEN_REPARSE_POINT;
    openPacket.Options = IO_FORCE_ACCESS_CHECK;
    openPacket.NetworkInformation = Buffer;
    openPacket.QueryOnly = TRUE;
    openPacket.FullAttributes = TRUE;
    openPacket.LocalFileObject = &localFileObject;

    //
    // Open the object by its name.  Because of the special QueryOnly flag set
    // in the open packet, the parse routine will open the file using the fast
    // path open and perform the query, effectively closing it as well.
    //

    status = ObOpenObjectByName( ObjectAttributes,
                                 (POBJECT_TYPE) NULL,
                                 KernelMode,
                                 NULL,
                                 DesiredAccess,
                                 &openPacket,
                                 &handle );

    //
    // The opeation is successful if the parse check field of the open packet
    // indicates that the parse routine was actually invoked, and the final
    // status field of the packet is set to success.  The QueryOnly field is
    // set to whether or not the fast path was invoked.
    //

    if (openPacket.ParseCheck != OPEN_PACKET_PATTERN) {

        //
        // The parse routine was not invoked properly so the operation did
        // not work at all.
        //

        IoStatus->Status = status;
    } else {

        //
        // The fast path routine was successfully invoked so return the
        // final status of the operation.
        //

        IoStatus->Status = openPacket.FinalStatus;
        IoStatus->Information = openPacket.Information;
    }
    return TRUE;
}

VOID
IoFreeController(
    IN PCONTROLLER_OBJECT ControllerObject
    )

/*++

Routine Description:

    This routine is invoked to deallocate the specified controller object.
    No checks are made to ensure that the controller is really allocated
    to a device object.  However, if it is not, then kernel will bugcheck.

    If another device is waiting in the queue to allocate the controller
    object it will be pulled from the queue and its execution routine will
    be invoked.

Arguments:

    ControllerObject - Pointer to the controller object to be deallocated.

Return Value:

    None.

--*/

{
    PKDEVICE_QUEUE_ENTRY packet;
    PDEVICE_OBJECT deviceObject;
    IO_ALLOCATION_ACTION action;

    //
    // Simply remove the next entry from the controller's device wait queue.
    // If one was successfully removed, invoke its execution routine.
    //

    packet = KeRemoveDeviceQueue( &ControllerObject->DeviceWaitQueue );
    if (packet != NULL) {
        deviceObject = CONTAINING_RECORD( packet,
                                          DEVICE_OBJECT,
                                          Queue.Wcb.WaitQueueEntry );
        action = deviceObject->Queue.Wcb.DeviceRoutine( deviceObject,
                                                        deviceObject->CurrentIrp,
                                                        0,
                                                        deviceObject->Queue.Wcb.DeviceContext );

        //
        // If the execution routine wants the controller to be deallocate
        // now, deallocate it.
        //

        if (action == DeallocateObject) {
            IoFreeController( ControllerObject );
        }
    }
}

VOID
IoFreeIrp(
    IN PIRP Irp
    )
{
    pIoFreeIrp(Irp);
}


VOID
IopFreeIrp(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine deallocates the specified I/O Request Packet.

Arguments:

    Irp - I/O Request Packet to deallocate.

Return Value:

    None.

--*/

{
    PNPAGED_LOOKASIDE_LIST lookasideList;
    PP_NPAGED_LOOKASIDE_NUMBER number;
    PKPRCB prcb;

    //
    // Ensure that the data structure being freed is really an IRP.
    //

    ASSERT( Irp->Type == IO_TYPE_IRP );

    if (Irp->Type != IO_TYPE_IRP) {
        KeBugCheckEx( MULTIPLE_IRP_COMPLETE_REQUESTS, (ULONG_PTR) Irp, __LINE__, 0, 0 );
    }


    ASSERT(IsListEmpty(&(Irp)->ThreadListEntry));
    Irp->Type = 0;

    //
    // Ensure that all of the owners of the IRP have at least been notified
    // that the request is going away.
    //

    ASSERT( Irp->CurrentLocation >= Irp->StackCount );

    //
    // Deallocate the IRP.
    //

    if (Irp->AllocationFlags & IRP_LOOKASIDE_ALLOCATION) {
        Irp->AllocationFlags ^= IRP_LOOKASIDE_ALLOCATION;
        InterlockedDecrement( &IopLookasideIrpFloat );
    }
    if (!(Irp->AllocationFlags & IRP_ALLOCATED_FIXED_SIZE) ||
        (Irp->AllocationFlags & IRP_ALLOCATED_MUST_SUCCEED) ||
        (IopLookasideIrpFloat >= IopLookasideIrpLimit)) {
        ExFreePool( Irp );

    } else {
        number = LookasideSmallIrpList;
        if (Irp->StackCount != 1) {
            number = LookasideLargeIrpList;
        }

        prcb = KeGetCurrentPrcb();
        lookasideList = prcb->PPLookasideList[number].P;
        lookasideList->L.TotalFrees += 1;
        if ( ExQueryDepthSList( &lookasideList->L.ListHead ) >= lookasideList->L.Depth ) {
            lookasideList->L.FreeMisses += 1;
            lookasideList = prcb->PPLookasideList[number].L;
            lookasideList->L.TotalFrees += 1;
            if (ExQueryDepthSList( &lookasideList->L.ListHead ) >= lookasideList->L.Depth) {
                lookasideList->L.FreeMisses += 1;
                ExFreePool( Irp );

            } else {
                if (Irp->AllocationFlags & IRP_QUOTA_CHARGED) {
                    Irp->AllocationFlags ^= IRP_QUOTA_CHARGED;
                    ExReturnPoolQuota( Irp );
                }

                ExInterlockedPushEntrySList( &lookasideList->L.ListHead,
                                             (PSINGLE_LIST_ENTRY) Irp,
                                             &lookasideList->Lock );
            }

        } else {
            if (Irp->AllocationFlags & IRP_QUOTA_CHARGED) {
                Irp->AllocationFlags ^= IRP_QUOTA_CHARGED;
                ExReturnPoolQuota( Irp );
            }

            ExInterlockedPushEntrySList( &lookasideList->L.ListHead,
                                         (PSINGLE_LIST_ENTRY) Irp,
                                         &lookasideList->Lock );
        }
    }

    return;
}

VOID
IoFreeMdl(
    IN PMDL Mdl
    )

/*++

Routine Description:

    This routine frees a Memory Descriptor List (MDL).  It only frees the
    specified MDL; any chained MDLs must be freed explicitly through another
    call to this routine.

Arguments:

    Mdl - Pointer to the Memory Descriptor List to be freed.

Return Value:

    None.

--*/

{
    PNPAGED_LOOKASIDE_LIST lookasideList;

    //
    // Tell memory management that this MDL will be re-used.  This will
    // cause MM to unmap any pages that have been mapped for this MDL if
    // it is a partial MDL.
    //

    MmPrepareMdlForReuse(Mdl);
    if (((Mdl->MdlFlags & MDL_ALLOCATED_FIXED_SIZE) == 0) ||
        ((Mdl->MdlFlags & MDL_ALLOCATED_MUST_SUCCEED) != 0)) {
        ExFreePool(Mdl);

    } else {
        ExFreeToPPNPagedLookasideList(LookasideMdlList, Mdl);
    }
}

PDEVICE_OBJECT
IoGetAttachedDevice(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine returns the highest level device object associated with
    the specified device.

    N.B. Caller must own the IopDatabaseLock. External callers of this
    function must ensure nobody is attaching or detaching from the stack.
    If they cannot, they *must* use IoGetAttachedDeviceReference.

Arguments:

    DeviceObject - Supplies a pointer to the device for which the attached
        device is to be returned.

Return Value:

    The function value is the highest level device attached to the specified
    device.  If no devices are attached, then the pointer to the device
    object itself is returned.

--*/

{
    //
    // Loop through all of the device object's attached to the specified
    // device.  When the last device object is found that is not attached
    // to, return it.
    //

    while (DeviceObject->AttachedDevice) {
        DeviceObject = DeviceObject->AttachedDevice;
    }

    return DeviceObject;
}

PDEVICE_OBJECT
IoGetAttachedDeviceReference(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine synchronizes with the Io database and returns a refernce
    to the highest level device object associated withthe specified device.

Arguments:

    DeviceObject - Supplies a pointer to the device for which the attached
        device is to be returned.

Return Value:

    The function value is a reference to the highest level device attached
    to the specified device.  If no devices are attached, then the pointer
    to the device object itself is returned.

--*/
{
    KIRQL               irql;

    ExAcquireFastLock( &IopDatabaseLock, &irql );
    DeviceObject = IoGetAttachedDevice (DeviceObject);
    ObReferenceObject (DeviceObject);
    ExReleaseFastLock( &IopDatabaseLock, irql );
    return DeviceObject;
}

PDEVICE_OBJECT
IoGetBaseFileSystemDeviceObject(
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine returns the base (lowest-level) file system volume device
    object associated with a file.  I.e., it locates the file system w/o
    walking the attached device object list.

Arguments:

    FileObject - Supplies a pointer to the file object for which the base
        file system device object is to be returned.

Return Value:

    The function value is the lowest level volume device object associated
    w/the file.

--*/

{
    PDEVICE_OBJECT deviceObject;

    //
    // If the file object has a mounted Vpb, use its DeviceObject.
    //

    if (FileObject->Vpb != NULL && FileObject->Vpb->DeviceObject != NULL) {
        deviceObject = FileObject->Vpb->DeviceObject;

    //
    // Otherwise, if the real device has a VPB that indicates that it is
    // mounted, then use the file system device object associated with the
    // VPB.
    //

    } else if (!(FileObject->Flags & FO_DIRECT_DEVICE_OPEN) &&
               FileObject->DeviceObject->Vpb != NULL &&
               FileObject->DeviceObject->Vpb->DeviceObject != NULL) {

            deviceObject = FileObject->DeviceObject->Vpb->DeviceObject;

    //
    // Otherwise, just return the real device object.
    //

    } else {

        deviceObject = FileObject->DeviceObject;
    }

    ASSERT( deviceObject != NULL );

    //
    // Simply return the resultant file object.
    //

    return deviceObject;
}

PCONFIGURATION_INFORMATION
IoGetConfigurationInformation( VOID )

/*++

Routine Description:

    This routine returns a pointer to the system's device configuration
    information structure so that drivers and the system can determine how
    many different types of devices exist in the system.

Arguments:

    None.

Return Value:

    The function value is a pointer to the configuration information
    structure.

--*/

{
    PAGED_CODE();

    //
    // Simply return a pointer to the built-in configuration information
    // structure.
    //

    return (&ConfigurationInformation);
}

PEPROCESS
IoGetCurrentProcess( VOID )

/*++

Routine Description:

    This routine returns a pointer to the current process.  It is actually
    a jacket routine for the PS version of the same function since device
    drivers using the ntddk header file cannot see into a thread object.

Arguments:

    None.

Return Value:

    The function value is a pointer to the current process.

Note:

    Note that this function cannot be paged because it is invoked at raised
    IRQL in debug builds, which is the only time that the PAGED_CODE macro
    actually does anything.  Therefore, it is impossible to find code that
    invokes this function at raised IRQL in a normal system w/o simply running
    into the "proper conditions".  This is too risky to actually page this
    routine, so it is left nonpaged.

--*/

{
    //
    // Simply return a pointer to the current process.
    //

    return PsGetCurrentProcess();
}

NTSTATUS
IoGetDeviceObjectPointer(
    IN PUNICODE_STRING ObjectName,
    IN ACCESS_MASK DesiredAccess,
    OUT PFILE_OBJECT *FileObject,
    OUT PDEVICE_OBJECT *DeviceObject
    )

/*++

Routine Description:

    This routine returns a pointer to the device object specified by the
    object name.  It also returns a pointer to the referenced file object
    that has been opened to the device that ensures that the device cannot
    go away.

    To close access to the device, the caller should dereference the file
    object pointer.

Arguments:

    ObjectName - Name of the device object for which a pointer is to be
        returned.

    DesiredAccess - Access desired to the target device object.

    FileObject - Supplies the address of a variable to receive a pointer
        to the file object for the device.

    DeviceObject - Supplies the address of a variable to receive a pointer
        to the device object for the specified device.

Return Value:

    The function value is a referenced pointer to the specified device
    object, if the device exists.  Otherwise, NULL is returned.

--*/

{
    PFILE_OBJECT fileObject;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Initialize the object attributes to open the device.
    //

    InitializeObjectAttributes( &objectAttributes,
                                ObjectName,
                                0,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ZwOpenFile( &fileHandle,
                         DesiredAccess,
                         &objectAttributes,
                         &ioStatus,
                         0,
                         FILE_NON_DIRECTORY_FILE );

    if (NT_SUCCESS( status )) {

        //
        // The open operation was successful.  Dereference the file handle
        // and obtain a pointer to the device object for the handle.
        //

        status = ObReferenceObjectByHandle( fileHandle,
                                            0,
                                            IoFileObjectType,
                                            KernelMode,
                                            (PVOID *) &fileObject,
                                            NULL );
        if (NT_SUCCESS( status )) {

            *FileObject = fileObject;

            //
            // Get a pointer to the device object for this file.
            //
            *DeviceObject = IoGetRelatedDeviceObject( fileObject );
        }

        (VOID) ZwClose( fileHandle );
    }

    return status;
}

PDEVICE_OBJECT
IoGetDeviceToVerify(
    IN PETHREAD Thread
    )

/*++

Routine Description:

    This routine returns a pointer to the device object that is to be verified.
    The pointer is set in the thread object by a device driver when the disk
    or CD-ROM media appears to have changed since the last access to the device.

Arguments:

    Thread - Pointer to the thread whose field is to be queried.

Return Value:

    The function value is a pointer to the device to be verified, or NULL.

Note:

    This function cannot be made a macro, since fields in the thread object
    move from release to release, so this must remain a full function.


--*/

{
    //
    // Simply return the device to be verified.
    //

    return Thread->DeviceToVerify;
}

NTKERNELAPI
PVOID
IoGetDriverObjectExtension(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID ClientIdentificationAddress
    )
/*++

Routine Description:

    This routine returns a pointer to the client driver object extension.
    This extension was allocated using IoAllocateDriverObjectExtension. If
    an extension with the create Id does not exist for the specified driver
    object then NULL is returned.

Arguments:

    DriverObject - Pointer to driver object owning the extension.

    ClientIdentificationAddress - Supplies the unique identifier which was
        used to create the extension.

Return Value:

    The function value is a pointer to the client driver object extension,
    or NULL.

--*/

{
    KIRQL irql;
    PIO_CLIENT_EXTENSION extension;

    ExAcquireFastLock( &IopDatabaseLock, &irql );
    extension = DriverObject->DriverExtension->ClientDriverExtension;
    while (extension != NULL) {

        if (extension->ClientIdentificationAddress == ClientIdentificationAddress) {
            break;
        }

        extension = extension->NextExtension;
    }

    ExReleaseFastLock( &IopDatabaseLock, irql );

    if (extension == NULL) {
        return NULL;
    }

    return extension + 1;
}

PGENERIC_MAPPING
IoGetFileObjectGenericMapping(
    VOID
    )

/*++

Routine Description:

    This routine returns a pointer to the generic mapping for a file object.

Arguments:

    None.

Return Value:

    A pointer to the generic mapping for a file object.

--*/

{
    //
    // Simply return a pointer to the generic mapping for a file object.
    //

    return &IopFileMapping;
}

PVOID
IoGetInitialStack(
    VOID
    )

/*++

Routine Description:

    This routine returns the base initial address of the current thread's
    stack.

Arguments:

    None.

Return Value:

    The base initial address of the current thread's stack.

Note:

    This function cannot be made a macro, since fields in the thread object
    move from release to release, so this must remain a full function.

--*/

{
    //
    // Simply return the initial stack for this thread.
    //

    return PsGetCurrentThread()->Tcb.InitialStack;
}

PDEVICE_OBJECT
IoGetRelatedDeviceObject(
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine returns a pointer to the actual device object than an I/O
    Request Packet (IRP) should be given to based on the specified file
    object.

    N.B. - Callers of this function must ensure no device object is
    attaching or detaching from this stack for the duration of this call.
    This is because the database lock is *not* held!

Arguments:

    FileObject - Pointer to the file object representing the open file.

Return Value:

    The return value is a pointer to the device object for the driver to
    whom the request is to be given.

--*/

{
    PDEVICE_OBJECT deviceObject;

    //
    // If the file object was taken out against the mounted file system, it
    // will have a Vpb. Traverse it to get to the DeviceObject. Note that in
    // this case we should never follow FileObject->DeviceObject, as that
    // mapping may be invalid after a forced dismount.
    //

    if (FileObject->Vpb != NULL && FileObject->Vpb->DeviceObject != NULL) {

        ASSERT(!(FileObject->Flags & FO_DIRECT_DEVICE_OPEN));
        deviceObject = FileObject->Vpb->DeviceObject;


        //
        // If a driver opened a disk device using direct device open and 
        // later on it uses IoGetRelatedTargetDeviceObject to find the
        // device object it wants to send an IRP then it should not get the
        // filesystem device object. This is so that if the device object is in the
        // process of being mounted then vpb is not stable.
        //

    } else if (!(FileObject->Flags & FO_DIRECT_DEVICE_OPEN) &&
               FileObject->DeviceObject->Vpb != NULL &&
               FileObject->DeviceObject->Vpb->DeviceObject != NULL) {

            deviceObject = FileObject->DeviceObject->Vpb->DeviceObject;

    //
    // This is a direct open against the device stack (and there is no mounted
    // file system to strain the IRPs through).
    //

    } else {

        deviceObject = FileObject->DeviceObject;
    }

    ASSERT( deviceObject != NULL );

    //
    // Check to see whether or not the device has any associated devices.
    // If so, return the highest level device; otherwise, return a pointer
    // to the device object itself.
    //

    if (deviceObject->AttachedDevice != NULL) {
        deviceObject = IoGetAttachedDevice( deviceObject );
    }

    return deviceObject;
}

ULONG
IoGetRequestorProcessId(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine returns a 32-bit value that is unique to the process that
    originally requested the specified I/O operation.

Arguments:

    Irp - Pointer to the I/O Request Packet.

Return Value:

    The function value is the 32-bit process ID.


--*/

{
    PEPROCESS process;

    process = IoGetRequestorProcess( Irp );
    if (process != NULL) {

        //
        // UniqueProcessId is a kernel-mode handle, safe to truncate to ULONG.
        //

        return HandleToUlong( process->UniqueProcessId );
    } else {

        //
        // Return a PID of zero if there is no process associated with the IRP.
        //

        return 0;
    }
}

PEPROCESS
IoGetRequestorProcess(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine returns a pointer to the process that originally
    requested the specified I/O operation.

Arguments:

    Irp - Pointer to the I/O Request Packet.

Return Value:

    The function value is a pointer to the original requesting process.


--*/

{
    //
    // Return the address of the process that requested the I/O operation.
    //

    if (Irp->Tail.Overlay.Thread) {
        return THREAD_TO_PROCESS( Irp->Tail.Overlay.Thread );
    } else {
        return NULL;
    }
}

PIRP
IoGetTopLevelIrp(
    VOID
    )

/*++

Routine Description:

    This routine returns the contents of the TopLevelIrp field of the current
    thread.

Arguments:

    None.

Return Value:

    The final function value is the contents of the TopLevelIrp field.

Note:

    This function cannot be made a macro, since fields in the thread object
    move from release to release, so this must remain a full function.

--*/

{
    //
    // Simply return the TopLevelIrp field of the thread.
    //

    return (PIRP) (PsGetCurrentThread()->TopLevelIrp);
}

VOID
IoInitializeIrp(
    IN OUT PIRP Irp,
    IN USHORT PacketSize,
    IN CCHAR StackSize
    )

/*++

Routine Description:

    Initializes an IRP.

Arguments:

    Irp - a pointer to the IRP to initialize.

    PacketSize - length, in bytes, of the IRP.

    StackSize - Number of stack locations in the IRP.

Return Value:

    None.

--*/

{
    IOV_INITIALIZE_IRP(Irp, PacketSize, StackSize);

    //
    // Begin by zeroing the entire packet.
    //

    RtlZeroMemory( Irp, PacketSize );

    //
    // Initialize the remainder of the packet by setting the appropriate fields
    // and setting up the I/O stack locations in the packet.
    //

    Irp->Type = (CSHORT) IO_TYPE_IRP;
    Irp->Size = (USHORT) PacketSize;
    Irp->StackCount = (CCHAR) StackSize;
    Irp->CurrentLocation = (CCHAR) (StackSize + 1);
    Irp->ApcEnvironment = KeGetCurrentApcEnvironment();
    InitializeListHead (&(Irp)->ThreadListEntry);
    Irp->Tail.Overlay.CurrentStackLocation =
        ((PIO_STACK_LOCATION) ((UCHAR *) (Irp) +
            sizeof( IRP ) +
            ( (StackSize) * sizeof( IO_STACK_LOCATION ))));
}

VOID
IoReuseIrp(
    PIRP Irp,
    NTSTATUS Status)
/*++

Routine Description:

    This routine is used by drivers to initialize an already allocated IRP for reuse.
    It does what IoInitializeIrp does but it saves the allocation flags so that we know
    how to free the Irp and take care of quote requirements. Drivers should call IoReuseIrp
    instead of calling IoInitializeIrp to reinitialize an IRP.

Arguments:

    Irp - Pointer to Irp to be reused

    Status - Status to preinitialize the Iostatus field.

--*/
{

    USHORT  PacketSize;
    CCHAR   StackSize;
    UCHAR   AllocationFlags;

    // Did anyone forget to pull their cancel routine?
    ASSERT(Irp->CancelRoutine == NULL) ;

    // We probably don't want thread enqueue'd IRPs to be used
    // ping-pong style as they cannot be dequeue unless they
    // complete entirely. Not really an issue for worker threads,
    // but definitely for operations on application threads.
    ASSERT(IsListEmpty(&Irp->ThreadListEntry)) ;

    AllocationFlags = Irp->AllocationFlags;
    StackSize = Irp->StackCount;
    PacketSize =  IoSizeOfIrp(StackSize);
    IopInitializeIrp(Irp, PacketSize, StackSize);
    Irp->AllocationFlags = AllocationFlags;
    Irp->IoStatus.Status = Status;

}



NTSTATUS
IoInitializeTimer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_TIMER_ROUTINE TimerRoutine,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used by drivers to initialize a timer entry for a device
    object.

Arguments:

    DeviceObject - Pointer to device object to be used.

    TimerRoutine - Driver routine to be executed when timer expires.

    Context - Context parameter that is passed to the driver routine.

Return Value:

    The function value indicates whether or not the timer was initialized.

--*/

{
    PIO_TIMER timer;

    PAGED_CODE();

    //
    // Begin by getting the address of the timer to be used.  If no timer has
    // been allocated, allocate one and initialize it.
    //

    timer = DeviceObject->Timer;
    if (!timer) {
        timer = ExAllocatePoolWithTag( NonPagedPool, sizeof( IO_TIMER ), 'iToI' );
        if (!timer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Initialize the timer entry so that it is suitable for being placed
        // into the I/O system's timer queue.
        //

        RtlZeroMemory( timer, sizeof( IO_TIMER ) );
        timer->Type = IO_TYPE_TIMER;
        timer->DeviceObject = DeviceObject;
        DeviceObject->Timer = timer;
    }

    //
    // Set the address of the driver's timer routine and the context parameter
    // passed to it and insert it onto the timer queue.  Note that the timer
    // enable flag is not set, so this routine will not actually be invoked
    // yet.
    //

    timer->TimerRoutine = TimerRoutine;
    timer->Context = Context;

    ExInterlockedInsertTailList( &IopTimerQueueHead,
                                 &timer->TimerList,
                                 &IopTimerLock );
    return STATUS_SUCCESS;
}

BOOLEAN
IoIsOperationSynchronous(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine determines whether an I/O operation is to be considered
    synchronous or an asynchronous, from the implementors point-of-view.
    Synchronous I/O is defined by how the file was opened, or the API being
    used to perform the operation, or by the type of paging I/O being
    performed, if the operation is paging I/O.

    It is possible for asynchronous paging I/O to occur to a file that was
    opened for synchronous I/O.  This occurs when the Modified Page Writer
    is doing I/O to a file that is mapped, when too many modified pages exist
    in the system.

Arguments:

    Irp - Pointer to the I/O Request Packet (IRP) representing the operation
        to be performed.

Return Value:

    A value of TRUE is returned if the operation is synchronous, otherwise
    FALSE is returned.

--*/

{
    //
    // Determine whether this is a synchronous I/O operation.  Synchronous I/O
    // is defined as an operation that is:
    //
    //     A file opened for synchronous I/O
    //         OR
    //     A synchronous API operation
    //         OR
    //     A synchronous paging I/O operation
    //
    //  AND this is NOT an asynchronous paging I/O operation occurring to some
    //  file that was opened for either synchronous or asynchronous I/O.
    //

    if ((IoGetCurrentIrpStackLocation( Irp )->FileObject->Flags & FO_SYNCHRONOUS_IO ||
        Irp->Flags & IRP_SYNCHRONOUS_API ||
        Irp->Flags & IRP_SYNCHRONOUS_PAGING_IO) &&
        !(Irp->Flags & IRP_PAGING_IO &&
        !(Irp->Flags & IRP_SYNCHRONOUS_PAGING_IO))) {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOLEAN
IoIsSystemThread(
    IN PETHREAD Thread
    )

/*++

Routine Description:

    This routine returns a BOOLEAN indicating whether or not the specified
    thread is a system thread.

Arguments:

    Thread - Pointer to the thread to be checked.

Return Value:

    A value of TRUE is returned if the indicated thread is a system thread,
    else FALSE.

Note:

    This function cannot be made a macro, since fields in the thread object
    move from release to release, so this must remain a full function.

--*/

{
    return (BOOLEAN) IS_SYSTEM_THREAD(Thread);
}

BOOLEAN
IoIsValidNameGraftingBuffer(
    IN PIRP Irp,
    IN PREPARSE_DATA_BUFFER ReparseBuffer
    )

/*++

Routine Description:

    This routine returns a BOOLEAN indicating whether or not the specified
    buffer is a valid name grafting buffer. All internal validity checks are
    encapsulated in this routine.

    Among the checks performed is whether the name lengths stored within the
    buffer in the private data section are compatible with the total size of
    the buffer that has been passed in.

Arguments:

    Irp - Pointer to the I/O Request Packet (IRP) representing the operation
        to be performed.

    Buffer - Pointer to a reparse data buffer that is supposed to contain
        a self-consistent set of names to perform name grafting.

Return Value:

    A value of TRUE is returned if the buffer is correct for name grafting,
    else FALSE.

Note:

    This function needs to be kept synchronized with the definition of
    REPARSE_DATA_BUFFER.

--*/

{
    PIO_STACK_LOCATION thisStackPointer = NULL;
    UNICODE_STRING     drivePath;

    ASSERT( FIELD_OFFSET( REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer[0] ) ==
            FIELD_OFFSET( REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer[0] ) );
    ASSERT( ReparseBuffer->ReparseDataLength < MAXIMUM_REPARSE_DATA_BUFFER_SIZE );

    //
    // Determine whether we have the correct kind of reparse tag in the buffer.
    //

    if (ReparseBuffer->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT) {

        //
        // The reparse tag is not an NT name grafting tag.
        //

        return FALSE;
    }

    //
    // Determine whether we have enough data for all the length fields.
    //

    if (ReparseBuffer->ReparseDataLength <
        (FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer[0]) - REPARSE_DATA_BUFFER_HEADER_SIZE)) {

        //
        // The buffer is shorter than the minimum needed to express a pair of valid
        // names.
        //

        return FALSE;
    }

    //
    // Get the address of the current stack location.
    //

    thisStackPointer = IoGetCurrentIrpStackLocation( Irp );

    //
    // Determine whether the data lengths returned are consistent with the buffer in
    // which they are retrieved.
    //
    // This check is meaningful only when the buffer has been allocated. When this routine
    // is used when a name grafting is being set there is no allocated output buffer.
    //

    if ((thisStackPointer->Parameters.FileSystemControl.OutputBufferLength > 0) &&
        (thisStackPointer->Parameters.FileSystemControl.OutputBufferLength <
        (ULONG)(FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer[0]) +
                ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength +
                ReparseBuffer->MountPointReparseBuffer.PrintNameLength +
                2 * sizeof( UNICODE_NULL )))) {

        //
        // The length of the appropriate buffer header, plus the lengths of the substitute
        // and print names are longer than the length of the buffer passed in.
        // Thus, this data is not self-consistent.
        //
        // Note that it is only the I/O subsystem that needs to check for this internal
        // consistency in the buffer as it will do a blind data copy using these lengths
        // when transmogrifying the names. The file system returning the buffer only needs
        // to ascertain that the total syze of the data retrieved does not exceed the size
        // of the output buffer.
        //

        return FALSE;
    }

    //
    // Now we determine whether the names were placed according to the reparse point
    // specification.
    //

    //
    // Determine whether the SubstituteNameOffset is zero.
    //

    if (ReparseBuffer->MountPointReparseBuffer.SubstituteNameOffset != 0) {

        //
        // Incorrect offset for the substitute name.
        //

        return FALSE;
    }

    //
    // Determine whether PrintNameOffset is correct.
    //

    if (ReparseBuffer->MountPointReparseBuffer.PrintNameOffset !=
        (ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength + sizeof( UNICODE_NULL )) ) {

        //
        // Incorrect offset for the print name.
        //

        return FALSE;
    }

    //
    // Determine whether ReparseDataLength is correct for name grafting operations.
    // We require a buffer of type REPARSE_DATA_BUFFER.
    //

    if (ReparseBuffer->ReparseDataLength !=
        (FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer[0]) - REPARSE_DATA_BUFFER_HEADER_SIZE) +
        ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength +
        ReparseBuffer->MountPointReparseBuffer.PrintNameLength +
        2 * sizeof( UNICODE_NULL )) {

        //
        // Incorrect length of the reparse data.
        //

        return FALSE;
    }

    //
    // Determine that the substitute name is not a UNC name.
    // This assumes that ReparseBuffer->MountPointReparseBuffer.SubstituteNameOffset is zero (0).
    //

    {
        //
        // This conditional is a transcription of part of the code of RtlDetermineDosPathNameType_U
        // present in ntos\dll\curdir.c
        //
        // The only two legal names that can begin with \\ are:  \\.  and  \\?
        // All other names that begin with  \\  are disallowed.
        //

        if ((ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength > 6) &&
            (ReparseBuffer->MountPointReparseBuffer.PathBuffer[0] == L'\\') &&
            (ReparseBuffer->MountPointReparseBuffer.PathBuffer[1] == L'\\') &&
            !((ReparseBuffer->MountPointReparseBuffer.PathBuffer[2] == L'.') ||
              (ReparseBuffer->MountPointReparseBuffer.PathBuffer[2] == L'?'))) {

            //
            // The name is not one we want to deal with.
            //

            return FALSE;
        }

        //
        // When  RtlDosPathNameToNtPathName_U  is used, the UNC names are returned with a prefix
        // of the form  \??\UNC\
        //

        if ((ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength > 16) &&
            (ReparseBuffer->MountPointReparseBuffer.PathBuffer[0] == L'\\') &&
            (ReparseBuffer->MountPointReparseBuffer.PathBuffer[1] == L'?') &&
            (ReparseBuffer->MountPointReparseBuffer.PathBuffer[2] == L'?') &&
            (ReparseBuffer->MountPointReparseBuffer.PathBuffer[3] == L'\\') &&
            (ReparseBuffer->MountPointReparseBuffer.PathBuffer[4] == L'U') &&
            (ReparseBuffer->MountPointReparseBuffer.PathBuffer[5] == L'N') &&
            (ReparseBuffer->MountPointReparseBuffer.PathBuffer[6] == L'C') &&
            (ReparseBuffer->MountPointReparseBuffer.PathBuffer[7] == L'\\')) {

            //
            // The name is not one we want to deal with.
            //

            return FALSE;
        }

        //
        // See whether there is a drive letter that is mapped at the beginning of the name.
        // If the drive letter is C, then the prefix has the form  \??\C:
        // Note that we skip the offset 4 on purpose.
        //

        if ((ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength > 12) &&
            (ReparseBuffer->MountPointReparseBuffer.PathBuffer[0] == L'\\') &&
            (ReparseBuffer->MountPointReparseBuffer.PathBuffer[1] == L'?') &&
            (ReparseBuffer->MountPointReparseBuffer.PathBuffer[2] == L'?') &&
            (ReparseBuffer->MountPointReparseBuffer.PathBuffer[3] == L'\\') &&
            (ReparseBuffer->MountPointReparseBuffer.PathBuffer[5] == L':')) {

            NTSTATUS           status;
            UNICODE_STRING     linkValue;
            OBJECT_ATTRIBUTES  objectAttributes;
            HANDLE             linkHandle;
            PWCHAR             linkValueBuffer = NULL;   //  MAX_PATH is 260
            WCHAR              pathNameValue[sizeof(L"\\??\\C:\0")];

            RtlCopyMemory( &pathNameValue, L"\\??\\C:\0", sizeof(L"\\??\\C:\0") );

            RtlInitUnicodeString( &drivePath, pathNameValue );

            //
            // Place the appropriate drive letter in the buffer overwriting offset 4.
            //

            drivePath.Buffer[4] = ReparseBuffer->MountPointReparseBuffer.PathBuffer[4];

            InitializeObjectAttributes( &objectAttributes,
                                        &drivePath,
                                        OBJ_CASE_INSENSITIVE,
                                        (HANDLE) NULL,
                                        (PSECURITY_DESCRIPTOR) NULL );

            status = ZwOpenSymbolicLinkObject( &linkHandle,
                                               SYMBOLIC_LINK_QUERY,
                                               &objectAttributes );

#if DBG
//     DbgPrint( "IO Name grafting drivePath: %Z ZwOpenSymbolicLinkObject status %x\n", &drivePath, status );
#endif // DBG

            if ( NT_SUCCESS( status ) ) {

                //
                // Now query the link and see if there is a redirection
                //

                linkValueBuffer = ExAllocatePoolWithTag( NonPagedPool,
                                                         2 * 260,
                                                         '  oI' );
                if ( !linkValueBuffer ) {

                    //
                    // Insufficient resources. Return FALSE.
                    //

                    NtClose( linkHandle );
                    return FALSE;
                }

                linkValue.Buffer = linkValueBuffer;
                linkValue.Length = 0;
                linkValue.MaximumLength = (USHORT)(2 * 260);

                status = ZwQuerySymbolicLinkObject( linkHandle,
                                                    &linkValue,
                                                    NULL );
                NtClose( linkHandle );

#if DBG
//     DbgPrint( "IO Name grafting linkValue: %Z ZwQuerySymbolicLinkObject status %x\n", &linkValue, status );
#endif // DBG

                if ( NT_SUCCESS( status ) ) {

                    //
                    // The link is a re-directed drive when it has the prefix
                    // \Device\LanmanRedirector\
                    //

                    if ((linkValue.Buffer[ 0] == L'\\') &&
                        (linkValue.Buffer[ 1] == L'D') &&
                        (linkValue.Buffer[ 2] == L'e') &&
                        (linkValue.Buffer[ 3] == L'v') &&
                        (linkValue.Buffer[ 4] == L'i') &&
                        (linkValue.Buffer[ 5] == L'c') &&
                        (linkValue.Buffer[ 6] == L'e') &&
                        (linkValue.Buffer[ 7] == L'\\') &&
                        (linkValue.Buffer[ 8] == L'L') &&
                        (linkValue.Buffer[ 9] == L'a') &&
                        (linkValue.Buffer[10] == L'n') &&
                        (linkValue.Buffer[14] == L'R') &&
                        (linkValue.Buffer[15] == L'e') &&
                        (linkValue.Buffer[16] == L'd') &&
                        (linkValue.Buffer[17] == L'i') &&
                        (linkValue.Buffer[18] == L'r') &&
                        (linkValue.Buffer[23] == L'r') &
                        (linkValue.Buffer[24] == L'\\')) {

                        //
                        // Free the buffer.
                        //

                        ExFreePool( linkValueBuffer );

                        return FALSE;
                    }

                    //
                    // Fix for penetration bug. May be relaxed in the future 03/99.
                    // Exploit the fact that we have queried the symbolic link.
                    // Want only to allow links to volumes that have the prefix
                    //   \Device\Harddisk
                    //

                    if (!((linkValue.Buffer[ 0] == L'\\') &&
                          (linkValue.Buffer[ 1] == L'D') &&
                          (linkValue.Buffer[ 2] == L'e') &&
                          (linkValue.Buffer[ 3] == L'v') &&
                          (linkValue.Buffer[ 4] == L'i') &&
                          (linkValue.Buffer[ 5] == L'c') &&
                          (linkValue.Buffer[ 6] == L'e') &&
                          (linkValue.Buffer[ 7] == L'\\') &&
                          (linkValue.Buffer[ 8] == L'H') &&
                          (linkValue.Buffer[ 9] == L'a') &&
                          (linkValue.Buffer[10] == L'r') &&
                          (linkValue.Buffer[11] == L'd') &&
                          (linkValue.Buffer[12] == L'd') &&
                          (linkValue.Buffer[13] == L'i') &&
                          (linkValue.Buffer[14] == L's') &&
                          (linkValue.Buffer[15] == L'k'))) {

                        //
                        // Free the buffer.
                        //

                        ExFreePool( linkValueBuffer );

                        return FALSE;
                    }
                }

                //
                // Free the buffer.
                //

                ExFreePool( linkValueBuffer );
            }
        }
    }

    //
    // Fix for penetration bug. May be relaxed in the future 03/99.
    // Determine that we either have an NT file name or a volume mount point target name.
    //
    // This closes the door of having an arbitrary device name that, with the help of the
    // server, can be used to bypass access checks to the underlying device.
    //

    {
        UNICODE_STRING volumeName;

        if (
            //
            // The shortest valid name is one of the kind \??\C: whose length is 12 when
            // in Unicode. All names used by volume mount points are longer.
            //

            ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength < 12 ) {

            return FALSE;
        }

        //
        // The name has at least 6 Unicode characters.
        //
        // We have verified above that MountPointReparseBuffer.SubstituteNameOffset
        // is zero.
        //

        volumeName.Length =
        volumeName.MaximumLength = ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength;
        volumeName.Buffer = (PWSTR) ReparseBuffer->MountPointReparseBuffer.PathBuffer;

        //
        // When we do not have a name that begins with a drive letter and it is not
        // a valid volume mount point name then we return false.
        //

        if ( !((ReparseBuffer->MountPointReparseBuffer.PathBuffer[0] == L'\\') &&
               (ReparseBuffer->MountPointReparseBuffer.PathBuffer[1] == L'?') &&
               (ReparseBuffer->MountPointReparseBuffer.PathBuffer[2] == L'?') &&
               (ReparseBuffer->MountPointReparseBuffer.PathBuffer[3] == L'\\') &&
               //
               // Notice that we skip index 4, where the drive letter is to be.
               //
               (ReparseBuffer->MountPointReparseBuffer.PathBuffer[5] == L':'))

             &&

             !MOUNTMGR_IS_VOLUME_NAME( &volumeName ) ) {

            return FALSE;
        }
    }

    //
    // Otherwise return TRUE.
    //

    return TRUE;
}

VOID
IopDoNameTransmogrify(
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PREPARSE_DATA_BUFFER ReparseBuffer
    )

/*++

Routine Description:

    This routine is called to do the name grafting needed for junctions.

Arguments:

    Irp - Pointer to the I/O Request Packet (IRP) representing the operation
        to be performed.

    FileObject - Pointer to the file object whose name is being affected.

    ReparseBuffer - Pointer to a reparse data buffer that is supposed to contain
        a self-consistent set of names to perform name grafting.

Return Value:

    No explicit return value. The appropriate fields off the IRP are set.

Note:

    This function needs to be kept synchronized with the definition of
    REPARSE_DATA_BUFFER.

--*/

{
    USHORT pathLength = 0;
    USHORT neededBufferLength = 0;
    PVOID outBuffer = NULL;
    PWSTR pathBuffer = NULL;

    //
    // We do the appropriate paste of the new name in the FileName buffer
    // and deallocate the buffer that brought the data from the file system.
    //

    ASSERT( Irp->IoStatus.Status == STATUS_REPARSE );
    ASSERT( Irp->IoStatus.Information == IO_REPARSE_TAG_MOUNT_POINT );

    ASSERT( Irp->Tail.Overlay.AuxiliaryBuffer != NULL );

    ASSERT( ReparseBuffer != NULL );
    ASSERT( ReparseBuffer->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT );
    ASSERT( ReparseBuffer->ReparseDataLength < MAXIMUM_REPARSE_DATA_BUFFER_SIZE );
    ASSERT( ReparseBuffer->Reserved < MAXIMUM_REPARSE_DATA_BUFFER_SIZE );

#if DBG
//    DbgPrint( "iosubs.c DoNameTransmogrify: Tag %x Reserved %x MaximumLength %x FileName %Z\n",
//              ReparseBuffer->ReparseTag, ReparseBuffer->Reserved, FileObject->FileName.MaximumLength, &(FileObject->FileName) );
#endif // DBG

    //
    // Determine whether we have enough data for all the length fields.
    //
    // Determine whether the lengths returned are consistent with the maximum
    // buffer. This is the best self-defense check we can do at this time as
    // the stack pointer is already invalid.
    //

    if (ReparseBuffer->ReparseDataLength >=
        (FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer[0]) - REPARSE_DATA_BUFFER_HEADER_SIZE)) {

        if (MAXIMUM_REPARSE_DATA_BUFFER_SIZE <
            (FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer[0]) +
                ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength +
                ReparseBuffer->MountPointReparseBuffer.PrintNameLength)) {

            Irp->IoStatus.Status = STATUS_IO_REPARSE_DATA_INVALID;
        }
    } else {
        Irp->IoStatus.Status = STATUS_IO_REPARSE_DATA_INVALID;
    }

    //
    // The value in  ReparseBuffer->Reserved  is the length of the file
    // name that has still to be parsed.
    //

    //
    // Copy the buffer when it has the appropriate length, else return a null UNICODE name:
    //   (1) Do defensive sanity checks on the name lengths returned.
    //
    // We only care to do this if we have no error conditions.
    //

    if (NT_SUCCESS( Irp->IoStatus.Status )) {

        pathBuffer = (PWSTR)((PCHAR)ReparseBuffer->MountPointReparseBuffer.PathBuffer +
                             ReparseBuffer->MountPointReparseBuffer.SubstituteNameOffset);
        pathLength = ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength;
    }

    //
    // Notice that if the data returned in AuxiliaryBuffer is not long enough then
    // pathLength has value 0 and pathBuffer has value NULL.
    //
    // The value in  ReparseBuffer->Reserved  is the length of the file name that
    // has still to be parsed.
    //
    // We only care to do this if we have no error conditions.
    //

    if (ReparseBuffer->Reserved < 0) {

        //
        // This is an invalid offset.
        //

        Irp->IoStatus.Status = STATUS_IO_REPARSE_DATA_INVALID;
    }

    if (NT_SUCCESS( Irp->IoStatus.Status )) {

        //
        // Check for overflow. (pathLength <= MAXIMUM_REPARSE_DATA_BUFFER_SIZE)
        // so pathLength + sizeof(UNICODE_NULL) cannot overflow.
        //

        if (((USHORT)MAXUSHORT - ReparseBuffer->Reserved ) > (pathLength +(USHORT)sizeof(UNICODE_NULL))) {
            neededBufferLength = pathLength + ReparseBuffer->Reserved + sizeof( UNICODE_NULL );
            //
            // If the out name buffer isn't large enough, allocate a new one.
            //

            if (FileObject->FileName.MaximumLength < neededBufferLength) {
                outBuffer = ExAllocatePoolWithTag( PagedPool,
                                                   neededBufferLength,
                                                   'cFoI' );
                if (!outBuffer) {
                    Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                outBuffer = FileObject->FileName.Buffer;
            }
        } else {
            Irp->IoStatus.Status = STATUS_NAME_TOO_LONG;
        }
    }

    //
    // Place in the out name buffer the remaining part of the name.
    //
    // We only care to do this if we have no error conditions.
    //

    if (NT_SUCCESS( Irp->IoStatus.Status )) {

        if (ReparseBuffer->Reserved) {

            RtlMoveMemory ( (PCHAR)outBuffer + pathLength,
                            (PCHAR)FileObject->FileName.Buffer +
                                  (FileObject->FileName.Length - ReparseBuffer->Reserved),
                            ReparseBuffer->Reserved );
        }

        //
        // Copy into the front of the out name buffer the value of the
        // reparse point.
        //

        if (pathLength) {

            RtlCopyMemory( (PCHAR)outBuffer,
                           (PCHAR)pathBuffer,
                           pathLength );
        }

        FileObject->FileName.Length = neededBufferLength - sizeof( UNICODE_NULL );

        //
        // Free the old name buffer when needed and update the appropriate values.
        //

        if (outBuffer != FileObject->FileName.Buffer) {

            if (FileObject->FileName.Buffer != NULL) {
                ExFreePool( FileObject->FileName.Buffer );
            }
            FileObject->FileName.Buffer = outBuffer;
            FileObject->FileName.MaximumLength = neededBufferLength;
            ((PWSTR)outBuffer)[ (neededBufferLength / sizeof( WCHAR ))-1 ] = UNICODE_NULL;
        }
    }

    //
    // Free the buffer that came from the file system.
    // NULL the pointer.
    //

    ExFreePool( ReparseBuffer );
    ReparseBuffer = NULL;

    //
    // Clear the tag from then Information field.
    //

    Irp->IoStatus.Information = IO_REPARSE_TAG_RESERVED_ZERO;
}

PIRP
IoMakeAssociatedIrp(
    IN PIRP Irp,
    IN CCHAR StackSize
    )

/*++

Routine Description:

    This routine allocates an I/O Request Packet from the system nonpaged pool
    and makes it an associated IRP to the specified IRP.  The packet will be
    allocated to contain StackSize stack locations.  The IRP iwll also be
    initialized.

    Note that it is up to the caller to have set the number of associated IRPs
    in the master packet before calling this routine for the first time.  The
    count should be set in the master packet in:  AssociatedIrp.IrpCount.

Arguments:

    Irp - Pointer to master IRP to be associated with.

    StackSize - Specifies the maximum number of stack locations required.

Return Value:

    The function value is the address of the associated IRP or NULL, if the
    IRP could be allocated.

--*/

{
    USHORT allocateSize;
    UCHAR fixedSize;
    PIRP associatedIrp;
    PNPAGED_LOOKASIDE_LIST lookasideList;
    UCHAR mustSucceed;
    PP_NPAGED_LOOKASIDE_NUMBER number;
    USHORT packetSize;
    PKPRCB prcb;

    //
    // If the size of the packet required is less than or equal to those on
    // the lookaside lists, then attempt to allocate the packet from the
    // lookaside lists.
    //

    associatedIrp = NULL;
    fixedSize = 0;
    mustSucceed = 0;
    packetSize = IoSizeOfIrp(StackSize);
    allocateSize = packetSize;
    if (StackSize <= (CCHAR)IopLargeIrpStackLocations) {
        fixedSize = IRP_ALLOCATED_FIXED_SIZE;
        number = LookasideSmallIrpList;
        if (StackSize != 1) {
            allocateSize = IoSizeOfIrp((CCHAR)IopLargeIrpStackLocations);
            number = LookasideLargeIrpList;
        }

        prcb = KeGetCurrentPrcb();
        lookasideList = prcb->PPLookasideList[number].P;
        lookasideList->L.TotalAllocates += 1;
        associatedIrp = (PIRP)ExInterlockedPopEntrySList(&lookasideList->L.ListHead,
                                                         &lookasideList->Lock);

        if (associatedIrp == NULL) {
            lookasideList->L.AllocateMisses += 1;
            lookasideList = prcb->PPLookasideList[number].L;
            lookasideList->L.TotalAllocates += 1;
            associatedIrp = (PIRP)ExInterlockedPopEntrySList(&lookasideList->L.ListHead,
                                                             &lookasideList->Lock);
        }
    }

    //
    // If an IRP was not allocated from the lookaside list, then allocate
    // the packet from nonpaged pool.
    //

    if (!associatedIrp) {
        if (fixedSize != 0) {
            lookasideList->L.AllocateMisses += 1;
        }

        //
        // There are no free packets on the lookaside list, or the packet is
        // too large to be allocated from one of the lists, so it must be
        // allocated from general non-paged pool.
        //

        associatedIrp = ExAllocatePoolWithTag(NonPagedPool, allocateSize, ' prI');
        if (!associatedIrp) {
            if (KeGetPreviousMode() == KernelMode) {
                mustSucceed = IRP_ALLOCATED_MUST_SUCCEED;
                associatedIrp = ExAllocatePoolWithTag(NonPagedPoolMustSucceed,
                                                      allocateSize,
                                                      ' prI' );
            }

            if (!associatedIrp) {
                return NULL;
            }
        }

    }

    //
    // Initialize the packet.
    //

    IopInitializeIrp(associatedIrp, packetSize, StackSize);
    associatedIrp->Flags |= IRP_ASSOCIATED_IRP;
    associatedIrp->AllocationFlags |= (fixedSize | mustSucceed);

    //
    // Set the thread ID to be that of the master.
    //

    associatedIrp->Tail.Overlay.Thread = Irp->Tail.Overlay.Thread;

    //
    // Now make the association between this packet and the master.
    //

    associatedIrp->AssociatedIrp.MasterIrp = Irp;
    return associatedIrp;
}

BOOLEAN
IoPageFileCreated(
    IN HANDLE FileHandle
    )

/*++

Routine Description:

    This routine is invoked by the Memory Manager when a new page file is
    created.  The purpose is to lookup the paging file to determine whether
    or not it was created on the boot device.  If so, and taking crash dumps
    is enabled, then the retrieval pointers are queried from the file system
    for the file so that they can then be used later in writing the crash
    dump to the paging file.

    The Memory Manager will continue to invoke this function for each paging
    file created in the system until the one for the boot device is encountered.

Arguments:

    FileHandle - Handle to the paging file just created.

Return Value:

    A value of TRUE is returned if this paging file resides on the system's
    boot device, otherwise FALSE.

--*/

{
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    NTSTATUS status;
    BOOLEAN result;

    PAGED_CODE();

     return IopConfigureCrashDump( FileHandle );
}

NTSTATUS
IoPageRead(
    IN PFILE_OBJECT FileObject,
    IN PMDL MemoryDescriptorList,
    IN PLARGE_INTEGER StartingOffset,
    IN PKEVENT Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )

/*++

Routine Description:

    This routine provides a special, fast interface for the Pager to read pages
    in from the disk quickly and with very little overhead.  All of the special
    handling for this request is recognized by setting the IRP_PAGING_IO flag
    in the IRP flags word.  In-page operations are detected by using the IRP
    flag IRP_INPUT_OPERATION.

Arguments:

    FileObject - A pointer to a referenced file object describing which file
        the read should be performed from.

    MemoryDescriptorList - An MDL which describes the physical pages that the
        pages should be read into from the disk.  All of the pages have been
        locked in memory.  The MDL also describes the length of the read
        operation.

    StartingOffset - Pointer to the offset in the file from which the read
        should take place.

    Event - A pointer to a kernel event structure to be used for synchronization
        purposes.  The event will be set to the Signaled state once the in-page
        operation completes.

    IoStatusBlock - A pointer to the I/O status block in which the final status
        and information should be stored.

Return Value:

    The function value is the final status of the queue request to the I/O
    system subcomponents.

Notes:

    This routine is invoked at APC_LEVEL; this level is honored throughout the
    execution of the entire I/O request, including completion.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT deviceObject;

    //
    // Increment performance counter.  The Cache Manager I/Os always are
    // "recursive".
    //

    if (MmIsRecursiveIoFault()) {
        *CcMissCounter += (MemoryDescriptorList->ByteCount + PAGE_SIZE - 1) >> PAGE_SHIFT;
    }

    //
    // Begin by getting a pointer to the device object that the file resides
    // on.
    //

    deviceObject = IoGetRelatedDeviceObject( FileObject );

    //
    // Allocate an I/O Request Packet (IRP) for this in-page operation.
    //

    irp = IoAllocateIrp( deviceObject->StackSize, FALSE );
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get a pointer to the first stack location in the packet.  This location
    // will be used to pass the function codes and parameters to the first
    // driver.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    //
    // Fill in the IRP according to this request.
    //

    irp->MdlAddress = MemoryDescriptorList;
    irp->Flags = IRP_PAGING_IO | IRP_NOCACHE | IRP_SYNCHRONOUS_PAGING_IO | IRP_INPUT_OPERATION;
    irp->RequestorMode = KernelMode;
    irp->UserIosb = IoStatusBlock;
    irp->UserEvent = Event;
    irp->UserBuffer = (PVOID) ((PCHAR) MemoryDescriptorList->StartVa + MemoryDescriptorList->ByteOffset);
    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Fill in the normal read parameters.
    //

    irpSp->MajorFunction = IRP_MJ_READ;
    irpSp->FileObject = FileObject;
    irpSp->Parameters.Read.Length = MemoryDescriptorList->ByteCount;
    irpSp->Parameters.Read.ByteOffset = *StartingOffset;

    //
    // Queue the packet to the appropriate driver based on whether or not there
    // is a VPB associated with the device.
    //

    return IoCallDriver( deviceObject, irp );
}

NTSTATUS
IoQueryFileInformation(
    IN PFILE_OBJECT FileObject,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN ULONG Length,
    OUT PVOID FileInformation,
    OUT PULONG ReturnedLength
    )

/*++

Routine Description:

    This routine returns the requested information about a specified file.
    The information returned is determined by the FileInformationClass that
    is specified, and it is placed into the caller's FileInformation buffer.

Arguments:

    FileObject - Supplies a pointer to the file object about which the requested
        information is returned.

    FileInformationClass - Specifies the type of information which should be
        returned about the file.

    Length - Supplies the length, in bytes, of the FileInformation buffer.

    FileInformation - Supplies a buffer to receive the requested information
        returned about the file.  This buffer must not be pageable and must
        reside in system space.

    ReturnedLength - Supplies a variable that is to receive the length of the
        information written to the FileInformation buffer.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    PAGED_CODE();

    //
    // Simply invoke the common routine to perform the query operation.
    //

    return IopQueryXxxInformation( FileObject,
                                   FileInformationClass,
                                   Length,
                                   FileInformation,
                                   ReturnedLength,
                                   TRUE );
}

NTSTATUS
IoQueryVolumeInformation(
    IN PFILE_OBJECT FileObject,
    IN FS_INFORMATION_CLASS FsInformationClass,
    IN ULONG Length,
    OUT PVOID FsInformation,
    OUT PULONG ReturnedLength
    )

/*++

Routine Description:

    This routine returns the requested information about a specified volume.
    The information returned is determined by the FsInformationClass that
    is specified, and it is placed into the caller's FsInformation buffer.

Arguments:

    FileObject - Supplies a pointer to the file object about which the requested
        information is returned.

    FsInformationClass - Specifies the type of information which should be
        returned about the volume.

    Length - Supplies the length of the FsInformation buffer in bytes.

    FsInformation - Supplies a buffer to receive the requested information
        returned about the file.  This buffer must not be pageable and must
        reside in system space.

    ReturnedLength - Supplies a variable that is to receive the length of the
        information written to the FsInformation buffer.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    PAGED_CODE();

    //
    // Simply invoke the common routine to perform the query operation.
    //

    return IopQueryXxxInformation( FileObject,
                                   FsInformationClass,
                                   Length,
                                   FsInformation,
                                   ReturnedLength,
                                   FALSE );
}

VOID
IoQueueThreadIrp(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine queues the specified I/O Request Packet (IRP) to the current
    thread's IRP pending queue.  This queue locates all of the outstanding
    I/O requests for the thread.

Arguments:

    Irp - Pointer to the I/O Request Packet (IRP) to be queued.

Return Value:

    None.

--*/

{
    //
    // Simply queue the packet using the internal queueing routine.
    //

    IopQueueThreadIrp( Irp );
}

VOID
IoRaiseHardError(
    IN PIRP Irp,
    IN PVPB Vpb OPTIONAL,
    IN PDEVICE_OBJECT RealDeviceObject
    )

/*++

Routine Description:

    This routine pops up a hard error in the context of the thread that
    originally requested the I/O operation specified by the input IRP.  This
    is done by queueing a kernel APC to the original thread, passing it a
    pointer to the device objects and the IRP.  Once the pop up is performed,
    the routine either completes the I/O request then, or it calls the driver
    back with the same IRP.

    If the original request was an IoPageRead, then it was at APC level and
    we have to create a thread to "hold" this pop-up.  Note that we have to
    queue to an ExWorker thread to create the thread since this can only be
    done from the system process.

Arguments:

    Irp - A pointer to the I/O Request Packet (IRP) for the request that
        failed.

    Vpb - This is the volume parameter block of the offending media.  If the
        media not yet mounted, this parameter should be absent.

    RealDeviceObject - A pointer to the device object that represents the
        device that the file system believes it has mounted.  This is
        generally the "real" device object in the VPB, but may, in fact,
        be a device object attached to the physical device.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp;

    //
    // If pop-ups are disabled for the requesting thread, just complete the
    // request.
    //

    if (Irp->Tail.Overlay.Thread->HardErrorsAreDisabled) {

        //
        // An error was incurred, so zero out the information field before
        // completing the request if this was an input operation.  Otherwise,
        // IopCompleteRequest will try to copy to the user's buffer.
        //

        if (Irp->Flags & IRP_INPUT_OPERATION) {
            Irp->IoStatus.Information = 0;
        }

        IoCompleteRequest( Irp, IO_DISK_INCREMENT );

        return;
    }

    //
    //  If this Irp resulted from a call to IoPageRead(), the caller must
    //  have been at APC level, so don't try enqueing an APC.
    //
    //  Also if this is a cleanup Irp, force this pop-up to go to the new
    //  thread so that it cannot be disabled.
    //

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    if ((Irp->Flags == (IRP_PAGING_IO |
                        IRP_NOCACHE |
                        IRP_SYNCHRONOUS_PAGING_IO |
                        IRP_INPUT_OPERATION)) ||
        (IrpSp->MajorFunction == IRP_MJ_CLEANUP)) {

        PIOP_APC_HARD_ERROR_PACKET packet;

        packet = ExAllocatePoolWithTag( NonPagedPool,
                                        sizeof( IOP_APC_HARD_ERROR_PACKET ),
                                        'rEoI' );

        if ( packet == NULL ) {

            IoCompleteRequest( Irp, IO_DISK_INCREMENT );
            return;
        }

        ExInitializeWorkItem( &packet->Item, IopStartApcHardError, packet );
        packet->Irp = Irp;
        packet->Vpb = Vpb;
        packet->RealDeviceObject = RealDeviceObject;

        ExQueueWorkItem( &packet->Item, CriticalWorkQueue );

    } else {

        PKAPC apc;

        //
        // Begin by allocating and initializing an APC that can be sent to the
        // target thread.
        //

        apc = ExAllocatePoolWithTag( NonPagedPool, sizeof( KAPC ), 'CPAK' );

        //
        // If we could not get the pool, we have no choice but to just complete
        // the Irp, thereby passing the error onto the caller.
        //

        if ( apc == NULL ) {

            IoCompleteRequest( Irp, IO_DISK_INCREMENT );
            return;
        }

        KeInitializeApc( apc,
                         &Irp->Tail.Overlay.Thread->Tcb,
                         Irp->ApcEnvironment,
                         IopDeallocateApc,
                         IopAbortRequest,
                         IopRaiseHardError,
                         KernelMode,
                         Irp );

        (VOID) KeInsertQueueApc( apc,
                                 Vpb,
                                 RealDeviceObject,
                                 0 );
    }
}

BOOLEAN
IoRaiseInformationalHardError(
    IN NTSTATUS ErrorStatus,
    IN PUNICODE_STRING String OPTIONAL,
    IN PKTHREAD Thread OPTIONAL
    )
/*++

Routine Description:

    This routine pops up a hard error in the hard error popup thread.  The
    routine returns immediately, enqueuing the actual pop-up to a worker
    thread.  The hard error that is raised is informational in the sense that
    only the OK button is displayed.

Arguments:

    ErrorStatus - The error condition.

    String - Depending on the error, a string may have to be enqueued.

    Thread - If present, enqueue an APC to this thread rather than using
        the hard error thread.

Return Value:

    BOOLEAN - TRUE if we decided to dispatch a pop-up.  FALSE if we decided
        not to because:

        - pop-ups are disabled in the requested thread, or

        - a pool allocation failed, or

        - an equivalent pop-up is currently pending a user response (i.e.
          waiting for the user to press <OK>) or in the queue, or

        - too many pop-ups have already been queued.

--*/

//
//  This macro compares two pop-ups to see if they are content equivalent.
//

#define ArePacketsEquivalent(P1,P2) (                              \
    (P1->ErrorStatus == P2->ErrorStatus) &&                        \
    ((!P1->String.Buffer && !P2->String.Buffer) ||                 \
     ((P1->String.Length == P2->String.Length) &&                  \
      (RtlEqualMemory(P1->String.Buffer,                           \
                        P2->String.Buffer,                         \
                        P1->String.Length))))                      \
)

{
    KIRQL oldIrql;
    PVOID stringBuffer;
    PLIST_ENTRY links;

    PIOP_HARD_ERROR_PACKET hardErrorPacket;

    //
    // If pop-ups are disabled for the requesting thread, just return.
    //

    if (ARGUMENT_PRESENT(Thread) ?
        CONTAINING_RECORD(Thread, ETHREAD, Tcb)->HardErrorsAreDisabled :
        PsGetCurrentThread()->HardErrorsAreDisabled) {

        return FALSE;
    }

    //
    // If this is one of those special error popup codes that CSRSS expects
    // to be called with a correct set of arguments, disallow from a driver
    //
    if ( ErrorStatus == STATUS_VDM_HARD_ERROR ||
         ErrorStatus == STATUS_UNHANDLED_EXCEPTION ||
         ErrorStatus == STATUS_SERVICE_NOTIFICATION ) {
        return FALSE;
    }

    //
    //  If this request is going to be sent to the hard error thread, and
    //  there are more than 25 entries already in the queue, don't
    //  add any more.  We'll do another safe check later on.
    //

    if ( !ARGUMENT_PRESENT( Thread ) &&
         (KeReadStateSemaphore( &IopHardError.WorkQueueSemaphore ) >=
          IOP_MAXIMUM_OUTSTANDING_HARD_ERRORS) ) {

        return FALSE;
    } else {
        if (IopHardError.NumPendingApcPopups > IOP_MAXIMUM_OUTSTANDING_HARD_ERRORS) {
            return FALSE;
        }
    }

    //
    //  Allocate the packet, and a buffer for the string if present.
    //

    hardErrorPacket = ExAllocatePoolWithTag( NonPagedPool,
                                             sizeof(IOP_HARD_ERROR_PACKET),
                                             'rEoI');

    if (!hardErrorPacket) { return FALSE; }

    //
    //  Zero out the packet and fill the NT_STATUS we will pop up.
    //

    RtlZeroMemory( hardErrorPacket, sizeof(IOP_HARD_ERROR_PACKET) );

    hardErrorPacket->ErrorStatus = ErrorStatus;

    //
    //  If there is a string, copy it.
    //

    if ( ARGUMENT_PRESENT( String ) && String->Length ) {

        stringBuffer = ExAllocatePoolWithTag( NonPagedPool,
                                              String->Length,
                                              'rEoI' );

        if (!stringBuffer) {
            ExFreePool( hardErrorPacket );
            return FALSE;
        }

        hardErrorPacket->String.Length = String->Length;
        hardErrorPacket->String.MaximumLength = String->Length;

        hardErrorPacket->String.Buffer = stringBuffer;

        RtlCopyMemory( stringBuffer, String->Buffer, String->Length );
    }

    //
    //  If there is an Thread, enqueue an APC for ourself, otherwise send
    //  it off to the to the hard error thread.
    //

    if ( ARGUMENT_PRESENT( Thread ) ) {

        PKAPC apc;

        //
        // Begin by allocating and initializing an APC that can be sent to the
        // target thread.
        //

        apc = ExAllocatePoolWithTag( NonPagedPool, sizeof( KAPC ), 'CPAK' );

        //
        // If we could not get the pool, we have no choice but to just
        // free the packet and return.
        //

        if ( apc == NULL ) {

            if ( hardErrorPacket->String.Buffer ) {
                ExFreePool( hardErrorPacket->String.Buffer );
            }

            ExFreePool( hardErrorPacket );

            return FALSE;
        }

        InterlockedIncrement(&IopHardError.NumPendingApcPopups);
        KeInitializeApc( apc,
                         Thread,
                         CurrentApcEnvironment,
                         IopDeallocateApc,
                         NULL,
                         IopRaiseInformationalHardError,
                         KernelMode,
                         hardErrorPacket );

        (VOID) KeInsertQueueApc( apc, NULL, NULL, 0 );

    } else {

        //
        //  Get exclusive access to the work queue.
        //

        ExAcquireSpinLock( &IopHardError.WorkQueueSpinLock, &oldIrql );

        //
        //  Check the Signal state again, if OK, go ahead and enqueue.
        //

        if ( KeReadStateSemaphore( &IopHardError.WorkQueueSemaphore ) >=
             IOP_MAXIMUM_OUTSTANDING_HARD_ERRORS ) {

            ExReleaseSpinLock( &IopHardError.WorkQueueSpinLock, oldIrql );

            if ( hardErrorPacket->String.Buffer ) {
                ExFreePool( hardErrorPacket->String.Buffer );
            }
            ExFreePool( hardErrorPacket );
            return FALSE;
        }

        //
        //  If there is a pop-up currently up, check for a match
        //

        if (IopCurrentHardError &&
            ArePacketsEquivalent( hardErrorPacket, IopCurrentHardError )) {

            ExReleaseSpinLock( &IopHardError.WorkQueueSpinLock, oldIrql );

            if ( hardErrorPacket->String.Buffer ) {
                ExFreePool( hardErrorPacket->String.Buffer );
            }
            ExFreePool( hardErrorPacket );
            return FALSE;
        }

        //
        //  Run down the list of queued pop-ups looking for a match.
        //

        links = IopHardError.WorkQueue.Flink;

        while (links != &IopHardError.WorkQueue) {

            PIOP_HARD_ERROR_PACKET queueHardErrorPacket;

            queueHardErrorPacket = CONTAINING_RECORD( links,
                                                      IOP_HARD_ERROR_PACKET,
                                                      WorkQueueLinks );

            if (ArePacketsEquivalent( hardErrorPacket,
                                      queueHardErrorPacket )) {

                ExReleaseSpinLock( &IopHardError.WorkQueueSpinLock, oldIrql );

                if ( hardErrorPacket->String.Buffer ) {
                    ExFreePool( hardErrorPacket->String.Buffer );
                }
                ExFreePool( hardErrorPacket );
                return FALSE;
            }

            links = links->Flink;
        }

        //
        //  Enqueue this packet.
        //

        InsertTailList( &IopHardError.WorkQueue,
                        &hardErrorPacket->WorkQueueLinks );

        //
        //  Bump the count on the semaphore so that the hard error thread
        //  will know that an entry has been placed in the queue.
        //

        (VOID) KeReleaseSemaphore( &IopHardError.WorkQueueSemaphore,
                                   0,
                                   1L,
                                   FALSE );

        //
        //  If we are not currently running in an ExWorkerThread, queue
        //  a work item.
        //

        if ( !IopHardError.ThreadStarted ) {
            IopHardError.ThreadStarted = TRUE;
            ExQueueWorkItem( &IopHardError.ExWorkItem, DelayedWorkQueue );
        }

        //
        //  Finally, release the spinlockevent, allowing access to the work queue again.
        //  The combination of releasing both the event and the semaphore will
        //  enable the thread to wake up and obtain the entry.
        //

        ExReleaseSpinLock( &IopHardError.WorkQueueSpinLock, oldIrql );
    }

    return TRUE;
}

VOID
IoRegisterBootDriverReinitialization(
    IN PDRIVER_OBJECT DriverObject,
    IN PDRIVER_REINITIALIZE DriverReinitializationRoutine,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is invoked by boot drivers during their initialization or
    during their reinitialization to register with the I/O system to be
    called again once all devices have been enumerated and started.
    Note that it  is possible for this to occur during a normally running
    system, if the  driver is loaded dynamically, so all references to the
    reinitialization queue must be synchronized.

Arguments:

    DriverObject - Pointer to the driver's driver object.

    DriverReinitializationRoutine - The address of the reinitialization
        routine that is to be invoked.

    Context - Pointer to the context that is passed to the driver's
        reinitialization routine.

Return Value:

    None.

--*/

{
    PREINIT_PACKET reinitEntry;

    PAGED_CODE();

    //
    // Allocate a reinitialization entry to be inserted onto the list.  Note
    // that if the entry cannot be allocated, then the request is simply
    // dropped on the floor.
    //

    reinitEntry = ExAllocatePoolWithTag( NonPagedPool,
                                         sizeof( REINIT_PACKET ),
                                         'iRoI' );
    if (!reinitEntry) {
        return;
    }

    DriverObject->Flags |= DRVO_BOOTREINIT_REGISTERED;
    reinitEntry->DriverObject = DriverObject;
    reinitEntry->DriverReinitializationRoutine = DriverReinitializationRoutine;
    reinitEntry->Context = Context;

    ExInterlockedInsertTailList( &IopBootDriverReinitializeQueueHead,
                                 &reinitEntry->ListEntry,
                                 &IopDatabaseLock );
}

VOID
IoRegisterDriverReinitialization(
    IN PDRIVER_OBJECT DriverObject,
    IN PDRIVER_REINITIALIZE DriverReinitializationRoutine,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is invoked by drivers during their initialization or during
    their reinitialization to register with the I/O system to be called again
    before I/O system initialization is complete.  Note that it is possible
    for this to occur during a normally running system, if the driver is
    loaded dynamically, so all references to the reinitialization queue must
    be synchronized.

Arguments:

    DriverObject - Pointer to the driver's driver object.

    DriverReinitializationRoutine - The address of the reinitialization
        routine that is to be invoked.

    Context - Pointer to the context that is passed to the driver's
        reinitialization routine.

Return Value:

    None.

--*/

{
    PREINIT_PACKET reinitEntry;

    PAGED_CODE();

    //
    // Allocate a reinitialization entry to be inserted onto the list.  Note
    // that if the entry cannot be allocated, then the request is simply
    // dropped on the floor.
    //

    reinitEntry = ExAllocatePoolWithTag( NonPagedPool,
                                         sizeof( REINIT_PACKET ),
                                         'iRoI' );
    if (!reinitEntry) {
        return;
    }

    DriverObject->Flags |= DRVO_REINIT_REGISTERED;
    reinitEntry->DriverObject = DriverObject;
    reinitEntry->DriverReinitializationRoutine = DriverReinitializationRoutine;
    reinitEntry->Context = Context;

    ExInterlockedInsertTailList( &IopDriverReinitializeQueueHead,
                                 &reinitEntry->ListEntry,
                                 &IopDatabaseLock );
}

VOID
IoRegisterFileSystem(
    IN OUT PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine inserts the device object for the file system which the device
    object represents into the list of file systems in the system.

Arguments:

    DeviceObject - Pointer to device object for the file system.

Return Value:

    None.


--*/

{
    PNOTIFICATION_PACKET nPacket;
    PLIST_ENTRY listHead = NULL;
    PLIST_ENTRY entry;

    PAGED_CODE();

    //
    // Allocate the I/O database resource for a write operation.
    //

    (VOID) ExAcquireResourceExclusive( &IopDatabaseResource, TRUE );

    //
    // Insert the device object into the appropriate file system queue based on
    // the driver type in the device object.  Notice that if the device type is
    // unrecognized, the file system is simply not registered.
    //

    if (DeviceObject->DeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) {
        listHead = &IopNetworkFileSystemQueueHead;
    } else if (DeviceObject->DeviceType == FILE_DEVICE_CD_ROM_FILE_SYSTEM) {
        listHead = &IopCdRomFileSystemQueueHead;
    } else if (DeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM) {
        listHead = &IopDiskFileSystemQueueHead;
    } else if (DeviceObject->DeviceType == FILE_DEVICE_TAPE_FILE_SYSTEM) {
        listHead = &IopTapeFileSystemQueueHead;
    }

    //
    //  Low priority filesystems are inserted one-from-back on the queue (ahead of
    //  raw, behind everything else), as opposed to on the front.
    //

    if (listHead != NULL) {
        if (DeviceObject->Flags & DO_LOW_PRIORITY_FILESYSTEM ) {
            InsertTailList( listHead->Blink,
                            &DeviceObject->Queue.ListEntry );
        } else {
            InsertHeadList( listHead,
                            &DeviceObject->Queue.ListEntry );
        }
    }

    //
    // Ensure that this file system's device is operable.
    //

    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // Notify all of the registered drivers that this file system has been
    // registered as an active file system of some type.
    //

    entry = IopFsNotifyChangeQueueHead.Flink;
    while (entry != &IopFsNotifyChangeQueueHead) {
        nPacket = CONTAINING_RECORD( entry, NOTIFICATION_PACKET, ListEntry );
        entry = entry->Flink;
        nPacket->NotificationRoutine( DeviceObject, TRUE );
    }

    //
    // Release the I/O database resource.
    //

    ExReleaseResource( &IopDatabaseResource );

    //
    // Increment the number of reasons that this driver cannot be unloaded.
    //

    ExInterlockedAddUlong( &DeviceObject->ReferenceCount, 1, &IopDatabaseLock );
}

NTSTATUS
IoRegisterFsRegistrationChange(
    IN PDRIVER_OBJECT DriverObject,
    IN PDRIVER_FS_NOTIFICATION DriverNotificationRoutine
    )

/*++

Routine Description:

    This routine registers the specified driver's notification routine to be
    invoked whenever a file system registers or unregisters itself as an active
    file system in the system.

Arguments:

    DriverObject - Pointer to the driver object for the driver.

    DriverNotificationRoutine - Address of routine to invoke when a file system
        registers or unregisters itself.

Return Value:

    The return status is the final value of the function.

--*/

{
    PNOTIFICATION_PACKET nPacket;

    PAGED_CODE();

    //
    // Begin by attempting to allocate storage for the shutdown packet.  If
    // one cannot be allocated, simply return an appropriate error.
    //

    nPacket = ExAllocatePoolWithTag( PagedPool,
                                     sizeof( NOTIFICATION_PACKET ),
                                     'sFoI' );
    if (!nPacket) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the notification packet and insert it onto the tail of the
    // list.
    //

    nPacket->DriverObject = DriverObject;
    nPacket->NotificationRoutine = DriverNotificationRoutine;

    ExAcquireResourceExclusive( &IopDatabaseResource, TRUE );
    InsertTailList( &IopFsNotifyChangeQueueHead, &nPacket->ListEntry );
    ExReleaseResource( &IopDatabaseResource );

    //
    // Increment the number of reasons that this driver cannot be unloaded.
    //

    ObReferenceObject( DriverObject );

    return STATUS_SUCCESS;
}

NTSTATUS
IoRegisterLastChanceShutdownNotification(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine allows a driver to register that it would like to have its
    shutdown routine invoked at very late in system shutdown.  This gives
    the driver an opportunity to get control just before the system is fully
    shutdown.

Arguments:

    DeviceObject - Pointer to the driver's device object.

Return Value:

    None.

--*/

{
    PSHUTDOWN_PACKET shutdown;

    PAGED_CODE();

    //
    // Begin by attempting to allocate storage for the shutdown packet.  If
    // one cannot be allocated, simply return an appropriate error.
    //

    shutdown = ExAllocatePoolWithTag( NonPagedPool,
                                      sizeof( SHUTDOWN_PACKET ),
                                      'hSoI' );
    if (!shutdown) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the shutdown packet and insert it onto the head of the list.
    // Note that this is done because some drivers have dependencies on LIFO
    // notification ordering.
    //

    shutdown->DeviceObject = DeviceObject;

    ExInterlockedInsertHeadList( &IopNotifyLastChanceShutdownQueueHead,
                                 &shutdown->ListEntry,
                                 &IopDatabaseLock );

    //
    // Do the bookkeeping to indicate that this driver has successfully
    // registered a shutdown notification routine.
    //

    DeviceObject->Flags |= DO_SHUTDOWN_REGISTERED;

    return STATUS_SUCCESS;
}

NTSTATUS
IoRegisterShutdownNotification(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine allows a driver to register that it would like to have its
    shutdown routine invoked when the system is being shutdown.  This gives
    the driver an opportunity to get control just before the system is fully
    shutdown.

Arguments:

    DeviceObject - Pointer to the driver's device object.

Return Value:

    None.

--*/

{
    PSHUTDOWN_PACKET shutdown;

    PAGED_CODE();

    //
    // Begin by attempting to allocate storage for the shutdown packet.  If
    // one cannot be allocated, simply return an appropriate error.
    //

    shutdown = ExAllocatePoolWithTag( NonPagedPool,
                                      sizeof( SHUTDOWN_PACKET ),
                                      'hSoI' );
    if (!shutdown) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the shutdown packet and insert it onto the head of the list.
    // Note that this is done because some drivers have dependencies on LIFO
    // notification ordering.
    //

    shutdown->DeviceObject = DeviceObject;

    ExInterlockedInsertHeadList( &IopNotifyShutdownQueueHead,
                                 &shutdown->ListEntry,
                                 &IopDatabaseLock );

    //
    // Do the bookkeeping to indicate that this driver has successfully
    // registered a shutdown notification routine.
    //

    DeviceObject->Flags |= DO_SHUTDOWN_REGISTERED;

    return STATUS_SUCCESS;
}

VOID
IoReleaseCancelSpinLock(
    IN KIRQL Irql
    )

/*++

Routine Description:

    This routine is invoked to release the cancel spin lock.  This spin lock
    must be acquired before setting the address of a cancel routine in an
    IRP and released after the cancel routine has been set.

Arguments:

    Irql - Supplies the IRQL value returned from acquiring the spin lock.

Return Value:

    None.

--*/

{
    //
    // Simply release the cancel spin lock.
    //

    ExReleaseSpinLock( &IopCancelSpinLock, Irql );
}

VOID
IoReleaseVpbSpinLock(
    IN KIRQL Irql
    )

/*++

Routine Description:

    This routine is invoked to release the Volume Parameter Block (VPB) spin
    lock.  This spin lock must be acquired before accessing the mount flag,
    reference count, and device object fields of a VPB.

Arguments:

    Irql - Supplies the IRQL value returned from acquiring the spin lock.

Return Value:

    None.

--*/

{
    //
    // Simply release the VPB spin lock.
    //

    ExReleaseSpinLock( &IopVpbSpinLock, Irql );
}

VOID
IoRemoveShareAccess(
    IN PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess
    )

/*++

Routine Description:

    This routine is invoked to remove the access and share access information
    in a file system Share Access structure for a given open instance.

Arguments:

    FileObject - Pointer to the file object of the current access being closed.

    ShareAccess - Pointer to the share access structure that describes
         how the file is currently being accessed.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    // If this accessor wanted some type of access other than READ_ or
    // WRITE_ATTRIBUTES, then account for the fact that he has closed the
    // file.  Otherwise, he hasn't been accounted for in the first place
    // so don't do anything.
    //

    if (FileObject->ReadAccess ||
        FileObject->WriteAccess ||
        FileObject->DeleteAccess) {

        //
        // Decrement the number of opens in the Share Access structure.
        //

        ShareAccess->OpenCount--;

        //
        // For each access type, decrement the appropriate count in the Share
        // Access structure.
        //

        if (FileObject->ReadAccess) {
            ShareAccess->Readers--;
        }

        if (FileObject->WriteAccess) {
            ShareAccess->Writers--;
        }

        if (FileObject->DeleteAccess) {
            ShareAccess->Deleters--;
        }

        //
        // For each shared access type, decrement the appropriate count in the
        // Share Access structure.
        //

        if (FileObject->SharedRead) {
            ShareAccess->SharedRead--;
        }

        if (FileObject->SharedWrite) {
            ShareAccess->SharedWrite--;
        }

        if (FileObject->SharedDelete) {
            ShareAccess->SharedDelete--;
        }
    }
}

VOID
IoSetDeviceToVerify(
    IN PETHREAD Thread,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine sets the device to verify field in the thread object.  This
    function is invoked by file systems to NULL this field, or to set it to
    predefined values.

Arguments:

    Thread - Pointer to the thread whose field is to be set.

    DeviceObject - Pointer to the device to be verified, or NULL, or ...

Return Value:

    None.

Note:

    This function cannot be made a macro, since fields in the thread object
    move from release to release, so this must remain a full function.

--*/

{
    //
    // Simply set the device to be verified in the specified thread.
    //

    Thread->DeviceToVerify = DeviceObject;
}

VOID
IoSetHardErrorOrVerifyDevice(
    IN PIRP Irp,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is invoked when a driver realizes that the media
    has possibly changed on a device, and it must be verified before
    continuing, or a hard error has occured.  The device is stored
    in the thread local storage of the Irp's originating thread.

Arguments:

    Irp - Pointer to an I/O Request Packet to get the thread.

    DeviceObject - This is the device that needs to be verified.

Return Value:

    None.

--*/

{
    //
    // Store the address of the device object that needs verification in
    // the appropriate field of the thread pointed to by the specified I/O
    // Request Packet.
    //

    ASSERT( Irp->Tail.Overlay.Thread != NULL );

    Irp->Tail.Overlay.Thread->DeviceToVerify = DeviceObject;
}

NTSTATUS
IoSetInformation(
    IN PFILE_OBJECT FileObject,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN ULONG Length,
    IN PVOID FileInformation
    )

/*++

Routine Description:

    This routine sets the requested information for the specified file.
    The information that is set is determined by the FileInformationClass
    paramter, and the information itself is passed in the FileInformation
    buffer.

Arguments:

    FileObject - Supplies a pointer to the file object for the file that
        is to be changed.

    FileInformationClass - Specifies the type of information that should
        be set on the file.

    Length - Supplies the length of the FileInformation buffer in bytes.

    FileInformation - A buffer containing the file information to set.  This
        buffer must not be pageable and must reside in system space.

Return Value:

    The status returned is the final completion status of the operation.


--*/

{
    PIRP irp;
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    KEVENT event;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK localIoStatus;
    HANDLE targetHandle = NULL;
    BOOLEAN synchronousIo;

    PAGED_CODE();

    //
    // Reference the file object here so that no special checks need be made
    // in I/O completion to determine whether or not to dereference the file
    // object.
    //

    ObReferenceObject( FileObject );

    //
    // Make a special check here to determine whether this is a synchronous
    // I/O operation.  If it is, then wait here until the file is owned by
    // the current thread.  If this is not a (serialized) synchronous I/O
    // operation, then initialize the local event.
    //

    if (FileObject->Flags & FO_SYNCHRONOUS_IO) {

        BOOLEAN interrupted;

        if (!IopAcquireFastLock( FileObject )) {
            status = IopAcquireFileObjectLock( FileObject,
                                               KernelMode,
                                               (BOOLEAN) ((FileObject->Flags & FO_ALERTABLE_IO) != 0),
                                               &interrupted );
            if (interrupted) {
                ObDereferenceObject( FileObject );
                return status;
            }
        }
        KeClearEvent( &FileObject->Event );
        synchronousIo = TRUE;
    } else {
        KeInitializeEvent( &event, SynchronizationEvent, FALSE );
        synchronousIo = FALSE;
    }

    //
    // Get the address of the target device object.
    //

    deviceObject = IoGetRelatedDeviceObject( FileObject );

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case there is
    // not enough memory to satisfy the request.
    //

    irp = IoAllocateIrp( deviceObject->StackSize, TRUE );
    if (!irp) {

        //
        // An IRP could not be allocated.  Cleanup and return an appropriate
        // error status code.
        //

        IopAllocateIrpCleanup( FileObject, (PKEVENT) NULL );

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = KernelMode;

    //
    // Fill in the service independent parameters in the IRP.
    //

    if (synchronousIo) {
        irp->UserEvent = (PKEVENT) NULL;
    } else {
        irp->UserEvent = &event;
        irp->Flags = IRP_SYNCHRONOUS_API;
    }
    irp->UserIosb = &localIoStatus;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_SET_INFORMATION;
    irpSp->FileObject = FileObject;

    //
    // Set the system buffer address to the address of the caller's buffer and
    // set the flags so that the buffer is not deallocated.
    //

    irp->AssociatedIrp.SystemBuffer = FileInformation;
    irp->Flags |= IRP_BUFFERED_IO;

    //
    // Copy the caller's parameters to the service-specific portion of the IRP.
    //

    irpSp->Parameters.SetFile.Length = Length;
    irpSp->Parameters.SetFile.FileInformationClass = FileInformationClass;

    //
    // Insert the packet at the head of the IRP list for the thread.
    //

    IopQueueThreadIrp( irp );

    //
    // Everything is now set to invoke the device driver with this request.
    // However, it is possible that the information that the caller wants to
    // set is device independent (I/O system dependent).  If this is the case,
    // then the request can be satisfied here without having to have all of
    // the drivers implement the same code.  Note that having the IRP is still
    // necessary since the I/O completion code requires it.
    //

    if (FileInformationClass == FileModeInformation) {

        PFILE_MODE_INFORMATION modeBuffer = FileInformation;

        //
        // Set or clear the appropriate flags in the file object.
        //

        if (!(FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING)) {
            if (modeBuffer->Mode & FILE_WRITE_THROUGH) {
                FileObject->Flags |= FO_WRITE_THROUGH;
            } else {
                FileObject->Flags &= ~FO_WRITE_THROUGH;
            }
        }

        if (modeBuffer->Mode & FILE_SEQUENTIAL_ONLY) {
            FileObject->Flags |= FO_SEQUENTIAL_ONLY;
        } else {
            FileObject->Flags &= ~FO_SEQUENTIAL_ONLY;
        }

        if (modeBuffer->Mode & FO_SYNCHRONOUS_IO) {
            if (modeBuffer->Mode & FILE_SYNCHRONOUS_IO_ALERT) {
                FileObject->Flags |= FO_ALERTABLE_IO;
            } else {
                FileObject->Flags &= ~FO_ALERTABLE_IO;
            }
        }

        status = STATUS_SUCCESS;

        //
        // Complete the I/O operation.
        //

        irp->IoStatus.Status = status;
        irp->IoStatus.Information = 0;

        IoSetNextIrpStackLocation( irp );
        IoCompleteRequest( irp, 0 );

    } else if (FileInformationClass == FileRenameInformation ||
               FileInformationClass == FileLinkInformation ||
               FileInformationClass == FileMoveClusterInformation) {

        //
        // Note that the following code assumes that a rename information
        // and a set link information structure look exactly the same.
        //

        PFILE_RENAME_INFORMATION renameBuffer = FileInformation;

        //
        // Copy the value of the replace BOOLEAN (or the ClusterCount field)
        // from the caller's buffer to the I/O stack location parameter
        // field where it is expected by file systems.
        //

        if (FileInformationClass == FileMoveClusterInformation) {
            irpSp->Parameters.SetFile.ClusterCount =
                ((FILE_MOVE_CLUSTER_INFORMATION *) renameBuffer)->ClusterCount;
        } else {
            irpSp->Parameters.SetFile.ReplaceIfExists = renameBuffer->ReplaceIfExists;
        }

        //
        // Check to see whether or not a fully qualified pathname was supplied.
        // If so, then more processing is required.
        //

        if (renameBuffer->FileName[0] == (UCHAR) OBJ_NAME_PATH_SEPARATOR ||
            renameBuffer->RootDirectory != NULL) {

            //
            // A fully qualified file name was specified as the target of the
            // rename operation.  Attempt to open the target file and ensure
            // that the replacement policy for the file is consistent with the
            // caller's request, and ensure that the file is on the same volume.
            //

            status = IopOpenLinkOrRenameTarget( &targetHandle,
                                                irp,
                                                renameBuffer,
                                                FileObject );
            if (!NT_SUCCESS( status )) {
                IoSetNextIrpStackLocation( irp );
                IoCompleteRequest( irp, 2 );

            } else {

                //
                // The fully qualified file name specifies a file on the same
                // volume and if it exists, then the caller specified that it
                // should be replaced.
                //

                status = IoCallDriver( deviceObject, irp );

            }

        } else {

            //
            // This is a simple rename operation, so call the driver and let
            // it perform the rename operation within the same directory as
            // the source file.
            //

            status = IoCallDriver( deviceObject, irp );

        }

    } else {

        //
        // This is not a request that can be performed here, so invoke the
        // driver at its appropriate dispatch entry with the IRP.
        //

        status = IoCallDriver( deviceObject, irp );

    }

    //
    // If this operation was a synchronous I/O operation, check the return
    // status to determine whether or not to wait on the file object.  If
    // the file object is to be waited on, wait for the operation to complete
    // and obtain the final status from the file object itself.
    //

    if (synchronousIo) {
        if (status == STATUS_PENDING) {
            status = KeWaitForSingleObject( &FileObject->Event,
                                            Executive,
                                            KernelMode,
                                            (BOOLEAN) ((FileObject->Flags & FO_ALERTABLE_IO) != 0),
                                            (PLARGE_INTEGER) NULL );
            if (status == STATUS_ALERTED) {
                IopCancelAlertedRequest( &FileObject->Event, irp );
            }
            status = localIoStatus.Status;
        }
        IopReleaseFileObjectLock( FileObject );

    } else {

        //
        // This is a normal synchronous I/O operation, as opposed to a
        // serialized synchronous I/O operation.  For this case, wait for
        // the local event and copy the final status information back to
        // the caller.
        //

        if (status == STATUS_PENDING) {
            (VOID) KeWaitForSingleObject( &event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          (PLARGE_INTEGER) NULL );
            status = localIoStatus.Status;
        }
    }

    //
    // If a target handle was created because of a rename operation, close
    // the handle now.
    //

    if (targetHandle != (HANDLE) NULL) {
        NtClose( targetHandle );
    }

    return status;
}

VOID
IoSetShareAccess(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    OUT PSHARE_ACCESS ShareAccess
    )

/*++

Routine Description:

    This routine is invoked to set the access and share access information
    in a file system Share Access structure for the first open.

Arguments:

    DesiredAccess - Desired access of current open request.

    DesiredShareAccess - Shared access requested by current open request.

    FileObject - Pointer to the file object of the current open request.

    ShareAccess - Pointer to the share access structure that describes
         how the file is currently being accessed.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    // Set the access type in the file object for the current accessor.
    //

    FileObject->ReadAccess = (BOOLEAN) ((DesiredAccess & (FILE_EXECUTE
        | FILE_READ_DATA)) != 0);
    FileObject->WriteAccess = (BOOLEAN) ((DesiredAccess & (FILE_WRITE_DATA
        | FILE_APPEND_DATA)) != 0);
    FileObject->DeleteAccess = (BOOLEAN) ((DesiredAccess & DELETE) != 0);

    //
    // Check to see whether the current file opener would like to read,
    // write, or delete the file.  If so, account for it in the share access
    // structure; otherwise, skip it.
    //

    if (FileObject->ReadAccess ||
        FileObject->WriteAccess ||
        FileObject->DeleteAccess) {

        //
        // Only update the share modes if the user wants to read, write or
        // delete the file.
        //

        FileObject->SharedRead = (BOOLEAN) ((DesiredShareAccess & FILE_SHARE_READ) != 0);
        FileObject->SharedWrite = (BOOLEAN) ((DesiredShareAccess & FILE_SHARE_WRITE) != 0);
        FileObject->SharedDelete = (BOOLEAN) ((DesiredShareAccess & FILE_SHARE_DELETE) != 0);

        //
        // Set the Share Access structure open count.
        //

        ShareAccess->OpenCount = 1;

        //
        // Set the number of readers, writers, and deleters in the Share Access
        // structure.
        //

        ShareAccess->Readers = FileObject->ReadAccess;
        ShareAccess->Writers = FileObject->WriteAccess;
        ShareAccess->Deleters = FileObject->DeleteAccess;

        //
        // Set the number of shared readers, writers, and deleters in the Share
        // Access structure.
        //

        ShareAccess->SharedRead = FileObject->SharedRead;
        ShareAccess->SharedWrite = FileObject->SharedWrite;
        ShareAccess->SharedDelete = FileObject->SharedDelete;

    } else {

        //
        // No read, write, or delete access has been requested.  Simply zero
        // the appropriate fields in the structure so that the next accessor
        // sees a consistent state.
        //

        ShareAccess->OpenCount = 0;
        ShareAccess->Readers = 0;
        ShareAccess->Writers = 0;
        ShareAccess->Deleters = 0;
        ShareAccess->SharedRead = 0;
        ShareAccess->SharedWrite = 0;
        ShareAccess->SharedDelete = 0;
    }
}

BOOLEAN
IoSetThreadHardErrorMode(
    IN BOOLEAN EnableHardErrors
    )

/*++

Routine Description:

    This routine either enables or disables hard errors for the current
    thread and returns the old state of the flag.

Arguments:

    EnableHardErrors - Supplies a BOOLEAN value indicating whether or not
        hard errors are to be enabled for the current thread.

Return Value:

    The final function value is the previous state of whether or not hard
    errors were enabled.

--*/

{
    PETHREAD thread;
    BOOLEAN oldFlag;

    //
    // Get a pointer to the current thread, capture the current state of
    // hard errors, and set the new state.
    //

    thread = PsGetCurrentThread();
    oldFlag = !thread->HardErrorsAreDisabled;
    thread->HardErrorsAreDisabled = !EnableHardErrors;

    return oldFlag;
}

VOID
IoSetTopLevelIrp(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sets the top level IRP field in the current thread's thread
    object.  This function is invoked by file systems to either set this field
    to the address of an I/O Request Packet (IRP) or to null it.

Arguments:

    Irp - Pointer to the IRP to be stored in the top level IRP field.

Return Value:

    None.

Note:

    This function cannot be made a macro, since fields in the thread object
    move from release to release, so this must remain a full function.

--*/

{
    //
    // Simply set the top level IRP field in the current thread's thread
    // object.
    //

    (PIRP) (PsGetCurrentThread())->TopLevelIrp = Irp;
    return;
}

VOID
IoShutdownSystem (
    IN ULONG Phase
    )

/*++

Routine Description:

    This routine shuts down the I/O portion of the system in preparation
    for a power-off or reboot.

Arguments:

    RebootPending - Indicates whether a reboot is imminently pending.

    Phase - Indicates which phase of shutdown is being performed.

Return Value:

    None

--*/

{
    PLIST_ENTRY entry;
    PSHUTDOWN_PACKET shutdown;
    PDEVICE_OBJECT deviceObject;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PVOID unlockHandle;

    PAGED_CODE();

    //
    // Initialize the event used to synchronize the complete of all of the
    // shutdown routines.
    //

    KeInitializeEvent( &event, NotificationEvent, FALSE );

    if (Phase == 0) {

        //
        // Walk the list of the drivers in the system that have registered
        // themselves as wanting to know when the system is about to be
        // shutdown and invoke each.
        //

        while ((entry = ExInterlockedRemoveHeadList( &IopNotifyShutdownQueueHead, &IopDatabaseLock )) != NULL) {
            shutdown = CONTAINING_RECORD( entry, SHUTDOWN_PACKET, ListEntry );

            //
            // Another driver has been found that has indicated that it requires
            // shutdown notification.  Invoke the driver's shutdown entry point.
            //

            deviceObject = IoGetAttachedDevice( shutdown->DeviceObject );

            irp = IoBuildSynchronousFsdRequest( IRP_MJ_SHUTDOWN,
                                                deviceObject,
                                                (PVOID) NULL,
                                                0,
                                                (PLARGE_INTEGER) NULL,
                                                &event,
                                                &ioStatus );

            if (IoCallDriver( deviceObject, irp ) == STATUS_PENDING) {
#if DBG
                PUNICODE_STRING DeviceName = ObGetObjectName( shutdown->DeviceObject );

                DbgPrint( "IO: Waiting for shutdown of device object (%x) - %wZ\n",
                          shutdown->DeviceObject,
                          DeviceName
                        );
#endif // DBG
                (VOID) KeWaitForSingleObject( &event,
                                              Executive,
                                              KernelMode,
                                              FALSE,
                                              (PLARGE_INTEGER) NULL );
            }

            ExFreePool( shutdown );
            KeClearEvent( &event );
        }

    } else if (Phase == 1) {

#if defined(REMOTE_BOOT)
        //
        // If this is a remote boot client then allow the cache to close the database and
        // mark it clean.
        //

        IopShutdownCsc();
#endif // defined(REMOTE_BOOT)

        // Gain access to the file system header queues by acquiring the
        // database resource for shared access.
        //

        ExAcquireResourceShared( &IopDatabaseResource, TRUE );

        //
        // Loop through each of the disk file systems, invoking each to shutdown
        // each of their mounted volumes.
        //

        for (entry = IopDiskFileSystemQueueHead.Flink;
             entry != &IopDiskFileSystemQueueHead;
             entry = entry->Flink) {

            deviceObject = CONTAINING_RECORD( entry, DEVICE_OBJECT, Queue.ListEntry );
            if (deviceObject->AttachedDevice) {
                deviceObject = IoGetAttachedDevice( deviceObject );
            }

            //
            // Another file system has been found.  Invoke this file system at
            // its shutdown entry point.
            //

            irp = IoBuildSynchronousFsdRequest( IRP_MJ_SHUTDOWN,
                                                deviceObject,
                                                (PVOID) NULL,
                                                0,
                                                (PLARGE_INTEGER) NULL,
                                                &event,
                                                &ioStatus );
            if (IoCallDriver( deviceObject, irp ) == STATUS_PENDING) {
#if DBG
                PUNICODE_STRING DeviceName = ObGetObjectName( deviceObject );

                DbgPrint( "IO: Waiting for shutdown of device object (%x) - %wZ\n",
                          deviceObject,
                          DeviceName
                        );
#endif // DBG
                (VOID) KeWaitForSingleObject( &event,
                                              Executive,
                                              KernelMode,
                                              FALSE,
                                              (PLARGE_INTEGER) NULL );
            }

            KeClearEvent( &event );
        }

        //
        // Walk the list of the drivers in the system that have registered
        // themselves as wanting to know at the last chance when the system
        // is about to be shutdown and invoke each.
        //

        while ((entry = ExInterlockedRemoveHeadList( &IopNotifyLastChanceShutdownQueueHead, &IopDatabaseLock )) != NULL) {
            shutdown = CONTAINING_RECORD( entry, SHUTDOWN_PACKET, ListEntry );

            //
            // Another driver has been found that has indicated that it requires
            // shutdown notification.  Invoke the driver's shutdown entry point.
            //

            deviceObject = IoGetAttachedDevice( shutdown->DeviceObject );

            irp = IoBuildSynchronousFsdRequest( IRP_MJ_SHUTDOWN,
                                                deviceObject,
                                                (PVOID) NULL,
                                                0,
                                                (PLARGE_INTEGER) NULL,
                                                &event,
                                                &ioStatus );

            if (IoCallDriver( deviceObject, irp ) == STATUS_PENDING) {
#if DBG
                PUNICODE_STRING DeviceName = ObGetObjectName( shutdown->DeviceObject );

                DbgPrint( "IO: Waiting for last chance shutdown of device object (%x) - %wZ\n",
                          shutdown->DeviceObject,
                          DeviceName
                        );
#endif // DBG
                (VOID) KeWaitForSingleObject( &event,
                                              Executive,
                                              KernelMode,
                                              FALSE,
                                              (PLARGE_INTEGER) NULL );
            }

            ExFreePool( shutdown );
            KeClearEvent( &event );
        }

        //
        // N.B. The system has stopped.  The IopDatabaseResource lock is
        // not released so that no other mount operations can take place.
        //
        // ExReleaseResource( &IopDatabaseResource );
        //
    }

    return ;
}

VOID
IoStartNextPacket(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Cancelable
    )

/*++

Routine Description:

    This routine is invoked to dequeue the next packet (IRP) from the
    specified device work queue and invoke the device driver's start I/O
    routine for it.  If the Cancelable paramter is TRUE, then the update of
    current IRP is synchronized using the cancel spinlock.

Arguments:

    DeviceObject - Pointer to device object itself.

    Cancelable - Indicates that IRPs in the device queue may be cancelable.

Return Value:

    None.

--*/

{
    KIRQL cancelIrql;
    PIRP irp;
    PKDEVICE_QUEUE_ENTRY packet;

    //
    // Begin by checking to see whether or not this driver's requests are
    // to be considered cancelable.  If so, then acquire the cancel spinlock.
    //

    if (Cancelable) {
        IoAcquireCancelSpinLock( &cancelIrql );
    }

    //
    // Clear the current IRP field before starting another request.
    //

    DeviceObject->CurrentIrp = (PIRP) NULL;

    //
    // Remove the next packet from the head of the queue.  If a packet was
    // found, then process it.
    //

    packet = KeRemoveDeviceQueue( &DeviceObject->DeviceQueue );

    if (packet) {
        irp = CONTAINING_RECORD( packet, IRP, Tail.Overlay.DeviceQueueEntry );

        //
        // A packet was located so make it the current packet for this
        // device.
        //

        DeviceObject->CurrentIrp = irp;
        if (Cancelable) {
           IoReleaseCancelSpinLock( cancelIrql );
        }

        //
        // Invoke the driver's start I/O routine for this packet.
        //

        DeviceObject->DriverObject->DriverStartIo( DeviceObject, irp );
    } else {

        //
        // No packet was found, so simply release the cancel spinlock if
        // it was acquired.
        //

        if (Cancelable) {
           IoReleaseCancelSpinLock( cancelIrql );
        }
    }
}

VOID
IoStartNextPacketByKey(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Cancelable,
    IN ULONG Key
    )

/*++

Routine Description:

    This routine is invoked to dequeue the next packet (IRP) from the
    specified device work queue by key and invoke the device driver's start
    I/O routine for it.  If the Cancelable paramter is TRUE, then the
    update of current IRP is synchronized using the cancel spinlock.

Arguments:

    DeviceObject - Pointer to device object itself.

    Cancelable - Indicates that IRPs in the device queue may be cancelable.

    Key - Specifics the Key used to remove the entry from the queue.

Return Value:

    None.

--*/

{
    KIRQL                cancelIrql;
    PIRP                 irp;
    PKDEVICE_QUEUE_ENTRY packet;

    //
    // Begin by determining whether or not requests for this device are to
    // be considered cancelable.  If so, then acquire the cancel spinlock.
    //

    if (Cancelable) {
        IoAcquireCancelSpinLock( &cancelIrql );
    }

    //
    // Clear the current IRP field before starting another request.
    //

    DeviceObject->CurrentIrp = (PIRP) NULL;

    //
    // Attempt to remove the indicated packet according to the key from the
    // device queue.  If one is found, then process it.
    //

    packet = KeRemoveByKeyDeviceQueue( &DeviceObject->DeviceQueue, Key );

    if (packet) {
        irp = CONTAINING_RECORD( packet, IRP, Tail.Overlay.DeviceQueueEntry );

        //
        // A packet was successfully located.  Make it the current packet
        // and invoke the driver's start I/O routine for it.
        //

        DeviceObject->CurrentIrp = irp;

        if (Cancelable) {
           IoReleaseCancelSpinLock( cancelIrql );
        }

        DeviceObject->DriverObject->DriverStartIo( DeviceObject, irp );

    } else {

        //
        // No packet was found, so release the cancel spinlock if it was
        // acquired.
        //

        if (Cancelable) {
           IoReleaseCancelSpinLock( cancelIrql );
        }
    }
}

VOID
IoStartPacket(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PULONG Key OPTIONAL,
    IN PDRIVER_CANCEL CancelFunction OPTIONAL
    )

/*++

Routine Description:

    This routine attempts to start the specified packet request (IRP) on the
    specified device.  If the device is already busy, then the packet is
    simply queued to the device queue. If a non-NULL CancelFunction is
    supplied, it will be put in the IRP.  If the IRP has been canceled, the
    CancelFunction will be called after the IRP has been inserted into the
    queue or made the current packet.

Arguments:

    DeviceObject - Pointer to device object itself.

    Irp - I/O Request Packet which should be started on the device.

    Key - Key to be used in inserting packet into device queue;  optional
        (if not specified, then packet is inserted at the tail).

    CancelFunction - Pointer to an optional cancel routine.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;
    KIRQL cancelIrql;
    BOOLEAN i;

    //
    // Raise the IRQL of the processor to dispatch level for synchronization.
    //

    KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );

    //
    // If the driver has indicated that packets are cancelable, then acquire
    // the cancel spinlock and set the address of the cancel function to
    // indicate that the packet is not only cancelable, but indicates what
    // routine to invoke should it be cancelled.
    //

    if (CancelFunction) {
        IoAcquireCancelSpinLock( &cancelIrql );
        Irp->CancelRoutine = CancelFunction;
    }

    //
    // If a key parameter was specified, then insert the request into the
    // work queue according to the key;  otherwise, simply insert it at the
    // tail.
    //

    if (Key) {
        i = KeInsertByKeyDeviceQueue( &DeviceObject->DeviceQueue,
                                      &Irp->Tail.Overlay.DeviceQueueEntry,
                                      *Key );
    } else {
        i = KeInsertDeviceQueue( &DeviceObject->DeviceQueue,
                                 &Irp->Tail.Overlay.DeviceQueueEntry );
    }

    //
    // If the packet was not inserted into the queue, then this request is
    // now the current packet for this device.  Indicate so by storing its
    // address in the current IRP field, and begin processing the request.
    //

    if (!i) {

        DeviceObject->CurrentIrp = Irp;

        if (CancelFunction) {

            IoReleaseCancelSpinLock( cancelIrql );
        }

        //
        // Invoke the driver's start I/O routine to get the request going on the device.
        // The StartIo routine should handle the cancellation.
        //

        DeviceObject->DriverObject->DriverStartIo( DeviceObject, Irp );

    } else {

        //
        // The packet was successfully inserted into the device's work queue.
        // Make one last check to determine whether or not the packet has
        // already been marked cancelled.  If it has, then invoke the
        // driver's cancel routine now.  Note that because the cancel
        // spinlock is currently being held, an attempt to cancel the request
        // from another processor at this point will simply wait until this
        // routine is finished, and then get it cancelled.
        //

        if (CancelFunction) {
            if (Irp->Cancel) {
                Irp->CancelIrql = cancelIrql;
                Irp->CancelRoutine = (PDRIVER_CANCEL) NULL;
                CancelFunction( DeviceObject, Irp );
            } else {
                IoReleaseCancelSpinLock( cancelIrql );
            }
        }
    }

    //
    // Restore the IRQL back to its value upon entry to this function before
    // returning to the caller.
    //

    KeLowerIrql( oldIrql );
}


VOID
IoStartTimer(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine starts the timer associated with the specified device object.

Arguments:

    DeviceObject - Device object associated with the timer to be started.

Return Value:

    None.

--*/

{
    PIO_TIMER timer;
    KIRQL irql;

    //
    // Get the address of the timer.
    //

    timer = DeviceObject->Timer;

    //
    // If the driver is not being unloaded, then it is okay to start timers.
    //

    if (!(DeviceObject->DeviceObjectExtension->ExtensionFlags &
        (DOE_UNLOAD_PENDING | DOE_DELETE_PENDING | DOE_REMOVE_PENDING | DOE_REMOVE_PROCESSED))) {

        //
        // Likewise, check to see whether or not the timer is already
        // enabled.  If so, then simply exit.  Otherwise, enable the timer
        // by placing it into the I/O system timer queue.
        //

        ExAcquireFastLock( &IopTimerLock, &irql );
        if (!timer->TimerFlag) {
            timer->TimerFlag = TRUE;
            IopTimerCount++;
        }
        ExReleaseFastLock( &IopTimerLock, irql );
    }
}

VOID
IoStopTimer(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routines stops the timer associated with the specified device object
    from invoking being invoked.

Arguments:

    DeviceObject - Device object associated with the timer to be stopped.

Return Value:

    None.

--*/

{
    KIRQL irql;
    PIO_TIMER timer;

    //
    // Obtain the I/O system timer queue lock, and disable the specified
    // timer.
    //

    timer = DeviceObject->Timer;

    ExAcquireFastLock( &IopTimerLock, &irql );
    if (timer->TimerFlag) {
        timer->TimerFlag = FALSE;
        IopTimerCount--;
    }
    ExReleaseFastLock( &IopTimerLock, irql );
}

NTSTATUS
IoSynchronousPageWrite(
    IN PFILE_OBJECT FileObject,
    IN PMDL MemoryDescriptorList,
    IN PLARGE_INTEGER StartingOffset,
    IN PKEVENT Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )

/*++

Routine Description:

    This routine provides a special, fast interface for the Modified Page Writer
    (MPW) to write pages to the disk quickly and with very little overhead.  All
    of the special handling for this request is recognized by setting the
    IRP_PAGING_IO flag in the IRP flags word.

Arguments:

    FileObject - A pointer to a referenced file object describing which file
        the write should be performed on.

    MemoryDescriptorList - An MDL which describes the physical pages that the
        pages should be written to the disk.  All of the pages have been locked
        in memory.  The MDL also describes the length of the write operation.

    StartingOffset - Pointer to the offset in the file from which the write
        should take place.

    Event - A pointer to a kernel event structure to be used for synchronization
        purposes.  The event will be set to the Signlaged state once the pages
        have been written.

    IoStatusBlock - A pointer to the I/O status block in which the final status
        and information should be stored.

Return Value:

    The function value is the final status of the queue request to the I/O
    system subcomponents.


--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT deviceObject;

    //
    // Increment performance counters
    //

    if (CcIsFileCached(FileObject)) {
        CcDataFlushes += 1;
        CcDataPages += (MemoryDescriptorList->ByteCount + PAGE_SIZE - 1) >> PAGE_SHIFT;
    }

    //
    // Begin by getting a pointer to the device object that the file resides
    // on.
    //

    deviceObject = IoGetRelatedDeviceObject( FileObject );

    //
    // Allocate an I/O Request Packet (IRP) for this out-page operation.
    //

    irp = IoAllocateIrp( deviceObject->StackSize, FALSE );
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get a pointer to the first stack location in the packet.  This location
    // will be used to pass the function codes and parameters to the first
    // driver.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    //
    // Fill in the IRP according to this request.
    //

    irp->MdlAddress = MemoryDescriptorList;
    irp->Flags = IRP_PAGING_IO | IRP_NOCACHE | IRP_SYNCHRONOUS_PAGING_IO;

    irp->RequestorMode = KernelMode;
    irp->UserIosb = IoStatusBlock;
    irp->UserEvent = Event;
    irp->UserBuffer = (PVOID) ((PCHAR) MemoryDescriptorList->StartVa + MemoryDescriptorList->ByteOffset);
    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Fill in the normal write parameters.
    //

    irpSp->MajorFunction = IRP_MJ_WRITE;
    irpSp->Parameters.Write.Length = MemoryDescriptorList->ByteCount;
    irpSp->Parameters.Write.ByteOffset = *StartingOffset;
    irpSp->FileObject = FileObject;

    //
    // Queue the packet to the appropriate driver based on whether or not there
    // is a VPB associated with the device.
    //

    return IoCallDriver( deviceObject, irp );
}

PEPROCESS
IoThreadToProcess(
    IN PETHREAD Thread
    )

/*++

Routine Description:

    This routine returns a pointer to the process for the specified thread.

Arguments:

    Thread - Thread whose process is to be returned.

Return Value:

    A pointer to the thread's process.

Note:

    This function cannot be made a macro, since fields in the thread object
    move from release to release, so this must remain a full function.


--*/

{
    //
    // Simply return the thread's process.
    //

    return THREAD_TO_PROCESS( Thread );
}

VOID
IoUnregisterFileSystem(
    IN OUT PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine removes the device object for the file system from the active
    list of file systems in the system.

Arguments:

    DeviceObject - Pointer to device object for the file system.

Return Value:

    None.


--*/

{
    PNOTIFICATION_PACKET nPacket;
    PLIST_ENTRY entry;

    PAGED_CODE();

    //
    // Acquire the I/O database resource for a write operation.
    //

    (VOID)ExAcquireResourceExclusive( &IopDatabaseResource, TRUE );

    //
    // Remove the device object from whatever queue it happens to be in at the
    // moment.  There is no need to check here to determine if the device queue
    // is in a queue since it is assumed that the caller registered it as a
    // valid file system.
    //

    RemoveEntryList( &DeviceObject->Queue.ListEntry );

    //
    // Notify all of the registered drivers that this file system has been
    // unregistered as an active file system of some type.
    //

    entry = IopFsNotifyChangeQueueHead.Flink;
    while (entry != &IopFsNotifyChangeQueueHead) {
        nPacket = CONTAINING_RECORD( entry, NOTIFICATION_PACKET, ListEntry );
        entry = entry->Flink;
        nPacket->NotificationRoutine( DeviceObject, FALSE );
    }

    //
    // Release the I/O database resource.
    //

    ExReleaseResource( &IopDatabaseResource );

    //
    // Decrement the number of reasons that this driver cannot be unloaded.
    //

    ExInterlockedAddUlong( &DeviceObject->ReferenceCount,
                           0xffffffff,
                           &IopDatabaseLock );

}

VOID
IoUnregisterFsRegistrationChange(
    IN PDRIVER_OBJECT DriverObject,
    IN PDRIVER_FS_NOTIFICATION DriverNotificationRoutine
    )

/*++

Routine Description:

    This routine unregisters the specified driver's notification routine from
    begin invoked whenever a file system registers or unregisters itself as an
    active file system in the system.

Arguments:

    DriverObject - Pointer to the driver object for the driver.

    DriverNotificationRoutine - Address of routine to unregister.

Return Value:

    None.

--*/

{
    PNOTIFICATION_PACKET nPacket;
    PLIST_ENTRY entry;

    PAGED_CODE();

    //
    // Begin by acquiring the database resource exclusively.
    //

    ExAcquireResourceExclusive( &IopDatabaseResource, TRUE );

    //
    // Walk the list of registered notification routines and unregister the
    // specified routine.
    //

    for (entry = IopFsNotifyChangeQueueHead.Flink;
        entry != &IopFsNotifyChangeQueueHead;
        entry = entry->Flink) {
        nPacket = CONTAINING_RECORD( entry, NOTIFICATION_PACKET, ListEntry );
        if (nPacket->DriverObject == DriverObject &&
            nPacket->NotificationRoutine == DriverNotificationRoutine) {
            RemoveEntryList( entry );
            ExFreePool( nPacket );
            break;
        }
    }

    ExReleaseResource( &IopDatabaseResource );

    ObDereferenceObject( DriverObject );

}

VOID
IoUnregisterShutdownNotification(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine removes a registered driver from the shutdown notification
    queue.  Henceforth, the driver will not be notified when the system is
    being shutdown.

Arguments:

    DeviceObject - Pointer to the driver's device object.

Return Value:

    None.

--*/

{
    PLIST_ENTRY entry;
    PSHUTDOWN_PACKET shutdown;
    KIRQL irql;
    PVOID unlockHandle;

    PAGED_CODE();

    //
    // Lock this code into memory for the duration of this function's execution.
    //

    unlockHandle = MmLockPagableCodeSection( IoUnregisterShutdownNotification );

    //
    // Acquire the spinlock that protects the shutdown notification queue, and
    // walk the queue looking for the caller's entry.  Once found, remove it
    // from the queue.  It is an error to not find it, but it is ignored here.
    //

    ExAcquireSpinLock( &IopDatabaseLock, &irql );

    for (entry = IopNotifyShutdownQueueHead.Flink;
         entry != &IopNotifyShutdownQueueHead;
         entry = entry->Flink) {

        //
        // An entry has been located.  If it is the one that is being searched
        // for, simply remove it from the list and deallocate it.
        //

        shutdown = CONTAINING_RECORD( entry, SHUTDOWN_PACKET, ListEntry );
        if (shutdown->DeviceObject == DeviceObject) {
            RemoveEntryList( entry );
            entry = entry->Blink;
            ExFreePool( shutdown );
        }
    }

    for (entry = IopNotifyLastChanceShutdownQueueHead.Flink;
         entry != &IopNotifyLastChanceShutdownQueueHead;
         entry = entry->Flink) {

        //
        // An entry has been located.  If it is the one that is being searched
        // for, simply remove it from the list and deallocate it.
        //

        shutdown = CONTAINING_RECORD( entry, SHUTDOWN_PACKET, ListEntry );
        if (shutdown->DeviceObject == DeviceObject) {
            RemoveEntryList( entry );
            entry = entry->Blink;
            ExFreePool( shutdown );
        }
    }

    //
    // Release the spinlock.
    //

    ExReleaseSpinLock( &IopDatabaseLock, irql );

    MmUnlockPagableImageSection( unlockHandle );

    DeviceObject->Flags &= ~DO_SHUTDOWN_REGISTERED;

}

VOID
IoUpdateShareAccess(
    IN OUT PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess
    )

/*++

Routine Description:

    This routine updates the share access context for a file according to
    the desired access and share access by the current open requestor.  The
    IoCheckShareAccess routine must already have been invoked and succeeded
    in order to invoke this routine.  Note that when the former routine was
    invoked the Update parameter must have been FALSE.

Arguments:

    FileObject - Pointer to the file object of the current open request.

    ShareAccess - Pointer to the share access structure that describes how
        the file is currently being accessed.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    // Check to see whether or not the desired accesses need read, write,
    // or delete access to the file.
    //

    if (FileObject->ReadAccess ||
        FileObject->WriteAccess ||
        FileObject->DeleteAccess) {

        //
        // The open request requires read, write, or delete access so update
        // the share access context for the file.
        //

        ShareAccess->OpenCount++;

        ShareAccess->Readers += FileObject->ReadAccess;
        ShareAccess->Writers += FileObject->WriteAccess;
        ShareAccess->Deleters += FileObject->DeleteAccess;

        ShareAccess->SharedRead += FileObject->SharedRead;
        ShareAccess->SharedWrite += FileObject->SharedWrite;
        ShareAccess->SharedDelete += FileObject->SharedDelete;
    }
}


NTSTATUS
IoVerifyVolume(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN AllowRawMount
    )

/*++

Routine Description:

    This routine is invoked to check a mounted volume on the specified device
    when it appears as if the media may have changed since it was last
    accessed.  If the volume is not the same volume, and a new mount is not
    to be attempted, return the error.

    If the verify fails, this routine is used to perform a new mount operation
    on the device.  In this case, a "clean" VPB is allocated and a new mount
    operation is attempted.  If no mount operation succeeds, then again the
    error handling described above occurs.

Arguments:

    DeviceObject - Pointer to device object on which the volume is to be
        mounted.

    AllowRawMount - Indicates that this verify is on behalf of a DASD open
        request, thus we want to allow a raw mount if the verify fails.

Return Value:

    The function value is a successful status code if a volume was successfully
    mounted on the device.  Otherwise, an error code is returned.


--*/

{
    NTSTATUS status;
    KEVENT event;
    PIRP irp;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION irpSp;
    BOOLEAN verifySkipped = FALSE;
    PDEVICE_OBJECT fsDeviceObject;

    PAGED_CODE();

    //
    //  Acquire the DeviceObject lock.  Nothing in this routine can raise
    //  so no try {} finally {} is required.
    //

    status = KeWaitForSingleObject( &DeviceObject->DeviceLock,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    (PLARGE_INTEGER) NULL );

    ASSERT( status == STATUS_SUCCESS );

    //
    // If this volume is not mounted by anyone, skip the verify operation,
    // but do the mount.
    //

    if (!(DeviceObject->Vpb->Flags & VPB_MOUNTED)) {

        verifySkipped = TRUE;

        status = STATUS_SUCCESS;

    } else {

        //
        // This volume needs to be verified.  Initialize the event to be
        // used while waiting for the operation to complete.
        //

        KeInitializeEvent( &event, NotificationEvent, FALSE );
        status = STATUS_UNSUCCESSFUL;

        //
        // Allocate and initialize an IRP for this verify operation.  Notice
        // that the flags for this operation appear the same as a page read
        // operation.  This is because the completion code for both of the
        // operations is exactly the same logic.
        //

        fsDeviceObject = DeviceObject->Vpb->DeviceObject;
        while (fsDeviceObject->AttachedDevice) {
            fsDeviceObject = fsDeviceObject->AttachedDevice;
        }
        irp = IoAllocateIrp( fsDeviceObject->StackSize, FALSE );
        if (!irp) {

            KeSetEvent( &DeviceObject->DeviceLock, 0, FALSE );
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        irp->Flags = IRP_MOUNT_COMPLETION | IRP_SYNCHRONOUS_PAGING_IO;
        irp->RequestorMode = KernelMode;
        irp->UserEvent = &event;
        irp->UserIosb = &ioStatus;
        irp->Tail.Overlay.Thread = PsGetCurrentThread();
        irpSp = IoGetNextIrpStackLocation( irp );
        irpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
        irpSp->MinorFunction = IRP_MN_VERIFY_VOLUME;
        irpSp->Flags = AllowRawMount ? SL_ALLOW_RAW_MOUNT : 0;
        irpSp->Parameters.VerifyVolume.Vpb = DeviceObject->Vpb;
        irpSp->Parameters.VerifyVolume.DeviceObject = DeviceObject->Vpb->DeviceObject;

        status = IoCallDriver( fsDeviceObject, irp );

        //          IopLoadFileSystemDriver
        // Wait for the I/O operation to complete.
        //

        if (status == STATUS_PENDING) {
            (VOID) KeWaitForSingleObject( &event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          (PLARGE_INTEGER) NULL );
            status = ioStatus.Status;
        }
    }

    //
    // If the verify operation was skipped or unsuccessful perform a mount
    // operation.
    //

    if ((status == STATUS_WRONG_VOLUME) || verifySkipped) {

        //
        // A mount operation is to be attempted.  Allocate a new VPB
        // for this device and attempt to mount it.  Note that at this
        // point, allowing allocation of the VPB to fail is simply too
        // difficult to deal with, so if one cannot be allocated normally,
        // allocate one specifying that it must succeed.
        //

        IopCreateVpb (DeviceObject);
        PoVolumeDevice (DeviceObject);

        //
        // Now mount the volume.
        //

        if (!NT_SUCCESS( IopMountVolume( DeviceObject, AllowRawMount, TRUE, FALSE ) )) {
            DeviceObject->Flags &= ~DO_VERIFY_VOLUME;
        }
    }

    //
    //  Release the device lock.
    //

    KeSetEvent( &DeviceObject->DeviceLock, 0, FALSE );

    //
    // Return the status from the verify operation as the final status of
    // this function.
    //

    return status;
}

VOID
IoWriteErrorLogEntry(
    IN OUT PVOID ElEntry
    )

/*++

Routine Description:

    This routine places the error log entry specified by the input argument
    onto the queue of buffers to be written to the error log process's port.
    The error log thread will then actually send it.

Arguments:

    ElEntry Pointer to the error log entry.

Return Value:

    None.

--*/

{
    PERROR_LOG_ENTRY entry;
    KIRQL oldIrql;

    //
    // Get the address of the error log entry header, acquire the spin lock,
    // insert the entry onto the queue, if there are no pending requests
    // then queue a worker thread request and release the spin lock.
    //

    entry = ((PERROR_LOG_ENTRY) ElEntry) - 1;

    if (IopErrorLogDisabledThisBoot) {
        //
        // Do nothing, drop the reference.
        //

        if (entry->DeviceObject != NULL) {
            //
            // IopErrorLogThread tests for NULL before derefing.
            // So do the same here.
            //
            ObDereferenceObject (entry->DeviceObject);
        }
        if (entry->DriverObject != NULL) {
            ObDereferenceObject (entry->DriverObject);
        }
        ExFreePool (entry);
        return;

    }

    //
    // Set the time that the entry was logged.
    //

    KeQuerySystemTime( (PVOID) &entry->TimeStamp );

    ExAcquireSpinLock( &IopErrorLogLock, &oldIrql );

    //
    // Queue the request to the error log queue.
    //

    InsertTailList( &IopErrorLogListHead, &entry->ListEntry );

    //
    // If there is no pending work, then queue a request to a worker thread.
    //

    if (!IopErrorLogPortPending) {

        IopErrorLogPortPending = TRUE;

        ExInitializeWorkItem( &IopErrorLogWorkItem, IopErrorLogThread, NULL );
        ExQueueWorkItem( &IopErrorLogWorkItem, DelayedWorkQueue );

    }

    ExReleaseSpinLock(&IopErrorLogLock, oldIrql);
}

NTSTATUS
IoGetBootDiskInformation(
    IN OUT PBOOTDISK_INFORMATION BootDiskInformation,
    IN ULONG Size
    )

/*++

Routine Description:

    This routine provides the caller with the signature and offset of
    the boot disk and system disk. This information is obtained from the
    loader block. The callers have to be boot drivers which have registered
    for a callback once all disk devices have been started
Arguments:

    BootDiskInformation - Supplies a pointer to the structure allocated by the
    caller for requested information.
    Size - Size of the BootDiskInformation structure.

Return Value:

    STATUS_SUCCESS - successful.
    STATUS_TOO_LATE - indicates that the Loader Block has already been freed
    STATUS_INVALID_PARAMETER - size allocated for boot disk information
    is insufficient.

--*/

{
    PLOADER_PARAMETER_BLOCK LoaderBlock = NULL;
    STRING arcBootDeviceString;
    UCHAR deviceNameBuffer[128];
    STRING deviceNameString;
    UNICODE_STRING deviceNameUnicodeString;
    PDEVICE_OBJECT deviceObject;
    UCHAR arcNameBuffer[128];
    STRING arcNameString;
    UNICODE_STRING arcNameUnicodeString;
    PFILE_OBJECT fileObject;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    DISK_GEOMETRY diskGeometry;
    PDRIVE_LAYOUT_INFORMATION driveLayout;
    PLIST_ENTRY listEntry;
    PARC_DISK_SIGNATURE diskBlock;
    ULONG diskNumber;
    ULONG partitionNumber;
    PCHAR arcName;
    PULONG buffer;
    PIRP irp;
    KEVENT event;
    LARGE_INTEGER offset;
    ULONG checkSum;
    ULONG i;
    BOOLEAN singleBiosDiskFound;
    BOOLEAN bootDiskFound = FALSE;
    BOOLEAN systemDiskFound = FALSE;
    PARC_DISK_INFORMATION arcInformation;
    ULONG totalDriverDisksFound = IoGetConfigurationInformation()->DiskCount;
    STRING arcSystemDeviceString;
    STRING osLoaderPathString;
    UNICODE_STRING osLoaderPathUnicodeString;
    PARTITION_INFORMATION PartitionInfo;

    if (IopLoaderBlock == NULL) {
        return STATUS_TOO_LATE;
    }

    if (Size < sizeof(BOOTDISK_INFORMATION)) {
        return STATUS_INVALID_PARAMETER;
    }

    LoaderBlock = (PLOADER_PARAMETER_BLOCK)IopLoaderBlock;
    arcInformation = LoaderBlock->ArcDiskInformation;

    //
    // If a single bios disk was found if there is only a
    // single entry on the disk signature list.
    //
    singleBiosDiskFound = (arcInformation->DiskSignatures.Flink->Flink ==
                           &arcInformation->DiskSignatures) ? (TRUE) : (FALSE);

    //
    // Get ARC boot device name from loader block.
    //

    RtlInitAnsiString( &arcBootDeviceString,
                       LoaderBlock->ArcBootDeviceName );
    //
    // Get ARC system device name from loader block.
    //

    RtlInitAnsiString( &arcSystemDeviceString,
                       LoaderBlock->ArcHalDeviceName );
    //
    // For each disk, get its drive layout and check to see if the
    // signature is among the list of signatures in the loader block.
    // If yes, check to see if the disk contains the boot or system
    // partitions. If yes, fill up the requested structure.
    //

    for (diskNumber = 0;
         diskNumber < totalDriverDisksFound;
         diskNumber++) {

        //
        // Construct the NT name for a disk and obtain a reference.
        //

        sprintf( deviceNameBuffer,
                 "\\Device\\Harddisk%d\\Partition0",
                 diskNumber );
        RtlInitAnsiString( &deviceNameString, deviceNameBuffer );
        status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                               &deviceNameString,
                                               TRUE );
        if (!NT_SUCCESS( status )) {
            continue;
        }

        status = IoGetDeviceObjectPointer( &deviceNameUnicodeString,
                                           FILE_READ_ATTRIBUTES,
                                           &fileObject,
                                           &deviceObject );
        RtlFreeUnicodeString( &deviceNameUnicodeString );

        if (!NT_SUCCESS( status )) {
            continue;
        }

        //
        // Create IRP for get drive geometry device control.
        //

        irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                             deviceObject,
                                             NULL,
                                             0,
                                             &diskGeometry,
                                             sizeof(DISK_GEOMETRY),
                                             FALSE,
                                             &event,
                                             &ioStatusBlock );
        if (!irp) {
            ObDereferenceObject( fileObject );
            continue;
        }

        KeInitializeEvent( &event,
                           NotificationEvent,
                           FALSE );
        status = IoCallDriver( deviceObject,
                               irp );

        if (status == STATUS_PENDING) {
            KeWaitForSingleObject( &event,
                                   Suspended,
                                   KernelMode,
                                   FALSE,
                                   NULL );
            status = ioStatusBlock.Status;
        }

        if (!NT_SUCCESS( status )) {
            ObDereferenceObject( fileObject );
            continue;
        }

        //
        // Get partition information for this disk.
        //

        status = IoReadPartitionTable( deviceObject,
                                       diskGeometry.BytesPerSector,
                                       TRUE,
                                       &driveLayout );

        ObDereferenceObject( fileObject );

        if (!NT_SUCCESS( status )) {
            continue;
        }

        //
        // Make sure sector size is at least 512 bytes.
        //

        if (diskGeometry.BytesPerSector < 512) {
            diskGeometry.BytesPerSector = 512;
        }

        //
        // For each ARC disk information record in the loader block
        // match the disk signature and checksum to determine its ARC
        // name and construct the NT ARC names symbolic links.
        //

        for (listEntry = arcInformation->DiskSignatures.Flink;
             listEntry != &arcInformation->DiskSignatures;
             listEntry = listEntry->Flink) {

            //
            // Get next record and compare disk signatures.
            //

            diskBlock = CONTAINING_RECORD( listEntry,
                                           ARC_DISK_SIGNATURE,
                                           ListEntry );

            //
            // Compare disk signatures.
            //
            // Or if there is only a single disk drive from
            // both the bios and driver viewpoints then
            // assign an arc name to that drive.
            //

            if ((singleBiosDiskFound && (totalDriverDisksFound == 1)) ||
                (diskBlock->Signature == driveLayout->Signature &&
                 diskBlock->ValidPartitionTable)) {

                //
                // Create unicode device name for physical disk.
                //

                sprintf( deviceNameBuffer,
                         "\\Device\\Harddisk%d\\Partition0",
                         diskNumber );
                RtlInitAnsiString( &deviceNameString, deviceNameBuffer );
                status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                                       &deviceNameString,
                                                       TRUE );
                if (!NT_SUCCESS( status )) {
                    continue;
                }

                //
                // Create unicode ARC name for this partition.
                //

                arcName = diskBlock->ArcName;
                sprintf( arcNameBuffer,
                         "\\ArcName\\%s",
                         arcName );
                RtlInitAnsiString( &arcNameString, arcNameBuffer );
                status = RtlAnsiStringToUnicodeString( &arcNameUnicodeString,
                                                       &arcNameString,
                                                       TRUE );
                if (!NT_SUCCESS( status )) {
                    continue;
                }

                //
                // Create an ARC name for every partition on this disk.
                //

                for (partitionNumber = 0;
                     partitionNumber < driveLayout->PartitionCount;
                     partitionNumber++) {

                    //
                    // Create unicode NT device name.
                    //

                    sprintf( deviceNameBuffer,
                             "\\Device\\Harddisk%d\\Partition%d",
                             diskNumber,
                             partitionNumber+1 );
                    RtlInitAnsiString( &deviceNameString, deviceNameBuffer );
                    status = RtlAnsiStringToUnicodeString(
                                                           &deviceNameUnicodeString,
                                                           &deviceNameString,
                                                           TRUE );
                    if (!NT_SUCCESS( status )) {
                        continue;
                    }

                    //
                    // Create unicode ARC name for this partition and
                    // check to see if this is the boot disk.
                    //

                    sprintf( arcNameBuffer,
                             "%spartition(%d)",
                             arcName,
                             partitionNumber+1 );
                    RtlInitAnsiString( &arcNameString, arcNameBuffer );
                    if (RtlEqualString( &arcNameString,
                                        &arcBootDeviceString,
                                        TRUE )) {
                        BootDiskInformation->BootDeviceSignature =
                                                      driveLayout->Signature;
                        //
                        // Get Partition Information for the offset of the
                        // partition within the disk
                        //
                        status = IoGetDeviceObjectPointer(
                                           &deviceNameUnicodeString,
                                           FILE_READ_ATTRIBUTES,
                                           &fileObject,
                                           &deviceObject );
                        RtlFreeUnicodeString( &deviceNameUnicodeString );

                        if (!NT_SUCCESS( status )) {
                            continue;
                        }

                        //
                        // Create IRP for get drive geometry device control.
                        //

                        irp = IoBuildDeviceIoControlRequest(
                                             IOCTL_DISK_GET_PARTITION_INFO,
                                             deviceObject,
                                             NULL,
                                             0,
                                             &PartitionInfo,
                                             sizeof(PARTITION_INFORMATION),
                                             FALSE,
                                             &event,
                                             &ioStatusBlock );
                        if (!irp) {
                            ObDereferenceObject( fileObject );
                            continue;
                        }

                        KeInitializeEvent( &event,
                                           NotificationEvent,
                                           FALSE );
                        status = IoCallDriver( deviceObject,
                                               irp );

                        if (status == STATUS_PENDING) {
                            KeWaitForSingleObject( &event,
                                                   Suspended,
                                                   KernelMode,
                                                   FALSE,
                                                   NULL );
                            status = ioStatusBlock.Status;
                        }

                        if (!NT_SUCCESS( status )) {
                            ObDereferenceObject( fileObject );
                            continue;
                        }
                        BootDiskInformation->BootPartitionOffset =
                                        PartitionInfo.StartingOffset.QuadPart;
                        bootDiskFound = TRUE;
                    }

                    //
                    // See if this is the system partition.
                    //
                    if (RtlEqualString( &arcNameString,
                                        &arcSystemDeviceString,
                                        TRUE )) {
                        BootDiskInformation->SystemDeviceSignature =
                                                      driveLayout->Signature;
                        //
                        // Get Partition Information for the offset of the
                        // partition within the disk
                        //
                        status = IoGetDeviceObjectPointer(
                                           &deviceNameUnicodeString,
                                           FILE_READ_ATTRIBUTES,
                                           &fileObject,
                                           &deviceObject );
                        RtlFreeUnicodeString( &deviceNameUnicodeString );

                        if (!NT_SUCCESS( status )) {
                            continue;
                        }

                        //
                        // Create IRP for get drive geometry device control.
                        //

                        irp = IoBuildDeviceIoControlRequest(
                                             IOCTL_DISK_GET_PARTITION_INFO,
                                             deviceObject,
                                             NULL,
                                             0,
                                             &PartitionInfo,
                                             sizeof(PARTITION_INFORMATION),
                                             FALSE,
                                             &event,
                                             &ioStatusBlock );
                        if (!irp) {
                            ObDereferenceObject( fileObject );
                            continue;
                        }

                        KeInitializeEvent( &event,
                                           NotificationEvent,
                                           FALSE );
                        status = IoCallDriver( deviceObject,
                                               irp );

                        if (status == STATUS_PENDING) {
                            KeWaitForSingleObject( &event,
                                                   Suspended,
                                                   KernelMode,
                                                   FALSE,
                                                   NULL );
                            status = ioStatusBlock.Status;
                        }

                        if (!NT_SUCCESS( status )) {
                            ObDereferenceObject( fileObject );
                            continue;
                        }
                        BootDiskInformation->SystemPartitionOffset =
                                        PartitionInfo.StartingOffset.QuadPart;
                        systemDiskFound = TRUE;
                    }
                }
            }
        }
        ExFreePool( driveLayout );
    }
    return STATUS_SUCCESS;
}

//
// Thunks to support standard call callers
//

#ifdef IoCallDriver
#undef IoCallDriver
#endif

NTSTATUS
IoCallDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    return IofCallDriver (DeviceObject, Irp);
}



#ifdef IoCompleteRequest
#undef IoCompleteRequest
#endif

VOID
IoCompleteRequest(
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    )
{
    IofCompleteRequest (Irp, PriorityBoost);
}


PSECURITY_DESCRIPTOR
IopCreateDefaultDeviceSecurityDescriptor(
    IN DEVICE_TYPE DeviceType,
    IN ULONG DeviceCharacteristics,
    IN BOOLEAN DeviceHasName,
    IN PUCHAR Buffer,
    OUT PACL *AllocatedAcl,
    OUT PSECURITY_INFORMATION SecurityInformation OPTIONAL
    )
{
    PSECURITY_DESCRIPTOR descriptor = (PSECURITY_DESCRIPTOR) Buffer;

    NTSTATUS status;

    if(ARGUMENT_PRESENT(SecurityInformation)) {
        (*SecurityInformation) = 0;
    }

    *AllocatedAcl = NULL;

    switch ( DeviceType ) {

        case FILE_DEVICE_DISK_FILE_SYSTEM:
        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
        case FILE_DEVICE_FILE_SYSTEM:
        case FILE_DEVICE_TAPE_FILE_SYSTEM: {

            //
            // Use the standard public default protection for these types of devices.
            //

            RtlCreateSecurityDescriptor(descriptor,
                                        SECURITY_DESCRIPTOR_REVISION );

            RtlSetDaclSecurityDescriptor(descriptor,
                                         TRUE,
                                         SePublicDefaultUnrestrictedDacl,
                                         FALSE );

            if(ARGUMENT_PRESENT(SecurityInformation)) {
                (*SecurityInformation) |= DACL_SECURITY_INFORMATION;
            }

            break;
        }

        case FILE_DEVICE_CD_ROM:
        case FILE_DEVICE_MASS_STORAGE:
        case FILE_DEVICE_DISK:
        case FILE_DEVICE_VIRTUAL_DISK:
        case FILE_DEVICE_NETWORK_FILE_SYSTEM:
        case FILE_DEVICE_DFS_FILE_SYSTEM:
        case FILE_DEVICE_NETWORK: {

            if ((DeviceHasName) &&
                ((DeviceCharacteristics & FILE_FLOPPY_DISKETTE) != 0)) {

                status = RtlCreateSecurityDescriptor(
                            descriptor,
                            SECURITY_DESCRIPTOR_REVISION );

                ASSERT( NT_SUCCESS( status ) );

                status = RtlSetDaclSecurityDescriptor(
                            descriptor,
                            TRUE,
                            SePublicOpenUnrestrictedDacl,
                            FALSE );

                ASSERT( NT_SUCCESS( status ) );

                if(ARGUMENT_PRESENT(SecurityInformation)) {
                    (*SecurityInformation) |= DACL_SECURITY_INFORMATION;
                }

            } else {

                UCHAR i;
                PACL acl;
                BOOLEAN aceFound;
                BOOLEAN aceFoundForCDROM;
                PACCESS_ALLOWED_ACE ace;

                //
                // Protect the device so that an administrator can run chkdsk
                // on it. This is done by making a copy of the default public
                // ACL and changing the accesses granted to the administrators
                // alias.
                //
                // The logic here is:
                //
                //      - Copy the public default dacl into another buffer
                //
                //      - Find the ACE granting ADMINISTRATORS access
                //
                //      - Change the granted access mask of that ACE to give
                //        administrators write access.
                //
                //

                acl = ExAllocatePoolWithTag(
                        PagedPool,
                        SePublicDefaultUnrestrictedDacl->AclSize,
                        'eSoI' );

                if (!acl) {
                    return NULL;
                }

                RtlCopyMemory( acl,
                               SePublicDefaultUnrestrictedDacl,
                               SePublicDefaultUnrestrictedDacl->AclSize );

                //
                // Find the Administrators ACE
                //

                aceFound = FALSE;
                aceFoundForCDROM = FALSE;

                for ( i = 0, status = RtlGetAce(acl, 0, &ace);
                      NT_SUCCESS(status);
                      i++, status = RtlGetAce(acl, i, &ace)) {

                    PSID sid;

                    sid = &(ace->SidStart);
                    if (RtlEqualSid( SeAliasAdminsSid, sid )) {
                        PACCESS_MASK mask;

                        ace->Mask |= ( GENERIC_READ |
                                       GENERIC_WRITE |
                                       GENERIC_EXECUTE );

                        aceFound = TRUE;
                    }

                    if (DeviceType == FILE_DEVICE_CD_ROM) {

                         if (RtlEqualSid( SeWorldSid, sid )) {
                             ace->Mask |= GENERIC_READ;
                             aceFoundForCDROM = TRUE;
                         }
                     }
                }

                //
                // If the ACE wasn't found, then the public default ACL has been
                // changed.  For this case, this code needs to be updated to match
                // the new public default DACL.
                //

                ASSERT(aceFound == TRUE);

                if (DeviceType == FILE_DEVICE_CD_ROM) {
                    ASSERT(aceFoundForCDROM == TRUE);
                }

                //
                // Finally, build a full security descriptor from the above DACL.
                //

                RtlCreateSecurityDescriptor( descriptor,
                                             SECURITY_DESCRIPTOR_REVISION );

                RtlSetDaclSecurityDescriptor( descriptor,
                                              TRUE,
                                              acl,
                                              FALSE );

                if(ARGUMENT_PRESENT(SecurityInformation)) {
                    (*SecurityInformation) |= DACL_SECURITY_INFORMATION;
                }

                *AllocatedAcl = acl;
            }

            break;
        }

        default: {

            status = RtlCreateSecurityDescriptor( descriptor,
                                                  SECURITY_DESCRIPTOR_REVISION );
            ASSERT( NT_SUCCESS( status ) );

            status = RtlSetDaclSecurityDescriptor( descriptor,
                                                   TRUE,
                                                   SePublicOpenUnrestrictedDacl,
                                                   FALSE );

            if(ARGUMENT_PRESENT(SecurityInformation)) {
                (*SecurityInformation) |= DACL_SECURITY_INFORMATION;
            }

            break;
        }
    }

    return descriptor;
}


NTSTATUS
IoGetRequestorSessionId(
    IN PIRP Irp,
    OUT PULONG pSessionId
    )

/*++

Routine Description:

    This routine returns the session ID for process that originally
    requested the specified I/O operation.

Arguments:

    Irp - Pointer to the I/O Request Packet.

    pSessionId - Pointer to the session Id which is set upon successful return.

Return Value:

    Returns STATUS_SUCCESS if the session ID was available, otherwise
    STATUS_UNSUCCESSFUL.

--*/

{
    PEPROCESS Process;

    //
    // Get the address of the process that requested the I/O operation.
    //

    if (Irp->Tail.Overlay.Thread) {
        Process = THREAD_TO_PROCESS( Irp->Tail.Overlay.Thread );
        *pSessionId = Process->SessionId;
        return(STATUS_SUCCESS);
    }

    *pSessionId = (ULONG) -1;
    return(STATUS_UNSUCCESSFUL);
}


VOID
IopUpdateOtherOperationCount(
    VOID
    )
/*++

Routine Description:

    This routine is invoked to update the operation count for the current
    process to indicate that an I/O service other than a read or write
    has been invoked.

    There is an implicit assumption that this call is always made in the context
    of the issuing thread.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (IoCountOperations == TRUE) {
        IoOtherOperationCount += 1;
        ExInterlockedAddLargeStatistic( &THREAD_TO_PROCESS(PsGetCurrentThread())->OtherOperationCount, 1);
    }
}


VOID
IopUpdateReadOperationCount(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to update the read operation count for the
    current process to indicate that the NtReadFile system service has
    been invoked.

    There is an implicit assumption that this call is always made in the context
    of the issuing thread.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (IoCountOperations == TRUE) {
        IoReadOperationCount += 1;
        ExInterlockedAddLargeStatistic( &THREAD_TO_PROCESS(PsGetCurrentThread())->ReadOperationCount, 1);
    }
}


VOID
IopUpdateWriteOperationCount(
    VOID
    )
/*++

Routine Description:

    This routine is invoked to update the write operation count for the
    current process to indicate that the NtWriteFile service other has
    been invoked.

    There is an implicit assumption that this call is always made in the context
    of the issuing thread.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (IoCountOperations == TRUE) {
        IoWriteOperationCount += 1;
        ExInterlockedAddLargeStatistic( &THREAD_TO_PROCESS(PsGetCurrentThread())->WriteOperationCount, 1);
    }
}

VOID
IopUpdateOtherTransferCount(
    IN ULONG TransferCount
    )
/*++

Routine Description:

    This routine is invoked to update the transfer count for the current
    process for an operation other than a read or write system service.

    There is an implicit assumption that this call is always made in the context
    of the issuing thread. Also note that overflow is folded into the thread's
    process.

Arguments:

    TransferCount - The count of the number of bytes transferred.

Return Value:

    None.

--*/
{
    if (IoCountOperations == TRUE) {
        ExInterlockedAddLargeStatistic( &IoOtherTransferCount, TransferCount );
        ExInterlockedAddLargeStatistic( &THREAD_TO_PROCESS(PsGetCurrentThread())->OtherTransferCount, TransferCount);
    }
}


VOID
IopUpdateReadTransferCount(
    IN ULONG TransferCount
    )
/*++

Routine Description:

    This routine is invoked to update the read transfer count for the
    current process.

    There is an implicit assumption that this call is always made in the context
    of the issuing thread. Also note that overflow is folded into the thread's
    process.

Arguments:

    TransferCount - The count of the number of bytes transferred.

Return Value:

    None.

--*/
{
    if (IoCountOperations == TRUE) {
        ExInterlockedAddLargeStatistic( &IoReadTransferCount, TransferCount );
        ExInterlockedAddLargeStatistic( &THREAD_TO_PROCESS(PsGetCurrentThread())->ReadTransferCount, TransferCount);
    }
}

VOID
IopUpdateWriteTransferCount(
    IN ULONG TransferCount
    )
/*++

Routine Description:

    This routine is invoked to update the write transfer count for the
    current process.

    There is an implicit assumption that this call is always made in the context
    of the issuing thread. Also note that overflow is folded into the thread's
    process.

Arguments:

    TransferCount - The count of the number of bytes transferred.

Return Value:

    None.

--*/
{
    if (IoCountOperations == TRUE) {
        ExInterlockedAddLargeStatistic( &IoWriteTransferCount, TransferCount );
        ExInterlockedAddLargeStatistic( &THREAD_TO_PROCESS(PsGetCurrentThread())->WriteTransferCount, TransferCount);
    }
}

VOID
IoCancelFileOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject
    )
/*++

Routine Description:

    This routine is invoked by a filter driver to send a close to the
    next filesystem driver below. It's needed as part of the file open
    process. The filter driver forwards the open to the FSD and the FSD
    returns success. The filter driver then examines some stuff and
    decides that the open has to be failed. In this case it has to send
    a close to the FSD.

    We can safely assume a thread context because it has to be called only
    in the context of file open. If the file object already has a handle
    then the owner of the handle can then simply close the handle to the
    file object and we will close the file.

    This code is extracted from IopCloseFile and IopDeleteFile. So it is
    duplication of code but it prevents duplication elsewhere in other FSDs.

Arguments:

    FileObject - Points to the file that needs to be closed.

    DeviceObject - Points to the device object of the filesystem driver below
        the filter driver.

Return Value:

    None

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PFAST_IO_DISPATCH fastIoDispatch;
    NTSTATUS status;
    KEVENT event;
    KIRQL irql;
    IO_STATUS_BLOCK ioStatusBlock;
    PVPB vpb;
    BOOLEAN referenceCountDecremented;


    //
    // Cannot call this function if a handle has already been created
    // for this file.
    //
    ASSERT(!(FileObject->Flags & FO_HANDLE_CREATED));

    //
    // Initialize the local event that will be used to synchronize access
    // to the driver completing this I/O operation.
    //

    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Reset the event in the file object.
    //

    KeClearEvent( &FileObject->Event );

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this
    // operation.
    //

    irp = IopAllocateIrpMustSucceed( DeviceObject->StackSize );
    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = KernelMode;

    //
    // Fill in the service independent parameters in the IRP.
    //

    irp->UserEvent = &event;
    irp->UserIosb = &irp->IoStatus;
    irp->Overlay.AsynchronousParameters.UserApcRoutine = (PIO_APC_ROUTINE) NULL;
    irp->Flags = IRP_SYNCHRONOUS_API | IRP_CLOSE_OPERATION;

    //
    // Get a pointer to the stack location for the first driver.  This will
    // be used to pass the original function codes and parameters.  No
    // function-specific parameters are required for this operation.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_CLEANUP;
    irpSp->FileObject = FileObject;

    //
    // Insert the packet at the head of the IRP list for the thread.
    //

    IopQueueThreadIrp( irp );

    //
    // Invoke the driver at its appropriate dispatch entry with the IRP.
    //

    status = IoCallDriver( DeviceObject, irp );

    //
    // If no error was incurred, wait for the I/O operation to complete.
    //

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      UserRequest,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
    }

    //
    // The following code tears down the IRP by hand since it may not
    // be possible for it to be completed (either because this code was
    // invoked as APC_LEVEL in the first place - or because the reference
    // count on the object cannot be incremented due to this routine
    // being invoked by the delete file procedure below).  Cleanup IRPs
    // therefore use close semantics (the close operation flag is set
    // in the IRP) so that the I/O complete request routine itself sets
    // the event to the Signaled state.
    //

    KeRaiseIrql( APC_LEVEL, &irql );
    IopDequeueThreadIrp( irp );
    KeLowerIrql( irql );

    //
    // Reuse the IRP for the next operation.
    //

    IoReuseIrp( irp , STATUS_SUCCESS);

    //
    // Reset the event in the file object.
    //

    KeClearEvent( &FileObject->Event );
    KeClearEvent(&event);

    //
    // Get a pointer to the stack location for the first driver.  This is
    // where the function codes and parameters are placed.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    //
    // Fill in the IRP, indicating that this file object is being deleted.
    //

    irpSp->MajorFunction = IRP_MJ_CLOSE;
    irpSp->FileObject = FileObject;
    irp->UserIosb = &ioStatusBlock;
    irp->UserEvent = &event;
    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->AssociatedIrp.SystemBuffer = (PVOID) NULL;
    irp->Flags = IRP_CLOSE_OPERATION | IRP_SYNCHRONOUS_API;

    //
    // Place this packet in the thread's I/O pending queue.
    //

    IopQueueThreadIrp( irp );

    //
    // Decrement the reference count on the VPB, if necessary.  We
    // have to do this BEFORE handing the Irp to the file system
    // because of a trick the file systems play with close, and
    // believe me, you really don't want to know what it is.
    //
    // Since there is not a error path here (close cannot fail),
    // and the file system is the only ome who can actually synchronize
    // with the actual completion of close processing, the file system
    // is the one responsible for Vpb deletion.
    //

    vpb = FileObject->Vpb;

    if (vpb && !(FileObject->Flags & FO_DIRECT_DEVICE_OPEN)) {
        ExInterlockedAddUlong( &vpb->ReferenceCount,
                               0xffffffff,
                               &IopVpbSpinLock );
        FileObject->Flags |= FO_FILE_OPEN_CANCELLED;
    }

    //
    // Give the device driver the packet.  If this request does not work,
    // there is nothing that can be done about it.  This is unfortunate
    // because the driver may have had problems that it was about to
    // report about other operations (e.g., write behind failures, etc.)
    // that it can no longer report.  The reason is that this routine
    // is really initially invoked by NtClose, which has already closed
    // the caller's handle, and that's what the return status from close
    // indicates:  the handle has successfully been closed.
    //

    status = IoCallDriver( DeviceObject, irp );

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
    }

    //
    // Perform any completion operations that need to be performed on
    // the IRP that was used for this request.  This is done here as
    // as opposed to in normal completion code because there is a race
    // condition between when this routine executes if it was invoked
    // from a special kernel APC (e.g., some IRP was just completed and
    // dereferenced this file object for the last time), and when the
    // special kernel APC because of this packet's completion executing.
    //
    // This problem is solved by not having to queue a special kernel
    // APC routine for completion of this packet.  Rather, it is treated
    // much like a synchronous paging I/O operation, except that the
    // packet is not even freed during I/O completion.  This is because
    // the packet is still in this thread's queue, and there is no way
    // to get it out except at APC_LEVEL.  Unfortunately, the part of
    // I/O completion that needs to dequeue the packet is running at
    // DISPATCH_LEVEL.
    //
    // Hence, the packet must be removed from the queue (synchronized,
    // of course), and then it must be freed.
    //

    KeRaiseIrql( APC_LEVEL, &irql );
    IopDequeueThreadIrp( irp );
    KeLowerIrql( irql );

    IoFreeIrp( irp );

}

VOID
IoRetryIrpCompletions(
    VOID
    )
/*++

Routine Description:

    This routine is called from Mm when a page fault has completed. It's
    called on the special occasion when a thread faults a page and then when
    it's waiting for the inpage to complete, an IopCompleteRequest APC fires
    and we fault the same page again (say if the user buffer falls on the
    same page).  Note the fault during the APC may be referencing the same or
    a different user virtual address but this is irrelevant - the problem lies
    in the fact that both virtual address references are to the same physical
    page and thus result in a collided fault in the Mm.

    Mm detects this case (to avoid deadlock) and returns STATUS_FAULT_COLLISION
    and the I/O manager bails out the APC after marking the Irp with the flag
    IRP_RETRY_IO_COMPLETION. Later on when Mm has decided the fault has
    progressed far enough to avoid deadlock, it calls back into this routine 
    which calls IopCompleteRequest again.  The code in IopCompleteRequest is
    written in a reentrant way so that the retry knows the completion is only
    partially processed so far. We can fault in two places in IopCompleteRequest
    and in both cases if we call IopCompleteRequest again they will now work.

    This call must be called in the context of the thread that is faulting.
    This function should be called at APC_LEVEL.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PLIST_ENTRY header;
    PLIST_ENTRY entry;
    KIRQL irql;
    PETHREAD thread;
    PIRP irp;
    PVOID saveAuxiliaryPointer = NULL;
    PFILE_OBJECT fileObject;


    thread = PsGetCurrentThread();

    ASSERT(KeGetCurrentIrql() == APC_LEVEL);

    //
    // Raise the IRQL so that the IrpList cannot be modified by a completion
    // APC.
    //

    header = &thread->IrpList;
    entry = thread->IrpList.Flink;

    //
    // Walk the list of pending IRPs, completing each of them.
    //

    while (header != entry) {

        irp = CONTAINING_RECORD( entry, IRP, ThreadListEntry );
        entry = entry->Flink;

        if (irp->Flags & IRP_RETRY_IO_COMPLETION) {

            ASSERT(!(irp->Flags & IRP_CREATE_OPERATION));

            irp->Flags &= ~IRP_RETRY_IO_COMPLETION;
            fileObject = irp->Tail.Overlay.OriginalFileObject;
            IopCompleteRequest(
                    &irp->Tail.Apc, 
                    NULL, 
                    NULL, 
                    &fileObject,
                    &saveAuxiliaryPointer);
        }
    }
}
