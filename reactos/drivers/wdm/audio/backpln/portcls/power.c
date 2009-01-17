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
    PCExtension* DeviceExt;
    IAdapterPowerManagement * pPower;

    DPRINT1("PcRegisterAdapterPowerManagement pUnknown %p pvContext %p\n", pUnknown, pvContext);

    if (!pUnknown || !pvContext)
        return STATUS_INVALID_PARAMETER;


    pDeviceObject = (PDEVICE_OBJECT)pvContext;
    DeviceExt = (PCExtension*)pDeviceObject->DeviceExtension;

    Status = pUnknown->lpVtbl->QueryInterface(pUnknown, &IID_IAdapterPowerManagement, (PVOID*)&pPower);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PcRegisterAdapterPowerManagement no IAdapterPowerManagement interface %x\n", Status);
        DeviceExt->AdapterPowerManagement = NULL;
        return STATUS_SUCCESS;
    }

    DeviceExt->AdapterPowerManagement = pPower;
    DPRINT1("PcRegisterAdapterPowerManagement success %x\n", Status);
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
    PCExtension* DeviceExt;

    if (!DeviceObject || !RequestedNewState)
        return STATUS_INVALID_PARAMETER;

    DeviceExt = (PCExtension*)DeviceObject->DeviceExtension;
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

