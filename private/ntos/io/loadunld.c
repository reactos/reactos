/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    loadunld.c

Abstract:

    This module contains the code to implement the NtLoadDriver and
    NtUnLoadDriver system services for the NT I/O system.

Author:

    Darryl E. Havens (darrylh) 5-Apr-1992

Environment:

    Kernel mode only

Revision History:


--*/

#include "iop.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtLoadDriver)
#pragma alloc_text(PAGE, NtUnloadDriver)
#endif

NTSTATUS
NtLoadDriver(
    IN PUNICODE_STRING DriverServiceName
    )

/*++

Routine Description:

    This service dynamically loads a device or file system driver into
    the currently running system.  It requires that the caller have the
    appropriate privilege to execute this service.

Arguments:

    DriverServiceName - Specifies the name of the node in the registry
        associated with the driver to be loaded.

Return Value:

    The status returned is the final completion status of the load operation.

--*/

{
    KPROCESSOR_MODE requestorMode;
    UNICODE_STRING driverServiceName;
    PWCHAR nameBuffer = (PWCHAR) NULL;
    LOAD_PACKET loadPacket;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {

        //
        // The caller's access mode is not kernel so check to ensure that
        // the caller has the privilege to load a driver and probe and
        // capture the name of the driver service entry.
        //

        if (!SeSinglePrivilegeCheck( SeLoadDriverPrivilege, requestorMode )) {
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        //
        // The caller has the appropriate privilege to load and unload
        // drivers, so capture the driver service name string so that it
        // can be used to locate the driver from the registry node.
        //

        try {

            driverServiceName = ProbeAndReadUnicodeString( DriverServiceName );

            if (!driverServiceName.Length) {
                return STATUS_INVALID_PARAMETER;
            }

            ProbeForRead( driverServiceName.Buffer,
                          driverServiceName.Length,
                          sizeof( WCHAR ) );

            nameBuffer = ExAllocatePoolWithQuota( PagedPool,
                                                  driverServiceName.Length );

            RtlCopyMemory( nameBuffer,
                           driverServiceName.Buffer,
                           driverServiceName.Length );

            driverServiceName.Buffer = nameBuffer;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred while attempting to capture the
            // input name string or while attempting to allocate the name
            // string buffer.  Simply clean everything up and return an
            // appropriate error status code.
            //

            if (nameBuffer) {
                ExFreePool( nameBuffer );
            }
            return GetExceptionCode();
        }
    } else {
        driverServiceName = *DriverServiceName;
    }

    //
    // Because drivers may wish to create a system thread and execute in
    // its context, the remainder of this service must be executed in the
    // context of the primary system process.  This is accomplished by
    // queueing a request to one of the EX worker threads and having it
    // invoke the I/O system routine to complete this work.
    //
    // Fill in a request packet and queue it to the worker thread then, so
    // that it can actually do the load.
    //

    KeInitializeEvent( &loadPacket.Event, NotificationEvent, FALSE );
    loadPacket.DriverObject = (PDRIVER_OBJECT) NULL;
    loadPacket.DriverServiceName = &driverServiceName;
    
    if (PsGetCurrentProcess() == PsInitialSystemProcess) {
    
        //
        // If we are already in the system process, just use this thread.
        //

        IopLoadUnloadDriver(&loadPacket);
    
    } else {
    
        ExInitializeWorkItem( &loadPacket.WorkQueueItem,
                              IopLoadUnloadDriver,
                              &loadPacket );
    
        ExQueueWorkItem( &loadPacket.WorkQueueItem, DelayedWorkQueue );
    
        KeWaitForSingleObject( &loadPacket.Event,
                               UserRequest,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER) NULL );
    
    }
    
    //
    // The load operation is now complete.  If a name buffer was allocated,
    // deallocate it now, and return the final status of the load operation.
    //

    if (nameBuffer) {
         ExFreePool( nameBuffer );
    }

    return loadPacket.FinalStatus;
}

