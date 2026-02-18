/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ACPI interface with SATA ports, IDE controllers and drives
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

#include <acpiioct.h>

/* FUNCTIONS ******************************************************************/

_IRQL_requires_max_(DISPATCH_LEVEL)
static
NTSTATUS
AtaAcpiEvaluateObject(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_opt_ PACPI_EVAL_OUTPUT_BUFFER OutputBuffer,
    _In_ ULONG OutputBufferLength)
{
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    KEVENT Event;
    NTSTATUS Status;
    PDEVICE_OBJECT TopDeviceObject;

    /* Get the ACPI bus filter device for this DO */
    TopDeviceObject = IoGetAttachedDeviceReference(DeviceObject);

    /*
     * We could be called at DISPATCH_LEVEL,
     * so use IoAllocateIrp() rather going through IoBuildDeviceIoControlRequest().
     */
    Irp = IoAllocateIrp(TopDeviceObject->StackSize, 0);
    if (!Irp)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }
    Irp->AssociatedIrp.SystemBuffer = InputBuffer;
    Irp->UserBuffer = OutputBuffer;
    Irp->Flags |= IRP_BUFFERED_IO | IRP_INPUT_OPERATION;
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    /*
     * Submit an asynchronous evaluation request.
     * Note that _STM should be evaluated while the device queue has been paused,
     * so we must not cause *any* page faults and use IOCTL_ACPI_EVAL_METHOD.
     * AtaAcpiEvaluateObject() also has to be non-pageable.
     */
    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_ACPI_ASYNC_EVAL_METHOD;
    IoStack->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    IoStack->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    IoSetCompletionRoutine(Irp,
                           PciIdeXPdoCompletionRoutine,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    Status = IoCallDriver(TopDeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    RtlCopyMemory(OutputBuffer, InputBuffer, min(Irp->IoStatus.Information, OutputBufferLength));

    if (OutputBuffer && NT_SUCCESS(Status))
    {
        ASSERT(OutputBuffer->Signature == ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE);

        if ((OutputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) ||
            (OutputBuffer->Count == 0))
        {
            ERR("Invalid ACPI output buffer\n");

            Status = STATUS_ACPI_INVALID_DATA;
        }
    }

Exit:
    ObDereferenceObject(TopDeviceObject);
    return Status;
}

BOOLEAN
AtaAcpiGetTimingMode(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PIDE_ACPI_TIMING_MODE_BLOCK TimingMode)
{
    UCHAR Buffer[FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
                 ACPI_METHOD_ARGUMENT_LENGTH(sizeof(*TimingMode))];
    ACPI_EVAL_INPUT_BUFFER InputBuffer;
    NTSTATUS Status;
    PACPI_EVAL_OUTPUT_BUFFER OutputBuffer = (PACPI_EVAL_OUTPUT_BUFFER)Buffer;

    RtlZeroMemory(Buffer, sizeof(Buffer));

    InputBuffer.MethodNameAsUlong = 'MTG_'; // _GTM
    InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    /* Evaluate the _GTM method */
    Status = AtaAcpiEvaluateObject(DeviceObject,
                                   &InputBuffer,
                                   sizeof(InputBuffer),
                                   OutputBuffer,
                                   sizeof(Buffer));

    if (!NT_SUCCESS(Status))
    {
        ASSERT(Status != STATUS_BUFFER_OVERFLOW);

        TRACE("Failed to evaluate the _GTM method, status 0x%lx\n", Status);
        return FALSE;
    }

    if (OutputBuffer->Argument[0].DataLength < sizeof(*TimingMode))
    {
        ERR("Buffer too small, size %u/%u\n",
            OutputBuffer->Argument[0].DataLength, sizeof(*TimingMode));
        return FALSE;
    }

    if (OutputBuffer->Argument[0].Type != ACPI_METHOD_ARGUMENT_BUFFER)
    {
        ERR("Unexpected method type %u\n", OutputBuffer->Argument[0].Type);
        return FALSE;
    }

    RtlCopyMemory(TimingMode, OutputBuffer->Argument[0].Data, sizeof(*TimingMode));

    return TRUE;
}

NTSTATUS
AtaAcpiSetTimingMode(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIDE_ACPI_TIMING_MODE_BLOCK TimingMode,
    _In_opt_ PIDENTIFY_DEVICE_DATA IdBlock1,
    _In_opt_ PIDENTIFY_DEVICE_DATA IdBlock2)
{
    PACPI_EVAL_INPUT_BUFFER_COMPLEX InputBuffer;
    PACPI_METHOD_ARGUMENT Argument;
    NTSTATUS Status;
    ULONG InputSize;

    InputSize = FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX, Argument) +
                ACPI_METHOD_ARGUMENT_LENGTH(sizeof(*TimingMode)) +
                ACPI_METHOD_ARGUMENT_LENGTH(sizeof(*IdBlock1)) +
                ACPI_METHOD_ARGUMENT_LENGTH(sizeof(*IdBlock2));

    InputBuffer = ExAllocatePoolUninitialized(NonPagedPool, InputSize, TAG_PCIIDEX);
    if (!InputBuffer)
    {
        ERR("Failed to allocate memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    InputBuffer->MethodNameAsUlong = 'MTS_'; // _STM
    InputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    InputBuffer->ArgumentCount = 3;
    InputBuffer->Size = InputSize;

    /* Argument 1: The channel timing information block */
    Argument = InputBuffer->Argument;
    ACPI_METHOD_SET_ARGUMENT_BUFFER(Argument, TimingMode, sizeof(*TimingMode));

    /* Argument 2: The ATA drive ID block */
    Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    Argument->Type = ACPI_METHOD_ARGUMENT_BUFFER;
    Argument->DataLength = sizeof(*IdBlock1);
    if (IdBlock1)
        RtlCopyMemory(Argument->Data, IdBlock1, sizeof(*IdBlock1));
    else
        RtlZeroMemory(Argument->Data, sizeof(*IdBlock1));

    /* Argument 3: The ATA drive ID block */
    Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    Argument->Type = ACPI_METHOD_ARGUMENT_BUFFER;
    Argument->DataLength = sizeof(*IdBlock2);
    if (IdBlock2)
        RtlCopyMemory(Argument->Data, IdBlock2, sizeof(*IdBlock2));
    else
        RtlZeroMemory(Argument->Data, sizeof(*IdBlock2));

    /* Evaluate the _STM method */
    Status = AtaAcpiEvaluateObject(DeviceObject, InputBuffer, InputSize, NULL, 0);
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            INFO("Failed to set transfer timings, status 0x%lx\n", Status);
        else
            ERR("Failed to set transfer timings, status 0x%lx\n", Status);
    }

    ExFreePoolWithTag(InputBuffer, TAG_PCIIDEX);
    return Status;
}

CODE_SEG("PAGE")
PVOID
AtaAcpiGetTaskFile(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    ACPI_EVAL_INPUT_BUFFER InputBuffer;
    PACPI_EVAL_OUTPUT_BUFFER OutputBuffer;
    ULONG RetryCount, OutputSize;
    NTSTATUS Status;

    PAGED_CODE();
    /*
     * We invoke this routine within the PnP START handler only,
     * and thus specify that the code is in a pageable section.
     */
    ASSERT(PsGetCurrentProcess() == PsInitialSystemProcess);

    InputBuffer.MethodNameAsUlong = 'FTG_'; // _GTF
    InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    /*
     * The output buffer must be large enough to hold the list of ATA commands to the drive.
     * We assume that 10 commands is the common case.
     */
    OutputSize = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
                 ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ATA_ACPI_TASK_FILE) * 10);

    for (RetryCount = 0; RetryCount < 2; ++RetryCount)
    {
        OutputBuffer = ExAllocatePoolZero(NonPagedPool, OutputSize, TAG_PCIIDEX);
        if (!OutputBuffer)
        {
            ERR("Failed to allocate memory of size %lu\n", OutputSize);
            return NULL;
        }

        /* Evaluate the _GTF method */
        Status = AtaAcpiEvaluateObject(DeviceObject,
                                       &InputBuffer,
                                       sizeof(InputBuffer),
                                       OutputBuffer,
                                       OutputSize);

        /* Increase the allocation size if it's too small */
        if (Status == STATUS_BUFFER_OVERFLOW)
        {
            OutputSize = OutputBuffer->Length;

            ExFreePoolWithTag(OutputBuffer, TAG_PCIIDEX);
            continue;
        }

        break;
    }

    if (!NT_SUCCESS(Status))
        goto Cleanup;

    if (OutputBuffer->Argument[0].Type != ACPI_METHOD_ARGUMENT_BUFFER)
    {
        ERR("Unexpected method type %u\n", OutputBuffer->Argument[0].Type);
        goto Cleanup;
    }

    if (OutputBuffer->Argument[0].DataLength % sizeof(ATA_ACPI_TASK_FILE))
    {
        ERR("Incorrect command stream length %u\n", OutputBuffer->Argument[0].DataLength);
        goto Cleanup;
    }

    if (OutputBuffer->Argument[0].DataLength == 0)
    {
        WARN("Empty command list\n");
        goto Cleanup;
    }

    return OutputBuffer;

