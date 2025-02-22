/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Power IRP management test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include "PoIrp.h"

static PDRIVER_OBJECT TestDriverObject;
static KMT_MESSAGE_HANDLER TestMessageHandler;

static PDEVICE_OBJECT DeviceObject1;
static PDEVICE_OBJECT DeviceObject2;
static PDEVICE_OBJECT DeviceObject3;

static
NTSTATUS
CreateTestDevices(
    _In_ PDRIVER_OBJECT DriverObject)
{
    NTSTATUS Status;
    PDEVICE_OBJECT AttachedDevice;

    Status = IoCreateDevice(DriverObject, 0, NULL, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject1);
    if (!NT_SUCCESS(Status))
        return Status;

    DeviceObject1->Flags &= ~DO_DEVICE_INITIALIZING;

    Status = IoCreateDevice(DriverObject, 0, NULL, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject2);
    if (!NT_SUCCESS(Status))
    {
        IoDeleteDevice(DeviceObject1);
        return Status;
    }

    AttachedDevice = IoAttachDeviceToDeviceStack(DeviceObject2, DeviceObject1);
    ok(AttachedDevice == DeviceObject1, "Device attached to %p is %p, expected %p\n", DeviceObject2, AttachedDevice, DeviceObject1);
    if (AttachedDevice == NULL)
    {
        IoDeleteDevice(DeviceObject2);
        IoDeleteDevice(DeviceObject1);
        return STATUS_UNSUCCESSFUL;
    }

    DeviceObject2->Flags &= ~DO_DEVICE_INITIALIZING;

    Status = IoCreateDevice(DriverObject, 0, NULL, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject3);
    if (!NT_SUCCESS(Status))
    {
        IoDetachDevice(DeviceObject1);
        IoDeleteDevice(DeviceObject2);
        IoDeleteDevice(DeviceObject1);
        return Status;
    }

    AttachedDevice = IoAttachDeviceToDeviceStack(DeviceObject3, DeviceObject1);
    ok(AttachedDevice == DeviceObject2, "Device attached to %p is %p, expected %p\n", DeviceObject2, AttachedDevice, DeviceObject2);
    if (AttachedDevice == NULL)
    {
        IoDeleteDevice(DeviceObject3);
        IoDetachDevice(DeviceObject1);
        IoDeleteDevice(DeviceObject2);
        IoDeleteDevice(DeviceObject1);
        return STATUS_UNSUCCESSFUL;
    }

    DeviceObject3->Flags &= ~DO_DEVICE_INITIALIZING;

    return Status;
}

NTSTATUS
TestEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PCUNICODE_STRING RegistryPath,
    _Out_ PCWSTR *DeviceName,
    _Inout_ INT *Flags)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);

    TestDriverObject = DriverObject;

    *DeviceName = L"PoIrp";
    *Flags = TESTENTRY_NO_EXCLUSIVE_DEVICE;

    KmtRegisterMessageHandler(0, NULL, TestMessageHandler);

    return Status;
}

VOID
TestUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE();
}

//
// PoRequestPowerIrp test
//
static KEVENT TestDoneEvent;
static PIRP RequestedPowerIrp;
static PIRP RequestedPowerIrpReturned;

static
VOID
NTAPI
RequestedPowerCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR MinorFunction,
    _In_ POWER_STATE PowerState,
    _In_opt_ PVOID Context,
    _In_ PIO_STATUS_BLOCK IoStatus)
{
    PIRP Irp;
    PIO_STACK_LOCATION IoStackLocation;

    ok_eq_pointer(DeviceObject, DeviceObject2);
    ok_eq_uint(MinorFunction, IRP_MN_SET_POWER);
    ok_eq_uint(PowerState.DeviceState, PowerDeviceD0);
    ok_eq_pointer(Context, &RequestedPowerIrp);
    Irp = CONTAINING_RECORD(IoStatus, IRP, IoStatus);
    ok_eq_pointer(Irp, RequestedPowerIrp);
    ok_eq_ulongptr(IoStatus->Information, 7);
    ok_eq_hex(IoStatus->Status, STATUS_WAIT_3);
    KeSetEvent(&TestDoneEvent, IO_NO_INCREMENT, FALSE);

    ok_eq_uint(Irp->StackCount, 5);
    ok_eq_uint(Irp->CurrentLocation, 4);
    ok_eq_pointer(Irp->Tail.Overlay.Thread, NULL);
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    ok_eq_uint(IoStackLocation->MajorFunction, 0);
    ok_eq_uint(IoStackLocation->MinorFunction, 0);
    ok_eq_pointer(IoStackLocation->CompletionRoutine, NULL);
    ok_eq_pointer(IoStackLocation->Context, NULL);
    ok_eq_pointer(IoStackLocation->Parameters.Others.Argument1, DeviceObject);
    ok_eq_pointer(IoStackLocation->Parameters.Others.Argument2, (PVOID)(ULONG_PTR)MinorFunction);
    ok_eq_pointer(IoStackLocation->Parameters.Others.Argument3, (PVOID)(ULONG_PTR)PowerState.SystemState);
    ok_eq_pointer(IoStackLocation->Parameters.Others.Argument4, Context);
}

