/*
 * PROJECT:         ReactOS Composite Battery Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/compbatt/compbatt.h
 * PURPOSE:         Main Header File
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#ifndef _COMPBATT_PCH_
#define _COMPBATT_PCH_

#include <wdm.h>
#include <batclass.h>

#define COMPBATT_BATTERY_INFORMATION_PRESENT    0x04
#define COMPBATT_TAG_ASSIGNED                   0x80

typedef struct _COMPBATT_BATTERY_DATA
{
    LIST_ENTRY BatteryLink;
    IO_REMOVE_LOCK RemoveLock;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    WORK_QUEUE_ITEM WorkItem;
    BOOLEAN WaitFlag;
    BATTERY_WAIT_STATUS WaitStatus;
    union
    {
        BATTERY_WAIT_STATUS WorkerWaitStatus;
        BATTERY_STATUS WorkerStatus;
    };
    ULONG Tag;
    ULONG Flags;
    BATTERY_INFORMATION BatteryInformation;
    BATTERY_STATUS BatteryStatus;
    ULONGLONG InterruptTime;
    UNICODE_STRING BatteryName;
} COMPBATT_BATTERY_DATA, *PCOMPBATT_BATTERY_DATA;

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
    ULONGLONG InterruptTime;
    POWER_STATE PowerState;
    ULONG LowCapacity;
    ULONG HighCapacity;
    PDEVICE_OBJECT AttachedDevice;
    PDEVICE_OBJECT DeviceObject;
    PVOID NotificationEntry;
} COMPBATT_DEVICE_EXTENSION, *PCOMPBATT_DEVICE_EXTENSION;

NTSTATUS
NTAPI
CompBattAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PdoDeviceObject
);

NTSTATUS
NTAPI
CompBattPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NTAPI
CompBattPnpDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NTAPI
CompBattQueryInformation(
    IN PCOMPBATT_DEVICE_EXTENSION FdoExtension,
    IN ULONG Tag,
    IN BATTERY_QUERY_INFORMATION_LEVEL InfoLevel,
    IN OPTIONAL LONG AtRate,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnedLength
);
                       
NTSTATUS
NTAPI
CompBattQueryStatus(
    IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    IN ULONG Tag,
    IN PBATTERY_STATUS BatteryStatus
);

NTSTATUS
NTAPI
CompBattSetStatusNotify(
    IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    IN ULONG BatteryTag,
    IN PBATTERY_NOTIFY BatteryNotify
);

NTSTATUS
NTAPI
CompBattDisableStatusNotify(
    IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
CompBattQueryTag(
    IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    OUT PULONG Tag
);

NTSTATUS
NTAPI
CompBattMonitorIrpComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
);

NTSTATUS
NTAPI
CompBattMonitorIrpCompleteWorker(
    IN PCOMPBATT_BATTERY_DATA BatteryData
);

NTSTATUS
NTAPI
CompBattGetDeviceObjectPointer(
    IN PUNICODE_STRING DeviceName,
    IN ACCESS_MASK DesiredAccess,
    OUT PFILE_OBJECT *FileObject,
    OUT PDEVICE_OBJECT *DeviceObject
);

NTSTATUS
NTAPI
BatteryIoctl(
    IN ULONG IoControlCode, 
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl
);

extern ULONG CompBattDebug;

#endif /* _COMPBATT_PCH_ */
