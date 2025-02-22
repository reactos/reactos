/*
 * PROJECT:         ReactOS ACPI-Compliant Control Method Battery
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/cmbatt/cmbatt.h
 * PURPOSE:         Main Header File
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#ifndef _CMBATT_PCH_
#define _CMBATT_PCH_

#include <wdm.h>
#include <batclass.h>
#include <wmilib.h>
#include <wdmguid.h>

#define IOCTL_BATTERY_QUERY_UNIQUE_ID  \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x101, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x294404

#define IOCTL_BATTERY_QUERY_STA  \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x102, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x294408

#define IOCTL_BATTERY_QUERY_PSR  \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x103, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x29440C

#define IOCTL_BATTERY_SET_TRIP_POINT \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x104, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x294410

#define IOCTL_BATTERY_QUERY_BIF_BIX \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x105, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x294414

#define IOCTL_BATTERY_QUERY_BST \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x106, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x294418

#define CMBATT_GENERIC_STATUS   0x01
#define CMBATT_GENERIC_INFO     0x02
#define CMBATT_GENERIC_WARNING  0x04
#define CMBATT_ACPI_WARNING     0x08
#define CMBATT_POWER_INFO       0x10
#define CMBATT_PNP_INFO         0x20
#define CMBATT_ACPI_ENTRY_EXIT  0x40
#define CMBATT_PNP_ENTRY_EXIT   0x200
#define CMBATT_ACPI_ASSERT      0x400

//
// Constant used to determine if the battery was discharging
// for over 15 seconds since last time the AC adapter got unplugged.
//
#define CMBATT_DISCHARGE_TIME   150000000

//
// Bogus constant used to determine if the remaining battery capacity
// overflows which is returned by the hardware.
//
#define CMBATT_CAPACITY_BOGUS   0x100000

typedef enum _CMBATT_EXTENSION_TYPE
{
    CmBattAcAdapter,
    CmBattBattery
} CMBATT_EXTENSION_TYPE;

#define ACPI_BUS_CHECK              0x00
#define ACPI_DEVICE_CHECK           0x01

#define ACPI_STA_PRESENT            0x01
#define ACPI_STA_ENABLED            0x02
#define ACPI_STA_SHOW_UI            0x04
#define ACPI_STA_FUNCTIONAL         0x08
#define ACPI_STA_BATTERY_PRESENT    0x10

#define ACPI_BATT_NOTIFY_STATUS     0x80
#define ACPI_BATT_NOTIFY_INFO       0x81

#define ACPI_BATT_STAT_DISCHARG		0x0001
#define ACPI_BATT_STAT_CHARGING		0x0002
#define ACPI_BATT_STAT_CRITICAL		0x0004

#define CM_MAX_VALUE                0x7FFFFFFF
#define CM_UNKNOWN_VALUE            0xFFFFFFFF

#define CONVERT_MAH_TO_MWH(mA, Volts) (((mA) * (Volts) + 500) / 1000)

#define CONVERT_BATT_INFO(Capacity, DesignVoltage) \
    (((Capacity) != BATTERY_UNKNOWN_CAPACITY) \
        ? CONVERT_MAH_TO_MWH((Capacity), (DesignVoltage)) \
        : BATTERY_UNKNOWN_CAPACITY)

typedef struct _ACPI_BST_DATA
{
    ULONG State;
    ULONG PresentRate;
    ULONG RemainingCapacity;
    ULONG PresentVoltage;
} ACPI_BST_DATA, *PACPI_BST_DATA;

#define ACPI_BATT_POWER_UNIT_WATTS  0x0
#define ACPI_BATT_POWER_UNIT_AMPS   0x1

#define ASCIIZ_MAX_LENGTH   256

typedef struct _ACPI_BIF_DATA
{
    ULONG PowerUnit;
    ULONG DesignCapacity;
    ULONG LastFullCapacity;
    ULONG BatteryTechnology;
    ULONG DesignVoltage;
    ULONG DesignCapacityWarning;
    ULONG DesignCapacityLow;
    ULONG BatteryCapacityGranularity1;
    ULONG BatteryCapacityGranularity2;
    CHAR ModelNumber[ASCIIZ_MAX_LENGTH];
    CHAR SerialNumber[ASCIIZ_MAX_LENGTH];
    CHAR BatteryType[ASCIIZ_MAX_LENGTH];
    CHAR OemInfo[ASCIIZ_MAX_LENGTH];
} ACPI_BIF_DATA, *PACPI_BIF_DATA;

typedef struct _ACPI_BIX_DATA
{
    ULONG Revision;
    ULONG PowerUnit;
    ULONG DesignCapacity;
    ULONG LastFullCapacity;
    ULONG BatteryTechnology;
    ULONG DesignVoltage;
    ULONG DesignCapacityWarning;
    ULONG DesignCapacityLow;
    ULONG CycleCount;
    ULONG Accuracy;
    ULONG MaxSampleTime;
    ULONG MinSampleTime;
    ULONG MaxAverageInterval;
    ULONG MinAverageInterval;
    ULONG BatteryCapacityGranularity1;
    ULONG BatteryCapacityGranularity2;
    CHAR ModelNumber[ASCIIZ_MAX_LENGTH];
    CHAR SerialNumber[ASCIIZ_MAX_LENGTH];
    CHAR BatteryType[ASCIIZ_MAX_LENGTH];
    CHAR OemInfo[ASCIIZ_MAX_LENGTH];
    ULONG SwapCapability;
} ACPI_BIX_DATA, *PACPI_BIX_DATA;

typedef struct _ACPI_BATT_STATIC_INFO
{
    ACPI_BIF_DATA BifData;
    ACPI_BIX_DATA BixData;
    BOOLEAN ExtendedData;
} ACPI_BATT_STATIC_INFO, *PACPI_BATT_STATIC_INFO;

#define CMBATT_BATT_STATIC_INFO_TAG     'nItS'

#define CMBATT_AR_NOTIFY            0x01
#define CMBATT_AR_INSERT            0x02
#define CMBATT_AR_REMOVE            0x04

typedef struct _CMBATT_DEVICE_EXTENSION
{
    CMBATT_EXTENSION_TYPE FdoType;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT FdoDeviceObject;
    PDEVICE_OBJECT PdoDeviceObject;
    PDEVICE_OBJECT AttachedDevice;
    FAST_MUTEX FastMutex;
    ULONG HandleCount;
    PIRP PowerIrp;
    POWER_STATE PowerState;
    WMILIB_CONTEXT WmiLibInfo;
    BOOLEAN WaitWakeEnable;
    IO_REMOVE_LOCK RemoveLock;
    ULONG DeviceId;
    PUNICODE_STRING DeviceName;
    ACPI_INTERFACE_STANDARD AcpiInterface;
    BOOLEAN DelayNotification;
    BOOLEAN ArFlag;
    PVOID ClassData;
    BOOLEAN Started;
    BOOLEAN NotifySent;
    LONG ArLockValue;
    ULONG TagData;
    ULONG Tag;
    ACPI_BST_DATA BstData;
    ACPI_BATT_STATIC_INFO BattInfo;
    ULONG Id;
    ULONG State;
    ULONG RemainingCapacity;
    ULONG PresentVoltage;
    ULONG Rate;
    BATTERY_INFORMATION BatteryInformation;
    ULONG BatteryCapacityGranularity1;
    ULONG BatteryCapacityGranularity2;
    BOOLEAN TripPointSet;
    ULONG TripPointValue;
    ULONG TripPointOld;
    ULONGLONG InterruptTime;
} CMBATT_DEVICE_EXTENSION, *PCMBATT_DEVICE_EXTENSION;

NTSTATUS
NTAPI
CmBattPowerDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
);

NTSTATUS
NTAPI
CmBattPnpDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
);

NTSTATUS
NTAPI
CmBattAddDevice(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT DeviceObject
);

NTSTATUS
NTAPI
CmBattSystemControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
);

NTSTATUS
NTAPI
CmBattGetBstData(
    PCMBATT_DEVICE_EXTENSION DeviceExtension,
    PACPI_BST_DATA BstData
);

NTSTATUS
NTAPI
CmBattGetPsrData(
    PDEVICE_OBJECT DeviceObject,
    PULONG PsrData
);

NTSTATUS
NTAPI
CmBattGetStaData(
    PDEVICE_OBJECT DeviceObject,
    PULONG StaData
);

NTSTATUS
NTAPI
CmBattGetBifData(
    PCMBATT_DEVICE_EXTENSION DeviceExtension,
    PACPI_BIF_DATA BifData
);

NTSTATUS
NTAPI
CmBattGetBixData(
    _In_ PCMBATT_DEVICE_EXTENSION DeviceExtension,
    _Out_ PACPI_BIX_DATA BixData
);

NTSTATUS
NTAPI
CmBattSetTripPpoint(
    PCMBATT_DEVICE_EXTENSION DeviceExtension,
    ULONG AlarmValue
);

VOID
NTAPI
CmBattNotifyHandler(
    IN PCMBATT_DEVICE_EXTENSION DeviceExtension,
    IN ULONG NotifyValue
);

NTSTATUS
NTAPI
CmBattWmiDeRegistration(
    PCMBATT_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
CmBattWmiRegistration(
    PCMBATT_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
CmBattGetUniqueId(
    PDEVICE_OBJECT DeviceObject,
    PULONG UniqueId
);

NTSTATUS
NTAPI
CmBattQueryInformation(
    IN PCMBATT_DEVICE_EXTENSION FdoExtension,
    IN ULONG Tag,
    IN BATTERY_QUERY_INFORMATION_LEVEL InfoLevel,
    IN OPTIONAL LONG AtRate,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnedLength
);

NTSTATUS
NTAPI
CmBattQueryStatus(
    IN PCMBATT_DEVICE_EXTENSION DeviceExtension,
    IN ULONG Tag,
    IN PBATTERY_STATUS BatteryStatus
);

NTSTATUS
NTAPI
CmBattSetStatusNotify(
    IN PCMBATT_DEVICE_EXTENSION DeviceExtension,
    IN ULONG BatteryTag,
    IN PBATTERY_NOTIFY BatteryNotify
);

NTSTATUS
NTAPI
CmBattDisableStatusNotify(
    IN PCMBATT_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
CmBattQueryTag(
    IN PCMBATT_DEVICE_EXTENSION DeviceExtension,
    OUT PULONG Tag
);

extern PDEVICE_OBJECT AcAdapterPdo;
extern ULONG CmBattDebug;

#endif /* _CMBATT_PCH_ */
