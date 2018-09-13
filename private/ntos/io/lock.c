/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    lock.c

Abstract:

    This module contains the code to implement the NtLockFile and the
    NtUnlockFile system services for the NT I/O system.

Author:

    Darryl E. Havens (darrylh) 29-Nov-1989

Environment:

    Kernel mode only

Revision History:


--*/

#include "iop.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtLockFile)
#pragma alloc_text(PAGE, NtUnlockFile)
#endif

NTSTATUS
NtLockFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER ByteOffset,
    IN PLARGE_INTEGER Length,
    IN ULONG Key,
    IN BOOLEAN FailImmediately,
    IN BOOLEAN ExclusiveLock
    )

/*++

Routine Description:

    This service locks a specified range of bytes on the file specified by
    the FileHandle parameter.  The lock may either be an exclusive lock or
    a shared lock.  Furthermore, the caller has the option of specifying
    whether or not the service should return immediately if the lock cannot
    be acquired without waiting.

Arguments:

    FileHandle - Supplies a handle to an open file.

    Event - Supplies an optional event to be set to the Signaled state when
        the operation is complete.

    ApcRoutine - Supplies an optional APC routine to be executed when the
        operation is complete.

    ApcContext - Supplies a context parameter to be passed to the ApcRoutine,
        if an ApcRoutine was specified.

    IoStatusBlock - Address of the caller's I/O status block.

    ByteOffset - Specifies the starting byte offset of the range to lock.

    Length - Specifies the length of the byte range to be locked.

    Key - Specifies the key to be associated with the lock.

    FailImmediately - Specifies that if the lock cannot immediately be
        acquired that the service should return to the caller.

    ExclusiveLock - Specifies, if TRUE, that the lock should be an exclusive
        lock;  otherwise the lock is a shared lock.

Return Value:

    The status returned is success if the operation was properly queued to
    the I/O system.  Once the operation completes, the status can be
    determined by examining the Status field of the I/O status block.

--*/

