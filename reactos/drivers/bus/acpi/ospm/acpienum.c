/* $Id: acpienum.c,v 1.3 2001/08/23 17:32:04 chorns Exp $
 *
 * PROJECT:         ReactOS ACPI bus driver
 * FILE:            acpi/ospm/acpienum.c
 * PURPOSE:         ACPI namespace enumerator
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      01-05-2001  CSH  Created
 */
#include <acpisys.h>
#include <bm.h>

#define NDEBUG
#include <debug.h>


void
bm_print1 (
	BM_NODE			*node,
	u32                     flags)
{
	ACPI_BUFFER             buffer;
	BM_DEVICE		*device = NULL;
	char                    *type_string = NULL;

	if (!node) {
		return;
	}

	device = &(node->device);

	if (flags & BM_PRINT_PRESENT) {
		if (!BM_DEVICE_PRESENT(device)) {
			return;
		}
	}

	buffer.length = 256;
	buffer.pointer = acpi_os_callocate(buffer.length);
	if (!buffer.pointer) {
		return;
	}

	acpi_get_name(device->acpi_handle, ACPI_FULL_PATHNAME, &buffer);

	switch(device->id.type) {
	case BM_TYPE_SYSTEM:
		type_string = "System";
		break;
	case BM_TYPE_SCOPE:
		type_string = "Scope";
		break;
	case BM_TYPE_PROCESSOR:
		type_string = "Processor";
		break;
	case BM_TYPE_THERMAL_ZONE:
		type_string = "ThermalZone";
		break;
	case BM_TYPE_POWER_RESOURCE:
		type_string = "PowerResource";
		break;
	case BM_TYPE_FIXED_BUTTON:
		type_string = "Button";
		break;
	case BM_TYPE_DEVICE:
		type_string = "Device";
		break;
	default:
		type_string = "Unknown";
		break;
	}

	if (!(flags & BM_PRINT_GROUP)) {
		DbgPrint("+------------------------------------------------------------\n");
	}

		DbgPrint("%s[0x%02x] hid[%s] %s\n", type_string, device->handle, device->id.hid, buffer.pointer);
		DbgPrint("  acpi_handle[0x%08x] flags[0x%02x] status[0x%02x]\n", device->acpi_handle, device->flags, device->status);

	if (flags & BM_PRINT_IDENTIFICATION) {
		DbgPrint("  identification: uid[%s] adr[0x%08x]\n", device->id.uid, device->id.adr);
	}

	if (flags & BM_PRINT_LINKAGE) {
		DbgPrint("  linkage: this[%p] parent[%p] next[%p]\n", node, node->parent, node->next);
		DbgPrint("    scope.head[%p] scope.tail[%p]\n", node->scope.head, node->scope.tail);
	}

	if (flags & BM_PRINT_POWER) {
		DbgPrint("  power: state[D%d] flags[0x%08X]\n", device->power.state, device->power.flags);
		DbgPrint("    S0[0x%02x] S1[0x%02x] S2[0x%02x]\n", device->power.dx_supported[0], device->power.dx_supported[1], device->power.dx_supported[2]);
		DbgPrint("    S3[0x%02x] S4[0x%02x] S5[0x%02x]\n", device->power.dx_supported[3], device->power.dx_supported[4], device->power.dx_supported[5]);
	}

	if (!(flags & BM_PRINT_GROUP)) {
		DbgPrint("+------------------------------------------------------------\n");
	}

	acpi_os_free(buffer.pointer);

	return;
}


