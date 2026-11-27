/*
 * PROJECT:     ReactOS VirtIO Sound Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main entry point
 * COPYRIGHT:   Copyright 2026 Ralf Lang <ralf.lang@ralf-lang.de>
 */

/**
 * @file
 * @brief Main entry point for VirtIO Sound Driver
 *
 * This file contains the driver entry point and initialization functions for the VirtIO sound driver.
 *
 * @copyright Copyright 2026 Ralf Lang <ralf.lang@ralf-lang.de>
 * @license GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 */

/* INITGUID: Instantiate the storage for the GUIDs declared by portcls.h */
#define INITGUID
#include "virtiosnd.h"

/**
 * @brief Maximum number of miniports supported
 */
#define VIRTIO_SND_MAX_MINIPORTS 2

/**
 * @brief Pointer to the original PnP dispatch function
 */
static PDRIVER_DISPATCH VirtioSndPortClsPnpDispatch;

/**
 * @brief PnP dispatch function for VirtIO sound driver
 *
 * This function handles PnP IRPs for the VirtIO sound driver. It specifically handles
 * stop and remove device notifications by stopping the transport layer.
 *
 * @param DeviceObject Pointer to the device object
 * @param Irp Pointer to the I/O request packet
 * @return NTSTATUS Status of the operation
 */
static NTSTATUS NTAPI
VirtioSndPnpDispatch(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);

    if (IrpStack->MinorFunction == IRP_MN_STOP_DEVICE || IrpStack->MinorFunction == IRP_MN_REMOVE_DEVICE)
    {
        VirtioSndStopTransport(DeviceObject);
    }

    return VirtioSndPortClsPnpDispatch(DeviceObject, Irp);
}

/**
 * @brief Adds a VirtIO sound device to the system
 *
 * This function is called by the system to add a VirtIO sound device to the driver.
 * It registers the device with the PortCls driver.
 *
 * @param DriverObject Pointer to the driver object
 * @param PhysicalDeviceObject Pointer to the physical device object
 * @return NTSTATUS Status of the operation
 */
static NTSTATUS NTAPI
VirtioSndAddDevice(_In_ PDRIVER_OBJECT DriverObject, _In_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    PAGED_CODE();

    return PcAddAdapterDevice(
        DriverObject, PhysicalDeviceObject, (PCPFNSTARTDEVICE)VirtioSndStartDevice, VIRTIO_SND_MAX_MINIPORTS, 0);
}

/**
 * @brief Driver entry point
 *
 * This is the main entry point for the VirtIO sound driver. It initializes the driver
 * and registers it with the system.
 *
 * @param DriverObject Pointer to the driver object
 * @param RegistryPath Pointer to the registry path
 * @return NTSTATUS Status of the operation
 */
NTSTATUS
NTAPI
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = PcInitializeAdapterDriver(DriverObject, RegistryPath, (PDRIVER_ADD_DEVICE)VirtioSndAddDevice);
    if (!NT_SUCCESS(Status))
        return Status;

    VirtioSndPortClsPnpDispatch = DriverObject->MajorFunction[IRP_MJ_PNP];
    if (VirtioSndPortClsPnpDispatch == NULL)
        return STATUS_UNSUCCESSFUL;

    DriverObject->MajorFunction[IRP_MJ_PNP] = VirtioSndPnpDispatch;
    return STATUS_SUCCESS;
}
