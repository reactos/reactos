/*
 * PROJECT:     ReactOS Composite Battery Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Composite battery main header file
 * COPYRIGHT:   Copyright 2010 ReactOS Portable Systems Group <ros.arm@reactos.org>
 *              Copyright 2024 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#ifndef _COMPBATT_PCH_
#define _COMPBATT_PCH_

#include <wdm.h>
#include <batclass.h>

/* DEFINES ********************************************************************/

//
// I/O remove lock allocate tag
//
#define COMPBATT_TAG                            'aBoC'

//
// Composite battery flags
//
#define COMPBATT_BATTERY_INFORMATION_PRESENT    0x04
#define COMPBATT_STATUS_NOTIFY_SET              0x10
#define COMPBATT_TAG_ASSIGNED                   0x80

//
// IRP complete worker mode states
//
#define COMPBATT_QUERY_TAG                      1
#define COMPBATT_READ_STATUS                    2

//
// Low/High capacity wait constants
//
#define COMPBATT_WAIT_MIN_LOW_CAPACITY          0
#define COMPBATT_WAIT_MAX_HIGH_CAPACITY         0x7FFFFFFF

//
// One hour in seconds, used to calculate the total rate of each battery for time estimation
//
#define COMPBATT_ATRATE_HOUR_IN_SECS            3600

//
// Time constant of which the battery status data is considered fresh (50000000 * 100ns == 5s)
//
#define COMPBATT_FRESH_STATUS_TIME              50000000

//
// Macro that calculates the delta of a battery's capacity
//
#define COMPUTE_BATT_CAP_DELTA(LowDiff, Batt, TotalRate)            \
    ((ULONG)(((LONGLONG)LowDiff * (Batt)->BatteryStatus.Rate) / TotalRate))

//
// Macro that calculates the "At Rate" time drain of the battery
//
#define COMPUTE_ATRATE_DRAIN(Batt, Time)                            \
    ((LONG)((Batt)->BatteryStatus.Capacity) * COMPBATT_ATRATE_HOUR_IN_SECS / Time)

//
// Composite battery debug levels
//
#define COMPBATT_DEBUG_INFO                     0x1
#define COMPBATT_DEBUG_TRACE                    0x2
#define COMPBATT_DEBUG_WARN                     0x4
#define COMPBATT_DEBUG_ERR                      0x10
#define COMPBATT_DEBUG_ALL_LEVELS               (COMPBATT_DEBUG_INFO | COMPBATT_DEBUG_TRACE | COMPBATT_DEBUG_WARN | COMPBATT_DEBUG_ERR)

/* STRUCTURES *****************************************************************/

//
// Individual ACPI battery data
//
typedef struct _COMPBATT_BATTERY_DATA
{
    LIST_ENTRY BatteryLink;
    IO_REMOVE_LOCK RemoveLock;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    WORK_QUEUE_ITEM WorkItem;
    UCHAR Mode;
    BATTERY_WAIT_STATUS WaitStatus;
    union
    {
        BATTERY_WAIT_STATUS WorkerWaitStatus;
        BATTERY_STATUS WorkerStatus;
        ULONG WorkerTag;
    } WorkerBuffer;
    ULONG Tag;
    ULONG Flags;
    BATTERY_INFORMATION BatteryInformation;
    BATTERY_STATUS BatteryStatus;
    ULONGLONG InterruptTime;
    UNICODE_STRING BatteryName;
} COMPBATT_BATTERY_DATA, *PCOMPBATT_BATTERY_DATA;

//
// Composite battery device extension data
//
typedef struct _COMPBATT_DEVICE_EXTENSION
{
    PVOID ClassData;
    ULONG NextTag;
    LIST_ENTRY BatteryList;
    FAST_MUTEX Lock;
    ULONG Tag;
    ULONG Flags;
    BATTERY_INFORMATION BatteryInformation;
    BATTERY_STATUS BatteryStatus;
    BATTERY_WAIT_STATUS WaitNotifyStatus;
    ULONGLONG InterruptTime;
    PDEVICE_OBJECT AttachedDevice;
    PDEVICE_OBJECT DeviceObject;
    PVOID NotificationEntry;
} COMPBATT_DEVICE_EXTENSION, *PCOMPBATT_DEVICE_EXTENSION;

/* PROTOTYPES *****************************************************************/

NTSTATUS
NTAPI
CompBattAddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PdoDeviceObject
);

NTSTATUS
NTAPI
CompBattPowerDispatch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
);

NTSTATUS
NTAPI
CompBattPnpDispatch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
);

NTSTATUS
NTAPI
CompBattQueryInformation(
    _In_ PCOMPBATT_DEVICE_EXTENSION FdoExtension,
    _In_ ULONG Tag,
    _In_ BATTERY_QUERY_INFORMATION_LEVEL InfoLevel,
    _In_opt_ LONG AtRate,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnedLength
);

NTSTATUS
NTAPI
CompBattQueryStatus(
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG Tag,
    _Out_ PBATTERY_STATUS BatteryStatus
);

NTSTATUS
NTAPI
CompBattGetEstimatedTime(
    _Out_ PULONG Time,
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
CompBattSetStatusNotify(
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG BatteryTag,
    _In_ PBATTERY_NOTIFY BatteryNotify
);

NTSTATUS
NTAPI
CompBattDisableStatusNotify(
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
CompBattQueryTag(
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    _Out_ PULONG Tag
);

NTSTATUS
NTAPI
CompBattMonitorIrpComplete(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PVOID Context
);

VOID
NTAPI
CompBattMonitorIrpCompleteWorker(
    _In_ PCOMPBATT_BATTERY_DATA BatteryData
);

NTSTATUS
NTAPI
CompBattGetDeviceObjectPointer(
    _In_ PUNICODE_STRING DeviceName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PFILE_OBJECT *FileObject,
    _Out_ PDEVICE_OBJECT *DeviceObject
);

NTSTATUS
NTAPI
BatteryIoctl(
    _In_ ULONG IoControlCode,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_ PVOID OutputBuffer,
    _Inout_ ULONG OutputBufferLength,
    _In_ BOOLEAN InternalDeviceIoControl
);

NTSTATUS
NTAPI
CompBattRemoveBattery(
    _In_ PUNICODE_STRING BatteryName,
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension
);

extern ULONG CompBattDebug;

#endif /* _COMPBATT_PCH_ */