static
NTSTATUS
RequestedPowerIrpHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStackLocation)
{
    if (RequestedPowerIrp == NULL)
        RequestedPowerIrp = Irp;
    else
        ok_eq_pointer(Irp, RequestedPowerIrp);

    ok_eq_uint(Irp->StackCount, 5);
    ok_eq_ulongptr(Irp->IoStatus.Information, 0);
    ok_eq_hex(Irp->IoStatus.Status, STATUS_NOT_SUPPORTED);
    ok_eq_pointer(Irp->Tail.Overlay.Thread, NULL);
    ok_eq_uint(IoStackLocation->MajorFunction, IRP_MJ_POWER);
    ok_eq_uint(IoStackLocation->MinorFunction, IRP_MN_SET_POWER);
    ok_eq_pointer(IoStackLocation->Context, RequestedPowerCompletion);
    ok_eq_uint(IoStackLocation->Parameters.Power.Type, DevicePowerState);
    ok_eq_uint(IoStackLocation->Parameters.Power.State.DeviceState, PowerDeviceD0);

    if (DeviceObject == DeviceObject1)
    {
        ok_eq_uint(Irp->CurrentLocation, 3);
        Irp->IoStatus.Information = 7;
        Irp->IoStatus.Status = STATUS_WAIT_3;
        PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }
    else if (DeviceObject == DeviceObject2)
    {
        ok_eq_uint(Irp->CurrentLocation, 3);
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(DeviceObject1, Irp);
    }
    else if (DeviceObject == DeviceObject3)
    {
        ok_eq_uint(Irp->CurrentLocation, 3);
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(DeviceObject2, Irp);
    }
    else
    {
        ok(0, "\n");
        PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;
    }
}

static
VOID
TestPoRequestPowerIrp(VOID)
{
    NTSTATUS Status;
    POWER_STATE PowerState;

    KmtRegisterIrpHandler(IRP_MJ_POWER, NULL, RequestedPowerIrpHandler);

    KeInitializeEvent(&TestDoneEvent, NotificationEvent, FALSE);

    PowerState.DeviceState = PowerDeviceD0;
    Status = PoRequestPowerIrp(DeviceObject2,
                               IRP_MN_SET_POWER,
                               PowerState,
                               RequestedPowerCompletion,
                               &RequestedPowerIrp,
                               &RequestedPowerIrpReturned);
    ok(Status == STATUS_PENDING, "PoRequestPowerIrp returned %lx\n", Status);
    ok_eq_pointer(RequestedPowerIrpReturned, RequestedPowerIrp);

    Status = KeWaitForSingleObject(&TestDoneEvent, Executive, KernelMode, FALSE, NULL);
    ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
    KmtUnregisterIrpHandler(IRP_MJ_POWER, NULL, RequestedPowerIrpHandler);
}


//
// Message handler
//
static
NTSTATUS
TestMessageHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG ControlCode,
    _In_ PVOID Buffer OPTIONAL,
    _In_ SIZE_T InLength,
    _Inout_ PSIZE_T OutLength)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    switch (ControlCode)
    {
        case IOCTL_RUN_TEST:
        {
            Status = CreateTestDevices(TestDriverObject);
            ok_eq_hex(Status, STATUS_SUCCESS);
            if (!NT_SUCCESS(Status))
                return Status;

            TestPoRequestPowerIrp();

            IoDetachDevice(DeviceObject2);
            IoDeleteDevice(DeviceObject3);
            IoDetachDevice(DeviceObject1);
            IoDeleteDevice(DeviceObject2);
            IoDeleteDevice(DeviceObject1);

            break;
        }
        default:
            return STATUS_NOT_SUPPORTED;
    }

    return Status;
}
