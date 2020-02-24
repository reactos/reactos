#include "common/fxglobals.h"
#include "common/fxdevice.h"
#include "common/fxdeviceinterface.h"
#include "common/fxvalidatefunctions.h"
#include "wdf.h"

extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceCreateDeviceInterface)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    CONST GUID *InterfaceClassGUID,
    __in_opt
    PCUNICODE_STRING ReferenceString
    )
/*++

Routine Description:
    Creates a device interface associated with the passed in device object

Arguments:
    Device - Handle which represents the device exposing the interface

    InterfaceGUID - GUID describing the interface being exposed

    ReferenceString - OPTIONAL string which allows the driver writer to
        distinguish between different exposed interfaces

Return Value:
    STATUS_SUCCESS or appropriate NTSTATUS code

  --*/
{
    DDI_ENTRY();
        
    SINGLE_LIST_ENTRY **ppPrev, *pCur;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDeviceInterface *pDeviceInterface;
    FxDevice *pDevice;
    FxPkgPnp* pPkgPnp;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID*) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, InterfaceClassGUID);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (ReferenceString != NULL)
    {
        status = FxValidateUnicodeString(pFxDriverGlobals, ReferenceString);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    if (pDevice->IsLegacy())
    {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "WDFDEVICE %p is not a PNP device, device interface creation not "
            "allowed %!STATUS!", Device, status);

        return status;
    }

    pDeviceInterface = new(pFxDriverGlobals, PagedPool) FxDeviceInterface();

    if (pDeviceInterface == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "WDFDEVICE %p DeviceInterface object creation failed, %!STATUS!",
            Device, status);

        return status;
    }

    status = pDeviceInterface->Initialize(pFxDriverGlobals,
                                          InterfaceClassGUID,
                                          ReferenceString);

    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "WDFDEVICE %p, DeviceInterface object initialization failed, %!STATUS!",
            Device, status );

        goto Done;
    }

    pPkgPnp = pDevice->m_PkgPnp;

    pPkgPnp->m_DeviceInterfaceLock.AcquireLock(pFxDriverGlobals);

    status = pDeviceInterface->Register(pDevice);

    if (NT_SUCCESS(status))
    {
        //
        // Insert into the end of the list
        //
        ppPrev = &pPkgPnp->m_DeviceInterfaceHead.Next;
        pCur = pPkgPnp->m_DeviceInterfaceHead.Next;
        while (pCur != NULL)
        {
            ppPrev = &pCur->Next;
            pCur = pCur->Next;
        }

        *ppPrev = &pDeviceInterface->m_Entry;
    }

    pPkgPnp->m_DeviceInterfaceLock.ReleaseLock(pFxDriverGlobals);

Done:
    if (!NT_SUCCESS(status))
    {
        delete pDeviceInterface;
        pDeviceInterface = NULL;
    }

    return status;
}

}