NTSTATUS
IopCheckUnloadDriver(
    IN PDRIVER_OBJECT driverObject,
    OUT PBOOLEAN unloadDriver
    )
{
    PDEVICE_OBJECT deviceObject;
    KIRQL irql;

    //
    // Check to see whether the driver has already been marked for an unload
    // operation by anyone in the past.
    //

    ExAcquireSpinLock( &IopDatabaseLock, &irql );

    if ((driverObject->DeviceObject == NULL &&
        (driverObject->Flags & DRVO_UNLOAD_INVOKED)) ||
        (driverObject->DeviceObject &&
        driverObject->DeviceObject->DeviceObjectExtension->ExtensionFlags
        & DOE_UNLOAD_PENDING)) {

        //
        // The driver has already been marked for unload or is being
        // unloaded.  Simply return a successful completion status since
        // the driver is on its way out and therefore has been "marked for
        // unload".
        //

        ExReleaseSpinLock( &IopDatabaseLock, irql );

        ObDereferenceObject( driverObject );
        return STATUS_SUCCESS;
    }

    //
    // The driver exists, and it implements unload, and it has not, so far,
    // been marked for an unload operation.  Simply mark all of the devices
    // that the driver owns as being marked for unload.  While this is going
    // on, count the references for each of the devices.  If all of the
    // devices have a zero reference count, then tell the driver that it
    // should unload itself.
    //

    deviceObject = driverObject->DeviceObject;
    *unloadDriver = TRUE;

    while (deviceObject) {
        deviceObject->DeviceObjectExtension->ExtensionFlags |= DOE_UNLOAD_PENDING;
        if (deviceObject->ReferenceCount || deviceObject->AttachedDevice) {
	    *unloadDriver = FALSE;
        }
        deviceObject = deviceObject->NextDevice;
    }

    if (*unloadDriver) {
        driverObject->Flags |= DRVO_UNLOAD_INVOKED;
    }

    ExReleaseSpinLock( &IopDatabaseLock, irql );
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NtUnloadDriver(
    IN PUNICODE_STRING DriverServiceName
    )

/*++

Routine Description:

    This service dynamically unloads a device or file system driver from
    the currently running system.  It requires that the caller have the
    appropriate privilege to execute this service.

Arguments:

    DriverServiceName - Specifies the name of the node in the registry
        associated with the driver to be unloaded.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    KPROCESSOR_MODE requestorMode;
    UNICODE_STRING driverServiceName;
    PWCHAR nameBuffer = (PWCHAR) NULL;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE keyHandle;
    UNICODE_STRING driverName;
    HANDLE driverHandle;
    PDRIVER_OBJECT driverObject;
    BOOLEAN unloadDriver;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {

        //
        // The caller's access mode is not kernel so check to ensure that
        // the caller has the privilege to unload a driver and probe and
        // capture the name of the driver service entry.
        //

        if (!SeSinglePrivilegeCheck( SeLoadDriverPrivilege, requestorMode )) {
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        //
        // The caller has the appropriate privilege to load and unload
        // drivers, so capture the driver service name string so that it
        // can be used to locate the driver from the registry node.
        //

        try {

            driverServiceName = ProbeAndReadUnicodeString( DriverServiceName );

            if (!driverServiceName.Length) {
                return STATUS_INVALID_PARAMETER;
            }

            ProbeForRead( driverServiceName.Buffer,
                          driverServiceName.Length,
                          sizeof( WCHAR ) );

            nameBuffer = ExAllocatePoolWithQuota( PagedPool,
                                                  driverServiceName.Length );

            RtlCopyMemory( nameBuffer,
                           driverServiceName.Buffer,
                           driverServiceName.Length );

            driverServiceName.Buffer = nameBuffer;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred while attempting to capture the
            // input name string or while attempting to allocate the name
            // string buffer.  Simply clean everything up and return an
            // appropriate error status code.
            //

            if (nameBuffer) {
                ExFreePool( nameBuffer );
            }
            return GetExceptionCode();
        }

        //
        // Now that the caller's parameters have been captured and everything
        // appears to have checked out, actually attempt to unload the driver.
        // This is done with a previous mode of kernel so that drivers will
        // not fail to unload because the caller didn't happen to have access
        // to some resource that the driver needs in order to complete its
        // unload operation.
        //

        status = ZwUnloadDriver( &driverServiceName );
        ExFreePool( nameBuffer );
        return status;
    }

    //
    // The caller's mode is now kernel mode.  Attempt to actually unload the
    // driver specified by the indicated registry node.  Begin by opening
    // the registry node for this driver.
    //

    status = IopOpenRegistryKey( &keyHandle,
                                 (HANDLE) NULL,
                                 DriverServiceName,
                                 KEY_READ,
                                 FALSE );
    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // Get the optional object name for this driver from the value for this
    // key.  If one exists, then its name overrides the default name of the
    // driver.
    //

    status = IopGetDriverNameFromKeyNode( keyHandle,
                                          &driverName );
    NtClose( keyHandle );
    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // Now attempt to open the driver object for the specified driver.
    //

    InitializeObjectAttributes( &objectAttributes,
                                &driverName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ObOpenObjectByName( &objectAttributes,
                                 IoDriverObjectType,
                                 KernelMode,
                                 NULL,
                                 FILE_READ_DATA,
                                 (PVOID) NULL,
                                 &driverHandle );

    //
    // Perform some common cleanup by getting rid of buffers that have been
    // allocated up to this point so that error conditions do not have as
    // much work to do on each exit path.
    //

    ExFreePool( driverName.Buffer );

    //
    // If the driver object could not be located in the first place, then
    // return now before attempting to do anything else.
    //

    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // The driver object was located, so convert the handle into a pointer
    // so that the driver object itself can be examined.
    //

    status = ObReferenceObjectByHandle( driverHandle,
                                        0,
                                        IoDriverObjectType,
                                        KernelMode,
                                        (PVOID *) &driverObject,
                                        NULL );
    NtClose( driverHandle );

    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // Check to see whether or not this driver implements unload.  Also,
    // if the driver has no section associated with it, then it was loaded
    // be the OS loader and therefore cannot be unloaded.  If either is true,
    // return an appropriate error status code.
    //

    if (driverObject->DriverUnload == (PDRIVER_UNLOAD) NULL ||
        !driverObject->DriverSection) {
        ObDereferenceObject( driverObject );
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Check to see whether the driver has already been marked for an unload
    // operation by anyone in the past.
    //

    status = IopCheckUnloadDriver(driverObject,&unloadDriver);

    if ( NT_SUCCESS(status) ) {
        return status;
    }

    if (unloadDriver) {

        if (PsGetCurrentProcess() == PsInitialSystemProcess) {

            //
            // The current thread is alrady executing in the context of the
            // system process, so simply invoke the driver's unload routine.
            //

            driverObject->DriverUnload( driverObject );

        } else {

            //
            // The current thread is not executing in the context of the system
            // process, which is required in order to invoke the driver's unload
            // routine.  Queue a worker item to one of the worker threads to
            // get into the appropriate process context and then invoke the
            // routine.
            //

            LOAD_PACKET loadPacket;

            KeInitializeEvent( &loadPacket.Event, NotificationEvent, FALSE );
            loadPacket.DriverObject = driverObject;
            ExInitializeWorkItem( &loadPacket.WorkQueueItem,
                                  IopLoadUnloadDriver,
                                  &loadPacket );
            ExQueueWorkItem( &loadPacket.WorkQueueItem, DelayedWorkQueue );
            (VOID) KeWaitForSingleObject( &loadPacket.Event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          (PLARGE_INTEGER) NULL );
        }
        ObMakeTemporaryObject( driverObject );
        ObDereferenceObject( driverObject );
    }

    //
    // The driver has either been unloaded, or it has successfully been
    // marked for an unload operation.  Simply dereference the pointer to
    // the object and return success.
    //

    ObDereferenceObject( driverObject );
    return STATUS_SUCCESS;
}
