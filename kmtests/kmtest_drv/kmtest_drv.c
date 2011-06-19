/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <ntddk.h>
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
static DRIVER_DISPATCH DriverCreate;
static DRIVER_DISPATCH DriverClose;
static DRIVER_DISPATCH DriverIoControl;

/* Device Extension layout */
typedef struct
{
    PKMT_RESULTBUFFER ResultBuffer;
    PMDL Mdl;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/* Globals */
static PDEVICE_OBJECT MainDeviceObject;

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
NTSTATUS NTAPI DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING DeviceName;
    PDEVICE_EXTENSION DeviceExtension;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);

    DPRINT("DriverEntry\n");

    RtlInitUnicodeString(&DeviceName, L"\\Device\\Kmtest");
    Status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), &DeviceName,
                            FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN | FILE_READ_ONLY_DEVICE,
                            TRUE, &MainDeviceObject);

    if (!NT_SUCCESS(Status))
        goto cleanup;

    DPRINT("DriverEntry. Created DeviceObject %p\n",
             MainDeviceObject);
    DeviceExtension = MainDeviceObject->DeviceExtension;
    DeviceExtension->ResultBuffer = NULL;
    DeviceExtension->Mdl = NULL;

    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreate;
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
static VOID NTAPI DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverObject);

    DPRINT("DriverUnload\n");

    if (MainDeviceObject)
    {
        PDEVICE_EXTENSION DeviceExtension = MainDeviceObject->DeviceExtension;
        ASSERT(!DeviceExtension->Mdl);
        ASSERT(!DeviceExtension->ResultBuffer);
        ASSERT(!ResultBuffer);
        IoDeleteDevice(MainDeviceObject);
    }
}

/**
 * @name DriverCreate
 *
 * Driver Dispatch function for CreateFile
 *
 * @param DeviceObject
 *        Device Object
 * @param Irp
 *        I/O request packet
 *
 * @return Status
 */
static NTSTATUS NTAPI DriverCreate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation;
    PDEVICE_EXTENSION DeviceExtension;

    PAGED_CODE();

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("DriverCreate. DeviceObject=%p\n",
             DeviceObject);

    DeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(!DeviceExtension->Mdl);
    ASSERT(!DeviceExtension->ResultBuffer);
    ASSERT(!ResultBuffer);

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/**
 * @name DriverClose
 *
 * Driver Dispatch function for CloseHandle.
 *
 * @param DeviceObject
 *        Device Object
 * @param Irp
 *        I/O request packet
 *
 * @return Status
 */
static NTSTATUS NTAPI DriverClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation;
    PDEVICE_EXTENSION DeviceExtension;

    PAGED_CODE();

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("DriverClose. DeviceObject=%p\n",
             DeviceObject);

    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->Mdl)
    {
        MmUnlockPages(DeviceExtension->Mdl);
        IoFreeMdl(DeviceExtension->Mdl);
        DeviceExtension->Mdl = NULL;
        ResultBuffer = DeviceExtension->ResultBuffer = NULL;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/**
 * @name DriverIoControl
 *
 * Driver Dispatch function for DeviceIoControl.
 *
 * @param DeviceObject
 *        Device Object
 * @param Irp
 *        I/O request packet
 *
 * @return Status
 */
static NTSTATUS NTAPI DriverIoControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation;
    ULONG Length = 0;

    PAGED_CODE();

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("DriverIoControl. Code=0x%08X, DeviceObject=%p\n",
             IoStackLocation->Parameters.DeviceIoControl.IoControlCode,
             DeviceObject);

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
            PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

            DPRINT("DriverIoControl. IOCTL_KMTEST_SET_RESULTBUFFER, inlen=%lu, outlen=%lu\n",
                     IoStackLocation->Parameters.DeviceIoControl.InputBufferLength,
                     IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength);

            if (DeviceExtension->Mdl)
            {
                MmUnlockPages(DeviceExtension->Mdl);
                IoFreeMdl(DeviceExtension->Mdl);
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
                MmProbeAndLockPages(DeviceExtension->Mdl, KernelMode, IoModifyAccess);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
                IoFreeMdl(DeviceExtension->Mdl);
                DeviceExtension->Mdl = NULL;
                break;
            } _SEH2_END;

            ResultBuffer = DeviceExtension->ResultBuffer = MmGetSystemAddressForMdlSafe(DeviceExtension->Mdl, NormalPagePriority);

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
