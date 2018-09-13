/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    qsea.c

Abstract:

    This module contains the code to implement the NtQueryEaFile and the
    NtSetEaFile system services for the NT I/O system.

Author:

    Darryl E. Havens (darrylh) 20-Jun-1989

Environment:

    Kernel mode only

Revision History:


--*/

#include "iop.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtQueryEaFile)
#pragma alloc_text(PAGE, NtSetEaFile)
#endif

NTSTATUS
NtQueryEaFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN PVOID EaList OPTIONAL,
    IN ULONG EaListLength,
    IN PULONG EaIndex OPTIONAL,
    IN BOOLEAN RestartScan
    )

/*++

Routine Description:

    This service returns the Extended Attributes (EAs) associated with the
    file specified by the FileHandle parameter.  The amount of information
    returned is based on the size of the EAs, and the size of the buffer.
    That is, either all of the EAs are written to the buffer, or the buffer
    is filled with complete EAs if the buffer is not large enough to contain
    all of the EAs.  Only complete EAs are ever written to the buffer; no
    partial EAs will ever be returned.

Arguments:

    FileHandle - Supplies a handle to the file for which the EAs are returned.

    IoStatusBlock - Address of the caller's I/O status block.

    Buffer - Supplies a buffer to receive the EAs for the file.

    Length - Supplies the length, in bytes, of the buffer.

    ReturnSingleEntry - Indicates that only a single entry should be returned
        rather than filling the buffer with as many EAs as possible.

    EaList - Optionally supplies a list of EA names whose values are returned.

    EaListLength - Supplies the length of the EA list, if an EA list was
        specified.

    EaIndex - Supplies the optional index of an EA whose value is to be
        returned.  If specified, then only that EA is returned.

    RestartScan - Indicates whether the scan of the EAs should be restarted
        from the beginning.

Return Value:

    The status returned is the final completion status of the operation.

--*/

#define GET_OFFSET_LENGTH( CurrentEa, EaBase ) (    \
    (ULONG) ((PCHAR) CurrentEa - (PCHAR) EaBase) )


