/*
 *  FreeLoader
 *
 *  Copyright (C) 2003  Eric Kohl
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
#include <disk.h>
#include <mm.h>
#include <portio.h>

#include "../../reactos/registry.h"
#include "hardware.h"


#define MILLISEC     (10)
#define PRECISION    (8)

#define HZ (100)
#define CLOCK_TICK_RATE (1193182)
#define LATCH (CLOCK_TICK_RATE / HZ)




typedef struct _CM_INT13_DRIVE_PARAMETER
{
  U16 DriveSelect;
  U32 MaxCylinders;
  U16 SectorsPerTrack;
  U16 MaxHeads;
  U16 NumberDrives;
} CM_INT13_DRIVE_PARAMETER, *PCM_INT13_DRIVE_PARAMETER;


typedef struct _CM_DISK_GEOMETRY_DEVICE_DATA
{
  U32 BytesPerSector;
  U32 NumberOfCylinders;
  U32 SectorsPerTrack;
  U32 NumberOfHeads;
} CM_DISK_GEOMETRY_DEVICE_DATA, *PCM_DISK_GEOMETRY_DEVICE_DATA;




typedef struct _CM_PNP_BIOS_DEVICE_NODE
{
  U16 Size;
  U8  Node;
  U32 ProductId;
  U8  DeviceType[3];
  U16 DeviceAttributes;
} __attribute__((packed)) CM_PNP_BIOS_DEVICE_NODE, *PCM_PNP_BIOS_DEVICE_NODE;


typedef struct _CM_PNP_BIOS_INSTALLATION_CHECK
{
  U8  Signature[4];
  U8  Revision;
  U8  Length;
  U16 ControlField;
  U8  Checksum;
  U32 EventFlagAddress;
  U16 RealModeEntryOffset;
  U16 RealModeEntrySegment;
  U16 ProtectedModeEntryOffset;
  U32 ProtectedModeCodeBaseAddress;
  U32 OemDeviceId;
  U16 RealModeDataBaseAddress;
  U32 ProtectedModeDataBaseAddress;
} __attribute__((packed)) CM_PNP_BIOS_INSTALLATION_CHECK, *PCM_PNP_BIOS_INSTALLATION_CHECK;



static char Hex[] = "0123456789ABCDEF";
static unsigned int delay_count = 1;


/* FUNCTIONS ****************************************************************/


static VOID
__KeStallExecutionProcessor(U32 Loops)
{
  register unsigned int i;
  for (i = 0; i < Loops; i++);
}

VOID KeStallExecutionProcessor(U32 Microseconds)
{
  __KeStallExecutionProcessor((delay_count * Microseconds) / 1000);
}


static U32
Read8254Timer(VOID)
{
  U32 Count;

  WRITE_PORT_UCHAR((PU8)0x43, 0x00);
  Count = READ_PORT_UCHAR((PU8)0x40);
  Count |= READ_PORT_UCHAR((PU8)0x40) << 8;

  return Count;
}


static VOID
WaitFor8254Wraparound(VOID)
{
  U32 CurCount;
  U32 PrevCount = ~0;
  S32 Delta;

  CurCount = Read8254Timer();

  do
    {
      PrevCount = CurCount;
      CurCount = Read8254Timer();
      Delta = CurCount - PrevCount;

      /*
       * This limit for delta seems arbitrary, but it isn't, it's
       * slightly above the level of error a buggy Mercury/Neptune
       * chipset timer can cause.
       */
    }
  while (Delta < 300);
}


