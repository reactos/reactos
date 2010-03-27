/*
 * PROJECT:         ReactOS Composite Battery Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/compbatt/compbatt.h
 * PURPOSE:         Main Header File
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include <ntddk.h>
#include <initguid.h>
#include <batclass.h>
#include <debug.h>

typedef struct _COMPBATT_BATTERY_ENTRY
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
} COMPBATT_BATTERY_ENTRY, *PCOMPBATT_BATTERY_ENTRY;

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

extern ULONG CmBattDebug;

/* EOF */
