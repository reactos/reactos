/*
 *  FreeLoader
 *
 *  Copyright (C) 2004  Eric Kohl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

typedef struct _ROUTING_SLOT
{
  UCHAR  BusNumber;
  UCHAR  DeviceNumber;
  UCHAR  LinkA;
  USHORT BitmapA;
  UCHAR  LinkB;
  USHORT BitmapB;
  UCHAR  LinkC;
  USHORT BitmapC;
  UCHAR  LinkD;
  USHORT BitmapD;
  UCHAR  SlotNumber;
  UCHAR  Reserved;
} __attribute__((packed)) ROUTING_SLOT, *PROUTING_SLOT;

typedef struct _PCI_IRQ_ROUTING_TABLE
{
  ULONG Signature;
  USHORT Version;
  USHORT Size;
  UCHAR  RouterBus;
  UCHAR  RouterSlot;
  USHORT ExclusiveIRQs;
  ULONG CompatibleRouter;
  ULONG MiniportData;
  UCHAR  Reserved[11];
  UCHAR  Checksum;
  ROUTING_SLOT Slot[1];
} __attribute__((packed)) PCI_IRQ_ROUTING_TABLE, *PPCI_IRQ_ROUTING_TABLE;

typedef struct _CM_PCI_BUS_DATA
{
  UCHAR  VersionMinor;
  UCHAR  VersionMajor;
  UCHAR  BusCount;
  UCHAR  HardwareMechanism;
} __attribute__((packed)) CM_PCI_BUS_DATA, *PCM_PCI_BUS_DATA;

#if 0
static PPCI_IRQ_ROUTING_TABLE
GetPciIrqRoutingTable(VOID)
{
  PPCI_IRQ_ROUTING_TABLE Table;
  PUCHAR Ptr;
  ULONG Sum;
  ULONG i;

  Table = (PPCI_IRQ_ROUTING_TABLE)0xF0000;
  while ((ULONG)Table < 0x100000)
    {
      if (Table->Signature == 0x52495024)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "Found signature\n"));

	  Ptr = (PUCHAR)Table;
	  Sum = 0;
	  for (i = 0; i < Table->Size; i++)
	    {
	      Sum += Ptr[i];
	    }

	  if ((Sum & 0xFF) != 0)
	    {
	      DbgPrint((DPRINT_HWDETECT,
			"Invalid routing table\n"));
	      return NULL;
	    }

	  DbgPrint((DPRINT_HWDETECT,
		   "Valid checksum\n"));

	  return Table;
	}

      Table = (PPCI_IRQ_ROUTING_TABLE)((ULONG)Table + 0x10);
    }

  return NULL;
}
#endif

static BOOLEAN
FindPciBios(PCM_PCI_BUS_DATA BusData)
{
  REGS  RegsIn;
  REGS  RegsOut;

  RegsIn.b.ah = 0xB1; /* Subfunction B1h */
  RegsIn.b.al = 0x01; /* PCI BIOS present */

  Int386(0x1A, &RegsIn, &RegsOut);

  if (INT386_SUCCESS(RegsOut) && RegsOut.d.edx == 0x20494350 && RegsOut.b.ah == 0)
    {
      DbgPrint((DPRINT_HWDETECT, "Found PCI bios\n"));

      DbgPrint((DPRINT_HWDETECT, "AL: %x\n", RegsOut.b.al));
      DbgPrint((DPRINT_HWDETECT, "BH: %x\n", RegsOut.b.bh));
      DbgPrint((DPRINT_HWDETECT, "BL: %x\n", RegsOut.b.bl));
      DbgPrint((DPRINT_HWDETECT, "CL: %x\n", RegsOut.b.cl));

      BusData->BusCount = RegsOut.b.cl + 1;
      BusData->VersionMajor = RegsOut.b.bh;
      BusData->VersionMinor = RegsOut.b.bl;
      BusData->HardwareMechanism = RegsOut.b.al;

      return TRUE;
    }


  DbgPrint((DPRINT_HWDETECT, "No PCI bios found\n"));

  return FALSE;
}

