/*
 * batclass.h
 *
 * Battery class driver interface
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Battery device GUIDs */

DEFINE_GUID(GUID_DEVICE_BATTERY,
  0x72631e54L, 0x78A4, 0x11d0, 0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a);

#if (NTDDI_VERSION >= NTDDI_WINXP)
DEFINE_GUID(BATTERY_STATUS_WMI_GUID,
  0xfc4670d1, 0xebbf, 0x416e, 0x87, 0xce, 0x37, 0x4a, 0x4e, 0xbc, 0x11, 0x1a);
DEFINE_GUID(BATTERY_RUNTIME_WMI_GUID,
  0x535a3767, 0x1ac2, 0x49bc, 0xa0, 0x77, 0x3f, 0x7a, 0x02, 0xe4, 0x0a, 0xec);
DEFINE_GUID(BATTERY_TEMPERATURE_WMI_GUID,
  0x1a52a14d, 0xadce, 0x4a44, 0x9a, 0x3e, 0xc8, 0xd8, 0xf1, 0x5f, 0xf2, 0xc2);
DEFINE_GUID(BATTERY_FULL_CHARGED_CAPACITY_WMI_GUID,
  0x40b40565, 0x96f7, 0x4435, 0x86, 0x94, 0x97, 0xe0, 0xe4, 0x39, 0x59, 0x05);
DEFINE_GUID(BATTERY_CYCLE_COUNT_WMI_GUID,
  0xef98db24, 0x0014, 0x4c25, 0xa5, 0x0b, 0xc7, 0x24, 0xae, 0x5c, 0xd3, 0x71);
DEFINE_GUID(BATTERY_STATIC_DATA_WMI_GUID,
  0x05e1e463, 0xe4e2, 0x4ea9, 0x80, 0xcb, 0x9b, 0xd4, 0xb3, 0xca, 0x06, 0x55);
DEFINE_GUID(BATTERY_STATUS_CHANGE_WMI_GUID,
  0xcddfa0c3, 0x7c5b, 0x4e43, 0xa0, 0x34, 0x05, 0x9f, 0xa5, 0xb8, 0x43, 0x64);
DEFINE_GUID(BATTERY_TAG_CHANGE_WMI_GUID,
  0x5e1f6e19, 0x8786, 0x4d23, 0x94, 0xfc, 0x9e, 0x74, 0x6b, 0xd5, 0xd8, 0x88);
#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#ifndef _BATCLASS_
#define _BATCLASS_

/* BATTERY_INFORMATION.Capabilities constants */
#define BATTERY_SET_CHARGE_SUPPORTED      0x00000001
#define BATTERY_SET_DISCHARGE_SUPPORTED   0x00000002
#define BATTERY_SET_RESUME_SUPPORTED      0x00000004
#define BATTERY_IS_SHORT_TERM             0x20000000
#define BATTERY_CAPACITY_RELATIVE         0x40000000
#define BATTERY_SYSTEM_BATTERY            0x80000000

/* BATTERY_INFORMATION.Capacity constants */
#define BATTERY_UNKNOWN_CAPACITY          0xFFFFFFFF

/* BatteryEstimatedTime constant */
#define BATTERY_UNKNOWN_TIME              0xFFFFFFFF

#define MAX_BATTERY_STRING_SIZE           128

/* BATTERY_STATUS.PowerState flags */
#define BATTERY_POWER_ON_LINE             0x00000001
#define BATTERY_DISCHARGING               0x00000002
#define BATTERY_CHARGING                  0x00000004
#define BATTERY_CRITICAL                  0x00000008

/* BATTERY_STATUS.Voltage constant */
#define BATTERY_UNKNOWN_VOLTAGE           0xFFFFFFFF

/* BATTERY_STATUS.Rate constant */
#define BATTERY_UNKNOWN_RATE              0x80000000