VOID
HalpCalibrateStallExecution(VOID)
{
  U32 i;
  U32 calib_bit;
  U32 CurCount;

  /* Initialise timer interrupt with MILLISECOND ms interval        */
  WRITE_PORT_UCHAR((PU8)0x43, 0x34);  /* binary, mode 2, LSB/MSB, ch 0 */
  WRITE_PORT_UCHAR((PU8)0x40, LATCH & 0xff); /* LSB */
  WRITE_PORT_UCHAR((PU8)0x40, LATCH >> 8); /* MSB */
  
  /* Stage 1:  Coarse calibration                                   */
  
  WaitFor8254Wraparound();
  
  delay_count = 1;
  
  do {
    delay_count <<= 1;                  /* Next delay count to try */

    WaitFor8254Wraparound();
  
    __KeStallExecutionProcessor(delay_count);      /* Do the delay */
  
    CurCount = Read8254Timer();
  } while (CurCount > LATCH / 2);
  
  delay_count >>= 1;              /* Get bottom value for delay     */
  
  /* Stage 2:  Fine calibration                                     */
  
  calib_bit = delay_count;        /* Which bit are we going to test */
  
  for(i=0;i<PRECISION;i++) {
    calib_bit >>= 1;             /* Next bit to calibrate          */
    if(!calib_bit) break;        /* If we have done all bits, stop */
  
    delay_count |= calib_bit;        /* Set the bit in delay_count */
  
    WaitFor8254Wraparound();
  
    __KeStallExecutionProcessor(delay_count);      /* Do the delay */
  
    CurCount = Read8254Timer();
    if (CurCount <= LATCH / 2)   /* If a tick has passed, turn the */
      delay_count &= ~calib_bit; /* calibrated bit back off        */
  }
  
  /* We're finished:  Do the finishing touches                      */
  
  delay_count /= (MILLISEC / 2);   /* Calculate delay_count for 1ms */
}


VOID
SetComponentInformation(HKEY ComponentKey,
			U32 Flags,
			U32 Key,
			U32 Affinity)
{
  CM_COMPONENT_INFORMATION CompInfo;
  S32 Error;

  CompInfo.Flags = Flags;
  CompInfo.Version = 0;
  CompInfo.Key = Key;
  CompInfo.Affinity = Affinity;

  /* Set 'Component Information' value */
  Error = RegSetValue(ComponentKey,
		      "Component Information",
		      REG_BINARY,
		      (PU8)&CompInfo,
		      sizeof(CM_COMPONENT_INFORMATION));
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }
}


static VOID
DetectPnpBios(HKEY SystemKey, U32 *BusNumber)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_PNP_BIOS_DEVICE_NODE DeviceNode;
  PCM_PNP_BIOS_INSTALLATION_CHECK InstData;
  char Buffer[80];
  HKEY BusKey;
  U32 x;
  U32 NodeSize = 0;
  U32 NodeCount = 0;
  U8 NodeNumber;
  U32 FoundNodeCount;
  int i;
  U32 PnpBufferSize;
  U32 Size;
  char *Ptr;
  S32 Error;

  InstData = (PCM_PNP_BIOS_INSTALLATION_CHECK)PnpBiosSupported();
  if (InstData == NULL || strncmp(InstData->Signature, "$PnP", 4))
    {
      DbgPrint((DPRINT_HWDETECT, "PnP-BIOS not supported\n"));
      return;
    }
  DbgPrint((DPRINT_HWDETECT, "Signature '%c%c%c%c'\n",
	    InstData->Signature[0], InstData->Signature[1],
	    InstData->Signature[2], InstData->Signature[3]));


  x = PnpBiosGetDeviceNodeCount(&NodeSize, &NodeCount);
  if (x != 0 || NodeSize == 0 || NodeCount == 0)
    {
      DbgPrint((DPRINT_HWDETECT, "PnP-BIOS failed to enumerate device nodes\n"));
      return;
    }
  DbgPrint((DPRINT_HWDETECT, "PnP-BIOS supported\n"));
  DbgPrint((DPRINT_HWDETECT, "MaxNodeSize %u  NodeCount %u\n", NodeSize, NodeCount));
  DbgPrint((DPRINT_HWDETECT, "Estimated buffer size %u\n", NodeSize * NodeCount));

  /* Create new bus key */
  sprintf(Buffer,
	  "MultifunctionAdapter\\%u", *BusNumber);
  Error = RegCreateKey(SystemKey,
		       Buffer,
		       &BusKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return;
    }

  /* Increment bus number */
  (*BusNumber)++;

  /* Set 'Identifier' value */
  Error = RegSetValue(BusKey,
		      "Identifier",
		      REG_SZ,
		      (PU8)"PNP BIOS",
		      9);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
      return;
    }

  /* Set 'Configuration Data' value */
  Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) + (NodeSize * NodeCount);
  FullResourceDescriptor = MmAllocateMemory(Size);
  if (FullResourceDescriptor == NULL)
    {
      DbgPrint((DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n"));
      return;
    }
  memset(FullResourceDescriptor, 0, Size);

  /* Initialize resource descriptor */
  FullResourceDescriptor->InterfaceType = Internal;
  FullResourceDescriptor->BusNumber = 0;
  FullResourceDescriptor->PartialResourceList.Count = 1;
  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].Type =
    CmResourceTypeDeviceSpecific;
  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].ShareDisposition =
    CmResourceShareUndetermined;
