//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "pnppriv.hpp"

#include <initguid.h>
#include <wdmguid.h>

extern "C" {
#if defined(EVENT_TRACING)
#include "PoxInterfaceKm.tmh"
#endif
}

VOID
FxPoxInterface::StateCallback(
    __in PVOID Context,
    __in ULONG Component,
    __in ULONG State
    )
{
    PPOX_SETTINGS poxSettings = NULL;
    FxPoxInterface * pThis = NULL;

    pThis = (FxPoxInterface*) Context;

    DoTraceLevelMessage(
        pThis->m_PkgPnp->GetDriverGlobals(),
        TRACE_LEVEL_VERBOSE,
        TRACINGPNP,
        "WDFDEVICE 0x%p !devobj 0x%p PO_FX_COMPONENT_IDLE_STATE_CALLBACK "
        "invoked.",
        pThis->m_PkgPnp->GetDevice()->GetHandle(),
        pThis->m_PkgPnp->GetDevice()->GetDeviceObject()
        );

    //
    // If the client driver has specified power framework settings, retrieve
    // them.
    //
    poxSettings = pThis->GetPowerFrameworkSettings();

    //
    // If the client driver has specified an F-state change callback, invoke it.
    //
    if ((NULL != poxSettings) &&
        (NULL != poxSettings->ComponentIdleStateCallback)) {

        DoTraceLevelMessage(
            pThis->m_PkgPnp->GetDriverGlobals(),
            TRACE_LEVEL_VERBOSE,
            TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p Invoking client driver's "
            "PO_FX_COMPONENT_IDLE_STATE_CALLBACK.",
            pThis->m_PkgPnp->GetDevice()->GetHandle(),
            pThis->m_PkgPnp->GetDevice()->GetDeviceObject()
            );

        poxSettings->ComponentIdleStateCallback(
                       poxSettings->PoFxDeviceContext,
                       Component,
                       State);
    } else {
        PoFxCompleteIdleState(pThis->m_PoHandle, Component);
    }
}

VOID
FxPoxInterface::ComponentActiveCallback(
    __in PVOID Context,
    __in ULONG Component
    )
{
    PPOX_SETTINGS poxSettings = NULL;
    FxPoxInterface * pThis = NULL;

    pThis = (FxPoxInterface*) Context;

    DoTraceLevelMessage(
        pThis->m_PkgPnp->GetDriverGlobals(),
        TRACE_LEVEL_VERBOSE,
        TRACINGPNP,
        "WDFDEVICE 0x%p !devobj 0x%p PO_FX_COMPONENT_ACTIVE_CONDITION_CALLBACK "
        "invoked.",
        pThis->m_PkgPnp->GetDevice()->GetHandle(),
        pThis->m_PkgPnp->GetDevice()->GetDeviceObject()
        );

    //
    // If the client driver has specified power framework settings, retrieve
    // them.
    //
    poxSettings = pThis->GetPowerFrameworkSettings();

    //
    // If the client driver has specified a component-active callback, invoke it
    //
    if ((NULL != poxSettings) &&
        (NULL != poxSettings->ComponentActiveConditionCallback)) {

        DoTraceLevelMessage(
            pThis->m_PkgPnp->GetDriverGlobals(),
            TRACE_LEVEL_VERBOSE,
            TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p Invoking client driver's "
            "PO_FX_COMPONENT_ACTIVE_CONDITION_CALLBACK.",
            pThis->m_PkgPnp->GetDevice()->GetHandle(),
            pThis->m_PkgPnp->GetDevice()->GetDeviceObject()
            );

        poxSettings->ComponentActiveConditionCallback(
                           poxSettings->PoFxDeviceContext,
                           Component
                           );
    } else {
        //
        // Nothing to do.
        //
        DO_NOTHING();
    }

    return;
}

