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

#ifndef __BATCLASS_H
#define __BATCLASS_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ntddk.h"

#if defined(_BATTERYCLASS_)
  #define BCLASSAPI DECLSPEC_EXPORT
#else
  #define BCLASSAPI DECLSPEC_IMPORT
#endif


/* Battery device GUIDs */

DEFINE_GUID(GUID_DEVICE_BATTERY,
  0x72631e54L, 0x78A4, 0x11d0, 0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a);

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


/* BATTERY_INFORMATION.Capabilities constants */
#define BATTERY_SET_CHARGE_SUPPORTED      0x00000001
#define BATTERY_SET_DISCHARGE_SUPPORTED   0x00000002
#define BATTERY_SET_RESUME_SUPPORTED      0x00000004
#define BATTERY_IS_SHORT_TERM             0x20000000
#define BATTERY_CAPACITY_RELATIVE         0x40000000
#define BATTERY_SYSTEM_BATTERY            0x80000000

typedef struct _BATTERY_INFORMATION {
  ULONG  Capabilities;
  UCHAR  Technology;
  UCHAR  Reserved[3];
  UCHAR  Chemistry[4];
  ULONG  DesignedCapacity;
  ULONG  FullChargedCapacity;
  ULONG  DefaultAlert1;
  ULONG  DefaultAlert2;
  ULONG  CriticalBias;
  ULONG  CycleCount;
} BATTERY_INFORMATION, *PBATTERY_INFORMATION;

typedef struct _BATTERY_MANUFACTURE_DATE {
  UCHAR  Day;
  UCHAR  Month;
  USHORT  Year;
} BATTERY_MANUFACTURE_DATE, *PBATTERY_MANUFACTURE_DATE;

typedef struct _BATTERY_NOTIFY {
	ULONG  PowerState;
	ULONG  LowCapacity;
	ULONG  HighCapacity;
} BATTERY_NOTIFY, *PBATTERY_NOTIFY;

/* BATTERY_STATUS.PowerState flags */
#define BATTERY_POWER_ON_LINE             0x00000001
#define BATTERY_DISCHARGING               0x00000002
#define BATTERY_CHARGING                  0x00000004
#define BATTERY_CRITICAL                  0x00000008

/* BATTERY_STATUS.Voltage constant */
#define BATTERY_UNKNOWN_VOLTAGE           0xFFFFFFFF

/* BATTERY_STATUS.Rate constant */
#define BATTERY_UNKNOWN_RATE              0x80000000

typedef struct _BATTERY_STATUS {
  ULONG  PowerState;
  ULONG  Capacity;
  ULONG  Voltage;
  LONG  Rate;
} BATTERY_STATUS, *PBATTERY_STATUS;

/* BATTERY_INFORMATION.Capacity constants */
#define BATTERY_UNKNOWN_CAPACITY          0xFFFFFFFF

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

/* BatteryEstimatedTime constant */
#define BATTERY_UNKNOWN_TIME              0x80000000

/* NTSTATUS possibly returned by BCLASS_QUERY_STATUS */
#define BATTERY_TAG_INVALID 0

typedef struct _BATTERY_QUERY_INFORMATION {
  ULONG  BatteryTag;
  BATTERY_QUERY_INFORMATION_LEVEL  InformationLevel;
  LONG  AtRate;
} BATTERY_QUERY_INFORMATION, *PBATTERY_QUERY_INFORMATION;

typedef enum _BATTERY_SET_INFORMATION_LEVEL {
  BatteryCriticalBias = 0,
  BatteryCharge,
  BatteryDischarge
} BATTERY_SET_INFORMATION_LEVEL;

#define MAX_BATTERY_STRING_SIZE           128

typedef struct _BATTERY_SET_INFORMATION {
	ULONG  BatteryTag;
	BATTERY_SET_INFORMATION_LEVEL  InformationLevel;
	UCHAR  Buffer[1];
} BATTERY_SET_INFORMATION, *PBATTERY_SET_INFORMATION;

typedef struct _BATTERY_WAIT_STATUS {
	ULONG  BatteryTag;
	ULONG  Timeout;
	ULONG  PowerState;
	ULONG  LowCapacity;
	ULONG  HighCapacity;
} BATTERY_WAIT_STATUS, *PBATTERY_WAIT_STATUS;


