/*
 * PROJECT: ReactOS ACPI bus driver
 * FILE:    acpi/ospm/include/acpisys.h
 * PURPOSE: ACPI bus driver definitions
 */
#define ACPI_DEBUG
#include <acpi.h>
#define __INCLUDE_TYPES_H
#include <platform/types.h>
#undef ROUND_UP
#include <ddk/ntddk.h>
#include <bm.h>

typedef ACPI_STATUS (*ACPI_DRIVER_FUNCTION)(VOID);


typedef enum {
  dsStopped,
  dsStarted
} ACPI_DEVICE_STATE;

typedef struct _ACPI_DEVICE
{
  // Entry on device list
  LIST_ENTRY DeviceListEntry;
  // Bus manager handle
  BM_HANDLE BmHandle;
  // Physical Device Object
  PDEVICE_OBJECT Pdo;
  // Initialization function
  ACPI_DRIVER_FUNCTION Initialize;
  // Cleanup function
  ACPI_DRIVER_FUNCTION Terminate;
} ACPI_DEVICE, *PACPI_DEVICE;

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
  // Namespace device list
  LIST_ENTRY DeviceListHead;
  // Number of devices in device list
  ULONG DeviceListCount;
  // Lock for namespace device list
  KSPIN_LOCK DeviceListLock;
} ACPI_DEVICE_EXTENSION, *PACPI_DEVICE_EXTENSION;

NTSTATUS
ACPIEnumerateRootBusses(
  PACPI_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
ACPIEnumerateNamespace(
  PACPI_DEVICE_EXTENSION DeviceExtension);

/* EOF */