VOID
FxPoxInterface::ComponentIdleCallback(
    __in PVOID Context,
    __in ULONG Component
    )
{
    PPOX_SETTINGS poxSettings = NULL;
    FxPoxInterface * pThis = NULL;

    pThis = (FxPoxInterface*) Context;

    DoTraceLevelMessage(
        pThis->m_PkgPnp->GetDriverGlobals(),
        TRACE_LEVEL_VERBOSE,
        TRACINGPNP,
        "WDFDEVICE 0x%p !devobj 0x%p PO_FX_COMPONENT_IDLE_CONDITION_CALLBACK "
        "invoked.",
        pThis->m_PkgPnp->GetDevice()->GetHandle(),
        pThis->m_PkgPnp->GetDevice()->GetDeviceObject()
        );

    //
    // If the client driver has specified power framework settings, retrieve
    // them.
    //
    poxSettings = pThis->GetPowerFrameworkSettings();

    //
    // If the client driver has specified a component-idle callback, invoke it
    //
    if ((NULL != poxSettings) &&
        (NULL != poxSettings->ComponentIdleConditionCallback)) {

        DoTraceLevelMessage(
            pThis->m_PkgPnp->GetDriverGlobals(),
            TRACE_LEVEL_VERBOSE,
            TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p Invoking client driver's "
            "PO_FX_COMPONENT_IDLE_CONDITION_CALLBACK.",
            pThis->m_PkgPnp->GetDevice()->GetHandle(),
            pThis->m_PkgPnp->GetDevice()->GetDeviceObject()
            );

        poxSettings->ComponentIdleConditionCallback(
                           poxSettings->PoFxDeviceContext,
                           Component
                           );
    } else {
        //
        // We're being notified that we're idle, but there is no action that we
        // need to take here. We power down the device only when we get the
        // device-power-not-required event.
        //
        PoFxCompleteIdleCondition(pThis->m_PoHandle, Component);
    }
    return;
}

VOID
FxPoxInterface::PowerRequiredCallback(
    __in PVOID Context
    )
{
    FxPoxInterface * pThis = NULL;

    pThis = (FxPoxInterface*) Context;
    pThis->PowerRequiredCallbackWorker(TRUE /* InvokedFromPoxCallback */);
    return;
}

VOID
FxPoxInterface::PowerNotRequiredCallback(
    __in PVOID Context
    )
{
    FxPoxInterface * pThis = NULL;

    pThis = (FxPoxInterface*) Context;
    pThis->PowerNotRequiredCallbackWorker(TRUE /* InvokedFromPoxCallback */);
    PoFxCompleteDevicePowerNotRequired(pThis->m_PoHandle);
    return;
}

_Function_class_(PO_FX_POWER_CONTROL_CALLBACK)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FxPoxInterface::PowerControlCallback(
    _In_ PVOID Context,
    _In_ LPCGUID PowerControlCode,
    _In_reads_bytes_opt_(InBufferSize) PVOID InBuffer,
    _In_ SIZE_T InBufferSize,
    _Out_writes_bytes_opt_(OutBufferSize) PVOID OutBuffer,
    _In_ SIZE_T OutBufferSize,
    _Out_opt_ PSIZE_T BytesReturned
    )
{
    NTSTATUS status;
    PPOX_SETTINGS poxSettings = NULL;
    FxPoxInterface * pThis = NULL;

    pThis = (FxPoxInterface*) Context;

    DoTraceLevelMessage(
        pThis->m_PkgPnp->GetDriverGlobals(),
        TRACE_LEVEL_VERBOSE,
        TRACINGPNP,
        "WDFDEVICE 0x%p !devobj 0x%p PO_FX_POWER_CONTROL_CALLBACK invoked.",
        pThis->m_PkgPnp->GetDevice()->GetHandle(),
        pThis->m_PkgPnp->GetDevice()->GetDeviceObject()
        );

    //
    // If the client driver has specified power framework settings, retrieve
    // them.
    //
    poxSettings = pThis->GetPowerFrameworkSettings();

    //
    // The client driver must have specified a power control callback
    //
    ASSERT((NULL != poxSettings) &&
           (NULL != poxSettings->PowerControlCallback));

    //
    // Invoke the client driver's power control callback
    //
    status = poxSettings->PowerControlCallback(poxSettings->PoFxDeviceContext,
                                               PowerControlCode,
                                               InBuffer,
                                               InBufferSize,
                                               OutBuffer,
                                               OutBufferSize,
                                               BytesReturned);

    DoTraceLevelMessage(
        pThis->m_PkgPnp->GetDriverGlobals(),
        TRACE_LEVEL_VERBOSE,
        TRACINGPNP,
        "WDFDEVICE 0x%p !devobj 0x%p Client driver's "
        "PO_FX_POWER_CONTROL_CALLBACK returned %!STATUS!.",
        pThis->m_PkgPnp->GetDevice()->GetHandle(),
        pThis->m_PkgPnp->GetDevice()->GetDeviceObject(),
        status
        );

    return status;
}

