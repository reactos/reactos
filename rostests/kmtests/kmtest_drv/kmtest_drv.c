/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <ntddk.h>
#include <ntifs.h>
#include <ndk/ketypes.h>
#include <ntstrsafe.h>
#include <limits.h>
#include <pseh/pseh2.h>

//#define NDEBUG
#include <debug.h>

#include <kmt_public.h>
#define KMT_DEFINE_TEST_FUNCTIONS
#include <kmt_test.h>

/* Prototypes */
DRIVER_INITIALIZE DriverEntry;
static DRIVER_UNLOAD DriverUnload;
__drv_dispatchType(IRP_MJ_CREATE)
static DRIVER_DISPATCH DriverCreate;
__drv_dispatchType(IRP_MJ_CLEANUP)
static DRIVER_DISPATCH DriverCleanup;
__drv_dispatchType(IRP_MJ_CLOSE)
static DRIVER_DISPATCH DriverClose;
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
static DRIVER_DISPATCH DriverIoControl;

/* Globals */
static PDEVICE_OBJECT MainDeviceObject;
PDRIVER_OBJECT KmtDriverObject = NULL;

/* Entry */
/**
 * @name DriverEntry
 *
 * Driver Entry point.
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
    UNICODE_STRING DeviceName;
    PKMT_DEVICE_EXTENSION DeviceExtension;
    PKPRCB Prcb;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);

    DPRINT("DriverEntry\n");

    Prcb = KeGetCurrentPrcb();
    KmtIsCheckedBuild = (Prcb->BuildType & PRCB_BUILD_DEBUG) != 0;
    KmtIsMultiProcessorBuild = (Prcb->BuildType & PRCB_BUILD_UNIPROCESSOR) == 0;
    KmtDriverObject = DriverObject;

    RtlInitUnicodeString(&DeviceName, KMTEST_DEVICE_DRIVER_PATH);
    Status = IoCreateDevice(DriverObject, sizeof(KMT_DEVICE_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN | FILE_READ_ONLY_DEVICE,
                            FALSE, &MainDeviceObject);

    if (!NT_SUCCESS(Status))
        goto cleanup;

    DPRINT("DriverEntry. Created DeviceObject %p. DeviceExtension %p\n",
             MainDeviceObject, MainDeviceObject->DeviceExtension);
    DeviceExtension = MainDeviceObject->DeviceExtension;
    DeviceExtension->ResultBuffer = NULL;
    DeviceExtension->Mdl = NULL;

    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreate;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DriverCleanup;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverIoControl;

cleanup:
    if (MainDeviceObject && !NT_SUCCESS(Status))
    {
        IoDeleteDevice(MainDeviceObject);
        MainDeviceObject = NULL;
    }

    return Status;
}

/* Dispatch functions */
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

    if (MainDeviceObject)
    {
        PKMT_DEVICE_EXTENSION DeviceExtension = MainDeviceObject->DeviceExtension;
        ASSERT(!DeviceExtension->Mdl);
        ASSERT(!DeviceExtension->ResultBuffer);
        ASSERT(!ResultBuffer);
        IoDeleteDevice(MainDeviceObject);
    }
}

