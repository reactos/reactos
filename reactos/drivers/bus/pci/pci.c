/* $Id: pci.c,v 1.3 2003/04/26 07:06:55 hbirr Exp $
 *
 * PROJECT:         ReactOS PCI Bus driver
 * FILE:            pci.c
 * PURPOSE:         Driver entry
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      10-09-2001  CSH  Created
 */
#include <pci.h>

#define NDEBUG
#include <debug.h>


#ifdef  ALLOC_PRAGMA

// Make the initialization routines discardable, so that they 
// don't waste space

#pragma  alloc_text(init, DriverEntry)

#endif  /*  ALLOC_PRAGMA  */

/*** PUBLIC ******************************************************************/

PCI_BUS_TYPE PciBusConfigType = pbtUnknown;


/*** PRIVATE *****************************************************************/

static NTSTATUS
PciReadConfigUchar(UCHAR Bus,
		   UCHAR Slot,
		   UCHAR Offset,
		   PUCHAR Value)
{
   switch (PciBusConfigType)
     {
     case pbtType1:
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	*Value = READ_PORT_UCHAR((PUCHAR)0xCFC + (Offset & 3));
	return STATUS_SUCCESS;

     case pbtType2:
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	*Value = READ_PORT_UCHAR((PUCHAR)(IOADDR(Slot, Offset)));
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
PciReadConfigUshort(UCHAR Bus,
		    UCHAR Slot,
		    UCHAR Offset,
		    PUSHORT Value)
{
   if ((Offset & 1) != 0)
     {
	return STATUS_INVALID_PARAMETER;
     }

   switch (PciBusConfigType)
     {
     case pbtType1:
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	*Value = READ_PORT_USHORT((PUSHORT)0xCFC + (Offset & 2));
	return STATUS_SUCCESS;

     case pbtType2:
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	*Value = READ_PORT_USHORT((PUSHORT)(IOADDR(Slot, Offset)));
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
PciReadConfigUlong(UCHAR Bus,
		   UCHAR Slot,
		   UCHAR Offset,
		   PULONG Value)
{
   if ((Offset & 3) != 0)
     {
	return STATUS_INVALID_PARAMETER;
     }

   switch (PciBusConfigType)
     {
     case pbtType1:
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	*Value = READ_PORT_ULONG((PULONG)0xCFC);
	return STATUS_SUCCESS;

     case pbtType2:
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	*Value = READ_PORT_ULONG((PULONG)(IOADDR(Slot, Offset)));
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
PciWriteConfigUchar(UCHAR Bus,
		    UCHAR Slot,
		    UCHAR Offset,
		    UCHAR Value)
{
   switch (PciBusConfigType)
     {
     case pbtType1:
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	WRITE_PORT_UCHAR((PUCHAR)0xCFC + (Offset&3), Value);
	return STATUS_SUCCESS;

     case pbtType2:
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	WRITE_PORT_UCHAR((PUCHAR)(IOADDR(Slot,Offset)), Value);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
PciWriteConfigUshort(UCHAR Bus,
		     UCHAR Slot,
		     UCHAR Offset,
		     USHORT Value)
{
   if ((Offset & 1) != 0)
     {
	return STATUS_INVALID_PARAMETER;
     }

   switch (PciBusConfigType)
     {
     case pbtType1:
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	WRITE_PORT_USHORT((PUSHORT)0xCFC + (Offset & 2), Value);
	return STATUS_SUCCESS;

     case pbtType2:
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	WRITE_PORT_USHORT((PUSHORT)(IOADDR(Slot, Offset)), Value);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
PciWriteConfigUlong(UCHAR Bus,
		    UCHAR Slot,
		    UCHAR Offset,
		    ULONG Value)
{
   if ((Offset & 3) != 0)
     {
	return STATUS_INVALID_PARAMETER;
     }

   switch (PciBusConfigType)
     {
     case pbtType1:
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	WRITE_PORT_ULONG((PULONG)0xCFC, Value);
	return STATUS_SUCCESS;

     case pbtType2:
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	WRITE_PORT_ULONG((PULONG)(IOADDR(Slot, Offset)), Value);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


ULONG
PciGetBusData(ULONG BusNumber,
	       ULONG SlotNumber,
	       PVOID Buffer,
	       ULONG Offset,
	       ULONG Length)
{
   PVOID Ptr = Buffer;
   ULONG Address = Offset;
   ULONG Len = Length;
   ULONG Vendor;
   UCHAR HeaderType;

#if 0
   DPRINT("  BusNumber %lu\n", BusNumber);
   DPRINT("  SlotNumber %lu\n", SlotNumber);
   DPRINT("  Offset 0x%lx\n", Offset);
   DPRINT("  Length 0x%lx\n", Length);
#endif

   if ((Length == 0) || (PciBusConfigType == 0))
     return 0;

   /* 0E=PCI_HEADER_TYPE */
   PciReadConfigUchar(BusNumber,
		      SlotNumber & 0xF8,
		      0x0E,
		      &HeaderType);
   if (((HeaderType & 0x80) == 0) && ((SlotNumber & 0x07) != 0))
     return 0;

   PciReadConfigUlong(BusNumber,
		      SlotNumber,
		      0x00,
		      &Vendor);
   /* some broken boards return 0 if a slot is empty: */
   if (Vendor == 0xFFFFFFFF || Vendor == 0)
     return 0;

   if ((Address & 1) && (Len >= 1))
     {
	PciReadConfigUchar(BusNumber,
			   SlotNumber,
			   Address,
			   Ptr);
	Ptr = Ptr + 1;
	Address++;
	Len--;
     }

   if ((Address & 2) && (Len >= 2))
     {
	PciReadConfigUshort(BusNumber,
			    SlotNumber,
			    Address,
			    Ptr);
	Ptr = Ptr + 2;
	Address += 2;
	Len -= 2;
     }

   while (Len >= 4)
     {
	PciReadConfigUlong(BusNumber,
			   SlotNumber,
			   Address,
			   Ptr);
	Ptr = Ptr + 4;
	Address += 4;
	Len -= 4;
     }

   if (Len >= 2)
     {
	PciReadConfigUshort(BusNumber,
			    SlotNumber,
			    Address,
			    Ptr);
	Ptr = Ptr + 2;
	Address += 2;
	Len -= 2;
     }

   if (Len >= 1)
     {
	PciReadConfigUchar(BusNumber,
			   SlotNumber,
			   Address,
			   Ptr);
	Ptr = Ptr + 1;
	Address++;
	Len--;
     }

   return Length - Len;
}


ULONG
PciSetBusData(ULONG BusNumber,
	       ULONG SlotNumber,
	       PVOID Buffer,
	       ULONG Offset,
	       ULONG Length)
{
   PVOID Ptr = Buffer;
   ULONG Address = Offset;
   ULONG Len = Length;
   ULONG Vendor;
   UCHAR HeaderType;

#if 0
   DPRINT("  BusNumber %lu\n", BusNumber);
   DPRINT("  SlotNumber %lu\n", SlotNumber);
   DPRINT("  Offset 0x%lx\n", Offset);
   DPRINT("  Length 0x%lx\n", Length);
#endif

   if ((Length == 0) || (PciBusConfigType == 0))
     return 0;

   /* 0E=PCI_HEADER_TYPE */
   PciReadConfigUchar(BusNumber,
		      SlotNumber & 0xF8,
		      0x0E,
		      &HeaderType);
   if (((HeaderType & 0x80) == 0) && ((SlotNumber & 0x07) != 0))
     return 0;

   PciReadConfigUlong(BusNumber,
		      SlotNumber,
		      0x00,
		      &Vendor);
   /* some broken boards return 0 if a slot is empty: */
   if (Vendor == 0xFFFFFFFF || Vendor == 0)
     return 0;

   if ((Address & 1) && (Len >= 1))
     {
	PciWriteConfigUchar(BusNumber,
			    SlotNumber,
			    Address,
			    *(PUCHAR)Ptr);
	Ptr = Ptr + 1;
	Address++;
	Len--;
     }

   if ((Address & 2) && (Len >= 2))
     {
	PciWriteConfigUshort(BusNumber,
			     SlotNumber,
			     Address,
			     *(PUSHORT)Ptr);
	Ptr = Ptr + 2;
	Address += 2;
	Len -= 2;
     }

   while (Len >= 4)
     {
	PciWriteConfigUlong(BusNumber,
			    SlotNumber,
			    Address,
			    *(PULONG)Ptr);
	Ptr = Ptr + 4;
	Address += 4;
	Len -= 4;
     }

   if (Len >= 2)
     {
	PciWriteConfigUshort(BusNumber,
			     SlotNumber,
			     Address,
			     *(PUSHORT)Ptr);
	Ptr = Ptr + 2;
	Address += 2;
	Len -= 2;
     }

   if (Len >= 1)
     {
	PciWriteConfigUchar(BusNumber,
			    SlotNumber,
			    Address,
			    *(PUCHAR)Ptr);
	Ptr = Ptr + 1;
	Address++;
	Len--;
     }

   return Length - Len;
}


PCI_BUS_TYPE
PciGetBusConfigType(VOID)
{
   ULONG Value;

   DPRINT("Called\n");

   DPRINT("Checking configuration type 1:\n");
   WRITE_PORT_UCHAR((PUCHAR)0xCFB, 0x01);
   Value = READ_PORT_ULONG((PULONG)0xCF8);
   WRITE_PORT_ULONG((PULONG)0xCF8, 0x80000000);
   if (READ_PORT_ULONG((PULONG)0xCF8) == 0x80000000)
     {
	WRITE_PORT_ULONG((PULONG)0xCF8, Value);
	DPRINT("  Success!\n");
	return pbtType1;
     }
   WRITE_PORT_ULONG((PULONG)0xCF8, Value);
   DPRINT("  Unsuccessful!\n");

   DPRINT("Checking configuration type 2:\n");
   WRITE_PORT_UCHAR((PUCHAR)0xCFB, 0x00);
   WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0x00);
   WRITE_PORT_UCHAR((PUCHAR)0xCFA, 0x00);
   if (READ_PORT_UCHAR((PUCHAR)0xCF8) == 0x00 &&
       READ_PORT_UCHAR((PUCHAR)0xCFB) == 0x00)
     {
	DPRINT("  Success!\n");
	return pbtType2;
     }
   DPRINT("  Unsuccessful!\n");

   DPRINT("No pci bus found!\n");
   return pbtUnknown;
}


NTSTATUS
STDCALL
PciDispatchDeviceControl(
  IN PDEVICE_OBJECT DeviceObject, 
  IN PIRP Irp) 
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called. IRP is at (0x%X)\n", Irp);

  Irp->IoStatus.Information = 0;

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
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


NTSTATUS
STDCALL
PciPnpControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
/*
 * FUNCTION: Handle Plug and Play IRPs
 * ARGUMENTS:
 *     DeviceObject = Pointer to PDO or FDO
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
  PCOMMON_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DeviceExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  DPRINT("IsFDO %d\n", DeviceExtension->IsFDO);

  if (DeviceExtension->IsFDO) {
    Status = FdoPnpControl(DeviceObject, Irp);
  } else {
    Status = PdoPnpControl(DeviceObject, Irp);
  }

  return Status;
}


NTSTATUS
STDCALL
PciPowerControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
/*
 * FUNCTION: Handle power management IRPs
 * ARGUMENTS:
 *     DeviceObject = Pointer to PDO or FDO
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
  PCOMMON_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DeviceExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (DeviceExtension->IsFDO) {
    Status = FdoPowerControl(DeviceObject, Irp);
  } else {
    Status = PdoPowerControl(DeviceObject, Irp);
  }

  return Status;
}


NTSTATUS
STDCALL
PciAddDevice(
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT PhysicalDeviceObject)
{
  PFDO_DEVICE_EXTENSION DeviceExtension;
  PDEVICE_OBJECT Fdo;
  NTSTATUS Status;

  DPRINT("Called\n");

  Status = IoCreateDevice(DriverObject, sizeof(FDO_DEVICE_EXTENSION),
    NULL, FILE_DEVICE_BUS_EXTENDER, FILE_DEVICE_SECURE_OPEN, TRUE, &Fdo);
  if (!NT_SUCCESS(Status)) {
    DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
    return Status;
  }

  DeviceExtension = (PFDO_DEVICE_EXTENSION)Fdo->DeviceExtension;

  RtlZeroMemory(DeviceExtension, sizeof(FDO_DEVICE_EXTENSION));

  DeviceExtension->Common.IsFDO = TRUE;

  DeviceExtension->Ldo =
    IoAttachDeviceToDeviceStack(Fdo, PhysicalDeviceObject);

  DeviceExtension->State = dsStopped;

  Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

  //Fdo->Flags |= DO_POWER_PAGABLE;

  DPRINT("Done AddDevice\n");

  return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
DriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath)
{
  DbgPrint("Peripheral Component Interconnect Bus Driver\n");

  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = (PDRIVER_DISPATCH) PciDispatchDeviceControl;
  DriverObject->MajorFunction[IRP_MJ_PNP] = (PDRIVER_DISPATCH) PciPnpControl;
  DriverObject->MajorFunction[IRP_MJ_POWER] = (PDRIVER_DISPATCH) PciPowerControl;
  DriverObject->DriverExtension->AddDevice = PciAddDevice;

  return STATUS_SUCCESS;
}


BOOLEAN
PciCreateUnicodeString(
  PUNICODE_STRING	Destination,
  PWSTR Source,
  POOL_TYPE PoolType)
{
  ULONG Length;

  if (!Source)
  {
    RtlInitUnicodeString(Destination, NULL);
    return TRUE;
  }

  Length = (wcslen(Source) + 1) * sizeof(WCHAR);

  Destination->Buffer = ExAllocatePool(PoolType, Length);

  if (Destination->Buffer == NULL)
  {
    return FALSE;
  }

  RtlCopyMemory(Destination->Buffer, Source, Length);

  Destination->MaximumLength = Length;

  Destination->Length = Length - sizeof(WCHAR);

  return TRUE;
}

/* EOF */
