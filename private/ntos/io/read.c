/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    read.c

Abstract:

    This module contains the code to implement the NtReadFile system service.

Author:

    Darryl E. Havens (darrylh) 14-Apr-1989

Environment:

    Kernel mode

Revision History:


--*/

#include "iop.h"

ULONG IopCacheHitIncrement = 0;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtReadFile)
#pragma alloc_text(PAGE, NtReadFileScatter)
#endif

NTSTATUS
NtReadFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    )

/*++

Routine Description:

    This service reads Length bytes of data from the file associated with
    FileHandle starting at ByteOffset and puts the data into the caller's
    Buffer.  If the end of the file is reached before Length bytes have
    been read, then the operation will terminate.  The actual length of
    the data read from the file will be returned in the second longword
    of the IoStatusBlock.

Arguments:

    FileHandle - Supplies a handle to the file to be read.

    Event - Optionally supplies an event to be signaled when the read operation
        is complete.

    ApcRoutine - Optionally supplies an APC routine to be executed when the read
        operation is complete.

    ApcContext - Supplies a context parameter to be passed to the ApcRoutine, if
        an ApcRoutine was specified.

    IoStatusBlock - Address of the caller's I/O status block.

    Buffer - Address of buffer to receive the data read from the file.

    Length - Supplies the length, in bytes, of the data to read from the file.

    ByteOffset - Optionally specifies the starting byte offset within the file
        to begin the read operation.  If not specified and the file is open
        for synchronous I/O, then the current file position is used.  If the
        file is not opened for synchronous I/O and the parameter is not
        specified, then it is an error.

    Key - Optionally specifies a key to be used if there are locks associated
        with the file.

Return Value:

    The status returned is success if the read operation was properly queued
    to the I/O system.  Once the read completes the status of the operation
    can be determined by examining the Status field of the I/O status block.

--*/

