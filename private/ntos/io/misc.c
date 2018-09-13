/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This module contains the code to implement the NtFlushBuffersFile,
    NtSetNewSizeFile, IoQueueWorkItem, and NtCancelIoFile system services
    for the NT I/O system.

Author:

    Darryl E. Havens (darrylh) 22-Jun-1989

Environment:

    Kernel mode only

Revision History:


--*/

#include "iop.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtCancelIoFile)
#pragma alloc_text(PAGE, NtDeleteFile)
#pragma alloc_text(PAGE, NtFlushBuffersFile)
#pragma alloc_text(PAGE, NtQueryAttributesFile)
#pragma alloc_text(PAGE, NtQueryFullAttributesFile)
#endif

//
// Local function prototypes follow
//

VOID
IopProcessWorkItem(
    IN PVOID Parameter
    );


NTSTATUS
NtCancelIoFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )

/*++

Routine Description:

    This service causes all pending I/O operations for the specified file to be
    marked as canceled.  Most types of operations can be canceled immediately,
    while others may continue toward completion before they are actually
    canceled and the caller is notified.

    Only those pending operations that were issued by the current thread using
    the specified handle are canceled.  Any operations issued for the file by
    any other thread or any other process continues normally.

Arguments:

    FileHandle - Supplies a handle to the file whose operations are to be
        canceled.

    IoStatusBlock - Address of the caller's I/O status block.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    PIRP irp;
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    KPROCESSOR_MODE requestorMode;
    PETHREAD thread;
    BOOLEAN found = FALSE;
    PLIST_ENTRY header;
    PLIST_ENTRY entry;
    KIRQL irql;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {

        //
        // The caller's access mode is user, so probe each of the arguments
        // and capture them as necessary.  If any failures occur, the condition
        // handler will be invoked to handle them.  It will simply cleanup and
        // return an access violation status code back to the system service
        // dispatcher.
        //

        try {

            //
            // The IoStatusBlock parameter must be writeable by the caller.
            //

            ProbeForWriteIoStatus( IoStatusBlock );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred attempting to probe the caller'
            // I/O status block.  Simply return an appropriate error status
            // code.
            //

            return GetExceptionCode();
        }
    }

    //
    // There were no blatant errors so far, so reference the file object so
    // the target device object can be found.  Note that if the handle does
    // not refer to a file object, or if the caller does not have the required
    // access to the file, then it will fail.
    //

    status = ObReferenceObjectByHandle( FileHandle,
                                        0,
                                        IoFileObjectType,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        NULL );
    if (!NT_SUCCESS( status )) {
        return(status);
    }

    //
    // Note that here the I/O system would normally make a check to determine
    // whether or not the file was opened for synchronous I/O.  If it was, then
    // it would attempt to exclusively acquire the file object lock.  However,
    // since this service is attempting to cancel all of the I/O for the file,
    // it does not make much sense to wait until it has all completed before
    // attempting to cancel it.
    //

    //
    // Get the address of the current thread.  The thread contains a list of
    // the pending operations for this file.
    //

    thread = PsGetCurrentThread();

    //
    // Update the operation count statistic for the current process for
    // operations other than read and write.
    //

    IopUpdateOtherOperationCount();

    //
    // Walk the list of IRPs on the thread's pending I/O queue looking for IRPs
    // which specify the same file as the FileHandle refers to.  For each IRP
    // found, set its cancel flag.  If no IRPs are found, simply complete the
    // I/O here.  The only synchronization needed here is to block out all APCs
    // for this thread so that no I/O can complete and remove packets from the
    // queue.  No considerations need be made for multi-processing since this
    // thread can only be running on one processor at a time and this routine
    // has control of the thread for now.
    //

    KeRaiseIrql( APC_LEVEL, &irql );

    header = &thread->IrpList;
    entry = thread->IrpList.Flink;

    while (header != entry) {

        //
        // An IRP has been found for this thread.  If the IRP refers to the
        // appropriate file object, set its cancel flag and remember that it
        // was found;  otherwise, simply continue the loop.
        //

        irp = CONTAINING_RECORD( entry, IRP, ThreadListEntry );
        if (irp->Tail.Overlay.OriginalFileObject == fileObject) {
            found = TRUE;
            IoCancelIrp( irp );
        }

        entry = entry->Flink;
    }

    //
    // Lower the IRQL back down to what it was on entry to this procedure.
    //

    KeLowerIrql( irql );

    if (found) {

        LARGE_INTEGER interval;

        //
        // Delay execution for a time and let the request
        // finish.  The delay time is 10ms.
        //

        interval.QuadPart = -10 * 1000 * 10;

        //
        // Wait for a while so the canceled requests can complete.
        //

        while (found) {

            (VOID) KeDelayExecutionThread( KernelMode, FALSE, &interval );

            found = FALSE;

            //
            // Raise the IRQL to prevent modification to the IRP list by the
            // thread's APC routine.
            //

            KeRaiseIrql( APC_LEVEL, &irql );

            //
            // Check the IRP list for requests which refer to the specified
            // file object.
            //

            entry = thread->IrpList.Flink;

            while (header != entry) {

                //
                // An IRP has been found for this thread.  If the IRP refers
                // to the appropriate file object,  remember that it
                // was found;  otherwise, simply continue the loop.
                //

                irp = CONTAINING_RECORD( entry, IRP, ThreadListEntry );
                if (irp->Tail.Overlay.OriginalFileObject == fileObject) {
                    found = TRUE;
                    break;
                }

                entry = entry->Flink;
            }

            //
            // Lower the IRQL back down to what it was on entry to this procedure.
            //

            KeLowerIrql( irql );

        }
    }

    try {

        //
        // Write the status back to the user.
        //

        IoStatusBlock->Status = STATUS_SUCCESS;
        IoStatusBlock->Information = 0L;

    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception was incurred attempting to write the caller's
        // I/O status block; however, the service completed sucessfully so
        // just return sucess.
        //

    }

    //
    // Dereference the file object.
    //

    ObDereferenceObject( fileObject );

    return STATUS_SUCCESS;
}

NTSTATUS
NtDeleteFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )

/*++

Routine Description:

    This service deletes the specified file.

Arguments:

    ObjectAttributes - Supplies the attributes to be used for file object (name,
        SECURITY_DESCRIPTOR, etc.)

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    KPROCESSOR_MODE requestorMode;
    NTSTATUS status;
    OPEN_PACKET openPacket;
    DUMMY_FILE_OBJECT localFileObject;
    HANDLE handle;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    //
    // Build a parse open packet that tells the parse method to open the file
    // for open for delete access w/the delete bit set, and then close it.
    //

    RtlZeroMemory( &openPacket, sizeof( OPEN_PACKET ) );

    openPacket.Type = IO_TYPE_OPEN_PACKET;
    openPacket.Size = sizeof( OPEN_PACKET );
    openPacket.CreateOptions = FILE_DELETE_ON_CLOSE;
    openPacket.ShareAccess = (USHORT) FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    openPacket.Disposition = FILE_OPEN;
    openPacket.DeleteOnly = TRUE;
    openPacket.LocalFileObject = &localFileObject;

    //
    // Update the open count for this process.
    //

    IopUpdateOtherOperationCount();

    //
    // Open the object by its name.  Because of the special DeleteOnly flag
    // set in the open packet, the parse routine will open the file, and
    // then realize that it is only deleting the file, and will therefore
    // immediately dereference the file.  This will cause the cleanup and
    // the close to be sent to the file system, thus causing the file to
    // be deleted.
    //

    status = ObOpenObjectByName( ObjectAttributes,
                                 (POBJECT_TYPE) NULL,
                                 requestorMode,
                                 NULL,
                                 DELETE,
                                 &openPacket,
                                 &handle );

    //
    // The operation is successful if the parse check field of the open packet
    // indicates that the parse routine was actually invoked, and the final
    // status field of the packet is set to success.
    //

    if (openPacket.ParseCheck != OPEN_PACKET_PATTERN) {
        return status;
    } else {
        return openPacket.FinalStatus;
    }
}

NTSTATUS
NtFlushBuffersFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )

/*++

Routine Description:

    This service causes all buffered data to the file to be written.

Arguments:

    FileHandle - Supplies a handle to the file whose buffers should be flushed.

    IoStatusBlock - Address of the caller's I/O status block.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    PIRP irp;
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PKEVENT event;
    KPROCESSOR_MODE requestorMode;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK localIoStatus;
    OBJECT_HANDLE_INFORMATION objectHandleInformation;
    BOOLEAN synchronousIo;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {

        //
        // The caller's access mode is not kernel so probe each of the arguments
        // and capture them as necessary.  If any failures occur, the condition
        // handler will be invoked to handle them.  It will simply cleanup and
        // return an access violation status code back to the system service
        // dispatcher.
        //

        try {

            //
            // The IoStatusBlock parameter must be writeable by the caller.
            //

            ProbeForWriteIoStatus( IoStatusBlock );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred attempting to probe the caller's
            // I/O status block.  Simply return an appropriate error status
            // code.
            //

            return GetExceptionCode();

        }
    }

    //
    // There were no blatant errors so far, so reference the file object so
    // the target device object can be found.  Note that if the handle does
    // not refer to a file object, or if the caller does not have the required
    // access to the file, then it will fail.
    //

    status = ObReferenceObjectByHandle( FileHandle,
                                        0,
                                        IoFileObjectType,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        &objectHandleInformation );
    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // Ensure that the caller has either WRITE or APPEND access to the file
    // before allowing this call to continue.  This is especially important
    // if the caller opened a volume, where a flush operation may flush more
    // than what this opener has written to buffers.  Note however that if
    // this is a pipe, then the APPEND access cannot be made since this
    // access code is overlaid with the CREATE_PIPE_INSTANCE access.
    //

    if (SeComputeGrantedAccesses( objectHandleInformation.GrantedAccess,
                                  (!(fileObject->Flags & FO_NAMED_PIPE) ?
                                  FILE_APPEND_DATA : 0) |
                                  FILE_WRITE_DATA ) == 0) {
        ObDereferenceObject( fileObject );
        return STATUS_ACCESS_DENIED;
    }

    //
    // Make a special check here to determine whether this is a synchronous
    // I/O operation.  If it is, then wait here until the file is owned by
    // the current thread.  If this is not a (serialized) synchronous I/O
    // operation, then allocate and initialize the local event.
    //

    if (fileObject->Flags & FO_SYNCHRONOUS_IO) {

        BOOLEAN interrupted;

        if (!IopAcquireFastLock( fileObject )) {
            status = IopAcquireFileObjectLock( fileObject,
                                               requestorMode,
                                               (BOOLEAN) ((fileObject->Flags & FO_ALERTABLE_IO) != 0),
                                               &interrupted );
            if (interrupted) {
                ObDereferenceObject( fileObject );
                return status;
            }
        }
        synchronousIo = TRUE;
    } else {

        //
        // This is a synchronous API being invoked for a file that is opened
        // for asynchronous I/O.  This means that this system service is
        // to synchronize the completion of the operation before returning
        // to the caller.  A local event is used to do this.
        //

        event = ExAllocatePool( NonPagedPool, sizeof( KEVENT ) );
        if (event == NULL) {
            ObDereferenceObject( fileObject );
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        KeInitializeEvent( event, SynchronizationEvent, FALSE );
        synchronousIo = FALSE;
    }

    //
    // Set the file object to the Not-Signaled state.
    //

    KeClearEvent( &fileObject->Event );

    //
    // Get the address of the target device object.
    //

    deviceObject = IoGetRelatedDeviceObject( fileObject );

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case the
    // caller does not have enough quota to allocate the packet.

    irp = IoAllocateIrp( deviceObject->StackSize, TRUE );
    if (!irp) {

        //
        // An exception was incurred while attempting to allocate the IRP.
        // Cleanup and return an appropriate error status code.
        //

        if (!(fileObject->Flags & FO_SYNCHRONOUS_IO)) {
            ExFreePool( event );
        }

        IopAllocateIrpCleanup( fileObject, (PKEVENT) NULL );

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = requestorMode;

    //
    // Fill in the service independent parameters in the IRP.
    //

    if (synchronousIo) {
        irp->UserEvent = (PKEVENT) NULL;
        irp->UserIosb = IoStatusBlock;
    } else {
        irp->UserEvent = event;
        irp->UserIosb = &localIoStatus;
        irp->Flags = IRP_SYNCHRONOUS_API;
    }
    irp->Overlay.AsynchronousParameters.UserApcRoutine = (PIO_APC_ROUTINE) NULL;

    //
    // Get a pointer to the stack location for the first driver.  This is used
    // to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_FLUSH_BUFFERS;
    irpSp->FileObject = fileObject;

    //
    // Queue the packet, call the driver, and synchronize appopriately with
    // I/O completion.
    //

    status = IopSynchronousServiceTail( deviceObject,
                                        irp,
                                        fileObject,
                                        FALSE,
                                        requestorMode,
                                        synchronousIo,
                                        OtherTransfer );

    //
    // If the file for this operation was not opened for synchronous I/O, then
    // synchronization of completion of the I/O operation has not yet occurred
    // since the allocated event must be used for synchronous APIs on files
    // opened for asynchronous I/O.  Synchronize the completion of the I/O
    // operation now.
    //

    if (!synchronousIo) {

        status = IopSynchronousApiServiceTail( status,
                                               event,
                                               irp,
                                               requestorMode,
                                               &localIoStatus,
                                               IoStatusBlock );
    }

    return status;
}

NTSTATUS
NtQueryAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_BASIC_INFORMATION FileInformation
    )

/*++

Routine Description:

    This service queries the basic attributes information for a specified file.

Arguments:

    ObjectAttributes - Supplies the attributes to be used for file object (name,
        SECURITY_DESCRIPTOR, etc.)

    FileInformation - Supplies an output buffer to receive the returned file
        attributes information.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    KPROCESSOR_MODE requestorMode;
    NTSTATUS status;
    OPEN_PACKET openPacket;
    DUMMY_FILE_OBJECT localFileObject;
    FILE_NETWORK_OPEN_INFORMATION networkInformation;
    HANDLE handle;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {

        try {

            //
            // The caller's mode is not kernel, so probe the output buffer.
            //

            ProbeForWrite( FileInformation,
                           sizeof( FILE_BASIC_INFORMATION ),
                           sizeof( ULONG ));

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred while probing the caller's parameters.
            // Simply return an appropriate error status code.
            //

            return GetExceptionCode();
        }
    }

    //
    // Build a parse open packet that tells the parse method to open the file,
    // query the file's basic attributes, and close the file.
    //

    RtlZeroMemory( &openPacket, sizeof( OPEN_PACKET ) );

    openPacket.Type = IO_TYPE_OPEN_PACKET;
    openPacket.Size = sizeof( OPEN_PACKET );
    openPacket.ShareAccess = (USHORT) FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    openPacket.Disposition = FILE_OPEN;
    openPacket.CreateOptions = FILE_OPEN_REPARSE_POINT;
    openPacket.BasicInformation = FileInformation;
    openPacket.NetworkInformation = &networkInformation;
    openPacket.QueryOnly = TRUE;
    openPacket.LocalFileObject = &localFileObject;

    //
    // Update the open count for this process.
    //

    IopUpdateOtherOperationCount();

    //
    // Open the object by its name.  Because of the special QueryOnly flag set
    // in the open packet, the parse routine will open the file, and then
    // realize that it is only performing a query.  It will therefore perform
    // the query, and immediately close the file.
    //

    status = ObOpenObjectByName( ObjectAttributes,
                                 (POBJECT_TYPE) NULL,
                                 requestorMode,
                                 NULL,
                                 FILE_READ_ATTRIBUTES,
                                 &openPacket,
                                 &handle );

    //
    // The operation is successful if the parse check field of the open packet
    // indicates that the parse routine was actually invoked, and the final
    // status field of the packet is set to success.
    //

    if (openPacket.ParseCheck != OPEN_PACKET_PATTERN) {
        return status;
    } else {
        return openPacket.FinalStatus;
    }
}

NTSTATUS
NtQueryFullAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
    )

/*++

Routine Description:

    This service queries the network attributes information for a specified
    file.

Arguments:

    ObjectAttributes - Supplies the attributes to be used for file object (name,
        SECURITY_DESCRIPTOR, etc.)

    FileInformation - Supplies an output buffer to receive the returned file
        attributes information.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    KPROCESSOR_MODE requestorMode;
    NTSTATUS status;
    OPEN_PACKET openPacket;
    DUMMY_FILE_OBJECT localFileObject;
    FILE_NETWORK_OPEN_INFORMATION networkInformation;
    HANDLE handle;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {

        try {

            //
            // The caller's mode is not kernel, so probe the output buffer.
            //

            ProbeForWrite( FileInformation,
                           sizeof( FILE_NETWORK_OPEN_INFORMATION ),
#if defined(_X86_)
                           sizeof( LONG ));
#else
                           sizeof( LONGLONG ));
#endif // defined(_X86_)

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred while probing the caller's parameters.
            // Simply return an appropriate error status code.
            //

            return GetExceptionCode();
        }
    }

    //
    // Build a parse open packet that tells the parse method to open the file,
    // query the file's full attributes, and close the file.
    //

    RtlZeroMemory( &openPacket, sizeof( OPEN_PACKET ) );

    openPacket.Type = IO_TYPE_OPEN_PACKET;
    openPacket.Size = sizeof( OPEN_PACKET );
    openPacket.ShareAccess = (USHORT) FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    openPacket.Disposition = FILE_OPEN;
    openPacket.CreateOptions = FILE_OPEN_REPARSE_POINT;
    openPacket.QueryOnly = TRUE;
    openPacket.FullAttributes = TRUE;
    openPacket.LocalFileObject = &localFileObject;
    if (requestorMode != KernelMode) {
        openPacket.NetworkInformation = &networkInformation;
    } else {
        openPacket.NetworkInformation = FileInformation;
    }

    //
    // Update the open count for this process.
    //

    IopUpdateOtherOperationCount();

    //
    // Open the object by its name.  Because of the special QueryOnly flag set
    // in the open packet, the parse routine will open the file, and then
    // realize that it is only performing a query.  It will therefore perform
    // the query, and immediately close the file.
    //

    status = ObOpenObjectByName( ObjectAttributes,
                                 (POBJECT_TYPE) NULL,
                                 requestorMode,
                                 NULL,
                                 FILE_READ_ATTRIBUTES,
                                 &openPacket,
                                 &handle );

    //
    // The operation is successful if the parse check field of the open packet
    // indicates that the parse routine was actually invoked, and the final
    // status field of the packet is set to success.
    //

    if (openPacket.ParseCheck != OPEN_PACKET_PATTERN) {
        return status;
    } else {
        status = openPacket.FinalStatus;
    }

    if (NT_SUCCESS( status )) {
        if (requestorMode != KernelMode) {
            try {

                //
                // The query worked, so copy the returned information to the
                // caller's output buffer.
                //

                RtlMoveMemory( FileInformation,
                               &networkInformation,
                               sizeof( FILE_NETWORK_OPEN_INFORMATION ) );

            } except(EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode();
            }
        }
    }

    return status;
}

PIO_WORKITEM
IoAllocateWorkItem(
    PDEVICE_OBJECT DeviceObject
    )
{
    PIO_WORKITEM ioWorkItem;
    PWORK_QUEUE_ITEM exWorkItem;

    //
    // Allocate a new workitem structure.
    // 

    ioWorkItem = ExAllocatePool( NonPagedPool, sizeof( IO_WORKITEM ));
    if (ioWorkItem != NULL) {

        //
        // Initialize the invariant portions of both ioWorkItem and
        // exWorkItem.
        //

#if DBG
        ioWorkItem->Size = sizeof( IO_WORKITEM );
#endif

        ioWorkItem->DeviceObject = DeviceObject;

        exWorkItem = &ioWorkItem->WorkItem;
        ExInitializeWorkItem( exWorkItem, IopProcessWorkItem, ioWorkItem );
    }

    return ioWorkItem;
}

VOID
IoFreeWorkItem(
    PIO_WORKITEM IoWorkItem
    )

/*++

Routine Description:

    This function is the "wrapper" routine for IoQueueWorkItem.  It calls
    the original worker function, then dereferences the device object to
    (possibly) allow the driver object to go away.

Arguments:

    Parameter - Supplies a pointer to an IO_WORKITEM for us to process.

Return Value:

    None

--*/