#if 0
static VOID
DetectPciIrqRoutingTable(FRLDRHKEY BusKey)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  PPCI_IRQ_ROUTING_TABLE Table;
  FRLDRHKEY TableKey;
  ULONG Size;
  LONG Error;

  Table = GetPciIrqRoutingTable();
  if (Table != NULL)
    {
      DbgPrint((DPRINT_HWDETECT, "Table size: %u\n", Table->Size));

      Error = RegCreateKey(BusKey,
			   L"RealModeIrqRoutingTable\\0",
			   &TableKey);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
	  return;
	}

      /* Set 'Component Information' */
      SetComponentInformation(TableKey,
                              0x0,
                              0x0,
                              0xFFFFFFFF);

      /* Set 'Identifier' value */
      Error = RegSetValue(TableKey,
			  L"Identifier",
			  REG_SZ,
			  (PCHAR)L"PCI Real-mode IRQ Routing Table",
			  32 * sizeof(WCHAR));
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
	  return;
	}

      /* Set 'Configuration Data' value */
      Size = FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList.PartialDescriptors) +
	     2 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) + Table->Size;
      FullResourceDescriptor = MmAllocateMemory(Size);
      if (FullResourceDescriptor == NULL)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "Failed to allocate resource descriptor\n"));
	  return;
	}

      /* Initialize resource descriptor */
      memset(FullResourceDescriptor, 0, Size);
      FullResourceDescriptor->InterfaceType = Isa;
      FullResourceDescriptor->BusNumber = 0;
      FullResourceDescriptor->PartialResourceList.Version = 1;
      FullResourceDescriptor->PartialResourceList.Revision = 1;
      FullResourceDescriptor->PartialResourceList.Count = 2;

      PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[0];
      PartialDescriptor->Type = CmResourceTypeBusNumber;
      PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
      PartialDescriptor->u.BusNumber.Start = 0;
      PartialDescriptor->u.BusNumber.Length = 1;

      PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[1];
      PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
      PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
      PartialDescriptor->u.DeviceSpecificData.DataSize = Table->Size;

      memcpy(&FullResourceDescriptor->PartialResourceList.PartialDescriptors[2],
	     Table,
	     Table->Size);

      /* Set 'Configuration Data' value */
      Error = RegSetValue(TableKey,
			  L"Configuration Data",
			  REG_FULL_RESOURCE_DESCRIPTOR,
			  (PCHAR) FullResourceDescriptor,
			  Size);
      MmFreeMemory(FullResourceDescriptor);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "RegSetValue(Configuration Data) failed (Error %u)\n",
		    (int)Error));
	  return;
	}
    }
}
#endif

PCONFIGURATION_COMPONENT_DATA
DetectPciBios(ULONG *BusNumber,
              PCONFIGURATION_COMPONENT_DATA ComponentRoot,
              PCONFIGURATION_COMPONENT_DATA PreviousComponent,
              BOOLEAN NextChild)
{
	PCONFIGURATION_COMPONENT_DATA PciBusComponentData, PreviousBusData;
	PCONFIGURATION_COMPONENT PciBusComponent;
	PVOID Identifier;
	CM_PCI_BUS_DATA BusData;
	ULONG Size;
	ULONG i;
	BOOLEAN NextComponentChild;

	/* Return Previous component in case no PCI buses are detected */
	PreviousBusData = PreviousComponent;
	NextComponentChild = NextChild;

	/* Report the PCI BIOS */
	if (FindPciBios(&BusData))
	{
		/* Increment bus number */
		(*BusNumber)++;

		/* Detect PCI IRQ routing table and store it */
		//DetectPciIrqRoutingTable(BiosKey);

		/* Attach to the component we've got -- as child */
		PreviousBusData = PreviousComponent;
		NextComponentChild = NextChild;

		/* Report PCI buses */
		for (i = 0; i < (ULONG)BusData.BusCount; i++)
		{
			/* Create data component */
			PciBusComponentData = MmAllocateMemory(sizeof(CONFIGURATION_COMPONENT_DATA));
			RtlZeroMemory(PciBusComponentData, sizeof(CONFIGURATION_COMPONENT_DATA));

			/* Save a pointer to the ComponentEntry */
			PciBusComponent = &PciBusComponentData->ComponentEntry;

			/* Fill ComponentEntry */
			PciBusComponent->Class = AdapterClass;
			PciBusComponent->Type = MultiFunctionAdapter;
			PciBusComponent->AffinityMask = 0xFFFFFFFF;
			Identifier = MmAllocateMemory(sizeof("PCI")+1);
			sprintf(Identifier, "PCI");
			PciBusComponent->Identifier = PaToVa(Identifier);
			PciBusComponent->IdentifierLength = strlen(Identifier)+1;

			/* We have to treat first PCI bus's data differently */
			if (i == 0)
			{
				PCM_PARTIAL_RESOURCE_LIST ResourceList;
				PVOID DeviceSpecific;

				Size = sizeof(CM_PARTIAL_RESOURCE_LIST) + sizeof(CM_PCI_BUS_DATA);
				PciBusComponent->ConfigurationDataLength = Size;
				PciBusComponentData->ConfigurationData = MmAllocateMemory(Size);
				RtlZeroMemory(PciBusComponentData->ConfigurationData, Size);

				ResourceList = (PCM_PARTIAL_RESOURCE_LIST)PciBusComponentData->ConfigurationData;
				ResourceList->Count = 1;
				ResourceList->PartialDescriptors[0].Type = CmResourceTypeDeviceSpecific;
				ResourceList->PartialDescriptors[0].u.DeviceSpecificData.DataSize = sizeof(CM_PCI_BUS_DATA);

				/* Fill actual data */
				DeviceSpecific = (PVOID)((ULONG_PTR)PciBusComponentData->ConfigurationData + sizeof(CM_PARTIAL_RESOURCE_LIST));
				RtlCopyMemory(DeviceSpecific, (PVOID)&BusData, sizeof(CM_PCI_BUS_DATA));
			}

			if (NextComponentChild)
			{
				PreviousBusData->Child = PciBusComponentData;
				PciBusComponentData->Parent = PreviousBusData;

				NextComponentChild = FALSE;
			}
			else
			{
				PreviousBusData->Sibling = PciBusComponentData;
				PciBusComponentData->Parent = PreviousBusData->Parent;
			}

			PreviousBusData = PciBusComponentData;

			/* Increment bus number */
			(*BusNumber)++;
		}

	}

	return PreviousBusData;
}

/* EOF */
