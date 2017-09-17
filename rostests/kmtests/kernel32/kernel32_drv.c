/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test driver for kernel32 filesystem tests
 * COPYRIGHT:   Copyright 2013-2017 Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

#include "kernel32_test.h"

static KMT_MESSAGE_HANDLER TestMessageHandler;
static KMT_IRP_HANDLER TestDirectoryControl;
static KMT_IRP_HANDLER TestQueryInformation;
static KMT_IRP_HANDLER TestSetInformation;

static UNICODE_STRING ExpectedExpression = RTL_CONSTANT_STRING(L"<not set>");
static WCHAR ExpressionBuffer[MAX_PATH];
static BOOLEAN ExpectingSetAttributes = FALSE;
static ULONG ExpectedSetAttributes = -1;
static BOOLEAN ExpectingQueryAttributes = FALSE;
static ULONG ReturnQueryAttributes = -1;

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

    *DeviceName = L"kernel32";
    *Flags = TESTENTRY_NO_EXCLUSIVE_DEVICE;

    KmtRegisterIrpHandler(IRP_MJ_DIRECTORY_CONTROL, NULL, TestDirectoryControl);
    KmtRegisterIrpHandler(IRP_MJ_QUERY_INFORMATION, NULL, TestQueryInformation);
    KmtRegisterIrpHandler(IRP_MJ_SET_INFORMATION, NULL, TestSetInformation);
    KmtRegisterMessageHandler(0, NULL, TestMessageHandler);

    return Status;
}

VOID
TestUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();
}

static
NTSTATUS
TestMessageHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ControlCode,
    IN PVOID Buffer OPTIONAL,
    IN SIZE_T InLength,
    IN OUT PSIZE_T OutLength)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    switch (ControlCode)
    {
        case IOCTL_EXPECT_EXPRESSION:
        {
            C_ASSERT(sizeof(ExpressionBuffer) <= UNICODE_STRING_MAX_BYTES);
            DPRINT("IOCTL_EXPECT_EXPRESSION, InLength = %lu\n", InLength);
            if (InLength > sizeof(ExpressionBuffer))
                return STATUS_BUFFER_OVERFLOW;

            if (InLength % sizeof(WCHAR) != 0)
                return STATUS_INVALID_PARAMETER;

            RtlInitEmptyUnicodeString(&ExpectedExpression, ExpressionBuffer, sizeof(ExpressionBuffer));
            RtlCopyMemory(ExpressionBuffer, Buffer, InLength);
            ExpectedExpression.Length = (USHORT)InLength;
            DPRINT("IOCTL_EXPECT_EXPRESSION: %wZ\n", &ExpectedExpression);

            break;
        }
        case IOCTL_RETURN_QUERY_ATTRIBUTES:
        {
            DPRINT("IOCTL_RETURN_QUERY_ATTRIBUTES, InLength = %lu\n", InLength);
            if (InLength != sizeof(ULONG))
                return STATUS_INVALID_PARAMETER;

            ReturnQueryAttributes = *(PULONG)Buffer;
            ExpectingQueryAttributes = TRUE;
            DPRINT("IOCTL_RETURN_QUERY_ATTRIBUTES: %lu\n", ReturnQueryAttributes);
            break;
        }
        case IOCTL_EXPECT_SET_ATTRIBUTES:
        {
            DPRINT("IOCTL_EXPECT_SET_ATTRIBUTES, InLength = %lu\n", InLength);
            if (InLength != sizeof(ULONG))
                return STATUS_INVALID_PARAMETER;

            ExpectedSetAttributes = *(PULONG)Buffer;
            ExpectingSetAttributes = TRUE;
            DPRINT("IOCTL_EXPECT_SET_ATTRIBUTES: %lu\n", ExpectedSetAttributes);
            break;
        }
        default:
            return STATUS_NOT_SUPPORTED;
    }

    return Status;
}

