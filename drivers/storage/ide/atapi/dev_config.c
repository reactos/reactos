/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Device initialization and configuration code
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* FUNCTIONS ******************************************************************/

static
VOID
AtaDeviceEnterConfig(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    Context->CurrentTaskFile = 0;

    DevExt->Device.DeviceFlags |= DEVICE_PIO_ONLY;
    DevExt->Device.DeviceFlags &= ~(DEVICE_HAS_MEDIA_STATUS | DEVICE_SENSE_DATA_REPORTING);
    DevExt->Device.MultiSectorCount = 0;

    if (IS_ATAPI(&DevExt->Device))
        AtaFsmSetLocalState(Context, CFG_STATE_SET_PIO_TRANSFER);
    else
        AtaFsmSetLocalState(Context, CFG_STATE_SET_GEOMETRY);
}

static
VOID
AtaDeviceSetGeometry(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    if (!(DevExt->Device.DeviceFlags & DEVICE_LBA_MODE))
    {
        Request->Flags = REQUEST_FLAG_SET_DEVICE_REGISTER;
        Request->TimeOut = 5;

        RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
        Request->TaskFile.Command = IDE_COMMAND_SET_DRIVE_PARAMETERS;
        Request->TaskFile.SectorCount = (UCHAR)DevExt->Device.SectorsPerTrack;
        Request->TaskFile.DriveSelect = (UCHAR)(DevExt->Device.Heads - 1) |
                                        DevExt->Device.DeviceSelect;
        AtaFsmIssueCommand(Context);
    }

    AtaFsmSetLocalState(Context, CFG_STATE_SET_PIO_TRANSFER);
}

static
VOID
AtaDeviceSetPioTransferMode(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ULONG Mode;

    if (_BitScanReverse(&Mode, DevExt->TransferModeSelectedBitmap & PIO_ALL) == 0)
        Mode = 0;

    Request->Flags = 0;
    Request->TimeOut = 3;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_SET_FEATURE;
    Request->TaskFile.Feature = IDE_FEATURE_SET_TRANSFER_MODE;
    Request->TaskFile.SectorCount = 0x08 | Mode;
    AtaFsmIssueCommand(Context);
    AtaFsmSetLocalState(Context, CFG_STATE_SET_DMA_TRANSFER);
}

static
VOID
AtaDeviceSetDmaTransferMode(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ULONG Mode;

    if (!_BitScanReverse(&Mode, DevExt->TransferModeSelectedBitmap & ~PIO_ALL))
    {
        AtaFsmSetLocalState(Context, CFG_STATE_SET_MULTIPLE);
        return;
    }

    if (Mode >= UDMA_MODE(0))
    {
        Mode -= UDMA_MODE(0);
        Mode |= 0x40;
    }
    else if (Mode >= MWDMA_MODE(0))
    {
        Mode -= MWDMA_MODE(0);
        Mode |= 0x20;
    }
    else
    {
        Mode -= SWDMA_MODE(0);
        Mode |= 0x10;
    }

    Request->Flags = 0;
    Request->TimeOut = 3;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_SET_FEATURE;
    Request->TaskFile.Feature = IDE_FEATURE_SET_TRANSFER_MODE;
    Request->TaskFile.SectorCount = Mode;
    AtaFsmIssueCommand(Context);
    AtaFsmSetLocalState(Context, CFG_STATE_SET_DMA_TRANSFER_RESULT);
}

static
VOID
AtaDeviceSetDmaTransferModeResult(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    if (Request->SrbStatus == SRB_STATUS_SUCCESS)
    {
        DevExt->Device.DeviceFlags &= ~DEVICE_PIO_ONLY;
    }
    else
    {
        WARN("Failed to set %lx DMA settings for '%s'\n",
             DevExt->TransferModeSelectedBitmap,
             DevExt->FriendlyName);

        DevExt->Device.DeviceFlags &= ~DEVICE_NCQ;
        DevExt->TransferModeSelectedBitmap &= PIO_ALL;
    }

    AtaFsmSetLocalState(Context, CFG_STATE_SET_MULTIPLE);
}

static
VOID
AtaDeviceSetMultipleMode(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    if (IS_ATAPI(&DevExt->Device))
        goto Skip;

    DevExt->Device.MultiSectorCount = AtaDevMaximumSectorsPerDrq(&DevExt->IdentifyDeviceData);
    if (DevExt->Device.MultiSectorCount == 0)
        goto Skip;

    Request->Flags = 0;
    Request->TimeOut = 3;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_SET_MULTIPLE;
    Request->TaskFile.SectorCount = DevExt->Device.MultiSectorCount;
    AtaFsmIssueCommand(Context);
    AtaFsmSetLocalState(Context, CFG_STATE_SET_MULTIPLE_RESULT);
    return;

Skip:
    if (DevExt->GtfDataBuffer)
        AtaFsmSetLocalState(Context, CFG_STATE_SET_ACPI_TASK_FILE);
    else
        AtaFsmSetLocalState(Context, CFG_STATE_LOCK_SECURITY);
}

static
VOID
AtaDeviceSetMultipleModeResult(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    if (Request->SrbStatus != SRB_STATUS_SUCCESS)
        DevExt->Device.MultiSectorCount = 0;

    if (DevExt->GtfDataBuffer)
        AtaFsmSetLocalState(Context, CFG_STATE_SET_ACPI_TASK_FILE);
    else
        AtaFsmSetLocalState(Context, CFG_STATE_LOCK_SECURITY);
}

