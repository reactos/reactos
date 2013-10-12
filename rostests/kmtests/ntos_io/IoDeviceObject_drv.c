/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver Object Test Driver
 * PROGRAMMER:      Michael Martin <martinmnet@hotmail.com>
 *                  Thomas Faber <thomas.faber@reactos.org>
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

static DRIVER_DISPATCH TestDispatch;
static VOID TestDriverObject(IN PDRIVER_OBJECT DriverObject, IN DRIVER_STATUS DriverStatus);
static BOOLEAN TestZwLoad(IN PDRIVER_OBJECT DriverObject, IN PCUNICODE_STRING DriverRegistryPath, IN PWCHAR NewDriverRegPath);
static BOOLEAN TestZwUnload(IN PDRIVER_OBJECT DriverObject, IN PCUNICODE_STRING DriverRegistryPath, IN PWCHAR NewDriverRegPath);
static VOID TestLowerDeviceKernelAPI(IN PDEVICE_OBJECT DeviceObject);
static VOID TestDeviceCreated(IN PDEVICE_OBJECT DeviceObject, IN BOOLEAN ExclusiveAccess);
static VOID TestDeviceDeletion(IN PDEVICE_OBJECT DeviceObject, IN BOOLEAN Lower, IN BOOLEAN Attached);
static VOID TestDeviceCreateDelete(IN PDRIVER_OBJECT DriverObject);
static VOID TestAttachDevice(IN PDEVICE_OBJECT DeviceObject, IN PWCHAR NewDriverRegPath);
static VOID TestDetachDevice(IN PDEVICE_OBJECT AttachedDevice);

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
    BOOLEAN Ret;
    INT i;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(DeviceName);

    *Flags = TESTENTRY_NO_CREATE_DEVICE | TESTENTRY_NO_REGISTER_DISPATCH;

    ThisDriverObject = DriverObject;

    TestDriverObject(DriverObject, DriverEntry);

    /* Create and delete device, on return MainDeviceObject has been created */
    TestDeviceCreateDelete(DriverObject);

    /* Make sure a device object was created */
    if (!skip(MainDeviceObject != NULL, "Device object creation failed\n"))
    {
        PWCHAR LowerDriverRegPath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Kmtest-IoHelper";

        /* Load driver test and load the lower driver */
        Ret = TestZwLoad(DriverObject, RegistryPath, LowerDriverRegPath);
        if (!skip(Ret, "Failed to load helper driver\n"))
        {
            TestAttachDevice(MainDeviceObject, L"\\Device\\Kmtest-IoHelper");
            if (!skip(AttachDeviceObject != NULL, "No attached device object\n"))
                TestLowerDeviceKernelAPI(MainDeviceObject);

            /* Unload lower driver without detaching from its device */
            TestZwUnload(DriverObject, RegistryPath, LowerDriverRegPath);
            TestLowerDeviceKernelAPI(MainDeviceObject);
        }
    }

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i)
        DriverObject->MajorFunction[i] = NULL;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = TestDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = TestDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = TestDispatch;

    return Status;
}

VOID
TestUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    if (!skip(AttachDeviceObject != NULL, "no attached device object\n"))
    {
        TestDeviceDeletion(MainDeviceObject, FALSE, TRUE);
        TestDeviceDeletion(AttachDeviceObject, TRUE, FALSE);
        TestDetachDevice(AttachDeviceObject);
    }

    TestDeviceDeletion(MainDeviceObject, FALSE, FALSE);
    TestDriverObject(DriverObject, DriverUnload);

    if (MainDeviceObject)
        IoDeleteDevice(MainDeviceObject);
}