//  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].Flags =

  Ptr = (char *)(((PVOID)&FullResourceDescriptor->PartialResourceList.PartialDescriptors[0]) +
		 sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

  /* Set instalation check data */
  memcpy (Ptr, InstData, sizeof(CM_PNP_BIOS_INSTALLATION_CHECK));
  Ptr += sizeof(CM_PNP_BIOS_INSTALLATION_CHECK);

  /* Copy device nodes */
  FoundNodeCount = 0;
  PnpBufferSize = sizeof(CM_PNP_BIOS_INSTALLATION_CHECK);
  for (i = 0; i < 0xFF; i++)
    {
      NodeNumber = (U8)i;

      x = PnpBiosGetDeviceNode(&NodeNumber, (PVOID)DISKREADBUFFER);
      if (x == 0)
	{
	  DeviceNode = (PCM_PNP_BIOS_DEVICE_NODE)DISKREADBUFFER;

	  DbgPrint((DPRINT_HWDETECT,
		    "Node: %u  Size %u (0x%x)\n",
		    DeviceNode->Node,
		    DeviceNode->Size,
		    DeviceNode->Size));

	  memcpy (Ptr,
		  DeviceNode,
		  DeviceNode->Size);

	  Ptr += DeviceNode->Size;
	  PnpBufferSize += DeviceNode->Size;

	  FoundNodeCount++;
	  if (FoundNodeCount >= NodeCount)
	    break;
	}
    }

  /* Set real data size */
  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].u.DeviceSpecificData.DataSize =
    PnpBufferSize;
  Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) + PnpBufferSize;

  DbgPrint((DPRINT_HWDETECT, "Real buffer size: %u\n", PnpBufferSize));
  DbgPrint((DPRINT_HWDETECT, "Resource size: %u\n", Size));

  /* Set 'Configuration Data' value */
  Error = RegSetValue(BusKey,
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
    }
}



static VOID
SetHarddiskConfigurationData(HKEY DiskKey,
			     U32 DriveNumber)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_DISK_GEOMETRY_DEVICE_DATA DiskGeometry;
  EXTENDED_GEOMETRY ExtGeometry;
  GEOMETRY Geometry;
  U32 Size;
  S32 Error;

  /* Set 'Configuration Data' value */
  Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
	 sizeof(CM_DISK_GEOMETRY_DEVICE_DATA);
  FullResourceDescriptor = MmAllocateMemory(Size);
  if (FullResourceDescriptor == NULL)
    {
      DbgPrint((DPRINT_HWDETECT,
		"Failed to allocate a full resource descriptor\n"));
      return;
    }

  memset(FullResourceDescriptor, 0, Size);
  FullResourceDescriptor->InterfaceType = InterfaceTypeUndefined;
  FullResourceDescriptor->BusNumber = 0;
  FullResourceDescriptor->PartialResourceList.Count = 1;
//  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].Type =
//  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].ShareDisposition =
//  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].Flags =
  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].u.DeviceSpecificData.DataSize =
    sizeof(CM_DISK_GEOMETRY_DEVICE_DATA);

  /* Get pointer to geometry data */
  DiskGeometry = ((PVOID)FullResourceDescriptor) + sizeof(CM_FULL_RESOURCE_DESCRIPTOR);

  /* Get the disk geometry */
  ExtGeometry.Size = sizeof(EXTENDED_GEOMETRY);
  if (DiskGetExtendedDriveParameters(DriveNumber, &ExtGeometry, ExtGeometry.Size))
    {
      DiskGeometry->BytesPerSector = ExtGeometry.BytesPerSector;
      DiskGeometry->NumberOfCylinders = ExtGeometry.Cylinders;
      DiskGeometry->SectorsPerTrack = ExtGeometry.SectorsPerTrack;
      DiskGeometry->NumberOfHeads = ExtGeometry.Heads;
    }
  else if(DiskGetDriveParameters(DriveNumber, &Geometry))
    {
      DiskGeometry->BytesPerSector = Geometry.BytesPerSector;
      DiskGeometry->NumberOfCylinders = Geometry.Cylinders;
      DiskGeometry->SectorsPerTrack = Geometry.Sectors;
      DiskGeometry->NumberOfHeads = Geometry.Heads;
    }
  else
    {
      DbgPrint((DPRINT_HWDETECT, "Reading disk geometry failed\n"));
      MmFreeMemory(FullResourceDescriptor);
      return;
    }
  DbgPrint((DPRINT_HWDETECT,
	   "Disk %x: %u Cylinders  %u Heads  %u Sectors  %u Bytes\n",
	   DriveNumber,
	   DiskGeometry->NumberOfCylinders,
	   DiskGeometry->NumberOfHeads,
	   DiskGeometry->SectorsPerTrack,
	   DiskGeometry->BytesPerSector));

  Error = RegSetValue(DiskKey,
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
    }
}


