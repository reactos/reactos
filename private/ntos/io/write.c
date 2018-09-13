/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    write.c

Abstract:

    This module contains the code to implement the NtWriteFile system service.

Author:

    Darryl E. Havens (darrylh) 14-Apr-1989

Environment:

    Kernel mode

Revision History:


--*/

#include "iop.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtWriteFile)
#pragma alloc_text(PAGE, NtWriteFile64)
#pragma alloc_text(PAGE, NtWriteFileGather)
#endif

NTSTATUS
NtWriteFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    )

/*++

Routine Description:

    This service writes Length bytes of data from the caller's Buffer to the
    file associated with FileHandle starting at StartingBlock|ByteOffset.
    The actual number of bytes written to the file will be returned in the
    second longword of the IoStatusBlock.

    If the writer has the file open for APPEND access, then the data will be
    written to the current EOF mark.  The StartingBlock and ByteOffset are
    ignored if the caller has APPEND access.

Arguments:

    FileHandle - Supplies a handle to the file to be written.

    Event - Optionally supplies an event to be set to the Signaled state when
        the write operation is complete.

    ApcRoutine - Optionally supplies an APC routine to be executed when the
        write operation is complete.

    ApcContext - Supplies a context parameter to be passed to the APC routine
        when it is invoked, if an APC routine was specified.

    IoStatusBlock - Supplies the address of the caller's I/O status block.

    Buffer - Supplies the address of the buffer containing data to be written
        to the file.

    Length - Length, in bytes, of the data to be written to the file.

    ByteOffset - Specifies the starting byte offset within the file to begin
        the write operation.  If not specified and the file is open for
        synchronous I/O, then the current file position is used.  If the
        file is not opened for synchronous I/O and the parameter is not
        specified, then it is in error.

    Key - Optionally specifies a key to be used if there are locks associated
        with the file.

Return Value:

    The status returned is success if the write operation was properly queued
    to the I/O system.  Once the write completes the status of the operation
    can be determined by examining the Status field of the I/O status block.

--*/