static
NTSTATUS
NTAPI
TestDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation;

    PAGED_CODE();

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("TestIrpHandler. Function=%s, DeviceObject=%p, AttachDeviceObject=%p\n",
        KmtMajorFunctionNames[IoStackLocation->MajorFunction],
        DeviceObject,
        AttachDeviceObject);

    if (AttachDeviceObject)
    {
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(AttachDeviceObject, Irp);
        return Status;
    }

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

    ok(DriverObject->Size == sizeof(DRIVER_OBJECT), "Size does not match, got %x\n",DriverObject->Size);
    ok(DriverObject->Type == 4, "Type does not match 4. got %d\n", DriverObject->Type);

    if (DriverStatus == DriverEntry)
    {
        ok(DriverObject->DeviceObject == NULL, "Expected DeviceObject pointer to be 0, got %p\n",
            DriverObject->DeviceObject);
        ok (DriverObject->Flags == DRVO_LEGACY_DRIVER,
            "Expected Flags to be DRVO_LEGACY_DRIVER, got %lu\n",
            DriverObject->Flags);
    }
    else if (DriverStatus == DriverIrp)
    {
        ok(DriverObject->DeviceObject != NULL, "Expected DeviceObject pointer to non null\n");
        ok (DriverObject->Flags == (DRVO_LEGACY_DRIVER | DRVO_INITIALIZED),
            "Expected Flags to be DRVO_LEGACY_DRIVER | DRVO_INITIALIZED, got %lu\n",
            DriverObject->Flags);
    }
    else if (DriverStatus == DriverUnload)
    {
        ok(DriverObject->DeviceObject != NULL, "Expected DeviceObject pointer to non null\n");
        ok (DriverObject->Flags == (DRVO_LEGACY_DRIVER | DRVO_INITIALIZED | DRVO_UNLOAD_INVOKED),
            "Expected Flags to be DRVO_LEGACY_DRIVER | DRVO_INITIALIZED | DRVO_UNLOAD_INVOKED, got %lu\n",
            DriverObject->Flags);
    }
    else
        ASSERT(FALSE);

    /* Select a routine that was not changed */
    FirstMajorFunc = DriverObject->MajorFunction[1];
    ok(FirstMajorFunc != 0, "Expected MajorFunction[1] to be non NULL\n");

    if (!skip(FirstMajorFunc != NULL, "First major function not set!\n"))
    {
        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        {
            if (DriverStatus > 0) CheckThisDispatchRoutine = (i > 3) && (i != 14);
            else CheckThisDispatchRoutine = TRUE;

            if (CheckThisDispatchRoutine)
            {
                ok(DriverObject->MajorFunction[i] == FirstMajorFunc, "Expected MajorFunction[%d] to match %p\n",
                    i, FirstMajorFunc);
            }
        }
    }
}

