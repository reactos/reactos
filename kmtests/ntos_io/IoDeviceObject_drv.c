/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver Object Test Driver
 * PROGRAMMER:      Michael Martin <martinmnet@hotmail.com>
 *                  Thomas Faber <thfabba@gmx.de>
 */

#include <kmt_test.h>

//#define NDEBUG
#include <debug.h>

typedef enum
{
    DriverEntry,
    DriverIrp,
    DriverUnload
} DRIVER_STATUS;

static KMT_IRP_HANDLER TestIrpHandler;
static VOID TestDriverObject(IN PDRIVER_OBJECT DriverObject, IN DRIVER_STATUS DriverStatus);

static PDEVICE_OBJECT MainDeviceObject;
static PDEVICE_OBJECT AttachDeviceObject;
static PDRIVER_OBJECT ThisDriverObject;

NTSTATUS
TestEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PCUNICODE_STRING RegistryPath,
    OUT PCWSTR *DeviceName,
    IN OUT INT *Flags)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);
    UNREFERENCED_PARAMETER(Flags);

    *DeviceName = L"IoDeviceObject";

    KmtRegisterIrpHandler(IRP_MJ_CREATE, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_CLOSE, NULL, TestIrpHandler);

    TestDriverObject(DriverObject, DriverEntry);

    return Status;
}

VOID
TestUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    TestDriverObject(DriverObject, DriverUnload);
}

static
NTSTATUS
TestIrpHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation)
{
    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(IoStackLocation);

    TestDriverObject(DeviceObject->DriverObject, DriverIrp);

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

static
VOID
TestDriverObject(
    IN PDRIVER_OBJECT DriverObject,
    IN DRIVER_STATUS DriverStatus)
{
    BOOLEAN CheckThisDispatchRoutine;
    PVOID FirstMajorFunc;
    int i;

    ok(DriverObject->Size == sizeof(DRIVER_OBJECT), "Size does not match, got %x",DriverObject->Size);
    ok(DriverObject->Type == 4, "Type does not match 4. got %d",DriverObject->Type);

    if (DriverStatus == DriverEntry)
    {
        ok(DriverObject->DeviceObject == NULL, "Expected DeviceObject pointer to be 0, got %p",
            DriverObject->DeviceObject);
        ok (DriverObject->Flags == DRVO_LEGACY_DRIVER,
            "Expected Flags to be DRVO_LEGACY_DRIVER, got %lu",
            DriverObject->Flags);
    }
    else if (DriverStatus == DriverIrp)
    {
        ok(DriverObject->DeviceObject != NULL, "Expected DeviceObject pointer to non null");
        ok (DriverObject->Flags == (DRVO_LEGACY_DRIVER | DRVO_INITIALIZED),
            "Expected Flags to be DRVO_LEGACY_DRIVER | DRVO_INITIALIZED, got %lu",
            DriverObject->Flags);
    }
    else if (DriverStatus == DriverUnload)
    {
        ok(DriverObject->DeviceObject != NULL, "Expected DeviceObject pointer to non null");
        ok (DriverObject->Flags == (DRVO_LEGACY_DRIVER | DRVO_INITIALIZED | DRVO_UNLOAD_INVOKED),
            "Expected Flags to be DRVO_LEGACY_DRIVER | DRVO_INITIALIZED | DRVO_UNLOAD_INVOKED, got %lu",
            DriverObject->Flags);
    }
    else
        ASSERT(FALSE);

    /* Select a routine that was not changed */
    FirstMajorFunc = DriverObject->MajorFunction[1];
    ok(FirstMajorFunc != 0, "Expected MajorFunction[1] to be non NULL");

    if (!skip(FirstMajorFunc != NULL, "First major function not set!\n"))
    {
        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        {
            if (DriverStatus > 0) CheckThisDispatchRoutine = (i > 3) && (i != 14);
            else CheckThisDispatchRoutine = TRUE;

            if (CheckThisDispatchRoutine)
            {
                ok(DriverObject->MajorFunction[i] == FirstMajorFunc, "Expected MajorFunction[%d] to match %p",
                    i, FirstMajorFunc);
            }
        }
    }
}

BOOLEAN ZwLoadTest(PDRIVER_OBJECT DriverObject, PUNICODE_STRING DriverRegistryPath, PWCHAR NewDriverRegPath)
{
    UNICODE_STRING RegPath;
    NTSTATUS Status;

    /* Try to load ourself */
    Status = ZwLoadDriver(DriverRegistryPath);
    ok (Status == STATUS_IMAGE_ALREADY_LOADED, "Expected NTSTATUS STATUS_IMAGE_ALREADY_LOADED, got 0x%lX", Status);

    if (Status != STATUS_IMAGE_ALREADY_LOADED)
    {
        DbgPrint("WARNING: Loading this a second time will cause BUGCHECK!\n");
    }

    /* Try to load with a Registry Path that doesnt exist */
    RtlInitUnicodeString(&RegPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\deadbeef");
    Status = ZwLoadDriver(&RegPath);
    ok (Status == STATUS_OBJECT_NAME_NOT_FOUND, "Expected NTSTATUS STATUS_OBJECT_NAME_NOT_FOUND, got 0x%lX", Status);

    /* Load the driver */
    RtlInitUnicodeString(&RegPath, NewDriverRegPath);
    Status = ZwLoadDriver(&RegPath);
    ok(Status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%lX", Status);

    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN ZwUnloadTest(PDRIVER_OBJECT DriverObject, PUNICODE_STRING DriverRegistryPath, PWCHAR NewDriverRegPath)
{
    UNICODE_STRING RegPath;
    NTSTATUS Status;

    /* Try to unload ourself, which should fail as our Unload routine hasnt been set yet. */
    Status = ZwUnloadDriver(DriverRegistryPath);
    ok (Status == STATUS_INVALID_DEVICE_REQUEST, "Expected NTSTATUS STATUS_INVALID_DEVICE_REQUEST, got 0x%lX", Status);

    /* Try to unload with a Registry Path that doesnt exist */
    RtlInitUnicodeString(&RegPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\deadbeef");
    Status = ZwUnloadDriver(&RegPath);
    ok (Status == STATUS_OBJECT_NAME_NOT_FOUND, "Expected NTSTATUS STATUS_OBJECT_NAME_NOT_FOUND, got 0x%lX", Status);

    /* Unload the driver */
    RtlInitUnicodeString(&RegPath, NewDriverRegPath);
    Status = ZwUnloadDriver(&RegPath);
    ok(Status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%lX", Status);

    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    return TRUE;
}

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
