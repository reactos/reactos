#ifndef __INCLUDE_DDK_POTYPES_H
#define __INCLUDE_DDK_POTYPES_H

struct _DEVICE_OBJECT;
struct _IO_STATUS_BLOCK;

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
  IN struct _DEVICE_OBJECT *DeviceObject,
  IN UCHAR MinorFunction,
  IN POWER_STATE PowerState,
  IN PVOID Context,
  IN struct _IO_STATUS_BLOCK *IoStatus);


typedef struct _POWER_SEQUENCE {
  ULONG SequenceD1;
  ULONG SequenceD2;
  ULONG SequenceD3;
} POWER_SEQUENCE, *PPOWER_SEQUENCE;

typedef VOID (*PINTERFACE_REFERENCE)(PVOID Context);
typedef VOID (*PINTERFACE_DEREFERENCE)(PVOID Context);

typedef struct _INTERFACE {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
} INTERFACE, *PINTERFACE;

typedef enum {
  BusQueryDeviceID = 0,
  BusQueryHardwareIDs = 1,
  BusQueryCompatibleIDs = 2,
  BusQueryInstanceID = 3,
  BusQueryDeviceSerialNumber = 4
} BUS_QUERY_ID_TYPE, *PBUS_QUERY_ID_TYPE;

typedef enum {
  DeviceTextDescription = 0,
  DeviceTextLocationInformation = 1
} DEVICE_TEXT_TYPE, *PDEVICE_TEXT_TYPE;

#endif /* __INCLUDE_DDK_POTYPES_H */
