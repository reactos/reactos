/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Example Test Driver
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <ntddk.h>
#include <ntifs.h>
#include <ndk/ketypes.h>

#define KMT_DEFINE_TEST_FUNCTIONS
#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

#include <kmt_public.h>

/* types */
typedef struct
{
    UCHAR MajorFunction;
    PDEVICE_OBJECT DeviceObject;
    PKMT_IRP_HANDLER IrpHandler;
} KMT_IRP_HANDLER_ENTRY, *PKMT_IRP_HANDLER_ENTRY;

typedef struct
{
    ULONG ControlCode;
    PDEVICE_OBJECT DeviceObject;
    PKMT_MESSAGE_HANDLER MessageHandler;
} KMT_MESSAGE_HANDLER_ENTRY, *PKMT_MESSAGE_HANDLER_ENTRY;

/* Prototypes */
DRIVER_INITIALIZE DriverEntry;
static DRIVER_UNLOAD DriverUnload;
static DRIVER_DISPATCH DriverDispatch;
static KMT_IRP_HANDLER DeviceControlHandler;

/* Globals */
static PDEVICE_OBJECT TestDeviceObject;
static PDEVICE_OBJECT KmtestDeviceObject;

#define KMT_MAX_IRP_HANDLERS 256
static KMT_IRP_HANDLER_ENTRY IrpHandlers[KMT_MAX_IRP_HANDLERS] = { { 0 } };
#define KMT_MAX_MESSAGE_HANDLERS 256
static KMT_MESSAGE_HANDLER_ENTRY MessageHandlers[KMT_MAX_MESSAGE_HANDLERS] = { { 0 } };

/**
 * @name DriverEntry
 *
 * Driver entry point.
 *
 * @param DriverObject
 *        Driver Object
 * @param RegistryPath
 *        Driver Registry Path
 *
 * @return Status
 */
NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR DeviceNameBuffer[128] = L"\\Device\\Kmtest-";
    UNICODE_STRING KmtestDeviceName;
    PFILE_OBJECT KmtestFileObject;
    PKMT_DEVICE_EXTENSION KmtestDeviceExtension;
    UNICODE_STRING DeviceName;
    PCWSTR DeviceNameSuffix;
    INT Flags = 0;
    int i;
    PKPRCB Prcb;

    PAGED_CODE();

    DPRINT("DriverEntry\n");

    Prcb = KeGetCurrentPrcb();
    KmtIsCheckedBuild = (Prcb->BuildType & PRCB_BUILD_DEBUG) != 0;
    KmtIsMultiProcessorBuild = (Prcb->BuildType & PRCB_BUILD_UNIPROCESSOR) == 0;

    /* get the Kmtest device, so that we get a ResultBuffer pointer */
    RtlInitUnicodeString(&KmtestDeviceName, KMTEST_DEVICE_DRIVER_PATH);
    Status = IoGetDeviceObjectPointer(&KmtestDeviceName, FILE_ALL_ACCESS, &KmtestFileObject, &KmtestDeviceObject);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get Kmtest device object pointer\n");
        goto cleanup;
    }

    Status = ObReferenceObjectByPointer(KmtestDeviceObject, FILE_ALL_ACCESS, NULL, KernelMode);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference Kmtest device object\n");
        goto cleanup;
    }

    ObDereferenceObject(KmtestFileObject);
    KmtestFileObject = NULL;
    KmtestDeviceExtension = KmtestDeviceObject->DeviceExtension;
    ResultBuffer = KmtestDeviceExtension->ResultBuffer;
    DPRINT("KmtestDeviceObject: %p\n", (PVOID)KmtestDeviceObject);
    DPRINT("KmtestDeviceExtension: %p\n", (PVOID)KmtestDeviceExtension);
    DPRINT("Setting ResultBuffer: %p\n", (PVOID)ResultBuffer);

    /* call TestEntry */
    RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);
    DeviceName.MaximumLength = sizeof DeviceNameBuffer;
    TestEntry(DriverObject, RegistryPath, &DeviceNameSuffix, &Flags);

    /* create test device */
    if (!(Flags & TESTENTRY_NO_CREATE_DEVICE))
    {
        RtlAppendUnicodeToString(&DeviceName, DeviceNameSuffix);
        Status = IoCreateDevice(DriverObject, 0, &DeviceName,
                                FILE_DEVICE_UNKNOWN,
                                FILE_DEVICE_SECURE_OPEN |
                                    (Flags & TESTENTRY_NO_READONLY_DEVICE ? 0 : FILE_READ_ONLY_DEVICE),
                                Flags & TESTENTRY_NO_EXCLUSIVE_DEVICE ? FALSE : TRUE,
                                &TestDeviceObject);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Could not create device object %wZ\n", &DeviceName);
            goto cleanup;
        }

        if (Flags & TESTENTRY_BUFFERED_IO_DEVICE)
            TestDeviceObject->Flags |= DO_BUFFERED_IO;

        DPRINT("DriverEntry. Created DeviceObject %p\n",
                 TestDeviceObject);
    }

    /* initialize dispatch functions */
    if (!(Flags & TESTENTRY_NO_REGISTER_UNLOAD))
        DriverObject->DriverUnload = DriverUnload;
    if (!(Flags & TESTENTRY_NO_REGISTER_DISPATCH))
        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i)
            DriverObject->MajorFunction[i] = DriverDispatch;

