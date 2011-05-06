/*
 * Driver Regression Tests
 *
 * Copyright 2009 Michael Martin <martinmnet@hotmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* INCLUDES *******************************************************************/

#include "kmtest.h"
#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include "ntddser.h"
#include "ntndk.h"

VOID LowerDeviceKernelAPITest(PDEVICE_OBJECT DeviceObject, BOOLEAN UnLoading)
{
    PDEVICE_OBJECT RetObject;

    RetObject = IoGetLowerDeviceObject(DeviceObject);

    if (UnLoading)
    {
        ok(RetObject == 0,
            "Expected no Lower DeviceObject, got %p", RetObject);
    }
    else
    {
        ok(RetObject == AttachDeviceObject,
            "Expected an Attached DeviceObject %p, got %p", AttachDeviceObject, RetObject);
    }

    if (RetObject)
    {
        ObDereferenceObject(RetObject);
    }

    RetObject = IoGetDeviceAttachmentBaseRef(DeviceObject);
    ok(RetObject == DeviceObject,
        "Expected an Attached DeviceObject %p, got %p", DeviceObject, RetObject);

    if (RetObject)
    {
        ObDereferenceObject(RetObject);
    }

}
VOID DeviceCreatedTest(PDEVICE_OBJECT DeviceObject, BOOLEAN ExclusiveAccess)
{
    PEXTENDED_DEVOBJ_EXTENSION extdev;

    /*Check the device object members */
    ok(DeviceObject->Type==3, "Expected Type = 3, got %x", DeviceObject->Type);
    ok(DeviceObject->Size = 0xb8, "Expected Size = 0xba, got %x", DeviceObject->Size);
    ok(DeviceObject->ReferenceCount == 0, "Expected ReferenceCount = 0, got %lu",
        DeviceObject->ReferenceCount);
    ok(DeviceObject->DriverObject == ThisDriverObject,
        "Expected DriverObject member to match this DriverObject %p, got %p",
        ThisDriverObject, DeviceObject->DriverObject);
    ok(DeviceObject->NextDevice == NULL, "Expected NextDevice to be NULL, got %p", DeviceObject->NextDevice);
    ok(DeviceObject->AttachedDevice == NULL, "Expected AttachDevice to be NULL, got %p", DeviceObject->AttachedDevice);
    ok(DeviceObject->Characteristics == 0, "Expected Characteristics to be 0");
    if (ExclusiveAccess)
    {
        ok((DeviceObject->Flags == (DO_DEVICE_HAS_NAME | DO_DEVICE_INITIALIZING | DO_EXCLUSIVE)),
            "Expected Flags DO_DEVICE_HAS_NAME | DO_DEVICE_INITIALIZING | DO_EXCLUSIVE, got %lu", DeviceObject->Flags);
    }
    else
    {
        ok((DeviceObject->Flags == (DO_DEVICE_HAS_NAME | DO_DEVICE_INITIALIZING)),
            "Expected Flags DO_DEVICE_HAS_NAME | DO_DEVICE_INITIALIZING, got %lu", DeviceObject->Flags);
    }
    ok(DeviceObject->DeviceType == FILE_DEVICE_UNKNOWN,
        "Expected DeviceType to match creation parameter FILE_DEVICE_UNKNWOWN, got %lu",
        DeviceObject->DeviceType);
    ok(DeviceObject->ActiveThreadCount == 0, "Expected ActiveThreadCount = 0, got %lu\n", DeviceObject->ActiveThreadCount);

    /*Check the extended extension */
    extdev = (PEXTENDED_DEVOBJ_EXTENSION)DeviceObject->DeviceObjectExtension;
    ok(extdev->ExtensionFlags == 0, "Expected Extended ExtensionFlags to be 0, got %lu", extdev->ExtensionFlags);
    ok (extdev->Type == 13, "Expected Type of 13, got %d", extdev->Type);
    ok (extdev->Size == 0, "Expected Size of 0, got %d", extdev->Size);
    ok (extdev->DeviceObject == DeviceObject, "Expected DeviceOject to match newly created device %p, got %p",
        DeviceObject, extdev->DeviceObject);
    ok(extdev->AttachedTo == NULL, "Expected AttachTo to be NULL, got %p", extdev->AttachedTo);
    ok(extdev->StartIoCount == 0, "Expected StartIoCount = 0, got %lu", extdev->StartIoCount);
    ok(extdev->StartIoKey == 0, "Expected StartIoKey = 0, got %lu", extdev->StartIoKey);
    ok(extdev->StartIoFlags == 0, "Expected StartIoFlags = 0, got %lu", extdev->StartIoFlags);
}

