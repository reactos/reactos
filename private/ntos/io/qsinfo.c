/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    qsinfo.c

Abstract:

    This module contains the code to implement the NtQueryInformationFile and
    NtSetInformationFile system services for the NT I/O system.

Author:

    Darryl E. Havens (darrylh) 6-Jun-1989

Environment:

    Kernel mode only

Revision History:


--*/

#include "iop.h"

//
// Create local definitions for long flag names to make code slightly more
// readable.
//

#define FSIO_A  FILE_SYNCHRONOUS_IO_ALERT
#define FSIO_NA FILE_SYNCHRONOUS_IO_NONALERT

//
// Forward declarations of local routines.
//

ULONG
IopGetModeInformation(
    IN PFILE_OBJECT FileObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopGetModeInformation)
#pragma alloc_text(PAGE, NtQueryInformationFile)
#pragma alloc_text(PAGE, NtSetInformationFile)
#endif


ULONG
IopGetModeInformation(
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This encapsulates extracting and translating the mode bits from
    the passed file object, to be returned from a query information call.

Arguments:

    FileObject - Specifies the file object for which to return Mode info.

Return Value:

    The translated mode information is returned.

--*/

{
    ULONG mode = 0;

    if (FileObject->Flags & FO_WRITE_THROUGH) {
        mode = FILE_WRITE_THROUGH;
    }
    if (FileObject->Flags & FO_SEQUENTIAL_ONLY) {
        mode |= FILE_SEQUENTIAL_ONLY;
    }
    if (FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {
        mode |= FILE_NO_INTERMEDIATE_BUFFERING;
    }
    if (FileObject->Flags & FO_SYNCHRONOUS_IO) {
        if (FileObject->Flags & FO_ALERTABLE_IO) {
            mode |= FILE_SYNCHRONOUS_IO_ALERT;
        } else {
            mode |= FILE_SYNCHRONOUS_IO_NONALERT;
        }
    }
    if (FileObject->Flags & FO_DELETE_ON_CLOSE) {
        mode |= FILE_DELETE_ON_CLOSE;
    }
    return mode;
}

NTSTATUS
NtQueryInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )

/*++

Routine Description:

    This service returns the requested information about a specified file.
    The information returned is determined by the FileInformationClass that
    is specified, and it is placed into the caller's FileInformation buffer.

Arguments:

    FileHandle - Supplies a handle to the file about which the requested
        information should be returned.

    IoStatusBlock - Address of the caller's I/O status block.

    FileInformation - Supplies a buffer to receive the requested information
        returned about the file.

    Length - Supplies the length, in bytes, of the FileInformation buffer.

    FileInformationClass - Specifies the type of information which should be
        returned about the file.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    PIRP irp;
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PKEVENT event = (PKEVENT) NULL;
    KPROCESSOR_MODE requestorMode;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK localIoStatus;
    OBJECT_HANDLE_INFORMATION handleInformation;
    BOOLEAN synchronousIo;
    BOOLEAN skipDriver;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {

        //
        // Ensure that the FileInformationClass parameter is legal for querying
        // information about the file.
        //

        if ((ULONG) FileInformationClass >= FileMaximumInformation ||
            !IopQueryOperationLength[FileInformationClass]) {
            return STATUS_INVALID_INFO_CLASS;
        }

        //
        // Ensure that the supplied buffer is large enough to contain the
        // information associated with the specified set operation that is
        // to be performed.
        //

        if (Length < (ULONG) IopQueryOperationLength[FileInformationClass]) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

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

            //
            // The FileInformation buffer must be writeable by the caller.
            //

#if defined(_X86_)
            ProbeForWrite( FileInformation, Length, sizeof( ULONG ) );
#elif defined(_WIN64)

            //
            // If we are a wow64 process, follow the X86 rules
            //

            if (PsGetCurrentProcess()->Wow64Process) {
                ProbeForWrite( FileInformation, Length, sizeof( ULONG ) );
            } else {
                ProbeForWrite( FileInformation,
                               Length,
                               IopQuerySetAlignmentRequirement[FileInformationClass] );
            }
#else
            ProbeForWrite( FileInformation,
                           Length,
                           IopQuerySetAlignmentRequirement[FileInformationClass] );
#endif

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred while probing the caller's
            // parameters.  Simply return an appropriate error status
            // code.
            //

#if DBG
            if (GetExceptionCode() == STATUS_DATATYPE_MISALIGNMENT) {
                DbgBreakPoint();
            }
#endif // DBG

            return GetExceptionCode();
        }

#if DBG

    } else {

        //
        // The caller's mode is kernel.  Ensure that at least the information
        // class and lengths are appropriate.
        //

        if ((ULONG) FileInformationClass >= FileMaximumInformation ||
            !IopQueryOperationLength[FileInformationClass]) {
            return STATUS_INVALID_INFO_CLASS;
        }

        if (Length < (ULONG) IopQueryOperationLength[FileInformationClass]) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

#endif // DBG

    }

    //
    // There were no blatant errors so far, so reference the file object so
    // the target device object can be found.  Note that if the handle does
    // not refer to a file object, or if the caller does not have the required
    // access to the file, then it will fail.
    //

    status = ObReferenceObjectByHandle( FileHandle,
                                        IopQueryOperationAccess[FileInformationClass],
                                        IoFileObjectType,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        &handleInformation);

    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // Get the address of the target device object.  If this file represents
    // a device that was opened directly, then simply use the device or its
    // attached device(s) directly.  Also get the address of the Fast Io
    // dispatch structure.
    //

    if (!(fileObject->Flags & FO_DIRECT_DEVICE_OPEN)) {
        deviceObject = IoGetRelatedDeviceObject( fileObject );
    } else {
        deviceObject = IoGetAttachedDevice( fileObject->DeviceObject );
    }
    fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

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

        //
        // Make a special check here to determine whether or not the caller
        // is attempting to query the file position pointer.  If so, then
        // return it immediately and get out.
        //

        if (FileInformationClass == FilePositionInformation) {

            //
            // The caller has requested the current file position context
            // information.  This is a relatively frequent call, so it is
            // optimized here to cut through the normal IRP path.
            //
            // Begin by establishing a condition handler and attempting to
            // return both the file position information as well as the I/O
            // status block.  If writing the output buffer fails, then return
            // an appropriate error status code.  If writing the I/O status
            // block fails, then ignore the error.  This is what would
            // normally happen were everything to go through normal special
            // kernel APC processing.
            //

            BOOLEAN writingBuffer = TRUE;
            PFILE_POSITION_INFORMATION fileInformation = FileInformation;

            try {

                //
                // Return the current position information.
                //

                fileInformation->CurrentByteOffset = fileObject->CurrentByteOffset;
                writingBuffer = FALSE;

                //
                // Write the I/O status block.
                //

                IoStatusBlock->Status = STATUS_SUCCESS;
                IoStatusBlock->Information = sizeof( FILE_POSITION_INFORMATION );

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                //
                // One of writing the caller's buffer or writing the I/O
                // status block failed.  Set the final status appropriately.
                //

                if (writingBuffer) {
                    status = GetExceptionCode();
                }

            }

            //
            // Note that the state of the event in the file object has not yet
            // been reset, so it need not be set either.  Therefore, simply
            // cleanup and return.
            //

            IopReleaseFileObjectLock( fileObject );
            ObDereferenceObject( fileObject );
            return status;

        //
        // Also do a special check if the caller it doing a query for basic or
        // standard information and if so then try the fast query calls if they
        // exist.
        //

        } else if (fastIoDispatch &&
                   (((FileInformationClass == FileBasicInformation) &&
                     fastIoDispatch->FastIoQueryBasicInfo) ||
                    ((FileInformationClass == FileStandardInformation) &&
                     fastIoDispatch->FastIoQueryStandardInfo))) {

            IO_STATUS_BLOCK localIoStatus;
            BOOLEAN queryResult = FALSE;
            BOOLEAN writingStatus = FALSE;

            //
            // Do the query and setting of the IoStatusBlock inside an exception
            // handler.  Note that if an exception occurs, other than writing
            // the status back, then the IRP route will be taken.  If an error
            // occurs attempting to write the status back to the caller's buffer
            // then it will be ignored, just as it would be on the long path.
            //

            try {

                if (FileInformationClass == FileBasicInformation) {
                    queryResult = fastIoDispatch->FastIoQueryBasicInfo( fileObject,
                                                                        TRUE,
                                                                        FileInformation,
                                                                        &localIoStatus,
                                                                        deviceObject );
                } else {
                    queryResult = fastIoDispatch->FastIoQueryStandardInfo( fileObject,
                                                                           TRUE,
                                                                           FileInformation,
                                                                           &localIoStatus,
                                                                           deviceObject );
                }

                if (queryResult) {
                    status = localIoStatus.Status;
                    writingStatus = TRUE;
                    *IoStatusBlock = localIoStatus;
                }

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                //
                // If the result of the preceeding block is an exception that
                // occurred after the Fast I/O path itself, then the query
                // actually succeeded so everything is done already, but the
                // user's I/O status buffer is not writable.  This case is
                // ignored to be consistent w/the long path.
                //

                if (!writingStatus) {
                    status = GetExceptionCode();
                }
            }

            //
            // If the results of the preceeding statement block is true, then
            // the fast query call succeeeded, so simply cleanup and return.
            //

            if (queryResult) {

                //
                // Note that once again, the event in the file object has not
                // yet been set reset, so it need not be set to the Signaled
                // state, so simply cleanup and return.
                //

                IopReleaseFileObjectLock( fileObject );
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
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case the
    // caller does not have enough quota to allocate the packet.
    //

    irp = IoAllocateIrp( deviceObject->StackSize, TRUE );
    if (!irp) {

        //
        // An IRP could not be allocated.  Cleanup and return an appropriate
        // error status code.
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
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_QUERY_INFORMATION;
    irpSp->FileObject = fileObject;

    //
    // Allocate a buffer which should be used to put the information into by
    // the driver.  This will be copied back to the caller's buffer when the
    // service completes.  This is done by setting the flag which says that
    // this is an input operation.
    //

    irp->UserBuffer = FileInformation;
    irp->AssociatedIrp.SystemBuffer = (PVOID) NULL;
    irp->MdlAddress = (PMDL) NULL;

    try {

        //
        // Allocate the system buffer using an exception handler so that
        // errors can be caught and handled.
        //

        irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithQuota( NonPagedPool,
                                                                   Length );
    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception was incurred by attempting to allocate the intermediary
        // system buffer.  Cleanup everything and return an appropriate error
        // status code.
        //

        IopExceptionCleanup( fileObject,
                             irp,
                             (PKEVENT) NULL,
                             event );

        return GetExceptionCode();
    }

    irp->Flags |= IRP_BUFFERED_IO |
                  IRP_DEALLOCATE_BUFFER |
                  IRP_INPUT_OPERATION |
                  IRP_DEFER_IO_COMPLETION;

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.QueryFile.Length = Length;
    irpSp->Parameters.QueryFile.FileInformationClass = FileInformationClass;

    //
    // Insert the packet at the head of the IRP list for the thread.
    //

    IopQueueThreadIrp( irp );

    //
    // Update the operation count statistic for the current process for
    // operations other than read and write.
    //

    IopUpdateOtherOperationCount();

    //
    // Everything is now set to invoke the device driver with this request.
    // However, it is possible that the information that the caller wants
    // is device independent.  If this is the case, then the request can
    // be satisfied here without having to have all of the drivers implement
    // the same code.  Note that having the IRP is still necessary since
    // the I/O completion code requires it.
    //

    skipDriver = FALSE;

    if (FileInformationClass == FileAccessInformation) {

        PFILE_ACCESS_INFORMATION accessBuffer = irp->AssociatedIrp.SystemBuffer;

        //
        // Return the access information for this file.
        //

        accessBuffer->AccessFlags = handleInformation.GrantedAccess;

        //
        // Complete the I/O operation.
        //

        irp->IoStatus.Information = sizeof( FILE_ACCESS_INFORMATION );
        skipDriver = TRUE;

    } else if (FileInformationClass == FileModeInformation) {

        PFILE_MODE_INFORMATION modeBuffer = irp->AssociatedIrp.SystemBuffer;

        //
        // Return the mode information for this file.
        //

        modeBuffer->Mode = IopGetModeInformation( fileObject );

        //
        // Complete the I/O operation.
        //

        irp->IoStatus.Information = sizeof( FILE_MODE_INFORMATION );
        skipDriver = TRUE;

    } else if (FileInformationClass == FileAlignmentInformation) {

        PFILE_ALIGNMENT_INFORMATION alignmentInformation = irp->AssociatedIrp.SystemBuffer;

        //
        // Return the alignment information for this file.
        //

        alignmentInformation->AlignmentRequirement = deviceObject->AlignmentRequirement;

        //
        // Complete the I/O operation.
        //

        irp->IoStatus.Information = sizeof( FILE_ALIGNMENT_INFORMATION );
        skipDriver = TRUE;

    } else if (FileInformationClass == FileAllInformation) {

        PFILE_ALL_INFORMATION allInformation = irp->AssociatedIrp.SystemBuffer;

        //
        // The caller has requested all of the information about the file.
        // This request is handled specially because the service will fill
        // in the Access and Mode and Alignment information in the buffer
        // and then pass the buffer to the driver to fill in the remainder.
        //
        // Begin by returning the Access information for the file.
        //

        allInformation->AccessInformation.AccessFlags =
            handleInformation.GrantedAccess;

        //
        // Return the mode information for this file.
        //

        allInformation->ModeInformation.Mode =
            IopGetModeInformation( fileObject );

        //
        // Return the alignment information for this file.
        //

        allInformation->AlignmentInformation.AlignmentRequirement =
            deviceObject->AlignmentRequirement;

        //
        // Finally, set the information field of the IoStatus block in the IRP
        // to account for the amount information already filled in and invoke
        // the driver to fill in the remainder.
        //

        irp->IoStatus.Information = sizeof( FILE_ACCESS_INFORMATION ) +
                                    sizeof( FILE_MODE_INFORMATION ) +
                                    sizeof( FILE_ALIGNMENT_INFORMATION );
    }

    if (skipDriver) {

        //
        // The requested operation has already been performed.  Simply
        // set the final status in the packet and the return state.
        //

        status = STATUS_SUCCESS;
        irp->IoStatus.Status = STATUS_SUCCESS;

    } else {

        //
        // This is not a request that can be [completely] performed here, so
        // invoke the driver at its appropriate dispatch entry with the IRP.
        //

        status = IoCallDriver( deviceObject, irp );
    }

    //
    // If this operation was a synchronous I/O operation, check the return
    // status to determine whether or not to wait on the file object.  If
    // the file object is to be waited on, wait for the operation to complete
    // and obtain the final status from the file object itself.
    //

    if (status == STATUS_PENDING) {

        if (synchronousIo) {

            status = KeWaitForSingleObject( &fileObject->Event,
                                            Executive,
                                            requestorMode,
                                            (BOOLEAN) ((fileObject->Flags & FO_ALERTABLE_IO) != 0),
                                            (PLARGE_INTEGER) NULL );

            if (status == STATUS_ALERTED || status == STATUS_USER_APC) {

                //
                // The wait request has ended either because the thread was
                // alerted or an APC was queued to this thread, because of
                // thread rundown or CTRL/C processing.  In either case, try
                // to bail out of this I/O request carefully so that the IRP
                // completes before this routine exists so that synchronization
                // with the file object will remain intact.
                //

                IopCancelAlertedRequest( &fileObject->Event, irp );

            }

            status = fileObject->FinalStatus;

            IopReleaseFileObjectLock( fileObject );

        } else {

            //
            // This is a normal synchronous I/O operation, as opposed to a
            // serialized synchronous I/O operation.  For this case, wait for
            // the local event and copy the final status information back to
            // the caller.
            //

            status = KeWaitForSingleObject( event,
                                            Executive,
                                            requestorMode,
                                            FALSE,
                                            (PLARGE_INTEGER) NULL );

            if (status == STATUS_ALERTED || status == STATUS_USER_APC) {

                //
                // The wait request has ended either because the thread was
                // alerted or an APC was queued to this thread, because of
                // thread rundown or CTRL/C processing.  In either case, try
                // to bail out of this I/O request carefully so that the IRP
                // completes before this routine exists or the event will not
                // be around to set to the Signaled state.
                //

                IopCancelAlertedRequest( event, irp );

            }

            status = localIoStatus.Status;

            try {

                *IoStatusBlock = localIoStatus;

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception occurred attempting to write the caller's I/O
                // status block.  Simply change the final status of the operation
                // to the exception code.
                //

                status = GetExceptionCode();
            }

            ExFreePool( event );

        }

    } else {

        //
        // The I/O operation finished without return a status of pending.
        // This means that the operation has not been through I/O completion,
        // so it must be done here.
        //

        PKNORMAL_ROUTINE normalRoutine;
        PVOID normalContext;
        KIRQL irql;

        if (!synchronousIo) {

            //
            // This is not a synchronous I/O operation, it is a synchronous
            // I/O API to a file opened for asynchronous I/O.  Since this
            // code path need never wait on the allocated and supplied event,
            // get rid of it so that it doesn't have to be set to the
            // Signaled state by the I/O completion code.
            //

            irp->UserEvent = (PKEVENT) NULL;
            ExFreePool( event );
        }

        irp->UserIosb = IoStatusBlock;
        KeRaiseIrql( APC_LEVEL, &irql );
        IopCompleteRequest( &irp->Tail.Apc,
                            &normalRoutine,
                            &normalContext,
                            (PVOID *) &fileObject,
                            &normalContext );
        KeLowerIrql( irql );

        if (synchronousIo) {
            IopReleaseFileObjectLock( fileObject );
        }
    }

    return status;
}

NTSTATUS
NtSetInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )

/*++

Routine Description:

    This service changes the provided information about a specified file.  The
    information that is changed is determined by the FileInformationClass that
    is specified.  The new information is taken from the FileInformation buffer.

Arguments:

    FileHandle - Supplies a handle to the file whose information should be
        changed.

    IoStatusBlock - Address of the caller's I/O status block.

    FileInformation - Supplies a buffer containing the information which should
        be changed on the file.

    Length - Supplies the length, in bytes, of the FileInformation buffer.

    FileInformationClass - Specifies the type of information which should be
        changed about the file.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    PIRP irp;
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PKEVENT event = (PKEVENT) NULL;
    KPROCESSOR_MODE requestorMode;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK localIoStatus;
    HANDLE targetHandle = (HANDLE) NULL;
    BOOLEAN synchronousIo;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {

        //
        // Ensure that the FileInformationClass parameter is legal for setting
        // information about the file.
        //

        if ((ULONG) FileInformationClass >= FileMaximumInformation ||
            !IopSetOperationLength[FileInformationClass]) {
            return STATUS_INVALID_INFO_CLASS;
        }

        //
        // Ensure that the supplied buffer is large enough to contain the
        // information associated with the specified set operation that is
        // to be performed.
        //

        if (Length < (ULONG) IopSetOperationLength[FileInformationClass]) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

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

            //
            // The FileInformation buffer must be readable by the caller.
            //

#if defined(_X86_)
            ProbeForRead( FileInformation,
                          Length,
                          Length == sizeof( BOOLEAN ) ? sizeof( BOOLEAN ) : sizeof( ULONG ) );
#elif defined(_WIN64)
            // If we are a wow64 process, follow the X86 rules
            if (PsGetCurrentProcess()->Wow64Process) {
                ProbeForRead( FileInformation,
                              Length,
                              Length == sizeof( BOOLEAN ) ? sizeof( BOOLEAN ) : sizeof( ULONG ) );
            }
            else {
                ProbeForRead( FileInformation,
                              Length,
                              IopQuerySetAlignmentRequirement[FileInformationClass] );
            }
#else
            ProbeForRead( FileInformation,
                          Length,
                          IopQuerySetAlignmentRequirement[FileInformationClass] );
#endif

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred while probing the caller's parameters.
            // Simply return an appropriate error status code.
            //

#if DBG
            if (GetExceptionCode() == STATUS_DATATYPE_MISALIGNMENT) {
                DbgBreakPoint();
            }
#endif // DBG

            return GetExceptionCode();

        }

#if DBG

    } else {

        //
        // The caller's mode is kernel.  Ensure that at least the information
        // class and lengths are appropriate.
        //

        if ((ULONG) FileInformationClass >= FileMaximumInformation ||
            !IopSetOperationLength[FileInformationClass]) {
            return STATUS_INVALID_INFO_CLASS;
        }

        if (Length < (ULONG) IopSetOperationLength[FileInformationClass]) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

#endif // DBG

    }

    //
    // There were no blatant errors so far, so reference the file object so
    // the target device object can be found.  Note that if the handle does
    // not refer to a file object, or if the caller does not have the required
    // access to the file, then it will fail.
    //

    status = ObReferenceObjectByHandle( FileHandle,
                                        IopSetOperationAccess[FileInformationClass],
                                        IoFileObjectType,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        NULL );
    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // Get the address of the target device object.  If this file represents
    // a device that was opened directly, then simply use the device or its
    // attached device(s) directly.
    //

    if (!(fileObject->Flags & FO_DIRECT_DEVICE_OPEN)) {
        deviceObject = IoGetRelatedDeviceObject( fileObject );
    } else {
        deviceObject = IoGetAttachedDevice( fileObject->DeviceObject );
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

        //
        // Make a special check here to determine whether or not the caller
        // is attempting to set the file position pointer information.  If so,
        // then set it immediately and get out.
        //

        if (FileInformationClass == FilePositionInformation) {

            //
            // The caller has requested setting the current file position
            // context information.  This is a relatively frequent call, so
            // it is optimized here to cut through the normal IRP path.
            //
            // Begin by checking to see whether the file was opened with no
            // intermediate buffering.  If so, then the file pointer must be
            // set in a manner consistent with the alignment requirement of
            // read and write operations to a non-buffered file.
            //

            PFILE_POSITION_INFORMATION fileInformation = FileInformation;
            LARGE_INTEGER currentByteOffset;

            try {

                //
                // Attempt to read the position information from the buffer.
                //

                currentByteOffset.QuadPart = fileInformation->CurrentByteOffset.QuadPart;

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                IopReleaseFileObjectLock( fileObject );
                ObDereferenceObject( fileObject );
                return GetExceptionCode();
            }

            if ((fileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING &&
                 (deviceObject->SectorSize &&
                 (currentByteOffset.LowPart &
                 (deviceObject->SectorSize - 1)))) ||
                 currentByteOffset.HighPart < 0) {

                    status = STATUS_INVALID_PARAMETER;

            } else {

                //
                // Set the current file position information.
                //

                fileObject->CurrentByteOffset.QuadPart = currentByteOffset.QuadPart;

                try {

                    //
                    // Write the I/O status block.
                    //

                    IoStatusBlock->Status = STATUS_SUCCESS;
                    IoStatusBlock->Information = 0;

                } except( EXCEPTION_EXECUTE_HANDLER ) {

                    //
                    // Writes to I/O status blocks are ignored since the
                    // operation succeeded.
                    //

                    NOTHING;

                }

            }

            //
            // Update the transfer count statistic for the current process for
            // operations other than read and write.
            //
        
            IopUpdateOtherTransferCount( Length );

            //
            // Note that the file object's event has not yet been reset,
            // so it is not necessary to set it to the Signaled state, since
            // that is it's state at this point by definition.  Therefore,
            // simply cleanup and return.
            //

            IopReleaseFileObjectLock( fileObject );
            ObDereferenceObject( fileObject );
            return status;
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
    // If a link is being tracked, handle this out-of-line.
    //

    if (FileInformationClass == FileTrackingInformation) {
        status = IopTrackLink( fileObject,
                               &localIoStatus,
                               FileInformation,
                               Length,
                               synchronousIo ? &fileObject->Event : event,
                               requestorMode );
        if (NT_SUCCESS( status )) {
            try {
                IoStatusBlock->Information = 0;
                IoStatusBlock->Status = status;
            } except(EXCEPTION_EXECUTE_HANDLER) {
                NOTHING;
            }
        }

        if (synchronousIo) {
            IopReleaseFileObjectLock( fileObject );
        } else {
            ExFreePool( event );
        }
        ObDereferenceObject( fileObject );
        return status;
    }

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case the
    // caller does not have enough quota to allocate the packet.

    irp = IoAllocateIrp( deviceObject->StackSize, TRUE );
    if (!irp) {

        //
        // An IRP could not be allocated.  Cleanup and return an appropriate
        // error status code.
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
    // Get a pointer to the stack location for the first driver.  This will
    // be used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_SET_INFORMATION;
    irpSp->FileObject = fileObject;

    //
    // Allocate a buffer and copy the information that is to be set on the
    // file into it.  Also, set the flags so that the completion code will
    // properly handle getting rid of the buffer and will not attempt to
    // copy data.
    //

    irp->AssociatedIrp.SystemBuffer = (PVOID) NULL;
    irp->MdlAddress = (PMDL) NULL;

    try {

        PVOID systemBuffer;

        systemBuffer =
        irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithQuota( NonPagedPool,
                                                                   Length );
        RtlCopyMemory( irp->AssociatedIrp.SystemBuffer,
                       FileInformation,
                       Length );

        //
        //  Negative file offsets are illegal.
        //

        ASSERT((FIELD_OFFSET(FILE_END_OF_FILE_INFORMATION, EndOfFile) |
                FIELD_OFFSET(FILE_ALLOCATION_INFORMATION, AllocationSize) |
                FIELD_OFFSET(FILE_POSITION_INFORMATION, CurrentByteOffset)) == 0);

        if (((FileInformationClass == FileEndOfFileInformation) ||
             (FileInformationClass == FileAllocationInformation) ||
             (FileInformationClass == FilePositionInformation)) &&
            (((PFILE_POSITION_INFORMATION)systemBuffer)->CurrentByteOffset.HighPart < 0)) {

            ExRaiseStatus(STATUS_INVALID_PARAMETER);
        }



    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception was incurred while allocating the intermediary
        // system buffer or while copying the caller's data into the
        // buffer. Cleanup and return an appropriate error status code.
        //

        IopExceptionCleanup( fileObject,
                             irp,
                             (PKEVENT) NULL,
                             event );

        return GetExceptionCode();

    }

    irp->Flags |= IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_DEFER_IO_COMPLETION;

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.SetFile.Length = Length;
    irpSp->Parameters.SetFile.FileInformationClass = FileInformationClass;

    //
    // Insert the packet at the head of the IRP list for the thread.
    //

    IopQueueThreadIrp( irp );

    //
    // Update the operation count statistic for the current process for
    // operations other than read and write.
    //

    IopUpdateOtherOperationCount();


    //
    // Everything is now set to invoke the device driver with this request.
    // However, it is possible that the information that the caller wants
    // to set is device independent.  If this is the case, then the request
    // can be satisfied here without having to have all of the drivers
    // implement the same code.  Note that having the IRP is still necessary
    // since the I/O completion code requires it.
    //

    if (FileInformationClass == FileModeInformation) {

        PFILE_MODE_INFORMATION modeBuffer = irp->AssociatedIrp.SystemBuffer;

        //
        // Set the various flags in the mode field for the file object, if
        // they are reasonable.  There are 4 different invalid combinations
        // that the caller may not specify:
        //
        //     1)  An invalid flag was set in the mode field.  Not all Create/
        //         Open options may be changed.
        //
        //     2)  The caller set one of the synchronous I/O flags (alert or
        //         nonalert), but the file is not opened for synchronous I/O.
        //
        //     3)  The file is opened for synchronous I/O but the caller did
        //         not set either of the synchronous I/O flags (alert or non-
        //         alert).
        //
        //     4)  The caller set both of the synchronous I/O flags (alert and
        //         nonalert).
        //

        if ((modeBuffer->Mode & ~FILE_VALID_SET_FLAGS) ||
            ((modeBuffer->Mode & (FSIO_A | FSIO_NA)) && (!(fileObject->Flags & FO_SYNCHRONOUS_IO))) ||
            ((!(modeBuffer->Mode & (FSIO_A | FSIO_NA))) && (fileObject->Flags & FO_SYNCHRONOUS_IO)) ||
            (((modeBuffer->Mode & FSIO_A) && (modeBuffer->Mode & FSIO_NA) ))) {
            status = STATUS_INVALID_PARAMETER;

        } else {

            //
            // Set or clear the appropriate flags in the file object.
            //

            if (!(fileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING)) {
                if (modeBuffer->Mode & FILE_WRITE_THROUGH) {
                    fileObject->Flags |= FO_WRITE_THROUGH;
                } else {
                    fileObject->Flags &= ~FO_WRITE_THROUGH;
                }
            }

            if (modeBuffer->Mode & FILE_SEQUENTIAL_ONLY) {
                fileObject->Flags |= FO_SEQUENTIAL_ONLY;
            } else {
                fileObject->Flags &= ~FO_SEQUENTIAL_ONLY;
            }

            if (fileObject->Flags & FO_SYNCHRONOUS_IO) {
                if (modeBuffer->Mode & FSIO_A) {
                    fileObject->Flags |= FO_ALERTABLE_IO;
                } else {
                    fileObject->Flags &= ~FO_ALERTABLE_IO;
                }
            }

            status = STATUS_SUCCESS;
        }

        //
        // Complete the I/O operation.
        //

        irp->IoStatus.Status = status;
        irp->IoStatus.Information = 0L;

    } else if (FileInformationClass == FileRenameInformation ||
               FileInformationClass == FileLinkInformation ||
               FileInformationClass == FileMoveClusterInformation) {

        //
        // Note that following code depends on the fact that the rename
        // information, link information and copy-on-write information
        // structures look exactly the same.
        //

        PFILE_RENAME_INFORMATION renameBuffer = irp->AssociatedIrp.SystemBuffer;

        //
        // The information being set is a variable-length structure with
        // embedded size information.  Walk the structure to ensure that
        // it is valid so the driver does not walk off the end and incur
        // an access violation in kernel mode.
        //

        if ((ULONG) (Length - FIELD_OFFSET( FILE_RENAME_INFORMATION, FileName[0] )) < renameBuffer->FileNameLength) {
            status = STATUS_INVALID_PARAMETER;
            irp->IoStatus.Status = status;

        } else {

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
            // Check to see whether or not a fully qualified pathname was
            // supplied.  If so, then more processing is required.
            //

            if (renameBuffer->FileName[0] == (WCHAR) OBJ_NAME_PATH_SEPARATOR ||
                renameBuffer->RootDirectory) {

                //
                // A fully qualified file name was specified as the target of
                // the rename operation.  Attempt to open the target file and
                // ensure that the replacement policy for the file is consistent
                // with the caller's request, and ensure that the file is on the
                // same volume.
                //

                status = IopOpenLinkOrRenameTarget( &targetHandle,
                                                    irp,
                                                    renameBuffer,
                                                    fileObject );
                if (!NT_SUCCESS( status )) {
                    irp->IoStatus.Status = status;

                } else {

                    //
                    // The fully qualified file name specifies a file on the
                    // same volume and if it exists, then the caller specified
                    // that it should be replaced.
                    //

                    status = IoCallDriver( deviceObject, irp );

                }

            } else {

                //
                // This is a simple rename operation, so call the driver and
                // let it perform the rename operation within the same directory
                // as the source file.
                //

                status = IoCallDriver( deviceObject, irp );

            }
        }

    } else if (FileInformationClass == FileDispositionInformation) {

        PFILE_DISPOSITION_INFORMATION disposition = irp->AssociatedIrp.SystemBuffer;

        //
        // Check to see whether the disposition delete field has been set to
        // TRUE and, if so, copy the handle being used to do this to the IRP
        // stack location parameter.
        //

        if (disposition->DeleteFile) {
            irpSp->Parameters.SetFile.DeleteHandle = FileHandle;
        }

        //
        // Simply invoke the driver to perform the (un)delete operation.
        //

        status = IoCallDriver( deviceObject, irp );

    } else if (FileInformationClass == FileCompletionInformation) {

        PFILE_COMPLETION_INFORMATION completion = irp->AssociatedIrp.SystemBuffer;
        PIO_COMPLETION_CONTEXT context;
        PVOID portObject;

        //
        // It is an error if this file object already has an LPC port associated
        // with it.
        //

        if (fileObject->CompletionContext || fileObject->Flags & FO_SYNCHRONOUS_IO) {

            status = STATUS_INVALID_PARAMETER;

        } else {

            //
            // Attempt to reference the port object by its handle and convert it
            // into a pointer to the port object itself.
            //

            status = ObReferenceObjectByHandle( completion->Port,
                                                IO_COMPLETION_MODIFY_STATE,
                                                IoCompletionObjectType,
                                                requestorMode,
                                                (PVOID *) &portObject,
                                                NULL );
            if (NT_SUCCESS( status )) {

                //
                // Allocate the memory to be associated w/this file object
                //

                context = ExAllocatePoolWithTag( PagedPool,
                                                 sizeof( IO_COMPLETION_CONTEXT ),
                                                 'cCoI' );
                if (!context) {

                    ObDereferenceObject( portObject );
                    status = STATUS_INSUFFICIENT_RESOURCES;

                } else {

                    //
                    // Everything was successful.  Capture the completion port
                    // and the key.
                    //

                    context->Port = portObject;
                    context->Key = completion->Key;

                    if (!InterlockedCompareExchangePointer( &fileObject->CompletionContext, context, NULL )) {

                        status = STATUS_SUCCESS;

                    } else {

                        //
                        // Someone set the completion context after the check.
                        // Simply drop everything on the floor and return an
                        // error.
                        //

                        ExFreePool( context );
                        ObDereferenceObject( portObject );
                        status = STATUS_INVALID_PARAMETER;
                    }
                }
            }
        }

        //
        // Complete the I/O operation.
        //

        irp->IoStatus.Status = status;
        irp->IoStatus.Information = 0;

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

    if (status == STATUS_PENDING) {

        if (synchronousIo) {

            status = KeWaitForSingleObject( &fileObject->Event,
                                            Executive,
                                            requestorMode,
                                            (BOOLEAN) ((fileObject->Flags & FO_ALERTABLE_IO) != 0),
                                            (PLARGE_INTEGER) NULL );

            if (status == STATUS_ALERTED || status == STATUS_USER_APC) {

                //
                // The wait request has ended either because the thread was
                // alerted or an APC was queued to this thread, because of
                // thread rundown or CTRL/C processing.  In either case, try
                // to bail out of this I/O request carefully so that the IRP
                // completes before this routine exists so that synchronization
                // with the file object will remain intact.
                //

                IopCancelAlertedRequest( &fileObject->Event, irp );

            }

            status = fileObject->FinalStatus;

            IopReleaseFileObjectLock( fileObject );

        } else {

            //
            // This is a normal synchronous I/O operation, as opposed to a
            // serialized synchronous I/O operation.  For this case, wait for
            // the local event and copy the final status information back to
            // the caller.
            //

            status = KeWaitForSingleObject( event,
                                            Executive,
                                            requestorMode,
                                            FALSE,
                                            (PLARGE_INTEGER) NULL );

            if (status == STATUS_ALERTED || status == STATUS_USER_APC) {

                //
                // The wait request has ended either because the thread was
                // alerted or an APC was queued to this thread, because of
                // thread rundown or CTRL/C processing.  In either case, try
                // to bail out of this I/O request carefully so that the IRP
                // completes before this routine exists or the event will not
                // be around to set to the Signaled state.
                //

                IopCancelAlertedRequest( event, irp );

            }

            status = localIoStatus.Status;

            try {

                *IoStatusBlock = localIoStatus;

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception occurred attempting to write the caller's I/O
                // status block.  Simply change the final status of the
                // operation to the exception code.
                //

                status = GetExceptionCode();
            }

            ExFreePool( event );

        }

    } else {

        //
        // The I/O operation finished without return a status of pending.
        // This means that the operation has not been through I/O completion,
        // so it must be done here.
        //

        PKNORMAL_ROUTINE normalRoutine;
        PVOID normalContext;
        KIRQL irql;

        if (!synchronousIo) {

            //
            // This is not a synchronous I/O operation, it is a synchronous
            // I/O API to a file opened for asynchronous I/O.  Since this
            // code path need never wait on the allocated and supplied event,
            // get rid of it so that it doesn't have to be set to the
            // Signaled state by the I/O completion code.
            //

            irp->UserEvent = (PKEVENT) NULL;
            ExFreePool( event );
        }

        irp->UserIosb = IoStatusBlock;
        KeRaiseIrql( APC_LEVEL, &irql );
        IopCompleteRequest( &irp->Tail.Apc,
                            &normalRoutine,
                            &normalContext,
                            (PVOID *) &fileObject,
                            &normalContext );
        KeLowerIrql( irql );

        if (synchronousIo) {
            IopReleaseFileObjectLock( fileObject );
        }

    }

    //
    // If there was a target handle generated because of a rename operation,
    // close it now.
    //

    if (targetHandle) {
        NtClose( targetHandle );
    }

    return status;
}
