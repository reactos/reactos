/* $Id: acpisys.c,v 1.1 2001/05/01 23:00:03 chorns Exp $
 *
 * PROJECT:         ReactOS ACPI bus driver
 * FILE:            acpi/ospm/acpisys.c
 * PURPOSE:         Driver entry
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      01-05-2001  CSH  Created
 */
#include <acpisys.h>

#define NDEBUG
#include <debug.h>

#ifdef  ALLOC_PRAGMA

// Make the initialization routines discardable, so that they 
// don't waste space

#pragma  alloc_text(init, DriverEntry)

// Make the PASSIVE_LEVEL routines pageable, so that they don't
// waste nonpaged memory

#pragma  alloc_text(page, ACPIDispatchOpenClose)
#pragma  alloc_text(page, ACPIDispatchRead)
#pragma  alloc_text(page, ACPIDispatchWrite)

#endif  /*  ALLOC_PRAGMA  */


NTSTATUS
STDCALL
ACPIDispatchDeviceControl(
  IN PDEVICE_OBJECT DeviceObject, 
  IN PIRP Irp) 
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called. IRP is at (0x%X)\n", Irp);

  Irp->IoStatus.Information = 0;

  IrpSp  = IoGetCurrentIrpStackLocation(Irp);
  switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->Parameters.DeviceIoControl.IoControlCode);
    Status = STATUS_NOT_IMPLEMENTED;
    break;
  }

  if (Status != STATUS_PENDING) {
    Irp->IoStatus.Status = Status;

    DPRINT("Completing IRP at 0x%X\n", Irp);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}