{
    PIRP irp;
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PKEVENT eventObject = (PKEVENT) NULL;
    KPROCESSOR_MODE requestorMode;
    PIO_STACK_LOCATION irpSp;
    LARGE_INTEGER fileOffset;
    LARGE_INTEGER length;
    ACCESS_MASK grantedAccess;
    OBJECT_HANDLE_INFORMATION handleInformation;
    BOOLEAN synchronousIo;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    //
    // Reference the file object so the target device can be found and the
    // access rights mask can be used in the following checks for callers
    // in user mode.  Note that if the handle does not refer to a file
    // object, then it will fail.
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

    if (requestorMode != KernelMode) {

        //
        // The caller's access mode is not kernel so probe each of the arguments
        // and capture them as necessary.  If any failures occur, the condition
        // handler will be invoked to handle them.  It will simply cleanup and
        // return an access violation status code back to the system service
        // dispatcher.
        //

        //
        // Check to ensure that the caller has either READ or WRITE access to
        // the file.  If not, cleanup and return an error.
        //

        if (!SeComputeGrantedAccesses( grantedAccess, FILE_READ_DATA | FILE_WRITE_DATA )) {
            ObDereferenceObject( fileObject );
            return STATUS_ACCESS_DENIED;
        }

        try {

            //
            // The IoStatusBlock parameter must be writeable by the caller.
            //

            ProbeForWriteIoStatusEx( IoStatusBlock , ApcRoutine);

            //
            // The ByteOffset parameter must be readable by the caller.  Probe
            // and capture it.
            //

            ProbeForRead( ByteOffset,
                          sizeof( LARGE_INTEGER ),
                          sizeof( ULONG ) );
            fileOffset = *ByteOffset;

            //
            // Likewise, the Length parameter must also be readable by the
            // caller.  Probe and capture it as well.
            //

            ProbeForRead( Length,
                          sizeof( LARGE_INTEGER ),
                          sizeof( ULONG ) );
            length = *Length;

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

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred attempting to probe the caller's
            // parameters.  Dereference the file object and return an
            // appropriate error status code.
            //

            ObDereferenceObject( fileObject );
            return GetExceptionCode();
        }

    } else {

        //
        // The caller's mode was kernel.  Get the ByteOffset and Length
        // parameter 's to the expected locations.
        //

        fileOffset = *ByteOffset;
        length = *Length;
    }

    //
    // Get the address of the event object and set the event to the Not-
    // Signaled state, if an event was specified.  Note here, too, that if
    // the handle does not refer to an event, or if the event cannot be
    // written, then the reference will fail.  Since certain legacy
    // applications rely on an old bug in Win32's LockFileEx, we must
    // tolerate bad event handles.
    //

    if (ARGUMENT_PRESENT( Event )) {
        status = ObReferenceObjectByHandle( Event,
                                            EVENT_MODIFY_STATE,
                                            ExEventObjectType,
                                            requestorMode,
                                            (PVOID *) &eventObject,
                                            NULL );
        if (!NT_SUCCESS( status )) {
            ASSERT( !eventObject );
        } else {
            KeClearEvent( eventObject );
        }
    }

    //
    // Get the address of the target device object and the fast Io dispatch
    // structure.
    //

    deviceObject = IoGetRelatedDeviceObject( fileObject );
    fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

    //
    // Turbo lock support.  If the fast Io Dispatch specifies a fast lock
    // routine then we'll first try and calling it with the specified lock
    // parameters.
    //

    if (fastIoDispatch && fastIoDispatch->FastIoLock) {

        IO_STATUS_BLOCK localIoStatus;

        if (fastIoDispatch->FastIoLock( fileObject,
                                        &fileOffset,
                                        &length,
                                        PsGetCurrentProcess(),
                                        Key,
                                        FailImmediately,
                                        ExclusiveLock,
                                        &localIoStatus,
                                        deviceObject )) {

            //
            // Carefully return the I/O status.
            //

            try {
                *IoStatusBlock = localIoStatus;
            } except( EXCEPTION_EXECUTE_HANDLER ) {
                localIoStatus.Status = GetExceptionCode();
                localIoStatus.Information = 0;
            }

            //
            // If a valid event was specified, set it.
            //

            if (eventObject) {
                KeSetEvent( eventObject, 0, FALSE );
                ObDereferenceObject( eventObject );
            }

            //
            // Note that the file object event need not be set to the
            // Signaled state, as it is already set.
            //

            //
            // If this file object has a completion port associated with it
            // and this request has a non-NULL APC context then a completion
            // message needs to be queued.
            //

            if (fileObject->CompletionContext && ARGUMENT_PRESENT( ApcContext )) {
                if (!NT_SUCCESS(IoSetIoCompletion( fileObject->CompletionContext->Port,
                                                   fileObject->CompletionContext->Key,
                                                   ApcContext,
                                                   localIoStatus.Status,
                                                   localIoStatus.Information,
                                                   TRUE ))) {
                    localIoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            //
            // Cleanup and return.
            //

            fileObject->LockOperation = TRUE;
            ObDereferenceObject( fileObject );
            return localIoStatus.Status;
        }
    }

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
        synchronousIo = TRUE;
    } else {
        synchronousIo = FALSE;
    }

    //
    // Set the file object to the Not-Signaled state and mark it as having had
    // a lock operation performed on it.
    //

    KeClearEvent( &fileObject->Event );
    fileObject->LockOperation = TRUE;

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

        IopAllocateIrpCleanup( fileObject, eventObject );

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = requestorMode;

    //
    // Fill in the service independent parameters in the IRP.
    //

    irp->UserEvent = eventObject;
    irp->UserIosb = IoStatusBlock;
    irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_LOCK_CONTROL;
    irpSp->MinorFunction = IRP_MN_LOCK;
    irpSp->FileObject = fileObject;

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Flags = 0;
    if (FailImmediately) {
        irpSp->Flags = SL_FAIL_IMMEDIATELY;
    }
    if (ExclusiveLock) {
        irpSp->Flags |= SL_EXCLUSIVE_LOCK;
    }
    irpSp->Parameters.LockControl.Key = Key;
    irpSp->Parameters.LockControl.ByteOffset = fileOffset;

    try {
        PLARGE_INTEGER lengthBuffer;

        //
        // Attempt to allocate an intermediary buffer to hold the length of
        // this lock operation.  If it fails, either because there is no
        // more quota, or because there are no more resources, then the
        // exception handler will be invoked to cleanup and exit.
        //

        lengthBuffer = ExAllocatePoolWithQuota( NonPagedPool,
                                                sizeof( LARGE_INTEGER ) );

        *lengthBuffer = length;
        irp->Tail.Overlay.AuxiliaryBuffer = (PCHAR) lengthBuffer;
        irpSp->Parameters.LockControl.Length = lengthBuffer;
    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception was incurred.  Simply clean everything up and
        // return an appropriate error status code.
        //

        IopExceptionCleanup( fileObject,
                             irp,
                             eventObject,
                             (PKEVENT) NULL );

        return GetExceptionCode();
    }

    //
    // Queue the packet, call the driver, and synchronize appopriately with
    // I/O completion.
    //

    return IopSynchronousServiceTail( deviceObject,
                                      irp,
                                      fileObject,
                                      FALSE,
                                      requestorMode,
                                      synchronousIo,
                                      OtherTransfer );
}

NTSTATUS
NtUnlockFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER ByteOffset,
    IN PLARGE_INTEGER Length,
    IN ULONG Key
    )

