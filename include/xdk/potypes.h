/******************************************************************************
 *                            Power Management Support Types                  *
 ******************************************************************************/

#ifndef _PO_DDK_
#define _PO_DDK_

/* Power States/Levels */
typedef enum _SYSTEM_POWER_STATE {
    PowerSystemUnspecified,
    PowerSystemWorking,
    PowerSystemSleeping1,
    PowerSystemSleeping2,
    PowerSystemSleeping3,
    PowerSystemHibernate,
    PowerSystemShutdown,
    PowerSystemMaximum
} SYSTEM_POWER_STATE, *PSYSTEM_POWER_STATE;

#define POWER_SYSTEM_MAXIMUM PowerSystemMaximum

typedef enum _POWER_INFORMATION_LEVEL {
    SystemPowerPolicyAc,
    SystemPowerPolicyDc,
    VerifySystemPolicyAc,
    VerifySystemPolicyDc,
    SystemPowerCapabilities,
    SystemBatteryState,
    SystemPowerStateHandler,
    ProcessorStateHandler,
    SystemPowerPolicyCurrent,
    AdministratorPowerPolicy,
    SystemReserveHiberFile,
    ProcessorInformation,
    SystemPowerInformation,
    ProcessorStateHandler2,
    LastWakeTime,
    LastSleepTime,
    SystemExecutionState,
    SystemPowerStateNotifyHandler,
    ProcessorPowerPolicyAc,
    ProcessorPowerPolicyDc,
    VerifyProcessorPowerPolicyAc,
    VerifyProcessorPowerPolicyDc,
    ProcessorPowerPolicyCurrent,
    SystemPowerStateLogging,
    SystemPowerLoggingEntry,
    SetPowerSettingValue,
    NotifyUserPowerSetting,
    PowerInformationLevelUnused0,
    PowerInformationLevelUnused1,
    SystemVideoState,
    TraceApplicationPowerMessage,
    TraceApplicationPowerMessageEnd,
    ProcessorPerfStates,
    ProcessorIdleStates,
    ProcessorCap,
    SystemWakeSource,
    SystemHiberFileInformation,
    TraceServicePowerMessage,
    ProcessorLoad,
    PowerShutdownNotification,
    MonitorCapabilities,
    SessionPowerInit,
    SessionDisplayState,
    PowerRequestCreate,
    PowerRequestAction,
    GetPowerRequestList,
    ProcessorInformationEx,
    NotifyUserModeLegacyPowerEvent,
    GroupPark,
    ProcessorIdleDomains,
    WakeTimerList,
    SystemHiberFileSize,
    PowerInformationLevelMaximum
} POWER_INFORMATION_LEVEL;

typedef enum {
    PowerActionNone,
    PowerActionReserved,
    PowerActionSleep,
    PowerActionHibernate,
    PowerActionShutdown,
    PowerActionShutdownReset,
    PowerActionShutdownOff,
    PowerActionWarmEject
} POWER_ACTION, *PPOWER_ACTION;

typedef enum _DEVICE_POWER_STATE {
    PowerDeviceUnspecified,
    PowerDeviceD0,
    PowerDeviceD1,
    PowerDeviceD2,
    PowerDeviceD3,
    PowerDeviceMaximum
} DEVICE_POWER_STATE, *PDEVICE_POWER_STATE;

typedef union _POWER_STATE {
  SYSTEM_POWER_STATE  SystemState;
  DEVICE_POWER_STATE  DeviceState;
} POWER_STATE, *PPOWER_STATE;

typedef enum _POWER_STATE_TYPE {
  SystemPowerState = 0,
  DevicePowerState
} POWER_STATE_TYPE, *PPOWER_STATE_TYPE;

#if (NTDDI_VERSION >= NTDDI_WINXP) || !defined(_BATCLASS_)
typedef struct {
    ULONG Granularity;
    ULONG Capacity;
} BATTERY_REPORTING_SCALE, *PBATTERY_REPORTING_SCALE;
#endif /* (NTDDI_VERSION >= NTDDI_WINXP) || !defined(_BATCLASS_) */

#endif /* !_PO_DDK_ */

typedef VOID
(DDKAPI *PREQUEST_POWER_COMPLETE)(
  IN struct _DEVICE_OBJECT *DeviceObject,
  IN UCHAR MinorFunction,
  IN POWER_STATE PowerState,
  IN PVOID Context,
  IN struct _IO_STATUS_BLOCK *IoStatus);

typedef
NTSTATUS
(DDKAPI POWER_SETTING_CALLBACK)(
  IN LPCGUID SettingGuid,
  IN PVOID Value,
  IN ULONG ValueLength,
  IN OUT PVOID Context OPTIONAL);

typedef POWER_SETTING_CALLBACK *PPOWER_SETTING_CALLBACK;