NTSTATUS
ACPIEnumerateRootBusses(
  PFDO_DEVICE_EXTENSION DeviceExtension)
{
  BM_HANDLE_LIST HandleList;
  PACPI_DEVICE AcpiDevice;
  ACPI_STATUS AcpiStatus;
  BM_HANDLE DeviceHandle;
	BM_DEVICE_ID Criteria;
  KIRQL OldIrql;
  ULONG i;

  BM_NODE *Node;
  ULONG j;

  DPRINT("Called\n");

  RtlZeroMemory(&Criteria, sizeof(BM_DEVICE_ID));
  RtlMoveMemory(&Criteria.hid, PCI_ROOT_HID_STRING, sizeof(PCI_ROOT_HID_STRING));

  AcpiStatus = bm_search(BM_HANDLE_ROOT, &Criteria, &HandleList);

  if (ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("Got %d devices\n", HandleList.count);

    for (i = 0; i < HandleList.count; i++) {
      AcpiStatus = bm_get_node(HandleList.handles[i], 0, &Node);
      if (ACPI_SUCCESS(AcpiStatus)) {
        DPRINT("Got BM node information: (Node 0x%X)\n", Node);
        bm_print1(Node, BM_PRINT_ALL - BM_PRINT_PRESENT);
#if 1
        for (j=0; j < 4*1000;j++)
          KeStallExecutionProcessor(1000);
#endif
      } else {
        DPRINT("Could not get BM node\n");
      }

      AcpiDevice = (PACPI_DEVICE)ExAllocatePool(
          NonPagedPool, sizeof(ACPI_DEVICE));
      if (!AcpiDevice) {
        return STATUS_INSUFFICIENT_RESOURCES;
      }

      RtlZeroMemory(AcpiDevice, sizeof(ACPI_DEVICE));

      AcpiDevice->Pdo = NULL;
      AcpiDevice->BmHandle = HandleList.handles[i];

      KeAcquireSpinLock(&DeviceExtension->DeviceListLock, &OldIrql);
      InsertHeadList(&DeviceExtension->DeviceListHead,
        &AcpiDevice->DeviceListEntry);
      DeviceExtension->DeviceListCount++;
      KeReleaseSpinLock(&DeviceExtension->DeviceListLock, OldIrql);
    }
  } else {
    DPRINT("Got no devices (Status 0x%X)\n", AcpiStatus);
  }
        for (j=0; j < 4*10*1000;j++)
          KeStallExecutionProcessor(1000);

  return STATUS_SUCCESS;
}


NTSTATUS
ACPIEnumerateNamespace(
  PFDO_DEVICE_EXTENSION DeviceExtension)
{
  BM_HANDLE_LIST HandleList;
  PACPI_DEVICE AcpiDevice;
  ACPI_STATUS AcpiStatus;
  BM_HANDLE DeviceHandle;
	BM_DEVICE_ID Criteria;
  BM_NODE *Node;
  KIRQL OldIrql;
  ULONG i;

  DPRINT("Called\n");

	RtlZeroMemory(&Criteria, sizeof(BM_DEVICE_ID));

  DbgPrint("Listing ACPI namespace\n");
  Criteria.type = BM_TYPE_ALL;

  AcpiStatus = bm_search(BM_HANDLE_ROOT, &Criteria, &HandleList);
  if (ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("Got %d devices\n", HandleList.count);

    for (i = 0; i < HandleList.count; i++) {
      AcpiStatus = bm_get_node(HandleList.handles[i], 0, &Node);
      if (ACPI_SUCCESS(AcpiStatus)) {
        DPRINT("Got BM node information: (Node 0x%X)\n", Node);
#if 0
        {
          ULONG j;

          bm_print1(Node, BM_PRINT_ALL - BM_PRINT_PRESENT);
          for (j=0; j < 4*1000;j++)
            KeStallExecutionProcessor(1000);
        }
#endif
      } else {
        DPRINT("Could not get BM node\n");
      }

      AcpiDevice = (PACPI_DEVICE)ExAllocatePool(
          NonPagedPool, sizeof(ACPI_DEVICE));
      if (!AcpiDevice) {
        return STATUS_INSUFFICIENT_RESOURCES;
      }

      RtlZeroMemory(AcpiDevice, sizeof(ACPI_DEVICE));

      AcpiDevice->Pdo = NULL;
      AcpiDevice->BmHandle = HandleList.handles[i];

      KeAcquireSpinLock(&DeviceExtension->DeviceListLock, &OldIrql);
      InsertHeadList(&DeviceExtension->DeviceListHead,
        &AcpiDevice->DeviceListEntry);
      DeviceExtension->DeviceListCount++;
      KeReleaseSpinLock(&DeviceExtension->DeviceListLock, OldIrql);
    }
  } else {
    DPRINT("Got no devices (Status 0x%X)\n", AcpiStatus);
  }

  return STATUS_SUCCESS;
}

/* EOF */
