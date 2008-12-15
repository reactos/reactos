#include "private.h"

const GUID IID_IAdapterPowerManagement;

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

    if (!pUnknown || !pvContext)
        return STATUS_INVALID_PARAMETER;

    Status = pUnknown->lpVtbl->QueryInterface(pUnknown, &IID_IAdapterPowerManagement, (PVOID*)&pPower);
    if (!NT_SUCCESS(Status))
        return Status;

    pDeviceObject = (PDEVICE_OBJECT)pvContext;
    DeviceExt = (PCExtension*)pDeviceObject->DeviceExtension;

    if (DeviceExt->AdapterPowerManagement)
    {
        pPower->lpVtbl->Release(pPower);
        return STATUS_UNSUCCESSFUL;
    }

    DeviceExt->AdapterPowerManagement = pPower;
    return STATUS_SUCCESS;
}
