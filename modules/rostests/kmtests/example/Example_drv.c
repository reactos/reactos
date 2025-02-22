/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Kernel-Mode Test Suite Example test driver
 * COPYRIGHT:   Copyright 2011-2018 Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

//#define NDEBUG
#include <debug.h>

#include "Example.h"

/* prototypes */
static KMT_MESSAGE_HANDLER TestMessageHandler;
static KMT_IRP_HANDLER TestIrpHandler;

/* globals */
static PDRIVER_OBJECT TestDriverObject;

/**
 * @name TestEntry
 *
 * Test entry point.
 * This is called by DriverEntry as early as possible, but with ResultBuffer
 * initialized, so that test macros work correctly
 *
 * @param DriverObject
 *        Driver Object.
 *        This is guaranteed not to have been touched by DriverEntry before
 *        the call to TestEntry
 * @param RegistryPath
 *        Driver Registry Path
 *        This is guaranteed not to have been touched by DriverEntry before
 *        the call to TestEntry
 * @param DeviceName
 *        Pointer to receive a test-specific name for the device to create
 * @param Flags
 *        Pointer to a flags variable instructing DriverEntry how to proceed.
 *        See the KMT_TESTENTRY_FLAGS enumeration for possible values
 *        Initialized to zero on entry
 *
 * @return Status.
 *         DriverEntry will fail if this is a failure status
 */
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

    DPRINT("Entry!\n");

    ok_irql(PASSIVE_LEVEL);
    TestDriverObject = DriverObject;

    *DeviceName = L"Example";

    trace("Hi, this is the example driver\n");

    KmtRegisterIrpHandler(IRP_MJ_CREATE, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_CLOSE, NULL, TestIrpHandler);
    KmtRegisterMessageHandler(0, NULL, TestMessageHandler);

    return Status;
}

/**
 * @name TestUnload
 *
 * Test unload routine.
 * This is called by the driver's Unload routine as early as possible, with
 * ResultBuffer and the test device object still valid, so that test macros
 * work correctly
 *
 * @param DriverObject
 *        Driver Object.
 *        This is guaranteed not to have been touched by Unload before the call
 *        to TestEntry
 *
 * @return Status
 */
VOID
TestUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    DPRINT("Unload!\n");

    ok_irql(PASSIVE_LEVEL);
    ok_eq_pointer(DriverObject, TestDriverObject);

    trace("Unloading example driver\n");
}

/**
 * @name TestMessageHandler
 *
 * Test message handler routine
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
TestMessageHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ControlCode,
    IN PVOID Buffer OPTIONAL,
    IN SIZE_T InLength,
    IN OUT PSIZE_T OutLength)
{
    NTSTATUS Status = STATUS_SUCCESS;

    switch (ControlCode)
    {
        case IOCTL_NOTIFY:
        {
            static int TimesReceived = 0;

            ++TimesReceived;
            ok(TimesReceived == 1, "Received control code 1 %d times\n", TimesReceived);
            ok_eq_pointer(Buffer, NULL);
            ok_eq_ulong((ULONG)InLength, 0LU);
            ok_eq_ulong((ULONG)*OutLength, 0LU);
            break;
        }
        case IOCTL_SEND_STRING:
        {
            static int TimesReceived = 0;
            ANSI_STRING ExpectedString = RTL_CONSTANT_STRING("yay");
            ANSI_STRING ReceivedString;

            ++TimesReceived;
            ok(TimesReceived == 1, "Received control code 2 %d times\n", TimesReceived);
            ok(Buffer != NULL, "Buffer is NULL\n");
            ok_eq_ulong((ULONG)InLength, (ULONG)ExpectedString.Length);
            ok_eq_ulong((ULONG)*OutLength, 0LU);
            ReceivedString.MaximumLength = ReceivedString.Length = (USHORT)InLength;
            ReceivedString.Buffer = Buffer;
            ok(RtlCompareString(&ExpectedString, &ReceivedString, FALSE) == 0, "Received string: %Z\n", &ReceivedString);
            break;
        }
        case IOCTL_SEND_MYSTRUCT:
        {
            static int TimesReceived = 0;
            MY_STRUCT ExpectedStruct = { 123, ":D" };
            MY_STRUCT ResultStruct = { 456, "!!!" };

            ++TimesReceived;
            ok(TimesReceived == 1, "Received control code 3 %d times\n", TimesReceived);
            ok(Buffer != NULL, "Buffer is NULL\n");
            ok_eq_ulong((ULONG)InLength, (ULONG)sizeof ExpectedStruct);
            ok_eq_ulong((ULONG)*OutLength, 2LU * sizeof ExpectedStruct);
            if (!skip(Buffer && InLength >= sizeof ExpectedStruct, "Cannot read from buffer!\n"))
                ok(RtlCompareMemory(&ExpectedStruct, Buffer, sizeof ExpectedStruct) == sizeof ExpectedStruct, "Buffer does not contain expected values\n");

            if (!skip(Buffer && *OutLength >= 2 * sizeof ExpectedStruct, "Cannot write to buffer!\n"))
            {
                RtlCopyMemory((PCHAR)Buffer + sizeof ExpectedStruct, &ResultStruct, sizeof ResultStruct);
                *OutLength = 2 * sizeof ExpectedStruct;
            }
            break;
        }
        default:
            ok(0, "Got an unknown message! DeviceObject=%p, ControlCode=%lu, Buffer=%p, In=%lu, Out=%lu bytes\n",
                    DeviceObject, ControlCode, Buffer, InLength, *OutLength);
            break;
    }

    return Status;
}

/**
 * @name TestIrpHandler
 *
 * Test IRP handler routine
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
TestIrpHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("IRP!\n");

    ok_irql(PASSIVE_LEVEL);
    ok_eq_pointer(DeviceObject->DriverObject, TestDriverObject);

    if (IoStackLocation->MajorFunction == IRP_MJ_CREATE)
        trace("Got IRP_MJ_CREATE!\n");
    else if (IoStackLocation->MajorFunction == IRP_MJ_CLOSE)
        trace("Got IRP_MJ_CLOSE!\n");
    else
        trace("Got an IRP!\n");

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
