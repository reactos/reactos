/* $Id: pci.c,v 1.8 2003/04/06 10:45:15 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/pci.c
 * PURPOSE:         Interfaces to the PCI bus
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 *                  Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  05/06/1998: Created
 *                  17/08/2000: Added preliminary pci bus scanner
 *                  13/06/2001: Implemented access to pci configuration space
 */

/*
 * NOTES: Sections copied from the Linux pci support
 */

/* INCLUDES *****************************************************************/

#include <roscfg.h>
#include <ddk/ntddk.h>
#include <bus.h>
#ifdef MP
#include <mps.h>
#endif


#define NDEBUG
#include <internal/debug.h>


/* MACROS ******************************************************************/

/* FIXME These are also defined in drivers/bus/pci/pcidef.h.
   Maybe put PCI definitions in a central include file??? */

/* access type 1 macros */
#define CONFIG_CMD(bus, dev_fn, where) \
	(0x80000000 | (((ULONG)(bus)) << 16) | (((dev_fn) & 0x1F) << 11) | (((dev_fn) & 0xE0) << 3) | ((where) & ~3))

/* access type 2 macros */
#define IOADDR(dev_fn, where) \
	(0xC000 | (((dev_fn) & 0x1F) << 8) | (where))
#define FUNC(dev_fn) \
	((((dev_fn) & 0xE0) >> 4) | 0xf0)

#define  PCI_BASE_ADDRESS_SPACE	0x01	/* 0 = memory, 1 = I/O */
#define  PCI_BASE_ADDRESS_SPACE_IO 0x01
#define  PCI_BASE_ADDRESS_SPACE_MEMORY 0x00
#define  PCI_BASE_ADDRESS_MEM_TYPE_MASK 0x06
#define  PCI_BASE_ADDRESS_MEM_TYPE_32	0x00	/* 32 bit address */
#define  PCI_BASE_ADDRESS_MEM_TYPE_1M	0x02	/* Below 1M [obsolete] */
#define  PCI_BASE_ADDRESS_MEM_TYPE_64	0x04	/* 64 bit address */
#define  PCI_BASE_ADDRESS_MEM_PREFETCH	0x08	/* prefetchable? */
#define  PCI_BASE_ADDRESS_MEM_MASK	(~0x0fUL)
#define  PCI_BASE_ADDRESS_IO_MASK	(~0x03UL)
/* bit 1 is reserved if address_space = 1 */


/* GLOBALS ******************************************************************/

#define TAG_PCI  TAG('P', 'C', 'I', 'H')

static ULONG BusConfigType = 0;  /* undetermined config type */
static KSPIN_LOCK PciLock;

/* FUNCTIONS ****************************************************************/

