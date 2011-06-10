/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <ntddk.h>
#include <ntstrsafe.h>
#include <limits.h>

//#define NDEBUG
#include <debug.h>

#include <kmt_public.h>
#include <kmt_log.h>
#include <kmt_test.h>

/* Prototypes */
DRIVER_INITIALIZE DriverEntry;
static DRIVER_UNLOAD DriverUnload;
static DRIVER_DISPATCH DriverCreateClose;
static DRIVER_DISPATCH DriverIoControl;
static DRIVER_DISPATCH DriverRead;

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
    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);

    DPRINT("DriverEntry\n");

    Status = LogInit();

    if (!NT_SUCCESS(Status))
        goto cleanup;

    RtlInitUnicodeString(&DeviceName, L"\\Device\\Kmtest");
    Status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN | FILE_READ_ONLY_DEVICE,
                            TRUE, &MainDeviceObject);

    if (!NT_SUCCESS(Status))
        goto cleanup;

    DPRINT("DriverEntry. Created DeviceObject %p\n",
             MainDeviceObject);
    MainDeviceObject->Flags |= DO_DIRECT_IO;

    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverIoControl;
    DriverObject->MajorFunction[IRP_MJ_READ] = DriverRead;

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
        IoDeleteDevice(MainDeviceObject);

    LogFree();
}

/**
 * @name DriverCreateClose
 *
 * Driver Dispatch function for CreateFile/CloseHandle.
 *
 * @param DeviceObject
 *        Device Object
 * @param Irp
 *        I/O request packet
 *
 * @return Status
 */
static NTSTATUS NTAPI DriverCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation;

    PAGED_CODE();

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("DriverCreateClose. Function=%s, DeviceObject=%p\n",
             IoStackLocation->MajorFunction == IRP_MJ_CREATE ? "Create" : "Close",
             DeviceObject);

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

/**
 * @name DriverRead
 *
 * Driver Dispatch function for ReadFile.
 *
 * @param DeviceObject
 *        Device Object
 * @param Irp
 *        I/O request packet
 *
 * @return Status
 */
static NTSTATUS NTAPI DriverRead(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation;
    PVOID ReadBuffer;
    SIZE_T Length;

    PAGED_CODE();

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("DriverRead. Offset=%I64u, Length=%lu, DeviceObject=%p\n",
             IoStackLocation->Parameters.Read.ByteOffset.QuadPart,
             IoStackLocation->Parameters.Read.Length,
             DeviceObject);

    ReadBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

    Length = LogRead(ReadBuffer, IoStackLocation->Parameters.Read.Length);

    DPRINT("DriverRead. Length of data read: %lu\n",
             Length);

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Length;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