{
    PIRP irp;
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    KPROCESSOR_MODE requestorMode;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS exceptionCode;
    BOOLEAN synchronousIo;
    PKEVENT eventObject = (PKEVENT) NULL;
    ULONG keyValue = 0;
    LARGE_INTEGER fileOffset = {0,0};
    PULONG majorFunction;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    //
    // Reference the file object so the target device can be found.  Note
    // that if the caller does not have read access to the file, the operation
    // will fail.
    //

    status = ObReferenceObjectByHandle( FileHandle,
                                        FILE_READ_DATA,
                                        IoFileObjectType,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        NULL );
    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // Get the address of the target device object.
    //

    deviceObject = IoGetRelatedDeviceObject( fileObject );

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

            ProbeForWriteIoStatusEx(IoStatusBlock , ApcRoutine);

            //
            // The caller's data buffer must be writable from the caller's
            // mode.  This check ensures that this is the case.  Since the
            // buffer address is captured, the caller cannot change it,
            // even though he/she can change the protection from another
            // thread.  This error will be caught by the probe/lock or
            // buffer copy operations later.
            //

            ProbeForWrite( Buffer, Length, sizeof( UCHAR ) );

            //
            // If this file has an I/O completion port associated w/it, then
            // ensure that the caller did not supply an APC routine, as the
            // two are mutually exclusive methods for I/O completion
            // notification.
            //

            if (fileObject->CompletionContext && IopApcRoutinePresent( ApcRoutine )) {
                ObDereferenceObject( fileObject );
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Also ensure that the ByteOffset parameter is readable from
            // the caller's mode and capture it if it is present.
            //

            if (ARGUMENT_PRESENT( ByteOffset )) {
                ProbeForRead( ByteOffset,
                              sizeof( LARGE_INTEGER ),
                              sizeof( ULONG ) );
                fileOffset = *ByteOffset;
            }

            //
            // Check to see whether the caller has opened the file without
            // intermediate buffering.  If so, perform the following Buffer
            // and ByteOffset parameter checks differently.
            //

            if (fileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {

                //
                // The file was opened without intermediate buffering enabled.
                // Check that the Buffer is properly aligned, and that the
                // length is an integral number of 512-byte blocks.
                //

                if ((deviceObject->SectorSize &&
                    (Length & (deviceObject->SectorSize - 1))) ||
                    (ULONG_PTR) Buffer & deviceObject->AlignmentRequirement) {

                    //
                    // Check for sector sizes that are not a power of two.
                    //

                    if ((deviceObject->SectorSize &&
                        Length % deviceObject->SectorSize) ||
                        (ULONG_PTR) Buffer & deviceObject->AlignmentRequirement) {
                        ObDereferenceObject( fileObject );
                        return STATUS_INVALID_PARAMETER;
                    }
                }

                //
                // If a ByteOffset parameter was specified, ensure that it
                // is a valid argument.
                //

                if (ARGUMENT_PRESENT( ByteOffset )) {
                    if (deviceObject->SectorSize &&
                        (fileOffset.LowPart & (deviceObject->SectorSize - 1))) {
                        ObDereferenceObject( fileObject );
                        return STATUS_INVALID_PARAMETER;
                    }
                }
            }

            //
            // Finally, ensure that if there is a key parameter specified it
            // is readable by the caller.
            //

            if (ARGUMENT_PRESENT( Key )) {
                keyValue = ProbeAndReadUlong( Key );
            }

        } except(IopExceptionFilter( GetExceptionInformation(), &exceptionCode )) {

            //
            // An exception was incurred while attempting to probe the
            // caller's parameters.  Dereference the file object and return
            // an appropriate error status code.
            //

            ObDereferenceObject( fileObject );
            return exceptionCode;

        }

    } else {

        //
        // The caller's mode is kernel.  Get the same parameters that are
        // required from any other mode.
        //

        if (ARGUMENT_PRESENT( ByteOffset )) {
            fileOffset = *ByteOffset;
        }

        if (ARGUMENT_PRESENT( Key )) {
            keyValue = *Key;
        }

#if DBG
        if (fileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {

            //
            // The file was opened without intermediate buffering enabled.
            // Check that the Buffer is properly aligned, and that the
            // length is an integral number of the block size.
            //

            if ((deviceObject->SectorSize &&
                (Length & (deviceObject->SectorSize - 1))) ||
                (ULONG_PTR) Buffer & deviceObject->AlignmentRequirement) {

                //
                // Check for sector sizes that are not a power of two.
                //

                if ((deviceObject->SectorSize &&
                    Length % deviceObject->SectorSize) ||
                    (ULONG_PTR) Buffer & deviceObject->AlignmentRequirement) {
                    ObDereferenceObject( fileObject );
                    ASSERT( FALSE );
                    return STATUS_INVALID_PARAMETER;
                }
            }

            //
            // If a ByteOffset parameter was specified, ensure that it
            // is a valid argument.
            //

            if (ARGUMENT_PRESENT( ByteOffset )) {
                if (deviceObject->SectorSize &&
                    (fileOffset.LowPart & (deviceObject->SectorSize - 1))) {
                    ObDereferenceObject( fileObject );
                    ASSERT( FALSE );
                    return STATUS_INVALID_PARAMETER;
                }
            }
        }
#endif // DBG
    }

    //
    // Get the address of the event object and set the event to the Not-
    // Signaled state, if an one was specified.  Note here too, that if
    // the handle does not refer to an event, then the reference will fail.
    //

    if (ARGUMENT_PRESENT( Event )) {
        status = ObReferenceObjectByHandle( Event,
                                            EVENT_MODIFY_STATE,
                                            ExEventObjectType,
                                            requestorMode,
                                            (PVOID *) &eventObject,
                                            NULL );
        if (!NT_SUCCESS( status )) {
            ObDereferenceObject( fileObject );
            return status;
        } else {
            KeClearEvent( eventObject );
        }
    }

    //
    // Get the address of the driver object's Fast I/O dispatch structure.
    //

    fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

    //
    // Make a special check here to determine whether this is a synchronous
    // I/O operation.  If it is, then wait here until the file is owned by
    // the current thread.
    //

    if (fileObject->Flags & FO_SYNCHRONOUS_IO) {

        BOOLEAN interrupted;

        if (!IopAcquireFastLock( fileObject )) {
            status = IopAcquireFileObjectLock( fileObject,
                                               requestorMode,
                                               (BOOLEAN) ((fileObject->Flags & FO_ALERTABLE_IO) != 0),
                                               &interrupted );
            if (interrupted) {
                if (eventObject) {
                    ObDereferenceObject( eventObject );
                }
                ObDereferenceObject( fileObject );
                return status;
            }
        }

        if (!ARGUMENT_PRESENT( ByteOffset ) ||
            (fileOffset.LowPart == FILE_USE_FILE_POINTER_POSITION &&
            fileOffset.HighPart == -1)) {
            fileOffset = fileObject->CurrentByteOffset;
        }

        //
        // Turbo read support.  If the file is currently cached on this
        // file object, then call the Cache Manager directly via FastIoRead
        // and try to successfully complete the request here.  Note if
        // FastIoRead returns FALSE or we get an I/O error, we simply
        // fall through and go the "long way" and create an Irp.
        //

        if (fileObject->PrivateCacheMap) {

            IO_STATUS_BLOCK localIoStatus;

            ASSERT(fastIoDispatch && fastIoDispatch->FastIoRead);

            //
            //  Negative file offsets are illegal.
            //

            if (fileOffset.HighPart < 0) {
                if (eventObject) {
                    ObDereferenceObject( eventObject );
                }
                IopReleaseFileObjectLock( fileObject );
                ObDereferenceObject( fileObject );
                return STATUS_INVALID_PARAMETER;
            }

            if (fastIoDispatch->FastIoRead( fileObject,
                                            &fileOffset,
                                            Length,
                                            TRUE,
                                            keyValue,
                                            Buffer,
                                            &localIoStatus,
                                            deviceObject )

                    &&

                ((localIoStatus.Status == STATUS_SUCCESS) ||
                 (localIoStatus.Status == STATUS_BUFFER_OVERFLOW) ||
                 (localIoStatus.Status == STATUS_END_OF_FILE))) {

                //
                // Boost the priority of the current thread so that it appears
                // as if it just did I/O.  This causes background jobs that
                // get cache hits to be more responsive in terms of getting
                // more CPU time.
                //

                if (IopCacheHitIncrement) {
                    KeBoostPriorityThread( &PsGetCurrentThread()->Tcb,
                                           (KPRIORITY) IopCacheHitIncrement );
                }

                //
                // Carefully return the I/O status.
                //

                IopUpdateReadOperationCount( );
                IopUpdateReadTransferCount( (ULONG)localIoStatus.Information );

                try {
                    *IoStatusBlock = localIoStatus;
                } except( EXCEPTION_EXECUTE_HANDLER ) {
                    localIoStatus.Status = GetExceptionCode();
                    localIoStatus.Information = 0;
                }

                //
                // If an event was specified, set it.
                //

                if (ARGUMENT_PRESENT( Event )) {
                    KeSetEvent( eventObject, 0, FALSE );
                    ObDereferenceObject( eventObject );
                }

                //
                // Note that the file object event need not be set to the
                // Signaled state, as it is already set.
                //

                //
                // Cleanup and return.
                //

                IopReleaseFileObjectLock( fileObject );
                ObDereferenceObject( fileObject );

                return localIoStatus.Status;
            }
        }
        synchronousIo = TRUE;

    } else if (!ARGUMENT_PRESENT( ByteOffset ) && !(fileObject->Flags & (FO_NAMED_PIPE | FO_MAILSLOT))) {

        //
        // The file is not open for synchronous I/O operations, but the
        // caller did not specify a ByteOffset parameter.
        //

        if (eventObject) {
            ObDereferenceObject( eventObject );
        }
        ObDereferenceObject( fileObject );
        return STATUS_INVALID_PARAMETER;
    } else {
        synchronousIo = FALSE;
    }

    //
    //  Negative file offsets are illegal.
    //

    if (fileOffset.HighPart < 0) {
        if (eventObject) {
            ObDereferenceObject( eventObject );
        }
        if (synchronousIo) {
            IopReleaseFileObjectLock( fileObject );
        }
        ObDereferenceObject( fileObject );
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Set the file object to the Not-Signaled state.
    //

    KeClearEvent( &fileObject->Event );

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case the
    // caller does not have enough quota to allocate the packet.

    irp = IopAllocateIrp( deviceObject->StackSize, TRUE );
    if (!irp) {

        //
        // An IRP could not be allocated.  Cleanup and return an appropriate
        // error status code.
        //

        IopAllocateIrpCleanup( fileObject, eventObject );

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.AuxiliaryBuffer = (PVOID) NULL;
    irp->RequestorMode = requestorMode;
    irp->PendingReturned = FALSE;
    irp->Cancel = FALSE;
    irp->CancelRoutine = (PDRIVER_CANCEL) NULL;

    //
    // Fill in the service independent parameters in the IRP.
    //

    irp->UserEvent = eventObject;
    irp->UserIosb = IoStatusBlock;
    irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.  Note that
    // setting the major function here also sets:
    //
    //      MinorFunction = 0;
    //      Flags = 0;
    //      Control = 0;
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    majorFunction = (PULONG) (&irpSp->MajorFunction);
    *majorFunction = IRP_MJ_READ;
    irpSp->FileObject = fileObject;

    //
    // Now determine whether this device expects to have data buffered to it
    // or whether it performs direct I/O.  This is based on the DO_BUFFERED_IO
    // flag in the device object.  If the flag is set, then a system buffer is
    // allocated and the driver's data will be copied into it.  Otherwise, a
    // Memory Descriptor List (MDL) is allocated and the caller's buffer is
    // locked down using it.
    //

    irp->AssociatedIrp.SystemBuffer = (PVOID) NULL;
    irp->MdlAddress = (PMDL) NULL;

    if (deviceObject->Flags & DO_BUFFERED_IO) {

        //
        // The device does not support direct I/O.  Allocate a system buffer
        // and specify that it should be deallocated on completion.  Also
        // indicate that this is an input operation so the data will be copied
        // into the caller's buffer.  This is done using an exception handler
        // that will perform cleanup if the operation fails.  Note that this
        // is only done if the operation has a non-zero length.
        //

        if (Length) {

            try {

                //
                // Allocate the intermediary system buffer from nonpaged pool
                // and charge quota for it.
                //

                irp->AssociatedIrp.SystemBuffer =
                    ExAllocatePoolWithQuota( NonPagedPoolCacheAligned, Length );

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception was incurred while either probing the caller's
                // buffer or allocating the system buffer.  Determine what
                // actually happened, clean everything up, and return an
                // appropriate error status code.
                //

                IopExceptionCleanup( fileObject,
                                     irp,
                                     eventObject,
                                     (PKEVENT) NULL );

                return GetExceptionCode();

            }

            //
            // Remember the address of the caller's buffer so the copy can take
            // place during I/O completion.  Also, set the flags so that the
            // completion code knows to do the copy and to deallocate the buffer.
            //

            irp->UserBuffer = Buffer;
            irp->Flags = IRP_BUFFERED_IO |
                         IRP_DEALLOCATE_BUFFER |
                         IRP_INPUT_OPERATION;

        } else {

            //
            // This is a zero-length read.  Simply indicate that this is
            // buffered I/O, and pass along the request.  The buffer will
            // not be set to deallocate so the completion path does not
            // have to special-case the length.
            //

            irp->Flags = IRP_BUFFERED_IO | IRP_INPUT_OPERATION;

        }

    } else if (deviceObject->Flags & DO_DIRECT_IO) {

        //
        // This is a direct I/O operation.  Allocate an MDL and invoke the
        // memory management routine to lock the buffer into memory.  This
        // is done using an exception handler that will perform cleanup if
        // the operation fails.  Note that no MDL is allocated, nor is any
        // memory probed or locked if the length of the request was zero.
        //

        PMDL mdl;

        irp->Flags = 0;

        if (Length) {

            try {

                //
                // Allocate an MDL, charging quota for it, and hang it off of
                // the IRP.  Probe and lock the pages associated with the
                // caller's buffer for write access and fill in the MDL with
                // the PFNs of those pages.
                //

                mdl = IoAllocateMdl( Buffer, Length, FALSE, TRUE, irp );
                if (mdl == NULL) {
                    ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                }
                MmProbeAndLockPages( mdl, requestorMode, IoWriteAccess );

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception was incurred while either probing the caller's
                // buffer or allocating the MDL.  Determine what actually
                // happened, clean everything up, and return an appropriate
                // error status code.
                //

                IopExceptionCleanup( fileObject,
                                     irp,
                                     eventObject,
                                     (PKEVENT) NULL );

                return GetExceptionCode();

            }

        }

    } else {

        //
        // Pass the address of the user's buffer so the driver has access to
        // it.  It is now the driver's responsibility to do everything.
        //

        irp->Flags = 0;
        irp->UserBuffer = Buffer;
    }

    //
    // If this read operation is supposed to be performed with caching disabled
    // set the disable flag in the IRP so no caching is performed.
    //

    if (fileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {
        irp->Flags |= IRP_NOCACHE | IRP_READ_OPERATION | IRP_DEFER_IO_COMPLETION;
    } else {
        irp->Flags |= IRP_READ_OPERATION | IRP_DEFER_IO_COMPLETION;
    }

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.Read.Length = Length;
    irpSp->Parameters.Read.Key = keyValue;
    irpSp->Parameters.Read.ByteOffset = fileOffset;

    //
    // Queue the packet, call the driver, and synchronize appopriately with
    // I/O completion.
    //

    status =  IopSynchronousServiceTail( deviceObject,
                                         irp,
                                         fileObject,
                                         TRUE,
                                         requestorMode,
                                         synchronousIo,
                                         ReadTransfer );

    return status;
}

NTSTATUS
NtReadFileScatter(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    )

/*++

Routine Description:

    This service reads Length bytes of data from the file associated with
    FileHandle starting at ByteOffset and puts the data into the caller's
    buffer segments.  The buffer segments are not virtually contiguous,
    but are 8 KB in length and alignment. If the end of the file is reached
    before Length bytes have been read, then the operation will terminate.
    The actual length of the data read from the file will be returned in
    the second longword of the IoStatusBlock.

Arguments:

    FileHandle - Supplies a handle to the file to be read.

    Event - Unused the I/O must use a completion port.

    ApcRoutine - Optionally supplies an APC routine to be executed when the read
        operation is complete.

    ApcContext - Supplies a context parameter to be passed to the ApcRoutine, if
        an ApcRoutine was specified.

    IoStatusBlock - Address of the caller's I/O status block.

    SegmentArray - An array of buffer segment pointers that specify
        where the data should be placed.

    Length - Supplies the length, in bytes, of the data to read from the file.

    ByteOffset - Optionally specifies the starting byte offset within the file
        to begin the read operation.  If not specified and the file is open
        for synchronous I/O, then the current file position is used.  If the
        file is not opened for synchronous I/O and the parameter is not
        specified, then it is an error.

    Key - Unused.

Return Value:

    The status returned is success if the read operation was properly queued
    to the I/O system.  Once the read completes the status of the operation
    can be determined by examining the Status field of the I/O status block.

Notes:

    This interface is only supported for no buffering and asynchronous I/O.

--*/

{
    PIRP irp;
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PFILE_SEGMENT_ELEMENT capturedArray = NULL;
    KPROCESSOR_MODE requestorMode;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS exceptionCode;
    PKEVENT eventObject = (PKEVENT) NULL;
    ULONG keyValue = 0;
    ULONG elementCount;
    LARGE_INTEGER fileOffset = {0,0};
    PULONG majorFunction;
    ULONG i;
    BOOLEAN synchronousIo;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    //
    // Reference the file object so the target device can be found.  Note
    // that if the caller does not have read access to the file, the operation
    // will fail.
    //

    status = ObReferenceObjectByHandle( FileHandle,
                                        FILE_READ_DATA,
                                        IoFileObjectType,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        NULL );
    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // Get the address of the target device object.
    //

    deviceObject = IoGetRelatedDeviceObject( fileObject );

    //
    // Verify this is a valid scatter read request.  In particular it must be
    // non cached, asynchronous, use completion ports, non buffer I/O device
    // and directed at a file system device.
    //

    if (!(fileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) ||
        (fileObject->Flags & FO_SYNCHRONOUS_IO) ||
        deviceObject->Flags & DO_BUFFERED_IO ||
        (deviceObject->DeviceType != FILE_DEVICE_DISK_FILE_SYSTEM &&
         deviceObject->DeviceType != FILE_DEVICE_DFS &&
         deviceObject->DeviceType != FILE_DEVICE_TAPE_FILE_SYSTEM &&
         deviceObject->DeviceType != FILE_DEVICE_CD_ROM_FILE_SYSTEM &&
         deviceObject->DeviceType != FILE_DEVICE_NETWORK_FILE_SYSTEM &&
         deviceObject->DeviceType != FILE_DEVICE_FILE_SYSTEM &&
         deviceObject->DeviceType != FILE_DEVICE_DFS_VOLUME )) {

        ObDereferenceObject( fileObject );
        return STATUS_INVALID_PARAMETER;
    }

    elementCount = BYTES_TO_PAGES(Length);

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

            ProbeForWriteIoStatusEx( IoStatusBlock , ApcRoutine);

            //
            // If this file has an I/O completion port associated w/it, then
            // ensure that the caller did not supply an APC routine, as the
            // two are mutually exclusive methods for I/O completion
            // notification.
            //

            if (fileObject->CompletionContext && IopApcRoutinePresent( ApcRoutine )) {
                ObDereferenceObject( fileObject );
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Also ensure that the ByteOffset parameter is readable from
            // the caller's mode and capture it if it is present.
            //

            if (ARGUMENT_PRESENT( ByteOffset )) {
                ProbeForRead( ByteOffset,
                              sizeof( LARGE_INTEGER ),
                              sizeof( ULONG ) );
                fileOffset = *ByteOffset;
            }

            //
            // Check to see whether the caller has opened the file without
            // intermediate buffering.  If so, perform the following ByteOffset
            // parameter check differently.
            //

            if (fileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {

                //
                // The file was opened without intermediate buffering enabled.
                // Check that the Buffer is properly aligned, and that the
                // length is an integral number of 512-byte blocks.
                //

                if ((deviceObject->SectorSize &&
                    (Length & (deviceObject->SectorSize - 1)))) {

                    //
                    // Check for sector sizes that are not a power of two.
                    //

                    if ((deviceObject->SectorSize &&
                        Length % deviceObject->SectorSize)) {
                        ObDereferenceObject( fileObject );
                        return STATUS_INVALID_PARAMETER;
                    }
                }

                //
                // If a ByteOffset parameter was specified, ensure that it
                // is a valid argument.
                //

                if (ARGUMENT_PRESENT( ByteOffset )) {
                    if (deviceObject->SectorSize &&
                        (fileOffset.LowPart & (deviceObject->SectorSize - 1))) {
                        ObDereferenceObject( fileObject );
                        return STATUS_INVALID_PARAMETER;
                    }
                }
            }

            //
            // The SegmentArray paramter must be accessible.
            //

#ifdef _X86_
            ProbeForRead( SegmentArray,
                          elementCount * sizeof( FILE_SEGMENT_ELEMENT ),
                          sizeof( ULONG )
                          );
#elif defined(_WIN64)

            //
            // If we are a wow64 process, follow the X86 rules
            //

            if (PsGetCurrentProcess()->Wow64Process) {
                ProbeForRead( SegmentArray,
                              elementCount * sizeof( FILE_SEGMENT_ELEMENT ),
                              sizeof( ULONG )
                              );
            } else {
                ProbeForRead( SegmentArray,
                              elementCount * sizeof( FILE_SEGMENT_ELEMENT ),
                              TYPE_ALIGNMENT( FILE_SEGMENT_ELEMENT )
                              );
            }
#else
            ProbeForRead( SegmentArray,
                          elementCount * sizeof( FILE_SEGMENT_ELEMENT ),
                          TYPE_ALIGNMENT( FILE_SEGMENT_ELEMENT )
                          );
#endif

            if (Length != 0) {

                //
                // Capture the segment array so it cannot be changed after
                // it has been looked at.
                //

                capturedArray = ExAllocatePoolWithQuota( PagedPool,
                                                elementCount * sizeof( FILE_SEGMENT_ELEMENT )
                                                );

                RtlCopyMemory( capturedArray,
                               SegmentArray,
                               elementCount * sizeof( FILE_SEGMENT_ELEMENT )
                               );

                SegmentArray = capturedArray;

                //
                // Verify that all the addresses are page aligned.
                //

                for (i = 0; i < elementCount; i++) {

                    if ( SegmentArray[i].Alignment & (PAGE_SIZE - 1)) {
                        ExRaiseStatus( STATUS_INVALID_PARAMETER );
                    }
                }
            }

            //
            // Finally, ensure that if there is a key parameter specified it
            // is readable by the caller.
            //

            if (ARGUMENT_PRESENT( Key )) {
                keyValue = ProbeAndReadUlong( Key );
            }

        } except(IopExceptionFilter( GetExceptionInformation(), &exceptionCode )) {

            //
            // An exception was incurred while attempting to probe the
            // caller's parameters.  Dereference the file object and return
            // an appropriate error status code.
            //

            ObDereferenceObject( fileObject );
            if (capturedArray != NULL) {
                ExFreePool( capturedArray );
            }
            return exceptionCode;

        }

    } else {

        //
        // The caller's mode is kernel.  Get the same parameters that are
        // required from any other mode.
        //

        if (ARGUMENT_PRESENT( ByteOffset )) {
            fileOffset = *ByteOffset;
        }

        if (ARGUMENT_PRESENT( Key )) {
            keyValue = *Key;
        }

#if DBG
        if (fileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {

            //
            // The file was opened without intermediate buffering enabled.
            // Check that the the length is an integral number of the block
            //  size.
            //

            if ((deviceObject->SectorSize &&
                (Length & (deviceObject->SectorSize - 1)))) {

                //
                // Check for sector sizes that are not a power of two.
                //

                if ((deviceObject->SectorSize &&
                    Length % deviceObject->SectorSize)) {
                    ObDereferenceObject( fileObject );
                    ASSERT( FALSE );
                    return STATUS_INVALID_PARAMETER;
                }
            }

            //
            // If a ByteOffset parameter was specified, ensure that it
            // is a valid argument.
            //

            if (ARGUMENT_PRESENT( ByteOffset )) {
                if (deviceObject->SectorSize &&
                    (fileOffset.LowPart & (deviceObject->SectorSize - 1))) {
                    ObDereferenceObject( fileObject );
                    ASSERT( FALSE );
                    return STATUS_INVALID_PARAMETER;
                }
            }
        }

        if (Length != 0) {

            //
            // Verify that all the addresses are page aligned.
            //

            for (i = 0; i < elementCount; i++) {

                if ( SegmentArray[i].Alignment & (PAGE_SIZE - 1)) {

                    ObDereferenceObject( fileObject );
                    ASSERT(FALSE);
                    return STATUS_INVALID_PARAMETER;
                }
            }
        }
#endif // DBG
    }

    //
    // Get the address of the event object and set the event to the Not-
    // Signaled state, if an one was specified.  Note here too, that if
    // the handle does not refer to an event, then the reference will fail.
    //

    if (ARGUMENT_PRESENT( Event )) {
        status = ObReferenceObjectByHandle( Event,
                                            EVENT_MODIFY_STATE,
                                            ExEventObjectType,
                                            requestorMode,
                                            (PVOID *) &eventObject,
                                            NULL );
        if (!NT_SUCCESS( status )) {
            ObDereferenceObject( fileObject );
            if (capturedArray != NULL) {
                ExFreePool( capturedArray );
            }
            return status;
        } else {
            KeClearEvent( eventObject );
        }
    }

    //
    // Get the address of the driver object's Fast I/O dispatch structure.
    //

    fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

    //
    // Make a special check here to determine whether this is a synchronous
    // I/O operation.  If it is, then wait here until the file is owned by
    // the current thread.
    //

    if (fileObject->Flags & FO_SYNCHRONOUS_IO) {

        BOOLEAN interrupted;

        if (!IopAcquireFastLock( fileObject )) {
            status = IopAcquireFileObjectLock( fileObject,
                                               requestorMode,
                                               (BOOLEAN) ((fileObject->Flags & FO_ALERTABLE_IO) != 0),
                                               &interrupted );
            if (interrupted) {
                if (eventObject) {
                    ObDereferenceObject( eventObject );
                }
                ObDereferenceObject( fileObject );
                if (capturedArray != NULL) {
                    ExFreePool( capturedArray );
                }
                return status;
            }
        }

        if (!ARGUMENT_PRESENT( ByteOffset ) ||
            (fileOffset.LowPart == FILE_USE_FILE_POINTER_POSITION &&
            fileOffset.HighPart == -1)) {
            fileOffset = fileObject->CurrentByteOffset;
        }

        synchronousIo = TRUE;

    } else if (!ARGUMENT_PRESENT( ByteOffset ) && !(fileObject->Flags & (FO_NAMED_PIPE | FO_MAILSLOT))) {

        //
        // The file is not open for synchronous I/O operations, but the
        // caller did not specify a ByteOffset parameter.
        //

        if (eventObject) {
            ObDereferenceObject( eventObject );
        }
        ObDereferenceObject( fileObject );
        if (capturedArray != NULL) {
            ExFreePool( capturedArray );
        }
        return STATUS_INVALID_PARAMETER;
    } else {
        synchronousIo = FALSE;
    }

    //
    //  Negative file offsets are illegal.
    //

    if (fileOffset.HighPart < 0) {
        if (eventObject) {
            ObDereferenceObject( eventObject );
        }
        if (synchronousIo) {
            IopReleaseFileObjectLock( fileObject );
        }
        ObDereferenceObject( fileObject );
        if (capturedArray != NULL) {
            ExFreePool( capturedArray );
        }
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Set the file object to the Not-Signaled state.
    //

    KeClearEvent( &fileObject->Event );

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case the
    // caller does not have enough quota to allocate the packet.

    irp = IopAllocateIrp( deviceObject->StackSize, TRUE );
    if (!irp) {

        //
        // An IRP could not be allocated.  Cleanup and return an appropriate
        // error status code.
        //

        IopAllocateIrpCleanup( fileObject, eventObject );

        if (capturedArray != NULL) {
            ExFreePool( capturedArray );
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.AuxiliaryBuffer = (PVOID) NULL;
    irp->RequestorMode = requestorMode;
    irp->PendingReturned = FALSE;
    irp->Cancel = FALSE;
    irp->CancelRoutine = (PDRIVER_CANCEL) NULL;

    //
    // Fill in the service independent parameters in the IRP.
    //

    irp->UserEvent = eventObject;
    irp->UserIosb = IoStatusBlock;
    irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.  Note that
    // setting the major function here also sets:
    //
    //      MinorFunction = 0;
    //      Flags = 0;
    //      Control = 0;
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    majorFunction = (PULONG) (&irpSp->MajorFunction);
    *majorFunction = IRP_MJ_READ;
    irpSp->FileObject = fileObject;

    //
    // Always allocate a Memory Descriptor List (MDL) and lock down the
    // caller's buffer. This way the file system do not have change to
    // build a scatter MDL. Note buffered I/O is not supported for this
    // routine.
    //

    irp->AssociatedIrp.SystemBuffer = (PVOID) NULL;
    irp->MdlAddress = (PMDL) NULL;


    //
    // This is a direct I/O operation.  Allocate an MDL and invoke the
    // memory management routine to lock the buffer into memory.  This
    // is done using an exception handler that will perform cleanup if
    // the operation fails.  Note that no MDL is allocated, nor is any
    // memory probed or locked if the length of the request was zero.
    //

    irp->Flags = 0;

    if (Length) {

        PMDL mdl;

        try {

            //
            // Allocate an MDL, charging quota for it, and hang it off of
            // the IRP.  Probe and lock the pages associated with the
            // caller's buffer for write access and fill in the MDL with
            // the PFNs of those pages.
            //

            mdl = IoAllocateMdl( (PVOID)(ULONG_PTR) SegmentArray[0].Buffer, Length, FALSE, TRUE, irp );
            if (mdl == NULL) {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            //
            // The address of the first file segment is used as a base
            // address.
            //

            MmProbeAndLockSelectedPages( mdl,
                                         SegmentArray,
                                         requestorMode,
                                         IoWriteAccess );

            irp->UserBuffer = (PVOID)(ULONG_PTR) SegmentArray[0].Buffer;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred while either probing the caller's
            // buffer or allocating the MDL.  Determine what actually
            // happened, clean everything up, and return an appropriate
            // error status code.
            //

            IopExceptionCleanup( fileObject,
                                 irp,
                                 eventObject,
                                 (PKEVENT) NULL );

            if (capturedArray != NULL) {
                ExFreePool( capturedArray );
            }
            return GetExceptionCode();

        }

    }

    //
    // We are done with the captured buffer.
    //

    if (capturedArray != NULL) {
        ExFreePool( capturedArray );
    }

    //
    // If this read operation is supposed to be performed with caching disabled
    // set the disable flag in the IRP so no caching is performed.
    //

    if (fileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {
        irp->Flags |= IRP_NOCACHE | IRP_READ_OPERATION | IRP_DEFER_IO_COMPLETION;
    } else {
        irp->Flags |= IRP_READ_OPERATION | IRP_DEFER_IO_COMPLETION;
    }

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.Read.Length = Length;
    irpSp->Parameters.Read.Key = keyValue;
    irpSp->Parameters.Read.ByteOffset = fileOffset;

    //
    // Queue the packet, call the driver, and synchronize appopriately with
    // I/O completion.
    //

    status =  IopSynchronousServiceTail( deviceObject,
                                         irp,
                                         fileObject,
                                         TRUE,
                                         requestorMode,
                                         synchronousIo,
                                         ReadTransfer );

    return status;

}
