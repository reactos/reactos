/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ACPI interface with SATA ports, IDE controllers and drives
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

#include <acpiioct.h>

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
NTSTATUS
AtaAcpiEvaluateObject(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_opt_ PACPI_EVAL_OUTPUT_BUFFER OutputBuffer,
    _In_ ULONG OutputBufferLength)
{
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    KEVENT Event;
    NTSTATUS Status;
    PDEVICE_OBJECT TopDeviceObject;

    PAGED_CODE();

    /* Get the ACPI bus filter device for this DO */
    TopDeviceObject = IoGetAttachedDeviceReference(DeviceObject);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(IOCTL_ACPI_EVAL_METHOD,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        ObDereferenceObject(TopDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

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

    ObDereferenceObject(TopDeviceObject);
    return Status;
}

CODE_SEG("PAGE")
VOID
AtaAcpiSetTimingMode(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIDE_ACPI_TIMING_MODE_BLOCK TimingMode,
    _In_ PIDENTIFY_DEVICE_DATA IdBlock1,
    _In_ PIDENTIFY_DEVICE_DATA IdBlock2)
{
    PACPI_EVAL_INPUT_BUFFER_COMPLEX InputBuffer;
    PACPI_METHOD_ARGUMENT Argument;
    NTSTATUS Status;
    ULONG InputSize;

    PAGED_CODE();

    InputSize = FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX, Argument) +
                ACPI_METHOD_ARGUMENT_LENGTH(sizeof(*TimingMode)) +
                ACPI_METHOD_ARGUMENT_LENGTH(sizeof(*IdBlock1)) +
                ACPI_METHOD_ARGUMENT_LENGTH(sizeof(*IdBlock2));

    InputBuffer = ExAllocatePoolZero(NonPagedPool, InputSize, ATAPORT_TAG);
    if (!InputBuffer)
    {
        ERR("Failed to allocate memory\n");
        return;
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
    ACPI_METHOD_SET_ARGUMENT_BUFFER(Argument, IdBlock1, sizeof(*IdBlock1));

    /* Argument 3: The ATA drive ID block */
    Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    ACPI_METHOD_SET_ARGUMENT_BUFFER(Argument, IdBlock2, sizeof(*IdBlock2));

    /* Evaluate the _STM method */
    Status = AtaAcpiEvaluateObject(ChanExt->Common.Self,
                                   InputBuffer,
                                   InputSize,
                                   NULL,
                                   0);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Failed to set transfer timings, status 0x%lx\n", Status);
    }

    ExFreePoolWithTag(InputBuffer, ATAPORT_TAG);
}

CODE_SEG("PAGE")
BOOLEAN
AtaAcpiGetTimingMode(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Out_ PIDE_ACPI_TIMING_MODE_BLOCK TimingMode)
{
    UCHAR Buffer[FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
                 ACPI_METHOD_ARGUMENT_LENGTH(sizeof(*TimingMode))];
    ACPI_EVAL_INPUT_BUFFER InputBuffer;
    NTSTATUS Status;
    PACPI_EVAL_OUTPUT_BUFFER OutputBuffer = (PACPI_EVAL_OUTPUT_BUFFER)Buffer;

    PAGED_CODE();

    RtlZeroMemory(Buffer, sizeof(Buffer));

    InputBuffer.MethodNameAsUlong = 'MTG_'; // _GTM
    InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    /* Evaluate the _GTM method */
    Status = AtaAcpiEvaluateObject(ChanExt->Common.Self,
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

CODE_SEG("PAGE")
PVOID
AtaAcpiGetTaskFile(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    ACPI_EVAL_INPUT_BUFFER InputBuffer;
    PACPI_EVAL_OUTPUT_BUFFER OutputBuffer;
    ULONG RetryCount, OutputSize;
    NTSTATUS Status;

    PAGED_CODE();

    InputBuffer.MethodNameAsUlong = 'FTG_'; // _GTF
    InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    /* The output buffer must be large enough to hold the list of ATA commands to the drive */
    OutputSize = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
                 ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ATA_ACPI_TASK_FILE) * 10);

    for (RetryCount = 0; RetryCount < 2; ++RetryCount)
    {
        OutputBuffer = ExAllocatePoolZero(NonPagedPool, OutputSize, ATAPORT_TAG);
        if (!OutputBuffer)
            return NULL;

        /* Evaluate the _GTF method */
        Status = AtaAcpiEvaluateObject(DevExt->Common.Self,
                                       &InputBuffer,
                                       sizeof(InputBuffer),
                                       OutputBuffer,
                                       OutputSize);
        if (NT_SUCCESS(Status))
            break;

        /* Increase the allocation size if it's too small */
        if (Status == STATUS_BUFFER_OVERFLOW)
        {
            ExFreePoolWithTag(OutputBuffer, ATAPORT_TAG);

            OutputSize = OutputBuffer->Length;
            continue;
        }

        if (!NT_SUCCESS(Status))
            goto Cleanup;
    }

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

    return OutputBuffer;

Cleanup:
    ExFreePoolWithTag(OutputBuffer, ATAPORT_TAG);

    return NULL;
}

CODE_SEG("PAGE")
VOID
AtaAcpiSetDeviceData(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIDENTIFY_DEVICE_DATA IdBlock)
{
    PACPI_EVAL_INPUT_BUFFER_COMPLEX InputBuffer;
    PACPI_METHOD_ARGUMENT Argument;
    NTSTATUS Status;
    ULONG InputSize;

    PAGED_CODE();

    InputSize = FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX, Argument) +
                ACPI_METHOD_ARGUMENT_LENGTH(sizeof(*IdBlock));

    InputBuffer = ExAllocatePoolZero(NonPagedPool, InputSize, ATAPORT_TAG);
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
    Status = AtaAcpiEvaluateObject(DevExt->Common.Self,
                                   InputBuffer,
                                   InputSize,
                                   NULL,
                                   0);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Failed to set device data, status 0x%lx\n", Status);
    }

    ExFreePoolWithTag(InputBuffer, ATAPORT_TAG);
}