NTSTATUS
FxPoxInterface::PoxRegisterDevice(
    VOID
    )
{

    NTSTATUS status;
    PO_FX_DEVICE poxDevice;
    PO_FX_COMPONENT_IDLE_STATE idleState;
    PPOX_SETTINGS poxSettings = NULL;

    RtlZeroMemory(&poxDevice, sizeof(poxDevice));
    RtlZeroMemory(&idleState, sizeof(idleState));

    poxDevice.Version = PO_FX_VERSION_V1;

    //
    // Specify callbacks and context
    //
    poxDevice.ComponentIdleStateCallback =
                    FxPoxInterface::StateCallback;
    poxDevice.ComponentActiveConditionCallback =
                    FxPoxInterface::ComponentActiveCallback;
    poxDevice.ComponentIdleConditionCallback =
                    FxPoxInterface::ComponentIdleCallback;
    poxDevice.DevicePowerRequiredCallback =
                    FxPoxInterface::PowerRequiredCallback;
    poxDevice.DevicePowerNotRequiredCallback =
                    FxPoxInterface::PowerNotRequiredCallback;
    poxDevice.DeviceContext = this;

    //
    // We register as a single component device
    //
    poxDevice.ComponentCount = 1;

    //
    // If the client driver has specified power framework settings, retrieve
    // them.
    //
    poxSettings = GetPowerFrameworkSettings();

    //
    // We specify a power control callback only if the client driver supplies us
    // a power control callback
    //
    poxDevice.PowerControlCallback =
        ((NULL == poxSettings) || (NULL == poxSettings->PowerControlCallback)) ?
            NULL :
            FxPoxInterface::PowerControlCallback;

    //
    // If the client driver has specified any settings for component 0, use
    // them. Otherwise use the default settings.
    //
    if ((NULL == poxSettings) || (NULL == poxSettings->Component)) {
        //
        // Default settings
        //

        //
        // We only support F0
        //
        poxDevice.Components[0].IdleStateCount = 1;

        //
        // Transition latency should be 0 for F0
        //
        idleState.TransitionLatency = 0;

        //
        // Residency requirement should be 0 for F0
        //
        idleState.ResidencyRequirement = 0;

        //
        // The value doesn't matter because we do not support multiple F-states.
        //
        idleState.NominalPower = PO_FX_UNKNOWN_POWER;

        //
        // Specify the state information for F0
        //
        poxDevice.Components[0].IdleStates = &idleState;
    }
    else {
        //
        // Client driver's settings
        //
        RtlCopyMemory(&(poxDevice.Components[0]),
                      poxSettings->Component,
                      sizeof(poxDevice.Components[0]));
    }

    //
    // Register with the power framework
    //
    status = PoFxRegisterDevice(
                m_PkgPnp->GetDevice()->GetPhysicalDevice(),
                &poxDevice,
                &(m_PoHandle)
                );
    if (FALSE == NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            m_PkgPnp->GetDriverGlobals(),
            TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p FxPox::PoxRegisterDevice failed. "
            "%!STATUS!.",
            m_PkgPnp->GetDevice()->GetHandle(),
            m_PkgPnp->GetDevice()->GetDeviceObject(),
            status);
        goto exit;
    }

    status = STATUS_SUCCESS;

exit:
    return status;

}

VOID
FxPoxInterface::PoxStartDevicePowerManagement(
    VOID
    )
{
    PoFxStartDevicePowerManagement(m_PoHandle);
}


VOID
FxPoxInterface::PoxUnregisterDevice(
    VOID
    )
{
    PoFxUnregisterDevice(m_PoHandle);
}


VOID
FxPoxInterface::PoxActivateComponent(
    VOID
    )
{
    //
    // We only support single component and don't
    // need to set any flags
    //
    PoFxActivateComponent(m_PoHandle,
                          0, //component
                          0  //flags
                          );
}


VOID
FxPoxInterface::PoxIdleComponent(
    VOID
    )
{
    //
    // We only support single component and don't
    // need to set any flags
    //
    PoFxIdleComponent(m_PoHandle,
                      0, //component
                      0  //flags
                      );
}


VOID
FxPoxInterface::PoxReportDevicePoweredOn(
    VOID
    )
{
    PoFxReportDevicePoweredOn(m_PoHandle);
}


VOID
FxPoxInterface::PoxSetDeviceIdleTimeout(
    __in ULONGLONG IdleTimeout
    )
{
    PoFxSetDeviceIdleTimeout(m_PoHandle, IdleTimeout);
}