cleanup:
    if (TestDeviceObject && !NT_SUCCESS(Status))
    {
        IoDeleteDevice(TestDeviceObject);
        TestDeviceObject = NULL;
    }

    if (KmtestDeviceObject && !NT_SUCCESS(Status))
    {
        ObDereferenceObject(KmtestDeviceObject);
        KmtestDeviceObject = NULL;
        if (KmtestFileObject)
            ObDereferenceObject(KmtestFileObject);
    }

    return Status;
}

/**
 * @name DriverUnload
 *
 * Driver cleanup funtion.
 *
 * @param DriverObject
 *        Driver Object
 */
static
VOID
NTAPI
DriverUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverObject);

    DPRINT("DriverUnload\n");

    TestUnload(DriverObject);

    if (TestDeviceObject)
        IoDeleteDevice(TestDeviceObject);

    if (KmtestDeviceObject)
        ObDereferenceObject(KmtestDeviceObject);
}

/**
 * @name KmtRegisterIrpHandler
 *
 * Register a handler with the IRP Dispatcher.
 * If multiple registered handlers match an IRP, it is unspecified which of
 * them is called on IRP reception
 *
 * @param MajorFunction
 *        IRP major function code to be handled
 * @param DeviceObject
 *        Device Object to handle IRPs for.
 *        Can be NULL to indicate any device object
 * @param IrpHandler
 *        Handler function to register.
 *
 * @return Status
 */
NTSTATUS
KmtRegisterIrpHandler(
    IN UCHAR MajorFunction,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PKMT_IRP_HANDLER IrpHandler)
{
    NTSTATUS Status = STATUS_SUCCESS;
    int i;

    if (MajorFunction > IRP_MJ_MAXIMUM_FUNCTION)
    {
        Status = STATUS_INVALID_PARAMETER_1;
        goto cleanup;
    }

    if (IrpHandler == NULL)
    {
        Status = STATUS_INVALID_PARAMETER_3;
        goto cleanup;
    }

    for (i = 0; i < sizeof IrpHandlers / sizeof IrpHandlers[0]; ++i)
        if (IrpHandlers[i].IrpHandler == NULL)
        {
            IrpHandlers[i].MajorFunction = MajorFunction;
            IrpHandlers[i].DeviceObject = DeviceObject;
            IrpHandlers[i].IrpHandler = IrpHandler;
            goto cleanup;
        }

    Status = STATUS_ALLOTTED_SPACE_EXCEEDED;

cleanup:
    return Status;
}

/**
 * @name KmtUnregisterIrpHandler
 *
 * Unregister a handler with the IRP Dispatcher.
 * Parameters must be specified exactly as in the call to
 * KmtRegisterIrpHandler. Only the first matching entry will be removed
 * if multiple exist
 *
 * @param MajorFunction
 *        IRP major function code of the handler to be removed
 * @param DeviceObject
 *        Device Object to of the handler to be removed
 * @param IrpHandler
 *        Handler function of the handler to be removed
 *
 * @return Status
 */
NTSTATUS
KmtUnregisterIrpHandler(
    IN UCHAR MajorFunction,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PKMT_IRP_HANDLER IrpHandler)
{
    NTSTATUS Status = STATUS_SUCCESS;
    int i;

    for (i = 0; i < sizeof IrpHandlers / sizeof IrpHandlers[0]; ++i)
        if (IrpHandlers[i].MajorFunction == MajorFunction &&
                IrpHandlers[i].DeviceObject == DeviceObject &&
                IrpHandlers[i].IrpHandler == IrpHandler)
        {
            IrpHandlers[i].IrpHandler = NULL;
            goto cleanup;
        }

    Status = STATUS_NOT_FOUND;

cleanup:
    return Status;
}

/**
 * @name DriverDispatch
 *
 * Driver Dispatch function
 *
 * @param DeviceObject
 *        Device Object
 * @param Irp
 *        I/O request packet
 *
 * @return Status
 */
