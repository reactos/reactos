/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/power.c
 * PURPOSE:         Power support functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

/*
 * @implemented
 */

NTSTATUS
NTAPI
PcRegisterAdapterPowerManagement(
    IN  PUNKNOWN pUnknown,
    IN  PVOID pvContext)
{
    NTSTATUS Status;
    PDEVICE_OBJECT pDeviceObject;
    PPCLASS_DEVICE_EXTENSION DeviceExt;
    IAdapterPowerManagement * pPower;

    DPRINT("PcRegisterAdapterPowerManagement pUnknown %p pvContext %p\n", pUnknown, pvContext);
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!pUnknown || !pvContext)
        return STATUS_INVALID_PARAMETER;


    pDeviceObject = (PDEVICE_OBJECT)pvContext;
    DeviceExt = (PPCLASS_DEVICE_EXTENSION)pDeviceObject->DeviceExtension;

    Status = pUnknown->lpVtbl->QueryInterface(pUnknown, &IID_IAdapterPowerManagement, (PVOID*)&pPower);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PcRegisterAdapterPowerManagement no IAdapterPowerManagement interface %x\n", Status);
        DeviceExt->AdapterPowerManagement = NULL;
        return STATUS_SUCCESS;
    }

    DeviceExt->AdapterPowerManagement = pPower;
    DPRINT("PcRegisterAdapterPowerManagement success %x\n", Status);
    return STATUS_SUCCESS;
}


static
VOID
NTAPI
PwrCompletionCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus)
{
    KeSetEvent((PRKEVENT)Context, IO_NO_INCREMENT, FALSE);
}

/*
 * @implemented
 */
NTSTATUS NTAPI
PcRequestNewPowerState(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  DEVICE_POWER_STATE RequestedNewState)
{
    KEVENT Event;
    NTSTATUS Status;
    POWER_STATE PowerState;
    PPCLASS_DEVICE_EXTENSION DeviceExt;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!DeviceObject || !RequestedNewState)
        return STATUS_INVALID_PARAMETER;

    DeviceExt = (PPCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    PowerState.DeviceState = RequestedNewState;
    PowerState.SystemState = PowerSystemUnspecified;

    Status = PoRequestPowerIrp(DeviceExt->PhysicalDeviceObject, IRP_MN_SET_POWER, PowerState, PwrCompletionCallback, (PVOID)&Event, NULL);
    if (NT_SUCCESS(Status))
    {
        KeWaitForSingleObject((PVOID)&Event, Executive, KernelMode, FALSE, NULL);
    }


    return Status;
}

