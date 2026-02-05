/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Device initialization and configuration code
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

#include <acpiioct.h>

/* FUNCTIONS ******************************************************************/

static
NTSTATUS
AtaDeviceSetGeometry(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;
    NTSTATUS Status;

    if (IS_ATAPI(&DevExt->Device) || (DevExt->Device.DeviceFlags & DEVICE_LBA_MODE))
        return STATUS_SUCCESS;

    Request->Flags = REQUEST_FLAG_SET_DEVICE_REGISTER;
    Request->TimeOut = 5;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_SET_DRIVE_PARAMETERS;
    Request->TaskFile.SectorCount = (UCHAR)DevExt->Device.SectorsPerTrack;
    Request->TaskFile.DriveSelect = (UCHAR)(DevExt->Device.Heads - 1) |
                                    DevExt->Device.DeviceSelect;

    Status = AtaPortSendRequest(PortData, DevExt);
    if (!NT_SUCCESS(Status))
    {
        WARN("CH %lu: Failed to set geometry for '%s'\n",
             PortData->PortNumber,
             DevExt->TransferModeSelectedBitmap,
             DevExt->FriendlyName);
    }

    return Status;
}

static
VOID
AtaDeviceBuildSetTransferModeTaskFile(
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ UCHAR Mode)
{
    Request->Flags = 0;
    Request->TimeOut = 3;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_SET_FEATURE;
    Request->TaskFile.Feature = IDE_FEATURE_SET_TRANSFER_MODE;
    Request->TaskFile.SectorCount = Mode;
}

static
NTSTATUS
AtaDeviceSetPioTransferMode(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;
    NTSTATUS Status;
    ULONG Mode;

    NT_VERIFY(_BitScanReverse(&Mode, DevExt->TransferModeSelectedBitmap & PIO_ALL) != 0);

    Mode = IDE_SET_ADVANCE_PIO_MODE(Mode);
    AtaDeviceBuildSetTransferModeTaskFile(Request, Mode);

    Status = AtaPortSendRequest(PortData, DevExt);
    if (!NT_SUCCESS(Status))
    {
        WARN("CH %lu: Failed to set %lu PIO settings for '%s'\n",
             PortData->PortNumber,
             Mode,
             DevExt->FriendlyName);
    }

    return Status;
}

static
NTSTATUS
AtaDeviceSetDmaTransferMode(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;
    NTSTATUS Status;
    ULONG Mode;

    if (!_BitScanReverse(&Mode, DevExt->TransferModeSelectedBitmap & ~PIO_ALL))
        return STATUS_SUCCESS;

    if (Mode >= UDMA_MODE(0))
        Mode = IDE_SET_UDMA_MODE(Mode - UDMA_MODE(0));
    else if (Mode >= MWDMA_MODE(0))
        Mode = IDE_SET_MWDMA_MODE(Mode - MWDMA_MODE(0));
    else
        Mode = IDE_SET_SWDMA_MODE(Mode - SWDMA_MODE(0));
    AtaDeviceBuildSetTransferModeTaskFile(Request, Mode);

    Status = AtaPortSendRequest(PortData, DevExt);
    if (NT_SUCCESS(Status))
    {
        DevExt->Device.DeviceFlags &= ~DEVICE_PIO_ONLY;
    }
    else
    {
        WARN("CH %lu: Failed to set 0x%lx DMA settings for '%s'\n",
             PortData->PortNumber,
             Mode,
             DevExt->FriendlyName);

        DevExt->Device.DeviceFlags &= ~DEVICE_NCQ;
        DevExt->TransferModeSelectedBitmap &= PIO_ALL;
    }

    return Status;
}

static
NTSTATUS
AtaDeviceSetMultipleMode(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;
    NTSTATUS Status;

    if (IS_ATAPI(&DevExt->Device))
        return STATUS_SUCCESS;

    DevExt->Device.MultiSectorCount = AtaDevMaximumSectorsPerDrq(&DevExt->IdentifyDeviceData);
    if (DevExt->Device.MultiSectorCount == 0)
        return STATUS_SUCCESS;

    Request->Flags = 0;
    Request->TimeOut = 3;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_SET_MULTIPLE;
    Request->TaskFile.SectorCount = DevExt->Device.MultiSectorCount;

    Status = AtaPortSendRequest(PortData, DevExt);
    if (!NT_SUCCESS(Status))
    {
        WARN("CH %lu: Failed to set %u multiple mode for '%s'\n",
             PortData->PortNumber,
             DevExt->Device.MultiSectorCount,
             DevExt->FriendlyName);

        DevExt->Device.MultiSectorCount = 0;
    }

    return Status;
}

static
BOOLEAN
AtaDeviceFilterAcpiTaskFile(
    _In_ PATA_ACPI_TASK_FILE AcpiTaskFile)
{
    switch (AcpiTaskFile->Command)
    {
        /* These features are managed by the driver */
        case IDE_COMMAND_SET_FEATURE:
        {
            if (AcpiTaskFile->Feature == IDE_FEATURE_SET_TRANSFER_MODE)
                return FALSE;
            break;
        }
        case IDE_COMMAND_SET_DRIVE_PARAMETERS:
        case IDE_COMMAND_SET_MULTIPLE:
            return FALSE;

        // TODO: Anything else to check?
        default:
            break;
    }

    return TRUE;
}