#define IOCTL_BATTERY_QUERY_TAG \
  CTL_CODE(FILE_DEVICE_BATTERY, 0x10, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_BATTERY_QUERY_INFORMATION \
  CTL_CODE(FILE_DEVICE_BATTERY, 0x11, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_BATTERY_SET_INFORMATION \
  CTL_CODE(FILE_DEVICE_BATTERY, 0x12, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_BATTERY_QUERY_STATUS \
  CTL_CODE(FILE_DEVICE_BATTERY, 0x13, METHOD_BUFFERED, FILE_READ_ACCESS)

/* NTSTATUS possibly returned by BCLASS_QUERY_STATUS */
#define BATTERY_TAG_INVALID 0

typedef enum _BATTERY_QUERY_INFORMATION_LEVEL {
  BatteryInformation = 0,
  BatteryGranularityInformation,
  BatteryTemperature,
  BatteryEstimatedTime,
  BatteryDeviceName,
  BatteryManufactureDate,
  BatteryManufactureName,
  BatteryUniqueID,
  BatterySerialNumber
} BATTERY_QUERY_INFORMATION_LEVEL;

typedef struct _BATTERY_QUERY_INFORMATION {
  ULONG BatteryTag;
  BATTERY_QUERY_INFORMATION_LEVEL InformationLevel;
  LONG AtRate;
} BATTERY_QUERY_INFORMATION, *PBATTERY_QUERY_INFORMATION;

typedef struct _BATTERY_INFORMATION {
  ULONG Capabilities;
  UCHAR Technology;
  UCHAR Reserved[3];
  UCHAR Chemistry[4];
  ULONG DesignedCapacity;
  ULONG FullChargedCapacity;
  ULONG DefaultAlert1;
  ULONG DefaultAlert2;
  ULONG CriticalBias;
  ULONG CycleCount;
} BATTERY_INFORMATION, *PBATTERY_INFORMATION;

typedef struct _BATTERY_MANUFACTURE_DATE {
  UCHAR Day;
  UCHAR Month;
  USHORT Year;
} BATTERY_MANUFACTURE_DATE, *PBATTERY_MANUFACTURE_DATE;

typedef enum _BATTERY_SET_INFORMATION_LEVEL {
  BatteryCriticalBias = 0,
  BatteryCharge,
  BatteryDischarge
} BATTERY_SET_INFORMATION_LEVEL;

typedef struct _BATTERY_SET_INFORMATION {
  ULONG BatteryTag;
  BATTERY_SET_INFORMATION_LEVEL InformationLevel;
  UCHAR Buffer[1];
} BATTERY_SET_INFORMATION, *PBATTERY_SET_INFORMATION;

typedef struct _BATTERY_WAIT_STATUS {
  ULONG BatteryTag;
  ULONG Timeout;
  ULONG PowerState;
  ULONG LowCapacity;
  ULONG HighCapacity;
} BATTERY_WAIT_STATUS, *PBATTERY_WAIT_STATUS;

typedef struct _BATTERY_STATUS {
  ULONG PowerState;
  ULONG Capacity;
  ULONG Voltage;
  LONG Rate;
} BATTERY_STATUS, *PBATTERY_STATUS;

#ifndef _WINDOWS_H

/* BATTERY_MINIPORT_INFO.XxxVersion */
#define BATTERY_CLASS_MAJOR_VERSION       0x0001
#define BATTERY_CLASS_MINOR_VERSION       0x0000

_Function_class_(BCLASS_QUERY_TAG_CALLBACK)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
_Check_return_
typedef NTSTATUS
(NTAPI BCLASS_QUERY_TAG_CALLBACK)(
  _In_ PVOID Context,
  _Out_ PULONG BatteryTag);
typedef BCLASS_QUERY_TAG_CALLBACK *PBCLASS_QUERY_TAG_CALLBACK;

_Function_class_(BCLASS_QUERY_INFORMATION_CALLBACK)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
_Check_return_
typedef NTSTATUS
(NTAPI BCLASS_QUERY_INFORMATION_CALLBACK)(
  _In_ PVOID Context,
  _In_ ULONG BatteryTag,
  _In_ BATTERY_QUERY_INFORMATION_LEVEL Level,
  _In_ LONG AtRate,
  _Out_writes_bytes_to_(BufferLength, *ReturnedLength) PVOID Buffer,
  _In_ ULONG BufferLength,
  _Out_ PULONG ReturnedLength);
typedef BCLASS_QUERY_INFORMATION_CALLBACK *PBCLASS_QUERY_INFORMATION_CALLBACK;

_Function_class_(BCLASS_QUERY_STATUS_CALLBACK)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
_Check_return_
typedef NTSTATUS
(NTAPI BCLASS_QUERY_STATUS_CALLBACK)(
  _In_ PVOID Context,
  _In_ ULONG BatteryTag,
  _Out_ PBATTERY_STATUS BatteryStatus);
typedef BCLASS_QUERY_STATUS_CALLBACK *PBCLASS_QUERY_STATUS_CALLBACK;

typedef struct _BATTERY_NOTIFY {
  ULONG PowerState;
  ULONG LowCapacity;
  ULONG HighCapacity;
} BATTERY_NOTIFY, *PBATTERY_NOTIFY;

_Function_class_(BCLASS_SET_STATUS_NOTIFY_CALLBACK)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
_Check_return_
typedef NTSTATUS
(NTAPI BCLASS_SET_STATUS_NOTIFY_CALLBACK)(
  _In_ PVOID Context,
  _In_ ULONG BatteryTag,
  _In_ PBATTERY_NOTIFY BatteryNotify);
typedef BCLASS_SET_STATUS_NOTIFY_CALLBACK *PBCLASS_SET_STATUS_NOTIFY_CALLBACK;

_Function_class_(BCLASS_SET_INFORMATION_CALLBACK)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
_Check_return_
typedef NTSTATUS
(NTAPI BCLASS_SET_INFORMATION_CALLBACK)(
  _In_ PVOID Context,
  _In_ ULONG BatteryTag,
  _In_ BATTERY_SET_INFORMATION_LEVEL Level,
  _In_opt_ PVOID Buffer);
typedef BCLASS_SET_INFORMATION_CALLBACK *PBCLASS_SET_INFORMATION_CALLBACK;

_Function_class_(BCLASS_DISABLE_STATUS_NOTIFY_CALLBACK)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
_Check_return_
typedef NTSTATUS
(NTAPI BCLASS_DISABLE_STATUS_NOTIFY_CALLBACK)(
  _In_ PVOID Context);
typedef BCLASS_DISABLE_STATUS_NOTIFY_CALLBACK *PBCLASS_DISABLE_STATUS_NOTIFY_CALLBACK;

typedef PBCLASS_QUERY_TAG_CALLBACK BCLASS_QUERY_TAG;
typedef PBCLASS_QUERY_INFORMATION_CALLBACK BCLASS_QUERY_INFORMATION;
typedef PBCLASS_QUERY_STATUS_CALLBACK BCLASS_QUERY_STATUS;
typedef PBCLASS_SET_STATUS_NOTIFY_CALLBACK BCLASS_SET_STATUS_NOTIFY;
typedef PBCLASS_SET_INFORMATION_CALLBACK BCLASS_SET_INFORMATION;
typedef PBCLASS_DISABLE_STATUS_NOTIFY_CALLBACK BCLASS_DISABLE_STATUS_NOTIFY;

typedef struct _BATTERY_MINIPORT_INFO {
  USHORT MajorVersion;
  USHORT MinorVersion;
  PVOID Context;
  BCLASS_QUERY_TAG QueryTag;
  BCLASS_QUERY_INFORMATION QueryInformation;
  BCLASS_SET_INFORMATION SetInformation;
  BCLASS_QUERY_STATUS QueryStatus;
  BCLASS_SET_STATUS_NOTIFY SetStatusNotify;
  BCLASS_DISABLE_STATUS_NOTIFY DisableStatusNotify;
  PDEVICE_OBJECT Pdo;
  PUNICODE_STRING DeviceName;
} BATTERY_MINIPORT_INFO, *PBATTERY_MINIPORT_INFO;

#if (NTDDI_VERSION >= NTDDI_WINXP)

typedef struct _BATTERY_WMI_STATUS {
  ULONG Tag;
  ULONG RemainingCapacity;
  LONG ChargeRate;
  LONG DischargeRate;
  ULONG Voltage;
  BOOLEAN PowerOnline;
  BOOLEAN Charging;
  BOOLEAN Discharging;
  BOOLEAN Critical;
} BATTERY_WMI_STATUS, *PBATTERY_WMI_STATUS;

typedef struct _BATTERY_WMI_RUNTIME {
  ULONG Tag;
  ULONG EstimatedRuntime;
} BATTERY_WMI_RUNTIME, *PBATTERY_WMI_RUNTIME;

typedef struct _BATTERY_WMI_TEMPERATURE {
  ULONG Tag;
  ULONG Temperature;
} BATTERY_WMI_TEMPERATURE, *PBATTERY_WMI_TEMPERATURE;

typedef struct _BATTERY_WMI_FULL_CHARGED_CAPACITY {
  ULONG Tag;
  ULONG FullChargedCapacity;
} BATTERY_WMI_FULL_CHARGED_CAPACITY, *PBATTERY_WMI_FULL_CHARGED_CAPACITY;

typedef struct _BATTERY_WMI_CYCLE_COUNT {
  ULONG Tag;
  ULONG CycleCount;
} BATTERY_WMI_CYCLE_COUNT, *PBATTERY_WMI_CYCLE_COUNT;

typedef struct _BATTERY_WMI_STATIC_DATA {
  ULONG Tag;
  WCHAR ManufactureDate[25];
  BATTERY_REPORTING_SCALE Granularity [4];
  ULONG Capabilities;
  UCHAR Technology;
  ULONG Chemistry;
  ULONG DesignedCapacity;
  ULONG DefaultAlert1;
  ULONG DefaultAlert2;
  ULONG CriticalBias;
  WCHAR Strings[1];
} BATTERY_WMI_STATIC_DATA, *PBATTERY_WMI_STATIC_DATA;

typedef struct _BATTERY_WMI_STATUS_CHANGE {
  ULONG Tag;
  BOOLEAN PowerOnline;
  BOOLEAN Charging;
  BOOLEAN Discharging;
  BOOLEAN Critical;
} BATTERY_WMI_STATUS_CHANGE, *PBATTERY_WMI_STATUS_CHANGE;

typedef struct _BATTERY_TAG_CHANGE {
  ULONG Tag;
} BATTERY_TAG_CHANGE, *PBATTERY_TAG_CHANGE;

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if defined(_BATTERYCLASS_)
#define BCLASSAPI
#else
#define BCLASSAPI DECLSPEC_IMPORT
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
_Check_return_
BCLASSAPI
NTSTATUS
NTAPI
BatteryClassInitializeDevice(
  _In_ PBATTERY_MINIPORT_INFO MiniportInfo,
  _Out_ PVOID *ClassData);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Check_return_
BCLASSAPI
NTSTATUS
NTAPI
BatteryClassIoctl(
  _In_ PVOID ClassData,
  _Inout_ PIRP Irp);

_IRQL_requires_max_(DISPATCH_LEVEL)
BCLASSAPI
NTSTATUS
NTAPI
BatteryClassStatusNotify(
  _In_ PVOID ClassData);

#if (NTDDI_VERSION >= NTDDI_WINXP)

_IRQL_requires_max_(PASSIVE_LEVEL)
_Check_return_
BCLASSAPI
NTSTATUS
NTAPI
BatteryClassQueryWmiDataBlock(
  _In_ PVOID ClassData,
  _Inout_ PDEVICE_OBJECT DeviceObject,
  _Inout_ PIRP Irp,
  _In_ ULONG GuidIndex,
  _Out_writes_(1) PULONG InstanceLengthArray,
  _In_ ULONG OutBufferSize,
  _Out_writes_bytes_opt_(OutBufferSize) PUCHAR Buffer);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Check_return_
BCLASSAPI
NTSTATUS
NTAPI
BatteryClassSystemControl(
  _In_ PVOID ClassData,
  _In_ PVOID WmiLibContext, /* PWMILIB_CONTEXT */
  _In_ PDEVICE_OBJECT DeviceObject,
  _Inout_ PIRP Irp,
  _Out_ PVOID Disposition); /* PSYSCTL_IRP_DISPOSITION */

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

_IRQL_requires_max_(PASSIVE_LEVEL)
BCLASSAPI
NTSTATUS
NTAPI
BatteryClassUnload(
  _In_ PVOID ClassData);

#endif /* _WINDOWS_H */

#endif /* _BATCLASS_ */

#ifdef __cplusplus
}
#endif
