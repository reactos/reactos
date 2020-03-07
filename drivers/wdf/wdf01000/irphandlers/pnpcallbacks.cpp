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

VOID
FxPnpDeviceD0Entry::Initialize(
    _In_ FxPkgPnp* PkgPnp,
    _In_ PFN_WDF_DEVICE_D0_ENTRY Method
    )
{
    m_Method = Method;
    m_PkgPnp = PkgPnp;
    m_CallbackType = FxCxCallbackD0Entry;
}

VOID
FxPnpDeviceD0Exit::Initialize(
    _In_ FxPkgPnp* PkgPnp,
    _In_ PFN_WDF_DEVICE_D0_EXIT Method
    )
{
    m_Method = Method;
    m_PkgPnp = PkgPnp;
    m_CallbackType = FxCxCallbackD0Exit;
}

VOID
FxPnpDevicePrepareHardware::Initialize(
    _In_ FxPkgPnp* PkgPnp,
    _In_ PFN_WDF_DEVICE_PREPARE_HARDWARE Method
    )
{
    m_Method = Method;
    m_PkgPnp = PkgPnp;
    m_CallbackType = FxCxCallbackPrepareHardware;
}

VOID
FxPnpDeviceReleaseHardware::Initialize(
    _In_ FxPkgPnp* PkgPnp,
    _In_ PFN_WDF_DEVICE_RELEASE_HARDWARE Method
    )
{
    m_Method = Method;
    m_PkgPnp = PkgPnp;
    m_CallbackType = FxCxCallbackReleaseHardware;
}

VOID
FxPnpDeviceSurpriseRemoval::Initialize(
    _In_ FxPkgPnp* PkgPnp,
    _In_ PFN_WDF_DEVICE_SURPRISE_REMOVAL Method
    )
{
    m_Method = Method;
    m_PkgPnp = PkgPnp;
    m_CallbackType = FxCxCallbackSurpriseRemoval;
}

VOID
FxPnpDeviceSelfManagedIoRestart::Initialize(
    _In_ FxPkgPnp* PkgPnp,
    _In_ PFN_WDF_DEVICE_SELF_MANAGED_IO_RESTART Method
    )
{
    m_Method = Method;
    m_PkgPnp = PkgPnp;
    m_CallbackType = FxCxCallbackSmIoRestart;
}

VOID
FxPnpDeviceSelfManagedIoSuspend::Initialize(
    _In_ FxPkgPnp* PkgPnp,
    _In_ PFN_WDF_DEVICE_SELF_MANAGED_IO_SUSPEND Method
    )
{
    m_Method = Method;
    m_PkgPnp = PkgPnp;
    m_CallbackType = FxCxCallbackSmIoSuspend;
}

VOID
FxPnpDeviceSelfManagedIoInit::Initialize(
    _In_ FxPkgPnp* PkgPnp,
    _In_ PFN_WDF_DEVICE_SELF_MANAGED_IO_INIT Method
    )
{
    m_Method = Method;
    m_PkgPnp = PkgPnp;
    m_CallbackType = FxCxCallbackSmIoInit;
}

VOID
FxPnpDeviceSelfManagedIoFlush::Initialize(
    _In_ FxPkgPnp* PkgPnp,
    _In_ PFN_WDF_DEVICE_SELF_MANAGED_IO_FLUSH Method
    )
{
    m_Method = Method;
    m_PkgPnp = PkgPnp;
    m_CallbackType = FxCxCallbackSmIoFlush;
}

VOID
FxPnpDeviceSelfManagedIoCleanup::Initialize(
    _In_ FxPkgPnp* PkgPnp,
    _In_ PFN_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP Method
    )
{
    m_Method = Method;
    m_PkgPnp = PkgPnp;
    m_CallbackType = FxCxCallbackSmIoCleanup;
}

VOID
FxPowerPolicyStateCallback::Invoke(
    __in WDF_DEVICE_POWER_POLICY_STATE State,
    __in WDF_STATE_NOTIFICATION_TYPE Type,
    __in WDFDEVICE Device,
    __in PCWDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA NotificationData
    )
{
    FxPowerPolicyStateCallbackInfo *pInfo;

    pInfo = &m_Methods[WdfDevStateNormalize(State)-WdfDevStatePwrPolObjectCreated];

    if (pInfo->Callback != NULL && (pInfo->Types & Type))
    {
        CallbackStart();
        pInfo->Callback(Device, NotificationData);
        CallbackEnd();
    }
}

VOID
FxPowerStateCallback::Invoke(
    __in WDF_DEVICE_POWER_STATE State,
    __in WDF_STATE_NOTIFICATION_TYPE Type,
    __in WDFDEVICE Device,
    __in PCWDF_DEVICE_POWER_NOTIFICATION_DATA NotificationData
    )
{
    FxPowerStateCallbackInfo *pInfo;

    pInfo = &m_Methods[WdfDevStateNormalize(State)-WdfDevStatePowerObjectCreated];

    if (pInfo->Callback != NULL && (pInfo->Types & Type))
    {
        CallbackStart();
        pInfo->Callback(Device, NotificationData);
        CallbackEnd();
    }
}

_Must_inspect_result_
NTSTATUS
FxPnpDeviceFilterResourceRequirements::Invoke(
    __in WDFDEVICE Device,
    __in WDFIORESREQLIST Collection
    )
{
    if (m_Method != NULL)
    {
        NTSTATUS status;

        CallbackStart();
        status = m_Method(Device, Collection);
        CallbackEnd();

        return status;
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

_Must_inspect_result_
NTSTATUS
FxPnpDeviceRemoveAddedResources::Invoke(
    __in WDFDEVICE Device,
    __in WDFCMRESLIST ResourcesRaw,
    __in WDFCMRESLIST ResourcesTranslated
    )
{
    if (m_Method != NULL)
    {
        NTSTATUS status;

        CallbackStart();
        status = m_Method(Device, ResourcesRaw, ResourcesTranslated);
        CallbackEnd();

        return status;
    }
    else
    {
        return STATUS_SUCCESS;
    }
}