static VOID
SetHarddiskIdentifier(HKEY DiskKey,
		      U32 DriveNumber)
{
  PMASTER_BOOT_RECORD Mbr;
  U32 *Buffer;
  U32 i;
  U32 Checksum;
  U32 Signature;
  char Identifier[20];
  S32 Error;

  /* Read the MBR */
  if (!DiskReadLogicalSectors(DriveNumber, 0ULL, 1, (PVOID)DISKREADBUFFER))
    {
      DbgPrint((DPRINT_HWDETECT, "Reading MBR failed\n"));
      return;
    }

  Buffer = (U32*)DISKREADBUFFER;
  Mbr = (PMASTER_BOOT_RECORD)DISKREADBUFFER;

  Signature =  Mbr->Signature;
  DbgPrint((DPRINT_HWDETECT, "Signature: %x\n", Signature));

  /* Calculate the MBR checksum */
  Checksum = 0;
  for (i = 0; i < 128; i++)
    {
      Checksum += Buffer[i];
    }
  Checksum = ~Checksum + 1;
  DbgPrint((DPRINT_HWDETECT, "Checksum: %x\n", Checksum));

  /* Convert checksum and signature to identifier string */
  Identifier[0] = Hex[(Checksum >> 28) & 0x0F];
  Identifier[1] = Hex[(Checksum >> 24) & 0x0F];
  Identifier[2] = Hex[(Checksum >> 20) & 0x0F];
  Identifier[3] = Hex[(Checksum >> 16) & 0x0F];
  Identifier[4] = Hex[(Checksum >> 12) & 0x0F];
  Identifier[5] = Hex[(Checksum >> 8) & 0x0F];
  Identifier[6] = Hex[(Checksum >> 4) & 0x0F];
  Identifier[7] = Hex[Checksum & 0x0F];
  Identifier[8] = '-';
  Identifier[9] = Hex[(Signature >> 28) & 0x0F];
  Identifier[10] = Hex[(Signature >> 24) & 0x0F];
  Identifier[11] = Hex[(Signature >> 20) & 0x0F];
  Identifier[12] = Hex[(Signature >> 16) & 0x0F];
  Identifier[13] = Hex[(Signature >> 12) & 0x0F];
  Identifier[14] = Hex[(Signature >> 8) & 0x0F];
  Identifier[15] = Hex[(Signature >> 4) & 0x0F];
  Identifier[16] = Hex[Signature & 0x0F];
  Identifier[17] = '-';
  Identifier[18] = 'A';
  Identifier[19] = 0;
  DbgPrint((DPRINT_HWDETECT, "Identifier: %xsn", Identifier));

  /* Set identifier */
  Error = RegSetValue(DiskKey,
		      "Identifier",
		      REG_SZ,
		      (PU8) Identifier,
		      20);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT,
		"RegSetValue(Identifier) failed (Error %u)\n",
		(int)Error));
    }
}


