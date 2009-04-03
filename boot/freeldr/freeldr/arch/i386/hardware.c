/*
 *  FreeLoader
 *
 *  Copyright (C) 2003, 2004  Eric Kohl
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

#define MILLISEC     (10)
#define PRECISION    (8)

#define HZ (100)
#define CLOCK_TICK_RATE (1193182)
#define LATCH (CLOCK_TICK_RATE / HZ)


/* No Mouse */
#define MOUSE_TYPE_NONE			0
/* Microsoft Mouse with 2 buttons */
#define MOUSE_TYPE_MICROSOFT	1
/* Logitech Mouse with 3 buttons */
#define MOUSE_TYPE_LOGITECH		2
/* Microsoft Wheel Mouse (aka Z Mouse) */
#define MOUSE_TYPE_WHEELZ		3
/* Mouse Systems Mouse */
#define MOUSE_TYPE_MOUSESYSTEMS	4


/* PS2 stuff */

/* Controller registers. */
#define CONTROLLER_REGISTER_STATUS                      0x64
#define CONTROLLER_REGISTER_CONTROL                     0x64
#define CONTROLLER_REGISTER_DATA                        0x60

/* Controller commands. */
#define CONTROLLER_COMMAND_READ_MODE                    0x20
#define CONTROLLER_COMMAND_WRITE_MODE                   0x60
#define CONTROLLER_COMMAND_GET_VERSION                  0xA1
#define CONTROLLER_COMMAND_MOUSE_DISABLE                0xA7
#define CONTROLLER_COMMAND_MOUSE_ENABLE                 0xA8
#define CONTROLLER_COMMAND_TEST_MOUSE                   0xA9
#define CONTROLLER_COMMAND_SELF_TEST                    0xAA
#define CONTROLLER_COMMAND_KEYBOARD_TEST                0xAB
#define CONTROLLER_COMMAND_KEYBOARD_DISABLE             0xAD
#define CONTROLLER_COMMAND_KEYBOARD_ENABLE              0xAE
#define CONTROLLER_COMMAND_WRITE_MOUSE_OUTPUT_BUFFER    0xD3
#define CONTROLLER_COMMAND_WRITE_MOUSE                  0xD4

/* Controller status */
#define CONTROLLER_STATUS_OUTPUT_BUFFER_FULL            0x01
#define CONTROLLER_STATUS_INPUT_BUFFER_FULL             0x02
#define CONTROLLER_STATUS_SELF_TEST                     0x04
#define CONTROLLER_STATUS_COMMAND                       0x08
#define CONTROLLER_STATUS_UNLOCKED                      0x10
#define CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL      0x20
#define CONTROLLER_STATUS_GENERAL_TIMEOUT               0x40
#define CONTROLLER_STATUS_PARITY_ERROR                  0x80
#define AUX_STATUS_OUTPUT_BUFFER_FULL                   (CONTROLLER_STATUS_OUTPUT_BUFFER_FULL | \
                                                         CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL)

/* Timeout in ms for sending to keyboard controller. */
#define CONTROLLER_TIMEOUT                              250

static CHAR Hex[] = "0123456789abcdef";
static unsigned int delay_count = 1;

extern ULONG reactos_disk_count;
extern ARC_DISK_SIGNATURE reactos_arc_disk_info[];
extern char reactos_arc_strings[32][256];

/* FUNCTIONS ****************************************************************/


static VOID
__StallExecutionProcessor(ULONG Loops)
{
  register volatile unsigned int i;
  for (i = 0; i < Loops; i++);
}


VOID StallExecutionProcessor(ULONG Microseconds)
{
  ULONGLONG LoopCount = ((ULONGLONG)delay_count * (ULONGLONG)Microseconds) / 1000ULL;
  __StallExecutionProcessor((ULONG)LoopCount);
}


static ULONG
Read8254Timer(VOID)
{
  ULONG Count;

  WRITE_PORT_UCHAR((PUCHAR)0x43, 0x00);
  Count = READ_PORT_UCHAR((PUCHAR)0x40);
  Count |= READ_PORT_UCHAR((PUCHAR)0x40) << 8;

  return Count;
}


static VOID
WaitFor8254Wraparound(VOID)
{
  ULONG CurCount;
  ULONG PrevCount = ~0;
  LONG Delta;

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
  ULONG i;
  ULONG calib_bit;
  ULONG CurCount;

  /* Initialise timer interrupt with MILLISECOND ms interval        */
  WRITE_PORT_UCHAR((PUCHAR)0x43, 0x34);  /* binary, mode 2, LSB/MSB, ch 0 */
  WRITE_PORT_UCHAR((PUCHAR)0x40, LATCH & 0xff); /* LSB */
  WRITE_PORT_UCHAR((PUCHAR)0x40, LATCH >> 8); /* MSB */

  /* Stage 1:  Coarse calibration                                   */

  WaitFor8254Wraparound();

  delay_count = 1;

  do {
    delay_count <<= 1;                  /* Next delay count to try */

    WaitFor8254Wraparound();

    __StallExecutionProcessor(delay_count);      /* Do the delay */

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

    __StallExecutionProcessor(delay_count);      /* Do the delay */

    CurCount = Read8254Timer();
    if (CurCount <= LATCH / 2)   /* If a tick has passed, turn the */
      delay_count &= ~calib_bit; /* calibrated bit back off        */
  }

  /* We're finished:  Do the finishing touches                      */
  delay_count /= (MILLISEC / 2);   /* Calculate delay_count for 1ms */
}