static
VOID
AtaDeviceAcpiTaskFileToInternalTaskFile(
    _In_ ATA_ACPI_TASK_FILE* __restrict AcpiTaskFile,
    _Out_ ATA_DEVICE_REQUEST* __restrict Request)
{
    Request->Flags = REQUEST_FLAG_SET_DEVICE_REGISTER;
    Request->TimeOut = 6;

    Request->TaskFile.Feature = AcpiTaskFile->Feature;
    Request->TaskFile.SectorCount = AcpiTaskFile->SectorCount;
    Request->TaskFile.LowLba = AcpiTaskFile->LowLba;
    Request->TaskFile.MidLba = AcpiTaskFile->MidLba;
    Request->TaskFile.HighLba = AcpiTaskFile->HighLba;
    Request->TaskFile.DriveSelect = AcpiTaskFile->DriveSelect;
    Request->TaskFile.Command = AcpiTaskFile->Command;
}

static
NTSTATUS
AtaDeviceExecuteAcpiTaskFile(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PACPI_EVAL_OUTPUT_BUFFER GtfDataBuffer = DevExt->GtfDataBuffer;
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;
    PATA_ACPI_TASK_FILE AcpiTaskFile;
    NTSTATUS Status;
    ULONG i;

    if (!GtfDataBuffer)
        return STATUS_SUCCESS;

    ASSERT(GtfDataBuffer->Signature == ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE);

    AcpiTaskFile = (PATA_ACPI_TASK_FILE)GtfDataBuffer->Argument[0].Data;

    for (i = 0; i < GtfDataBuffer->Argument[0].DataLength / sizeof(*AcpiTaskFile); ++i)
    {
        PCSTR Result;

        if (!AtaDeviceFilterAcpiTaskFile(AcpiTaskFile))
        {
            Result = "filtered out";
        }
        else
        {
            AtaDeviceAcpiTaskFileToInternalTaskFile(AcpiTaskFile, Request);

            Status = AtaPortSendRequest(PortData, DevExt);
            if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
                return Status;
            Result = NT_SUCCESS(Status) ? "completed" : "aborted";
        }
        INFO("CH %lu: GTF[%lu]: %02x:%02x:%02x:%02x:%02x:%02x:%02x -- %s\n",
              PortData->PortNumber,
              i,
              AcpiTaskFile->Feature,
              AcpiTaskFile->SectorCount,
              AcpiTaskFile->LowLba,
              AcpiTaskFile->MidLba,
              AcpiTaskFile->HighLba,
              AcpiTaskFile->DriveSelect,
              AcpiTaskFile->Command,
              Result);

        ++AcpiTaskFile;
    }

    return STATUS_SUCCESS;
}

/*
 * See MSDN note:
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/storage/security-group-commands
 */
static
NTSTATUS
AtaDeviceLockSecurityModeFeatureCommands(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;

    if (IS_ATAPI(&DevExt->Device) ||
        AtapInPEMode ||
        !AtaDevHasSecurityModeFeature(&DevExt->IdentifyDeviceData))
    {
        return STATUS_SUCCESS;
    }

    Request->Flags = 0;
    Request->TimeOut = 3;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_SECURITY_FREEZE_LOCK;

    return AtaPortSendRequest(PortData, DevExt);
}

static
NTSTATUS
AtaDeviceLockDeviceParameters(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;

    Request->Flags = 0;
    Request->TimeOut = 3;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_SET_FEATURE;
    Request->TaskFile.Feature = IDE_FEATURE_DISABLE_REVERT_TO_POWER_ON;

    return AtaPortSendRequest(PortData, DevExt);
}

static
NTSTATUS
AtaDeviceEnableMsnFeature(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;
    NTSTATUS Status;

    if (!AtaDevHasRemovableMediaStatusNotification(&DevExt->IdentifyDeviceData))
        return STATUS_SUCCESS;

    Request->Flags = 0;
    Request->TimeOut = 3;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_SET_FEATURE;
    Request->TaskFile.Feature = IDE_FEATURE_ENABLE_MSN;

    Status = AtaPortSendRequest(PortData, DevExt);
    if (NT_SUCCESS(Status))
    {
        DevExt->Device.DeviceFlags |= DEVICE_HAS_MEDIA_STATUS;
    }

    return Status;
}

NTSTATUS
AtaPortDeviceProcessConfig(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    NTSTATUS Status;

    DevExt->Device.DeviceFlags |= DEVICE_PIO_ONLY;
    DevExt->Device.DeviceFlags &= ~(DEVICE_HAS_MEDIA_STATUS | DEVICE_SENSE_DATA_REPORTING);
    DevExt->Device.MultiSectorCount = 0;

    Status = AtaDeviceSetGeometry(PortData, DevExt);
    if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
        return Status;

    Status = AtaDeviceSetPioTransferMode(PortData, DevExt);
    if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
        return Status;

    Status = AtaDeviceSetDmaTransferMode(PortData, DevExt);
    if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
        return Status;

    Status = AtaDeviceSetMultipleMode(PortData, DevExt);
    if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
        return Status;

    Status = AtaDeviceExecuteAcpiTaskFile(PortData, DevExt);
    if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
        return Status;

    Status = AtaDeviceLockSecurityModeFeatureCommands(PortData, DevExt);
    if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
        return Status;

    Status = AtaDeviceLockDeviceParameters(PortData, DevExt);
    if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
        return Status;

    Status = AtaDeviceEnableMsnFeature(PortData, DevExt);
    if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
        return Status;

    /* Identify data might have changed during the configuration, so update it here */
    Status = AtaDeviceSendIdentify(PortData,
                                   DevExt,
                                   IS_ATAPI(&DevExt->Device) ?
                                   IDE_COMMAND_ATAPI_IDENTIFY : IDE_COMMAND_IDENTIFY);
    if (NT_SUCCESS(Status))
    {
        RtlCopyMemory(&DevExt->IdentifyDeviceData,
                      DevExt->Device.LocalBuffer,
                      sizeof(DevExt->IdentifyDeviceData));
    }

    return Status;
}
