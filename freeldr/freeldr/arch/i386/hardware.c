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


#define MILLISEC     (10)
#define PRECISION    (8)

#define HZ (100)
#define CLOCK_TICK_RATE (1193182)
#define LATCH (CLOCK_TICK_RATE / HZ)


typedef enum
{
  InterfaceTypeUndefined = -1,
  Internal,
  Isa,
  Eisa,
  MicroChannel,
  TurboChannel,
  PCIBus,
  VMEBus,
  NuBus,
  PCMCIABus,
  CBus,
  MPIBus,
  MPSABus,
  ProcessorInternal,
  InternalPowerBus,
  PNPISABus,
  MaximumInterfaceType
} INTERFACE_TYPE, *PINTERFACE_TYPE;


typedef U64 PHYSICAL_ADDRESS;


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


typedef struct
{
  U8 Type;
  U8 ShareDisposition;
  U16 Flags;
  union
    {
      struct
	{
	  PHYSICAL_ADDRESS Start;
	  U32 Length;
	} __attribute__((packed)) Port;
      struct
	{
	  U32 Level;
	  U32 Vector;
	  U32 Affinity;
	} __attribute__((packed)) Interrupt;
      struct
	{
	  PHYSICAL_ADDRESS Start;
	  U32 Length;
	} __attribute__((packed)) Memory;
      struct
	{
	  U32 Channel;
	  U32 Port;
	  U32 Reserved1;
	} __attribute__((packed)) Dma;
      struct
	{
	  U32 DataSize;
	  U32 Reserved1;
	  U32 Reserved2;
	} __attribute__((packed)) DeviceSpecificData;
    } __attribute__((packed)) u;
} __attribute__((packed)) CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;


typedef struct
{
  U16 Version;
  U16 Revision;
  U32 Count;
  CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];
} __attribute__((packed))CM_PARTIAL_RESOURCE_LIST, *PCM_PARTIAL_RESOURCE_LIST;


typedef struct
{
  INTERFACE_TYPE InterfaceType;
  U32 BusNumber;
  CM_PARTIAL_RESOURCE_LIST PartialResourceList;
} __attribute__((packed)) CM_FULL_RESOURCE_DESCRIPTOR, *PCM_FULL_RESOURCE_DESCRIPTOR;


typedef struct _DEVICE_NODE
{
  U16 Size;
  U8  Node;
  U8  ProductId[4];
  U8  DeviceType[3];
  U16 DeviceAttributes;
} PACKED DEVICE_NODE, *PDEVICE_NODE;

typedef struct _MPS_CONFIG_TABLE_HEADER
{
  U32  Signature;
  U16 BaseTableLength;
  U8  SpecRev;
  U8  Checksum;
  CHAR   OemIdString[8];
  CHAR   ProductIdString[12];
  U32   OemTablePointer;
  U16   OemTableLength;
  U16   EntryCount;
  U32   AddressOfLocalAPIC;
  U16   ExtendedTableLength;
  U8    ExtendedTableChecksum;
  U8    Reserved;
} PACKED MPS_CONFIG_TABLE_HEADER, *PMPS_CONFIG_TABLE_HEADER;

typedef struct _MPS_PROCESSOR_ENTRY
{
  U8  EntryType;
  U8  LocalApicId;
  U8  LocalApicVersion;
  U8  CpuFlags;
  U32 CpuSignature;
  U32 FeatureFlags;
  U32 Reserved1;
  U32 Reserved2;
} PACKED MPS_PROCESSOR_ENTRY, *PMPS_PROCESSOR_ENTRY;

static char Hex[] = "0123456789ABCDEF";
static unsigned int delay_count = 1;

/* PROTOTYPES ***************************************************************/

/* i386cpu.S */

U32 CpuidSupported(VOID);
VOID GetCpuid(U32 Level, U32 *eax, U32 *ebx, U32 *ecx, U32 *edx);

U32 MpsSupported(VOID);
U32 MpsGetDefaultConfiguration(VOID);
U32 MpsGetConfigurationTable(PVOID ConfigTable);


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


static VOID
DetectCPU(VOID)
{
  char VendorIdentifier[13];
//  char Buffer[64];
  U32 eax = 0;
  U32 ebx = 0;
  U32 ecx = 0;
  U32 edx = 0;
  U32 *Ptr;

  eax = CpuidSupported();
  if (eax & 1)
    {
      printf("  CPUID supported\n");

      GetCpuid(0, &eax, &ebx, &ecx, &edx);
//      printf("    Level 0: %x %x %x %x\n", (int)eax, (int)ebx, (int)ecx, (int)edx);

      VendorIdentifier[12] = 0;
      Ptr = (U32*)&VendorIdentifier[0];
      *Ptr = ebx;
      Ptr++;
      *Ptr = edx;
      Ptr++;
      *Ptr = ecx;

      printf("    Vendor Identifier: %s\n",
	     VendorIdentifier);

      GetCpuid(1, &eax, &ebx, &ecx, &edx);
//      printf("    Level 1: %x %x %x %x\n", (int)eax, (int)ebx, (int)ecx, (int)edx);

      printf("    Identifier: x86 Family %u Model %u Stepping %u\n",
	     (unsigned int)((eax >> 8) & 0x0F),
	     (unsigned int)((eax >> 4) & 0x0F),
	     (unsigned int)(eax & 0x0F));

      printf("    FeatureSet: %x\n",
	     (unsigned int)edx);
    }
  else
    {
      edx = 0; /* No feature set */

      printf("  CPUID not supported\n");
      printf("    Vendor Identifier: Unknown\n");
      printf("    Identifier: x86 Family %u Model %u Stepping %u\n",
	     (unsigned int)((eax >> 8) & 0x0F),
	     (unsigned int)((eax >> 4) & 0x0F),
	     (unsigned int)(eax & 0x0F));
      printf("    FeatureSet: %x\n",
	     (unsigned int)edx);
    }

  /* FIXME: add more checks */


  /* FIXME: store cpu info in the registry (CPU 0 only) */

  printf("\n");
}


