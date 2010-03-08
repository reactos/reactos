/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/battery/cmbatt/miniclass.c
 * PURPOSE:         Control Method Battery Miniclass Driver
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 */

#include <cmbatt.h>

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
CmBattQueryTag(PVOID Context,
               PULONG BatteryTag)
{
  UNIMPLEMENTED

  *BatteryTag = 0;

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmBattDisableStatusNotify(PVOID Context)
{
  UNIMPLEMENTED

  return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
CmBattSetStatusNotify(PVOID Context,
                      ULONG BatteryTag,
                      PBATTERY_NOTIFY BatteryNotify)
{
  UNIMPLEMENTED

  return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
CmBattQueryInformation(PVOID Context,
                       ULONG BatteryTag,
                       BATTERY_QUERY_INFORMATION_LEVEL Level,
                       OPTIONAL LONG AtRate,
                       PVOID Buffer,
                       ULONG BufferLength,
                       PULONG ReturnedLength)
{
  UNIMPLEMENTED

  return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
CmBattQueryStatus(PVOID Context,
                  ULONG BatteryTag,
                  PBATTERY_STATUS BatteryStatus)
{
  UNIMPLEMENTED

  return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
CmBattSetInformation(PVOID Context,
                     ULONG BatteryTag,
                     BATTERY_SET_INFORMATION_LEVEL Level,
                     OPTIONAL PVOID Buffer)
{
  UNIMPLEMENTED

  return STATUS_NOT_SUPPORTED;
}