static NTSTATUS
ReadPciConfigUchar(UCHAR Bus,
		   UCHAR Slot,
		   UCHAR Offset,
		   PUCHAR Value)
{
   KIRQL oldIrql;

   switch (BusConfigType)
     {
     case 1:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	*Value = READ_PORT_UCHAR((PUCHAR)0xCFC + (Offset & 3));
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;

     case 2:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	*Value = READ_PORT_UCHAR((PUCHAR)(IOADDR(Slot, Offset)));
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
ReadPciConfigUshort(UCHAR Bus,
		    UCHAR Slot,
		    UCHAR Offset,
		    PUSHORT Value)
{
   KIRQL oldIrql;

   if ((Offset & 1) != 0)
     {
	return STATUS_INVALID_PARAMETER;
     }

   switch (BusConfigType)
     {
     case 1:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	*Value = READ_PORT_USHORT((PUSHORT)0xCFC + (Offset & 1));
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;

     case 2:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	*Value = READ_PORT_USHORT((PUSHORT)(IOADDR(Slot, Offset)));
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
ReadPciConfigUlong(UCHAR Bus,
		   UCHAR Slot,
		   UCHAR Offset,
		   PULONG Value)
{
   KIRQL oldIrql;

   if ((Offset & 3) != 0)
     {
	return STATUS_INVALID_PARAMETER;
     }

   switch (BusConfigType)
     {
     case 1:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	*Value = READ_PORT_ULONG((PULONG)0xCFC);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;

     case 2:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	*Value = READ_PORT_ULONG((PULONG)(IOADDR(Slot, Offset)));
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
WritePciConfigUchar(UCHAR Bus,
		    UCHAR Slot,
		    UCHAR Offset,
		    UCHAR Value)
{
   KIRQL oldIrql;

   switch (BusConfigType)
     {
     case 1:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	WRITE_PORT_UCHAR((PUCHAR)0xCFC + (Offset&3), Value);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;

     case 2:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	WRITE_PORT_UCHAR((PUCHAR)(IOADDR(Slot,Offset)), Value);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
WritePciConfigUshort(UCHAR Bus,
		     UCHAR Slot,
		     UCHAR Offset,
		     USHORT Value)
{
   KIRQL oldIrql;

   if ((Offset & 1) != 0)
     {
	return  STATUS_INVALID_PARAMETER;
     }

   switch (BusConfigType)
     {
     case 1:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	WRITE_PORT_USHORT((PUSHORT)0xCFC + (Offset & 1), Value);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;

     case 2:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	WRITE_PORT_USHORT((PUSHORT)(IOADDR(Slot, Offset)), Value);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
WritePciConfigUlong(UCHAR Bus,
		    UCHAR Slot,
		    UCHAR Offset,
		    ULONG Value)
{
   KIRQL oldIrql;

   if ((Offset & 3) != 0)
     {
	return  STATUS_INVALID_PARAMETER;
     }

   switch (BusConfigType)
     {
     case 1:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	WRITE_PORT_ULONG((PULONG)0xCFC, Value);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;

     case 2:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	WRITE_PORT_ULONG((PULONG)(IOADDR(Slot, Offset)), Value);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static ULONG STDCALL
HalpGetPciData(PBUS_HANDLER BusHandler,
	       ULONG BusNumber,
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

   DPRINT("HalpGetPciData() called.\n");
   DPRINT("  BusNumber %lu\n", BusNumber);
   DPRINT("  SlotNumber %lu\n", SlotNumber);
   DPRINT("  Offset 0x%lx\n", Offset);
   DPRINT("  Length 0x%lx\n", Length);

   if ((Length == 0) || (BusConfigType == 0))
     return 0;

   ReadPciConfigUlong(BusNumber,
		      SlotNumber & 0x1F,
		      0x00,
		      &Vendor);
   /* some broken boards return 0 if a slot is empty: */
   if (Vendor == 0xFFFFFFFF || Vendor == 0)
   {
     if (BusNumber == 0 && Offset == 0 && Length >= 2)
     {
	*(PUSHORT)Buffer = PCI_INVALID_VENDORID;
	return 2;
     }
     return 0;
   }

   /* 0E=PCI_HEADER_TYPE */
   ReadPciConfigUchar(BusNumber,
		      SlotNumber & 0x1F,
		      0x0E,
		      &HeaderType);
   if (((HeaderType & PCI_MULTIFUNCTION) == 0) && ((SlotNumber & 0xE0) != 0))
   {
     if (Offset == 0 && Length >= 2)
     {
	*(PUSHORT)Buffer = PCI_INVALID_VENDORID;
	return 2;
     }
     return 0;
   }
   ReadPciConfigUlong(BusNumber,
		      SlotNumber,
		      0x00,
		      &Vendor);
   /* some broken boards return 0 if a slot is empty: */
   if (Vendor == 0xFFFFFFFF || Vendor == 0)
   {
     if (BusNumber == 0 && Offset == 0 && Length >= 2)
     {
	*(PUSHORT)Buffer = PCI_INVALID_VENDORID;
	return 2;
     }
     return 0;
   }

   if ((Address & 1) && (Len >= 1))
     {
	ReadPciConfigUchar(BusNumber,
			   SlotNumber,
			   Address,
			   Ptr);
	Ptr = Ptr + 1;
	Address++;
	Len--;
     }

   if ((Address & 2) && (Len >= 2))
     {
	ReadPciConfigUshort(BusNumber,
			    SlotNumber,
			    Address,
			    Ptr);
	Ptr = Ptr + 2;
	Address += 2;
	Len -= 2;
     }

   while (Len >= 4)
     {
	ReadPciConfigUlong(BusNumber,
			   SlotNumber,
			   Address,
			   Ptr);
	Ptr = Ptr + 4;
	Address += 4;
	Len -= 4;
     }

   if (Len >= 2)
     {
	ReadPciConfigUshort(BusNumber,
			    SlotNumber,
			    Address,
			    Ptr);
	Ptr = Ptr + 2;
	Address += 2;
	Len -= 2;
     }

   if (Len >= 1)
     {
	ReadPciConfigUchar(BusNumber,
			   SlotNumber,
			   Address,
			   Ptr);
	Ptr = Ptr + 1;
	Address++;
	Len--;
     }

   return Length - Len;
}


static ULONG STDCALL
HalpSetPciData(PBUS_HANDLER BusHandler,
	       ULONG BusNumber,
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

   DPRINT("HalpSetPciData() called.\n");
   DPRINT("  BusNumber %lu\n", BusNumber);
   DPRINT("  SlotNumber %lu\n", SlotNumber);
   DPRINT("  Offset 0x%lx\n", Offset);
   DPRINT("  Length 0x%lx\n", Length);

   if ((Length == 0) || (BusConfigType == 0))
     return 0;

   ReadPciConfigUlong(BusNumber,
		      SlotNumber & 0x1F,
		      0x00,
		      &Vendor);
   /* some broken boards return 0 if a slot is empty: */
   if (Vendor == 0xFFFFFFFF || Vendor == 0)
     return 0;


   /* 0E=PCI_HEADER_TYPE */
   ReadPciConfigUchar(BusNumber,
		      SlotNumber & 0x1F,
		      0x0E,
		      &HeaderType);
   if (((HeaderType & PCI_MULTIFUNCTION) == 0) && ((SlotNumber & 0xE0) != 0))
     return 0;

   ReadPciConfigUlong(BusNumber,
		      SlotNumber,
		      0x00,
		      &Vendor);
   /* some broken boards return 0 if a slot is empty: */
   if (Vendor == 0xFFFFFFFF || Vendor == 0)
     return 0;

   if ((Address & 1) && (Len >= 1))
     {
	WritePciConfigUchar(BusNumber,
			    SlotNumber,
			    Address,
			    *(PUCHAR)Ptr);
	Ptr = Ptr + 1;
	Address++;
	Len--;
     }

   if ((Address & 2) && (Len >= 2))
     {
	WritePciConfigUshort(BusNumber,
			     SlotNumber,
			     Address,
			     *(PUSHORT)Ptr);
	Ptr = Ptr + 2;
	Address += 2;
	Len -= 2;
     }

   while (Len >= 4)
     {
	WritePciConfigUlong(BusNumber,
			    SlotNumber,
			    Address,
			    *(PULONG)Ptr);
	Ptr = Ptr + 4;
	Address += 4;
	Len -= 4;
     }

   if (Len >= 2)
     {
	WritePciConfigUshort(BusNumber,
			     SlotNumber,
			     Address,
			     *(PUSHORT)Ptr);
	Ptr = Ptr + 2;
	Address += 2;
	Len -= 2;
     }

   if (Len >= 1)
     {
	WritePciConfigUchar(BusNumber,
			    SlotNumber,
			    Address,
			    *(PUCHAR)Ptr);
	Ptr = Ptr + 1;
	Address++;
	Len--;
     }

   return Length - Len;
}


static ULONG
GetBusConfigType(VOID)
{
   ULONG Value;
   KIRQL oldIrql;

   DPRINT("GetBusConfigType() called\n");

   KeAcquireSpinLock(&PciLock, &oldIrql);
 
   DPRINT("Checking configuration type 1:");
   WRITE_PORT_UCHAR((PUCHAR)0xCFB, 0x01);
   Value = READ_PORT_ULONG((PULONG)0xCF8);
   WRITE_PORT_ULONG((PULONG)0xCF8, 0x80000000);
   if (READ_PORT_ULONG((PULONG)0xCF8) == 0x80000000)
     {
	WRITE_PORT_ULONG((PULONG)0xCF8, Value);
	KeReleaseSpinLock(&PciLock, oldIrql);
	DPRINT("  Success!\n");
	return 1;
     }
   WRITE_PORT_ULONG((PULONG)0xCF8, Value);
   DPRINT("  Unsuccessful!\n");

   DPRINT("Checking configuration type 2:");
   WRITE_PORT_UCHAR((PUCHAR)0xCFB, 0x00);
   WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0x00);
   WRITE_PORT_UCHAR((PUCHAR)0xCFA, 0x00);
   if (READ_PORT_UCHAR((PUCHAR)0xCF8) == 0x00 &&
       READ_PORT_UCHAR((PUCHAR)0xCFB) == 0x00)
     {
	KeReleaseSpinLock(&PciLock, oldIrql);
	DPRINT("  Success!\n");
	return 2;
     }
   KeReleaseSpinLock(&PciLock, oldIrql);
   DPRINT("  Unsuccessful!\n");

   DPRINT("No pci bus found!\n");
   return 0;
}


static ULONG STDCALL
HalpGetPciInterruptVector(PVOID BusHandler,
			  ULONG BusNumber,
			  ULONG BusInterruptLevel,
			  ULONG BusInterruptVector,
			  PKIRQL Irql,
			  PKAFFINITY Affinity)
{
#ifdef MP
  *Irql = PROFILE_LEVEL - BusInterruptVector;
  *Affinity = 0xFFFFFFFF;
  return IRQ2VECTOR(BusInterruptVector);
#else
  *Irql = PROFILE_LEVEL - BusInterruptVector;
  *Affinity = 0xFFFFFFFF;
  return BusInterruptVector;
#endif
}

static BOOLEAN STDCALL
HalpTranslatePciAddress(PBUS_HANDLER BusHandler,
			ULONG BusNumber,
			PHYSICAL_ADDRESS BusAddress,
			PULONG AddressSpace,
			PPHYSICAL_ADDRESS TranslatedAddress)
{
   if (*AddressSpace == 0)
     {
	/* memory space */

     }
   else if (*AddressSpace == 1)
     {
	/* io space */

     }
   else
     {
	/* other */
	return FALSE;
     }

   TranslatedAddress->QuadPart = BusAddress.QuadPart;

   return TRUE;
}

/*
 * Find the extent of a PCI decode..
 */
static ULONG STDCALL
PciSize(ULONG Base, ULONG Mask)
{
  ULONG Size = Mask & Base;   /* Find the significant bits */
  Size = Size & ~(Size - 1);  /* Get the lowest of them to find the decode size */
  return Size;
}

static NTSTATUS STDCALL
HalpAssignPciSlotResources(IN PBUS_HANDLER BusHandler,
			   IN ULONG BusNumber,
			   IN PUNICODE_STRING RegistryPath,
			   IN PUNICODE_STRING DriverClassName,
			   IN PDRIVER_OBJECT DriverObject,
			   IN PDEVICE_OBJECT DeviceObject,
			   IN ULONG SlotNumber,
			   IN OUT PCM_RESOURCE_LIST *AllocatedResources)
{
  UINT Address;
  UINT NoAddresses;
  ULONG BaseAddresses[PCI_TYPE0_ADDRESSES];
  ULONG Size[PCI_TYPE0_ADDRESSES];
  NTSTATUS Status = STATUS_SUCCESS;
  UCHAR Offset;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

  /* FIXME: Should handle 64-bit addresses */

  /* Read the PCI configuration space for the device and store base address and
     size information in temporary storage. Count the number of valid base addresses */
  NoAddresses = 0;
  for (Address = 0; Address < PCI_TYPE0_ADDRESSES; Address++)
    {
      Offset = offsetof(PCI_COMMON_CONFIG, u.type0.BaseAddresses[Address]);
      Status = ReadPciConfigUlong(BusNumber, SlotNumber,
                                  Offset, BaseAddresses + Address);
      if (! NT_SUCCESS(Status))
	{
	  return Status;
	}
      if (0xffffffff == BaseAddresses[Address])
	{
	  BaseAddresses[Address] = 0;
	}
      if (0 != BaseAddresses[Address])
	{
	  NoAddresses++;
	  Status = WritePciConfigUlong(BusNumber, SlotNumber, Offset, 0xffffffff);
	  if (! NT_SUCCESS(Status))
	    {
	      WritePciConfigUlong(BusNumber, SlotNumber, Offset, BaseAddresses[Address]);
	      return Status;
	    }
	  Status = ReadPciConfigUlong(BusNumber, SlotNumber,
	                              Offset, Size + Address);
	  if (! NT_SUCCESS(Status))
	    {
	      WritePciConfigUlong(BusNumber, SlotNumber, Offset, BaseAddresses[Address]);
	      return Status;
	    }
	  Status = WritePciConfigUlong(BusNumber, SlotNumber, Offset, BaseAddresses[Address]);
	  if (! NT_SUCCESS(Status))
	    {
	      return Status;
	    }

	}
    }

  /* Allocate output buffer and initialize */
  *AllocatedResources = ExAllocatePoolWithTag(PagedPool,
                                              sizeof(CM_RESOURCE_LIST) +
                                              (NoAddresses - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR),
                                              TAG_PCI);
  if (NULL == *AllocatedResources)
    {
    return STATUS_NO_MEMORY;
    }
  (*AllocatedResources)->Count = 1;
  (*AllocatedResources)->List[0].InterfaceType = PCIBus;
  (*AllocatedResources)->List[0].BusNumber = BusNumber;
  (*AllocatedResources)->List[0].PartialResourceList.Version = 1;
  (*AllocatedResources)->List[0].PartialResourceList.Revision = 1;
  (*AllocatedResources)->List[0].PartialResourceList.Count = NoAddresses;
  Descriptor = (*AllocatedResources)->List[0].PartialResourceList.PartialDescriptors;

  /* Store configuration information */
  for (Address = 0; Address < PCI_TYPE0_ADDRESSES; Address++)
    {
      if (0 != BaseAddresses[Address])
	{
	  if (PCI_BASE_ADDRESS_SPACE_MEMORY ==
              (BaseAddresses[Address] & PCI_BASE_ADDRESS_SPACE))
	    {
	      Descriptor->Type = CmResourceTypeMemory;
	      Descriptor->ShareDisposition = CmResourceShareDeviceExclusive; /* FIXME I have no idea... */
	      Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;             /* FIXME Just a guess */
	      Descriptor->u.Memory.Start.QuadPart = (BaseAddresses[Address] & PCI_BASE_ADDRESS_MEM_MASK);
	      Descriptor->u.Memory.Length = PciSize(Size[Address], PCI_BASE_ADDRESS_MEM_MASK);
	    }
	  else if (PCI_BASE_ADDRESS_SPACE_IO ==
                   (BaseAddresses[Address] & PCI_BASE_ADDRESS_SPACE))
	    {
	      Descriptor->Type = CmResourceTypePort;
	      Descriptor->ShareDisposition = CmResourceShareDeviceExclusive; /* FIXME I have no idea... */
	      Descriptor->Flags = CM_RESOURCE_PORT_IO;                       /* FIXME Just a guess */
	      Descriptor->u.Port.Start.QuadPart = BaseAddresses[Address] &= PCI_BASE_ADDRESS_IO_MASK;
	      Descriptor->u.Port.Length = PciSize(Size[Address], PCI_BASE_ADDRESS_IO_MASK & 0xffff);
	    }
	  else
	    {
	      assert(FALSE);
	      return STATUS_UNSUCCESSFUL;
	    }
	  Descriptor++;
	}
    }

  assert(Descriptor == (*AllocatedResources)->List[0].PartialResourceList.PartialDescriptors + NoAddresses);

  /* FIXME: Should store the resources in the registry resource map */

  return Status;
}


VOID
HalpInitPciBus(VOID)
{
  PBUS_HANDLER BusHandler;

  DPRINT("HalpInitPciBus() called.\n");

  KeInitializeSpinLock (&PciLock);

  BusConfigType = GetBusConfigType();
  if (BusConfigType == 0)
    return;

  DPRINT("Bus configuration %lu used\n", BusConfigType);

  /* pci bus (bus 0) handler */
  BusHandler = HalpAllocateBusHandler(PCIBus,
				      PCIConfiguration,
				      0);
  BusHandler->GetBusData = (pGetSetBusData)HalpGetPciData;
  BusHandler->SetBusData = (pGetSetBusData)HalpSetPciData;
  BusHandler->GetInterruptVector =
    (pGetInterruptVector)HalpGetPciInterruptVector;
  BusHandler->TranslateBusAddress = 
    (pTranslateBusAddress)HalpTranslatePciAddress;
//	BusHandler->AdjustResourceList =
//		(pGetSetBusData)HalpAdjustPciResourceList;
  BusHandler->AssignSlotResources =
    (pAssignSlotResources)HalpAssignPciSlotResources;


  /* agp bus (bus 1) handler */
  BusHandler = HalpAllocateBusHandler(PCIBus,
				      PCIConfiguration,
				      1);
  BusHandler->GetBusData = (pGetSetBusData)HalpGetPciData;
  BusHandler->SetBusData = (pGetSetBusData)HalpSetPciData;
  BusHandler->GetInterruptVector =
    (pGetInterruptVector)HalpGetPciInterruptVector;
  BusHandler->TranslateBusAddress = 
    (pTranslateBusAddress)HalpTranslatePciAddress;
//	BusHandler->AdjustResourceList =
//		(pGetSetBusData)HalpAdjustPciResourceList;
  BusHandler->AssignSlotResources =
    (pAssignSlotResources)HalpAssignPciSlotResources;

  DPRINT("HalpInitPciBus() finished.\n");
}

/* EOF */