#define IOCTL_BATTERY_QUERY_TAG \
  CTL_CODE(FILE_DEVICE_BATTERY, 0x10, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_BATTERY_QUERY_INFORMATION \
  CTL_CODE(FILE_DEVICE_BATTERY, 0x11, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_BATTERY_SET_INFORMATION \
  CTL_CODE(FILE_DEVICE_BATTERY, 0x12, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_BATTERY_QUERY_STATUS \
  CTL_CODE(FILE_DEVICE_BATTERY, 0x13, METHOD_BUFFERED, FILE_READ_ACCESS)


typedef NTSTATUS DDKAPI
(*BCLASS_DISABLE_STATUS_NOTIFY)(
  IN PVOID  Context);

typedef NTSTATUS DDKAPI
(*BCLASS_QUERY_INFORMATION)(
  IN PVOID  Context,
  IN ULONG  BatteryTag,
  IN BATTERY_QUERY_INFORMATION_LEVEL  Level,
  IN LONG  AtRate  OPTIONAL,
  OUT PVOID  Buffer,
  IN ULONG  BufferLength,
  OUT PULONG  ReturnedLength);

typedef NTSTATUS DDKAPI
(*BCLASS_QUERY_STATUS)(
  IN PVOID  Context,
  IN ULONG  BatteryTag,
  OUT PBATTERY_STATUS  BatteryStatus);

typedef NTSTATUS DDKAPI
(*BCLASS_QUERY_TAG)(
  IN PVOID  Context,
  OUT PULONG  BatteryTag);

typedef NTSTATUS DDKAPI
(*BCLASS_SET_INFORMATION)(
  IN PVOID  Context,
  IN ULONG  BatteryTag,
  IN BATTERY_SET_INFORMATION_LEVEL  Level,
  IN PVOID  Buffer  OPTIONAL);

typedef NTSTATUS DDKAPI
(*BCLASS_SET_STATUS_NOTIFY)(
  IN PVOID  Context,
  IN ULONG  BatteryTag,
  IN PBATTERY_NOTIFY  BatteryNotify);


typedef struct _BATTERY_MINIPORT_INFO {
  USHORT  MajorVersion;
  USHORT  MinorVersion;
  PVOID  Context;
  BCLASS_QUERY_TAG  QueryTag;
  BCLASS_QUERY_INFORMATION  QueryInformation;
  BCLASS_SET_INFORMATION  SetInformation;
  BCLASS_QUERY_STATUS  QueryStatus;
  BCLASS_SET_STATUS_NOTIFY  SetStatusNotify;
  BCLASS_DISABLE_STATUS_NOTIFY  DisableStatusNotify;
  PDEVICE_OBJECT  Pdo;
  PUNICODE_STRING  DeviceName;
} BATTERY_MINIPORT_INFO, *PBATTERY_MINIPORT_INFO;

/* BATTERY_MINIPORT_INFO.XxxVersion */
#define BATTERY_CLASS_MAJOR_VERSION       0x0001
#define BATTERY_CLASS_MINOR_VERSION       0x0000


BCLASSAPI
NTSTATUS
DDKAPI
BatteryClassInitializeDevice(
  IN PBATTERY_MINIPORT_INFO  MiniportInfo,
  IN PVOID  *ClassData);

BCLASSAPI
NTSTATUS
DDKAPI
BatteryClassIoctl(
  IN PVOID  ClassData,
  IN PIRP  Irp);

BCLASSAPI
NTSTATUS
DDKAPI
BatteryClassQueryWmiDataBlock(
  IN PVOID  ClassData,
  IN PDEVICE_OBJECT  DeviceObject,
  IN PIRP  Irp,
  IN ULONG  GuidIndex,
  IN OUT PULONG  InstanceLengthArray,
  IN ULONG  OutBufferSize,
  OUT PUCHAR  Buffer);

BCLASSAPI
NTSTATUS
DDKAPI
BatteryClassStatusNotify(
  IN PVOID  ClassData);

BCLASSAPI
NTSTATUS
DDKAPI
BatteryClassSystemControl(
  IN  PVOID  ClassData,
  IN  PWMILIB_CONTEXT  WmiLibContext,
  IN  PDEVICE_OBJECT  DeviceObject,
  IN  PIRP  Irp,
  OUT PSYSCTL_IRP_DISPOSITION  Disposition);

BCLASSAPI
NTSTATUS
DDKAPI
BatteryClassUnload(
  IN PVOID  ClassData);

#ifdef __cplusplus
}
#endif

#endif /* __BATCLASS_H */