VOID ACPIPrintInfo(
	PACPI_DEVICE_EXTENSION DeviceExtension)
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
ACPIStartDevice(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PACPI_DEVICE_EXTENSION DeviceExtension;
	ACPI_PHYSICAL_ADDRESS rsdp;
	ACPI_SYSTEM_INFO SysInfo;
  ACPI_STATUS AcpiStatus;
  ACPI_BUFFER	Buffer;
	UCHAR TypeA, TypeB;
  ULONG i;

  DPRINT("Called\n");

  DeviceExtension = (PACPI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  assert(DeviceExtension->State == dsStopped);

  AcpiStatus = acpi_initialize_subsystem();
  if (!ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("acpi_initialize_subsystem() failed with status 0x%X\n", AcpiStatus);
    return STATUS_NO_SUCH_DEVICE;
  }

  AcpiStatus = acpi_find_root_pointer(&rsdp);
  if (!ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("acpi_find_root_pointer() failed with status 0x%X\n", AcpiStatus);
    return STATUS_NO_SUCH_DEVICE;
	}

  /* From this point on, on error we must call acpi_terminate() */

  AcpiStatus = acpi_load_tables(rsdp);
	if (!ACPI_SUCCESS(AcpiStatus)) {
		DPRINT("acpi_load_tables() failed with status 0x%X\n", AcpiStatus);
		acpi_terminate();
		return STATUS_NO_SUCH_DEVICE;
	}

	Buffer.length  = sizeof(SysInfo);
	Buffer.pointer = &SysInfo;

  AcpiStatus = acpi_get_system_info(&Buffer);
	if (!ACPI_SUCCESS(AcpiStatus)) {
		DPRINT("acpi_get_system_info() failed with status 0x%X\n", AcpiStatus);
		acpi_terminate();
		return STATUS_NO_SUCH_DEVICE;
	}

	DPRINT("ACPI CA Core Subsystem version 0x%X\n", SysInfo.acpi_ca_version);

  AcpiStatus = acpi_enable_subsystem(ACPI_FULL_INITIALIZATION);
  if (!ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("acpi_enable_subsystem() failed with status 0x%X\n", AcpiStatus);
    acpi_terminate();
    return STATUS_NO_SUCH_DEVICE;
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

  DeviceExtension->State = dsStarted;

  ACPIPrintInfo(DeviceExtension);

  //ACPIEnumerateSystemDevices(DeviceExtension);

	return STATUS_SUCCESS;
}


NTSTATUS
ACPIQueryBusRelations(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PDEVICE_RELATIONS Relations;
  NTSTATUS Status;
  ULONG Size;

  DPRINT("Called\n");

  Size = sizeof(DEVICE_RELATIONS);
  Relations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, Size);
  if (!Relations)
    return STATUS_INSUFFICIENT_RESOURCES;

  /* FIXME: Create PDOs and return them to PnP manager */

  /* FIXME: For ACPI namespace devices create filter DOs and attach them
     just above */

  /* FIXME: For other devices in ACPI namespace, but not on motherboard,
     create PDOs */

  Relations->Count = 0;

  Irp->IoStatus.Information = (ULONG)Relations;

  return Status;
}


NTSTATUS
ACPIQueryDeviceRelations(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  NTSTATUS Status;

  DPRINT("Called\n");

  switch (IrpSp->Parameters.QueryDeviceRelations.Type) {
    case BusRelations:
      Status = ACPIQueryBusRelations(DeviceObject, Irp, IrpSp);
      break;

    default:
      Status = STATUS_NOT_IMPLEMENTED;
  }

  return Status;
}


NTSTATUS
ACPISetPower(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PACPI_DEVICE_EXTENSION DeviceExtension;
  ACPI_STATUS AcpiStatus;
  NTSTATUS Status;
  ULONG AcpiState;

  DPRINT("Called\n");

  DeviceExtension = (PACPI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

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


NTSTATUS
STDCALL
ACPIPnpControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  switch (IrpSp->MinorFunction) {
  case IRP_MN_QUERY_DEVICE_RELATIONS:
    Status = ACPIQueryDeviceRelations(DeviceObject, Irp, IrpSp);
    break;

  case IRP_MN_START_DEVICE:
    DPRINT("IRP_MN_START_DEVICE received\n");
    Status = ACPIStartDevice(DeviceObject, Irp);
    break;

  case IRP_MN_STOP_DEVICE:
    /* Currently not supported */
    Status = STATUS_UNSUCCESSFUL;
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
ACPIPowerControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MinorFunction) {
  case IRP_MN_SET_POWER:
    Status = ACPISetPower(DeviceObject, Irp, IrpSp);
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
ACPIAddDevice(
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT PhysicalDeviceObject)
{
  PACPI_DEVICE_EXTENSION DeviceExtension;
  PDEVICE_OBJECT Fdo;
  NTSTATUS Status;

  DPRINT("Called\n");

  Status = IoCreateDevice(DriverObject, sizeof(ACPI_DEVICE_EXTENSION),
    NULL, FILE_DEVICE_BUS_EXTENDER, FILE_DEVICE_SECURE_OPEN, TRUE, &Fdo);
  if (!NT_SUCCESS(Status)) {
    DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
    return Status;
  }

  DeviceExtension = (PACPI_DEVICE_EXTENSION)Fdo->DeviceExtension;

  DeviceExtension->Pdo = PhysicalDeviceObject;

  DeviceExtension->Ldo =
    IoAttachDeviceToDeviceStack(Fdo, PhysicalDeviceObject);

  DeviceExtension->State = dsStopped;

  Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

  DPRINT("Done AddDevice\n");

  return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
DriverEntry(
  IN PDRIVER_OBJECT DriverObject, 
  IN PUNICODE_STRING RegistryPath)
{
  DbgPrint("Advanced Configuration and Power Interface Bus Driver\n");

  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ACPIDispatchDeviceControl;
  DriverObject->MajorFunction[IRP_MJ_PNP] = ACPIPnpControl;
  DriverObject->MajorFunction[IRP_MJ_POWER] = ACPIPowerControl;
  DriverObject->DriverExtension->AddDevice = ACPIAddDevice;

  return STATUS_SUCCESS;
}

/* EOF */
