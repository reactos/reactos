/*
 * PROJECT: ReactOS ACPI bzus driver
 * FILE:    acpi/ospm/include/acpisys.h
 * PURPOSE: ACPI bus driver definitions
 */
//#define ACPI_DEBUG
#include <acpi.h>
#define __INCLUDE_TYPES_H
#include <platform/types.h>
#undef ROUND_UP
#include <ddk/ntddk.h>

typedef enum {
  dsStopped,
  dsStarted
} ACPI_DEVICE_STATE;

typedef struct _ACPI_DEVICE_EXTENSION
{
  // Physical Device Object
  PDEVICE_OBJECT Pdo;
  // Lower device object
  PDEVICE_OBJECT Ldo;
  // Current state of the driver
  ACPI_DEVICE_STATE State;
  // Supported system states
  BOOLEAN SystemStates[ACPI_S_STATE_COUNT];
} ACPI_DEVICE_EXTENSION, *PACPI_DEVICE_EXTENSION;

NTSTATUS
ACPIEnumerateSystemDevices(
  PACPI_DEVICE_EXTENSION DeviceExtension);

/* EOF */