static
VOID
AtaDeviceSetAcpiTaskFile(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    if (Context->CurrentTaskFile != 0)
    {
        INFO("PORT %lu: GTF[%lu] %s\n",
             GET_PORTDATA(Context)->PortNumber,
             Context->CurrentTaskFile - 1,
             (Request->SrbStatus == SRB_STATUS_SUCCESS) ? "completed" : "aborted");
    }

    if (AtaAcpiSetupNextGtfRequest(DevExt, Request, &Context->CurrentTaskFile))
        AtaFsmIssueCommand(Context);
    else
        AtaFsmSetLocalState(Context, CFG_STATE_LOCK_SECURITY);
}

/*
 * See MSDN note:
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/storage/security-group-commands
 */
static
VOID
AtaDeviceLockSecurityModeFeatureCommands(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    if (!IS_ATAPI(&DevExt->Device) &&
        !AtapInPEMode &&
        AtaDevHasSecurityModeFeature(&DevExt->IdentifyDeviceData))
    {
        Request->Flags = 0;
        Request->TimeOut = 3;

        RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
        Request->TaskFile.Command = IDE_COMMAND_SECURITY_FREEZE_LOCK;
        AtaFsmIssueCommand(Context);
    }
    AtaFsmSetLocalState(Context, CFG_STATE_LOCK_PARAMETERS);
}

static
VOID
AtaDeviceLockDeviceParameters(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    Request->Flags = 0;
    Request->TimeOut = 3;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_SET_FEATURE;
    Request->TaskFile.Feature = IDE_FEATURE_DISABLE_REVERT_TO_POWER_ON;
    AtaFsmIssueCommand(Context);
    AtaFsmSetLocalState(Context, CFG_STATE_ENABLE_MSN);
}

static
VOID
AtaDeviceEnableMsnFeature(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    if (!AtaDevHasRemovableMediaStatusNotification(&DevExt->IdentifyDeviceData))
    {
        AtaFsmSetLocalState(Context, CFG_STATE_UPDATE_IDENTIFY);
        return;
    }

    Request->Flags = 0;
    Request->TimeOut = 3;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_SET_FEATURE;
    Request->TaskFile.Feature = IDE_FEATURE_ENABLE_MSN;
    AtaFsmIssueCommand(Context);
    AtaFsmSetLocalState(Context, CFG_STATE_ENABLE_MSN_RESULT);
}

static
VOID
AtaDeviceEnableMsnFeatureResult(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    if (Request->SrbStatus == SRB_STATUS_SUCCESS)
        DevExt->Device.DeviceFlags |= DEVICE_HAS_MEDIA_STATUS;

    AtaFsmSetLocalState(Context, CFG_STATE_UPDATE_IDENTIFY);
}

static
VOID
AtaDeviceUpdateIdentifyData(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    AtaDeviceSendIdentify(Context,
                          IS_ATAPI(&DevExt->Device) ?
                          IDE_COMMAND_ATAPI_IDENTIFY : IDE_COMMAND_IDENTIFY);

    AtaFsmSetLocalState(Context, CFG_STATE_UPDATE_IDENTIFY_RESULT);
}

static
VOID
AtaDeviceUpdateIdentifyDataResult(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    RtlCopyMemory(&DevExt->IdentifyDeviceData,
                  DevExt->Device.LocalBuffer,
                  sizeof(DevExt->IdentifyDeviceData));

    AtaFsmCompleteDeviceConfigEvent(Context, DevExt);
}

VOID
AtaDeviceConfigRunStateMachine(
    _In_ PATA_WORKER_CONTEXT Context)
{
    PATA_DEVICE_REQUEST Request = Context->InternalRequest;
    PATAPORT_DEVICE_EXTENSION DevExt = Context->DevExt;

    switch (Context->LocalState)
    {
        case CFG_STATE_ENTER:
            AtaDeviceEnterConfig(Context, DevExt);
            break;
        case CFG_STATE_SET_GEOMETRY:
            AtaDeviceSetGeometry(Context, DevExt, Request);
            break;
        case CFG_STATE_SET_PIO_TRANSFER:
            AtaDeviceSetPioTransferMode(Context, DevExt, Request);
            break;
        case CFG_STATE_SET_DMA_TRANSFER:
            AtaDeviceSetDmaTransferMode(Context, DevExt, Request);
            break;
        case CFG_STATE_SET_DMA_TRANSFER_RESULT:
            AtaDeviceSetDmaTransferModeResult(Context, DevExt, Request);
            break;
        case CFG_STATE_SET_MULTIPLE:
            AtaDeviceSetMultipleMode(Context, DevExt, Request);
            break;
        case CFG_STATE_SET_ACPI_TASK_FILE:
            AtaDeviceSetAcpiTaskFile(Context, DevExt, Request);
            break;
        case CFG_STATE_SET_MULTIPLE_RESULT:
            AtaDeviceSetMultipleModeResult(Context, DevExt, Request);
            break;
        case CFG_STATE_LOCK_SECURITY:
            AtaDeviceLockSecurityModeFeatureCommands(Context, DevExt, Request);
            break;
        case CFG_STATE_LOCK_PARAMETERS:
            AtaDeviceLockDeviceParameters(Context, DevExt, Request);
            break;
        case CFG_STATE_ENABLE_MSN:
            AtaDeviceEnableMsnFeature(Context, DevExt, Request);
            break;
        case CFG_STATE_ENABLE_MSN_RESULT:
            AtaDeviceEnableMsnFeatureResult(Context, DevExt, Request);
            break;
        case CFG_STATE_UPDATE_IDENTIFY:
            AtaDeviceUpdateIdentifyData(Context, DevExt, Request);
            break;
        case CFG_STATE_UPDATE_IDENTIFY_RESULT:
            AtaDeviceUpdateIdentifyDataResult(Context, DevExt, Request);
            break;

        default:
            ASSERT(FALSE);
            UNREACHABLE;
    }
}
