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
#include <arch.h>
#include <rtl.h>
#include <debug.h>
#include <mm.h>
#include <portio.h>

#include "../../reactos/registry.h"
#include "hardware.h"

typedef struct _ROUTING_SLOT
{
  U8  BusNumber;
  U8  DeviceNumber;
  U8  LinkA;
  U16 BitmapA;
  U8  LinkB;
  U16 BitmapB;
  U8  LinkC;
  U16 BitmapC;
  U8  LinkD;
  U16 BitmapD;
  U8  SlotNumber;
  U8  Reserved;
} __attribute__((packed)) ROUTING_SLOT, *PROUTING_SLOT;

typedef struct _PCI_IRQ_ROUTING_TABLE
{
  U32 Signature;
  U16 Version;
  U16 Size;
  U8  RouterBus;
  U8  RouterSlot;
  U16 ExclusiveIRQs;
  U32 CompatibleRouter;
  U32 MiniportData;
  U8  Reserved[11];
  U8  Checksum;
  ROUTING_SLOT Slot[1];
} __attribute__((packed)) PCI_IRQ_ROUTING_TABLE, *PPCI_IRQ_ROUTING_TABLE;

typedef struct _CM_PCI_BUS_DATA
{
  U8  BusCount;
  U16 PciVersion;
  U8  HardwareMechanism;
} __attribute__((packed)) CM_PCI_BUS_DATA, *PCM_PCI_BUS_DATA;


static PPCI_IRQ_ROUTING_TABLE
GetPciIrqRoutingTable(VOID)
{
  PPCI_IRQ_ROUTING_TABLE Table;
  PU8 Ptr;
  U32 Sum;
  U32 i;

  Table = (PPCI_IRQ_ROUTING_TABLE)0xF0000;
  while ((U32)Table < 0x100000)
    {
      if (Table->Signature == 0x52495024)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "Found signature\n"));

	  Ptr = (PU8)Table;
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

      Table = (PPCI_IRQ_ROUTING_TABLE)((U32)Table + 0x10);
    }

  return NULL;
}


static BOOL
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
DetectPciIrqRoutingTable(HKEY BusKey)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  PPCI_IRQ_ROUTING_TABLE Table;
  HKEY TableKey;
  U32 Size;
  S32 Error;

  Table = GetPciIrqRoutingTable();
  if (Table != NULL)
    {
      DbgPrint((DPRINT_HWDETECT, "Table size: %u\n", Table->Size));

      Error = RegCreateKey(BusKey,
			   "RealModeIrqRoutingTable\\0",
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
			  "Identifier",
			  REG_SZ,
			  (PU8)"PCI Real-mode IRQ Routing Table",
			  32);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
	  return;
	}

      /* Set 'Configuration Data' value */
      Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
	     Table->Size;
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
      FullResourceDescriptor->PartialResourceList.Count = 1;

      PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[0];
      PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
      PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
      PartialDescriptor->u.DeviceSpecificData.DataSize = Table->Size;

      memcpy(((PVOID)FullResourceDescriptor) + sizeof(CM_FULL_RESOURCE_DESCRIPTOR),
	     Table,
	     Table->Size);

      /* Set 'Configuration Data' value */
      Error = RegSetValue(TableKey,
			  "Configuration Data",
			  REG_FULL_RESOURCE_DESCRIPTOR,
			  (PU8) FullResourceDescriptor,
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


VOID
DetectPciBios(HKEY SystemKey, U32 *BusNumber)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  CM_PCI_BUS_DATA BusData;
  char Buffer[80];
  HKEY BiosKey;
  U32 Size;
  S32 Error;
#if 0
  HKEY BusKey;
  U32 i;
#endif

  /* Report the PCI BIOS */
  if (FindPciBios(&BusData))
    {
      /* Create new bus key */
      sprintf(Buffer,
	      "MultifunctionAdapter\\%u", *BusNumber);
      Error = RegCreateKey(SystemKey,
			   Buffer,
			   &BiosKey);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
	  return;
	}

      /* Set 'Component Information' */
      SetComponentInformation(BiosKey,
                              0x0,
                              0x0,
                              0xFFFFFFFF);

      /* Increment bus number */
      (*BusNumber)++;

      /* Set 'Identifier' value */
      Error = RegSetValue(BiosKey,
			  "Identifier",
			  REG_SZ,
			  (PU8)"PCI BIOS",
			  9);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
	  return;
	}

      /* Set 'Configuration Data' value */
      Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) -
	     sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
      FullResourceDescriptor = MmAllocateMemory(Size);
      if (FullResourceDescriptor == NULL)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "Failed to allocate resource descriptor\n"));
	  return;
	}

      /* Initialize resource descriptor */
      memset(FullResourceDescriptor, 0, Size);
      FullResourceDescriptor->InterfaceType = Internal;
      FullResourceDescriptor->BusNumber = 0;
      FullResourceDescriptor->PartialResourceList.Count = 0;

      /* Set 'Configuration Data' value */
      Error = RegSetValue(BiosKey,
			  "Configuration Data",
			  REG_FULL_RESOURCE_DESCRIPTOR,
			  (PU8) FullResourceDescriptor,
			  Size);
      MmFreeMemory(FullResourceDescriptor);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "RegSetValue(Configuration Data) failed (Error %u)\n",
		    (int)Error));
	  return;
	}

      DetectPciIrqRoutingTable(BiosKey);

#if 0
      /*
       * FIXME:
       * Enabling this piece of code will corrupt the boot sequence!
       * This is probably caused by a bug in the registry code!
       */

      /* Report PCI buses */
      for (i = 0; i < (U32)BusData.BusCount; i++)
	{
	  sprintf(Buffer,
		  "MultifunctionAdapter\\%u", *BusNumber);
	  Error = RegCreateKey(SystemKey,
			       Buffer,
			       &BusKey);
	  if (Error != ERROR_SUCCESS)
	    {
	      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
	      printf("RegCreateKey() failed (Error %u)\n", (int)Error);
	      return;
	    }

	  /* Set 'Component Information' */
	  SetComponentInformation(BusKey,
				  0x0,
				  0x0,
				  0xFFFFFFFF);

	  /* Increment bus number */
	  (*BusNumber)++;


	  /* Set 'Identifier' value */
	  Error = RegSetValue(BusKey,
			      "Identifier",
			      REG_SZ,
			      (PU8)"PCI",
			      4);
	  if (Error != ERROR_SUCCESS)
	    {
	      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
	      return;
	    }
	}
#endif

    }
}

/* EOF */