static
NTSTATUS
TestDirectoryControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation)
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    PAGED_CODE();

    DPRINT("IRP %x/%x\n", IoStackLocation->MajorFunction, IoStackLocation->MinorFunction);
    ASSERT(IoStackLocation->MajorFunction == IRP_MJ_DIRECTORY_CONTROL);

    ok(IoStackLocation->MinorFunction == IRP_MN_QUERY_DIRECTORY, "Minor function: %u\n", IoStackLocation->MinorFunction);
    if (IoStackLocation->MinorFunction == IRP_MN_QUERY_DIRECTORY)
    {
        ok(IoStackLocation->Parameters.QueryDirectory.FileInformationClass == FileBothDirectoryInformation,
           "FileInformationClass: %d\n", IoStackLocation->Parameters.QueryDirectory.FileInformationClass);
        if (IoStackLocation->Parameters.QueryDirectory.FileInformationClass == FileBothDirectoryInformation)
        {
            ok(RtlEqualUnicodeString(IoStackLocation->Parameters.QueryDirectory.FileName, &ExpectedExpression, FALSE),
               "Expression is '%wZ', expected '%wZ'\n", IoStackLocation->Parameters.QueryDirectory.FileName, &ExpectedExpression);
            RtlZeroMemory(Irp->UserBuffer, IoStackLocation->Parameters.QueryDirectory.Length);
            Status = STATUS_SUCCESS;
        }
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

static
NTSTATUS
TestQueryInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation)
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    PFILE_BASIC_INFORMATION BasicInfo;

    PAGED_CODE();

    DPRINT("IRP %x/%x\n", IoStackLocation->MajorFunction, IoStackLocation->MinorFunction);
    ASSERT(IoStackLocation->MajorFunction == IRP_MJ_QUERY_INFORMATION);

    Irp->IoStatus.Information = 0;

    ok_eq_ulong(IoStackLocation->Parameters.QueryFile.FileInformationClass, FileBasicInformation);
    if (IoStackLocation->Parameters.QueryFile.FileInformationClass == FileBasicInformation)
    {
        ok(ExpectingQueryAttributes, "Unexpected QUERY_INFORMATION call\n");
        BasicInfo = Irp->AssociatedIrp.SystemBuffer;
        BasicInfo->CreationTime.QuadPart = 126011664000000000;
        BasicInfo->LastAccessTime.QuadPart = 130899112800000000;
        BasicInfo->LastWriteTime.QuadPart = 130899112800000000;
        BasicInfo->ChangeTime.QuadPart = 130899112800000000;
        BasicInfo->FileAttributes = ReturnQueryAttributes;
        ReturnQueryAttributes = -1;
        ExpectingQueryAttributes = FALSE;
        Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(*BasicInfo);
    }

    Irp->IoStatus.Status = Status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

static
NTSTATUS
TestSetInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation)
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    PFILE_BASIC_INFORMATION BasicInfo;

    PAGED_CODE();

    DPRINT("IRP %x/%x\n", IoStackLocation->MajorFunction, IoStackLocation->MinorFunction);
    ASSERT(IoStackLocation->MajorFunction == IRP_MJ_SET_INFORMATION);

    ok_eq_ulong(IoStackLocation->Parameters.SetFile.FileInformationClass, FileBasicInformation);
    if (IoStackLocation->Parameters.SetFile.FileInformationClass == FileBasicInformation)
    {
        ok(ExpectingSetAttributes, "Unexpected SET_INFORMATION call\n");
        BasicInfo = Irp->AssociatedIrp.SystemBuffer;
        ok_eq_longlong(BasicInfo->CreationTime.QuadPart, 0LL);
        ok_eq_longlong(BasicInfo->LastAccessTime.QuadPart, 0LL);
        ok_eq_longlong(BasicInfo->LastWriteTime.QuadPart, 0LL);
        ok_eq_longlong(BasicInfo->ChangeTime.QuadPart, 0LL);
        ok_eq_ulong(BasicInfo->FileAttributes, ExpectedSetAttributes);
        ExpectedSetAttributes = -1;
        ExpectingSetAttributes = FALSE;
        Status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
