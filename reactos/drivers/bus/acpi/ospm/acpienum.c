/* $Id: acpienum.c,v 1.1 2001/05/01 23:00:03 chorns Exp $
 *
 * PROJECT:         ReactOS ACPI bus driver
 * FILE:            acpi/ospm/acpienum.c
 * PURPOSE:         ACPI namespace enumerator
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      01-05-2001  CSH  Created
 */
#include <acpisys.h>
#include <acnamesp.h>

#define NDEBUG
#include <debug.h>


ACPI_STATUS ACPIEnumerateDevice(
  ACPI_HANDLE ObjHandle,
  UINT32 Level,
  PVOID Context,
  PVOID *ReturnValue)
{
  ACPI_DEVICE_INFO Info;
  ACPI_STATUS Status;
  ACPI_BUFFER Path;
  CHAR Buffer[256];

  Path.length = sizeof(Buffer);
  Path.pointer = Buffer;

  /* Get the full path of this device and print it */
  Status = acpi_get_name(ObjHandle, ACPI_FULL_PATHNAME, &Path);

  if (ACPI_SUCCESS(Status)) {
    DPRINT("Device: %s\n", Path.pointer);
  }

  /* Get the device info for this device and print it */
  Status = acpi_get_object_info(ObjHandle, &Info);
  if (ACPI_SUCCESS(Status)) {
    DPRINT(" HID: %.8X, ADR: %.8X, Status: %x\n",
      Info.hardware_id, Info.address,
      Info.current_status);
  }
  return AE_OK;
}

NTSTATUS
ACPIEnumerateSystemDevices(
  PACPI_DEVICE_EXTENSION DeviceExtension)
{
  ACPI_HANDLE SysBusHandle;

  DPRINT("Enumerating system devices\n");

  acpi_get_handle(0, NS_SYSTEM_BUS, &SysBusHandle);
  DPRINT("Display of all devices in the namespace:\n");
  acpi_walk_namespace(ACPI_TYPE_DEVICE, SysBusHandle,
    ~0, ACPIEnumerateDevice, NULL, NULL);

  return STATUS_SUCCESS;
}

/* EOF */