{
    PIRP irp;
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    KPROCESSOR_MODE requestorMode;
    PMDL mdl;
    PIO_STACK_LOCATION irpSp;
    ACCESS_MASK grantedAccess;
    OBJECT_HANDLE_INFORMATION handleInformation;
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
    // Reference the file object so the target device can be found and the
    // access rights mask can be used in the following checks for callers in
    // user mode.  Note that if the handle does not refer to a file object,
    // then it will fail.
    //

    status = ObReferenceObjectByHandle( FileHandle,
                                        0L,
                                        IoFileObjectType,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        &handleInformation);
    if (!NT_SUCCESS( status )) {
        return status;
    }

    grantedAccess = handleInformation.GrantedAccess;

    //
    // Get the address of the target device object.
    //

    deviceObject = IoGetRelatedDeviceObject( fileObject );

    //
    // Check to see if the requestor mode was user.  If so, perform a bunch
    // of extra checks.
    //

    if (requestorMode != KernelMode) {

        //
        // The caller's access mode is not kernel so probe each of the arguments
        // and capture them as necessary.  If any failures occur, the condition
        // handler will be invoked to handle them.  It will simply cleanup and
        // return an access violation status code back to the system service
        // dispatcher.
        //

        //
        // Check to ensure that the caller has either WRITE_DATA or APPEND_DATA
        // access to the file.  If not, cleanup and return an access denied
        // error status value.  Note that if this is a pipe then the APPEND_DATA
        // access check may not be made since this access code is overlaid with
        // CREATE_PIPE_INSTANCE access.
        //

        if (!SeComputeGrantedAccesses( grantedAccess, (!(fileObject->Flags & FO_NAMED_PIPE) ? FILE_APPEND_DATA : 0) | FILE_WRITE_DATA )) {
            ObDereferenceObject( fileObject );
            return STATUS_ACCESS_DENIED;
        }

        //
        // Attempt to probe the caller's parameters within the exception
        // handler block.
        //

        try {

            //
            // The IoStatusBlock parameter must be writeable by the caller.
            //

            ProbeForWriteIoStatusEx( IoStatusBlock , ApcRoutine);

            //
            // The caller's data buffer must be readable from the caller's
            // mode.  This check ensures that this is the case.  Since the
            // buffer address is captured, the caller cannot change it,
            // even though he/she can change the protection from another
            // thread.  This error will be caught by the probe/lock or
            // buffer copy operations later.
            //

            ProbeForRead( Buffer, Length, sizeof( UCHAR ) );

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
            // Check that the ByteOffset parameter is readable from the
            // caller's mode, if one was specified, and capture it.
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
                        return STATUS_INVALID_PARAMETER;
                    }
                }

                //
                // If a ByteOffset parameter was specified, ensure that it is
                // is of the proper type.
                //

                if (ARGUMENT_PRESENT( ByteOffset )) {
                    if (fileOffset.LowPart == FILE_WRITE_TO_END_OF_FILE &&
                        fileOffset.HighPart == -1) {
                        NOTHING;
                    } else if (fileOffset.LowPart == FILE_USE_FILE_POINTER_POSITION &&
                               fileOffset.HighPart == -1 &&
                               (fileObject->Flags & FO_SYNCHRONOUS_IO)) {
                        NOTHING;
                    } else if (deviceObject->SectorSize &&
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
            // caller's parameters.  Simply cleanup, dereference the file
            // object, and return with the appropriate status code.
            //

            ObDereferenceObject( fileObject );
            return exceptionCode;

        }

    } else {

        //
        // The caller's mode is kernel.  Get the appropriate parameters to
        // their expected locations without making all of the checks.
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
            // If a ByteOffset parameter was specified, ensure that it is
            // is of the proper type.
            //

            if (ARGUMENT_PRESENT( ByteOffset )) {
                if (fileOffset.LowPart == FILE_WRITE_TO_END_OF_FILE &&
                    fileOffset.HighPart == -1) {
                    NOTHING;
                } else if (fileOffset.LowPart == FILE_USE_FILE_POINTER_POSITION &&
                           fileOffset.HighPart == -1 &&
                           (fileObject->Flags & FO_SYNCHRONOUS_IO)) {
                    NOTHING;
                } else if (deviceObject->SectorSize &&
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
    // If the caller has only append access to the file, ignore the input
    // parameters and set the ByteOffset to indicate that this write is
    // to the end of the file.  Otherwise, ensure that the parameters are
    // valid.
    //

    if (SeComputeGrantedAccesses( grantedAccess, FILE_APPEND_DATA | FILE_WRITE_DATA ) == FILE_APPEND_DATA) {

        //
        // This is an append operation to the end of a file.  Set the
        // ByteOffset parameter to give drivers a consistent view of
        // this type of call.
        //

        fileOffset.LowPart = FILE_WRITE_TO_END_OF_FILE;
        fileOffset.HighPart = -1;
    }

    //
    // Get the address of the event object and set the event to the Not-
    // Signaled state, if an event was specified.  Note here too, that if
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
    // Get the address of the fast io dispatch structure.
    //

    fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

    //
    // Make a special check here to determine whether this is a synchronous
    // I/O operation.  If it is, then wait here until the file is owned by
    // the current thread.  If the wait terminates with an alerted status,
    // then cleanup and return the alerted status.  This allows the caller
    // specify FILE_SYNCHRONOUS_IO_ALERT as a synchronous I/O option.
    //
    // If everything works, then check to see whether a ByteOffset parameter
    // was supplied.  If not, or if it was and it is set to the "use file
    // pointer position", then initialize the file offset to be whatever
    // the current byte offset into the file is according to the file pointer
    // context information in the file object.
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

        synchronousIo = TRUE;

        if ((!ARGUMENT_PRESENT( ByteOffset ) && !fileOffset.LowPart ) ||
            (fileOffset.LowPart == FILE_USE_FILE_POINTER_POSITION &&
            fileOffset.HighPart == -1 )) {
            fileOffset = fileObject->CurrentByteOffset;
        }

        //
        // Turbo write support.  If the file is currently cached on this
        // file object, then call the Cache Manager directly via FsRtl
        // and try to successfully complete the request here.  Note if
        // FastIoWrite returns FALSE or we get an I/O error, we simply
        // fall through and go the "long way" and create an Irp.
        //

        if (fileObject->PrivateCacheMap) {

            IO_STATUS_BLOCK localIoStatus;

            ASSERT(fastIoDispatch && fastIoDispatch->FastIoWrite);

            //
            //  Negative file offsets are illegal.
            //

            if (fileOffset.HighPart < 0 &&
                (fileOffset.HighPart != -1 ||
                fileOffset.LowPart != FILE_WRITE_TO_END_OF_FILE)) {

                if (eventObject) {
                    ObDereferenceObject( eventObject );
                }
                IopReleaseFileObjectLock( fileObject );
                ObDereferenceObject( fileObject );
                return STATUS_INVALID_PARAMETER;
            }

            if (fastIoDispatch->FastIoWrite( fileObject,
                                             &fileOffset,
                                             Length,
                                             TRUE,
                                             keyValue,
                                             Buffer,
                                             &localIoStatus,
                                             deviceObject )

                    &&

                (localIoStatus.Status == STATUS_SUCCESS)) {

                IopUpdateWriteOperationCount( );
                IopUpdateWriteTransferCount( (ULONG)localIoStatus.Information );

                //
                // Carefully return the I/O status.

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

    } else if (!ARGUMENT_PRESENT( ByteOffset ) && !(fileObject->Flags & (FO_NAMED_PIPE | FO_MAILSLOT))) {

        //
        // The file is not open for synchronous I/O operations, but the
        // caller did not specify a ByteOffset parameter.  This is an error
        // situation, so cleanup and return with the appropriate status.
        //

        if (eventObject) {
            ObDereferenceObject( eventObject );
        }
        ObDereferenceObject( fileObject );
        return STATUS_INVALID_PARAMETER;

    } else {

        //
        // This is not a synchronous I/O operation.
        //

        synchronousIo = FALSE;
    }

    //
    //  Negative file offsets are illegal.
    //

    if (fileOffset.HighPart < 0 &&
        (fileOffset.HighPart != -1 ||
        fileOffset.LowPart != FILE_WRITE_TO_END_OF_FILE)) {

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
    //

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
    // setting the major function code here also sets:
    //
    //      MinorFunction = 0;
    //      Flags = 0;
    //      Control = 0;
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    majorFunction = (PULONG) irpSp;
    *majorFunction = IRP_MJ_WRITE;
    irpSp->FileObject = fileObject;
    if (fileObject->Flags & FO_WRITE_THROUGH) {
        irpSp->Flags = SL_WRITE_THROUGH;
    }

    //
    // Now determine whether this device expects to have data buffered to it
    // or whether it performs direct I/O.  This is based on the DO_BUFFERED_IO
    // flag in the device object.  If the flag is set, then a system buffer is
    // allocated and the caller's data is copied into it.  Otherwise, a Memory
    // Descriptor List (MDL) is allocated and the caller's buffer is locked
    // down using it.
    //

    irp->AssociatedIrp.SystemBuffer = (PVOID) NULL;
    irp->MdlAddress = (PMDL) NULL;

    if (deviceObject->Flags & DO_BUFFERED_IO) {

        //
        // The device does not support direct I/O.  Allocate a system buffer,
        // and copy the caller's data into it.  This is done using an
        // exception handler that will perform cleanup if the operation
        // fails.  Note that this is only done if the operation has a non-zero
        // length.
        //

        if (Length) {

            try {

                //
                // Allocate the intermediary system buffer from nonpaged pool,
                // charge quota for it, and copy the caller's data into it.
                //

                irp->AssociatedIrp.SystemBuffer =
                    ExAllocatePoolWithQuota( NonPagedPoolCacheAligned, Length );
                RtlCopyMemory( irp->AssociatedIrp.SystemBuffer, Buffer, Length );

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception was incurred while either probing the caller's
                // buffer, allocating the system buffer, or copying the data
                // from the caller's buffer to the system buffer.  Determine
                // what actually happened, clean everything up, and return an
                // appropriate error status code.
                //

                IopExceptionCleanup( fileObject,
                                     irp,
                                     eventObject,
                                     (PKEVENT) NULL );

                return GetExceptionCode();

            }

            //
            // Set the IRP_BUFFERED_IO flag in the IRP so that I/O completion
            // will know that this is not a direct I/O operation.  Also set the
            // IRP_DEALLOCATE_BUFFER flag so it will deallocate the buffer.
            //

            irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;

        } else {

            //
            // This is a zero-length write.  Simply indicate that this is
            // buffered I/O, and pass along the request.  The buffer will
            // not be set to deallocate so the completion path does not
            // have to special-case the length.
            //

            irp->Flags = IRP_BUFFERED_IO;
        }

    } else if (deviceObject->Flags & DO_DIRECT_IO) {

        //
        // This is a direct I/O operation.  Allocate an MDL and invoke the
        // memory management routine to lock the buffer into memory.  This
        // is done using an exception handler that will perform cleanup if
        // the operation fails.  Note that no MDL is allocated, nor is any
        // memory probed or locked if the length of the request was zero.
        //

        mdl = (PMDL) NULL;
        irp->Flags = 0;

        if (Length) {

            try {

                //
                // Allocate an MDL, charging quota for it, and hang it off of
                // the IRP.  Probe and lock the pages associated with the
                // caller's buffer for read access and fill in the MDL with
                // the PFNs of those pages.
                //

                mdl = IoAllocateMdl( Buffer, Length, FALSE, TRUE, irp );
                if (mdl == NULL) {
                    ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                }

                MmProbeAndLockPages( mdl, requestorMode, IoReadAccess );

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception was incurred while either allocating the MDL
                // or while attempting to probe and lock the caller's buffer.
                // Determine what actually happened, clean everything up, and
                // return an appropriate error status code.
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
        // Pass the address of the caller's buffer to the device driver.  It
        // is now up to the driver to do everything.
        //

        irp->Flags = 0;
        irp->UserBuffer = Buffer;

    }

    //
    // If this write operation is to be performed without any caching, set the
    // appropriate flag in the IRP so no caching is performed.
    //

    if (fileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {
        irp->Flags |= IRP_NOCACHE | IRP_WRITE_OPERATION | IRP_DEFER_IO_COMPLETION;
    } else {
        irp->Flags |= IRP_WRITE_OPERATION | IRP_DEFER_IO_COMPLETION;
    }

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.Write.Length = Length;
    irpSp->Parameters.Write.Key = keyValue;
    irpSp->Parameters.Write.ByteOffset = fileOffset;

    //
    // Queue the packet, call the driver, and synchronize appopriately with
    // I/O completion.
    //

    status = IopSynchronousServiceTail( deviceObject,
                                        irp,
                                        fileObject,
                                        TRUE,
                                        requestorMode,
                                        synchronousIo,
                                        WriteTransfer );

    return status;
}

NTSTATUS
NtWriteFileGather(
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

    This service writes Length bytes of data from the caller's segment
    buffers to the file associated with FileHandle starting at
    StartingBlock|ByteOffset. The actual number of bytes written to the file
    will be returned in the second longword of the IoStatusBlock.

    If the writer has the file open for APPEND access, then the data will be
    written to the current EOF mark.  The StartingBlock and ByteOffset are
    ignored if the caller has APPEND access.

Arguments:

    FileHandle - Supplies a handle to the file to be written.

    Event - Optionally supplies an event to be set to the Signaled state when
        the write operation is complete.

    ApcRoutine - Optionally supplies an APC routine to be executed when the
        write operation is complete.

    ApcContext - Supplies a context parameter to be passed to the APC routine
        when it is invoked, if an APC routine was specified.

    IoStatusBlock - Supplies the address of the caller's I/O status block.

    SegmentArray - An array of buffer segment pointers that specify
        where the data should be read from.

    Length - Length, in bytes, of the data to be written to the file.

    ByteOffset - Specifies the starting byte offset within the file to begin
        the write operation.  If not specified and the file is open for
        synchronous I/O, then the current file position is used.  If the
        file is not opened for synchronous I/O and the parameter is not
        specified, then it is in error.

    Key - Optionally specifies a key to be used if there are locks associated
        with the file.

Return Value:

    The status returned is success if the write operation was properly queued
    to the I/O system.  Once the write completes the status of the operation
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
    PMDL mdl;
    PIO_STACK_LOCATION irpSp;
    ACCESS_MASK grantedAccess;
    OBJECT_HANDLE_INFORMATION handleInformation;
    NTSTATUS exceptionCode;
    PKEVENT eventObject = (PKEVENT) NULL;
    ULONG elementCount;
    ULONG keyValue = 0;
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
    // Reference the file object so the target device can be found and the
    // access rights mask can be used in the following checks for callers in
    // user mode.  Note that if the handle does not refer to a file object,
    // then it will fail.
    //

    status = ObReferenceObjectByHandle( FileHandle,
                                        0L,
                                        IoFileObjectType,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        &handleInformation);
    if (!NT_SUCCESS( status )) {
        return status;
    }

    grantedAccess = handleInformation.GrantedAccess;

    //
    // Get the address of the target device object.
    //

    deviceObject = IoGetRelatedDeviceObject( fileObject );

    //
    // Verify this is a valid gather write request.  In particular it must
    // be non cached, asynchronous, use completion ports, non buffer I/O
    // device and directed at a file system device.
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
         deviceObject->DeviceType != FILE_DEVICE_DFS_VOLUME)) {

        ObDereferenceObject( fileObject );
        return STATUS_INVALID_PARAMETER;
    }

    elementCount = BYTES_TO_PAGES( Length );

    //
    // Check to see if the requestor mode was user.  If so, perform a bunch
    // of extra checks.
    //

    if (requestorMode != KernelMode) {

        //
        // The caller's access mode is not kernel so probe each of the arguments
        // and capture them as necessary.  If any failures occur, the condition
        // handler will be invoked to handle them.  It will simply cleanup and
        // return an access violation status code back to the system service
        // dispatcher.
        //

        //
        // Check to ensure that the caller has either WRITE_DATA or APPEND_DATA
        // access to the file.  If not, cleanup and return an access denied
        // error status value.  Note that if this is a pipe then the APPEND_DATA
        // access check may not be made since this access code is overlaid with
        // CREATE_PIPE_INSTANCE access.
        //

        if (!SeComputeGrantedAccesses( grantedAccess, (!(fileObject->Flags & FO_NAMED_PIPE) ? FILE_APPEND_DATA : 0) | FILE_WRITE_DATA )) {
            ObDereferenceObject( fileObject );
            return STATUS_ACCESS_DENIED;
        }

        //
        // Attempt to probe the caller's parameters within the exception
        // handler block.
        //

        try {

            //
            // The IoStatusBlock parameter must be writeable by the caller.
            //

            ProbeForWriteIoStatusEx( IoStatusBlock , ApcRoutine);

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
                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }
                }
            }

            //
            // If this file has an I/O completion port associated w/it, then
            // ensure that the caller did not supply an APC routine, as the
            // two are mutually exclusive methods for I/O completion
            // notification.
            //

            if (fileObject->CompletionContext && IopApcRoutinePresent( ApcRoutine )) {

                ExRaiseStatus(STATUS_INVALID_PARAMETER);

            }

            //
            // Check that the ByteOffset parameter is readable from the
            // caller's mode, if one was specified, and capture it.
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
                        Length % deviceObject->SectorSize) ) {

                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }
                }

                //
                // If a ByteOffset parameter was specified, ensure that it is
                // is of the proper type.
                //

                if (ARGUMENT_PRESENT( ByteOffset )) {
                    if (fileOffset.LowPart == FILE_WRITE_TO_END_OF_FILE &&
                        fileOffset.HighPart == -1) {
                        NOTHING;
                    } else if (fileOffset.LowPart == FILE_USE_FILE_POINTER_POSITION &&
                               fileOffset.HighPart == -1 &&
                               (fileObject->Flags & FO_SYNCHRONOUS_IO)) {
                        NOTHING;
                    } else if (deviceObject->SectorSize &&
                        (fileOffset.LowPart & (deviceObject->SectorSize - 1))) {

                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
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
            // caller's parameters.  Simply cleanup, dereference the file
            // object, and return with the appropriate status code.
            //

            ObDereferenceObject( fileObject );

            if (capturedArray != NULL) {
                ExFreePool( capturedArray );
            }

            return exceptionCode;

        }

    } else {

        //
        // The caller's mode is kernel.  Get the appropriate parameters to
        // their expected locations without making all of the checks.
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
            // If a ByteOffset parameter was specified, ensure that it is
            // is of the proper type.
            //

            if (ARGUMENT_PRESENT( ByteOffset )) {
                if (fileOffset.LowPart == FILE_WRITE_TO_END_OF_FILE &&
                    fileOffset.HighPart == -1) {
                    NOTHING;
                } else if (fileOffset.LowPart == FILE_USE_FILE_POINTER_POSITION &&
                           fileOffset.HighPart == -1 &&
                           (fileObject->Flags & FO_SYNCHRONOUS_IO)) {
                    NOTHING;
                } else if (deviceObject->SectorSize &&
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
    // If the caller has only append access to the file, ignore the input
    // parameters and set the ByteOffset to indicate that this write is
    // to the end of the file.  Otherwise, ensure that the parameters are
    // valid.
    //

    if (SeComputeGrantedAccesses( grantedAccess, FILE_APPEND_DATA | FILE_WRITE_DATA ) == FILE_APPEND_DATA) {

        //
        // This is an append operation to the end of a file.  Set the
        // ByteOffset parameter to give drivers a consistent view of
        // this type of call.
        //

        fileOffset.LowPart = FILE_WRITE_TO_END_OF_FILE;
        fileOffset.HighPart = -1;
    }

    //
    // Get the address of the event object and set the event to the Not-
    // Signaled state, if an event was specified.  Note here too, that if
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
    // Get the address of the fast io dispatch structure.
    //

    fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

    //
    // Make a special check here to determine whether this is a synchronous
    // I/O operation.  If it is, then wait here until the file is owned by
    // the current thread.  If the wait terminates with an alerted status,
    // then cleanup and return the alerted status.  This allows the caller
    // specify FILE_SYNCHRONOUS_IO_ALERT as a synchronous I/O option.
    //
    // If everything works, then check to see whether a ByteOffset parameter
    // was supplied.  If not, or if it was and it is set to the "use file
    // pointer position", then initialize the file offset to be whatever
    // the current byte offset into the file is according to the file pointer
    // context information in the file object.
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

        synchronousIo = TRUE;

        if ((!ARGUMENT_PRESENT( ByteOffset ) && !fileOffset.LowPart ) ||
            (fileOffset.LowPart == FILE_USE_FILE_POINTER_POSITION &&
            fileOffset.HighPart == -1 )) {
            fileOffset = fileObject->CurrentByteOffset;
        }

    } else if (!ARGUMENT_PRESENT( ByteOffset ) && !(fileObject->Flags & (FO_NAMED_PIPE | FO_MAILSLOT))) {

        //
        // The file is not open for synchronous I/O operations, but the
        // caller did not specify a ByteOffset parameter.  This is an error
        // situation, so cleanup and return with the appropriate status.
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

        //
        // This is not a synchronous I/O operation.
        //

        synchronousIo = FALSE;
    }

    //
    //  Negative file offsets are illegal.
    //

    if (fileOffset.HighPart < 0 &&
        (fileOffset.HighPart != -1 ||
        fileOffset.LowPart != FILE_WRITE_TO_END_OF_FILE)) {

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
    //

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
    // setting the major function code here also sets:
    //
    //      MinorFunction = 0;
    //      Flags = 0;
    //      Control = 0;
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    majorFunction = (PULONG) irpSp;
    *majorFunction = IRP_MJ_WRITE;
    irpSp->FileObject = fileObject;
    if (fileObject->Flags & FO_WRITE_THROUGH) {
        irpSp->Flags = SL_WRITE_THROUGH;
    }

    //
    // Now determine whether this device expects to have data buffered to it
    // or whether it performs direct I/O.  This is based on the DO_BUFFERED_IO
    // flag in the device object.  If the flag is set, then a system buffer is
    // allocated and the caller's data is copied into it.  Otherwise, a Memory
    // Descriptor List (MDL) is allocated and the caller's buffer is locked
    // down using it.
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

    mdl = (PMDL) NULL;
    irp->Flags = 0;

    if (Length) {

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
                                         IoReadAccess );

            irp->UserBuffer = (PVOID)(ULONG_PTR) SegmentArray[0].Buffer;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred while either allocating the MDL
            // or while attempting to probe and lock the caller's buffer.
            // Determine what actually happened, clean everything up, and
            // return an appropriate error status code.
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
    // If this write operation is to be performed without any caching, set the
    // appropriate flag in the IRP so no caching is performed.
    //

    if (fileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {
        irp->Flags |= IRP_NOCACHE | IRP_WRITE_OPERATION | IRP_DEFER_IO_COMPLETION;
    } else {
        irp->Flags |= IRP_WRITE_OPERATION | IRP_DEFER_IO_COMPLETION;
    }

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.Write.Length = Length;
    irpSp->Parameters.Write.Key = keyValue;
    irpSp->Parameters.Write.ByteOffset = fileOffset;

    //
    // Queue the packet, call the driver, and synchronize appopriately with
    // I/O completion.
    //

    status = IopSynchronousServiceTail( deviceObject,
                                        irp,
                                        fileObject,
                                        TRUE,
                                        requestorMode,
                                        synchronousIo,
                                        WriteTransfer );

    return status;

}
