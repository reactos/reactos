/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    qsfs.c

Abstract:

    This module contains the code to implement the NtQueryVolumeInformationFile
    and NtSetVolumeInformationFile system services for the NT I/O system.

Author:

    Darryl E. Havens (darrylh) 22-Jun-1989

Environment:

    Kernel mode only

Revision History:


--*/

#include "iop.h"
#pragma hdrstop
#include <ioevent.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtQueryVolumeInformationFile)
#pragma alloc_text(PAGE, NtSetVolumeInformationFile)
#endif

NTSTATUS
NtQueryVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    )

/*++

Routine Description:

    This service returns information about the volume associated with the
    FileHandle parameter.  The information returned in the buffer is defined
    by the FsInformationClass parameter.  The legal values for this parameter
    are as follows:

        o  FileFsVolumeInformation

        o  FileFsSizeInformation

        o  FileFsDeviceInformation

        o  FileFsAttributeInformation

Arguments:

    FileHandle - Supplies a handle to an open volume, directory, or file
        for which information about the volume is returned.

    IoStatusBlock - Address of the caller's I/O status block.

    FsInformation - Supplies a buffer to receive the requested information
        returned about the volume.

    Length - Supplies the length, in bytes, of the FsInformation buffer.

    FsInformationClass - Specifies the type of information which should be
        returned about the volume.

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
        // Ensure that the FsInformationClass parameter is legal for querying
        // information about the volume.
        //

        if ((ULONG) FsInformationClass >= FileFsMaximumInformation ||
            IopQueryFsOperationLength[FsInformationClass] == 0) {
            return STATUS_INVALID_INFO_CLASS;
        }

        //
        // Finally, ensure that the supplied buffer is large enough to contain
        // the information associated with the specified query operation that
        // is to be performed.
        //

        if (Length < (ULONG) IopQueryFsOperationLength[FsInformationClass]) {
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
            // The FsInformation buffer must be writeable by the caller.
            //

#if defined(_X86_)
            ProbeForWrite( FsInformation, Length, sizeof( ULONG ) );
#elif defined(_WIN64)

            //
            // If we are a wow64 process, follow the X86 rules
            //

            if (PsGetCurrentProcess()->Wow64Process) {
                ProbeForWrite( FsInformation, Length, sizeof( ULONG ) );
            } else {
                ProbeForWrite( FsInformation,
                               Length,
                               IopQuerySetFsAlignmentRequirement[FsInformationClass] );

            }
#else
            ProbeForWrite( FsInformation,
                           Length,
                           IopQuerySetFsAlignmentRequirement[FsInformationClass] );
#endif

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred probing the caller's parameters.
            // Simply return an appropriate error status code.
            //

#if DBG
            if (GetExceptionCode() == STATUS_DATATYPE_MISALIGNMENT) {
                DbgBreakPoint();
            }
#endif // DBG

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
                                        IopQueryFsOperationAccess[FsInformationClass],
                                        IoFileObjectType,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        NULL );
    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // If this open file object represents an open device that was explicitly
    // opened for querying the device's attributes, then ensure that the type
    // of information class was device information.
    //

    if ((fileObject->Flags & FO_DIRECT_DEVICE_OPEN) &&
        FsInformationClass != FileFsDeviceInformation) {
        ObDereferenceObject( fileObject );
        return STATUS_INVALID_DEVICE_REQUEST;
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
        synchronousIo = FALSE;
    }

    //
    // Get the address of the target device object.  A special check is made
    // here to determine whether this query is for device information.  If
    // it is, and either:
    //
    //     a)  The open was for the device itself, or
    //
    //     b)  The open was for a file but this is not a redirected device,
    //
    // then perform the query operation in-line.  That is, do not allocate
    // an IRP and call the driver, rather, simply copy the device type and
    // characteristics information from the target device object pointed
    // to by the device object in the file object (the "real" device object
    // in a mass storage device stack).
    //

    if (FsInformationClass == FileFsDeviceInformation &&
        (fileObject->Flags & FO_DIRECT_DEVICE_OPEN ||
        fileObject->DeviceObject->DeviceType != FILE_DEVICE_NETWORK_FILE_SYSTEM)) {

        PFILE_FS_DEVICE_INFORMATION deviceAttributes;
        BOOLEAN deviceMounted = FALSE;

        //
        // This query operation can be performed in-line.  Simply copy the
        // information directly from the target device object and indicate
        // that the operation was successful.  Begin, however, by determining
        // whether or not the device is mounted.  This cannot be done at the
        // same time as attempting to touch the user's buffer, as looking at
        // the mounted bit occurs at raised IRQL.
        //

        deviceObject = fileObject->DeviceObject;
        if (deviceObject->Vpb) {
            deviceMounted = IopGetMountFlag( deviceObject );
        }

        //
        // Copy the characteristics information from the device's object
        // into the caller's buffer.
        //

        deviceAttributes = (PFILE_FS_DEVICE_INFORMATION) FsInformation;

        try {

            deviceAttributes->DeviceType = deviceObject->DeviceType;
            deviceAttributes->Characteristics = deviceObject->Characteristics;
            if (deviceMounted) {
                deviceAttributes->Characteristics |= FILE_DEVICE_IS_MOUNTED;
            }

            IoStatusBlock->Status = STATUS_SUCCESS;
            IoStatusBlock->Information = sizeof( FILE_FS_DEVICE_INFORMATION );
            status = STATUS_SUCCESS;

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            //
            // An error occurred attempting to write into one of the caller's
            // buffers.  Simply indicate that the error occurred, and fall
            // through.
            //

            status = GetExceptionCode();
        }

        //
        // If this operation was performed as synchronous I/O, then release
        // the file object lock.
        //

        if (fileObject->Flags & FO_SYNCHRONOUS_IO) {
            IopReleaseFileObjectLock( fileObject );
        }

        //
        // Now simply cleanup and return the final status of the operation.
        //

        ObDereferenceObject( fileObject );
        return status;

    }

    //
    // This is either a query that is not for device characteristics
    // information, or it is a query for device information, but it is
    // a query for a redirected device.  Take the long route and actually
    // invoke the driver for the target device to get the information.
    //
    // Set the file object to the Not-Signaled state.
    //

    KeClearEvent( &fileObject->Event );

    //
    // Get a pointer to the device object for the target device.
    //

    deviceObject = IoGetRelatedDeviceObject( fileObject );

    //
    // If this I/O operation is not being performed as synchronous I/O,
    // then allocate an event that will be used to synchronize the
    // completion of this operation.  That is, this system service is
    // a synchronous API being invoked for a file that is opened for
    // asynchronous I/O.
    //

    if (!(fileObject->Flags & FO_SYNCHRONOUS_IO)) {
        event = ExAllocatePool( NonPagedPool, sizeof( KEVENT ) );
        if (event == NULL) {
            ObDereferenceObject( fileObject );
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        KeInitializeEvent( event, SynchronizationEvent, FALSE );
    }

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this
    // operation.  The allocation is performed with an exception handler
    // in case the caller does not have enough quota to allocate the packet.

    irp = IoAllocateIrp( deviceObject->StackSize, TRUE );
    if (!irp) {

        //
        // An IRP could not be allocated.  Cleanup and return an
        // appropriate error status code.
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
    irpSp->MajorFunction = IRP_MJ_QUERY_VOLUME_INFORMATION;
    irpSp->FileObject = fileObject;

    //
    // Allocate a buffer which should be used to put the information into
    // by the driver.  This will be copied back to the caller's buffer when
    // the service completes.  This is done by setting the flag which says
    // that this is an input operation.
    //

    irp->UserBuffer = FsInformation;
    irp->AssociatedIrp.SystemBuffer = (PVOID) NULL;
    irp->MdlAddress = (PMDL) NULL;

    //
    // Allocate the system buffer using an exception handler in case the
    // caller doesn't have enough quota remaining.
    //

    try {

        irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithQuota( NonPagedPool,
                                                                   Length );
    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception was incurred attempting to allocate the inter-
        // mediary buffer.  Cleanup and return with an appropriate error
        // status code.
        //

        IopExceptionCleanup( fileObject,
                             irp,
                             (PKEVENT) NULL,
                             event );

        return GetExceptionCode();

    }

    irp->Flags |= (ULONG) (IRP_BUFFERED_IO |
                           IRP_DEALLOCATE_BUFFER |
                           IRP_INPUT_OPERATION |
                           IRP_DEFER_IO_COMPLETION);

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.QueryVolume.Length = Length;
    irpSp->Parameters.QueryVolume.FsInformationClass = FsInformationClass;

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
NtSetVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    )

/*++

Routine Description:

    This service changes information about the volume "mounted" on the device
    specified by the FileHandle parameter.  The information to be changed is
    in the FsInformation buffer.  Its contents are defined by the FsInformation-
    Class parameter, whose values may be as follows:

        o  FileFsLabelInformation

Arguments:

    FileHandle - Supplies a handle to the volume whose information should be
        changed.

    IoStatusBlock - Address of the caller's I/O status block.

    FsInformation - Supplies a buffer containing the information which should
        be changed on the volume.

    Length - Supplies the length, in bytes, of the FsInformation buffer.

    FsInformationClass - Specifies the type of information which should be
        changed about the volume.

Return Value:

    The status returned is the final completion status of the operation.
    block.

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
    PFILE_FS_LABEL_INFORMATION labelInformation;
    BOOLEAN synchronousIo;
    PDEVICE_OBJECT targetDeviceObject;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {

        //
        // Ensure that the FsInformationClass parameter is legal for setting
        // information about the volume.
        //

        if ((ULONG) FsInformationClass >= FileFsMaximumInformation ||
            IopSetFsOperationLength[FsInformationClass] == 0) {
            return STATUS_INVALID_INFO_CLASS;
        }

        //
        // Finally, ensure that the supplied buffer is large enough to contain
        // the information associated with the specified set operation that is
        // to be performed.
        //

        if (Length < (ULONG) IopSetFsOperationLength[FsInformationClass]) {
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
            // The FsInformation buffer must be readable by the caller.
            //

#if defined(_X86_)
            ProbeForRead( FsInformation, Length, sizeof( ULONG ) );
#elif defined(_IA64_)
            // If we are a wow64 process, follow the X86 rules
            if (PsGetCurrentProcess()->Wow64Process) {
                ProbeForRead( FsInformation, Length, sizeof( ULONG ) );
            }
            else {
                ProbeForRead( FsInformation,
                              Length,
                              IopQuerySetFsAlignmentRequirement[FsInformationClass] );

            }
#else
            ProbeForRead( FsInformation,
                          Length,
                          IopQuerySetFsAlignmentRequirement[FsInformationClass] );
#endif

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred probing the caller's parameters.
            // Simply return an appropriate error status code.
            //


#if DBG
            if (GetExceptionCode() == STATUS_DATATYPE_MISALIGNMENT) {
                DbgBreakPoint();
            }
#endif DBG

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
                                        IopSetFsOperationAccess[FsInformationClass],
                                        IoFileObjectType,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        NULL );
    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // Retrieve the device object associated with this file handle.
    //
    
    status = IoGetRelatedTargetDevice( fileObject, &targetDeviceObject );

    if (NT_SUCCESS( status )) {
        //
        // The PDO associated with the devnode we got back from
        // IoGetRelatedTargetDevice has already been referenced by that
        // routine.  Store this reference away in the notification entry,
        // so we can deref it later when the notification entry is unregistered.
        //
    
        ASSERT(targetDeviceObject);
    
    } else {
        targetDeviceObject = NULL;
    }


    //
    // Make a special check here to determine whether this is a synchronous
    // I/O operation.  If it is, then wait here until the file is owned by
    // the current thread.  if this is not a (serialized) synchronous I/O
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
                if (targetDeviceObject != NULL) {
                    ObDereferenceObject( targetDeviceObject );
                }
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
            if (targetDeviceObject != NULL) {
                ObDereferenceObject( targetDeviceObject );
            }
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

        if (targetDeviceObject != NULL) {
            ObDereferenceObject( targetDeviceObject );
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
    irpSp->MajorFunction = IRP_MJ_SET_VOLUME_INFORMATION;
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

        irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithQuota( NonPagedPool,
                                                                   Length );
        RtlCopyMemory( irp->AssociatedIrp.SystemBuffer, FsInformation, Length );

    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception was incurred attempting to allocate the intermediary
        // buffer or while copying the caller's data to the buffer. Determine
        // what happened, cleanup, and return an appropriate error status
        // code.
        //

        IopExceptionCleanup( fileObject,
                             irp,
                             (PKEVENT) NULL,
                             event );

        if (targetDeviceObject != NULL) {
            ObDereferenceObject( targetDeviceObject );
        }
        
        return GetExceptionCode();

    }

    //
    // If the previous mode was not kernel, check the captured label buffer
    // for consistency.
    //

    if (requestorMode != KernelMode &&
        FsInformationClass == FileFsLabelInformation) {

        //
        // The previous mode was something other than kernel.  Check to see
        // whether or not the length of the label specified within the label
        // structure is consistent with the overall length of the structure
        // itself.  If not, then cleanup and get out.
        //

        labelInformation = (PFILE_FS_LABEL_INFORMATION) irp->AssociatedIrp.SystemBuffer;

        if ((LONG) labelInformation->VolumeLabelLength < 0 ||
            labelInformation->VolumeLabelLength +
            FIELD_OFFSET( FILE_FS_LABEL_INFORMATION, VolumeLabel ) > Length) {

            IopExceptionCleanup( fileObject,
                                 irp,
                                 (PKEVENT) NULL,
                                 event );

            if (targetDeviceObject != NULL) {
                ObDereferenceObject( targetDeviceObject );
            }
            
            return STATUS_INVALID_PARAMETER;
        }
    }

    irp->Flags |= (ULONG) (IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER);

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.SetVolume.Length = Length;
    irpSp->Parameters.SetVolume.FsInformationClass = FsInformationClass;


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

    //
    //  Notify anyone who cares about the label change
    //

    if (targetDeviceObject != NULL) {
        if (NT_SUCCESS( status )) {
            TARGET_DEVICE_CUSTOM_NOTIFICATION ChangeEvent;
    
            ChangeEvent.Version = 1;
            ChangeEvent.FileObject = NULL;
            ChangeEvent.NameBufferOffset = -1;
            ChangeEvent.Size = (USHORT)FIELD_OFFSET( TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer );
            
            RtlCopyMemory( &ChangeEvent.Event, &GUID_IO_VOLUME_CHANGE, sizeof( GUID_IO_VOLUME_CHANGE ));
            
            IoReportTargetDeviceChange( targetDeviceObject, &ChangeEvent );
        }

        ObDereferenceObject( targetDeviceObject );
    }

    return status;
}
