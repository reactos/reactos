/*
 * PROJECT:     ReactOS AHCI Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Common file
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "pciahci.h"

/* GLOBALS ********************************************************************/

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

IDE_CHANNEL_STATE
NTAPI
PciAhciPortEnabled(
    _In_ PVOID DeviceExtension,
    _In_ ULONG Channel)
{
    PAHCI_DEVICE_EXTENSION AhciExtension = DeviceExtension;

    if (AhciExtension->Hba->PortsImplemented & (1 << Channel))
    {
        DPRINT1("AHCI Port %lu is enabled\n", Channel);
        return ChannelEnabled;
    }

    return ChannelDisabled;
}

BOOLEAN
NTAPI
PciAhciSyncAccessRequired(
    _In_ PVOID DeviceExtension)
{
    return FALSE;
}

NTSTATUS
NTAPI
PciAhciTransferModeSelect(
    _In_ PVOID DeviceExtension,
    _In_ PPCIIDE_TRANSFER_MODE_SELECT XferMode)
{
    return STATUS_SUCCESS;
}

ULONG
NTAPI
PciAhciUseDma(
    _In_ PVOID DeviceExtension,
    _In_ PUCHAR CdbCommand,
    _In_ PUCHAR Slave)
{
    /* Nothing should prevent us to use DMA */
    return 1;
}

NTSTATUS
NTAPI
PciAhciGetControllerProperties(
    _In_ PVOID DeviceExtension,
    _Out_ PIDE_CONTROLLER_PROPERTIES ControllerProperties)
{
    PAHCI_DEVICE_EXTENSION AhciExtension = DeviceExtension;
    ULONG i;

    if (ControllerProperties->Size != sizeof(*ControllerProperties))
        return STATUS_REVISION_MISMATCH;

    ControllerProperties->PciIdeChannelEnabled = PciAhciPortEnabled;
    ControllerProperties->PciIdeSyncAccessRequired = PciAhciSyncAccessRequired;
    ControllerProperties->PciIdeTransferModeSelect = PciAhciTransferModeSelect;
    ControllerProperties->IgnoreActiveBitForAtaDevice = FALSE;
    ControllerProperties->AlwaysClearBusMasterInterrupt = TRUE;
    ControllerProperties->PciIdeUseDma = PciAhciUseDma;
    ControllerProperties->AlignmentRequirement = 1;
    ControllerProperties->DefaultPIO = 0;

    ControllerProperties->SupportedTransferMode[0][0] =
    ControllerProperties->SupportedTransferMode[0][1] =
    ControllerProperties->SupportedTransferMode[1][0] =
    ControllerProperties->SupportedTransferMode[1][1] = 0x7FFFFFFF;

    AhciExtension->Hba->GlobalControl |= AHCI_GHC_AE;

    for (i = 0; i < MAX_AHCI_DEVICES; ++i)
    {
        PAHCI_PORT_REGISTERS Port;

        if (!(AhciExtension->Hba->PortsImplemented & (1 << i)))
            continue;

        Port = &AhciExtension->Hba->Port[i];

        Port->SataError = Port->SataError;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;

    Status = PciIdeXInitialize(DriverObject,
                               RegistryPath,
                               PciAhciGetControllerProperties,
                               sizeof(AHCI_DEVICE_EXTENSION));

    return Status;
}