/*++

Routine Description:

    This service releases the lock associated with the specified byte range
    for the file specified by the FileHandle parameter.

Arguments:

    FileHandle - Supplies a handle to an open file.

    IoStatusBlock - Address of the caller's I/O status block.

    ByteOffset - Specifies the byte offset of the range to unlock.

    Length - Specifies the length of the byte range to unlock.

    Key - Specifies the key associated with the locked range.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    PIRP irp;
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    PKEVENT event;
    KPROCESSOR_MODE requestorMode;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK localIoStatus;
    LARGE_INTEGER fileOffset;
    LARGE_INTEGER length;
    ACCESS_MASK grantedAccess;
    OBJECT_HANDLE_INFORMATION handleInformation;
    BOOLEAN synchronousIo;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    //
    // Reference the file object so the target device can be found and the
    // access rights mask can be used in the following checks for callers
    // in user mode.  Note that if the handle does not refer to a file
    // object, then it will fail.
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
        // Check to ensure that the caller has either READ or WRITE access
        // to the file.  If not, cleanup and return an error.
        //

        if (!SeComputeGrantedAccesses( grantedAccess, FILE_READ_DATA | FILE_WRITE_DATA )) {
            ObDereferenceObject( fileObject );
            return STATUS_ACCESS_DENIED;
        }

        try {

            //
            // The IoStatusBlock parameter must be writeable by the caller.
            //

            ProbeForWriteIoStatus( IoStatusBlock );

            //
            // The ByteOffset parameter must be readable by the caller.  Probe
            // and capture it.
            //

            ProbeForRead( ByteOffset,
                          sizeof( LARGE_INTEGER ),
                          sizeof( ULONG ) );
            fileOffset = *ByteOffset;

            //
            // Likewise, the Length parameter must also be readable by the
            // caller.  Probe and capture it as well.
            //

            ProbeForRead( Length,
                          sizeof( LARGE_INTEGER ),
                          sizeof( ULONG ) );
            length = *Length;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred while attempting to probe the
            // caller's parameters.  Dereference the file object and return
            // an appropriate error status code.
            //

            ObDereferenceObject( fileObject );
            return GetExceptionCode();

        }

    } else {

        //
        // The caller's mode was kernel.  Get the ByteOffset and Length
        // parameter 's to the expected locations.
        //

        fileOffset = *ByteOffset;
        length = *Length;
    }

    //
    // Get the address of the target device object.  If this file represents
    // a device that was opened directly, then simply use the device or its
    // attached device(s) directly.  Also get the fast I/O dispatch address.
    //

    if (!(fileObject->Flags & FO_DIRECT_DEVICE_OPEN)) {
        deviceObject = IoGetRelatedDeviceObject( fileObject );
    } else {
        deviceObject = IoGetAttachedDevice( fileObject->DeviceObject );
    }
    fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

    //
    // Turbo lock support.  If the fast Io Dispatch specifies a fast lock
    // routine then we'll first try and calling it with the specified lock
    // parameters.
    //

    if (fastIoDispatch && fastIoDispatch->FastIoUnlockSingle) {

        IO_STATUS_BLOCK localIoStatus;

        if (fastIoDispatch->FastIoUnlockSingle( fileObject,
                                                &fileOffset,
                                                &length,
                                                PsGetCurrentProcess(),
                                                Key,
                                                &localIoStatus,
                                                deviceObject )) {

            //
            // Carefully return the I/O status.
            //

            try {
                *IoStatusBlock = localIoStatus;
            } except( EXCEPTION_EXECUTE_HANDLER ) {
                localIoStatus.Status = GetExceptionCode();
                localIoStatus.Information = 0;
            }

            //
            // Cleanup and return.
            //

            ObDereferenceObject( fileObject );
            return localIoStatus.Status;
        }
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
    // Get a pointer to the stack location for the first driver.  This will
    // be used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_LOCK_CONTROL;
    irpSp->MinorFunction = IRP_MN_UNLOCK_SINGLE;
    irpSp->FileObject = fileObject;

    try {
        PLARGE_INTEGER lengthBuffer;

        //
        // Attempt to allocate an intermediary buffer to hold the length of
        // this lock operation.  If it fails, either because there is no
        // more quota, or because there are no more resources, then the
        // exception handler will be invoked to cleanup and exit.
        //

        lengthBuffer = ExAllocatePoolWithQuota( NonPagedPool,
                                                sizeof( LARGE_INTEGER ) );

        *lengthBuffer = length;
        irp->Tail.Overlay.AuxiliaryBuffer = (PCHAR) lengthBuffer;
        irpSp->Parameters.LockControl.Length = lengthBuffer;
    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception was incurred.  Simply clean everything up and
        // return an appropriate error status code.
        //

        if (!(fileObject->Flags & FO_SYNCHRONOUS_IO)) {
            ExFreePool( event );
        }
  
        IopExceptionCleanup( fileObject,
                             irp,
                             NULL,
                             (PKEVENT) NULL );

        return GetExceptionCode();
    }

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.LockControl.Key = Key;
    irpSp->Parameters.LockControl.ByteOffset = fileOffset;

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