/**
 * @name DriverCreate
 *
 * Driver Dispatch function for IRP_MJ_CREATE
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
DriverCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation;

    PAGED_CODE();

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("DriverCreate. DeviceObject=%p, RequestorMode=%d, FileObject=%p, FsContext=%p, FsContext2=%p\n",
             DeviceObject, Irp->RequestorMode, IoStackLocation->FileObject,
             IoStackLocation->FileObject->FsContext, IoStackLocation->FileObject->FsContext2);

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/**
 * @name DriverCleanup
 *
 * Driver Dispatch function for IRP_MJ_CLEANUP
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
DriverCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation;
    PKMT_DEVICE_EXTENSION DeviceExtension;

    PAGED_CODE();

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("DriverCleanup. DeviceObject=%p, RequestorMode=%d, FileObject=%p, FsContext=%p, FsContext2=%p\n",
             DeviceObject, Irp->RequestorMode, IoStackLocation->FileObject,
             IoStackLocation->FileObject->FsContext, IoStackLocation->FileObject->FsContext2);

    ASSERT(IoStackLocation->FileObject->FsContext2 == NULL);
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->Mdl && IoStackLocation->FileObject->FsContext == DeviceExtension->Mdl)
    {
        MmUnlockPages(DeviceExtension->Mdl);
        IoFreeMdl(DeviceExtension->Mdl);
        DeviceExtension->Mdl = NULL;
        ResultBuffer = DeviceExtension->ResultBuffer = NULL;
    }
    else
    {
        ASSERT(IoStackLocation->FileObject->FsContext == NULL);
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/**
 * @name DriverClose
 *
 * Driver Dispatch function for IRP_MJ_CLOSE
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
DriverClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    DPRINT("DriverClose. DeviceObject=%p, RequestorMode=%d\n",
             DeviceObject, Irp->RequestorMode);

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/**
 * @name DriverIoControl
 *
 * Driver Dispatch function for IRP_MJ_DEVICE_CONTROL
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
DriverIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation;
    ULONG Length = 0;

    PAGED_CODE();

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("DriverIoControl. Code=0x%08X, DeviceObject=%p, FileObject=%p, FsContext=%p, FsContext2=%p\n",
             IoStackLocation->Parameters.DeviceIoControl.IoControlCode,
             DeviceObject, IoStackLocation->FileObject,
             IoStackLocation->FileObject->FsContext, IoStackLocation->FileObject->FsContext2);

    switch (IoStackLocation->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_KMTEST_GET_TESTS:
        {
            PCKMT_TEST TestEntry;
            LPSTR OutputBuffer = Irp->AssociatedIrp.SystemBuffer;
            size_t Remaining = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;

            DPRINT("DriverIoControl. IOCTL_KMTEST_GET_TESTS, outlen=%lu\n",
                     IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength);

            for (TestEntry = TestList; TestEntry->TestName; ++TestEntry)
            {
                RtlStringCbCopyExA(OutputBuffer, Remaining, TestEntry->TestName, &OutputBuffer, &Remaining, 0);
                if (Remaining)
                {
                    *OutputBuffer++ = '\0';
                    --Remaining;
                }
            }
            if (Remaining)
            {
                *OutputBuffer++ = '\0';
                --Remaining;
            }
            Length = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength - Remaining;
            break;
        }
        case IOCTL_KMTEST_RUN_TEST:
        {
            ANSI_STRING TestName;
            PCKMT_TEST TestEntry;

            DPRINT("DriverIoControl. IOCTL_KMTEST_RUN_TEST, inlen=%lu, outlen=%lu\n",
                     IoStackLocation->Parameters.DeviceIoControl.InputBufferLength,
                     IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength);
            TestName.Length = TestName.MaximumLength = (USHORT)min(IoStackLocation->Parameters.DeviceIoControl.InputBufferLength, USHRT_MAX);
            TestName.Buffer = Irp->AssociatedIrp.SystemBuffer;
            DPRINT("DriverIoControl. Run test: %Z\n", &TestName);

            for (TestEntry = TestList; TestEntry->TestName; ++TestEntry)
            {
                ANSI_STRING EntryName;
                if (TestEntry->TestName[0] == '-')
                    RtlInitAnsiString(&EntryName, TestEntry->TestName + 1);
                else
                    RtlInitAnsiString(&EntryName, TestEntry->TestName);

                if (!RtlCompareString(&TestName, &EntryName, FALSE))
                {
                    DPRINT1("DriverIoControl. Starting test %Z\n", &EntryName);
                    TestEntry->TestFunction();
                    DPRINT1("DriverIoControl. Finished test %Z\n", &EntryName);
                    break;
                }
            }

            if (!TestEntry->TestName)
                Status = STATUS_OBJECT_NAME_INVALID;

            break;
        }
        case IOCTL_KMTEST_SET_RESULTBUFFER:
        {
            PKMT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

            DPRINT("DriverIoControl. IOCTL_KMTEST_SET_RESULTBUFFER, buffer=%p, inlen=%lu, outlen=%lu\n",
                    IoStackLocation->Parameters.DeviceIoControl.Type3InputBuffer,
                    IoStackLocation->Parameters.DeviceIoControl.InputBufferLength,
                    IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength);

            if (DeviceExtension->Mdl)
            {
                if (IoStackLocation->FileObject->FsContext != DeviceExtension->Mdl)
                {
                    Status = STATUS_ACCESS_DENIED;
                    break;
                }
                MmUnlockPages(DeviceExtension->Mdl);
                IoFreeMdl(DeviceExtension->Mdl);
                IoStackLocation->FileObject->FsContext = NULL;
                ResultBuffer = DeviceExtension->ResultBuffer = NULL;
            }

            DeviceExtension->Mdl = IoAllocateMdl(IoStackLocation->Parameters.DeviceIoControl.Type3InputBuffer,
                                                    IoStackLocation->Parameters.DeviceIoControl.InputBufferLength,
                                                    FALSE, FALSE, NULL);
            if (!DeviceExtension->Mdl)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            _SEH2_TRY
            {
                MmProbeAndLockPages(DeviceExtension->Mdl, UserMode, IoModifyAccess);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
                IoFreeMdl(DeviceExtension->Mdl);
                DeviceExtension->Mdl = NULL;
                break;
            } _SEH2_END;

            ResultBuffer = DeviceExtension->ResultBuffer = MmGetSystemAddressForMdlSafe(DeviceExtension->Mdl, NormalPagePriority);
            IoStackLocation->FileObject->FsContext = DeviceExtension->Mdl;

            DPRINT("DriverIoControl. ResultBuffer: %ld %ld %ld %ld\n",
                    ResultBuffer->Successes, ResultBuffer->Failures,
                    ResultBuffer->LogBufferLength, ResultBuffer->LogBufferMaxLength);
            break;
        }
        default:
            DPRINT1("DriverIoControl. Invalid IoCtl code 0x%08X\n",
                     IoStackLocation->Parameters.DeviceIoControl.IoControlCode);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Length;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