VOID DeviceDeletionTest(PDEVICE_OBJECT DeviceObject, BOOLEAN Lower)
{
    PEXTENDED_DEVOBJ_EXTENSION extdev;

    /*Check the device object members */
    ok(DeviceObject->Type==3, "Expected Type = 3, got %d", DeviceObject->Type);
    ok(DeviceObject->Size = 0xb8, "Expected Size = 0xba, got %d", DeviceObject->Size);
    ok(DeviceObject->ReferenceCount == 0, "Expected ReferenceCount = 0, got %lu",
        DeviceObject->ReferenceCount);
    if (!Lower)
    {
        ok(DeviceObject->DriverObject == ThisDriverObject,
            "Expected DriverObject member to match this DriverObject %p, got %p",
            ThisDriverObject, DeviceObject->DriverObject);
    }
    ok(DeviceObject->NextDevice == NULL, "Expected NextDevice to be NULL, got %p", DeviceObject->NextDevice);

    if (Lower)
    {
        ok(DeviceObject->AttachedDevice == MainDeviceObject,
            "Expected AttachDevice to be %p, got %p", MainDeviceObject, DeviceObject->AttachedDevice);
    }
    else
    {
        ok(DeviceObject->AttachedDevice == NULL, "Expected AttachDevice to be NULL, got %p", DeviceObject->AttachedDevice);
    }

    ok(DeviceObject->Flags ==FILE_VIRTUAL_VOLUME,
        "Expected Flags FILE_VIRTUAL_VOLUME, got %lu", DeviceObject->Flags);
    ok(DeviceObject->DeviceType == FILE_DEVICE_UNKNOWN, 
        "Expected DeviceType to match creation parameter FILE_DEVICE_UNKNWOWN, got %lu",
        DeviceObject->DeviceType);
    ok(DeviceObject->ActiveThreadCount == 0, "Expected ActiveThreadCount = 0, got %lu\n", DeviceObject->ActiveThreadCount);

    /*Check the extended extension */
    extdev = (PEXTENDED_DEVOBJ_EXTENSION)DeviceObject->DeviceObjectExtension;
    ok(extdev->ExtensionFlags == DOE_UNLOAD_PENDING,
        "Expected Extended ExtensionFlags to be DOE_UNLOAD_PENDING, got %lu", extdev->ExtensionFlags);
    ok (extdev->Type == 13, "Expected Type of 13, got %d", extdev->Type);
    ok (extdev->Size == 0, "Expected Size of 0, got %d", extdev->Size);
    ok (extdev->DeviceObject == DeviceObject, "Expected DeviceOject to match newly created device %p, got %p",
        DeviceObject, extdev->DeviceObject);
    if (Lower)
    {
        /* Skip this for now */
        //ok(extdev->AttachedTo == MainDeviceObject, "Expected AttachTo to %p, got %p", MainDeviceObject, extdev->AttachedTo);
    }
    else
    {
        ok(extdev->AttachedTo == NULL, "Expected AttachTo to be NULL, got %p", extdev->AttachedTo);
    }
    ok(extdev->StartIoCount == 0, "Expected StartIoCount = 0, got %lu", extdev->StartIoCount);
    ok(extdev->StartIoKey == 0, "Expected StartIoKey = 0, got %lu", extdev->StartIoKey);
    ok(extdev->StartIoFlags == 0, "Expected StartIoFlags = 0, got %lu", extdev->StartIoFlags);
}

VOID DeviceCreateDeleteTest(PDRIVER_OBJECT DriverObject)
{
    NTSTATUS Status;
    UNICODE_STRING DeviceString;
    UNICODE_STRING DosDeviceString;
    PDEVICE_OBJECT DeviceObject;

    /* Create using wrong directory */
    RtlInitUnicodeString(&DeviceString, L"\\Device1\\Kmtest");
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceString,
                            FILE_DEVICE_UNKNOWN,
                            0,
                            FALSE,
                            &DeviceObject);
    ok(Status == STATUS_OBJECT_PATH_NOT_FOUND, "Expected STATUS_OBJECT_PATH_NOT_FOUND, got 0x%lX", Status);

    /* Create using correct params with exlusice access */
    RtlInitUnicodeString(&DeviceString, L"\\Device\\Kmtest");
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceString,
                            FILE_DEVICE_UNKNOWN,
                            0,
                            TRUE,
                            &DeviceObject);
    ok(Status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%lX", Status);

    DeviceCreatedTest(DeviceObject, TRUE);

    /* Delete the device */
    if (NT_SUCCESS(Status))
    {
        IoDeleteDevice(DeviceObject);
        ok(DriverObject->DeviceObject == 0, "Expected DriverObject->DeviceObject to be NULL, got %p",
            DriverObject->DeviceObject);
    }

    /* Create using correct params with exlusice access */
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceString,
                            FILE_DEVICE_UNKNOWN,
                            0,
                            FALSE,
                            &DeviceObject);
    ok(Status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%lX", Status);

    DeviceCreatedTest(DeviceObject, FALSE);

    /* Delete the device */
    if (NT_SUCCESS(Status))
    {
        IoDeleteDevice(DeviceObject);
        ok(DriverObject->DeviceObject == 0, "Expected DriverObject->DeviceObject to be NULL, got %p",
            DriverObject->DeviceObject);
    }

    /* Recreate device */
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceString,
                            FILE_DEVICE_UNKNOWN,
                            0,
                            FALSE,
                            &DeviceObject);
    ok(Status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%lX", Status);

    RtlInitUnicodeString(&DosDeviceString, L"\\DosDevices\\kmtest");
    Status = IoCreateSymbolicLink(&DosDeviceString, &DeviceString);

    if (!NT_SUCCESS(Status))
    {
            /* Delete device object if not successful */
            IoDeleteDevice(DeviceObject);
            return;
    }

    MainDeviceObject = DeviceObject;

    return;
}

BOOLEAN AttachDeviceTest(PDEVICE_OBJECT DeviceObject,  PWCHAR NewDriverRegPath)
{
    NTSTATUS Status;
    UNICODE_STRING LowerDeviceName;

    RtlInitUnicodeString(&LowerDeviceName, NewDriverRegPath);
    Status = IoAttachDevice(DeviceObject, &LowerDeviceName, &AttachDeviceObject);

    /* TODO: Add more tests */

    return TRUE;
}

BOOLEAN DetachDeviceTest(PDEVICE_OBJECT AttachedDevice)
{

    IoDetachDevice(AttachedDevice);

    /* TODO: Add more tests */

    return TRUE;
}
