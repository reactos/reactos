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
  UCHAR  BusCount;
  USHORT PciVersion;
  UCHAR  HardwareMechanism;
} __attribute__((packed)) CM_PCI_BUS_DATA, *PCM_PCI_BUS_DATA;


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
      BusData->PciVersion = RegsOut.w.bx;
      BusData->HardwareMechanism = RegsOut.b.cl;

      return TRUE;
    }


  DbgPrint((DPRINT_HWDETECT, "No PCI bios found\n"));

  return FALSE;
}


static VOID
DetectPciIrqRoutingTable(PCONFIGURATION_COMPONENT_DATA BusKey)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  PPCI_IRQ_ROUTING_TABLE Table;
  PCONFIGURATION_COMPONENT_DATA TableKey;
  ULONG Size;

  Table = GetPciIrqRoutingTable();
  if (Table != NULL)
    {
      DbgPrint((DPRINT_HWDETECT, "Table size: %u\n", Table->Size));

      FldrCreateComponentKey(BusKey,
                             L"RealModeIrqRoutingTable",
                             0,
                             SystemClass,
                             RealModeIrqRoutingTable,
                             &TableKey);

      /* Set 'Component Information' */
      FldrSetComponentInformation(TableKey,
                                  0x0,
                                  0x0,
                                  0xFFFFFFFF);

      /* Set 'Identifier' value */
      FldrSetIdentifier(TableKey, L"PCI Real-mode IRQ Routing Table");

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
      FldrSetConfigurationData(TableKey, FullResourceDescriptor, Size);
      MmFreeMemory(FullResourceDescriptor);
    }
}


VOID
DetectPciBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  CM_PCI_BUS_DATA BusData;
  PCONFIGURATION_COMPONENT_DATA BiosKey;
  ULONG Size;
#if 0
  PCONFIGURATION_COMPONENT_DATA BusKey;
  ULONG i;
  WCHAR szPci[] = L"PCI";
#endif

  /* Report the PCI BIOS */
  if (FindPciBios(&BusData))
    {
      /* Create new bus key */
      FldrCreateComponentKey(SystemKey,
                             L"MultifunctionAdapter",
                             *BusNumber,
                             AdapterClass,
                             MultiFunctionAdapter,
                             &BiosKey);

      /* Set 'Component Information' */
      FldrSetComponentInformation(BiosKey,
                                  0x0,
                                  0x0,
                                  0xFFFFFFFF);

      /* Increment bus number */
      (*BusNumber)++;

      /* Set 'Identifier' value */
      FldrSetIdentifier(BiosKey, L"PCI BIOS");

      /* Set 'Configuration Data' value */
      Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR);
      FullResourceDescriptor = MmAllocateMemory(Size);
      if (FullResourceDescriptor == NULL)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "Failed to allocate resource descriptor\n"));
	  return;
	}

      /* Initialize resource descriptor */
      memset(FullResourceDescriptor, 0, Size);
      FullResourceDescriptor->InterfaceType = PCIBus;
      FullResourceDescriptor->BusNumber = 0;
      FullResourceDescriptor->PartialResourceList.Version = 1;
      FullResourceDescriptor->PartialResourceList.Revision = 1;
      FullResourceDescriptor->PartialResourceList.Count = 1;
      FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].Type = CmResourceTypeBusNumber;
      FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].ShareDisposition = CmResourceShareDeviceExclusive;
      FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].u.BusNumber.Start = 0;
      FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].u.BusNumber.Length = 1;

      /* Set 'Configuration Data' value */
      FldrSetConfigurationData(BiosKey, FullResourceDescriptor, Size);
      MmFreeMemory(FullResourceDescriptor);

      DetectPciIrqRoutingTable(BiosKey);

#if 0
      /*
       * FIXME:
       * Enabling this piece of code will corrupt the boot sequence!
       * This is probably caused by a bug in the registry code!
       */

      /* Report PCI buses */
      for (i = 0; i < (ULONG)BusData.BusCount; i++)
      {
          FldrCreateComponentKey(SystemKey,
                                 L"MultifunctionAdapter",
                                 *BusNumber,
                                 AdapterClass,
                                 MultiFunctionAdapter,
                                 &BiosKey);
          
          /* Set 'Component Information' */
          FldrSetComponentInformation(BusKey,
                                      0x0,
                                      0x0,
                                      0xFFFFFFFF);
          
          /* Increment bus number */
          (*BusNumber)++;
          
          /* Set 'Identifier' value */
          FldrSetIdentifier(BiosKey, szPci);
      }
#endif
    }
}

/* EOF */