{
    PIRP irp;
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PKEVENT event = (PKEVENT) NULL;
    PCHAR auxiliaryBuffer = (PCHAR) NULL;
    BOOLEAN eaListPresent = FALSE;
    ULONG eaIndexValue = 0L;
    KPROCESSOR_MODE requestorMode;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK localIoStatus;
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

            //
            // The buffer must be writeable by the caller.
            //

            ProbeForWrite( Buffer, Length, sizeof( ULONG ) );

            //
            // If the optional EaIndex parameter was specified, then it must be
            // readable by the caller.  Capture its value.
            //

            if (ARGUMENT_PRESENT( EaIndex )) {
                eaIndexValue = ProbeAndReadUlong( EaIndex );
            }

            //
            // If the optional EaList parameter was specified, then it must be
            // readable by the caller.  Validate that the buffer contains a
            // legal get information structure.
            //

            if (ARGUMENT_PRESENT( EaList ) && EaListLength != 0) {

                PFILE_GET_EA_INFORMATION eas;
                LONG tempLength;
                ULONG entrySize;

                eaListPresent = TRUE;

                ProbeForRead( EaList, EaListLength, sizeof( ULONG ) );
                auxiliaryBuffer = ExAllocatePoolWithQuota( NonPagedPool,
                                                           EaListLength );
                RtlCopyMemory( auxiliaryBuffer, EaList, EaListLength );

                eas = (PFILE_GET_EA_INFORMATION) auxiliaryBuffer;
                tempLength = EaListLength;

                //
                // Walk the request buffer and ensure that its format is
                // valid.  That is, ensure that it does not walk off the
                // end of the buffer that has been captured.
                //

                for (;;) {

                    //
                    // Get the size of the current entry in the buffer.
                    //

                    if (tempLength < FIELD_OFFSET( FILE_GET_EA_INFORMATION, EaName[0])) {
                        tempLength = 0;
                        ExFreePool( auxiliaryBuffer );
                        auxiliaryBuffer = (PVOID) NULL;
                        IoStatusBlock->Status = STATUS_EA_LIST_INCONSISTENT;
                        IoStatusBlock->Information = tempLength;
                        return STATUS_EA_LIST_INCONSISTENT;
                        }

                    entrySize = FIELD_OFFSET( FILE_GET_EA_INFORMATION, EaName[0] ) + eas->EaNameLength + 1;

                    if ((ULONG) tempLength < entrySize) {
                        tempLength = GET_OFFSET_LENGTH( eas, auxiliaryBuffer );
                        ExFreePool( auxiliaryBuffer );
                        auxiliaryBuffer = (PVOID) NULL;
                        IoStatusBlock->Status = STATUS_EA_LIST_INCONSISTENT;
                        IoStatusBlock->Information = tempLength;
                        return STATUS_EA_LIST_INCONSISTENT;
                        }

                    if (eas->NextEntryOffset != 0) {

                        //
                        // There is another entry in the buffer and it must
                        // be longword aligned.  Ensure that the offset
                        // indicates that it is.  If it isn't, return an
                        // invalid parameter status.
                        //

                        if ((((entrySize + 3) & ~3) != eas->NextEntryOffset) ||
                            ((LONG) eas->NextEntryOffset < 0)) {
                            tempLength = GET_OFFSET_LENGTH( eas, auxiliaryBuffer );
                            ExFreePool( auxiliaryBuffer );
                            auxiliaryBuffer = (PVOID) NULL;
                            IoStatusBlock->Status = STATUS_EA_LIST_INCONSISTENT;
                            IoStatusBlock->Information = tempLength;
                            return STATUS_EA_LIST_INCONSISTENT;

                        } else {

                            //
                            // There is another entry in the buffer, so
                            // account for the size of the current entry
                            // in the length and get a pointer to the next
                            // entry.
                            //

                            tempLength -= eas->NextEntryOffset;
                            if (tempLength < 0) {
                                tempLength = GET_OFFSET_LENGTH( eas, auxiliaryBuffer );
                                ExFreePool( auxiliaryBuffer );
                                auxiliaryBuffer = (PVOID) NULL;
                                IoStatusBlock->Status = STATUS_EA_LIST_INCONSISTENT;
                                IoStatusBlock->Information = tempLength;
                                return STATUS_EA_LIST_INCONSISTENT;
                            }
                            eas = (PFILE_GET_EA_INFORMATION) ((PCHAR) eas + eas->NextEntryOffset);
                        }

                    } else {

                        //
                        // There are no other entries in the buffer.  Simply
                        // account for the overall buffer length according
                        // to the size of the current entry and exit the
                        // loop.
                        //

                        tempLength -= entrySize;
                        break;
                    }
                }

                //
                // All of the entries in the buffer have been processed.
                // Check to see whether the overall buffer length went
                // negative.  If so, return an error.
                //

                if (tempLength < 0) {
                    tempLength = GET_OFFSET_LENGTH( eas, auxiliaryBuffer );
                    ExFreePool( auxiliaryBuffer );
                    auxiliaryBuffer = (PVOID) NULL;
                    IoStatusBlock->Status = STATUS_EA_LIST_INCONSISTENT;
                    IoStatusBlock->Information = tempLength;
                    return STATUS_EA_LIST_INCONSISTENT;
                }

            }

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred while probing the caller's
            // parameters, allocating the pool buffer, or copying the
            // caller's EA list to the buffer.  Cleanup and return an
            // appropriate error status code.
            //

            if (auxiliaryBuffer != NULL) {
                ExFreePool( auxiliaryBuffer );
            }

            return GetExceptionCode();

        }

    } else {

        //
        // The caller's mode was KernelMode.  Simply allocate pool for the
        // EaList, if one was specified, and copy the string to it.  Also,
        // if an EaIndex was specified copy it as well.
        //

        if (ARGUMENT_PRESENT( EaList ) && (EaListLength != 0)) {
            eaListPresent = TRUE;
            try {
                auxiliaryBuffer = ExAllocatePoolWithQuota( NonPagedPool,
                                                           EaListLength );
            } except(EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }
            RtlCopyMemory( auxiliaryBuffer, EaList, EaListLength );
        }

        if (ARGUMENT_PRESENT( EaIndex )) {
            eaIndexValue = *EaIndex;
        }
    }

    //
    // There were no blatant errors so far, so reference the file object so
    // the target device object can be found.  Note that if the handle does
    // not refer to a file object, or if the caller does not have the required
    // access to the file, then it will fail.
    //

    status = ObReferenceObjectByHandle( FileHandle,
                                        FILE_READ_EA,
                                        IoFileObjectType,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        NULL );
    if (!NT_SUCCESS( status )) {
        if (eaListPresent) {
            ExFreePool( auxiliaryBuffer );
        }
        return status;
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
                if (eaListPresent) {
                    ExFreePool( auxiliaryBuffer );
                }
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
            if (eaListPresent) {
                ExFreePool( auxiliaryBuffer );
            }
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
        // An IRP could not be allocated.  Cleanup and return an appropriate
        // error status code.
        //

        if (!(fileObject->Flags & FO_SYNCHRONOUS_IO)) {
            ExFreePool( event );
        }

        IopAllocateIrpCleanup( fileObject, (PKEVENT) NULL );

        if (eaListPresent) {
            ExFreePool( auxiliaryBuffer );
        }

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
    irpSp->MajorFunction = IRP_MJ_QUERY_EA;
    irpSp->FileObject = fileObject;

    //
    // If the caller specified an EA list of names to be queried, then pass
    // the address of the intermediary buffer containing the list to the
    // driver.
    //

    if (eaListPresent) {
        irp->Tail.Overlay.AuxiliaryBuffer = auxiliaryBuffer;
        irpSp->Parameters.QueryEa.EaList = auxiliaryBuffer;
        irpSp->Parameters.QueryEa.EaListLength = EaListLength;
    }

    //
    // Now determine whether this driver expects to have data buffered
    // to it or whether it performs direct I/O.  This is based on the
    // DO_BUFFERED_IO flag in the device object.  If the flag is set,
    // then a system buffer is allocated and the driver's data will be
    // copied to it.  If the DO_DIRECT_IO flag is set in the device
    // object, then a Memory Descriptor List (MDL) is allocated and
    // the caller's buffer is locked down using it.  Finally, if the
    // driver specifies neither of the flags, then simply pass the
    // address and length of the buffer and allow the driver to perform
    // all of the checking and buffering if any is required.
    //

    if (deviceObject->Flags & DO_BUFFERED_IO) {

        //
        // The driver wishes the caller's buffered be copied into an
        // intermediary buffer.  Allocate the system buffer and specify
        // that it should be deallocated on completion.  Also indicate
        // that this is an input operation so the data will be copied
        // into the caller's buffer.  This is done using an exception
        // handler that will perform cleanup if the operation fails.
        //

        if (Length) {
            try {

                //
                // Allocate the intermediary system buffer from nonpaged
                // pool and charge quota for it.
                //

                irp->AssociatedIrp.SystemBuffer =
                   ExAllocatePoolWithQuota( NonPagedPool, Length );
 
            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception was incurred while either probing the
                // caller's buffer or allocating the system buffer.
                // Determine what actually happened, clean everything
                // up, and return an appropriate error status code.
                //

                IopExceptionCleanup( fileObject,
                                     irp,
                                     (PKEVENT) NULL,
                                     event );

                if (auxiliaryBuffer != NULL) {
                    ExFreePool( auxiliaryBuffer );
                }

                return GetExceptionCode();

            }

            //
            // Remember the address of the caller's buffer so the copy can
            // take place during I/O completion.  Also, set the flags so
            // that the completion code knows to do the copy and to deallocate
            // the buffer.
            //

            irp->UserBuffer = Buffer;
            irp->Flags |= (ULONG) (IRP_BUFFERED_IO |
                                   IRP_DEALLOCATE_BUFFER |
                                   IRP_INPUT_OPERATION);
        } else {
            //
            // This is a zero-length request.  Simply indicate that this is
            // buffered I/O, and pass along the request.  The buffer will
            // not be set to deallocate so the completion path does not
            // have to special-case the length.
            //

            irp->AssociatedIrp.SystemBuffer = NULL;
            irp->Flags |= (ULONG) (IRP_BUFFERED_IO | IRP_INPUT_OPERATION);

        }

    } else if (deviceObject->Flags & DO_DIRECT_IO) {

        PMDL mdl;

        //
        // This is a direct I/O operation.  Allocate an MDL and invoke
        // the memory management routine to lock the buffer into memory.
        // This is done using an exception handler that will perform
        // cleanup if the operation fails.
        //

        if (Length) {
            mdl = (PMDL) NULL;

            try {

                //
                // Allocate an MDL, charging quota for it, and hang it off
                // of the IRP.  Probe and lock the pages associated with
                // the caller's buffer for write access and fill in the MDL
                // with the PFNs of those pages.
                //

                mdl = IoAllocateMdl( Buffer, Length, FALSE, TRUE, irp );
                if (mdl == NULL) {
                    ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                }
                MmProbeAndLockPages( mdl, requestorMode, IoWriteAccess );

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception was incurred while either probing the
                // caller's buffer or allocating the MDL.  Determine what
                // actually happened, clean everything up, and return an
                // appropriate error status code.
                //

                IopExceptionCleanup( fileObject,
                                     irp,
                                     (PKEVENT) NULL,
                                     event );

                if (auxiliaryBuffer != NULL) {
                    ExFreePool( auxiliaryBuffer );
                }

                return GetExceptionCode();

            }
        }

    } else {

        //
        // Pass the address of the user's buffer so the driver has access
        // to it.  It is now the driver's responsibility to do everything.
        //

        irp->UserBuffer = Buffer;

    }

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.QueryEa.Length = Length;
    irpSp->Parameters.QueryEa.EaIndex = eaIndexValue;
    irpSp->Flags = 0;
    if (RestartScan) {
        irpSp->Flags = SL_RESTART_SCAN;
    }
    if (ReturnSingleEntry) {
        irpSp->Flags |= SL_RETURN_SINGLE_ENTRY;
    }
    if (ARGUMENT_PRESENT( EaIndex )) {
        irpSp->Flags |= SL_INDEX_SPECIFIED;
    }

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
NtSetEaFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This service replaces the Extended Attributes (EAs) associated with the file
    specified by the FileHandle parameter.  All of the EAs associated with the
    file are replaced by the EAs in the specified buffer.

Arguments:

    FileHandle - Supplies a handle to the file whose EAs should be changed.

    IoStatusBlock - Address of the caller's I/O status block.

    FileInformation - Supplies a buffer containing the new EAs which should be
        used to replace the EAs currently associated with the file.

    Length - Supplies the length, in bytes, of the buffer.

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
    BOOLEAN synchronousIo;

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

            //
            // The Buffer parameter must be readable by the caller.
            //

            ProbeForRead( Buffer, Length, sizeof( ULONG ) );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred while probing the caller's parameters.
            // Cleanup and return an appropriate error status code.
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
                                        FILE_WRITE_EA,
                                        IoFileObjectType,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        NULL );
    if (!NT_SUCCESS( status )) {
        return status;
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
    irpSp->MajorFunction = IRP_MJ_SET_EA;
    irpSp->FileObject = fileObject;

    //
    // Now determine whether this driver expects to have data buffered to it
    // or whether it performs direct I/O.  This is based on the DO_BUFFERED_IO
    // flag in the device object.  if the flag is set, then a system buffer is
    // allocated and driver's data is copied to it.  If the DO_DIRECT_IO flag
    // is set in the device object, then a Memory Descriptor List (MDL) is
    // allocated and the caller's buffer is locked down using it.  Finally, if
    // the driver specifies neither of the flags, then simply pass the address
    // and length of the buffer and allow the driver to perform all of the
    // checking and buffering if any is required.
    //

    if (deviceObject->Flags & DO_BUFFERED_IO) {

        PFILE_FULL_EA_INFORMATION systemBuffer;
        ULONG errorOffset;

        //
        // The driver wishes the caller's buffer to be copied into an
        // intermediary buffer.  Allocate the system buffer and specify
        // that it should be deallocated on completion.  Also check to
        // ensure that the caller's EA list is valid.  All of this is
        // performed within an exception handler that will perform
        // cleanup if the operation fails.
        //

        if (Length) {
            try {

            //
            // Allocate the intermediary system buffer and charge the caller
            // quota for its allocation.  Copy the caller's EA buffer into the
            // buffer and check to ensure that it is valid.
            //

                systemBuffer = ExAllocatePoolWithQuota( NonPagedPool, Length );

                irp->AssociatedIrp.SystemBuffer = systemBuffer;

                RtlCopyMemory( systemBuffer, Buffer, Length );

                status = IoCheckEaBufferValidity( systemBuffer,
                                                  Length,
                                                  &errorOffset );

                if (!NT_SUCCESS( status )) {
                    IoStatusBlock->Status = status;
                    IoStatusBlock->Information = errorOffset;
                    ExRaiseStatus( status );
                }

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception was incurred while allocating the buffer, copying
                // the caller's data into it, or walking the EA buffer.  Determine
                // what happened, cleanup, and return an appropriate error status
                // code.
                //

                IopExceptionCleanup( fileObject,
                                     irp,
                                     (PKEVENT) NULL,
                                     event );

                return GetExceptionCode();

            }

            //
            // Set the flags so that the completion code knows to deallocate the
            // buffer.
            //
    
            irp->Flags |= IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
        } else {
            irp->AssociatedIrp.SystemBuffer = NULL;
        }


    } else if (deviceObject->Flags & DO_DIRECT_IO) {

        PMDL mdl;

        //
        // This is a direct I/O operation.  Allocate an MDL and invoke the
        // memory management routine to lock the buffer into memory.  This is
        // done using an exception handler that will perform cleanup if the
        // operation fails.
        //

        mdl = (PMDL) NULL;

        if (Length) {
            try {

                //
                // Allocate an MDL, charging quota for it, and hang it off of the
                // IRP.  Probe and lock the pages associated with the caller's
                // buffer for read access and fill in the MDL with the PFNs of those
                // pages.
                //

                mdl = IoAllocateMdl( Buffer, Length, FALSE, TRUE, irp );
                if (mdl == NULL) {
                    ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                }
                MmProbeAndLockPages( mdl, requestorMode, IoReadAccess );

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception was incurred while either probing the caller's
                // buffer or allocating the MDL.  Determine what actually happened,
                // clean everything up, and return an appropriate error status code.
                //

                IopExceptionCleanup( fileObject,
                                     irp,
                                     (PKEVENT) NULL,
                                     event );

                return GetExceptionCode();

            }
        }

    } else {

        //
        // Pass the address of the user's buffer so the driver has access to
        // it.  It is now the driver's responsibility to do everything.
        //

        irp->UserBuffer = Buffer;

    }

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.SetEa.Length = Length;


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
