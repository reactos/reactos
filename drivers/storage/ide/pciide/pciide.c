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
	DPRINT("PciIdeChannelEnabled(%p, %lu)\n", DeviceExtension, Channel);

	return ChannelStateUnknown;
}

BOOLEAN NTAPI
PciIdeSyncAccessRequired(
	IN PVOID DeviceExtension)
{
    DPRINT("PciIdeSyncAccessRequired %p\n", DeviceExtension);

	return FALSE;
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
    ULONG SupportedMode;
    USHORT PciCommand;

	if (ControllerProperties->Size != sizeof(IDE_CONTROLLER_PROPERTIES))
		return STATUS_REVISION_MISMATCH;

	ControllerProperties->PciIdeChannelEnabled = PciIdeChannelEnabled;
	ControllerProperties->PciIdeSyncAccessRequired = PciIdeSyncAccessRequired;
	ControllerProperties->IgnoreActiveBitForAtaDevice = FALSE;
	ControllerProperties->AlwaysClearBusMasterInterrupt = TRUE;
	ControllerProperties->PciIdeUseDma = PciIdeUseDma;
	ControllerProperties->AlignmentRequirement = 1;

    PciIdeXGetBusData(DeviceExtension,
                      &PciCommand,
                      FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                      sizeof(PciCommand));
    if (!(PciCommand & PCI_ENABLE_BUS_MASTER))
    {
        SupportedMode = PIO_MODE0 | PIO_MODE1 | PIO_MODE2 | PIO_MODE3 | PIO_MODE4;
    }
    else
    {
        SupportedMode = PIO_MODE0 | PIO_MODE1 | PIO_MODE2 | PIO_MODE3 | PIO_MODE4 |
                        SWDMA_MODE0 | SWDMA_MODE1 | SWDMA_MODE2 |
                        MWDMA_MODE0 | MWDMA_MODE1 | MWDMA_MODE2 |
                        UDMA_MODE0 | UDMA_MODE1 | UDMA_MODE2 |
                        UDMA_MODE3 | UDMA_MODE4 | UDMA_MODE5 | UDMA_MODE6;
    }

	ControllerProperties->SupportedTransferMode[0][0] =
	ControllerProperties->SupportedTransferMode[0][1] =
	ControllerProperties->SupportedTransferMode[1][0] =
	ControllerProperties->SupportedTransferMode[1][1] = SupportedMode;

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
