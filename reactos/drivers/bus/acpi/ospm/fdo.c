/* $Id: fdo.c,v 1.1 2001/08/23 17:32:04 chorns Exp $
 *
 * PROJECT:         ReactOS ACPI bus driver
 * FILE:            acpi/ospm/fdo.c
 * PURPOSE:         ACPI device object dispatch routines
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      08-08-2001  CSH  Created
 */
#include <acpisys.h>
#include <bm.h>
#include <bn.h>

#define NDEBUG
#include <debug.h>

/*** PRIVATE *****************************************************************/

FADT_DESCRIPTOR_REV2	acpi_fadt;

NTSTATUS
FdoQueryBusRelations(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPDO_DEVICE_EXTENSION PdoDeviceExtension;
  PFDO_DEVICE_EXTENSION DeviceExtension;
  PDEVICE_RELATIONS Relations;
  PLIST_ENTRY CurrentEntry;
  ANSI_STRING AnsiString;
  ACPI_STATUS AcpiStatus;
  PACPI_DEVICE Device;
  NTSTATUS Status;
  BM_NODE *Node;
  ULONG Size;
  ULONG i;

  DPRINT("Called\n");

  DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  Size = sizeof(DEVICE_RELATIONS) + sizeof(Relations->Objects) *
    (DeviceExtension->DeviceListCount - 1);
  Relations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, Size);
  if (!Relations)
    return STATUS_INSUFFICIENT_RESOURCES;

  Relations->Count = DeviceExtension->DeviceListCount;

  i = 0;
  CurrentEntry = DeviceExtension->DeviceListHead.Flink;
  while (CurrentEntry != &DeviceExtension->DeviceListHead) {
    Device = CONTAINING_RECORD(CurrentEntry, ACPI_DEVICE, DeviceListEntry);

    /* FIXME: For ACPI namespace devices on the motherboard create filter DOs
       and attach them just above the ACPI bus device object (PDO) */

    /* FIXME: For other devices in ACPI namespace, but not on motherboard,
       create PDOs */

    if (!Device->Pdo) {
      /* Create a physical device object for the
         device as it does not already have one */
      Status = IoCreateDevice(DeviceObject->DriverObject, 0,
        NULL, FILE_DEVICE_CONTROLLER, 0, FALSE, &Device->Pdo);
      if (!NT_SUCCESS(Status)) {
        DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
        /* FIXME: Cleanup all new PDOs created in this call */
        ExFreePool(Relations);
        return Status;
      }

      PdoDeviceExtension = ExAllocatePool(
        NonPagedPool,
        sizeof(PDO_DEVICE_EXTENSION));
      if (!PdoDeviceExtension) {
        /* FIXME: Cleanup all new PDOs created in this call */
        ExFreePool(Relations);
      }

      RtlZeroMemory(
        PdoDeviceExtension,
        sizeof(PDO_DEVICE_EXTENSION));

      Device->Pdo->DeviceExtension = PdoDeviceExtension;

      Device->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

      PdoDeviceExtension->DeviceObject = Device->Pdo;

      PdoDeviceExtension->DevicePowerState = PowerDeviceD0;

      PdoDeviceExtension->Ldo = IoAttachDeviceToDeviceStack(
        DeviceObject,
        Device->Pdo);

      RtlInitUnicodeString(&PdoDeviceExtension->HardwareIDs, NULL);
      RtlInitUnicodeString(&PdoDeviceExtension->CompatibleIDs, NULL);

      AcpiStatus = bm_get_node(Device->BmHandle, 0, &Node);
      if (ACPI_SUCCESS(AcpiStatus)) {
        if (Node->device.flags & BM_FLAGS_HAS_A_HID) {
          RtlInitAnsiString(&AnsiString, Node->device.id.hid);
          Status = RtlAnsiStringToUnicodeString(
            &PdoDeviceExtension->HardwareIDs,
            &AnsiString,
            TRUE);
          assert(NT_SUCCESS(Status));
        }

        if (Node->device.flags & BM_FLAGS_HAS_A_CID) {
          RtlInitAnsiString(&AnsiString, Node->device.id.cid);
          Status = RtlAnsiStringToUnicodeString(
            &PdoDeviceExtension->CompatibleIDs,
            &AnsiString,
            TRUE);
          assert(NT_SUCCESS(Status));
        }
      }
    }

    /* Reference the physical device object. The PnP manager
       will dereference it again when it is no longer needed */
    ObReferenceObject(Device->Pdo);

    Relations->Objects[i] = Device->Pdo;

    i++;

    CurrentEntry = CurrentEntry->Flink;
  }

  Irp->IoStatus.Information = (ULONG)Relations;

  return Status;
}


