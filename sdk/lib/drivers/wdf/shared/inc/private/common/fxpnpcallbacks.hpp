/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPnpCallbacks.hpp

Abstract:

    This module implements the PnP/Power callback objects.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXPNPCALLBACKS_H_
#define _FXPNPCALLBACKS_H_

class FxPnpDeviceFilterResourceRequirements : public FxCallback {

public:
    PFN_WDF_DEVICE_FILTER_RESOURCE_REQUIREMENTS m_Method;

    FxPnpDeviceFilterResourceRequirements(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE Device,
        __in WDFIORESREQLIST Collection
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device, Collection);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceD0Entry : public FxCallback {

public:
    PFN_WDF_DEVICE_D0_ENTRY m_Method;

    FxPnpDeviceD0Entry(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE  Device,
        __in WDF_POWER_DEVICE_STATE PreviousState
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device, PreviousState);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceD0EntryPostInterruptsEnabled : public FxCallback {

public:
    PFN_WDF_DEVICE_D0_ENTRY_POST_INTERRUPTS_ENABLED m_Method;

    FxPnpDeviceD0EntryPostInterruptsEnabled(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE  Device,
        __in WDF_POWER_DEVICE_STATE PreviousState
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device, PreviousState);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceD0Exit : public FxCallback {

public:
    PFN_WDF_DEVICE_D0_EXIT m_Method;

    FxPnpDeviceD0Exit(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE  Device,
        __in WDF_POWER_DEVICE_STATE TargetState
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device, TargetState);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceD0ExitPreInterruptsDisabled : public FxCallback {

public:
    PFN_WDF_DEVICE_D0_EXIT_PRE_INTERRUPTS_DISABLED m_Method;

    FxPnpDeviceD0ExitPreInterruptsDisabled(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE  Device,
        __in WDF_POWER_DEVICE_STATE TargetState
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device, TargetState);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDevicePrepareHardware : public FxCallback {

public:
    PFN_WDF_DEVICE_PREPARE_HARDWARE m_Method;

    FxPnpDevicePrepareHardware(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE  Device,
        __in WDFCMRESLIST ResourcesRaw,
        __in WDFCMRESLIST ResourcesTranslated
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device, ResourcesRaw, ResourcesTranslated);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceReleaseHardware : public FxCallback {

public:
    PFN_WDF_DEVICE_RELEASE_HARDWARE m_Method;

    FxPnpDeviceReleaseHardware(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE  Device,
        __in WDFCMRESLIST ResourcesTranslated
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device, ResourcesTranslated);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceRemoveAddedResources : public FxCallback {
public:
    PFN_WDF_DEVICE_REMOVE_ADDED_RESOURCES m_Method;

public:
    FxPnpDeviceRemoveAddedResources(
        VOID
        )  : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE Device,
        __in WDFCMRESLIST ResourcesRaw,
        __in WDFCMRESLIST ResourcesTranslated
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device, ResourcesRaw, ResourcesTranslated);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceSelfManagedIoCleanup : public FxCallback {

public:
    PFN_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP m_Method;

    FxPnpDeviceSelfManagedIoCleanup(
        VOID
        ) : m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in  WDFDEVICE  Device
        )
    {
        if (m_Method != NULL) {
            CallbackStart();
            m_Method(Device);
            CallbackEnd();
        }
    }
};

class FxPnpDeviceSelfManagedIoFlush : public FxCallback {

public:
    PFN_WDF_DEVICE_SELF_MANAGED_IO_FLUSH m_Method;

    FxPnpDeviceSelfManagedIoFlush(
        VOID
        ) : m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in  WDFDEVICE  Device
        )
    {
        if (m_Method != NULL) {
            CallbackStart();
            m_Method(Device);
            CallbackEnd();
        }
    }
};

class FxPnpDeviceSelfManagedIoInit : public FxCallback {

public:
    PFN_WDF_DEVICE_SELF_MANAGED_IO_INIT m_Method;

    FxPnpDeviceSelfManagedIoInit(
        VOID
        ) : m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in  WDFDEVICE  Device
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceSelfManagedIoSuspend : public FxCallback {

public:
    PFN_WDF_DEVICE_SELF_MANAGED_IO_SUSPEND m_Method;

    FxPnpDeviceSelfManagedIoSuspend(
        VOID
        ) : m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in  WDFDEVICE  Device
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceSelfManagedIoRestart : public FxCallback {

public:
    PFN_WDF_DEVICE_SELF_MANAGED_IO_RESTART m_Method;

    FxPnpDeviceSelfManagedIoRestart(
        VOID
        ) : m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in  WDFDEVICE  Device
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceQueryStop : public FxCallback {

public:
    PFN_WDF_DEVICE_QUERY_STOP m_Method;

    FxPnpDeviceQueryStop(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in  WDFDEVICE  Device
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceQueryRemove : public FxCallback {

public:
    PFN_WDF_DEVICE_QUERY_REMOVE m_Method;

    FxPnpDeviceQueryRemove(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in  WDFDEVICE  Device
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceResourcesQuery : public FxCallback {

public:
    PFN_WDF_DEVICE_RESOURCES_QUERY m_Method;

    FxPnpDeviceResourcesQuery(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE  Device,
        __in WDFCMRESLIST Collection
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device, Collection);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceResourceRequirementsQuery : public FxCallback {

public:
    PFN_WDF_DEVICE_RESOURCE_REQUIREMENTS_QUERY m_Method;

    FxPnpDeviceResourceRequirementsQuery(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE  Device,
        __in WDFIORESREQLIST Collection
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device, Collection);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceEject : public FxCallback {

public:
    PFN_WDF_DEVICE_EJECT m_Method;

    FxPnpDeviceEject(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE  Device
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceSurpriseRemoval : public FxCallback {

public:
    PFN_WDF_DEVICE_SURPRISE_REMOVAL m_Method;

    FxPnpDeviceSurpriseRemoval(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in WDFDEVICE  Device
        )
    {
        if (m_Method != NULL) {
            CallbackStart();
            m_Method(Device);
            CallbackEnd();
        }
    }
};

class FxPnpDeviceUsageNotification : public FxCallback {

public:
    PFN_WDF_DEVICE_USAGE_NOTIFICATION m_Method;

    FxPnpDeviceUsageNotification(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in WDFDEVICE Device,
        __in WDF_SPECIAL_FILE_TYPE NotificationType,
        __in BOOLEAN InPath
        )
    {
        if (m_Method != NULL) {
            CallbackStart();
            m_Method(Device, NotificationType, InPath);
            CallbackEnd();
        }
    }
};

class FxPnpDeviceUsageNotificationEx : public FxCallback {

public:
    PFN_WDF_DEVICE_USAGE_NOTIFICATION_EX m_Method;

    FxPnpDeviceUsageNotificationEx(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE Device,
        __in WDF_SPECIAL_FILE_TYPE NotificationType,
        __in BOOLEAN InPath
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device, NotificationType, InPath);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPnpDeviceRelationsQuery : public FxCallback {

public:
    PFN_WDF_DEVICE_RELATIONS_QUERY m_Method;

    FxPnpDeviceRelationsQuery(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in WDFDEVICE Device,
        __in DEVICE_RELATION_TYPE RelationType
        )
    {
        if (m_Method != NULL) {
            CallbackStart();
            m_Method(Device, RelationType);
            CallbackEnd();
        }
    }
};

class FxPnpDeviceSetLock : public FxCallback {

public:
    PFN_WDF_DEVICE_SET_LOCK m_Method;

    FxPnpDeviceSetLock(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE Device,
        __in BOOLEAN Lock
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device, Lock);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_UNSUCCESSFUL;
        }
    }
};

class FxPnpDeviceReportedMissing : public FxCallback {

public:
    PFN_WDF_DEVICE_REPORTED_MISSING m_Method;

    FxPnpDeviceReportedMissing(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in WDFDEVICE Device
        )
    {
        if (m_Method != NULL) {
            CallbackStart();
            m_Method(Device);
            CallbackEnd();
        }
    }
};

class FxPowerDeviceEnableWakeAtBus :  public FxCallback {

public:
    PFN_WDF_DEVICE_ENABLE_WAKE_AT_BUS m_Method;

    FxPowerDeviceEnableWakeAtBus(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE Device,
        __in SYSTEM_POWER_STATE PowerState
        )
    {
        NTSTATUS status;

        if (m_Method != NULL) {
            CallbackStart();
            status = m_Method(Device, PowerState);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPowerDeviceDisableWakeAtBus :  public FxCallback {

public:
    PFN_WDF_DEVICE_DISABLE_WAKE_AT_BUS m_Method;

    FxPowerDeviceDisableWakeAtBus(
        VOID
        ) : FxCallback(), m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in WDFDEVICE Device
        )
    {
        if (m_Method != NULL) {
            CallbackStart();
            m_Method(Device);
            CallbackEnd();
        }
    }
};

class FxPowerDeviceArmWakeFromS0 :  public FxCallback {

public:
    PFN_WDF_DEVICE_ARM_WAKE_FROM_S0 m_Method;

    FxPowerDeviceArmWakeFromS0(
        VOID
        ) : m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE Device
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPowerDeviceArmWakeFromSx :  public FxCallback {

public:
    PFN_WDF_DEVICE_ARM_WAKE_FROM_SX m_Method;
    PFN_WDF_DEVICE_ARM_WAKE_FROM_SX_WITH_REASON m_MethodWithReason;

    FxPowerDeviceArmWakeFromSx(
        VOID
        ) : m_Method(NULL),
            m_MethodWithReason(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE Device,
        __in BOOLEAN DeviceWakeEnabled,
        __in BOOLEAN ChildrenArmedForWake
        )
    {
        if (m_MethodWithReason != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_MethodWithReason(Device,
                                        DeviceWakeEnabled,
                                        ChildrenArmedForWake);
            CallbackEnd();

            return status;
        }
        else if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }
};

class FxPowerDeviceDisarmWakeFromS0 :  public FxCallback {

public:
    PFN_WDF_DEVICE_DISARM_WAKE_FROM_S0 m_Method;

    FxPowerDeviceDisarmWakeFromS0(
        VOID
        ) : m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in WDFDEVICE Device
        )
    {
        if (m_Method != NULL) {
            CallbackStart();
            m_Method(Device);
            CallbackEnd();
        }
    }
};

class FxPowerDeviceDisarmWakeFromSx :  public FxCallback {

public:
    PFN_WDF_DEVICE_DISARM_WAKE_FROM_SX m_Method;

    FxPowerDeviceDisarmWakeFromSx(
        VOID
        ) : m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in WDFDEVICE Device
        )
    {
        if (m_Method != NULL) {
            CallbackStart();
            m_Method(Device);
            CallbackEnd();
        }
    }
};

class FxPowerDeviceWakeFromSxTriggered :  public FxCallback {

public:
    PFN_WDF_DEVICE_WAKE_FROM_SX_TRIGGERED m_Method;

    FxPowerDeviceWakeFromSxTriggered(
        VOID
        ) : m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in WDFDEVICE Device
        )
    {
        if (m_Method != NULL) {
            CallbackStart();
            m_Method(Device);
            CallbackEnd();
        }
    }
};

class FxPowerDeviceWakeFromS0Triggered :  public FxCallback {

public:
    PFN_WDF_DEVICE_WAKE_FROM_S0_TRIGGERED m_Method;

    FxPowerDeviceWakeFromS0Triggered(
        VOID
        ) : m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in WDFDEVICE Device
        )
    {
        if (m_Method != NULL) {
            CallbackStart();
            m_Method(Device);
            CallbackEnd();
        }
    }
};

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
        )
    {
        FxPnpStateCallbackInfo* pInfo;

        pInfo = &m_Methods[WdfDevStateNormalize(State)-WdfDevStatePnpObjectCreated];

        if (pInfo->Callback != NULL && (pInfo->Types & Type)) {
            CallbackStart();
            pInfo->Callback(Device, NotificationData);
            CallbackEnd();
        }
    }

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
        )
    {
        FxPowerStateCallbackInfo *pInfo;

        pInfo = &m_Methods[WdfDevStateNormalize(State)-WdfDevStatePowerObjectCreated];

        if (pInfo->Callback != NULL && (pInfo->Types & Type)) {
            CallbackStart();
            pInfo->Callback(Device, NotificationData);
            CallbackEnd();
        }
    }

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
        )
    {
        FxPowerPolicyStateCallbackInfo *pInfo;

        pInfo = &m_Methods[WdfDevStateNormalize(State)-WdfDevStatePwrPolObjectCreated];

        if (pInfo->Callback != NULL && (pInfo->Types & Type)) {
            CallbackStart();
            pInfo->Callback(Device, NotificationData);
            CallbackEnd();
        }
    }

    FxPowerPolicyStateCallbackInfo m_Methods[WdfDevStatePwrPolNull-WdfDevStatePwrPolObjectCreated];
};



#endif // _FXPNPCALLBACKS_H_
