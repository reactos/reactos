#include "common/fxpnpcallbacks.h"

VOID
FxPnpStateCallback::Invoke(
    __in WDF_DEVICE_PNP_STATE State,
    __in WDF_STATE_NOTIFICATION_TYPE Type,
    __in WDFDEVICE Device,
    __in PCWDF_DEVICE_PNP_NOTIFICATION_DATA NotificationData
    )
{
    FxPnpStateCallbackInfo* pInfo;

    pInfo = &m_Methods[WdfDevStateNormalize(State)-WdfDevStatePnpObjectCreated];

    if (pInfo->Callback != NULL && (pInfo->Types & Type))
    {
        CallbackStart();
        pInfo->Callback(Device, NotificationData);
        CallbackEnd();
    }
}