static
BOOLEAN
TestZwLoad(
    IN PDRIVER_OBJECT DriverObject,
    IN PCUNICODE_STRING DriverRegistryPath,
    IN PWCHAR NewDriverRegPath)
{
    UNICODE_STRING RegPath;
    NTSTATUS Status;

    /* Try to load ourself */
    Status = ZwLoadDriver((PUNICODE_STRING)DriverRegistryPath);
    ok (Status == STATUS_IMAGE_ALREADY_LOADED, "Expected NTSTATUS STATUS_IMAGE_ALREADY_LOADED, got 0x%lX\n", Status);

    if (Status != STATUS_IMAGE_ALREADY_LOADED)
    {
        DbgPrint("WARNING: Loading this a second time will cause BUGCHECK!\n");
    }

    /* Try to load with a Registry Path that doesnt exist */
    RtlInitUnicodeString(&RegPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\deadbeef");
    Status = ZwLoadDriver(&RegPath);
    ok (Status == STATUS_OBJECT_NAME_NOT_FOUND, "Expected NTSTATUS STATUS_OBJECT_NAME_NOT_FOUND, got 0x%lX\n", Status);

    /* Load the driver */
    RtlInitUnicodeString(&RegPath, NewDriverRegPath);
    Status = ZwLoadDriver(&RegPath);
    ok(Status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%lX\n", Status);

    return NT_SUCCESS(Status);
}

static
BOOLEAN
TestZwUnload(
    IN PDRIVER_OBJECT DriverObject,
    IN PCUNICODE_STRING DriverRegistryPath,
    IN PWCHAR NewDriverRegPath)
{
    UNICODE_STRING RegPath;
    NTSTATUS Status;

    /* Try to unload ourself, which should fail as our Unload routine hasnt been set yet. */
    Status = ZwUnloadDriver((PUNICODE_STRING)DriverRegistryPath);
    ok (Status == STATUS_INVALID_DEVICE_REQUEST, "Expected NTSTATUS STATUS_INVALID_DEVICE_REQUEST, got 0x%lX\n", Status);

    /* Try to unload with a Registry Path that doesnt exist */
    RtlInitUnicodeString(&RegPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\deadbeef");
    Status = ZwUnloadDriver(&RegPath);
    ok (Status == STATUS_OBJECT_NAME_NOT_FOUND, "Expected NTSTATUS STATUS_OBJECT_NAME_NOT_FOUND, got 0x%lX\n", Status);

    /* Unload the driver */
    RtlInitUnicodeString(&RegPath, NewDriverRegPath);
    Status = ZwUnloadDriver(&RegPath);
    ok(Status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%lX\n", Status);

    return NT_SUCCESS(Status);
}

static
VOID
TestLowerDeviceKernelAPI(
    IN PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_OBJECT RetObject;

    RetObject = IoGetLowerDeviceObject(DeviceObject);

    ok(RetObject == AttachDeviceObject,
        "Expected an Attached DeviceObject %p, got %p\n", AttachDeviceObject, RetObject);

    if (RetObject)
    {
        ObDereferenceObject(RetObject);
    }

    RetObject = IoGetDeviceAttachmentBaseRef(DeviceObject);
    ok(RetObject == AttachDeviceObject,
        "Expected an Attached DeviceObject %p, got %p\n", AttachDeviceObject, RetObject);

    if (RetObject)
    {
        ObDereferenceObject(RetObject);
    }

}

static
VOID
TestDeviceCreated(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN ExclusiveAccess)
{
    PEXTENDED_DEVOBJ_EXTENSION extdev;

    /* Check the device object members */
    ok(DeviceObject->Type == 3, "Expected Type = 3, got %x\n", DeviceObject->Type);
    ok(DeviceObject->Size == 0xb8, "Expected Size = 0xb8, got %x\n", DeviceObject->Size);
    ok(DeviceObject->ReferenceCount == 0, "Expected ReferenceCount = 0, got %lu\n",
        DeviceObject->ReferenceCount);
    ok(DeviceObject->DriverObject == ThisDriverObject,
        "Expected DriverObject member to match this DriverObject %p, got %p\n",
        ThisDriverObject, DeviceObject->DriverObject);
    ok(DeviceObject->NextDevice == NULL, "Expected NextDevice to be NULL, got %p\n", DeviceObject->NextDevice);
    ok(DeviceObject->AttachedDevice == NULL, "Expected AttachDevice to be NULL, got %p\n", DeviceObject->AttachedDevice);
    ok(DeviceObject->Characteristics == 0, "Expected Characteristics to be 0\n");
    if (ExclusiveAccess)
    {
        ok((DeviceObject->Flags == (DO_DEVICE_HAS_NAME | DO_DEVICE_INITIALIZING | DO_EXCLUSIVE)),
            "Expected Flags DO_DEVICE_HAS_NAME | DO_DEVICE_INITIALIZING | DO_EXCLUSIVE, got %lu\n", DeviceObject->Flags);
    }
    else
    {
        ok((DeviceObject->Flags == (DO_DEVICE_HAS_NAME | DO_DEVICE_INITIALIZING)),
            "Expected Flags DO_DEVICE_HAS_NAME | DO_DEVICE_INITIALIZING, got %lu\n", DeviceObject->Flags);
    }
    ok(DeviceObject->DeviceType == FILE_DEVICE_UNKNOWN,
        "Expected DeviceType to match creation parameter FILE_DEVICE_UNKNWOWN, got %lu\n",
        DeviceObject->DeviceType);
    ok(DeviceObject->ActiveThreadCount == 0, "Expected ActiveThreadCount = 0, got %lu\n", DeviceObject->ActiveThreadCount);

    /* Check the extended extension */
    extdev = (PEXTENDED_DEVOBJ_EXTENSION)DeviceObject->DeviceObjectExtension;
    ok(extdev->ExtensionFlags == 0, "Expected Extended ExtensionFlags to be 0, got %lu\n", extdev->ExtensionFlags);
    ok (extdev->Type == 13, "Expected Type of 13, got %d\n", extdev->Type);
    ok (extdev->Size == 0, "Expected Size of 0, got %d\n", extdev->Size);
    ok (extdev->DeviceObject == DeviceObject, "Expected DeviceOject to match newly created device %p, got %p\n",
        DeviceObject, extdev->DeviceObject);
    ok(extdev->AttachedTo == NULL, "Expected AttachTo to be NULL, got %p\n", extdev->AttachedTo);
    ok(extdev->StartIoCount == 0, "Expected StartIoCount = 0, got %lu\n", extdev->StartIoCount);
    ok(extdev->StartIoKey == 0, "Expected StartIoKey = 0, got %lu\n", extdev->StartIoKey);
    ok(extdev->StartIoFlags == 0, "Expected StartIoFlags = 0, got %lu\n", extdev->StartIoFlags);
}

static
VOID
TestDeviceDeletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Lower,
    IN BOOLEAN Attached)
{
    PEXTENDED_DEVOBJ_EXTENSION extdev;

    /* Check the device object members */
    ok(DeviceObject->Type == 3, "Expected Type = 3, got %d\n", DeviceObject->Type);
    ok(DeviceObject->Size == 0xb8, "Expected Size = 0xb8, got %d\n", DeviceObject->Size);
    ok(DeviceObject->ReferenceCount == 0, "Expected ReferenceCount = 0, got %lu\n",
        DeviceObject->ReferenceCount);
    if (!Lower)
    {
        ok(DeviceObject->DriverObject == ThisDriverObject,
            "Expected DriverObject member to match this DriverObject %p, got %p\n",
            ThisDriverObject, DeviceObject->DriverObject);
    }
    ok(DeviceObject->NextDevice == NULL, "Expected NextDevice to be NULL, got %p\n", DeviceObject->NextDevice);

    if (Lower)
    {
        ok(DeviceObject->AttachedDevice == MainDeviceObject,
            "Expected AttachDevice to be %p, got %p\n", MainDeviceObject, DeviceObject->AttachedDevice);
    }
    else
    {
        ok(DeviceObject->AttachedDevice == NULL, "Expected AttachDevice to be NULL, got %p\n", DeviceObject->AttachedDevice);
    }

    ok(DeviceObject->Flags == (DO_DEVICE_HAS_NAME | (Lower ? DO_EXCLUSIVE : 0)),
        "Expected Flags DO_DEVICE_HAS_NAME, got %lu\n", DeviceObject->Flags);
    ok(DeviceObject->DeviceType == FILE_DEVICE_UNKNOWN,
        "Expected DeviceType to match creation parameter FILE_DEVICE_UNKNWOWN, got %lu\n",
        DeviceObject->DeviceType);
    ok(DeviceObject->ActiveThreadCount == 0, "Expected ActiveThreadCount = 0, got %lu\n", DeviceObject->ActiveThreadCount);

    /*Check the extended extension */
    extdev = (PEXTENDED_DEVOBJ_EXTENSION)DeviceObject->DeviceObjectExtension;
    ok(extdev->ExtensionFlags == DOE_UNLOAD_PENDING,
        "Expected Extended ExtensionFlags to be DOE_UNLOAD_PENDING, got %lu\n", extdev->ExtensionFlags);
    ok (extdev->Type == 13, "Expected Type of 13, got %d\n", extdev->Type);
    ok (extdev->Size == 0, "Expected Size of 0, got %d\n", extdev->Size);
    ok (extdev->DeviceObject == DeviceObject, "Expected DeviceOject to match newly created device %p, got %p\n",
        DeviceObject, extdev->DeviceObject);
    if (Lower || !Attached)
    {
        ok(extdev->AttachedTo == NULL, "Expected AttachTo to be NULL, got %p\n", extdev->AttachedTo);
    }
    else
    {
        ok(extdev->AttachedTo == AttachDeviceObject, "Expected AttachTo to %p, got %p\n", AttachDeviceObject, extdev->AttachedTo);
    }
    ok(extdev->StartIoCount == 0, "Expected StartIoCount = 0, got %lu\n", extdev->StartIoCount);
    ok(extdev->StartIoKey == 0, "Expected StartIoKey = 0, got %lu\n", extdev->StartIoKey);
    ok(extdev->StartIoFlags == 0, "Expected StartIoFlags = 0, got %lu\n", extdev->StartIoFlags);
}

static
VOID
TestDeviceCreateDelete(
    IN PDRIVER_OBJECT DriverObject)
{
    NTSTATUS Status;
    UNICODE_STRING DeviceString;
    PDEVICE_OBJECT DeviceObject;

    /* Create using wrong directory */
    RtlInitUnicodeString(&DeviceString, L"\\Device1\\Kmtest-IoDeviceObject");
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceString,
                            FILE_DEVICE_UNKNOWN,
                            0,
                            FALSE,
                            &DeviceObject);
    ok(Status == STATUS_OBJECT_PATH_NOT_FOUND, "Expected STATUS_OBJECT_PATH_NOT_FOUND, got 0x%lX\n", Status);

    /* Create using correct params with exclusice access */
    RtlInitUnicodeString(&DeviceString, L"\\Device\\Kmtest-IoDeviceObject");
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceString,
                            FILE_DEVICE_UNKNOWN,
                            0,
                            TRUE,
                            &DeviceObject);
    ok(Status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%lX\n", Status);

    TestDeviceCreated(DeviceObject, TRUE);

    /* Delete the device */
    if (NT_SUCCESS(Status))
    {
        IoDeleteDevice(DeviceObject);
        ok(DriverObject->DeviceObject == 0, "Expected DriverObject->DeviceObject to be NULL, got %p\n",
            DriverObject->DeviceObject);
    }

    /* Create using correct params without exclusice access */
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceString,
                            FILE_DEVICE_UNKNOWN,
                            0,
                            FALSE,
                            &DeviceObject);
    ok(Status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%lX\n", Status);

    TestDeviceCreated(DeviceObject, FALSE);

    /* Delete the device */
    if (NT_SUCCESS(Status))
    {
        IoDeleteDevice(DeviceObject);
        ok(DriverObject->DeviceObject == 0, "Expected DriverObject->DeviceObject to be NULL, got %p\n",
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
    ok(Status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%lX\n", Status);

    if (NT_SUCCESS(Status))
        MainDeviceObject = DeviceObject;
}

static
VOID
TestAttachDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWCHAR NewDriverRegPath)
{
    NTSTATUS Status;
    UNICODE_STRING LowerDeviceName;

    RtlInitUnicodeString(&LowerDeviceName, NewDriverRegPath);
    Status = IoAttachDevice(DeviceObject, &LowerDeviceName, &AttachDeviceObject);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* TODO: Add more tests */
}

static
VOID
TestDetachDevice(
    IN PDEVICE_OBJECT AttachedDevice)
{
    IoDetachDevice(AttachedDevice);

    /* TODO: Add more tests */
}
