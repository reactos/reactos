/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    obdevmap.c

Abstract:

    This module contains routines for creating and querying Device Map objects.
    Device Map objects define a DOS device name space, such as drive letters
    and peripheral devices (e.g. COM1)

Author:

    Steve Wood (stevewo) 01-Oct-1996

Revision History:

--*/

#include "obp.h"

#if defined(ALLOC_PRAGMA)
#endif


NTSTATUS
ObSetDeviceMap (
    IN PEPROCESS TargetProcess OPTIONAL,
    IN HANDLE DirectoryHandle
    )

/*++

Routine Description:

    This function sets the device map for the specified process, using
    the specified object directory.  A device map is a structure
    associated with an object directory and a process.  When the object
    manager sees a references to a name beginning with \??\ or just \??,
    then it follows the device map object in the calling process's
    EPROCESS structure to get to the object directory to use for that
    reference.  This allows multiple virtual \??  object directories on
    a per process basis.  The WindowStation logic will use this
    functionality to allocate devices unique to each WindowStation.

Arguments:

    TargetProcess - Specifies the target process to associate the device map
        with.  If null then the current process is used and the directory
        becomes the system default dos device map.

    DirectoryHandle - Specifies the object directory to associate with the
        device map.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_SHARING_VIOLATION - The specified object directory is already
            associated with a device map.

        STATUS_INSUFFICIENT_RESOURCES - Unable to allocate pool for the device
            map data structure;

        STATUS_ACCESS_DENIED - Caller did not have DIRECTORY_TRAVERSE access
            to the specified object directory.

--*/

{
    NTSTATUS Status;
    POBJECT_DIRECTORY DosDevicesDirectory;
    PDEVICE_MAP DeviceMap;
    PVOID Object;
    HANDLE Handle;
    KIRQL OldIrql;

    PAGED_CODE();

    //
    //  Reference the object directory handle and see if it is already
    //  associated with a device map structure.  If so, fail this call.
    //

    Status = ObReferenceObjectByHandle( DirectoryHandle,
                                        DIRECTORY_TRAVERSE,
                                        ObpDirectoryObjectType,
                                        KeGetPreviousMode(),
                                        &DosDevicesDirectory,
                                        NULL );

    if (!NT_SUCCESS( Status )) {

        return( Status );
    }

    //
    //  Capture the device map
    //
    
    ExAcquireSpinLock( &ObpDeviceMapLock, &OldIrql );
    
    DeviceMap = DosDevicesDirectory->DeviceMap;
    
    //
    //  Check if the directory already has a dos device map
    //
    
    if (DeviceMap != NULL) {
        
        //
        //  Test if the DeviceMap is about to delete and has the
        //  ReferenceCount 0
        //
    
        if (DeviceMap->ReferenceCount != 0) {
        
            //
            //  We have a dos device map, so setup the target process to be either
            //  what the caller specified or the current process
            //

            PEPROCESS Target = TargetProcess;

            if (Target == NULL) {

                Target = PsGetCurrentProcess();
            }

            //
            //  If the current process already has the exact same dos device map
            //  then everything is already done and nothing left for us to do.
            //

            if (Target->DeviceMap == DeviceMap) {

                ExReleaseSpinLock( &ObpDeviceMapLock, OldIrql );
                
                ObDereferenceObject( DosDevicesDirectory );

                return Status;
            }

            //
            //  Add a new reference before releasing the spinlock
            //  to make sure nobody will release the devicemap while
            //  we try to attach to the process.
            //
            
            DeviceMap->ReferenceCount++;

            ExReleaseSpinLock( &ObpDeviceMapLock, OldIrql );
        
            //
            //  We can dereference the target processes device map because it
            //  no longer will have it's old dos device map.
            //

            ObDereferenceDeviceMap ( Target );

            //
            //  Now setup the new device map that we were feed in.  We lock it
            //  bump its ref count, make the targe process point to it, unlock it,
            //  and return to our caller
            //

            ExAcquireSpinLock( &ObpDeviceMapLock, &OldIrql );

            Target->DeviceMap = DeviceMap;

            ExReleaseSpinLock( &ObpDeviceMapLock, OldIrql );

            ObDereferenceObject( DosDevicesDirectory );

            return Status;
        }
    }

    ExReleaseSpinLock( &ObpDeviceMapLock, OldIrql );
    
    //
    //  The input directory does not have a dos device map. so we'll
    //  allocate and initialize a new device map structure.
    //

    DeviceMap = ExAllocatePoolWithTag( NonPagedPool, sizeof( *DeviceMap ), 'mDbO' );

    if (DeviceMap == NULL) {

        ObDereferenceObject( DosDevicesDirectory );
        Status = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        RtlZeroMemory( DeviceMap, sizeof( *DeviceMap ) );

        DeviceMap->ReferenceCount = 1;
        DeviceMap->DosDevicesDirectory = DosDevicesDirectory;

        DosDevicesDirectory->DeviceMap = DeviceMap;

        //
        //  If the caller specified a target process then remove the
        //  processes device map if in use and replace it with the new
        //  one, otherwise set this new device map to the system wide one
        //  and put it into the current process
        //

        if (TargetProcess != NULL) {

            ObDereferenceDeviceMap ( TargetProcess );

            TargetProcess->DeviceMap = DeviceMap;

        } else {

            ObSystemDeviceMap = DeviceMap;

            ObDereferenceDeviceMap ( PsGetCurrentProcess() );

            PsGetCurrentProcess()->DeviceMap = DeviceMap;
        }
    }

    return( Status );
}


