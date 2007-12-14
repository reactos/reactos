/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         PCI IDE bus driver
 * FILE:            drivers/storage/pciide/pciide.c
 * PURPOSE:         Main file
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "pciide.h"

#define NDEBUG
#include <debug.h>

IDE_CHANNEL_STATE NTAPI
PciIdeChannelEnabled(
	IN PVOID DeviceExtension,
	IN ULONG Channel)
{
	PCI_COMMON_CONFIG PciConfig;
	NTSTATUS Status;

	DPRINT("PciIdeChannelEnabled(%p, %lu)\n", DeviceExtension, Channel);

	Status = PciIdeXGetBusData(
		DeviceExtension,
		&PciConfig,
		0,
		PCI_COMMON_HDR_LENGTH);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("PciIdeXGetBusData() failed with status 0x%08lx\n", Status);
		return ChannelStateUnknown;
	}

	if (PCI_CONFIGURATION_TYPE(&PciConfig) != PCI_DEVICE_TYPE)
	{
		DPRINT("Wrong PCI card type. Disabling IDE channel #%lu\n", Channel);
		return ChannelDisabled;
	}

	if (PciConfig.BaseClass != PCI_CLASS_MASS_STORAGE_CTLR || PciConfig.SubClass != PCI_SUBCLASS_MSC_IDE_CTLR)
	{
		DPRINT("Wrong PCI card base class/sub class. Disabling IDE channel #%lu\n", Channel);
		return ChannelDisabled;
	}

	return ChannelStateUnknown;
}

BOOLEAN NTAPI
PciIdeSyncAccessRequired(
	IN PVOID DeviceExtension)
{
	DPRINT1("PciIdeSyncAccessRequired %p\n", DeviceExtension);

	return FALSE; /* FIXME */
}

NTSTATUS NTAPI
PciIdeTransferModeSelect(
	IN PVOID DeviceExtension,
	IN PPCIIDE_TRANSFER_MODE_SELECT XferMode)
{
	ULONG i;

	DPRINT1("PciIdeTransferModeSelect(%p %p)\n", DeviceExtension, XferMode);

	for (i = 0; i < MAX_IDE_DEVICE; i++)
		XferMode->DevicePresent[i] = FALSE; /* FIXME */

	return STATUS_SUCCESS;
}

ULONG NTAPI
PciIdeUseDma(
	IN PVOID DeviceExtension,
	IN PUCHAR CdbCommand,
	IN PUCHAR Slave)
{
	DPRINT("PciIdeUseDma(%p %p %p)\n", DeviceExtension, CdbCommand, Slave);

	/* Nothing should prevent us to use DMA */
	return 1;
}

NTSTATUS NTAPI
PciIdeGetControllerProperties(
	IN PVOID DeviceExtension,
	OUT PIDE_CONTROLLER_PROPERTIES ControllerProperties)
{
	if (ControllerProperties->Size != sizeof(IDE_CONTROLLER_PROPERTIES))
		return STATUS_REVISION_MISMATCH;

	ControllerProperties->PciIdeChannelEnabled = PciIdeChannelEnabled;
	ControllerProperties->PciIdeSyncAccessRequired = PciIdeSyncAccessRequired;
	ControllerProperties->PciIdeTransferModeSelect = PciIdeTransferModeSelect;
	ControllerProperties->IgnoreActiveBitForAtaDevice = FALSE;
	ControllerProperties->AlwaysClearBusMasterInterrupt = TRUE;
	ControllerProperties->PciIdeUseDma = PciIdeUseDma;
	ControllerProperties->AlignmentRequirement = 1;
	ControllerProperties->DefaultPIO = 0; /* FIXME */
	ControllerProperties->PciIdeUdmaModesSupported = NULL; /* optional */
	
	ControllerProperties->SupportedTransferMode[0][0] =
	ControllerProperties->SupportedTransferMode[0][1] =
	ControllerProperties->SupportedTransferMode[1][0] =
	ControllerProperties->SupportedTransferMode[1][0] =
		PIO_MODE0 | PIO_MODE1 | PIO_MODE2 | PIO_MODE3 | PIO_MODE4 |
		SWDMA_MODE0 | SWDMA_MODE1 | SWDMA_MODE2 |
		MWDMA_MODE0 | MWDMA_MODE1 | MWDMA_MODE2 |
		UDMA_MODE0 | UDMA_MODE1 | UDMA_MODE2 | UDMA_MODE3 | UDMA_MODE4;

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS Status;

	Status = PciIdeXInitialize(
		DriverObject,
		RegistryPath,
		PciIdeGetControllerProperties,
		0);

	return Status;
}
