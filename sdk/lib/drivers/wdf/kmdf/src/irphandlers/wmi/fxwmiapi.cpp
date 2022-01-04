/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWmiApi.cpp

Abstract:

    This module implements the C interface to the WMI package
    for the driver frameworks.

Author:



Environment:

    Kernel mode only

Revision History:



--*/

#include "fxwmipch.hpp"

//
// Extern "C" the tmh file and all external APIs
//
extern "C" {
#include "fxwmiapi.tmh"

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfWmiProviderCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_WMI_PROVIDER_CONFIG WmiProviderConfig,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES ProviderAttributes,
    __out
    WDFWMIPROVIDER* WmiProvider
    )
{
    FxDevice* pDevice;
    FxPowerPolicyOwnerSettings* ownerSettings;
    FxWmiProvider* pProvider;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &pDevice);

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), WmiProviderConfig);
    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), WmiProvider);

    //
    // If the Device is a power policy owner then do not allow client drivers
    // to register for GUID_POWER_DEVICE_ENABLE or GUID_POWER_DEVICE_WAKE_ENABLE
    // if the framework has already register a provider for those guids.
    //
    if (pDevice->m_PkgPnp->IsPowerPolicyOwner()) {
        ownerSettings = pDevice->m_PkgPnp->m_PowerPolicyMachine.m_Owner;

        if ((FxIsEqualGuid(&WmiProviderConfig->Guid,
                           &GUID_POWER_DEVICE_ENABLE) &&
             ownerSettings->m_IdleSettings.WmiInstance != NULL) ||

            (FxIsEqualGuid(&WmiProviderConfig->Guid,
                           &GUID_POWER_DEVICE_WAKE_ENABLE) &&
             ownerSettings->m_WakeSettings.WmiInstance != NULL)) {

            DoTraceLevelMessage(GetFxDriverGlobals(DriverGlobals), TRACE_LEVEL_ERROR,
                                TRACINGDEVICE, "WMI Guid already registered by "
                                "framework");
            return STATUS_WMI_GUID_DISCONNECTED;
        }
    }

    return FxWmiProvider::_Create(GetFxDriverGlobals(DriverGlobals),
                                  Device,
                                  ProviderAttributes,
                                  WmiProviderConfig,
                                  WmiProvider,
                                  &pProvider);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfWmiInstanceCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_WMI_INSTANCE_CONFIG InstanceConfig,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES InstanceAttributes,
    __out_opt
    WDFWMIINSTANCE* Instance
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxWmiProvider* pProvider;
    FxWmiInstanceExternal* pInstance;
    WDFWMIINSTANCE hInstance;
    NTSTATUS status;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);
    pInstance = NULL;

    FxPointerNotNull(pFxDriverGlobals, InstanceConfig);

    if (InstanceConfig->Size != sizeof(WDF_WMI_INSTANCE_CONFIG)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Expected InstanceConfig Size %d, got %d, %!STATUS!",
                            InstanceConfig->Size, sizeof(*InstanceConfig),
                            status);
        return status;
    }

    if (InstanceConfig->Provider == NULL &&
                                    InstanceConfig->ProviderConfig == NULL) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "InstanceConfig %p Provider and ProviderConfig are both NULL, only "
            "one can be, %!STATUS!", InstanceConfig, status);

        return status;
    }
    else if (InstanceConfig->Provider != NULL &&
                                    InstanceConfig->ProviderConfig != NULL) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "InstanceConfig %p Provider %p and ProviderConfig %p are both not "
            "NULL, only one can be, %!STATUS!", InstanceConfig,
            InstanceConfig->Provider, InstanceConfig->ProviderConfig, status);

        return status;
    }

    if (InstanceConfig->Provider != NULL) {
        FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                       InstanceConfig->Provider,
                                       FX_TYPE_WMI_PROVIDER,
                                       (PVOID*) &pProvider,
                                       &pFxDriverGlobals);
    }
    else {
        FxDevice* pDevice;
        FxPowerPolicyOwnerSettings* ownerSettings;
        WDFWMIPROVIDER hProvider;

        hProvider = NULL;
        FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                             Device,
                             FX_TYPE_DEVICE,
                             (PVOID*) &pDevice);

        //
        // If the Device is a power policy owner then do not allow client drivers
        // to register for GUID_POWER_DEVICE_ENABLE or GUID_POWER_DEVICE_WAKE_ENABLE
        // if the framework has already register a provider for those guids.
        //
        if (pDevice->m_PkgPnp->IsPowerPolicyOwner()) {
            ownerSettings = pDevice->m_PkgPnp->m_PowerPolicyMachine.m_Owner;

            if ((FxIsEqualGuid(&InstanceConfig->ProviderConfig->Guid,
                               &GUID_POWER_DEVICE_ENABLE) &&
                 ownerSettings->m_IdleSettings.WmiInstance != NULL) ||

                (FxIsEqualGuid(&InstanceConfig->ProviderConfig->Guid,
                               &GUID_POWER_DEVICE_WAKE_ENABLE) &&
                 ownerSettings->m_WakeSettings.WmiInstance != NULL)) {

                status = STATUS_WMI_GUID_DISCONNECTED;
                DoTraceLevelMessage(GetFxDriverGlobals(DriverGlobals), TRACE_LEVEL_ERROR,
                                    TRACINGDEVICE, "WMI Guid already registered by "
                                    "framework");
                return status;
            }
        }

        status = FxWmiProvider::_Create(pFxDriverGlobals,
                                        Device,
                                        NULL,
                                        InstanceConfig->ProviderConfig,
                                        &hProvider,
                                        &pProvider);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Use the object's globals and not the caller's
        //
        pFxDriverGlobals = pProvider->GetDriverGlobals();
    }

    status = FxWmiInstanceExternal::_Create(pFxDriverGlobals,
                                            pProvider,
                                            InstanceConfig,
                                            InstanceAttributes,
                                            &hInstance,
                                            &pInstance);

    if (NT_SUCCESS(status) && InstanceConfig->Register) {
        status = pProvider->AddInstance(pInstance);
    }

    if (NT_SUCCESS(status)) {
        if (Instance != NULL) {
            *Instance = hInstance;
        }
    }
    else {
        //
        // Something went wrong, cleanup
        //
        if (pInstance != NULL) {
            //
            // This will remove the instance from the provider's list as well.
            //
            pInstance->DeleteFromFailedCreate();
        }

        //
        // Only remove the provider if we created it in this function
        //
        if (InstanceConfig->ProviderConfig != NULL) {
            pProvider->DeleteFromFailedCreate();
        }
    }

    return status;
}

