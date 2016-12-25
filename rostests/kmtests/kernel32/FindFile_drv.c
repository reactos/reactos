/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test driver for FindFirstFile's wildcard substitution
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

#include "FindFile.h"

static KMT_MESSAGE_HANDLER TestMessageHandler;
static KMT_IRP_HANDLER TestIrpHandler;

static UNICODE_STRING ExpectedExpression = RTL_CONSTANT_STRING(L"<not set>");
static WCHAR ExpressionBuffer[MAX_PATH];

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

    *DeviceName = L"FindFile";
    *Flags = TESTENTRY_NO_EXCLUSIVE_DEVICE;

    KmtRegisterIrpHandler(IRP_MJ_DIRECTORY_CONTROL, NULL, TestIrpHandler);
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
        case IOCTL_EXPECT:
        {
            C_ASSERT(sizeof(ExpressionBuffer) <= UNICODE_STRING_MAX_BYTES);
            DPRINT("IOCTL_EXPECT, InLength = %lu\n", InLength);
            if (InLength > sizeof(ExpressionBuffer))
                return STATUS_BUFFER_OVERFLOW;

            if (InLength % sizeof(WCHAR) != 0)
                return STATUS_INVALID_PARAMETER;

            RtlInitEmptyUnicodeString(&ExpectedExpression, ExpressionBuffer, sizeof(ExpressionBuffer));
            RtlCopyMemory(ExpressionBuffer, Buffer, InLength);
            ExpectedExpression.Length = (USHORT)InLength;
            DPRINT("IOCTL_EXPECT: %wZ\n", &ExpectedExpression);

            break;
        }
        default:
            return STATUS_NOT_SUPPORTED;
    }

    return Status;
}

static
NTSTATUS
TestIrpHandler(
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