static VOID
DetectPnpBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
  PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
  PCM_PNP_BIOS_DEVICE_NODE DeviceNode;
  PCM_PNP_BIOS_INSTALLATION_CHECK InstData;
  PCONFIGURATION_COMPONENT_DATA BusKey;
  ULONG x;
  ULONG NodeSize = 0;
  ULONG NodeCount = 0;
  UCHAR NodeNumber;
  ULONG FoundNodeCount;
  int i;
  ULONG PnpBufferSize;
  ULONG Size;
  char *Ptr;

  InstData = (PCM_PNP_BIOS_INSTALLATION_CHECK)PnpBiosSupported();
  if (InstData == NULL || strncmp((CHAR*)InstData->Signature, "$PnP", 4))
    {
      DPRINTM(DPRINT_HWDETECT, "PnP-BIOS not supported\n");
      return;
    }
  DPRINTM(DPRINT_HWDETECT, "Signature '%c%c%c%c'\n",
	    InstData->Signature[0], InstData->Signature[1],
	    InstData->Signature[2], InstData->Signature[3]);


  x = PnpBiosGetDeviceNodeCount(&NodeSize, &NodeCount);
  NodeCount &= 0xFF; // needed since some fscked up BIOSes return
                     // wrong info (e.g. Mac Virtual PC)
                     // e.g. look: http://my.execpc.com/~geezer/osd/pnp/pnp16.c
  if (x != 0 || NodeSize == 0 || NodeCount == 0)
    {
      DPRINTM(DPRINT_HWDETECT, "PnP-BIOS failed to enumerate device nodes\n");
      return;
    }
  DPRINTM(DPRINT_HWDETECT, "PnP-BIOS supported\n");
  DPRINTM(DPRINT_HWDETECT, "MaxNodeSize %u  NodeCount %u\n", NodeSize, NodeCount);
  DPRINTM(DPRINT_HWDETECT, "Estimated buffer size %u\n", NodeSize * NodeCount);

    /* Create component key */
    FldrCreateComponentKey(SystemKey,
                           L"MultifunctionAdapter",
                           *BusNumber,
                           AdapterClass,
                           MultiFunctionAdapter,
                           &BusKey);
    (*BusNumber)++;
    
    /* Set the component information */
    FldrSetComponentInformation(BusKey,
                                0x0,
                                0x0,
                                0xFFFFFFFF);
    
    /* Set the identifier */
    FldrSetIdentifier(BusKey, "PNP BIOS");

    /* Set 'Configuration Data' value */
  Size = sizeof(CM_PARTIAL_RESOURCE_LIST) + (NodeSize * NodeCount);
  PartialResourceList = MmHeapAlloc(Size);
  if (PartialResourceList == NULL)
    {
      DPRINTM(DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n");
      return;
    }
  memset(PartialResourceList, 0, Size);

  /* Initialize resource descriptor */
  PartialResourceList->Version = 1;
  PartialResourceList->Revision = 1;
  PartialResourceList->Count = 1;
  PartialResourceList->PartialDescriptors[0].Type =
    CmResourceTypeDeviceSpecific;
  PartialResourceList->PartialDescriptors[0].ShareDisposition =
    CmResourceShareUndetermined;

  Ptr = (char *)(((ULONG_PTR)&PartialResourceList->PartialDescriptors[0]) +
		 sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

  /* Set instalation check data */
  memcpy (Ptr, InstData, sizeof(CM_PNP_BIOS_INSTALLATION_CHECK));
  Ptr += sizeof(CM_PNP_BIOS_INSTALLATION_CHECK);

  /* Copy device nodes */
  FoundNodeCount = 0;
  PnpBufferSize = sizeof(CM_PNP_BIOS_INSTALLATION_CHECK);
  for (i = 0; i < 0xFF; i++)
    {
      NodeNumber = (UCHAR)i;

      x = PnpBiosGetDeviceNode(&NodeNumber, (PVOID)DISKREADBUFFER);
      if (x == 0)
	{
	  DeviceNode = (PCM_PNP_BIOS_DEVICE_NODE)DISKREADBUFFER;

	  DPRINTM(DPRINT_HWDETECT,
		    "Node: %u  Size %u (0x%x)\n",
		    DeviceNode->Node,
		    DeviceNode->Size,
		    DeviceNode->Size);

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
  PartialResourceList->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
    PnpBufferSize;
  Size = sizeof(CM_PARTIAL_RESOURCE_LIST) + PnpBufferSize;

  DPRINTM(DPRINT_HWDETECT, "Real buffer size: %u\n", PnpBufferSize);
  DPRINTM(DPRINT_HWDETECT, "Resource size: %u\n", Size);
  
    FldrSetConfigurationData(BusKey, PartialResourceList, Size);
    MmHeapFree(PartialResourceList);
}



static VOID
SetHarddiskConfigurationData(PCONFIGURATION_COMPONENT_DATA DiskKey,
			     ULONG DriveNumber)
{
  PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
  PCM_DISK_GEOMETRY_DEVICE_DATA DiskGeometry;
  EXTENDED_GEOMETRY ExtGeometry;
  GEOMETRY Geometry;
  ULONG Size;

  /* Set 'Configuration Data' value */
  Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
	 sizeof(CM_DISK_GEOMETRY_DEVICE_DATA);
  PartialResourceList = MmHeapAlloc(Size);
  if (PartialResourceList == NULL)
    {
      DPRINTM(DPRINT_HWDETECT,
		"Failed to allocate a full resource descriptor\n");
      return;
    }

  memset(PartialResourceList, 0, Size);
  PartialResourceList->Version = 1;
  PartialResourceList->Revision = 1;
  PartialResourceList->Count = 1;
  PartialResourceList->PartialDescriptors[0].Type =
    CmResourceTypeDeviceSpecific;
//  PartialResourceList->PartialDescriptors[0].ShareDisposition =
//  PartialResourceList->PartialDescriptors[0].Flags =
  PartialResourceList->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
    sizeof(CM_DISK_GEOMETRY_DEVICE_DATA);

  /* Get pointer to geometry data */
  DiskGeometry = (PVOID)(((ULONG_PTR)PartialResourceList) + sizeof(CM_PARTIAL_RESOURCE_LIST));

  /* Get the disk geometry */
  ExtGeometry.Size = sizeof(EXTENDED_GEOMETRY);
  if (DiskGetExtendedDriveParameters(DriveNumber, &ExtGeometry, ExtGeometry.Size))
    {
      DiskGeometry->BytesPerSector = ExtGeometry.BytesPerSector;
      DiskGeometry->NumberOfCylinders = ExtGeometry.Cylinders;
      DiskGeometry->SectorsPerTrack = ExtGeometry.SectorsPerTrack;
      DiskGeometry->NumberOfHeads = ExtGeometry.Heads;
    }
  else if(MachDiskGetDriveGeometry(DriveNumber, &Geometry))
    {
      DiskGeometry->BytesPerSector = Geometry.BytesPerSector;
      DiskGeometry->NumberOfCylinders = Geometry.Cylinders;
      DiskGeometry->SectorsPerTrack = Geometry.Sectors;
      DiskGeometry->NumberOfHeads = Geometry.Heads;
    }
  else
    {
      DPRINTM(DPRINT_HWDETECT, "Reading disk geometry failed\n");
      MmHeapFree(PartialResourceList);
      return;
    }
  DPRINTM(DPRINT_HWDETECT,
	   "Disk %x: %u Cylinders  %u Heads  %u Sectors  %u Bytes\n",
	   DriveNumber,
	   DiskGeometry->NumberOfCylinders,
	   DiskGeometry->NumberOfHeads,
	   DiskGeometry->SectorsPerTrack,
	   DiskGeometry->BytesPerSector);

  FldrSetConfigurationData(DiskKey, PartialResourceList, Size);
  MmHeapFree(PartialResourceList);
}


static VOID
SetHarddiskIdentifier(PCONFIGURATION_COMPONENT_DATA DiskKey,
		      ULONG DriveNumber)
{
  PMASTER_BOOT_RECORD Mbr;
  ULONG *Buffer;
  ULONG i;
  ULONG Checksum;
  ULONG Signature;
  CHAR Identifier[20];
  CHAR ArcName[256];

  /* Read the MBR */
  if (!MachDiskReadLogicalSectors(DriveNumber, 0ULL, 1, (PVOID)DISKREADBUFFER))
    {
      DPRINTM(DPRINT_HWDETECT, "Reading MBR failed\n");
      return;
    }

  Buffer = (ULONG*)DISKREADBUFFER;
  Mbr = (PMASTER_BOOT_RECORD)DISKREADBUFFER;

  Signature =  Mbr->Signature;
  DPRINTM(DPRINT_HWDETECT, "Signature: %x\n", Signature);

  /* Calculate the MBR checksum */
  Checksum = 0;
  for (i = 0; i < 128; i++)
    {
      Checksum += Buffer[i];
    }
  Checksum = ~Checksum + 1;
  DPRINTM(DPRINT_HWDETECT, "Checksum: %x\n", Checksum);

  /* Fill out the ARC disk block */
  reactos_arc_disk_info[reactos_disk_count].Signature = Signature;
  reactos_arc_disk_info[reactos_disk_count].CheckSum = Checksum;
  sprintf(ArcName, "multi(0)disk(0)rdisk(%lu)", reactos_disk_count);
  strcpy(reactos_arc_strings[reactos_disk_count], ArcName);
  reactos_arc_disk_info[reactos_disk_count].ArcName =
      reactos_arc_strings[reactos_disk_count];
  reactos_disk_count++;

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
  DPRINTM(DPRINT_HWDETECT, "Identifier: %s\n", Identifier);

  /* Set identifier */
  FldrSetIdentifier(DiskKey, Identifier);
}

static ULONG
GetFloppyCount(VOID)
{
  UCHAR Data;

  WRITE_PORT_UCHAR((PUCHAR)0x70, 0x10);
  Data = READ_PORT_UCHAR((PUCHAR)0x71);

  return ((Data & 0xF0) ? 1 : 0) + ((Data & 0x0F) ? 1 : 0);
}


static UCHAR
GetFloppyType(UCHAR DriveNumber)
{
  UCHAR Data;

  WRITE_PORT_UCHAR((PUCHAR)0x70, 0x10);
  Data = READ_PORT_UCHAR((PUCHAR)0x71);

  if (DriveNumber == 0)
    return Data >> 4;
  else if (DriveNumber == 1)
    return Data & 0x0F;

  return 0;
}


static PVOID
GetInt1eTable(VOID)
{
  PUSHORT SegPtr = (PUSHORT)0x7A;
  PUSHORT OfsPtr = (PUSHORT)0x78;

  return (PVOID)((ULONG_PTR)(((ULONG)(*SegPtr)) << 4) + (ULONG)(*OfsPtr));
}


static VOID
DetectBiosFloppyPeripheral(PCONFIGURATION_COMPONENT_DATA ControllerKey)
{
  PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  PCM_FLOPPY_DEVICE_DATA FloppyData;
  CHAR Identifier[20];
  PCONFIGURATION_COMPONENT_DATA PeripheralKey;
  ULONG Size;
  ULONG FloppyNumber;
  UCHAR FloppyType;
  ULONG MaxDensity[6] = {0, 360, 1200, 720, 1440, 2880};
  PUCHAR Ptr;

  for (FloppyNumber = 0; FloppyNumber < 2; FloppyNumber++)
  {
    FloppyType = GetFloppyType(FloppyNumber);

    if ((FloppyType > 5) || (FloppyType == 0))
      continue;

    DiskResetController(FloppyNumber);

    Ptr = GetInt1eTable();
    
    FldrCreateComponentKey(ControllerKey,
                           L"FloppyDiskPeripheral",
                           FloppyNumber,
                           PeripheralClass,
                           FloppyDiskPeripheral,
                           &PeripheralKey);

    /* Set 'ComponentInformation' value */
    FldrSetComponentInformation(PeripheralKey,
                                Input | Output,
                                FloppyNumber,
                                0xFFFFFFFF);

    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
	   sizeof(CM_FLOPPY_DEVICE_DATA);
    PartialResourceList = MmHeapAlloc(Size);
    if (PartialResourceList == NULL)
    {
      DPRINTM(DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n");
      return;
    }

    memset(PartialResourceList, 0, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 1;

    PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
    PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(CM_FLOPPY_DEVICE_DATA);

    FloppyData = (PVOID)(((ULONG_PTR)PartialResourceList) + sizeof(CM_PARTIAL_RESOURCE_LIST));
    FloppyData->Version = 2;
    FloppyData->Revision = 0;
    FloppyData->MaxDensity = MaxDensity[FloppyType];
    FloppyData->MountDensity = 0;
    RtlCopyMemory(&FloppyData->StepRateHeadUnloadTime,
                  Ptr,
                  11);
    FloppyData->MaximumTrackValue = (FloppyType == 1) ? 39 : 79;
    FloppyData->DataTransferRate = 0;

    /* Set 'Configuration Data' value */
    FldrSetConfigurationData(PeripheralKey, PartialResourceList, Size);
    MmHeapFree(PartialResourceList);

    /* Set 'Identifier' value */
    sprintf(Identifier, "FLOPPY%ld", FloppyNumber + 1);
    FldrSetIdentifier(PeripheralKey, Identifier);
  }
}


static VOID
DetectBiosFloppyController(PCONFIGURATION_COMPONENT_DATA BusKey,
                           PCONFIGURATION_COMPONENT_DATA ControllerKey)
{
  PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  ULONG Size;
  ULONG FloppyCount;

  FloppyCount = GetFloppyCount();
  DPRINTM(DPRINT_HWDETECT,
	    "Floppy count: %u\n",
	    FloppyCount);
  
  Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
	 2 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
  PartialResourceList = MmHeapAlloc(Size);
  if (PartialResourceList == NULL)
    {
      DPRINTM(DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n");
      return;
    }
  memset(PartialResourceList, 0, Size);

  /* Initialize resource descriptor */
  PartialResourceList->Version = 1;
  PartialResourceList->Revision = 1;
  PartialResourceList->Count = 3;

  /* Set IO Port */
  PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
  PartialDescriptor->Type = CmResourceTypePort;
  PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
  PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
  PartialDescriptor->u.Port.Start.LowPart = 0x03F0;
  PartialDescriptor->u.Port.Start.HighPart = 0x0;
  PartialDescriptor->u.Port.Length = 8;

  /* Set Interrupt */
  PartialDescriptor = &PartialResourceList->PartialDescriptors[1];
  PartialDescriptor->Type = CmResourceTypeInterrupt;
  PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
  PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
  PartialDescriptor->u.Interrupt.Level = 6;
  PartialDescriptor->u.Interrupt.Vector = 6;
  PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

  /* Set DMA channel */
  PartialDescriptor = &PartialResourceList->PartialDescriptors[2];
  PartialDescriptor->Type = CmResourceTypeDma;
  PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
  PartialDescriptor->Flags = 0;
  PartialDescriptor->u.Dma.Channel = 2;
  PartialDescriptor->u.Dma.Port = 0;

  /* Set 'Configuration Data' value */
  FldrSetConfigurationData(ControllerKey, PartialResourceList, Size);
  MmHeapFree(PartialResourceList);

  if (FloppyCount) DetectBiosFloppyPeripheral(ControllerKey);
}

static VOID
DetectBiosDisks(PCONFIGURATION_COMPONENT_DATA SystemKey,
                PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_INT13_DRIVE_PARAMETER Int13Drives;
    GEOMETRY Geometry;
    PCONFIGURATION_COMPONENT_DATA DiskKey, ControllerKey;
    ULONG DiskCount;
    ULONG Size;
    ULONG i;
    BOOLEAN Changed;
    
    /* Count the number of visible drives */
    DiskReportError(FALSE);
    DiskCount = 0;
    
    /* There are some really broken BIOSes out there. There are even BIOSes
        * that happily report success when you ask them to read from non-existent
        * harddisks. So, we set the buffer to known contents first, then try to
        * read. If the BIOS reports success but the buffer contents haven't
        * changed then we fail anyway */
    memset((PVOID) DISKREADBUFFER, 0xcd, 512);
    while (MachDiskReadLogicalSectors(0x80 + DiskCount, 0ULL, 1, (PVOID)DISKREADBUFFER))
    {
        Changed = FALSE;
        for (i = 0; ! Changed && i < 512; i++)
        {
            Changed = ((PUCHAR)DISKREADBUFFER)[i] != 0xcd;
        }
        if (! Changed)
        {
            DPRINTM(DPRINT_HWDETECT, "BIOS reports success for disk %d but data didn't change\n",
                      (int)DiskCount);
            break;
        }
        DiskCount++;
        memset((PVOID) DISKREADBUFFER, 0xcd, 512);
    }
    DiskReportError(TRUE);
    DPRINTM(DPRINT_HWDETECT, "BIOS reports %d harddisk%s\n",
              (int)DiskCount, (DiskCount == 1) ? "": "s");
    
    FldrCreateComponentKey(BusKey,
                           L"DiskController",
                           0,
                           ControllerClass,
                           DiskController,
                           &ControllerKey);
    DPRINTM(DPRINT_HWDETECT, "Created key: DiskController\\0\n");
    
    /* Set 'ComponentInformation' value */
    FldrSetComponentInformation(ControllerKey,
                                Output | Input | Removable,
                                0,
                                0xFFFFFFFF);
    
    DetectBiosFloppyController(BusKey, ControllerKey);
    
    /* Allocate resource descriptor */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
        sizeof(CM_INT13_DRIVE_PARAMETER) * DiskCount;
    PartialResourceList = MmHeapAlloc(Size);
    if (PartialResourceList == NULL)
    {
        DPRINTM(DPRINT_HWDETECT,
                  "Failed to allocate resource descriptor\n");
        return;
    }
    
    /* Initialize resource descriptor */
    memset(PartialResourceList, 0, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 1;
    PartialResourceList->PartialDescriptors[0].Type = CmResourceTypeDeviceSpecific;
    PartialResourceList->PartialDescriptors[0].ShareDisposition = 0;
    PartialResourceList->PartialDescriptors[0].Flags = 0;
    PartialResourceList->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
        sizeof(CM_INT13_DRIVE_PARAMETER) * DiskCount;
    
    /* Get harddisk Int13 geometry data */
    Int13Drives = (PVOID)(((ULONG_PTR)PartialResourceList) + sizeof(CM_PARTIAL_RESOURCE_LIST));
    for (i = 0; i < DiskCount; i++)
    {
        if (MachDiskGetDriveGeometry(0x80 + i, &Geometry))
        {
            Int13Drives[i].DriveSelect = 0x80 + i;
            Int13Drives[i].MaxCylinders = Geometry.Cylinders - 1;
            Int13Drives[i].SectorsPerTrack = Geometry.Sectors;
            Int13Drives[i].MaxHeads = Geometry.Heads - 1;
            Int13Drives[i].NumberDrives = DiskCount;
            
            DPRINTM(DPRINT_HWDETECT,
                      "Disk %x: %u Cylinders  %u Heads  %u Sectors  %u Bytes\n",
                      0x80 + i,
                      Geometry.Cylinders - 1,
                      Geometry.Heads -1,
                      Geometry.Sectors,
                      Geometry.BytesPerSector);
        }
    }
    
    /* Set 'Configuration Data' value */
    FldrSetConfigurationData(SystemKey, PartialResourceList, Size);
    MmHeapFree(PartialResourceList);
    
    /* Create and fill subkey for each harddisk */
    for (i = 0; i < DiskCount; i++)
    {
        /* Create disk key */
        FldrCreateComponentKey(ControllerKey,
                               L"DiskPeripheral",
                               i,
                               PeripheralClass,
                               DiskPeripheral,
                               &DiskKey);
        
        /* Set 'ComponentInformation' value */
        FldrSetComponentInformation(DiskKey,
                                    Output | Input,
                                    0,
                                    0xFFFFFFFF);
        
        /* Set disk values */
        SetHarddiskConfigurationData(DiskKey, 0x80 + i);
        SetHarddiskIdentifier(DiskKey, 0x80 + i);
    }
}

static VOID
InitializeSerialPort(PUCHAR Port,
		     ULONG LineControl)
{
  WRITE_PORT_UCHAR(Port + 3, 0x80);  /* set DLAB on   */
  WRITE_PORT_UCHAR(Port,     0x60);  /* speed LO byte */
  WRITE_PORT_UCHAR(Port + 1, 0);     /* speed HI byte */
  WRITE_PORT_UCHAR(Port + 3, LineControl);
  WRITE_PORT_UCHAR(Port + 1, 0);     /* set comm and DLAB to 0 */
  WRITE_PORT_UCHAR(Port + 4, 0x09);  /* DR int enable */
  READ_PORT_UCHAR(Port + 5);  /* clear error bits */
}


static ULONG
DetectSerialMouse(PUCHAR Port)
{
  CHAR Buffer[4];
  ULONG i;
  ULONG TimeOut;
  UCHAR LineControl;

  /* Shutdown mouse or something like that */
  LineControl = READ_PORT_UCHAR(Port + 4);
  WRITE_PORT_UCHAR(Port + 4, (LineControl & ~0x02) | 0x01);
  StallExecutionProcessor(100000);

  /*
   * Clear buffer
   * Maybe there is no serial port although BIOS reported one (this
   * is the case on Apple hardware), or the serial port is misbehaving,
   * therefore we must give up after some time.
   */
  TimeOut = 200;
  while (READ_PORT_UCHAR(Port + 5) & 0x01)
    {
      if (--TimeOut == 0)
        return MOUSE_TYPE_NONE;
      READ_PORT_UCHAR(Port);
    }

  /*
   * Send modem control with 'Data Terminal Ready', 'Request To Send' and
   * 'Output Line 2' message. This enables mouse to identify.
   */
  WRITE_PORT_UCHAR(Port + 4, 0x0b);

  /* Wait 10 milliseconds for the mouse getting ready */
  StallExecutionProcessor(10000);

  /* Read first four bytes, which contains Microsoft Mouse signs */
  TimeOut = 200;
  for (i = 0; i < 4; i++)
    {
      while (((READ_PORT_UCHAR(Port + 5) & 1) == 0) && (TimeOut > 0))
	{
	  StallExecutionProcessor(1000);
	  --TimeOut;
	  if (TimeOut == 0)
	    return MOUSE_TYPE_NONE;
	}
      Buffer[i] = READ_PORT_UCHAR(Port);
    }

  DPRINTM(DPRINT_HWDETECT,
	    "Mouse data: %x %x %x %x\n",
	    Buffer[0],Buffer[1],Buffer[2],Buffer[3]);

  /* Check that four bytes for signs */
  for (i = 0; i < 4; ++i)
    {
      if (Buffer[i] == 'B')
	{
	  /* Sign for Microsoft Ballpoint */
//	  DbgPrint("Microsoft Ballpoint device detected\n");
//	  DbgPrint("THIS DEVICE IS NOT SUPPORTED, YET\n");
	  return MOUSE_TYPE_NONE;
	}
      else if (Buffer[i] == 'M')
	{
	  /* Sign for Microsoft Mouse protocol followed by button specifier */
	  if (i == 3)
	    {
	      /* Overflow Error */
	      return MOUSE_TYPE_NONE;
	    }

	  switch (Buffer[i + 1])
	    {
	      case '3':
		DPRINTM(DPRINT_HWDETECT,
			  "Microsoft Mouse with 3-buttons detected\n");
		return MOUSE_TYPE_LOGITECH;

	      case 'Z':
		DPRINTM(DPRINT_HWDETECT,
			  "Microsoft Wheel Mouse detected\n");
		return MOUSE_TYPE_WHEELZ;

	      /* case '2': */
	      default:
		DPRINTM(DPRINT_HWDETECT,
			  "Microsoft Mouse with 2-buttons detected\n");
		return MOUSE_TYPE_MICROSOFT;
	    }
	}
    }

  return MOUSE_TYPE_NONE;
}


static ULONG
GetSerialMousePnpId(PUCHAR Port, char *Buffer)
{
  ULONG TimeOut;
  ULONG i = 0;
  char c;
  char x;

  WRITE_PORT_UCHAR(Port + 4, 0x09);

  /* Wait 10 milliseconds for the mouse getting ready */
  StallExecutionProcessor(10000);

  WRITE_PORT_UCHAR(Port + 4, 0x0b);

  StallExecutionProcessor(10000);

  for (;;)
    {
      TimeOut = 200;
      while (((READ_PORT_UCHAR(Port + 5) & 1) == 0) && (TimeOut > 0))
	{
	  StallExecutionProcessor(1000);
	  --TimeOut;
	  if (TimeOut == 0)
	    {
	      return 0;
	    }
	}

      c = READ_PORT_UCHAR(Port);
      if (c == 0x08 || c == 0x28)
	break;
    }

  Buffer[i++] = c;
  x = c + 1;

  for (;;)
    {
      TimeOut = 200;
      while (((READ_PORT_UCHAR(Port + 5) & 1) == 0) && (TimeOut > 0))
	{
	  StallExecutionProcessor(1000);
	  --TimeOut;
	  if (TimeOut == 0)
	    return 0;
	}
      c = READ_PORT_UCHAR(Port);
      Buffer[i++] = c;
      if (c == x)
	break;
      if (i >= 256)
	break;
    }

  return i;
}


static VOID
DetectSerialPointerPeripheral(PCONFIGURATION_COMPONENT_DATA ControllerKey,
			      PUCHAR Base)
{
  CM_PARTIAL_RESOURCE_LIST PartialResourceList;
  char Buffer[256];
  CHAR Identifier[256];
  PCONFIGURATION_COMPONENT_DATA PeripheralKey;
  ULONG MouseType;
  ULONG Length;
  ULONG i;
  ULONG j;
  ULONG k;

  DPRINTM(DPRINT_HWDETECT,
	    "DetectSerialPointerPeripheral()\n");

  Identifier[0] = 0;

  InitializeSerialPort(Base, 2);
  MouseType = DetectSerialMouse(Base);

  if (MouseType != MOUSE_TYPE_NONE)
    {
      Length = GetSerialMousePnpId(Base, Buffer);
      DPRINTM(DPRINT_HWDETECT,
		"PnP ID length: %u\n",
		Length);

      if (Length != 0)
	{
	  /* Convert PnP sting to ASCII */
	  if (Buffer[0] == 0x08)
	    {
	      for (i = 0; i < Length; i++)
		Buffer[i] += 0x20;
	    }
	  Buffer[Length] = 0;

	  DPRINTM(DPRINT_HWDETECT,
		    "PnP ID string: %s\n",
		    Buffer);

	  /* Copy PnpId string */
          for (i = 0; i < 7; i++)
            {
              Identifier[i] = Buffer[3+i];
            }
          memcpy(&Identifier[7],
		 L" - ",
		 3 * sizeof(WCHAR));

	  /* Skip device serial number */
	  i = 10;
	  if (Buffer[i] == '\\')
	    {
	      for (j = ++i; i < Length; ++i)
		{
		  if (Buffer[i] == '\\')
		    break;
		}
	      if (i >= Length)
		i -= 3;
	    }

	  /* Skip PnP class */
	  if (Buffer[i] == '\\')
	    {
	      for (j = ++i; i < Length; ++i)
		{
		  if (Buffer[i] == '\\')
		    break;
		}

	      if (i >= Length)
	        i -= 3;
	    }

	  /* Skip compatible PnP Id */
	  if (Buffer[i] == '\\')
	    {
	      for (j = ++i; i < Length; ++i)
		{
		  if (Buffer[i] == '\\')
		    break;
		}
	      if (Buffer[j] == '*')
		++j;
	      if (i >= Length)
		i -= 3;
	    }

	  /* Get product description */
	  if (Buffer[i] == '\\')
	    {
	      for (j = ++i; i < Length; ++i)
		{
		  if (Buffer[i] == ';')
		    break;
		}
	      if (i >= Length)
		i -= 3;
	      if (i > j + 1)
		{
                  for (k = 0; k < i - j; k++)
                    {
                      Identifier[k + 10] = Buffer[k + j];
                    }
		  Identifier[10 + (i-j)] = 0;
		}
	    }

	  DPRINTM(DPRINT_HWDETECT,
		    "Identifier string: %s\n",
		    Identifier);
	}

      if (Length == 0 || strlen(Identifier) < 11)
	{
	  switch (MouseType)
	    {
	      case MOUSE_TYPE_LOGITECH:
		strcpy (Identifier,
			"LOGITECH SERIAL MOUSE");
		break;

	      case MOUSE_TYPE_WHEELZ:
		strcpy (Identifier,
			"MICROSOFT SERIAL MOUSE WITH WHEEL");
		break;

	      case MOUSE_TYPE_MICROSOFT:
	      default:
		strcpy (Identifier,
			"MICROSOFT SERIAL MOUSE");
		break;
	    }
	}

      /* Create 'PointerPeripheral' key */
      FldrCreateComponentKey(ControllerKey,
                             L"PointerPeripheral",
                             0,
                             PeripheralClass,
                             PointerPeripheral,
                             &PeripheralKey);
      DPRINTM(DPRINT_HWDETECT,
		"Created key: PointerPeripheral\\0\n");

      /* Set 'ComponentInformation' value */
      FldrSetComponentInformation(PeripheralKey,
                                  Input,
                                  0,
                                  0xFFFFFFFF);

      /* Set 'Configuration Data' value */
      memset(&PartialResourceList, 0, sizeof(CM_PARTIAL_RESOURCE_LIST));
      PartialResourceList.Version = 1;
      PartialResourceList.Revision = 1;
      PartialResourceList.Count = 0;

      FldrSetConfigurationData(PeripheralKey,
                               &PartialResourceList,
                               sizeof(CM_PARTIAL_RESOURCE_LIST) -
                               sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

      /* Set 'Identifier' value */
      FldrSetIdentifier(PeripheralKey, Identifier);
    }
}


static VOID
DetectSerialPorts(PCONFIGURATION_COMPONENT_DATA BusKey)
{
  PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  PCM_SERIAL_DEVICE_DATA SerialDeviceData;
  ULONG Irq[4] = {4, 3, 4, 3};
  ULONG Base;
  CHAR Buffer[80];
  PUSHORT BasePtr;
  ULONG ControllerNumber = 0;
  PCONFIGURATION_COMPONENT_DATA ControllerKey;
  ULONG i;
  ULONG Size;

  DPRINTM(DPRINT_HWDETECT, "DetectSerialPorts()\n");

  ControllerNumber = 0;
  BasePtr = (PUSHORT)0x400;
  for (i = 0; i < 4; i++, BasePtr++)
    {
      Base = (ULONG)*BasePtr;
      if (Base == 0)
        continue;

      DPRINTM(DPRINT_HWDETECT,
		"Found COM%u port at 0x%x\n",
		i + 1,
		Base);

      /* Create controller key */
      FldrCreateComponentKey(BusKey,
                             L"SerialController",
                             ControllerNumber,
                             ControllerClass,
                             SerialController,
                             &ControllerKey);

      /* Set 'ComponentInformation' value */
      FldrSetComponentInformation(ControllerKey,
                                  Output | Input | ConsoleIn | ConsoleOut,
                                  ControllerNumber,
                                  0xFFFFFFFF);

      /* Build full device descriptor */
      Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
	     2 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) +
	     sizeof(CM_SERIAL_DEVICE_DATA);
      PartialResourceList = MmHeapAlloc(Size);
      if (PartialResourceList == NULL)
	{
	  DPRINTM(DPRINT_HWDETECT,
		    "Failed to allocate resource descriptor\n");
	  continue;
	}
      memset(PartialResourceList, 0, Size);

      /* Initialize resource descriptor */
      PartialResourceList->Version = 1;
      PartialResourceList->Revision = 1;
      PartialResourceList->Count = 3;

      /* Set IO Port */
      PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
      PartialDescriptor->Type = CmResourceTypePort;
      PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
      PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
      PartialDescriptor->u.Port.Start.LowPart = Base;
      PartialDescriptor->u.Port.Start.HighPart = 0x0;
      PartialDescriptor->u.Port.Length = 7;

      /* Set Interrupt */
      PartialDescriptor = &PartialResourceList->PartialDescriptors[1];
      PartialDescriptor->Type = CmResourceTypeInterrupt;
      PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
      PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
      PartialDescriptor->u.Interrupt.Level = Irq[i];
      PartialDescriptor->u.Interrupt.Vector = 0;
      PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

      /* Set serial data (device specific) */
      PartialDescriptor = &PartialResourceList->PartialDescriptors[2];
      PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
      PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
      PartialDescriptor->Flags = 0;
      PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(CM_SERIAL_DEVICE_DATA);

      SerialDeviceData =
	(PCM_SERIAL_DEVICE_DATA)&PartialResourceList->PartialDescriptors[3];
      SerialDeviceData->BaudClock = 1843200; /* UART Clock frequency (Hertz) */

      /* Set 'Configuration Data' value */
      FldrSetConfigurationData(ControllerKey, PartialResourceList, Size);
      MmHeapFree(PartialResourceList);

      /* Set 'Identifier' value */
      sprintf(Buffer, "COM%ld", i + 1);
      FldrSetIdentifier(ControllerKey, Buffer);
      DPRINTM(DPRINT_HWDETECT,
		"Created value: Identifier %s\n",
		Buffer);

      if (!Rs232PortInUse(Base))
        {
          /* Detect serial mouse */
          DetectSerialPointerPeripheral(ControllerKey, UlongToPtr(Base));
        }

      ControllerNumber++;
    }
}


static VOID
DetectParallelPorts(PCONFIGURATION_COMPONENT_DATA BusKey)
{
  PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  ULONG Irq[3] = {7, 5, (ULONG)-1};
  CHAR Buffer[80];
  PCONFIGURATION_COMPONENT_DATA ControllerKey;
  PUSHORT BasePtr;
  ULONG Base;
  ULONG ControllerNumber;
  ULONG i;
  ULONG Size;

  DPRINTM(DPRINT_HWDETECT, "DetectParallelPorts() called\n");

  ControllerNumber = 0;
  BasePtr = (PUSHORT)0x408;
  for (i = 0; i < 3; i++, BasePtr++)
    {
      Base = (ULONG)*BasePtr;
      if (Base == 0)
        continue;

      DPRINTM(DPRINT_HWDETECT,
		"Parallel port %u: %x\n",
		ControllerNumber,
		Base);

      /* Create controller key */
      FldrCreateComponentKey(BusKey,
                             L"ParallelController",
                             ControllerNumber,
                             ControllerClass,
                             ParallelController,
                             &ControllerKey);

      /* Set 'ComponentInformation' value */
      FldrSetComponentInformation(ControllerKey,
                                  Output,
                                  ControllerNumber,
                                  0xFFFFFFFF);

      /* Build full device descriptor */
      Size = sizeof(CM_PARTIAL_RESOURCE_LIST);
      if (Irq[i] != (ULONG)-1)
	Size += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

      PartialResourceList = MmHeapAlloc(Size);
      if (PartialResourceList == NULL)
	{
	  DPRINTM(DPRINT_HWDETECT,
		    "Failed to allocate resource descriptor\n");
	  continue;
	}
      memset(PartialResourceList, 0, Size);

      /* Initialize resource descriptor */
      PartialResourceList->Version = 1;
      PartialResourceList->Revision = 1;
      PartialResourceList->Count = (Irq[i] != (ULONG)-1) ? 2 : 1;

      /* Set IO Port */
      PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
      PartialDescriptor->Type = CmResourceTypePort;
      PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
      PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
      PartialDescriptor->u.Port.Start.LowPart = Base;
      PartialDescriptor->u.Port.Start.HighPart = 0x0;
      PartialDescriptor->u.Port.Length = 3;

      /* Set Interrupt */
      if (Irq[i] != (ULONG)-1)
	{
	  PartialDescriptor = &PartialResourceList->PartialDescriptors[1];
	  PartialDescriptor->Type = CmResourceTypeInterrupt;
	  PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
	  PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
	  PartialDescriptor->u.Interrupt.Level = Irq[i];
	  PartialDescriptor->u.Interrupt.Vector = 0;
	  PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;
	}

      /* Set 'Configuration Data' value */
      FldrSetConfigurationData(ControllerKey, PartialResourceList, Size);
      MmHeapFree(PartialResourceList);

      /* Set 'Identifier' value */
      sprintf(Buffer, "PARALLEL%ld", i + 1);
      FldrSetIdentifier(ControllerKey, Buffer);
      DPRINTM(DPRINT_HWDETECT,
		"Created value: Identifier %s\n",
		Buffer);

      ControllerNumber++;
    }

  DPRINTM(DPRINT_HWDETECT, "DetectParallelPorts() done\n");
}


static BOOLEAN
DetectKeyboardDevice(VOID)
{
  UCHAR Status;
  UCHAR Scancode;
  ULONG Loops;
  BOOLEAN Result = TRUE;

  /* Identify device */
  WRITE_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA,
		   0xF2);

  /* Wait for reply */
  for (Loops = 0; Loops < 100; Loops++)
    {
      StallExecutionProcessor(10000);
      Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
      if ((Status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) != 0)
        break;
    }

  if ((Status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) == 0)
    {
      /* PC/XT keyboard or no keyboard */
      Result = FALSE;
    }

  Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
  if (Scancode != 0xFA)
    {
      /* No ACK received */
      Result = FALSE;
    }

  StallExecutionProcessor(10000);

  Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
  if ((Status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) == 0)
    {
      /* Found AT keyboard */
      return Result;
    }

  Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
  if (Scancode != 0xAB)
    {
      /* No 0xAB received */
      Result = FALSE;
    }

  StallExecutionProcessor(10000);

  Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
  if ((Status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) == 0)
    {
      /* No byte in buffer */
      Result = FALSE;
    }

  Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
  if (Scancode != 0x41)
    {
      /* No 0x41 received */
      Result = FALSE;
    }

  /* Found MF-II keyboard */
  return Result;
}


static VOID
DetectKeyboardPeripheral(PCONFIGURATION_COMPONENT_DATA ControllerKey)
{
  PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  PCM_KEYBOARD_DEVICE_DATA KeyboardData;
  PCONFIGURATION_COMPONENT_DATA PeripheralKey;
  ULONG Size;

  /* HACK: don't call DetectKeyboardDevice() as it fails in Qemu 0.8.2 */
  if (TRUE || DetectKeyboardDevice())
  {
      /* Create controller key */
      FldrCreateComponentKey(ControllerKey,
                             L"KeyboardPeripheral",
                             0,
                             PeripheralClass,
                             KeyboardPeripheral,
                             &PeripheralKey);
    DPRINTM(DPRINT_HWDETECT, "Created key: KeyboardPeripheral\\0\n");

    /* Set 'ComponentInformation' value */
    FldrSetComponentInformation(PeripheralKey,
                                Input | ConsoleIn,
                                0,
                                0xFFFFFFFF);

    /* Set 'Configuration Data' value */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
	   sizeof(CM_KEYBOARD_DEVICE_DATA);
    PartialResourceList = MmHeapAlloc(Size);
    if (PartialResourceList == NULL)
    {
      DPRINTM(DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n");
      return;
    }

    /* Initialize resource descriptor */
    memset(PartialResourceList, 0, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 1;

    PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
    PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(CM_KEYBOARD_DEVICE_DATA);

    KeyboardData = (PCM_KEYBOARD_DEVICE_DATA)(PartialDescriptor + 1);
    KeyboardData->Version = 1;
    KeyboardData->Revision = 1;
    KeyboardData->Type = 4;
    KeyboardData->Subtype = 0;
    KeyboardData->KeyboardFlags = 0x20;

    /* Set 'Configuration Data' value */
    FldrSetConfigurationData(PeripheralKey, PartialResourceList, Size);
    MmHeapFree(PartialResourceList);

    /* Set 'Identifier' value */
    FldrSetIdentifier(PeripheralKey, "PCAT_ENHANCED");
  }
}


static VOID
DetectKeyboardController(PCONFIGURATION_COMPONENT_DATA BusKey)
{
  PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  PCONFIGURATION_COMPONENT_DATA ControllerKey;
  ULONG Size;

  /* Create controller key */
  FldrCreateComponentKey(BusKey,
                         L"KeyboardController",
                         0,
                         ControllerClass,
                         KeyboardController,
                         &ControllerKey);
  DPRINTM(DPRINT_HWDETECT, "Created key: KeyboardController\\0\n");

  /* Set 'ComponentInformation' value */
  FldrSetComponentInformation(ControllerKey,
                              Input | ConsoleIn,
                              0,
                              0xFFFFFFFF);

  /* Set 'Configuration Data' value */
  Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
	  2 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
  PartialResourceList = MmHeapAlloc(Size);
  if (PartialResourceList == NULL)
    {
      DPRINTM(DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n");
      return;
    }

  /* Initialize resource descriptor */
  memset(PartialResourceList, 0, Size);
  PartialResourceList->Version = 1;
  PartialResourceList->Revision = 1;
  PartialResourceList->Count = 3;

  /* Set Interrupt */
  PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
  PartialDescriptor->Type = CmResourceTypeInterrupt;
  PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
  PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
  PartialDescriptor->u.Interrupt.Level = 1;
  PartialDescriptor->u.Interrupt.Vector = 0;
  PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

  /* Set IO Port 0x60 */
  PartialDescriptor = &PartialResourceList->PartialDescriptors[1];
  PartialDescriptor->Type = CmResourceTypePort;
  PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
  PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
  PartialDescriptor->u.Port.Start.LowPart = 0x60;
  PartialDescriptor->u.Port.Start.HighPart = 0x0;
  PartialDescriptor->u.Port.Length = 1;

  /* Set IO Port 0x64 */
  PartialDescriptor = &PartialResourceList->PartialDescriptors[2];
  PartialDescriptor->Type = CmResourceTypePort;
  PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
  PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
  PartialDescriptor->u.Port.Start.LowPart = 0x64;
  PartialDescriptor->u.Port.Start.HighPart = 0x0;
  PartialDescriptor->u.Port.Length = 1;

  /* Set 'Configuration Data' value */
  FldrSetConfigurationData(ControllerKey, PartialResourceList, Size);
  MmHeapFree(PartialResourceList);
 
  DetectKeyboardPeripheral(ControllerKey);
}


static VOID
PS2ControllerWait(VOID)
{
  ULONG Timeout;
  UCHAR Status;

  for (Timeout = 0; Timeout < CONTROLLER_TIMEOUT; Timeout++)
    {
      Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
      if ((Status & CONTROLLER_STATUS_INPUT_BUFFER_FULL) == 0)
	return;

      /* Sleep for one millisecond */
      StallExecutionProcessor(1000);
    }
}


static BOOLEAN
DetectPS2AuxPort(VOID)
{
#if 1
  /* Current detection is too unreliable. Just do as if
   * the PS/2 aux port is always present
   */
   return TRUE;
#else
  ULONG Loops;
  UCHAR Status;

  /* Put the value 0x5A in the output buffer using the
   * "WriteAuxiliary Device Output Buffer" command (0xD3).
   * Poll the Status Register for a while to see if the value really turns up
   * in the Data Register. If the KEYBOARD_STATUS_MOUSE_OBF bit is also set
   * to 1 in the Status Register, we assume this controller has an
   *  Auxiliary Port (a.k.a. Mouse Port).
   */
  PS2ControllerWait();
  WRITE_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_CONTROL,
		   CONTROLLER_COMMAND_WRITE_MOUSE_OUTPUT_BUFFER);
  PS2ControllerWait();

  /* 0x5A is a random dummy value */
  WRITE_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA,
		   0x5A);

  for (Loops = 0; Loops < 10; Loops++)
    {
      StallExecutionProcessor(10000);
      Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
      if ((Status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) != 0)
        break;
    }

  READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);

  return (Status & CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL);
#endif
}


static BOOLEAN
DetectPS2AuxDevice(VOID)
{
  UCHAR Scancode;
  UCHAR Status;
  ULONG Loops;
  BOOLEAN Result = TRUE;

  PS2ControllerWait();
  WRITE_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_CONTROL,
		   CONTROLLER_COMMAND_WRITE_MOUSE);
  PS2ControllerWait();

  /* Identify device */
  WRITE_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA,
		   0xF2);

  /* Wait for reply */
  for (Loops = 0; Loops < 100; Loops++)
    {
      StallExecutionProcessor(10000);
      Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
      if ((Status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) != 0)
        break;
    }

  Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
  if ((Status & CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL) == 0)
    Result = FALSE;

  Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
  if (Scancode != 0xFA)
    Result = FALSE;

  StallExecutionProcessor(10000);

  Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
  if ((Status & CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL) == 0)
    Result = FALSE;

  Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
  if (Scancode != 0x00)
    Result = FALSE;

  return Result;
}


static VOID
DetectPS2Mouse(PCONFIGURATION_COMPONENT_DATA BusKey)
{
  CM_PARTIAL_RESOURCE_LIST PartialResourceList;
  PCONFIGURATION_COMPONENT_DATA ControllerKey;
  PCONFIGURATION_COMPONENT_DATA PeripheralKey;

  if (DetectPS2AuxPort())
    {
      DPRINTM(DPRINT_HWDETECT, "Detected PS2 port\n");

      /* Create controller key */
      FldrCreateComponentKey(BusKey,
                             L"PointerController",
                             0,
                             ControllerClass,
                             PointerController,
                             &ControllerKey);
      DPRINTM(DPRINT_HWDETECT, "Created key: PointerController\\0\n");

      /* Set 'ComponentInformation' value */
      FldrSetComponentInformation(ControllerKey,
                                  Input,
                                  0,
                                  0xFFFFFFFF);

      memset(&PartialResourceList, 0, sizeof(CM_PARTIAL_RESOURCE_LIST));

      /* Initialize resource descriptor */
      PartialResourceList.Version = 1;
      PartialResourceList.Revision = 1;
      PartialResourceList.Count = 1;

      /* Set Interrupt */
      PartialResourceList.PartialDescriptors[0].Type = CmResourceTypeInterrupt;
      PartialResourceList.PartialDescriptors[0].ShareDisposition = CmResourceShareUndetermined;
      PartialResourceList.PartialDescriptors[0].Flags = CM_RESOURCE_INTERRUPT_LATCHED;
      PartialResourceList.PartialDescriptors[0].u.Interrupt.Level = 12;
      PartialResourceList.PartialDescriptors[0].u.Interrupt.Vector = 0;
      PartialResourceList.PartialDescriptors[0].u.Interrupt.Affinity = 0xFFFFFFFF;

      /* Set 'Configuration Data' value */
      FldrSetConfigurationData(ControllerKey,
                               &PartialResourceList,
                               sizeof(CM_PARTIAL_RESOURCE_LIST));

      if (DetectPS2AuxDevice())
	{
	  DPRINTM(DPRINT_HWDETECT, "Detected PS2 mouse\n");

          /* Create peripheral key */
          FldrCreateComponentKey(ControllerKey,
                                 L"PointerPeripheral",
                                 0,
                                 ControllerClass,
                                 PointerPeripheral,
                                 &PeripheralKey);
	  DPRINTM(DPRINT_HWDETECT, "Created key: PointerPeripheral\\0\n");

	  /* Set 'ComponentInformation' value */
	  FldrSetComponentInformation(PeripheralKey,
                                  Input,
                                  0,
                                  0xFFFFFFFF);

	  /* Initialize resource descriptor */
	  memset(&PartialResourceList, 0, sizeof(CM_PARTIAL_RESOURCE_LIST));
	  PartialResourceList.Version = 1;
	  PartialResourceList.Revision = 1;
	  PartialResourceList.Count = 0;

	  /* Set 'Configuration Data' value */
      FldrSetConfigurationData(PeripheralKey,
                               &PartialResourceList,
                               sizeof(CM_PARTIAL_RESOURCE_LIST) -
                               sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

	  /* Set 'Identifier' value */
      FldrSetIdentifier(PeripheralKey, "MICROSOFT PS2 MOUSE");
    }
  }
}


static VOID
DetectDisplayController(PCONFIGURATION_COMPONENT_DATA BusKey)
{
  CHAR Buffer[80];
  PCONFIGURATION_COMPONENT_DATA ControllerKey;
  USHORT VesaVersion;

  FldrCreateComponentKey(BusKey,
                         L"DisplayController",
                         0,
                         ControllerClass,
                         DisplayController,
                         &ControllerKey);
  DPRINTM(DPRINT_HWDETECT, "Created key: DisplayController\\0\n");

  /* Set 'ComponentInformation' value */
  FldrSetComponentInformation(ControllerKey,
                              0x00,
                              0,
                              0xFFFFFFFF);

  /* FIXME: Set 'ComponentInformation' value */

  VesaVersion = BiosIsVesaSupported();
  if (VesaVersion != 0)
    {
      DPRINTM(DPRINT_HWDETECT,
		"VESA version %c.%c\n",
		(VesaVersion >> 8) + '0',
		(VesaVersion & 0xFF) + '0');
    }
  else
    {
      DPRINTM(DPRINT_HWDETECT,
		"VESA not supported\n");
    }

  if (VesaVersion >= 0x0200)
    {
      strcpy(Buffer,
             "VBE Display");
    }
  else
    {
      strcpy(Buffer,
             "VGA Display");
    }

  /* Set 'Identifier' value */
  FldrSetIdentifier(ControllerKey, Buffer);

  /* FIXME: Add display peripheral (monitor) data */
}


static VOID
DetectIsaBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
  PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
  PCONFIGURATION_COMPONENT_DATA BusKey;
  ULONG Size;

  /* Create new bus key */
  FldrCreateComponentKey(SystemKey,
                         L"MultifunctionAdapter",
                         *BusNumber,
                         AdapterClass,
                         MultiFunctionAdapter,
                         &BusKey);

  /* Set 'Component Information' value similar to my NT4 box */
  FldrSetComponentInformation(BusKey,
                              0x0,
                              0x0,
                              0xFFFFFFFF);

  /* Increment bus number */
  (*BusNumber)++;

  /* Set 'Identifier' value */
  FldrSetIdentifier(BusKey, "ISA");

  /* Set 'Configuration Data' value */
  Size = sizeof(CM_PARTIAL_RESOURCE_LIST) -
	 sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
  PartialResourceList = MmHeapAlloc(Size);
  if (PartialResourceList == NULL)
    {
      DPRINTM(DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n");
      return;
    }

  /* Initialize resource descriptor */
  memset(PartialResourceList, 0, Size);
  PartialResourceList->Version = 1;
  PartialResourceList->Revision = 1;
  PartialResourceList->Count = 0;

  /* Set 'Configuration Data' value */
  FldrSetConfigurationData(BusKey, PartialResourceList, Size);
  MmHeapFree(PartialResourceList);

  /* Detect ISA/BIOS devices */
  DetectBiosDisks(SystemKey, BusKey);

  DetectSerialPorts(BusKey);

  DetectParallelPorts(BusKey);

  DetectKeyboardController(BusKey);

  DetectPS2Mouse(BusKey);

  DetectDisplayController(BusKey);

  /* FIXME: Detect more ISA devices */
}


PCONFIGURATION_COMPONENT_DATA
PcHwDetect(VOID)
{
  PCONFIGURATION_COMPONENT_DATA SystemKey;
  ULONG BusNumber = 0;

  DPRINTM(DPRINT_HWDETECT, "DetectHardware()\n");

  /* Create the 'System' key */
  FldrCreateSystemKey(&SystemKey);

  /* Set empty component information */
  FldrSetComponentInformation(SystemKey,
                              0x0,
                              0x0,
                              0xFFFFFFFF);
  
  /* Detect buses */
  DetectPciBios(SystemKey, &BusNumber);
  DetectApmBios(SystemKey, &BusNumber);
  DetectPnpBios(SystemKey, &BusNumber);
  DetectIsaBios(SystemKey, &BusNumber);
  DetectAcpiBios(SystemKey, &BusNumber);
  
  DPRINTM(DPRINT_HWDETECT, "DetectHardware() Done\n");

  return SystemKey;
}

/* EOF */
