#ifndef __INCLUDE_DDK_POTYPES_H
#define __INCLUDE_DDK_POTYPES_H

// Flags for PoSetSystemState
typedef ULONG EXECUTION_STATE;

#define ES_SYSTEM_REQUIRED  ((EXECUTION_STATE)0x00000001)
#define ES_DISPLAY_REQUIRED ((EXECUTION_STATE)0x00000002)
#define ES_USER_PRESENT     ((EXECUTION_STATE)0x00000004)
#define ES_CONTINUOUS       ((EXECUTION_STATE)0x80000000)

// PowerState for PoRequestPowerIrp
typedef enum _SYSTEM_POWER_STATE {
  PowerSystemUnspecified = 0,
  PowerSystemWorking,
  PowerSystemSleeping1,
  PowerSystemSleeping2,
  PowerSystemSleeping3,
  PowerSystemHibernate,
  PowerSystemShutdown,
  PowerSystemMaximum
} SYSTEM_POWER_STATE, *PSYSTEM_POWER_STATE;

// Values for IRP_MN_QUERY_POWER/IRP_MN_SET_POWER
typedef enum {
  PowerActionNone = 0,
  PowerActionReserved,
  PowerActionSleep,
  PowerActionHibernate,
  PowerActionShutdown,
  PowerActionShutdownReset,
  PowerActionShutdownOff,
  PowerActionWarmEject
} POWER_ACTION, *PPOWER_ACTION;

// State for PoRegisterDeviceForIdleDetection
typedef enum _DEVICE_POWER_STATE {
  PowerDeviceUnspecified = 0,
  PowerDeviceD0,
  PowerDeviceD1,
  PowerDeviceD2,
  PowerDeviceD3,
  PowerDeviceMaximum
} DEVICE_POWER_STATE, *PDEVICE_POWER_STATE;

// State for PoSetPowerState
typedef union _POWER_STATE {
  SYSTEM_POWER_STATE SystemState;
  DEVICE_POWER_STATE DeviceState;
} POWER_STATE, *PPOWER_STATE;

// Type for PoSetPowerState
typedef enum _POWER_STATE_TYPE {
  SystemPowerState = 0,
  DevicePowerState
} POWER_STATE_TYPE, *PPOWER_STATE_TYPE;

// CompletionFunction for PoRequestPowerIrp
typedef
VOID
(*PREQUEST_POWER_COMPLETE) (
  IN PDEVICE_OBJECT DeviceObject,
  IN UCHAR MinorFunction,
  IN POWER_STATE PowerState,
  IN PVOID Context,
  IN PIO_STATUS_BLOCK IoStatus);

#endif /* __INCLUDE_DDK_POTYPES_H */