Cleanup:
    ExFreePoolWithTag(OutputBuffer, TAG_PCIIDEX);

    return NULL;
}

CODE_SEG("PAGE")
VOID
AtaAcpiSetDeviceData(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIDENTIFY_DEVICE_DATA IdBlock)
{
    PACPI_EVAL_INPUT_BUFFER_COMPLEX InputBuffer;
    PACPI_METHOD_ARGUMENT Argument;
    NTSTATUS Status;
    ULONG InputSize;

    PAGED_CODE();
    ASSERT(PsGetCurrentProcess() == PsInitialSystemProcess);

    InputSize = FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX, Argument) +
                ACPI_METHOD_ARGUMENT_LENGTH(sizeof(*IdBlock));

    InputBuffer = ExAllocatePoolUninitialized(NonPagedPool, InputSize, TAG_PCIIDEX);
    if (!InputBuffer)
    {
        ERR("Failed to allocate memory\n");
        return;
    }
    InputBuffer->MethodNameAsUlong = 'DDS_'; // _SDD
    InputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    InputBuffer->ArgumentCount = 1;
    InputBuffer->Size = InputSize;

    /* Argument 1: The ATA drive ID block */
    Argument = InputBuffer->Argument;
    ACPI_METHOD_SET_ARGUMENT_BUFFER(Argument, IdBlock, sizeof(*IdBlock));

    /* Evaluate the _SDD method */
    Status = AtaAcpiEvaluateObject(DeviceObject, InputBuffer, InputSize, NULL, 0);
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            TRACE("Failed to set device data, status 0x%lx\n", Status);
        else
            WARN("Failed to set device data, status 0x%lx\n", Status);
    }

    ExFreePoolWithTag(InputBuffer, TAG_PCIIDEX);
}