/* static */
CODE_SEG("PAGE")
BOOLEAN
AtaAcpiFilterTaskFile(
    _In_ PATA_ACPI_TASK_FILE AcpiTaskFile)
{
    PAGED_CODE();

    switch (AcpiTaskFile->Command)
    {
        case IDE_COMMAND_SET_FEATURE:
        {
            if (AcpiTaskFile->Feature == IDE_FEATURE_SET_TRANSFER_MODE)
                return FALSE;

            break;
        }

        case IDE_COMMAND_SET_DRIVE_PARAMETERS:
        case IDE_COMMAND_SET_MULTIPLE:
            return FALSE;

        default:
            break;
    }

    return TRUE;
}

#if 0
static
CODE_SEG("PAGE")
VOID
AtaAcpiTaskFileToInternalTaskFile(
    _In_ PATA_ACPI_TASK_FILE AcpiTaskFile,
    _Out_ PATA_TASKFILE TaskFile)
{
    PAGED_CODE();

    TaskFile->Feature = AcpiTaskFile->Feature;
    TaskFile->SectorCount = AcpiTaskFile->SectorCount;
    TaskFile->LowLba = AcpiTaskFile->LowLba;
    TaskFile->MidLba = AcpiTaskFile->MidLba;
    TaskFile->HighLba = AcpiTaskFile->HighLba;
    TaskFile->DriveSelect = AcpiTaskFile->DriveSelect;
    TaskFile->Command = AcpiTaskFile->Command;
}
#endif

CODE_SEG("PAGE")
VOID
AtaAcpiExecuteTaskFile(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PVOID GtfDataBuffer,
    _In_ PATA_TASKFILE TaskFile)
{
    PAGED_CODE();

    // todo
}
