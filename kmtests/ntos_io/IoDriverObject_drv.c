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

    *DeviceName = L"IoDriverObject";

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