{
    ASSERT( IoWorkItem->Size == sizeof( IO_WORKITEM ));

    ExFreePool( IoWorkItem );
}

VOID
IoQueueWorkItem(
    IN PIO_WORKITEM IoWorkItem,
    IN PIO_WORKITEM_ROUTINE WorkerRoutine,
    IN WORK_QUEUE_TYPE QueueType,
    IN PVOID Context
    )
/*++

Routine Description:

    This function inserts a work item into a work queue that is processed
    by a worker thread of the corresponding type.  It effectively
    "wraps" ExQueueWorkItem, ensuring that the device object is referenced
    for the duration of the call.

Arguments:

    IoWorkItem - Supplies a pointer to the work item to add the the queue.
        This structure must have been allocated via IoAllocateWorkItem().

    WorkerRoutine - Supplies a pointer to the routine that is to be called
        in system thread context.

    QueueType - Specifies the type of work queue that the work item
        should be placed in.

    Context - Supplies the context parameter for the callback routine.

Return Value:

    None

--*/

{
    PWORK_QUEUE_ITEM exWorkItem;

    ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
    ASSERT( IoWorkItem->Size == sizeof( IO_WORKITEM ));

    //
    // Keep a reference on the device object so it doesn't go away.
    //

    ObReferenceObject( IoWorkItem->DeviceObject );

    //
    // Initialize the fields in IoWorkItem
    //

    IoWorkItem->Routine = WorkerRoutine;
    IoWorkItem->Context = Context;

    //
    // Get a pointer to the ExWorkItem, queue it, and return.
    // IopProcessWorkItem() will perform the dereference.
    // 

    exWorkItem = &IoWorkItem->WorkItem;
    ExQueueWorkItem( exWorkItem, QueueType );
}

VOID
IopProcessWorkItem(
    IN PVOID Parameter
    )

/*++

Routine Description:

    This function is the "wrapper" routine for IoQueueWorkItem.  It calls
    the original worker function, then dereferences the device object to
    (possibly) allow the driver object to go away.

Arguments:

    Parameter - Supplies a pointer to an IO_WORKITEM for us to process.

Return Value:

    None

--*/

{
    PIO_WORKITEM ioWorkItem;
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    //
    // Get a pointer to the ioWorkItem and store a copy of DeviceObject
    // locally.  This allow us to function properly if the worker routine
    // elects to free the work item.
    //

    ioWorkItem = (PIO_WORKITEM)Parameter;
    deviceObject = ioWorkItem->DeviceObject;

    //
    // Call the original worker.
    //

    ioWorkItem->Routine( deviceObject,
                         ioWorkItem->Context );

    //
    // Now we can dereference the device object, since its code is no longer
    // being executed for this work item.
    //

    ObDereferenceObject( deviceObject );
}

