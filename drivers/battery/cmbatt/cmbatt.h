/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            drivers/battery/cmbatt/cmbatt.h
* PURPOSE:         Control Method Battery Miniclass Driver
* PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
*/

#pragma once

#include <ntddk.h>
#include <batclass.h>

typedef struct _CMBATT_DEVICE_EXTENSION {
  PDEVICE_OBJECT Pdo;
  PDEVICE_OBJECT Ldo;
  PDEVICE_OBJECT Fdo;
  PVOID BattClassHandle;
  LIST_ENTRY ListEntry;
} CMBATT_DEVICE_EXTENSION, *PCMBATT_DEVICE_EXTENSION;

NTSTATUS
NTAPI
CmBattQueryTag(PVOID Context,
               PULONG BatteryTag);

NTSTATUS
NTAPI
CmBattDisableStatusNotify(PVOID Context);

NTSTATUS
NTAPI
CmBattSetStatusNotify(PVOID Context,
                      ULONG BatteryTag,
                      PBATTERY_NOTIFY BatteryNotify);

NTSTATUS
NTAPI
CmBattQueryInformation(PVOID Context,
                       ULONG BatteryTag,
                       BATTERY_QUERY_INFORMATION_LEVEL Level,
                       OPTIONAL LONG AtRate,
                       PVOID Buffer,
                       ULONG BufferLength,
                       PULONG ReturnedLength);

NTSTATUS
NTAPI
CmBattQueryStatus(PVOID Context,
                  ULONG BatteryTag,
                  PBATTERY_STATUS BatteryStatus);

NTSTATUS
NTAPI
CmBattSetInformation(PVOID Context,
                     ULONG BatteryTag,
                     BATTERY_SET_INFORMATION_LEVEL Level,
                     OPTIONAL PVOID Buffer);