static
NTSTATUS
NTAPI
DriverDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation;
    int i;

    PAGED_CODE();

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("DriverDispatch: Function=%s, Device=%p\n",
            KmtMajorFunctionNames[IoStackLocation->MajorFunction],
            DeviceObject);

    for (i = 0; i < sizeof IrpHandlers / sizeof IrpHandlers[0]; ++i)
    {
        if (IrpHandlers[i].MajorFunction == IoStackLocation->MajorFunction &&
                (IrpHandlers[i].DeviceObject == NULL || IrpHandlers[i].DeviceObject == DeviceObject) &&
                IrpHandlers[i].IrpHandler != NULL)
            return IrpHandlers[i].IrpHandler(DeviceObject, Irp, IoStackLocation);
    }

    /* default handler for DeviceControl */
    if (IoStackLocation->MajorFunction == IRP_MJ_DEVICE_CONTROL ||
            IoStackLocation->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL)
        return DeviceControlHandler(DeviceObject, Irp, IoStackLocation);

    /* default handler */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/**
 * @name KmtRegisterMessageHandler
 *
 * Register a handler with the DeviceControl Dispatcher.
 * If multiple registered handlers match a message, it is unspecified which of
 * them is called on message reception.
 * NOTE: message handlers registered with this function will not be called
 *       if a custom IRP handler matching the corresponding IRP is installed!
 *
 * @param ControlCode
 *        Control code to be handled, as passed by the application.
 *        Can be 0 to indicate any control code
 * @param DeviceObject
 *        Device Object to handle IRPs for.
 *        Can be NULL to indicate any device object
 * @param MessageHandler
 *        Handler function to register.
 *
 * @return Status
 */
NTSTATUS
KmtRegisterMessageHandler(
    IN ULONG ControlCode OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PKMT_MESSAGE_HANDLER MessageHandler)
{
    NTSTATUS Status = STATUS_SUCCESS;
    int i;

    if (ControlCode >= 0x400)
    {
        Status = STATUS_INVALID_PARAMETER_1;
        goto cleanup;
    }

    if (MessageHandler == NULL)
    {
        Status = STATUS_INVALID_PARAMETER_2;
        goto cleanup;
    }

    for (i = 0; i < sizeof MessageHandlers / sizeof MessageHandlers[0]; ++i)
        if (MessageHandlers[i].MessageHandler == NULL)
        {
            MessageHandlers[i].ControlCode = ControlCode;
            MessageHandlers[i].DeviceObject = DeviceObject;
            MessageHandlers[i].MessageHandler = MessageHandler;
            goto cleanup;
        }

    Status = STATUS_ALLOTTED_SPACE_EXCEEDED;

cleanup:
    return Status;
}

/**
 * @name KmtUnregisterMessageHandler
 *
 * Unregister a handler with the DeviceControl Dispatcher.
 * Parameters must be specified exactly as in the call to
 * KmtRegisterMessageHandler. Only the first matching entry will be removed
 * if multiple exist
 *
 * @param ControlCode
 *        Control code of the handler to be removed
 * @param DeviceObject
 *        Device Object to of the handler to be removed
 * @param MessageHandler
 *        Handler function of the handler to be removed
 *
 * @return Status
 */
NTSTATUS
KmtUnregisterMessageHandler(
    IN ULONG ControlCode OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PKMT_MESSAGE_HANDLER MessageHandler)
{
    NTSTATUS Status = STATUS_SUCCESS;
    int i;

    for (i = 0; i < sizeof MessageHandlers / sizeof MessageHandlers[0]; ++i)
        if (MessageHandlers[i].ControlCode == ControlCode &&
                MessageHandlers[i].DeviceObject == DeviceObject &&
                MessageHandlers[i].MessageHandler == MessageHandler)
        {
            MessageHandlers[i].MessageHandler = NULL;
            goto cleanup;
        }

    Status = STATUS_NOT_FOUND;

cleanup:
    return Status;
}

/**
 * @name DeviceControlHandler
 *
 * Default IRP_MJ_DEVICE_CONTROL/IRP_MJ_INTERNAL_DEVICE_CONTROL handler
 *
 * @param DeviceObject
 *        Device Object.
 *        This is guaranteed not to have been touched by the dispatch function
 *        before the call to the IRP handler
 * @param Irp
 *        Device Object.
 *        This is guaranteed not to have been touched by the dispatch function
 *        before the call to the IRP handler, except for passing it to
 *        IoGetCurrentStackLocation
 * @param IoStackLocation
 *        Device Object.
 *        This is guaranteed not to have been touched by the dispatch function
 *        before the call to the IRP handler
 *
 * @return Status
 */
static
NTSTATUS
DeviceControlHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ControlCode = (IoStackLocation->Parameters.DeviceIoControl.IoControlCode & 0x00000FFC) >> 2;
    SIZE_T OutLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
    int i;

    for (i = 0; i < sizeof MessageHandlers / sizeof MessageHandlers[0]; ++i)
    {
        if ((MessageHandlers[i].ControlCode == 0 ||
                MessageHandlers[i].ControlCode == ControlCode) &&
                (MessageHandlers[i].DeviceObject == NULL || MessageHandlers[i].DeviceObject == DeviceObject) &&
                MessageHandlers[i].MessageHandler != NULL)
        {
            Status = MessageHandlers[i].MessageHandler(DeviceObject, ControlCode, Irp->AssociatedIrp.SystemBuffer,
                                                        IoStackLocation->Parameters.DeviceIoControl.InputBufferLength,
                                                        &OutLength);
            break;
        }
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = OutLength;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