static VOID
DetectBiosDisks(HKEY SystemKey,
		HKEY BusKey)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_INT13_DRIVE_PARAMETER Int13Drives;
  GEOMETRY Geometry;
  char Buffer[80];
  HKEY DiskKey;
  U32 DiskCount;
  U32 Size;
  U32 i;
  S32 Error;

  /* Count the number of visible drives */
  DiskCount = 0;
  while (DiskGetDriveParameters(0x80 + DiskCount, &Geometry))
    {
      DiskCount++;
    }
  DbgPrint((DPRINT_HWDETECT, "BIOS reports %d harddisk%s\n",
	    (int)DiskCount, (DiskCount == 1) ? "": "s"));

  /* Allocate resource descriptor */
  Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
	 sizeof(CM_INT13_DRIVE_PARAMETER) * DiskCount;
  FullResourceDescriptor = MmAllocateMemory(Size);
  if (FullResourceDescriptor == NULL)
    {
      DbgPrint((DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n"));
      return;
    }

  /* Initialize resource descriptor */
  memset(FullResourceDescriptor, 0, Size);
  FullResourceDescriptor->InterfaceType = InterfaceTypeUndefined;
  FullResourceDescriptor->BusNumber = -1;
  FullResourceDescriptor->PartialResourceList.Count = 1;
//  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].Type =
//  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].ShareDisposition =
//  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].Flags =
  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].u.DeviceSpecificData.DataSize =
    sizeof(CM_INT13_DRIVE_PARAMETER) * DiskCount;

  /* Get harddisk Int13 geometry data */
  Int13Drives = ((PVOID)FullResourceDescriptor) + sizeof(CM_FULL_RESOURCE_DESCRIPTOR);
  for (i = 0; i < DiskCount; i++)
    {
      if (DiskGetDriveParameters(0x80 + i, &Geometry))
	{
	  Int13Drives[i].DriveSelect = 0x80 + i;
	  Int13Drives[i].MaxCylinders = Geometry.Cylinders - 1;
	  Int13Drives[i].SectorsPerTrack = Geometry.Sectors;
	  Int13Drives[i].MaxHeads = Geometry.Heads - 1;
	  Int13Drives[i].NumberDrives = DiskCount;

	  DbgPrint((DPRINT_HWDETECT,
		    "Disk %x: %u Cylinders  %u Heads  %u Sectors  %u Bytes\n",
		    0x80 + i,
		    Geometry.Cylinders - 1,
		    Geometry.Heads -1,
		    Geometry.Sectors,
		    Geometry.BytesPerSector));
	}
    }

  /* Set 'Configuration Data' value */
  Error = RegSetValue(SystemKey,
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

  /* Create and fill subkey for each harddisk */
  for (i = 0; i < DiskCount; i++)
    {
      /* Create disk key */
      sprintf (Buffer,
	       "DiskController\\0\\DiskPeripheral\\%u",
	       i);

      Error = RegCreateKey(BusKey,
			   Buffer,
			   &DiskKey);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT, "Failed to create drive key\n"));
	  continue;
	}
      DbgPrint((DPRINT_HWDETECT, "Created key: %s\n", Buffer));

      /* Set disk values */
      SetHarddiskConfigurationData(DiskKey, 0x80 + i);
      SetHarddiskIdentifier(DiskKey, 0x80 + i);
    }
}


static VOID
DetectIsaBios(HKEY SystemKey, U32 *BusNumber)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  char Buffer[80];
  HKEY BusKey;
  U32 Size;
  S32 Error;

  /* Create new bus key */
  sprintf(Buffer,
	  "MultifunctionAdapter\\%u", *BusNumber);
  Error = RegCreateKey(SystemKey,
		       Buffer,
		       &BusKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return;
    }

  /* Increment bus number */
  (*BusNumber)++;

  /* Set 'Identifier' value */
  Error = RegSetValue(BusKey,
		      "Identifier",
		      REG_SZ,
		      (PU8)"ISA",
		      4);
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
  FullResourceDescriptor->InterfaceType = Isa;
  FullResourceDescriptor->BusNumber = 0;
  FullResourceDescriptor->PartialResourceList.Count = 0;

  /* Set 'Configuration Data' value */
  Error = RegSetValue(SystemKey,
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


  /* Detect ISA/BIOS devices */
  DetectBiosDisks(SystemKey, BusKey);
//  DetectBiosFloppyDisks(SystemKey, BusKey);

// DetectBiosSerialPorts
// DetectBiosParallelPorts

// DetectBiosKeyboard
// DetectBiosMouse

  /* FIXME: Detect more ISA devices */
}


VOID
DetectHardware(VOID)
{
  HKEY SystemKey;
  U32 BusNumber = 0;
  S32 Error

  DbgPrint((DPRINT_HWDETECT, "DetectHardware()\n"));

  HalpCalibrateStallExecution ();

  /* Create the 'System' key */
  Error = RegCreateKey(NULL,
		       "\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System",
		       &SystemKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return;
    }

//  DetectSystemData ();
  DetectCPUs (SystemKey);

  /* Detect buses */

//  DetectPciBios (&BusNumber);
//  DetectApmBios (&BusNumber);
  DetectPnpBios (SystemKey, &BusNumber);
  DetectIsaBios (SystemKey, &BusNumber);
//  DetectAcpiBios (&BusNumber);



  DbgPrint ((DPRINT_HWDETECT, "DetectHardware() Done\n"));

//  printf ("*** System stopped ***\n");
//  for (;;);
}

/* EOF */