static BOOL
DetectMps(VOID)
{
  U32 DefaultConfig;
  char *Buffer;
//  char Signature[5];
  PMPS_CONFIG_TABLE_HEADER ConfigTable;
  PMPS_PROCESSOR_ENTRY CpuEntry;
  char *Ptr;
  unsigned long Offset;


  DefaultConfig = MpsGetDefaultConfiguration();
  if (DefaultConfig == 0)
    {
      /* Read configuration table */
      MpsGetConfigurationTable((PVOID)DISKREADBUFFER);
      Buffer = (char *)DISKREADBUFFER;

//      Signature[0] = Buffer[0];
//      Signature[1] = Buffer[1];
//      Signature[2] = Buffer[2];
//      Signature[3] = Buffer[3];
//      Signature[4] = 0;

//      printf("    Signature: %s\n",
//		 Signature);

      ConfigTable = (PMPS_CONFIG_TABLE_HEADER)DISKREADBUFFER;
      Offset = 0x2C;
      while (Offset < ConfigTable->BaseTableLength)
	{
	  Ptr = Buffer + Offset;

	  switch (*Ptr)
	    {
	      case 0:
		CpuEntry = (PMPS_PROCESSOR_ENTRY)Ptr;
		printf ("    Processor %u: x86 Family %u Model %u Stepping %u\n",
			CpuEntry->LocalApicId,
			(unsigned int)((CpuEntry->CpuSignature >> 8) & 0x0F),
			(unsigned int)((CpuEntry->CpuSignature >> 4) & 0x0F),
			(unsigned int)(CpuEntry->CpuSignature & 0x0F));

//		printf("    Processor Entry\n");
//		printf("      APIC Id %u  APIC Version %u  Flags %x  Signature %x  Feature %x\n",
//		       CpuEntry->LocalApicId,
//		       CpuEntry->LocalApicVersion,
//		       CpuEntry->CpuFlags,
//		       CpuEntry->CpuSignature,
//		       CpuEntry->FeatureFlags);

		Offset += 0x14;
		break;

	      case 1:
//		printf("    Bus Entry\n");
		Offset += 0x08;
		break;

	      case 2:
//		printf("    I/0 APIC Entry\n");
		Offset += 0x08;
		break;

	      case 3:
//		printf("    I/0 Interrupt Assignment Entry\n");
		Offset += 0x08;
		break;

	      case 4:
//		printf("    Local Interrupt Assignment Entry\n");
		Offset += 0x08;
		break;

	      default:
//		printf("    Unknown Entry %u\n",(unsigned int)*Ptr);
		return FALSE;
	    }

	}
    }
  else
    {
//      printf("    Unsupported MPS configuration: %x\n", (unsigned int)DefaultConfig);
      return FALSE;
    }

  return TRUE;
}


static VOID
DetectCPUs(VOID)
{
  if (MpsSupported ())
    DetectMps ();
  else
    DetectCPU ();
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
  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].u.DeviceSpecificData.DataSize =
    sizeof(CM_INT13_DRIVE_PARAMETER) * DiskCount;

  /* Get harddisk Int13 geometry data */
  Int13Drives = ((PVOID)FullResourceDescriptor) + sizeof(CM_FULL_RESOURCE_DESCRIPTOR);
  for (i = 0; i < DiskCount; i++)
    {
      if (DiskGetDriveParameters(0x80 + DiskCount, &Geometry))
	{
	  Int13Drives[i].DriveSelect = 0x80 + 1;
	  Int13Drives[i].MaxCylinders = Geometry.Cylinders - 1;
	  Int13Drives[i].SectorsPerTrack = Geometry.Sectors;
	  Int13Drives[i].MaxHeads = Geometry.Heads - 1;
	  Int13Drives[i].NumberDrives = DiskCount;
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
DetectIsaBus(U32 *BusNumber)
{
  char Buffer[80];
  HKEY SystemKey;
  HKEY BusKey;
  S32 Error;

  /* Create or open the 'System' key */
  Error = RegCreateKey(NULL,
		       "\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System",
		       &SystemKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return;
    }

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

  /* Set bus identifier */
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

  /* Detect ISA/BIOS devices */
//  DetectBiosFloppys(SystemKey, BusKey);
  DetectBiosDisks(SystemKey, BusKey);

// DetectBiosSerialPorts
// DetectBiosParallelPorts

// DetectBiosKeyboard
// DetectBiosMouse

  /* FIXME: Detect more ISA devices */
}


VOID
DetectHardware(VOID)
{
  U32 BusNumber = 0;

  DbgPrint((DPRINT_HWDETECT, "DetectHardware()\n"));

  HalpCalibrateStallExecution ();

//  DetectBiosData();
  DetectCPUs ();

  /* Detect buses */
//  DetectPciBios (&BusNumber);
//  DetectPci (&BusNumber);
//  DetectApm (&BusNumber);
//  DetectPnpBios (&BusNumber);

  DetectIsaBus (&BusNumber);


  DbgPrint ((DPRINT_HWDETECT, "DetectHardware() Done\n"));

//  printf ("*** System stopped ***\n");
//  for (;;);
}

/* EOF */
