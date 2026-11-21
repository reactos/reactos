/*
 * PROJECT:     ReactOS VirtIO Sound Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PnP handling
 * COPYRIGHT:   Copyright 2026 Ralf Lang <ralf.lang@ralf-lang.de>
 */

/**
 * @file
 * @brief PnP handling for VirtIO Sound Driver
 *
 * This file handles the Plug and Play (PnP) initialization of the VirtIO sound device.
 *
 * @copyright Copyright 2026 Ralf Lang <ralf.lang@ralf-lang.de>
 * @license GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 */

#include "virtiosnd.h"

/**
 * @brief Initializes the VirtIO sound device
 *
 * This function is called during PnP initialization.
 *
 * It sets up the VirtIO sound device.
 * It also validates the device resources and initializes the transport layer.
 *
 * @param DeviceObject Pointer to the device object
 * @param Irp Pointer to the I/O request packet
 * @param ResourceList Pointer to the resource list
 * @return NTSTATUS Status of the operation
 */
NTSTATUS
NTAPI
VirtioSndStartDevice(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp, _In_ PRESOURCELIST ResourceList)
{
    NTSTATUS Status;

    PAGED_CODE();

    if (DeviceObject == NULL || ResourceList == NULL || ResourceList->lpVtbl == NULL)
    {
        DbgPrint("virtiosnd: StartDevice received incomplete PnP state\n");
        return STATUS_INVALID_PARAMETER;
    }

    ULONG ResourceCount = ResourceList->lpVtbl->NumberOfEntries(ResourceList);
    ULONG MemoryCount = ResourceList->lpVtbl->NumberOfEntriesOfType(ResourceList, CmResourceTypeMemory);
    ULONG PortCount = ResourceList->lpVtbl->NumberOfEntriesOfType(ResourceList, CmResourceTypePort);
    ULONG InterruptCount = ResourceList->lpVtbl->NumberOfEntriesOfType(ResourceList, CmResourceTypeInterrupt);

    if (ResourceCount == 0)
    {
        DbgPrint("virtiosnd: PnP matched the device but assigned no resources\n");
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    DbgPrint(
        "virtiosnd: VirtIO Sound PCI device detected (FDO %p, resources %lu: memory %lu, "
        "ports %lu, interrupts %lu)\n",
        DeviceObject, ResourceCount, MemoryCount, PortCount, InterruptCount);

    Status = VirtioSndStartTransport(DeviceObject, ResourceList);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("virtiosnd: StartDevice transport probe failed with status %08lx\n", (ULONG)Status);
        return Status;
    }

    Status = VirtioSndInstallRenderSubdevice(DeviceObject, Irp, ResourceList);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("virtiosnd: WaveCyclic render subdevice registration failed with status %08lx\n", (ULONG)Status);
        VirtioSndStopTransport(DeviceObject);
    }

    return Status;
}
