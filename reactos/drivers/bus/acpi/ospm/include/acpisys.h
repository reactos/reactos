/*
 * PROJECT: ReactOS ACPI bus driver
 * FILE:    acpi/ospm/include/acpisys.h
 * PURPOSE: ACPI bus driver definitions
 */
#define ACPI_DEBUG
#include <acpi.h>
#include <ddk/ntddk.h>
#include <bm.h>

typedef ACPI_STATUS (*ACPI_DRIVER_FUNCTION)(VOID);


typedef enum {
  dsStopped,
  dsStarted,
  dsPaused,
  dsRemoved,
  dsSurpriseRemoved
} ACPI_DEVICE_STATE;


typedef struct _COMMON_DEVICE_EXTENSION
{
  // Pointer to device object, this device extension is associated with
  PDEVICE_OBJECT DeviceObject;
  // Wether this device extension is for an FDO or PDO
  BOOLEAN IsFDO;
  // Wether the device is removed
  BOOLEAN Removed;
  // Current device power state for the device
  DEVICE_POWER_STATE DevicePowerState;
  // Lower device object
  PDEVICE_OBJECT Ldo;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

/* Physical Device Object device extension for a child device */
typedef struct _PDO_DEVICE_EXTENSION
{
  // Common device data
  COMMON_DEVICE_EXTENSION Common;
  // Hardware IDs
  UNICODE_STRING HardwareIDs;
  // Compatible IDs
  UNICODE_STRING CompatibleIDs;
} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

typedef struct _FDO_DEVICE_EXTENSION
{
  // Common device data
  COMMON_DEVICE_EXTENSION Common;
  // Physical Device Object
  PDEVICE_OBJECT Pdo;
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
} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;


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


/* acpienum.c */

NTSTATUS
ACPIEnumerateRootBusses(
  PFDO_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
ACPIEnumerateNamespace(
  PFDO_DEVICE_EXTENSION DeviceExtension);


/* fdo.c */

NTSTATUS
STDCALL
FdoPnpControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp);

NTSTATUS
STDCALL
FdoPowerControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp);

/* pdo.c */

NTSTATUS
STDCALL
PdoPnpControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp);

NTSTATUS
STDCALL
PdoPowerControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp);

/* EOF */