WDFAPI
__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
WDFEXPORT(WdfWmiProviderGetDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIPROVIDER WmiProvider
    )
{
    FxWmiProvider *pProvider;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         WmiProvider,
                         FX_TYPE_WMI_PROVIDER,
                         (PVOID*) &pProvider);

    return pProvider->GetDevice()->GetHandle();
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFEXPORT(WdfWmiProviderIsEnabled)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIPROVIDER WmiProvider,
    __in
    WDF_WMI_PROVIDER_CONTROL ProviderControl
    )
{
    FxWmiProvider *pProvider;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         WmiProvider,
                         FX_TYPE_WMI_PROVIDER,
                         (PVOID*) &pProvider);

    return pProvider->IsEnabled(ProviderControl);
}

__drv_maxIRQL(DISPATCH_LEVEL)
ULONGLONG
WDFEXPORT(WdfWmiProviderGetTracingHandle)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIPROVIDER WmiProvider
    )
{
    FxWmiProvider *pProvider;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         WmiProvider,
                         FX_TYPE_WMI_PROVIDER,
                         (PVOID*) &pProvider);

    return pProvider->GetTracingHandle();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfWmiInstanceRegister)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIINSTANCE WmiInstance
    )
{
    FxWmiInstanceExternal* pInstance;
    FxWmiProvider* pProvider;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         WmiInstance,
                         FX_TYPE_WMI_INSTANCE,
                         (PVOID*) &pInstance);

    pProvider = pInstance->GetProvider();

    return pProvider->AddInstance(pInstance);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfWmiInstanceDeregister)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIINSTANCE WmiInstance
    )
{
    FxWmiInstanceExternal* pInstance;
    FxWmiProvider* pProvider;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         WmiInstance,
                         FX_TYPE_WMI_INSTANCE,
                         (PVOID*) &pInstance);

    pProvider = pInstance->GetProvider();
    pProvider->RemoveInstance(pInstance);
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
WDFEXPORT(WdfWmiInstanceGetDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIINSTANCE WmiInstance
    )
{
    FxWmiInstanceExternal* pInstance;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         WmiInstance,
                         FX_TYPE_WMI_INSTANCE,
                         (PVOID*) &pInstance);

    return pInstance->GetDevice()->GetHandle();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFWMIPROVIDER
WDFEXPORT(WdfWmiInstanceGetProvider)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIINSTANCE WmiInstance
    )
{
    FxWmiInstanceExternal *pInstance;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         WmiInstance,
                         FX_TYPE_WMI_INSTANCE,
                         (PVOID*) &pInstance);

    return pInstance->GetProvider()->GetHandle();
}


_Must_inspect_result_
__drv_maxIRQL(APC_LEVEL)
NTSTATUS
WDFEXPORT(WdfWmiInstanceFireEvent)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIINSTANCE WmiInstance,
    __in_opt
    ULONG EventDataSize,
    __in_bcount_opt(EventDataSize)
    PVOID EventData
    )
/*++

Routine Description:
    Fires an event based on the instance handle.

Arguments:
    WmiInstance - instance which the event is associated with
    EventDataSize - size of EventData in bytes
    EventData - buffer associated with the event

Return Value:
    NTSTATUS

  --*/
{
    FxWmiInstanceExternal* pInstance;
    NTSTATUS status;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         WmiInstance,
                         FX_TYPE_WMI_INSTANCE,
                         (PVOID*) &pInstance);

    status = FxVerifierCheckIrqlLevel(pInstance->GetDriverGlobals(), APC_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return pInstance->FireEvent(EventData, EventDataSize);
}

} // extern "C"