NTSTATUS
ObQueryDeviceMapInformation (
    IN PEPROCESS TargetProcess OPTIONAL,
    OUT PPROCESS_DEVICEMAP_INFORMATION DeviceMapInformation
    )

/*++

Routine Description:

    This function queries information from the device map associated with the
    specified process.  The returned information contains a bit map indicating
    which drive letters are defined in the associated object directory, along
    with an array of drive types that give the type of each drive letter.

Arguments:

    TargetProcess - Specifies the target process to retreive the device map
        from.  If not specified then we return the global default device map

    DeviceMapInformation - Specifies the location where to store the results.

Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_END_OF_FILE - The specified process was not associated with
            a device map.

        STATUS_ACCESS_VIOLATION - The DeviceMapInformation buffer pointer
            value specified an invalid address.

--*/

{
    NTSTATUS Status;
    PDEVICE_MAP DeviceMap;
    KIRQL OldIrql;
    PROCESS_DEVICEMAP_INFORMATION LocalMapInformation;

    //
    //  Check if the caller gave us a target process and if not then use
    //  the globally defined one
    //

    if (ARGUMENT_PRESENT( TargetProcess )) {

        DeviceMap = TargetProcess->DeviceMap;

    } else {

        DeviceMap = ObSystemDeviceMap;
    }

    //
    //  If we do not have a device map then we'll return an error otherwise
    //  we simply copy over the device map structure (bitmap and drive type
    //  array) into the output buffer
    //

    if (DeviceMap == NULL) {

        Status = STATUS_END_OF_FILE;

    } else {

        Status = STATUS_SUCCESS;

        //
        //  First, while using a spinlock to protect the device map from
        //  going away we will make local copy of the information.
        //

        ExAcquireSpinLock( &ObpDeviceMapLock, &OldIrql );

        RtlMoveMemory( &LocalMapInformation.Query,
                       &DeviceMap->DriveMap,
                       sizeof( DeviceMapInformation->Query ));

        ExReleaseSpinLock( &ObpDeviceMapLock, OldIrql );

        //
        //  Now we can copy the information to the caller buffer using
        //  a try-except to guard against the output buffer changing.
        //  Note that the caller must have already probed the buffer
        //  for write.
        //

        try {

            RtlMoveMemory( &DeviceMapInformation->Query,
                           &LocalMapInformation.Query,
                           sizeof( DeviceMapInformation->Query ));

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
        }

    }

    return Status;
}


VOID
ObInheritDeviceMap (
    IN PEPROCESS NewProcess,
    IN PEPROCESS ParentProcess OPTIONAL
    )

/*++

Routine Description:

    This function is called at process initialization time to inherit the
    device map for a process.  If no parent process, then inherits from
    the system device map.

Arguments:

    NewProcess - Supplies the process being initialized that needs a new
        dos device map

    ParentProcess - - Optionally specifies the parent process whose device
        map we inherit.  This process if specified must have a device map

Return Value:

    None.

--*/

{
    PDEVICE_MAP DeviceMap;
    KIRQL OldIrql;

    //
    //  If we are called with a parent process then grab its device map
    //  otherwise grab the system wide device map and check that is does
    //  exist
    //

    if (ParentProcess) {

        DeviceMap = ParentProcess->DeviceMap;

    } else {

        //
        //  Note: WindowStation guys may want a callout here to get the
        //  device map to use for this case.
        //

        DeviceMap = ObSystemDeviceMap;

        if (DeviceMap == NULL) {

            return;
        }
    }

    //
    //  With the device map bumps its reference count and add it to the
    //  new process
    //

    ExAcquireSpinLock( &ObpDeviceMapLock, &OldIrql );

    DeviceMap->ReferenceCount++;
    NewProcess->DeviceMap = DeviceMap;

    ExReleaseSpinLock( &ObpDeviceMapLock, OldIrql );

    return;
}


VOID
ObDereferenceDeviceMap (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function is called at process tear down time to decrement the
    reference count on a device map.  When the reference count goes to
    zero, it means no more processes are using this, so it can be freed
    and the reference on the associated object directory can be released.

Arguments:

    Process - Process being destroyed.

Return Value:

    None.

--*/

{
    PDEVICE_MAP DeviceMap;
    KIRQL OldIrql;

    //
    //  Grab the device map and then we only have work to do
    //  it there is one
    //

    DeviceMap = Process->DeviceMap;

    if (DeviceMap != NULL) {

        //
        //  To dereference the device map we need to null out the
        //  processes device map pointer, and decrement its ref count
        //  If the ref count goes to zero we can free up the memory
        //  and dereference the dos device directory object
        //

        ExAcquireSpinLock( &ObpDeviceMapLock, &OldIrql );

        Process->DeviceMap = NULL;
        DeviceMap->ReferenceCount--;

        if (DeviceMap->ReferenceCount == 0) {

            DeviceMap->DosDevicesDirectory->DeviceMap = NULL;

            ExReleaseSpinLock( &ObpDeviceMapLock, OldIrql );

            ObDereferenceObject( DeviceMap->DosDevicesDirectory );

            ExFreePool( DeviceMap );

        } else {

            ExReleaseSpinLock( &ObpDeviceMapLock, OldIrql );
        }
    }

    //
    //  And return to our caller
    //

    return;
}

