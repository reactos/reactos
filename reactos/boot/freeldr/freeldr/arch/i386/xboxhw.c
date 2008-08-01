/* $Id$
 *
 *  FreeLoader
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

static CHAR Hex[] = "0123456789ABCDEF";
//static unsigned int delay_count = 1;

extern ULONG reactos_disk_count;
extern ARC_DISK_SIGNATURE reactos_arc_disk_info[];
extern char reactos_arc_strings[32][256];

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
      DbgPrint((DPRINT_HWDETECT,
		"Failed to allocate a full resource descriptor\n"));
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

  if(MachDiskGetDriveGeometry(DriveNumber, &Geometry))
    {
      DiskGeometry->BytesPerSector = Geometry.BytesPerSector;
      DiskGeometry->NumberOfCylinders = Geometry.Cylinders;
      DiskGeometry->SectorsPerTrack = Geometry.Sectors;
      DiskGeometry->NumberOfHeads = Geometry.Heads;
    }
  else
    {
      DbgPrint((DPRINT_HWDETECT, "Reading disk geometry failed\n"));
      MmHeapFree(PartialResourceList);
      return;
    }
  DbgPrint((DPRINT_HWDETECT,
	   "Disk %x: %u Cylinders  %u Heads  %u Sectors  %u Bytes\n",
	   DriveNumber,
	   DiskGeometry->NumberOfCylinders,
	   DiskGeometry->NumberOfHeads,
	   DiskGeometry->SectorsPerTrack,
	   DiskGeometry->BytesPerSector));

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
      DbgPrint((DPRINT_HWDETECT, "Reading MBR failed\n"));
      return;
    }

  Buffer = (ULONG*)DISKREADBUFFER;
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
  DbgPrint((DPRINT_HWDETECT, "Identifier: %s\n", Identifier));

  /* Set identifier */
  FldrSetIdentifier(DiskKey, Identifier);
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
            DbgPrint((DPRINT_HWDETECT, "BIOS reports success for disk %d but data didn't change\n",
                      (int)DiskCount));
            break;
        }
        DiskCount++;
        memset((PVOID) DISKREADBUFFER, 0xcd, 512);
    }
    DiskReportError(TRUE);
    DbgPrint((DPRINT_HWDETECT, "BIOS reports %d harddisk%s\n",
              (int)DiskCount, (DiskCount == 1) ? "": "s"));
    
    FldrCreateComponentKey(BusKey,
                           L"DiskController",
                           0,
                           ControllerClass,
                           DiskController,
                           &ControllerKey);
    DbgPrint((DPRINT_HWDETECT, "Created key: DiskController\\0\n"));
    
    /* Set 'ComponentInformation' value */
    FldrSetComponentInformation(ControllerKey,
                                Output | Input | Removable,
                                0,
                                0xFFFFFFFF);
    
    //DetectBiosFloppyController(BusKey, ControllerKey);
    
    /* Allocate resource descriptor */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
        sizeof(CM_INT13_DRIVE_PARAMETER) * DiskCount;
    PartialResourceList = MmHeapAlloc(Size);
    if (PartialResourceList == NULL)
    {
        DbgPrint((DPRINT_HWDETECT,
                  "Failed to allocate resource descriptor\n"));
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
      DbgPrint((DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n"));
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


  /* FIXME: Detect more ISA devices */
}

PCONFIGURATION_COMPONENT_DATA
XboxHwDetect(VOID)
{
  PCONFIGURATION_COMPONENT_DATA SystemKey;
  ULONG BusNumber = 0;

  DbgPrint((DPRINT_HWDETECT, "DetectHardware()\n"));

  /* Create the 'System' key */
  FldrCreateSystemKey(&SystemKey);

  /* Set empty component information */
  FldrSetComponentInformation(SystemKey,
                              0x0,
                              0x0,
                              0xFFFFFFFF);

  /* TODO: Build actual xbox's hardware configuration tree */
  DetectIsaBios(SystemKey, &BusNumber);

  DbgPrint((DPRINT_HWDETECT, "DetectHardware() Done\n"));
  return SystemKey;
}

/* EOF */
