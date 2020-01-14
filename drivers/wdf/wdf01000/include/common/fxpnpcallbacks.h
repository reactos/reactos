#ifndef _FXPNPCALLBACKS_H_
#define _FXPNPCALLBACKS_H_

#include "common/fxcallback.h"


struct FxPnpStateCallbackInfo {
    //
    // Bit field of WDF_STATE_NOTIFICATION_TYPE defined values
    //
    ULONG Types;

    //
    // Function to call
    //
    PFN_WDF_DEVICE_PNP_STATE_CHANGE_NOTIFICATION Callback;
};

struct FxPnpStateCallback : public FxCallback {

    FxPnpStateCallback(
        VOID
        ) : FxCallback()
    {
        RtlZeroMemory(&m_Methods[0], sizeof(m_Methods));
    }

    VOID
    Invoke(
        __in WDF_DEVICE_PNP_STATE State,
        __in WDF_STATE_NOTIFICATION_TYPE Type,
        __in WDFDEVICE Device,
        __in PCWDF_DEVICE_PNP_NOTIFICATION_DATA NotificationData
        );

    FxPnpStateCallbackInfo m_Methods[WdfDevStatePnpNull - WdfDevStatePnpObjectCreated];
};

struct FxPowerStateCallbackInfo {
    //
    // Bit field of WDF_STATE_NOTIFICATION_TYPE defined values
    //
    ULONG Types;

    //
    // Function to call
    //
    PFN_WDF_DEVICE_POWER_STATE_CHANGE_NOTIFICATION Callback;
};

struct FxPowerStateCallback : public FxCallback {
    FxPowerStateCallback(
        VOID
        ) : FxCallback()
    {
        RtlZeroMemory(&m_Methods[0], sizeof(m_Methods));
    }

    VOID
    Invoke(
        __in WDF_DEVICE_POWER_STATE State,
        __in WDF_STATE_NOTIFICATION_TYPE Type,
        __in WDFDEVICE Device,
        __in PCWDF_DEVICE_POWER_NOTIFICATION_DATA NotificationData
        );

    FxPowerStateCallbackInfo m_Methods[WdfDevStatePowerNull-WdfDevStatePowerObjectCreated];
};

struct FxPowerPolicyStateCallbackInfo {
    //
    // Bit field of WDF_STATE_NOTIFICATION_TYPE defined values
    //
    ULONG Types;

    //
    // Function to call
    //
    PFN_WDF_DEVICE_POWER_POLICY_STATE_CHANGE_NOTIFICATION Callback;
};

struct FxPowerPolicyStateCallback : public FxCallback {
    FxPowerPolicyStateCallback(
        VOID
        ) : FxCallback()
    {
        RtlZeroMemory(&m_Methods[0], sizeof(m_Methods));
    }

    VOID
    Invoke(
        __in WDF_DEVICE_POWER_POLICY_STATE State,
        __in WDF_STATE_NOTIFICATION_TYPE Type,
        __in WDFDEVICE Device,
        __in PCWDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA NotificationData
        );

    FxPowerPolicyStateCallbackInfo m_Methods[WdfDevStatePwrPolNull-WdfDevStatePwrPolObjectCreated];
};

#endif //_FXPNPCALLBACKS_H_
