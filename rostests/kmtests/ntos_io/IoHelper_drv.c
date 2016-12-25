/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite I/O Test Helper driver
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

//#define NDEBUG
#include <debug.h>

static KMT_IRP_HANDLER TestIrpHandler;

NTSTATUS
TestEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PCUNICODE_STRING RegistryPath,
    OUT PCWSTR *DeviceName,
    IN OUT INT *Flags)
{
    NTSTATUS Status = STATUS_SUCCESS;
    INT i;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);
    UNREFERENCED_PARAMETER(Flags);

    DPRINT("TestEntry. DriverObject=%p, RegistryPath=%wZ\n", DriverObject, RegistryPath);

    *DeviceName = L"IoHelper";

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i)
        KmtRegisterIrpHandler(i, NULL, TestIrpHandler);

    return Status;
}

VOID
TestUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverObject);

    DPRINT("TestUnload. DriverObject=%p\n", DriverObject);
}

static
NTSTATUS
TestIrpHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("TestIrpHandler. Function=%s, DeviceObject=%p\n",
        KmtMajorFunctionNames[IoStackLocation->MajorFunction],
        DeviceObject);

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
