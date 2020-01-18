#include "common/fxglobals.h"
#include "common/fxdevice.h"
#include "common/fxdeviceinit.h"


extern "C" {

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetPnpPowerEventCallbacks)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_PNPPOWER_EVENT_CALLBACKS PnpPowerEventCallbacks
    )
{
    DDI_ENTRY();
        
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, PnpPowerEventCallbacks);

    if (PnpPowerEventCallbacks->Size != sizeof(WDF_PNPPOWER_EVENT_CALLBACKS) &&
        PnpPowerEventCallbacks->Size != sizeof(WDF_PNPPOWER_EVENT_CALLBACKS_V1_9))
    {

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "PnpPowerEventCallbacks size %d is invalid, exptected %d",
            PnpPowerEventCallbacks->Size, sizeof(WDF_PNPPOWER_EVENT_CALLBACKS)
            );

        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return;
    }

    //
    // Make sure only one of the callbacks EvtDeviceUsageNotification or 
    // EvtDeviceUsageNotificationEx is provided by driver for >V1.9.
    //
    /*if (PnpPowerEventCallbacks->Size > sizeof(WDF_PNPPOWER_EVENT_CALLBACKS_V1_9) &&
        PnpPowerEventCallbacks->EvtDeviceUsageNotification != NULL &&
        PnpPowerEventCallbacks->EvtDeviceUsageNotificationEx != NULL)
    {

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Driver can provide either EvtDeviceUsageNotification or "
            "EvtDeviceUsageNotificationEx callback but not both");

        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return;
    }*/

    //
    // Driver's PnpPowerEventCallbacks structure may be from a previous 
    // version and therefore may be different in size than the current version 
    // that framework is using. Therefore, copy only PnpPowerEventCallbacks->Size
    // bytes and not sizeof(PnpPowerEventCallbacks) bytes.
    //
    RtlCopyMemory(&DeviceInit->PnpPower.PnpPowerEventCallbacks,
                  PnpPowerEventCallbacks,
                  PnpPowerEventCallbacks->Size);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetRequestAttributes)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_OBJECT_ATTRIBUTES RequestAttributes
    )
{
    WDFNOTIMPLEMENTED();
}

} // extern "C"