VOID ACPIPrintInfo(
	PFDO_DEVICE_EXTENSION DeviceExtension)
{
  DbgPrint("ACPI: System firmware supports:\n");

	/*
	 * Print out basic system information
	 */
	DbgPrint("+------------------------------------------------------------\n");
	DbgPrint("| Sx states: %cS0 %cS1 %cS2 %cS3 %cS4 %cS5\n",
    (DeviceExtension->SystemStates[0]?'+':'-'),
    (DeviceExtension->SystemStates[1]?'+':'-'),
    (DeviceExtension->SystemStates[2]?'+':'-'),
    (DeviceExtension->SystemStates[3]?'+':'-'),
    (DeviceExtension->SystemStates[4]?'+':'-'),
    (DeviceExtension->SystemStates[5]?'+':'-'));
	DbgPrint("+------------------------------------------------------------\n");
}

NTSTATUS
ACPIInitializeInternalDriver(
  PFDO_DEVICE_EXTENSION DeviceExtension,
  ACPI_DRIVER_FUNCTION Initialize,
  ACPI_DRIVER_FUNCTION Terminate)
{
  ACPI_STATUS AcpiStatus;
  PACPI_DEVICE AcpiDevice;

  AcpiStatus = Initialize();
  if (!ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("BN init status 0x%X\n", AcpiStatus);
    return STATUS_UNSUCCESSFUL;
  }
#if 0
  AcpiDevice = (PACPI_DEVICE)ExAllocatePool(
    NonPagedPool, sizeof(ACPI_DEVICE));
  if (!AcpiDevice) {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  AcpiDevice->Initialize = Initialize;
  AcpiDevice->Terminate = Terminate;

  /* FIXME: Create PDO */

  AcpiDevice->Pdo = NULL;
  //AcpiDevice->BmHandle = HandleList.handles[i];

  ExInterlockedInsertHeadList(&DeviceExtension->DeviceListHead,
    &AcpiDevice->ListEntry, &DeviceExtension->DeviceListLock);
#endif
  return STATUS_SUCCESS;
}


NTSTATUS
ACPIInitializeInternalDrivers(
  PFDO_DEVICE_EXTENSION DeviceExtension)
{
  NTSTATUS Status;

  ULONG j;

  Status = ACPIInitializeInternalDriver(DeviceExtension,
    bn_initialize, bn_terminate);

  return STATUS_SUCCESS;
}


NTSTATUS
FdoStartDevice(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PFDO_DEVICE_EXTENSION DeviceExtension;
	ACPI_PHYSICAL_ADDRESS rsdp;
	ACPI_SYSTEM_INFO SysInfo;
  ACPI_STATUS AcpiStatus;
  ACPI_BUFFER	Buffer;
	UCHAR TypeA, TypeB;
  ULONG i;

  DPRINT("Called\n");

  DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  assert(DeviceExtension->State == dsStopped);

  AcpiStatus = acpi_initialize_subsystem();
  if (!ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("acpi_initialize_subsystem() failed with status 0x%X\n", AcpiStatus);
    return STATUS_UNSUCCESSFUL;
  }

  AcpiStatus = acpi_find_root_pointer(&rsdp);
  if (!ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("acpi_find_root_pointer() failed with status 0x%X\n", AcpiStatus);
    return STATUS_UNSUCCESSFUL;
	}

  /* From this point on, on error we must call acpi_terminate() */

  AcpiStatus = acpi_load_tables(rsdp);
	if (!ACPI_SUCCESS(AcpiStatus)) {
		DPRINT("acpi_load_tables() failed with status 0x%X\n", AcpiStatus);
		acpi_terminate();
		return STATUS_UNSUCCESSFUL;
	}

	Buffer.length  = sizeof(SysInfo);
	Buffer.pointer = &SysInfo;

  AcpiStatus = acpi_get_system_info(&Buffer);
	if (!ACPI_SUCCESS(AcpiStatus)) {
		DPRINT("acpi_get_system_info() failed with status 0x%X\n", AcpiStatus);
		acpi_terminate();
		return STATUS_UNSUCCESSFUL;
	}

	DPRINT("ACPI CA Core Subsystem version 0x%X\n", SysInfo.acpi_ca_version);

  assert(SysInfo.num_table_types > ACPI_TABLE_FADT);

  RtlMoveMemory(&acpi_fadt,
    &SysInfo.table_info[ACPI_TABLE_FADT],
    sizeof(FADT_DESCRIPTOR_REV2));

  AcpiStatus = acpi_enable_subsystem(ACPI_FULL_INITIALIZATION);
  if (!ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("acpi_enable_subsystem() failed with status 0x%X\n", AcpiStatus);
    acpi_terminate();
    return STATUS_UNSUCCESSFUL;
  }

  DPRINT("ACPI CA Core Subsystem enabled\n");

  /*
	 * Sx States:
	 * ----------
	 * Figure out which Sx states are supported
	 */
	for (i=0; i<=ACPI_S_STATES_MAX; i++) {
		AcpiStatus = acpi_hw_obtain_sleep_type_register_data(
			i,
			&TypeA,
			&TypeB);
    DPRINT("acpi_hw_obtain_sleep_type_register_data (%d) status 0x%X\n",
      i, AcpiStatus);
    if (ACPI_SUCCESS(AcpiStatus)) {
			DeviceExtension->SystemStates[i] = TRUE;
		}
	}

  ACPIPrintInfo(DeviceExtension);

  /* Initialize ACPI bus manager */
  AcpiStatus = bm_initialize();
  if (!ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("bm_initialize() failed with status 0x%X\n", AcpiStatus);
    acpi_terminate();
    return STATUS_UNSUCCESSFUL;
  }

  InitializeListHead(&DeviceExtension->DeviceListHead);
  KeInitializeSpinLock(&DeviceExtension->DeviceListLock);
  DeviceExtension->DeviceListCount = 0;

  ACPIEnumerateNamespace(DeviceExtension);

  //ACPIEnumerateRootBusses(DeviceExtension);

  ACPIInitializeInternalDrivers(DeviceExtension);

  DeviceExtension->State = dsStarted;

	return STATUS_SUCCESS;
}


NTSTATUS
FdoSetPower(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PFDO_DEVICE_EXTENSION DeviceExtension;
  ACPI_STATUS AcpiStatus;
  NTSTATUS Status;
  ULONG AcpiState;

  DPRINT("Called\n");

  DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (IrpSp->Parameters.Power.Type == SystemPowerState) {
    Status = STATUS_SUCCESS;
    switch (IrpSp->Parameters.Power.State.SystemState) {
    case PowerSystemSleeping1:
      AcpiState = ACPI_STATE_S1;
      break;
    case PowerSystemSleeping2:
      AcpiState = ACPI_STATE_S2;
      break;
    case PowerSystemSleeping3:
      AcpiState = ACPI_STATE_S3;
      break;
    case PowerSystemHibernate:
      AcpiState = ACPI_STATE_S4;
      break;
    case PowerSystemShutdown:
      AcpiState = ACPI_STATE_S5;
      break;
    default:
      Status = STATUS_UNSUCCESSFUL;
    }
    if (!DeviceExtension->SystemStates[AcpiState]) {
      DPRINT("System sleep state S%d is not supported by hardware\n", AcpiState);
      Status = STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(Status)) {
      DPRINT("Trying to enter sleep state %d\n", AcpiState);

      AcpiStatus = acpi_enter_sleep_state(AcpiState);
      if (!ACPI_SUCCESS(AcpiStatus)) {
        DPRINT("Failed to enter sleep state %d (Status 0x%X)\n",
          AcpiState, AcpiStatus);
        Status = STATUS_UNSUCCESSFUL;
      }
    }
  } else {
    Status = STATUS_UNSUCCESSFUL;
  }

  return Status;
}


/*** PUBLIC ******************************************************************/

NTSTATUS
STDCALL
FdoPnpControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
/*
 * FUNCTION: Handle Plug and Play IRPs for the ACPI device
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the ACPI driver
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  switch (IrpSp->MinorFunction) {
  case IRP_MN_CANCEL_REMOVE_DEVICE:
    Status = STATUS_NOT_IMPLEMENTED;
    break;

  case IRP_MN_CANCEL_STOP_DEVICE:
    Status = STATUS_NOT_IMPLEMENTED;
    break;

  case IRP_MN_DEVICE_USAGE_NOTIFICATION:
    Status = STATUS_NOT_IMPLEMENTED;
    break;

  case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
    Status = STATUS_NOT_IMPLEMENTED;
    break;

  case IRP_MN_QUERY_DEVICE_RELATIONS:
    Status = FdoQueryBusRelations(DeviceObject, Irp, IrpSp);
    break;

  case IRP_MN_QUERY_PNP_DEVICE_STATE:
    Status = STATUS_NOT_IMPLEMENTED;
    break;

  case IRP_MN_QUERY_REMOVE_DEVICE:
    Status = STATUS_NOT_IMPLEMENTED;
    break;

  case IRP_MN_QUERY_STOP_DEVICE:
    Status = STATUS_NOT_IMPLEMENTED;
    break;

  case IRP_MN_REMOVE_DEVICE:
    Status = STATUS_NOT_IMPLEMENTED;
    break;

  case IRP_MN_START_DEVICE:
    DPRINT("IRP_MN_START_DEVICE received\n");
    Status = FdoStartDevice(DeviceObject, Irp);
    break;

  case IRP_MN_STOP_DEVICE:
    /* Currently not supported */
    //bm_terminate();
    Status = STATUS_UNSUCCESSFUL;
    break;

  case IRP_MN_SURPRISE_REMOVAL:
    Status = STATUS_NOT_IMPLEMENTED;
    break;

  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    Status = STATUS_NOT_IMPLEMENTED;
    break;
  }

  if (Status != STATUS_PENDING) {
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}


NTSTATUS
STDCALL
FdoPowerControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
/*
 * FUNCTION: Handle power management IRPs for the ACPI device
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the ACPI driver
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MinorFunction) {
  case IRP_MN_SET_POWER:
    Status = FdoSetPower(DeviceObject, Irp, IrpSp);
    break;

  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    Status = STATUS_NOT_IMPLEMENTED;
    break;
  }

  if (Status != STATUS_PENDING) {
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}

/* EOF */
