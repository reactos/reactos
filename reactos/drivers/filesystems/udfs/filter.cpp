////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*

 Module Name: Filter.cpp

 Abstract:

    Contains code to handle register file system notification and attach to
    CDFS if required.

 Environment:

    Kernel mode only

*/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID    UDF_FILE_FILTER

VOID
UDFCheckOtherFS(PDEVICE_OBJECT deviceObject) {
    PFILTER_DEV_EXTENSION FilterDevExt;
    PDEVICE_OBJECT filterDeviceObject;
    NTSTATUS RC;

//    BrutePoint();

    // Acquire GlobalDataResource
    UDFAcquireResourceExclusive(&(UDFGlobalData.GlobalDataResource), TRUE);

    if (!NT_SUCCESS(RC = IoCreateDevice(
            UDFGlobalData.DriverObject,     // our driver object
            sizeof(FILTER_DEV_EXTENSION),   // don't need an extension
                                            //   for this object
            NULL,                           // name - can be used to
                                            //   "open" the driver
                                            //    see the R.Nagar's book
                                            //    for alternate choices
            FILE_DEVICE_CD_ROM_FILE_SYSTEM,
            0,                              // no special characteristics
                                            // do not want this as an
                                            //  exclusive device, though
                                            //  we might
            FALSE,
            &filterDeviceObject))) {
                // failed to create a filter device object, leave ...
        // Release the global resource.
        UDFReleaseResource( &(UDFGlobalData.GlobalDataResource) );
        return;
    }
    FilterDevExt = (PFILTER_DEV_EXTENSION)filterDeviceObject->DeviceExtension;
    // Zero it out (typically this has already been done by the I/O
    // Manager but it does not hurt to do it again)!
    RtlZeroMemory(FilterDevExt, sizeof(FILTER_DEV_EXTENSION));

    // Initialize the signature fields
    FilterDevExt->NodeIdentifier.NodeType = UDF_NODE_TYPE_FILTER_DEVOBJ;
    FilterDevExt->NodeIdentifier.NodeSize = sizeof(FILTER_DEV_EXTENSION);

    KdPrint(("UDFCheckOtherFS: Attaching filter devobj %x to FS devobj %x \n",filterDeviceObject,deviceObject));
    deviceObject = IoGetAttachedDevice( deviceObject );
    KdPrint(("UDFCheckOtherFS: top devobj is %x \n",deviceObject));
    FilterDevExt->lowerFSDeviceObject = deviceObject;

    RC = IoAttachDeviceByPointer( filterDeviceObject, deviceObject );
    if (!NT_SUCCESS(RC)) {
        IoDeleteDevice( filterDeviceObject );
    } else {
        filterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }
    // Release the global resource.
    UDFReleaseResource( &(UDFGlobalData.GlobalDataResource) );
}

VOID
UDFCheckOtherFSByName(PCWSTR DeviceObjectName) {
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    UNICODE_STRING nameString;
    NTSTATUS RC;

    KdPrint(("UDFCheckOtherFSByName: trying %s \n",DeviceObjectName));

    RtlInitUnicodeString( &nameString, DeviceObjectName );
    RC = IoGetDeviceObjectPointer(
                &nameString,
                FILE_READ_ATTRIBUTES,
                &fileObject,
                &deviceObject
                );
        
    if (!NT_SUCCESS(RC)) {
        KdPrint(("UDFCheckOtherFSByName: error %x while calling IoGetDeviceObjectPointer \n",RC));
        return;        
    }

    UDFCheckOtherFS(deviceObject);

    ObDereferenceObject( fileObject );
}

#if 0
VOID
NTAPI
UDFFsNotification(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FsActive
    )

/*

Routine Description:

    This routine is invoked whenever a file system has either registered or
    unregistered itself as an active file system.

    For the former case, this routine creates a device object and attaches it
    to the specified file system's device object.  This allows this driver
    to filter all requests to that file system.

    For the latter case, this file system's device object is located,
    detached, and deleted.  This removes this file system as a filter for
    the specified file system.

Arguments:

    DeviceObject - Pointer to the file system's device object.

    FsActive - bolean indicating whether the file system has registered
        (TRUE) or unregistered (FALSE) itself as an active file system.

Return Value:

    None.

*/

{
    // Begin by determine whether or not the file system is a cdrom-based file
    // system.  If not, then this driver is not concerned with it.
    if (DeviceObject->DeviceType != FILE_DEVICE_CD_ROM_FILE_SYSTEM) {
        return;
    }

    // Begin by determining whether this file system is registering or
    // unregistering as an active file system.
    if (FsActive) {
        KdPrint(("UDFFSNotification \n"));
        UDFCheckOtherFS(DeviceObject);
    }
}
#endif
