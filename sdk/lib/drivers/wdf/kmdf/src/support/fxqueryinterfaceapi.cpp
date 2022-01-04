/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxQueryInterfaceAPI.cpp

Abstract:

    This module implements the device interface object.

Author:




Environment:

    Kernel mode only

Revision History:

--*/
#include "fxsupportpch.hpp"

extern "C" {
// #include "FxQueryInterfaceAPI.tmh"
}

//
// Extern "C" the entire file
//
extern "C" {
_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfDeviceAddQueryInterface)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_QUERY_INTERFACE_CONFIG InterfaceConfig
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxQueryInterface *pQueryInterface;
    FxDevice *pDevice;
    PINTERFACE pInterface;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID*) &pDevice,
                                   &pFxDriverGlobals );

    pQueryInterface = NULL;

    FxPointerNotNull(pFxDriverGlobals, Device);
    FxPointerNotNull(pFxDriverGlobals, InterfaceConfig);
    FxPointerNotNull(pFxDriverGlobals, InterfaceConfig->InterfaceType);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pInterface = InterfaceConfig->Interface;

    if (InterfaceConfig->Size != sizeof(WDF_QUERY_INTERFACE_CONFIG)) {
        status = STATUS_INFO_LENGTH_MISMATCH;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "WDFDEVICE %p, WDF_QUERY_INTERFACE_CONFIG Size %d, expected %d, "
            "%!STATUS!", Device, InterfaceConfig->Size,
            sizeof(WDF_QUERY_INTERFACE_CONFIG), status);

        goto Done;
    }

    //
    // A pass through interface is only associated with PDOs
    //
    if (InterfaceConfig->SendQueryToParentStack && pDevice->IsPdo() == FALSE) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "SendQueryToParentStack TRUE, but WDFDEVICE %p not a PDO, %!STATUS!",
            Device, status);

        goto Done;
    }

    //
    // The only time we can have a NULL Interface is if the interface is
    // passthrough or it is an import interface (since on import interfaces
    // the callback must do the copying).
    //
    if (pInterface == NULL) {
        if (InterfaceConfig->SendQueryToParentStack || InterfaceConfig->ImportInterface) {
            //
            // A NULL interface is valid for this config
            //
            DO_NOTHING();
        }
        else {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                "WDFDEVICE %p,  SendQueryToParentStack is FALSE and "
                "InterfaceConfig->ImportInterface is FALSE, %!STATUS!",
                Device, status);

            goto Done;
        }
    }

    //
    // If it is an import interface, we need a callback so that the driver can
    // modify the interface being returned.
    //
    if (InterfaceConfig->ImportInterface &&
        InterfaceConfig->EvtDeviceProcessQueryInterfaceRequest == NULL) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "WDFDEVICE %p, ImportInterface is TRUE and "
            "EvtDeviceProcessQueryInterfaceRequest is NULL, %!STATUS!",
            Device, status);

        goto Done;
    }

    if (pInterface != NULL) {
        //
        // Make sure we are exposing the minimum size
        //
        if (pInterface->Size < sizeof(INTERFACE)) {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                "WDFDEVICE %p, Interface size %d < sizeof(INTERFACE) (%d), "
                "%!STATUS!", Device, pInterface->Size, sizeof(INTERFACE), status);

            goto Done;
        }
    }

    //
    // Since the QI irp is only allowed to be sent at passive level and
    // the list of FxQueryInterface's is locked by a lock which does not
    // raise IRQL, we can allocate the structure out paged pool.
    //
    pQueryInterface = new (pFxDriverGlobals, PagedPool)
        FxQueryInterface(pDevice, InterfaceConfig);

    if (pQueryInterface == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "WDFDEVICE %p, object creation failed, %!STATUS!", Device, status);

        goto Done;
    }

    if (pInterface != NULL) {
        //
        // Try to allocate memory for the interface.
        //
        pQueryInterface->m_Interface = (PINTERFACE)
            FxPoolAllocate(pFxDriverGlobals, PagedPool, pInterface->Size);

        if (pQueryInterface->m_Interface == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                "WDFDEVICE %p, interface allocation failed, %!STATUS!",
                Device, status);

            goto Done;
        }

        RtlCopyMemory(pQueryInterface->m_Interface,
                      pInterface,
                      pInterface->Size);

        if (pInterface->InterfaceReference == NULL) {
            pQueryInterface->m_Interface->InterfaceReference =
                FxDevice::_InterfaceReferenceNoOp;
        }
        if (pInterface->InterfaceDereference == NULL) {
            pQueryInterface->m_Interface->InterfaceDereference =
                FxDevice::_InterfaceDereferenceNoOp;
        }
    }

    status = STATUS_SUCCESS;

    pDevice->m_PkgPnp->AddQueryInterface(pQueryInterface, TRUE);

Done:
    //
    // Delete the query interface structure if there is an error.
    //
    if (!NT_SUCCESS(status) && pQueryInterface != NULL) {
        delete pQueryInterface;
        pQueryInterface = NULL;
    }

    return status;
}

} // extern "C"
