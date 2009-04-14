/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2003, 2004, 2005 ReactOS Team
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
/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/partlist.c
 * PURPOSE:         Partition list functions
 * PROGRAMMER:      Eric Kohl
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

static VOID
GetDriverName (PDISKENTRY DiskEntry)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  WCHAR KeyName[32];
  NTSTATUS Status;

  RtlInitUnicodeString (&DiskEntry->DriverName,
                        NULL);

  swprintf (KeyName,
            L"\\Scsi\\Scsi Port %lu",
            DiskEntry->Port);

  RtlZeroMemory (&QueryTable,
                 sizeof(QueryTable));

  QueryTable[0].Name = L"Driver";
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[0].EntryContext = &DiskEntry->DriverName;

  Status = RtlQueryRegistryValues (RTL_REGISTRY_DEVICEMAP,
                                   KeyName,
                                   QueryTable,
                                   NULL,
                                   NULL);
  if (!NT_SUCCESS (Status))
  {
    DPRINT1 ("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
  }
}


static VOID
AssignDriverLetters (PPARTLIST List)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  PLIST_ENTRY Entry1;
  //PLIST_ENTRY Entry2;
  CHAR Letter;
  UCHAR i;

  Letter = 'C';

  /* Assign drive letters to primary partitions */
  Entry1 = List->DiskListHead.Flink;
  while (Entry1 != &List->DiskListHead)
  {
    DiskEntry = CONTAINING_RECORD (Entry1, DISKENTRY, ListEntry);

    if (!IsListEmpty (&DiskEntry->PartListHead))
    {
      PartEntry = CONTAINING_RECORD (DiskEntry->PartListHead.Flink,
                                     PARTENTRY,
                                     ListEntry);

      for (i=0; i<3; i++)
        PartEntry->DriveLetter[i] = 0;

      if (PartEntry->Unpartitioned == FALSE)
      {
        for (i=0; i<3; i++)
        {
          if (IsContainerPartition (PartEntry->PartInfo[i].PartitionType))
            continue;

          if (IsRecognizedPartition (PartEntry->PartInfo[i].PartitionType) ||
             (PartEntry->PartInfo[i].PartitionType == PARTITION_ENTRY_UNUSED &&
             PartEntry->PartInfo[i].PartitionLength.QuadPart != 0LL))
          {
            if (Letter <= 'Z')
            {
              PartEntry->DriveLetter[i] = Letter;
              Letter++;
            }
          }
        }
      }
    }

    Entry1 = Entry1->Flink;
  }

  /* Assign drive letters to logical drives */
#if 0
  Entry1 = List->DiskListHead.Flink;
  while (Entry1 != &List->DiskListHead)
  {
    DiskEntry = CONTAINING_RECORD (Entry1, DISKENTRY, ListEntry);

    Entry2 = DiskEntry->PartListHead.Flink;
    if (Entry2 != &DiskEntry->PartListHead)
    {
      Entry2 = Entry2->Flink;
      while (Entry2 != &DiskEntry->PartListHead)
      {
        PartEntry = CONTAINING_RECORD (Entry2,
                                       PARTENTRY,
                                       ListEntry);

        PartEntry->DriveLetter = 0;

        if (PartEntry->Unpartitioned == FALSE &&
            !IsContainerPartition (PartEntry->PartInfo[0].PartitionType))
        {
          if (IsRecognizedPartition (PartEntry->PartInfo[0].PartitionType) ||
             (PartEntry->PartInfo[0].PartitionType == PARTITION_ENTRY_UNUSED &&
              PartEntry->PartInfo[0].PartitionLength.QuadPart != 0LL))
          {
            if (Letter <= 'Z')
            {
              PartEntry->DriveLetter = Letter;
              Letter++;
            }
          }
        }

        Entry2 = Entry2->Flink;
      }
    }

    Entry1 = Entry1->Flink;
  }
#endif
}


static VOID
UpdatePartitionNumbers (PDISKENTRY DiskEntry)
{
  PPARTENTRY PartEntry;
  PLIST_ENTRY Entry;
  ULONG PartNumber;
  ULONG i;

  PartNumber = 1;
  Entry = DiskEntry->PartListHead.Flink;
  while (Entry != &DiskEntry->PartListHead)
  {
    PartEntry = CONTAINING_RECORD (Entry,
                                   PARTENTRY,
                                   ListEntry);

    if (PartEntry->Unpartitioned == TRUE)
    {
      for (i = 0; i < 4; i++)
      {
        PartEntry->PartInfo[i].PartitionNumber = 0;
      }
    }
    else
    {
      for (i = 0; i < 4; i++)
      {
        if (IsContainerPartition (PartEntry->PartInfo[i].PartitionType))
        {
          PartEntry->PartInfo[i].PartitionNumber = 0;
        }
        else if (PartEntry->PartInfo[i].PartitionType == PARTITION_ENTRY_UNUSED &&
                 PartEntry->PartInfo[i].PartitionLength.QuadPart == 0ULL)
        {
          PartEntry->PartInfo[i].PartitionNumber = 0;
        }
        else
        {
          PartEntry->PartInfo[i].PartitionNumber = PartNumber;
          PartNumber++;
        }
      }
    }

    Entry = Entry->Flink;
  }
}


static VOID
AddPartitionToList (ULONG DiskNumber,
                    PDISKENTRY DiskEntry,
                    DRIVE_LAYOUT_INFORMATION *LayoutBuffer)
{
  PPARTENTRY PartEntry;
  ULONG i;
  ULONG j;

  for (i = 0; i < LayoutBuffer->PartitionCount; i += 4)
  {
    for (j = 0; j < 4; j++)
    {
      if (LayoutBuffer->PartitionEntry[i+j].PartitionType != PARTITION_ENTRY_UNUSED ||
          LayoutBuffer->PartitionEntry[i+j].PartitionLength.QuadPart != 0ULL)
      {
        break;
      }
    }
    if (j >= 4)
    {
      continue;
    }

    PartEntry = (PPARTENTRY)RtlAllocateHeap (ProcessHeap,
                 0,
                 sizeof(PARTENTRY));
    if (PartEntry == NULL)
    {
      return;
    }

    RtlZeroMemory (PartEntry,
                   sizeof(PARTENTRY));

    PartEntry->Unpartitioned = FALSE;

    for (j = 0; j < 4; j++)
    {
      RtlCopyMemory (&PartEntry->PartInfo[j],
                     &LayoutBuffer->PartitionEntry[i+j],
                     sizeof(PARTITION_INFORMATION));
    }

    if (IsContainerPartition(PartEntry->PartInfo[0].PartitionType))
    {
      PartEntry->FormatState = Unformatted;
    }
    else if ((PartEntry->PartInfo[0].PartitionType == PARTITION_FAT_12) ||
            (PartEntry->PartInfo[0].PartitionType == PARTITION_FAT_16) ||
            (PartEntry->PartInfo[0].PartitionType == PARTITION_HUGE) ||
            (PartEntry->PartInfo[0].PartitionType == PARTITION_XINT13) ||
            (PartEntry->PartInfo[0].PartitionType == PARTITION_FAT32) ||
            (PartEntry->PartInfo[0].PartitionType == PARTITION_FAT32_XINT13))
    {
#if 0
      if (CheckFatFormat())
      {
        PartEntry->FormatState = Preformatted;
      }
      else
      {
        PartEntry->FormatState = Unformatted;
      }
#endif
      PartEntry->FormatState = Preformatted;
    }
    else if (PartEntry->PartInfo[0].PartitionType == PARTITION_EXT2)
    {
#if 0
      if (CheckExt2Format())
      {
        PartEntry->FormatState = Preformatted;
      }
      else
      {
        PartEntry->FormatState = Unformatted;
      }
#endif
      PartEntry->FormatState = Preformatted;
    }
    else if (PartEntry->PartInfo[0].PartitionType == PARTITION_IFS)
    {
#if 0
      if (CheckNtfsFormat())
      {
        PartEntry->FormatState = Preformatted;
      }
      else if (CheckHpfsFormat())
      {
        PartEntry->FormatState = Preformatted;
      }
      else
      {
        PartEntry->FormatState = Unformatted;
      }
#endif
      PartEntry->FormatState = Preformatted;
    }
    else
    {
      PartEntry->FormatState = UnknownFormat;
    }

    InsertTailList (&DiskEntry->PartListHead,
                    &PartEntry->ListEntry);
  }
}


static VOID
ScanForUnpartitionedDiskSpace (PDISKENTRY DiskEntry)
{
  ULONGLONG LastStartingOffset;
  ULONGLONG LastPartitionLength;
  ULONGLONG LastUnusedPartitionLength;
  PPARTENTRY PartEntry;
  PPARTENTRY NewPartEntry;
  PLIST_ENTRY Entry;
  ULONG i;
  ULONG j;

  if (IsListEmpty (&DiskEntry->PartListHead))
  {
    /* Create a partition table that represents the empty disk */
    PartEntry = (PPARTENTRY)RtlAllocateHeap (ProcessHeap,
                 0,
                 sizeof(PARTENTRY));
    if (PartEntry == NULL)
      return;

    RtlZeroMemory (PartEntry,
                   sizeof(PARTENTRY));

    PartEntry->Unpartitioned = TRUE;
    PartEntry->UnpartitionedOffset = 0ULL;
    PartEntry->UnpartitionedLength = DiskEntry->DiskSize;

    PartEntry->FormatState = Unformatted;

    InsertTailList (&DiskEntry->PartListHead,
                    &PartEntry->ListEntry);
  }
  else
  {
    /* Start partition at head 1, cylinder 0 */
    LastStartingOffset = DiskEntry->TrackSize;
    LastPartitionLength = 0ULL;
    LastUnusedPartitionLength = 0ULL;

    i = 0;
    Entry = DiskEntry->PartListHead.Flink;
    while (Entry != &DiskEntry->PartListHead)
    {
      PartEntry = CONTAINING_RECORD (Entry, PARTENTRY, ListEntry);

      for (j = 0; j < 4; j++)
      {
        if ((!IsContainerPartition (PartEntry->PartInfo[j].PartitionType)) &&
            (PartEntry->PartInfo[j].PartitionType != PARTITION_ENTRY_UNUSED ||
             PartEntry->PartInfo[j].PartitionLength.QuadPart != 0LL))
        {
          LastUnusedPartitionLength =
          PartEntry->PartInfo[j].StartingOffset.QuadPart -
          (LastStartingOffset + LastPartitionLength);

          if (LastUnusedPartitionLength >= DiskEntry->CylinderSize)
          {
            DPRINT ("Unpartitioned disk space %I64u\n", LastUnusedPartitionLength);

            NewPartEntry = (PPARTENTRY)RtlAllocateHeap (ProcessHeap,
                            0,
                            sizeof(PARTENTRY));
            if (NewPartEntry == NULL)
              return;

            RtlZeroMemory (NewPartEntry,
                           sizeof(PARTENTRY));

            NewPartEntry->Unpartitioned = TRUE;
            NewPartEntry->UnpartitionedOffset = LastStartingOffset + LastPartitionLength;
            NewPartEntry->UnpartitionedLength = LastUnusedPartitionLength;
            if (j == 0)
              NewPartEntry->UnpartitionedLength -= DiskEntry->TrackSize;

            NewPartEntry->FormatState = Unformatted;

            /* Insert the table into the list */
            InsertTailList (&PartEntry->ListEntry,
                            &NewPartEntry->ListEntry);
          }

          LastStartingOffset = PartEntry->PartInfo[j].StartingOffset.QuadPart;
          LastPartitionLength = PartEntry->PartInfo[j].PartitionLength.QuadPart;
        }
      }

      i += 4;
      Entry = Entry->Flink;
    }

    /* Check for trailing unpartitioned disk space */
    if (DiskEntry->DiskSize > (LastStartingOffset + LastPartitionLength))
    {
      /* Round-down to cylinder size */
      LastUnusedPartitionLength =
        (DiskEntry->DiskSize - (LastStartingOffset + LastPartitionLength))
        & ~(DiskEntry->CylinderSize - 1);

      if (LastUnusedPartitionLength >= DiskEntry->CylinderSize)
      {
        DPRINT ("Unpartitioned disk space %I64u\n", LastUnusedPartitionLength);

        NewPartEntry = (PPARTENTRY)RtlAllocateHeap (ProcessHeap,
                                                    0,
                                                    sizeof(PARTENTRY));
        if (NewPartEntry == NULL)
          return;

        RtlZeroMemory (NewPartEntry,
                       sizeof(PARTENTRY));

        NewPartEntry->Unpartitioned = TRUE;
        NewPartEntry->UnpartitionedOffset = LastStartingOffset + LastPartitionLength;
        NewPartEntry->UnpartitionedLength = LastUnusedPartitionLength;

        /* Append the table to the list */
        InsertTailList (&DiskEntry->PartListHead,
                        &NewPartEntry->ListEntry);
      }
    }
  }
}

NTSTATUS
NTAPI
DiskIdentifierQueryRoutine(PWSTR ValueName,
                           ULONG ValueType,
                           PVOID ValueData,
                           ULONG ValueLength,
                           PVOID Context,
                           PVOID EntryContext)
{
  PBIOSDISKENTRY BiosDiskEntry = (PBIOSDISKENTRY)Context;
  UNICODE_STRING NameU;

  if (ValueType == REG_SZ &&
      ValueLength == 20 * sizeof(WCHAR))
  {
    NameU.Buffer = (PWCHAR)ValueData;
    NameU.Length = NameU.MaximumLength = 8 * sizeof(WCHAR);
    RtlUnicodeStringToInteger(&NameU, 16, &BiosDiskEntry->Checksum);

    NameU.Buffer = (PWCHAR)ValueData + 9;
    RtlUnicodeStringToInteger(&NameU, 16, &BiosDiskEntry->Signature);

    return STATUS_SUCCESS;
  }
  return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
DiskConfigurationDataQueryRoutine(PWSTR ValueName,
                                  ULONG ValueType,
                                  PVOID ValueData,
                                  ULONG ValueLength,
                                  PVOID Context,
                                  PVOID EntryContext)
{
  PBIOSDISKENTRY BiosDiskEntry = (PBIOSDISKENTRY)Context;
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_DISK_GEOMETRY_DEVICE_DATA DiskGeometry;
  ULONG i;

  if (ValueType != REG_FULL_RESOURCE_DESCRIPTOR ||
      ValueLength < sizeof(CM_FULL_RESOURCE_DESCRIPTOR))
    return STATUS_UNSUCCESSFUL;

  FullResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)ValueData;
  /* Hm. Version and Revision are not set on Microsoft Windows XP... */
  /*if (FullResourceDescriptor->PartialResourceList.Version != 1 ||
    FullResourceDescriptor->PartialResourceList.Revision != 1)
    return STATUS_UNSUCCESSFUL;*/

  for (i = 0; i < FullResourceDescriptor->PartialResourceList.Count; i++)
  {
    if (FullResourceDescriptor->PartialResourceList.PartialDescriptors[i].Type != CmResourceTypeDeviceSpecific ||
        FullResourceDescriptor->PartialResourceList.PartialDescriptors[i].u.DeviceSpecificData.DataSize != sizeof(CM_DISK_GEOMETRY_DEVICE_DATA))
      continue;

    DiskGeometry = (PCM_DISK_GEOMETRY_DEVICE_DATA)&FullResourceDescriptor->PartialResourceList.PartialDescriptors[i + 1];
    BiosDiskEntry->DiskGeometry = *DiskGeometry;

    return STATUS_SUCCESS;
  }
  return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
SystemConfigurationDataQueryRoutine(PWSTR ValueName,
                                    ULONG ValueType,
                                    PVOID ValueData,
                                    ULONG ValueLength,
                                    PVOID Context,
                                    PVOID EntryContext)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_INT13_DRIVE_PARAMETER* Int13Drives = (PCM_INT13_DRIVE_PARAMETER*)Context;
  ULONG i;

  if (ValueType != REG_FULL_RESOURCE_DESCRIPTOR ||
      ValueLength < sizeof (CM_FULL_RESOURCE_DESCRIPTOR))
    return STATUS_UNSUCCESSFUL;

  FullResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)ValueData;
  /* Hm. Version and Revision are not set on Microsoft Windows XP... */
  /*if (FullResourceDescriptor->PartialResourceList.Version != 1 ||
    FullResourceDescriptor->PartialResourceList.Revision != 1)
    return STATUS_UNSUCCESSFUL;*/

  for (i = 0; i < FullResourceDescriptor->PartialResourceList.Count; i++)
  {
    if (FullResourceDescriptor->PartialResourceList.PartialDescriptors[i].Type != CmResourceTypeDeviceSpecific ||
        FullResourceDescriptor->PartialResourceList.PartialDescriptors[i].u.DeviceSpecificData.DataSize % sizeof(CM_INT13_DRIVE_PARAMETER) != 0)
      continue;

    *Int13Drives = (CM_INT13_DRIVE_PARAMETER*) RtlAllocateHeap(ProcessHeap, 0, FullResourceDescriptor->PartialResourceList.PartialDescriptors[i].u.DeviceSpecificData.DataSize);
    if (*Int13Drives == NULL)
      return STATUS_NO_MEMORY;
    memcpy(*Int13Drives,
           &FullResourceDescriptor->PartialResourceList.PartialDescriptors[i + 1],
           FullResourceDescriptor->PartialResourceList.PartialDescriptors[i].u.DeviceSpecificData.DataSize);
    return STATUS_SUCCESS;
  }
  return STATUS_UNSUCCESSFUL;
}
#define ROOT_NAME   L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter"

static VOID
EnumerateBiosDiskEntries(PPARTLIST PartList)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[3];
  WCHAR Name[120];
  ULONG AdapterCount;
  ULONG DiskCount;
  NTSTATUS Status;
  PCM_INT13_DRIVE_PARAMETER Int13Drives;
  PBIOSDISKENTRY BiosDiskEntry;

  memset(QueryTable, 0, sizeof(QueryTable));

  QueryTable[1].Name = L"Configuration Data";
  QueryTable[1].QueryRoutine = SystemConfigurationDataQueryRoutine;
  Int13Drives = NULL;
  Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                  L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System",
                                  &QueryTable[1],
                                  (PVOID)&Int13Drives,
                                  NULL);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Unable to query the 'Configuration Data' key in '\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System', status=%lx\n", Status);
    return;
  }

  AdapterCount = 0;
  while (1)
  {
    swprintf(Name, L"%s\\%lu", ROOT_NAME, AdapterCount);
    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    Name,
                                    &QueryTable[2],
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
      break;
    }

    swprintf(Name, L"%s\\%lu\\DiskController", ROOT_NAME, AdapterCount);
    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    Name,
                                    &QueryTable[2],
                                    NULL,
                                    NULL);
    if (NT_SUCCESS(Status))
    {
      while (1)
      {
        swprintf(Name, L"%s\\%lu\\DiskController\\0", ROOT_NAME, AdapterCount);
        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        Name,
                                        &QueryTable[2],
                                        NULL,
                                        NULL);
        if (!NT_SUCCESS(Status))
        {
          RtlFreeHeap(ProcessHeap, 0, Int13Drives);
          return;
        }

        swprintf(Name, L"%s\\%lu\\DiskController\\0\\DiskPeripheral", ROOT_NAME, AdapterCount);
        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        Name,
                                        &QueryTable[2],
                                        NULL,
                                        NULL);
        if (NT_SUCCESS(Status))
        {
          QueryTable[0].Name = L"Identifier";
          QueryTable[0].QueryRoutine = DiskIdentifierQueryRoutine;
          QueryTable[1].Name = L"Configuration Data";
          QueryTable[1].QueryRoutine = DiskConfigurationDataQueryRoutine;
          DiskCount = 0;
          while (1)
          {
            BiosDiskEntry = (BIOSDISKENTRY*) RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, sizeof(BIOSDISKENTRY));
            if (BiosDiskEntry == NULL)
            {
              break;
            }
            swprintf(Name, L"%s\\%lu\\DiskController\\0\\DiskPeripheral\\%lu", ROOT_NAME, AdapterCount, DiskCount);
            Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                            Name,
                                            QueryTable,
                                            (PVOID)BiosDiskEntry,
                                            NULL);
            if (!NT_SUCCESS(Status))
            {
              RtlFreeHeap(ProcessHeap, 0, BiosDiskEntry);
              break;
            }
            BiosDiskEntry->DiskNumber = DiskCount;
            BiosDiskEntry->Recognized = FALSE;

            if (DiskCount < Int13Drives[0].NumberDrives)
            {
              BiosDiskEntry->Int13DiskData = Int13Drives[DiskCount];
            }
            else
            {
              DPRINT1("Didn't find int13 drive datas for disk %u\n", DiskCount);
            }


            InsertTailList(&PartList->BiosDiskListHead, &BiosDiskEntry->ListEntry);

            DPRINT("DiskNumber:        %lu\n", BiosDiskEntry->DiskNumber);
            DPRINT("Signature:         %08lx\n", BiosDiskEntry->Signature);
            DPRINT("Checksum:          %08lx\n", BiosDiskEntry->Checksum);
            DPRINT("BytesPerSector:    %lu\n", BiosDiskEntry->DiskGeometry.BytesPerSector);
            DPRINT("NumberOfCylinders: %lu\n", BiosDiskEntry->DiskGeometry.NumberOfCylinders);
            DPRINT("NumberOfHeads:     %lu\n", BiosDiskEntry->DiskGeometry.NumberOfHeads);
            DPRINT("DriveSelect:       %02x\n", BiosDiskEntry->Int13DiskData.DriveSelect);
            DPRINT("MaxCylinders:      %lu\n", BiosDiskEntry->Int13DiskData.MaxCylinders);
            DPRINT("SectorsPerTrack:   %d\n", BiosDiskEntry->Int13DiskData.SectorsPerTrack);
            DPRINT("MaxHeads:          %d\n", BiosDiskEntry->Int13DiskData.MaxHeads);
            DPRINT("NumberDrives:      %d\n", BiosDiskEntry->Int13DiskData.NumberDrives);

            DiskCount++;
          }
        }
        RtlFreeHeap(ProcessHeap, 0, Int13Drives);
        return;
      }
    }
    AdapterCount++;
  }
  RtlFreeHeap(ProcessHeap, 0, Int13Drives);
}

static VOID
AddDiskToList (HANDLE FileHandle,
               ULONG DiskNumber,
               PPARTLIST List)
{
  DRIVE_LAYOUT_INFORMATION *LayoutBuffer;
  DISK_GEOMETRY DiskGeometry;
  SCSI_ADDRESS ScsiAddress;
  PDISKENTRY DiskEntry;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  PPARTITION_SECTOR Mbr;
  PULONG Buffer;
  LARGE_INTEGER FileOffset;
  WCHAR Identifier[20];
  ULONG Checksum;
  ULONG Signature;
  ULONG i;
  PLIST_ENTRY ListEntry;
  PBIOSDISKENTRY BiosDiskEntry;

  Status = NtDeviceIoControlFile (FileHandle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &Iosb,
                                  IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                  NULL,
                                  0,
                                  &DiskGeometry,
                                  sizeof(DISK_GEOMETRY));
  if (!NT_SUCCESS (Status))
  {
    return;
  }

  if (DiskGeometry.MediaType != FixedMedia)
  {
    return;
  }

  Status = NtDeviceIoControlFile (FileHandle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &Iosb,
                                  IOCTL_SCSI_GET_ADDRESS,
                                  NULL,
                                  0,
                                  &ScsiAddress,
                                  sizeof(SCSI_ADDRESS));
  if (!NT_SUCCESS(Status))
  {
    return;
  }

  Mbr = (PARTITION_SECTOR*) RtlAllocateHeap(ProcessHeap,
                                            0,
                                            DiskGeometry.BytesPerSector);

  if (Mbr == NULL)
  {
    return;
  }

  FileOffset.QuadPart = 0;
  Status = NtReadFile(FileHandle,
                      NULL,
                      NULL,
                      NULL,
                      &Iosb,
                      (PVOID)Mbr,
                      DiskGeometry.BytesPerSector,
                      &FileOffset,
                      NULL);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap,
                0,
                Mbr);
    DPRINT1("NtReadFile failed, status=%x\n", Status);
    return;
  }
  Signature = Mbr->Signature;

  /* Calculate the MBR checksum */
  Checksum = 0;
  Buffer = (PULONG)Mbr;
  for (i = 0; i < 128; i++)
  {
    Checksum += Buffer[i];
  }
  Checksum = ~Checksum + 1;

  swprintf(Identifier, L"%08x-%08x-A", Checksum, Signature);
  DPRINT("Identifier: %S\n", Identifier);

  DiskEntry = (PDISKENTRY)RtlAllocateHeap (ProcessHeap,
                                           0,
                                           sizeof(DISKENTRY));
  if (DiskEntry == NULL)
  {
    return;
  }

  DiskEntry->Checksum = Checksum;
  DiskEntry->Signature = Signature;
  if (Signature == 0)
  {
    /* If we have no signature, set the disk to dirty. WritePartitionsToDisk creates a new signature */
    DiskEntry->Modified = TRUE;
  }
  DiskEntry->BiosFound = FALSE;

  /* Check if this disk has a valid MBR */
  if (Mbr->BootCode[0] == 0 && Mbr->BootCode[1] == 0)
    DiskEntry->NoMbr = TRUE;
  else
    DiskEntry->NoMbr = FALSE;

  /* Free Mbr sector buffer */
  RtlFreeHeap (ProcessHeap,
               0,
               Mbr);

  ListEntry = List->BiosDiskListHead.Flink;
  while(ListEntry != &List->BiosDiskListHead)
  {
    BiosDiskEntry = CONTAINING_RECORD(ListEntry, BIOSDISKENTRY, ListEntry);
    /* FIXME:
     *   Compare the size from bios and the reported size from driver.
     *   If we have more than one disk with a zero or with the same signatur
     *   we must create new signatures and reboot. After the reboot,
     *   it is possible to identify the disks.
     */
    if (BiosDiskEntry->Signature == Signature &&
        BiosDiskEntry->Checksum == Checksum &&
        !BiosDiskEntry->Recognized)
    {
      if (!DiskEntry->BiosFound)
      {
        DiskEntry->BiosDiskNumber = BiosDiskEntry->DiskNumber;
        DiskEntry->BiosFound = TRUE;
        BiosDiskEntry->Recognized = TRUE;
      }
      else
      {
      }
    }
    ListEntry = ListEntry->Flink;
  }

  if (!DiskEntry->BiosFound)
  {
    RtlFreeHeap(ProcessHeap, 0, DiskEntry);
    return;
  }

  InitializeListHead (&DiskEntry->PartListHead);

  DiskEntry->Cylinders = DiskGeometry.Cylinders.QuadPart;
  DiskEntry->TracksPerCylinder = DiskGeometry.TracksPerCylinder;
  DiskEntry->SectorsPerTrack = DiskGeometry.SectorsPerTrack;
  DiskEntry->BytesPerSector = DiskGeometry.BytesPerSector;

  DPRINT ("Cylinders %d\n", DiskEntry->Cylinders);
  DPRINT ("TracksPerCylinder %d\n", DiskEntry->TracksPerCylinder);
  DPRINT ("SectorsPerTrack %d\n", DiskEntry->SectorsPerTrack);
  DPRINT ("BytesPerSector %d\n", DiskEntry->BytesPerSector);

  DiskEntry->TrackSize =
    (ULONGLONG)DiskGeometry.SectorsPerTrack *
    (ULONGLONG)DiskGeometry.BytesPerSector;
  DiskEntry->CylinderSize =
    (ULONGLONG)DiskGeometry.TracksPerCylinder *
    DiskEntry->TrackSize;
  DiskEntry->DiskSize =
    DiskGeometry.Cylinders.QuadPart *
    DiskEntry->CylinderSize;

  DiskEntry->DiskNumber = DiskNumber;
  DiskEntry->Port = ScsiAddress.PortNumber;
  DiskEntry->Bus = ScsiAddress.PathId;
  DiskEntry->Id = ScsiAddress.TargetId;

  GetDriverName (DiskEntry);

  InsertAscendingList(&List->DiskListHead, DiskEntry, DISKENTRY, ListEntry, BiosDiskNumber);

  LayoutBuffer = (DRIVE_LAYOUT_INFORMATION*)RtlAllocateHeap (ProcessHeap,
                  0,
                  8192);
  if (LayoutBuffer == NULL)
  {
    return;
  }

  Status = NtDeviceIoControlFile (FileHandle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &Iosb,
                                  IOCTL_DISK_GET_DRIVE_LAYOUT,
                                  NULL,
                                  0,
                                  LayoutBuffer,
                                  8192);
  if (NT_SUCCESS (Status))
  {
    if (LayoutBuffer->PartitionCount == 0)
    {
      DiskEntry->NewDisk = TRUE;
    }

    AddPartitionToList (DiskNumber,
                        DiskEntry,
                        LayoutBuffer);

    ScanForUnpartitionedDiskSpace (DiskEntry);
  }

  RtlFreeHeap (ProcessHeap,
               0,
               LayoutBuffer);
}


PPARTLIST
CreatePartitionList (SHORT Left,
                     SHORT Top,
                     SHORT Right,
                     SHORT Bottom)
{
  PPARTLIST List;
  OBJECT_ATTRIBUTES ObjectAttributes;
  SYSTEM_DEVICE_INFORMATION Sdi;
  IO_STATUS_BLOCK Iosb;
  ULONG ReturnSize;
  NTSTATUS Status;
  ULONG DiskNumber;
  WCHAR Buffer[MAX_PATH];
  UNICODE_STRING Name;
  HANDLE FileHandle;

  List = (PPARTLIST)RtlAllocateHeap (ProcessHeap,
                                     0,
                                     sizeof (PARTLIST));
  if (List == NULL)
    return NULL;

  List->Left = Left;
  List->Top = Top;
  List->Right = Right;
  List->Bottom = Bottom;

  List->Line = 0;

  List->TopDisk = (ULONG)-1;
  List->TopPartition = (ULONG)-1;

  List->CurrentDisk = NULL;
  List->CurrentPartition = NULL;
  List->CurrentPartitionNumber = 0;

  InitializeListHead (&List->DiskListHead);
  InitializeListHead (&List->BiosDiskListHead);

  EnumerateBiosDiskEntries(List);

  Status = NtQuerySystemInformation (SystemDeviceInformation,
                                     &Sdi,
                                     sizeof(SYSTEM_DEVICE_INFORMATION),
                                     &ReturnSize);
  if (!NT_SUCCESS (Status))
  {
    RtlFreeHeap (ProcessHeap, 0, List);
    return NULL;
  }

  for (DiskNumber = 0; DiskNumber < Sdi.NumberOfDisks; DiskNumber++)
  {
    swprintf (Buffer,
              L"\\Device\\Harddisk%d\\Partition0",
              DiskNumber);
    RtlInitUnicodeString (&Name,
                          Buffer);

    InitializeObjectAttributes (&ObjectAttributes,
                                &Name,
                                0,
                                NULL,
                                NULL);

    Status = NtOpenFile (&FileHandle,
                         FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                         &ObjectAttributes,
                         &Iosb,
                         FILE_SHARE_READ,
                         FILE_SYNCHRONOUS_IO_NONALERT);
    if (NT_SUCCESS(Status))
    {
      AddDiskToList (FileHandle,
                     DiskNumber,
                     List);

      NtClose(FileHandle);
    }
  }

  AssignDriverLetters (List);

  List->TopDisk = 0;
  List->TopPartition = 0;

  /* Search for first usable disk and partition */
  if (IsListEmpty (&List->DiskListHead))
  {
    List->CurrentDisk = NULL;
    List->CurrentPartition = NULL;
    List->CurrentPartitionNumber = 0;
  }
  else
  {
    List->CurrentDisk =
      CONTAINING_RECORD (List->DiskListHead.Flink,
                         DISKENTRY,
                         ListEntry);

    if (IsListEmpty (&List->CurrentDisk->PartListHead))
    {
      List->CurrentPartition = 0;
      List->CurrentPartitionNumber = 0;
    }
    else
    {
      List->CurrentPartition =
        CONTAINING_RECORD (List->CurrentDisk->PartListHead.Flink,
                           PARTENTRY,
                           ListEntry);
      List->CurrentPartitionNumber = 0;
    }
  }

  return List;
}


VOID
DestroyPartitionList (PPARTLIST List)
{
  PDISKENTRY DiskEntry;
  PBIOSDISKENTRY BiosDiskEntry;
  PPARTENTRY PartEntry;
  PLIST_ENTRY Entry;

  /* Release disk and partition info */
  while (!IsListEmpty (&List->DiskListHead))
  {
    Entry = RemoveHeadList (&List->DiskListHead);
    DiskEntry = CONTAINING_RECORD (Entry, DISKENTRY, ListEntry);

    /* Release driver name */
    RtlFreeUnicodeString(&DiskEntry->DriverName);

    /* Release partition array */
    while (!IsListEmpty (&DiskEntry->PartListHead))
    {
      Entry = RemoveHeadList (&DiskEntry->PartListHead);
      PartEntry = CONTAINING_RECORD (Entry, PARTENTRY, ListEntry);

      RtlFreeHeap (ProcessHeap,
                   0,
                   PartEntry);
    }

    /* Release disk entry */
    RtlFreeHeap (ProcessHeap, 0, DiskEntry);
  }

  /* release the bios disk info */
  while(!IsListEmpty(&List->BiosDiskListHead))
  {
    Entry = RemoveHeadList(&List->BiosDiskListHead);
    BiosDiskEntry = CONTAINING_RECORD(Entry, BIOSDISKENTRY, ListEntry);

    RtlFreeHeap(ProcessHeap, 0, BiosDiskEntry);
  }

  /* Release list head */
  RtlFreeHeap (ProcessHeap, 0, List);
}


static VOID
PrintEmptyLine (PPARTLIST List)
{
  COORD coPos;
  DWORD Written;
  USHORT Width;
  USHORT Height;

  Width = List->Right - List->Left - 1;
  Height = List->Bottom - List->Top - 2;


  coPos.X = List->Left + 1;
  coPos.Y = List->Top + 1 + List->Line;

  if (List->Line >= 0 && List->Line <= Height)
  {
    FillConsoleOutputAttribute (StdOutput,
                                FOREGROUND_WHITE | BACKGROUND_BLUE,
                                Width,
                                coPos,
                                &Written);

    FillConsoleOutputCharacterA (StdOutput,
                                 ' ',
                                 Width,
                                 coPos,
                                 &Written);
  }
  List->Line++;
}


static VOID
PrintPartitionData (PPARTLIST List,
                    PDISKENTRY DiskEntry,
                    PPARTENTRY PartEntry,
                    ULONG PartNumber)
{
  CHAR LineBuffer[128];
  COORD coPos;
  DWORD Written;
  USHORT Width;
  USHORT Height;

  LARGE_INTEGER PartSize;
  PCHAR Unit;
  UCHAR Attribute;
  PCHAR PartType;

  Width = List->Right - List->Left - 1;
  Height = List->Bottom - List->Top - 2;


  coPos.X = List->Left + 1;
  coPos.Y = List->Top + 1 + List->Line;

  if (PartEntry->Unpartitioned == TRUE)
  {
#if 0
    if (PartEntry->UnpartitionledLength >= 0x280000000ULL) /* 10 GB */
    {
      PartSize.QuadPart = (PartEntry->UnpartitionedLength + (1 << 29)) >> 30;
      Unit = MUIGetString(STRING_GB);
    }
    else
#endif
      if (PartEntry->UnpartitionedLength >= 0xA00000ULL) /* 10 MB */
      {
        PartSize.QuadPart = (PartEntry->UnpartitionedLength + (1 << 19)) >> 20;
        Unit = MUIGetString(STRING_MB);
      }
      else
      {
        PartSize.QuadPart = (PartEntry->UnpartitionedLength + (1 << 9)) >> 10;
        Unit = MUIGetString(STRING_KB);
      }

      sprintf (LineBuffer,
               MUIGetString(STRING_UNPSPACE),
               PartSize.u.LowPart,
               Unit);
  }
  else
  {
    /* Determine partition type */
    PartType = NULL;
    if (PartEntry->New == TRUE)
    {
      PartType = MUIGetString(STRING_UNFORMATTED);
    }
    else if (PartEntry->Unpartitioned == FALSE)
    {
      if ((PartEntry->PartInfo[PartNumber].PartitionType == PARTITION_FAT_12) ||
          (PartEntry->PartInfo[PartNumber].PartitionType == PARTITION_FAT_16) ||
          (PartEntry->PartInfo[PartNumber].PartitionType == PARTITION_HUGE) ||
          (PartEntry->PartInfo[PartNumber].PartitionType == PARTITION_XINT13))
      {
        PartType = "FAT";
      }
      else if ((PartEntry->PartInfo[PartNumber].PartitionType == PARTITION_FAT32) ||
               (PartEntry->PartInfo[PartNumber].PartitionType == PARTITION_FAT32_XINT13))
      {
        PartType = "FAT32";
      }
      else if (PartEntry->PartInfo[PartNumber].PartitionType == PARTITION_EXT2)
      {
        PartType = "EXT2";
      }
      else if (PartEntry->PartInfo[PartNumber].PartitionType == PARTITION_IFS)
      {
        PartType = "NTFS"; /* FIXME: Not quite correct! */
      }
    }

#if 0
    if (PartEntry->PartInfo[PartNumber].PartitionLength.QuadPart >= 0x280000000LL) /* 10 GB */
    {
      PartSize.QuadPart = (PartEntry->PartInfo[PartNumber].PartitionLength.QuadPart + (1 << 29)) >> 30;
      Unit = MUIGetString(STRING_GB);
    }
    else
#endif
    if (PartEntry->PartInfo[PartNumber].PartitionLength.QuadPart >= 0xA00000LL) /* 10 MB */
    {
      PartSize.QuadPart = (PartEntry->PartInfo[PartNumber].PartitionLength.QuadPart + (1 << 19)) >> 20;
      Unit = MUIGetString(STRING_MB);
    }
    else
    {
      PartSize.QuadPart = (PartEntry->PartInfo[PartNumber].PartitionLength.QuadPart + (1 << 9)) >> 10;
      Unit = MUIGetString(STRING_KB);
    }

    if (PartType == NULL)
    {
      sprintf (LineBuffer,
               MUIGetString(STRING_HDDINFOUNK5),
               (PartEntry->DriveLetter[PartNumber] == 0) ? '-' : PartEntry->DriveLetter[PartNumber],
               (PartEntry->DriveLetter[PartNumber] == 0) ? '-' : ':',
               PartEntry->PartInfo[PartNumber].PartitionType,
               PartSize.u.LowPart,
               Unit);
    }
    else
    {
      sprintf (LineBuffer,
               "%c%c  %-24s         %6lu %s",
               (PartEntry->DriveLetter[PartNumber] == 0) ? '-' : PartEntry->DriveLetter[PartNumber],
               (PartEntry->DriveLetter[PartNumber] == 0) ? '-' : ':',
               PartType,
               PartSize.u.LowPart,
               Unit);
    }
  }

  Attribute = (List->CurrentDisk == DiskEntry &&
               List->CurrentPartition == PartEntry &&
               List->CurrentPartitionNumber == PartNumber) ?
               FOREGROUND_BLUE | BACKGROUND_WHITE :
               FOREGROUND_WHITE | BACKGROUND_BLUE;

  if (List->Line >= 0 && List->Line <= Height)
  {
    FillConsoleOutputCharacterA (StdOutput,
                                 ' ',
                                 Width,
                                 coPos,
                                 &Written);
  }
  coPos.X += 4;
  Width -= 8;
  if (List->Line >= 0 && List->Line <= Height)
  {
    FillConsoleOutputAttribute (StdOutput,
                                Attribute,
                                Width,
                                coPos,
                                &Written);
  }
  coPos.X++;
  Width -= 2;
  if (List->Line >= 0 && List->Line <= Height)
  {
    WriteConsoleOutputCharacterA (StdOutput,
                                  LineBuffer,
                                  min (strlen (LineBuffer), Width),
                                  coPos,
                                  &Written);
  }
  List->Line++;
}


static VOID
PrintDiskData (PPARTLIST List,
               PDISKENTRY DiskEntry)
{
  PPARTENTRY PartEntry;
  CHAR LineBuffer[128];
  COORD coPos;
  DWORD Written;
  USHORT Width;
  USHORT Height;
  ULARGE_INTEGER DiskSize;
  PCHAR Unit;
  ULONG i;

  Width = List->Right - List->Left - 1;
  Height = List->Bottom - List->Top - 2;


  coPos.X = List->Left + 1;
  coPos.Y = List->Top + 1 + List->Line;

#if 0
  if (DiskEntry->DiskSize >= 0x280000000ULL) /* 10 GB */
  {
    DiskSize.QuadPart = (DiskEntry->DiskSize + (1 << 29)) >> 30;
    Unit = MUIGetString(STRING_GB);
  }
  else
#endif
  {
    DiskSize.QuadPart = (DiskEntry->DiskSize + (1 << 19)) >> 20;
    if (DiskSize.QuadPart == 0)
      DiskSize.QuadPart = 1;
    Unit = MUIGetString(STRING_MB);
  }

  if (DiskEntry->DriverName.Length > 0)
  {
    sprintf (LineBuffer,
             MUIGetString(STRING_HDINFOPARTSELECT),
             DiskSize.u.LowPart,
             Unit,
             DiskEntry->DiskNumber,
             DiskEntry->Port,
             DiskEntry->Bus,
             DiskEntry->Id,
             DiskEntry->DriverName.Buffer);
  }
  else
  {
    sprintf (LineBuffer,
             MUIGetString(STRING_HDDINFOUNK6),
             DiskSize.u.LowPart,
             Unit,
             DiskEntry->DiskNumber,
             DiskEntry->Port,
             DiskEntry->Bus,
             DiskEntry->Id);
  }
  if (List->Line >= 0 && List->Line <= Height)
  {
    FillConsoleOutputAttribute (StdOutput,
                                FOREGROUND_WHITE | BACKGROUND_BLUE,
                                Width,
                                coPos,
                                &Written);

    FillConsoleOutputCharacterA (StdOutput,
                                 ' ',
                                 Width,
                                 coPos,
                                 &Written);
  }

  coPos.X++;
  if (List->Line >= 0 && List->Line <= Height)
  {
    WriteConsoleOutputCharacterA (StdOutput,
                                  LineBuffer,
                                  min ((USHORT)strlen (LineBuffer), Width - 2),
                                  coPos,
                                  &Written);
  }
  List->Line++;

  /* Print separator line */
  PrintEmptyLine (List);

  /* Print partition lines*/
  LIST_FOR_EACH(PartEntry, &DiskEntry->PartListHead, PARTENTRY, ListEntry)
  {
    /* Print disk entry */
    for (i=0; i<4; i++)
    {
      if (PartEntry->PartInfo[i].PartitionType != PARTITION_ENTRY_UNUSED ||
          PartEntry->PartInfo[i].PartitionLength.QuadPart != 0ULL)
      {
        PrintPartitionData (List,
                            DiskEntry,
                            PartEntry,
                            i);
      }
    }

    /* Print unpartitioned entry */
    if (PartEntry->Unpartitioned || PartEntry->New)
    {
        PrintPartitionData (List,
                            DiskEntry,
                            PartEntry,
                            0);
    }

  }

  /* Print separator line */
  PrintEmptyLine (List);
}


VOID
DrawPartitionList (PPARTLIST List)
{
  PLIST_ENTRY Entry, Entry2;
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry = NULL;
  COORD coPos;
  DWORD Written;
  SHORT i;
  SHORT CurrentDiskLine;
  SHORT CurrentPartLine;
  SHORT LastLine;
  BOOL CurrentPartLineFound = FALSE;
  BOOL CurrentDiskLineFound = FALSE;

  /* Calculate the line of the current disk and partition */
  CurrentDiskLine = 0;
  CurrentPartLine = 0;
  LastLine = 0;
  Entry = List->DiskListHead.Flink;
  while (Entry != &List->DiskListHead)
  {
    DiskEntry = CONTAINING_RECORD (Entry, DISKENTRY, ListEntry);
    LastLine += 2;
    if (CurrentPartLineFound == FALSE)
    {
      CurrentPartLine += 2;
    }
    Entry2 = DiskEntry->PartListHead.Flink;
    while (Entry2 != &DiskEntry->PartListHead)
    {
      PartEntry = CONTAINING_RECORD (Entry2, PARTENTRY, ListEntry);
      if (PartEntry == List->CurrentPartition)
      {
        CurrentPartLineFound = TRUE;
      }
      Entry2 = Entry2->Flink;
      if (CurrentPartLineFound == FALSE)
      {
        CurrentPartLine++;
      }
      LastLine++;
    }
    if (DiskEntry == List->CurrentDisk)
    {
      CurrentDiskLineFound = TRUE;
    }
    Entry = Entry->Flink;
    if (Entry != &List->DiskListHead)
    {
      if (CurrentDiskLineFound == FALSE)
      {
        CurrentPartLine ++;
        CurrentDiskLine = CurrentPartLine;
      }
      LastLine++;
    }
    else
    {
      LastLine--;
    }
  }

  /* If it possible, make the disk name visible */
  if (CurrentPartLine < List->Offset)
  {
    List->Offset = CurrentPartLine;
  }
  else if (CurrentPartLine - List->Offset > List->Bottom - List->Top - 2)
  {
    List->Offset = CurrentPartLine - (List->Bottom - List->Top - 2);
  }
  if (CurrentDiskLine < List->Offset && CurrentPartLine - CurrentDiskLine < List->Bottom - List->Top - 2)
  {
    List->Offset = CurrentDiskLine;
  }


  /* draw upper left corner */
  coPos.X = List->Left;
  coPos.Y = List->Top;
  FillConsoleOutputCharacterA (StdOutput,
                               0xDA, // '+',
                               1,
                               coPos,
                               &Written);

  /* draw upper edge */
  coPos.X = List->Left + 1;
  coPos.Y = List->Top;
  if (List->Offset == 0)
  {
    FillConsoleOutputCharacterA (StdOutput,
                                 0xC4, // '-',
                                 List->Right - List->Left - 1,
                                 coPos,
                                 &Written);
  }
  else
  {
    FillConsoleOutputCharacterA (StdOutput,
                                 0xC4, // '-',
                                 List->Right - List->Left - 5,
                                 coPos,
                                 &Written);
    coPos.X = List->Right - 5;
    WriteConsoleOutputCharacterA (StdOutput,
                                  "(\x18)", // "(up)"
                                  3,
                                  coPos,
                                  &Written);
    coPos.X = List->Right - 2;
    FillConsoleOutputCharacterA (StdOutput,
                                 0xC4, // '-',
                                 2,
                                 coPos,
                                 &Written);
  }

  /* draw upper right corner */
  coPos.X = List->Right;
  coPos.Y = List->Top;
  FillConsoleOutputCharacterA (StdOutput,
                               0xBF, // '+',
                               1,
                               coPos,
                               &Written);

  /* draw left and right edge */
  for (i = List->Top + 1; i < List->Bottom; i++)
  {
    coPos.X = List->Left;
    coPos.Y = i;
    FillConsoleOutputCharacterA (StdOutput,
                                 0xB3, // '|',
                                 1,
                                 coPos,
                                 &Written);

    coPos.X = List->Right;
    FillConsoleOutputCharacterA (StdOutput,
                                 0xB3, //'|',
                                 1,
                                 coPos,
                                 &Written);
  }

  /* draw lower left corner */
  coPos.X = List->Left;
  coPos.Y = List->Bottom;
  FillConsoleOutputCharacterA (StdOutput,
                               0xC0, // '+',
                               1,
                               coPos,
                               &Written);

  /* draw lower edge */
  coPos.X = List->Left + 1;
  coPos.Y = List->Bottom;
  if (LastLine - List->Offset <= List->Bottom - List->Top - 2)
  {
    FillConsoleOutputCharacterA (StdOutput,
                                 0xC4, // '-',
                                 List->Right - List->Left - 1,
                                 coPos,
                                 &Written);
  }
  else
  {
    FillConsoleOutputCharacterA (StdOutput,
                                 0xC4, // '-',
                                 List->Right - List->Left - 5,
                                 coPos,
                                 &Written);
    coPos.X = List->Right - 5;
    WriteConsoleOutputCharacterA (StdOutput,
                                 "(\x19)", // "(down)"
                                 3,
                                 coPos,
                                 &Written);
    coPos.X = List->Right - 2;
    FillConsoleOutputCharacterA (StdOutput,
                                 0xC4, // '-',
                                 2,
                                 coPos,
                                 &Written);
  }

  /* draw lower right corner */
  coPos.X = List->Right;
  coPos.Y = List->Bottom;
  FillConsoleOutputCharacterA (StdOutput,
                               0xD9, // '+',
                               1,
                               coPos,
                               &Written);

  /* print list entries */
  List->Line = - List->Offset;

  LIST_FOR_EACH(DiskEntry, &List->DiskListHead, DISKENTRY, ListEntry)
  {
    /* Print disk entry */
    PrintDiskData (List,
                   DiskEntry);
  }
}


DWORD
SelectPartition(PPARTLIST List, ULONG DiskNumber, ULONG PartitionNumber)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  PLIST_ENTRY Entry1;
  PLIST_ENTRY Entry2;
  ULONG i;

  /* Check for empty disks */
  if (IsListEmpty (&List->DiskListHead))
    return FALSE;

  /* Check for first usable entry on next disk */
  Entry1 = List->CurrentDisk->ListEntry.Flink;
  while (Entry1 != &List->DiskListHead)
  {
    DiskEntry = CONTAINING_RECORD (Entry1, DISKENTRY, ListEntry);

    if (DiskEntry->DiskNumber == DiskNumber)
    {
      Entry2 = DiskEntry->PartListHead.Flink;
      while (Entry2 != &DiskEntry->PartListHead)
      {
        PartEntry = CONTAINING_RECORD (Entry2, PARTENTRY, ListEntry);

        for (i = 0; i < 4; i++)
        {
          if (PartEntry->PartInfo[i].PartitionNumber == PartitionNumber)
          {
            List->CurrentDisk = DiskEntry;
            List->CurrentPartition = PartEntry;
            List->CurrentPartitionNumber = i;
            DrawPartitionList (List);
            return TRUE;
          }
        }
        Entry2 = Entry2->Flink;
      }
      return FALSE;
    }
    Entry1 = Entry1->Flink;
  }
  return FALSE;
}


VOID
ScrollDownPartitionList (PPARTLIST List)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  PLIST_ENTRY Entry1;
  PLIST_ENTRY Entry2;
  UCHAR i;

  /* Check for empty disks */
  if (IsListEmpty (&List->DiskListHead))
    return;

  /* Check for next usable entry on current disk */
  if (List->CurrentPartition != NULL)
  {
    Entry2 = &List->CurrentPartition->ListEntry;
    PartEntry = CONTAINING_RECORD (Entry2, PARTENTRY, ListEntry);

    /* Check if we can move inside primary partitions */
    for (i = List->CurrentPartitionNumber + 1; i < 4; i++)
    {
        if (PartEntry->PartInfo[i].PartitionType != PARTITION_ENTRY_UNUSED)
            break;
    }

    if (i == 4)
    {
        /* We're out of partitions in the current partition table.
           Try to move to the next one if possible. */
        Entry2 = Entry2->Flink;
    }
    else
    {
        /* Just advance to the next partition */
        List->CurrentPartitionNumber = i;
        DrawPartitionList (List);
        return;
    }

    while (Entry2 != &List->CurrentDisk->PartListHead)
    {
      PartEntry = CONTAINING_RECORD (Entry2, PARTENTRY, ListEntry);

//	  if (PartEntry->HidePartEntry == FALSE)
      {
        List->CurrentPartition = PartEntry;
        List->CurrentPartitionNumber = 0;
        DrawPartitionList (List);
        return;
      }
      Entry2 = Entry2->Flink;
    }
  }

  /* Check for first usable entry on next disk */
  if (List->CurrentDisk != NULL)
  {
    Entry1 = List->CurrentDisk->ListEntry.Flink;
    while (Entry1 != &List->DiskListHead)
    {
      DiskEntry = CONTAINING_RECORD (Entry1, DISKENTRY, ListEntry);

      Entry2 = DiskEntry->PartListHead.Flink;
      while (Entry2 != &DiskEntry->PartListHead)
      {
        PartEntry = CONTAINING_RECORD (Entry2, PARTENTRY, ListEntry);

//	      if (PartEntry->HidePartEntry == FALSE)
        {
          List->CurrentDisk = DiskEntry;
          List->CurrentPartition = PartEntry;
          List->CurrentPartitionNumber = 0;
          DrawPartitionList (List);
          return;
        }

        Entry2 = Entry2->Flink;
      }

      Entry1 = Entry1->Flink;
    }
  }
}


VOID
ScrollUpPartitionList (PPARTLIST List)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  PLIST_ENTRY Entry1;
  PLIST_ENTRY Entry2;
  UCHAR i;

  /* Check for empty disks */
  if (IsListEmpty (&List->DiskListHead))
    return;

  /* check for previous usable entry on current disk */
  if (List->CurrentPartition != NULL)
  {
    Entry2 = &List->CurrentPartition->ListEntry;
    PartEntry = CONTAINING_RECORD (Entry2, PARTENTRY, ListEntry);

    /* Check if we can move inside primary partitions */
    if (List->CurrentPartitionNumber > 0)
    {
        /* Find a previous partition */
        for (i = List->CurrentPartitionNumber - 1; i > 0; i--)
        {
            if (PartEntry->PartInfo[i].PartitionType != PARTITION_ENTRY_UNUSED)
                break;
        }

        /* Move to it and return */
        List->CurrentPartitionNumber = i;
        DrawPartitionList (List);
        return;
    }

    /* Move to the previous entry */
    Entry2 = Entry2->Blink;

    while (Entry2 != &List->CurrentDisk->PartListHead)
    {
      PartEntry = CONTAINING_RECORD (Entry2, PARTENTRY, ListEntry);

//	  if (PartEntry->HidePartEntry == FALSE)
      {
        List->CurrentPartition = PartEntry;

        /* Find last existing partition in the table */
        for (i = 3; i > 0; i--)
        {
            if (PartEntry->PartInfo[i].PartitionType != PARTITION_ENTRY_UNUSED)
                break;
        }

        /* Move to it */
        List->CurrentPartitionNumber = i;

        /* Draw partition list and return */
        DrawPartitionList (List);
        return;
      }
      Entry2 = Entry2->Blink;
    }
  }


  /* check for last usable entry on previous disk */
  if (List->CurrentDisk != NULL)
  {
    Entry1 = List->CurrentDisk->ListEntry.Blink;
    while (Entry1 != &List->DiskListHead)
    {
      DiskEntry = CONTAINING_RECORD (Entry1, DISKENTRY, ListEntry);

      Entry2 = DiskEntry->PartListHead.Blink;
      while (Entry2 != &DiskEntry->PartListHead)
      {
        PartEntry = CONTAINING_RECORD (Entry2, PARTENTRY, ListEntry);

//	      if (PartEntry->HidePartEntry == FALSE)
        {
          List->CurrentDisk = DiskEntry;
          List->CurrentPartition = PartEntry;

          /* Find last existing partition in the table */
          for (i = 3; i > 0; i--)
          {
            if (PartEntry->PartInfo[i].PartitionType != PARTITION_ENTRY_UNUSED)
              break;
          }

          /* Move to it */
          List->CurrentPartitionNumber = i;

          /* Draw partition list and return */
          DrawPartitionList (List);
          return;
        }

        Entry2 = Entry2->Blink;
      }

      Entry1 = Entry1->Blink;
    }
  }
}


static PPARTENTRY
GetPrevPartitionedEntry (PDISKENTRY DiskEntry,
                         PPARTENTRY CurrentEntry)
{
  PPARTENTRY PrevEntry;
  PLIST_ENTRY Entry;

  if (CurrentEntry->ListEntry.Blink == &DiskEntry->PartListHead)
    return NULL;

  Entry = CurrentEntry->ListEntry.Blink;
  while (Entry != &DiskEntry->PartListHead)
  {
    PrevEntry = CONTAINING_RECORD (Entry,
                                   PARTENTRY,
                                   ListEntry);
    if (PrevEntry->Unpartitioned == FALSE)
      return PrevEntry;

    Entry = Entry->Blink;
  }

  return NULL;
}


static PPARTENTRY
GetNextPartitionedEntry (PDISKENTRY DiskEntry,
                         PPARTENTRY CurrentEntry)
{
  PPARTENTRY NextEntry;
  PLIST_ENTRY Entry;

  if (CurrentEntry->ListEntry.Flink == &DiskEntry->PartListHead)
    return NULL;

  Entry = CurrentEntry->ListEntry.Flink;
  while (Entry != &DiskEntry->PartListHead)
  {
    NextEntry = CONTAINING_RECORD (Entry,
                                   PARTENTRY,
                                   ListEntry);
    if (NextEntry->Unpartitioned == FALSE)
      return NextEntry;

    Entry = Entry->Flink;
  }

  return NULL;
}


static PPARTENTRY
GetPrevUnpartitionedEntry (PDISKENTRY DiskEntry,
                           PPARTENTRY PartEntry)
{
  PPARTENTRY PrevPartEntry;

  if (PartEntry->ListEntry.Blink != &DiskEntry->PartListHead)
  {
    PrevPartEntry = CONTAINING_RECORD (PartEntry->ListEntry.Blink,
                                       PARTENTRY,
                                       ListEntry);
    if (PrevPartEntry->Unpartitioned == TRUE)
      return PrevPartEntry;
  }

  return NULL;
}


static PPARTENTRY
GetNextUnpartitionedEntry (PDISKENTRY DiskEntry,
                           PPARTENTRY PartEntry)
{
  PPARTENTRY NextPartEntry;

  if (PartEntry->ListEntry.Flink != &DiskEntry->PartListHead)
  {
    NextPartEntry = CONTAINING_RECORD (PartEntry->ListEntry.Flink,
                                       PARTENTRY,
                                       ListEntry);
    if (NextPartEntry->Unpartitioned == TRUE)
      return NextPartEntry;
  }

  return NULL;
}


VOID
CreateNewPartition (PPARTLIST List,
                    ULONGLONG PartitionSize,
                    BOOLEAN AutoCreate)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  PPARTENTRY PrevPartEntry;
  PPARTENTRY NextPartEntry;
  PPARTENTRY NewPartEntry;

  if (List == NULL ||
      List->CurrentDisk == NULL ||
      List->CurrentPartition == NULL ||
      List->CurrentPartition->Unpartitioned == FALSE)
  {
    return;
  }

  DiskEntry = List->CurrentDisk;
  PartEntry = List->CurrentPartition;

  if (AutoCreate == TRUE ||
      PartitionSize == PartEntry->UnpartitionedLength)
  {
    /* Convert current entry to 'new (unformatted)' */
    PartEntry->FormatState = Unformatted;
    PartEntry->PartInfo[0].StartingOffset.QuadPart =
      PartEntry->UnpartitionedOffset + DiskEntry->TrackSize;
    PartEntry->PartInfo[0].PartitionLength.QuadPart =
      PartEntry->UnpartitionedLength - DiskEntry->TrackSize;
    PartEntry->PartInfo[0].PartitionType = PARTITION_ENTRY_UNUSED;
    PartEntry->PartInfo[0].BootIndicator = FALSE; /* FIXME */
    PartEntry->PartInfo[0].RewritePartition = TRUE;
    PartEntry->PartInfo[1].RewritePartition = TRUE;
    PartEntry->PartInfo[2].RewritePartition = TRUE;
    PartEntry->PartInfo[3].RewritePartition = TRUE;

    /* Get previous and next partition entries */
    PrevPartEntry = GetPrevPartitionedEntry (DiskEntry,
                                             PartEntry);
    NextPartEntry = GetNextPartitionedEntry (DiskEntry,
                                             PartEntry);

    if (PrevPartEntry != NULL && NextPartEntry != NULL)
    {
      /* Current entry is in the middle of the list */

      /* Copy previous container partition data to current entry */
      RtlCopyMemory (&PartEntry->PartInfo[1],
                     &PrevPartEntry->PartInfo[1],
                     sizeof(PARTITION_INFORMATION));
      PartEntry->PartInfo[1].RewritePartition = TRUE;

      /* Update previous container partition data */

      PrevPartEntry->PartInfo[1].StartingOffset.QuadPart =
        PartEntry->PartInfo[0].StartingOffset.QuadPart - DiskEntry->TrackSize;

      if (DiskEntry->PartListHead.Flink == &PrevPartEntry->ListEntry)
      {
        /* Special case - previous partition is first partition */
        PrevPartEntry->PartInfo[1].PartitionLength.QuadPart =
          DiskEntry->DiskSize - PrevPartEntry->PartInfo[1].StartingOffset.QuadPart;
      }
      else
      {
        PrevPartEntry->PartInfo[1].PartitionLength.QuadPart =
          PartEntry->PartInfo[0].PartitionLength.QuadPart + DiskEntry->TrackSize;
      }

      PrevPartEntry->PartInfo[1].RewritePartition = TRUE;
    }
    else if (PrevPartEntry == NULL && NextPartEntry != NULL)
    {
      /* Current entry is the first entry */
      return;
    }
    else if (PrevPartEntry != NULL && NextPartEntry == NULL)
    {
      /* Current entry is the last entry */

      PrevPartEntry->PartInfo[1].StartingOffset.QuadPart =
        PartEntry->PartInfo[0].StartingOffset.QuadPart - DiskEntry->TrackSize;

      if (DiskEntry->PartListHead.Flink == &PrevPartEntry->ListEntry)
      {
        /* Special case - previous partition is first partition */
        PrevPartEntry->PartInfo[1].PartitionLength.QuadPart =
          DiskEntry->DiskSize - PrevPartEntry->PartInfo[1].StartingOffset.QuadPart;
      }
      else
      {
        PrevPartEntry->PartInfo[1].PartitionLength.QuadPart =
          PartEntry->PartInfo[0].PartitionLength.QuadPart + DiskEntry->TrackSize;
      }

      if ((PartEntry->PartInfo[1].StartingOffset.QuadPart +
           PartEntry->PartInfo[1].PartitionLength.QuadPart) <
          (1024LL * 255LL * 63LL * 512LL))
      {
        PrevPartEntry->PartInfo[1].PartitionType = PARTITION_EXTENDED;
      }
      else
      {
        PrevPartEntry->PartInfo[1].PartitionType = PARTITION_XINT13_EXTENDED;
      }

      PrevPartEntry->PartInfo[1].BootIndicator = FALSE;
      PrevPartEntry->PartInfo[1].RewritePartition = TRUE;
    }

    PartEntry->AutoCreate = AutoCreate;
    PartEntry->New = TRUE;
    PartEntry->Unpartitioned = FALSE;
    PartEntry->UnpartitionedOffset = 0ULL;
    PartEntry->UnpartitionedLength = 0ULL;
  }
  else
  {
    /* Insert an initialize a new partition entry */
    NewPartEntry = (PPARTENTRY)RtlAllocateHeap (ProcessHeap,
                                                0,
                                                sizeof(PARTENTRY));
    if (NewPartEntry == NULL)
      return;

    RtlZeroMemory (NewPartEntry,
                   sizeof(PARTENTRY));

    /* Insert the new entry into the list */
    InsertTailList (&PartEntry->ListEntry,
                    &NewPartEntry->ListEntry);

    NewPartEntry->New = TRUE;

    NewPartEntry->FormatState = Unformatted;
    NewPartEntry->PartInfo[0].StartingOffset.QuadPart =
      PartEntry->UnpartitionedOffset + DiskEntry->TrackSize;
    NewPartEntry->PartInfo[0].PartitionLength.QuadPart =
      PartitionSize - DiskEntry->TrackSize;
    NewPartEntry->PartInfo[0].PartitionType = PARTITION_ENTRY_UNUSED;
    NewPartEntry->PartInfo[0].BootIndicator = FALSE; /* FIXME */
    NewPartEntry->PartInfo[0].RewritePartition = TRUE;
    NewPartEntry->PartInfo[1].RewritePartition = TRUE;
    NewPartEntry->PartInfo[2].RewritePartition = TRUE;
    NewPartEntry->PartInfo[3].RewritePartition = TRUE;

    /* Get previous and next partition entries */
    PrevPartEntry = GetPrevPartitionedEntry (DiskEntry,
                                             NewPartEntry);
    NextPartEntry = GetNextPartitionedEntry (DiskEntry,
                                             NewPartEntry);

    if (PrevPartEntry != NULL && NextPartEntry != NULL)
    {
      /* Current entry is in the middle of the list */

      /* Copy previous container partition data to current entry */
      RtlCopyMemory (&NewPartEntry->PartInfo[1],
                     &PrevPartEntry->PartInfo[1],
                     sizeof(PARTITION_INFORMATION));
      NewPartEntry->PartInfo[1].RewritePartition = TRUE;

      /* Update previous container partition data */

      PrevPartEntry->PartInfo[1].StartingOffset.QuadPart =
        NewPartEntry->PartInfo[0].StartingOffset.QuadPart - DiskEntry->TrackSize;

      if (DiskEntry->PartListHead.Flink == &PrevPartEntry->ListEntry)
      {
        /* Special case - previous partition is first partition */
        PrevPartEntry->PartInfo[1].PartitionLength.QuadPart =
          DiskEntry->DiskSize - PrevPartEntry->PartInfo[1].StartingOffset.QuadPart;
      }
      else
      {
        PrevPartEntry->PartInfo[1].PartitionLength.QuadPart =
          NewPartEntry->PartInfo[0].PartitionLength.QuadPart + DiskEntry->TrackSize;
      }

      PrevPartEntry->PartInfo[1].RewritePartition = TRUE;
    }
    else if (PrevPartEntry == NULL && NextPartEntry != NULL)
    {
      /* Current entry is the first entry */
      return;
    }
    else if (PrevPartEntry != NULL && NextPartEntry == NULL)
    {
      /* Current entry is the last entry */

      PrevPartEntry->PartInfo[1].StartingOffset.QuadPart =
        NewPartEntry->PartInfo[0].StartingOffset.QuadPart - DiskEntry->TrackSize;

      if (DiskEntry->PartListHead.Flink == &PrevPartEntry->ListEntry)
      {
        /* Special case - previous partition is first partition */
        PrevPartEntry->PartInfo[1].PartitionLength.QuadPart =
          DiskEntry->DiskSize - PrevPartEntry->PartInfo[1].StartingOffset.QuadPart;
      }
      else
      {
        PrevPartEntry->PartInfo[1].PartitionLength.QuadPart =
          NewPartEntry->PartInfo[0].PartitionLength.QuadPart + DiskEntry->TrackSize;
      }

      if ((PartEntry->PartInfo[1].StartingOffset.QuadPart +
           PartEntry->PartInfo[1].PartitionLength.QuadPart) <
          (1024LL * 255LL * 63LL * 512LL))
      {
        PrevPartEntry->PartInfo[1].PartitionType = PARTITION_EXTENDED;
      }
      else
      {
        PrevPartEntry->PartInfo[1].PartitionType = PARTITION_XINT13_EXTENDED;
      }

      PrevPartEntry->PartInfo[1].BootIndicator = FALSE;
      PrevPartEntry->PartInfo[1].RewritePartition = TRUE;
    }

    /* Update offset and size of the remaining unpartitioned disk space */
    PartEntry->UnpartitionedOffset += PartitionSize;
    PartEntry->UnpartitionedLength -= PartitionSize;
  }

  DiskEntry->Modified = TRUE;

  UpdatePartitionNumbers (DiskEntry);

  AssignDriverLetters (List);
}


VOID
DeleteCurrentPartition (PPARTLIST List)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  PPARTENTRY PrevPartEntry;
  PPARTENTRY NextPartEntry;

  if (List == NULL ||
      List->CurrentDisk == NULL ||
      List->CurrentPartition == NULL ||
      List->CurrentPartition->Unpartitioned == TRUE)
  {
    return;
  }

  DiskEntry = List->CurrentDisk;
  PartEntry = List->CurrentPartition;

  /* Adjust container partition entries */

  /* Get previous and next partition entries */
  PrevPartEntry = GetPrevPartitionedEntry (DiskEntry,
                                           PartEntry);
  NextPartEntry = GetNextPartitionedEntry (DiskEntry,
                                           PartEntry);

  if (PrevPartEntry != NULL && NextPartEntry != NULL)
  {
    /* Current entry is in the middle of the list */

    /*
     * The first extended partition can not be deleted
     * as long as other extended partitions are present.
     */
    if (PrevPartEntry->ListEntry.Blink == &DiskEntry->PartListHead)
      return;

    /* Copy previous container partition data to current entry */
    RtlCopyMemory (&PrevPartEntry->PartInfo[1],
                   &PartEntry->PartInfo[1],
                   sizeof(PARTITION_INFORMATION));
    PrevPartEntry->PartInfo[1].RewritePartition = TRUE;
  }
  else if (PrevPartEntry == NULL && NextPartEntry != NULL)
  {
    /*
     * A primary partition can not be deleted as long as
     * extended partitions are present.
     */
    return;
  }
  else if (PrevPartEntry != NULL && NextPartEntry == NULL)
  {
    /* Current entry is the last entry */
    RtlZeroMemory (&PrevPartEntry->PartInfo[1],
                   sizeof(PARTITION_INFORMATION));
    PrevPartEntry->PartInfo[1].RewritePartition = TRUE;
  }


  /* Adjust unpartitioned disk space entries */

  /* Get pointer to previous and next unpartitioned entries */
  PrevPartEntry = GetPrevUnpartitionedEntry (DiskEntry,
                                             PartEntry);

  NextPartEntry = GetNextUnpartitionedEntry (DiskEntry,
                                             PartEntry);

  if (PrevPartEntry != NULL && NextPartEntry != NULL)
  {
    /* Merge previous, current and next unpartitioned entry */

    /* Adjust the previous entries length */
    PrevPartEntry->UnpartitionedLength +=
      (PartEntry->PartInfo[0].PartitionLength.QuadPart + DiskEntry->TrackSize +
      NextPartEntry->UnpartitionedLength);

    /* Remove the current entry */
    RemoveEntryList (&PartEntry->ListEntry);
    RtlFreeHeap (ProcessHeap,
                 0,
                 PartEntry);

    /* Remove the next entry */
    RemoveEntryList (&NextPartEntry->ListEntry);
    RtlFreeHeap (ProcessHeap,
                 0,
    NextPartEntry);

    /* Update current partition */
    List->CurrentPartition = PrevPartEntry;
  }
  else if (PrevPartEntry != NULL && NextPartEntry == NULL)
  {
    /* Merge current and previous unpartitioned entry */

    /* Adjust the previous entries length */
    PrevPartEntry->UnpartitionedLength +=
      (PartEntry->PartInfo[0].PartitionLength.QuadPart + DiskEntry->TrackSize);

    /* Remove the current entry */
    RemoveEntryList (&PartEntry->ListEntry);
    RtlFreeHeap (ProcessHeap,
                 0,
                 PartEntry);

    /* Update current partition */
    List->CurrentPartition = PrevPartEntry;
  }
  else if (PrevPartEntry == NULL && NextPartEntry != NULL)
  {
    /* Merge current and next unpartitioned entry */

    /* Adjust the next entries offset and length */
    NextPartEntry->UnpartitionedOffset =
      PartEntry->PartInfo[0].StartingOffset.QuadPart - DiskEntry->TrackSize;
    NextPartEntry->UnpartitionedLength +=
      (PartEntry->PartInfo[0].PartitionLength.QuadPart + DiskEntry->TrackSize);

    /* Remove the current entry */
    RemoveEntryList (&PartEntry->ListEntry);
    RtlFreeHeap (ProcessHeap,
                 0,
                 PartEntry);

    /* Update current partition */
    List->CurrentPartition = NextPartEntry;
  }
  else
  {
    /* Nothing to merge but change current entry */
    PartEntry->New = FALSE;
    PartEntry->Unpartitioned = TRUE;
    PartEntry->UnpartitionedOffset =
      PartEntry->PartInfo[0].StartingOffset.QuadPart - DiskEntry->TrackSize;
    PartEntry->UnpartitionedLength =
      PartEntry->PartInfo[0].PartitionLength.QuadPart + DiskEntry->TrackSize;

    /* Wipe the partition table */
    RtlZeroMemory (&PartEntry->PartInfo,
                   sizeof(PartEntry->PartInfo));
  }

  DiskEntry->Modified = TRUE;

  UpdatePartitionNumbers (DiskEntry);

  AssignDriverLetters (List);
}


VOID
CheckActiveBootPartition (PPARTLIST List)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  PLIST_ENTRY ListEntry;
  UCHAR i;

  /* Check for empty disk list */
  if (IsListEmpty (&List->DiskListHead))
  {
    List->ActiveBootDisk = NULL;
    List->ActiveBootPartition = NULL;
    List->ActiveBootPartitionNumber = 0;
    return;
  }

#if 0
  if (List->ActiveBootDisk != NULL &&
      List->ActiveBootPartition != NULL)
  {
    /* We already have an active boot partition */
    return;
  }
#endif

  DiskEntry = CONTAINING_RECORD (List->DiskListHead.Flink,
                                 DISKENTRY,
                                 ListEntry);

  /* Check for empty partition list */
  if (IsListEmpty (&DiskEntry->PartListHead))
  {
    List->ActiveBootDisk = NULL;
    List->ActiveBootPartition = NULL;
    List->ActiveBootPartitionNumber = 0;
    return;
  }

  PartEntry = CONTAINING_RECORD (DiskEntry->PartListHead.Flink,
                                 PARTENTRY,
                                 ListEntry);

  /* Set active boot partition */
  if ((DiskEntry->NewDisk == TRUE) ||
      (PartEntry->PartInfo[0].BootIndicator == FALSE &&
       PartEntry->PartInfo[1].BootIndicator == FALSE &&
       PartEntry->PartInfo[2].BootIndicator == FALSE &&
       PartEntry->PartInfo[3].BootIndicator == FALSE))
  {
    PartEntry->PartInfo[0].BootIndicator = TRUE;
    PartEntry->PartInfo[0].RewritePartition = TRUE;
    DiskEntry->Modified = TRUE;

    /* FIXME: Might be incorrect if partitions were created by Linux FDISK */
    List->ActiveBootDisk = DiskEntry;
    List->ActiveBootPartition = PartEntry;
    List->ActiveBootPartitionNumber = 0;

    return;
  }

  /* Disk is not new, scan all partitions to find a bootable one */
  List->ActiveBootDisk = NULL;
  List->ActiveBootPartition = NULL;
  List->ActiveBootPartitionNumber = 0;

  ListEntry = DiskEntry->PartListHead.Flink;
  while (ListEntry != &DiskEntry->PartListHead)
  {
    PartEntry = CONTAINING_RECORD(ListEntry,
                                  PARTENTRY,
                                  ListEntry);

    /* Check if it's partitioned */
    if (!PartEntry->Unpartitioned)
    {
      /* Go through all of its 4 partitions */
      for (i=0; i<4; i++)
      {
        if (PartEntry->PartInfo[i].PartitionType != PARTITION_ENTRY_UNUSED &&
            PartEntry->PartInfo[i].BootIndicator)
        {
          /* Yes, we found it */
          List->ActiveBootDisk = DiskEntry;
          List->ActiveBootPartition = PartEntry;
          List->ActiveBootPartitionNumber = i;

          DPRINT("Found bootable partition disk %d, drive letter %c\n",
              DiskEntry->BiosDiskNumber, PartEntry->DriveLetter[i]);

          break;
        }
      }
    }
    /* Go to the next one */
    ListEntry = ListEntry->Flink;
  }
}


BOOLEAN
CheckForLinuxFdiskPartitions (PPARTLIST List)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  PLIST_ENTRY Entry1;
  PLIST_ENTRY Entry2;
  ULONG PartitionCount;
  ULONG i;

  Entry1 = List->DiskListHead.Flink;
  while (Entry1 != &List->DiskListHead)
  {
    DiskEntry = CONTAINING_RECORD (Entry1,
                                   DISKENTRY,
                                   ListEntry);

    Entry2 = DiskEntry->PartListHead.Flink;
    while (Entry2 != &DiskEntry->PartListHead)
    {
      PartEntry = CONTAINING_RECORD (Entry2,
                                     PARTENTRY,
                                     ListEntry);

      if (PartEntry->Unpartitioned == FALSE)
      {
        PartitionCount = 0;

        for (i = 0; i < 4; i++)
        {
          if (!IsContainerPartition (PartEntry->PartInfo[i].PartitionType) &&
              PartEntry->PartInfo[i].PartitionLength.QuadPart != 0ULL)
          {
            PartitionCount++;
          }
        }

        if (PartitionCount > 1)
        {
          return TRUE;
        }
      }

      Entry2 = Entry2->Flink;
    }

    Entry1 = Entry1->Flink;
  }

  return FALSE;
}


BOOLEAN
WritePartitionsToDisk (PPARTLIST List)
{
  PDRIVE_LAYOUT_INFORMATION DriveLayout;
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK Iosb;
  WCHAR SrcPath[MAX_PATH];
  WCHAR DstPath[MAX_PATH];
  UNICODE_STRING Name;
  HANDLE FileHandle;
  PDISKENTRY DiskEntry1;
  PDISKENTRY DiskEntry2;
  PPARTENTRY PartEntry;
  PLIST_ENTRY Entry1;
  PLIST_ENTRY Entry2;
  ULONG PartitionCount;
  ULONG DriveLayoutSize;
  ULONG Index;
  NTSTATUS Status;

  if (List == NULL)
  {
    return TRUE;
  }

  Entry1 = List->DiskListHead.Flink;
  while (Entry1 != &List->DiskListHead)
  {
    DiskEntry1 = CONTAINING_RECORD (Entry1,
                                    DISKENTRY,
                                    ListEntry);

    if (DiskEntry1->Modified == TRUE)
    {
      /* Count partitioned entries */
      PartitionCount = 0;
      Entry2 = DiskEntry1->PartListHead.Flink;
      while (Entry2 != &DiskEntry1->PartListHead)
      {
        PartEntry = CONTAINING_RECORD (Entry2,
                                       PARTENTRY,
                                       ListEntry);
        if (PartEntry->Unpartitioned == FALSE)
        {
          PartitionCount += 4;
        }

        Entry2 = Entry2->Flink;
      }
      if (PartitionCount == 0)
      {
        DriveLayoutSize = sizeof (DRIVE_LAYOUT_INFORMATION) +
          ((4 - 1) * sizeof (PARTITION_INFORMATION));
      }
      else
      {
        DriveLayoutSize = sizeof (DRIVE_LAYOUT_INFORMATION) +
          ((PartitionCount - 1) * sizeof (PARTITION_INFORMATION));
      }
      DriveLayout = (PDRIVE_LAYOUT_INFORMATION)RtlAllocateHeap (ProcessHeap,
                                                                0,
                                                                DriveLayoutSize);
      if (DriveLayout == NULL)
      {
        DPRINT1 ("RtlAllocateHeap() failed\n");
        return FALSE;
      }

      RtlZeroMemory (DriveLayout,
                     DriveLayoutSize);

      if (PartitionCount == 0)
      {
        /* delete all partitions in the mbr */
        DriveLayout->PartitionCount = 4;
        for (Index = 0; Index < 4; Index++)
        {
          DriveLayout->PartitionEntry[Index].RewritePartition = TRUE;
        }
      }
      else
      {
        DriveLayout->PartitionCount = PartitionCount;

        Index = 0;
        Entry2 = DiskEntry1->PartListHead.Flink;
        while (Entry2 != &DiskEntry1->PartListHead)
        {
          PartEntry = CONTAINING_RECORD (Entry2,
                                         PARTENTRY,
                                         ListEntry);
          if (PartEntry->Unpartitioned == FALSE)
          {
            RtlCopyMemory (&DriveLayout->PartitionEntry[Index],
                           &PartEntry->PartInfo[0],
                           4 * sizeof (PARTITION_INFORMATION));
            Index += 4;
          }

          Entry2 = Entry2->Flink;
        }
      }
      if (DiskEntry1->Signature == 0)
      {
        LARGE_INTEGER SystemTime;
        TIME_FIELDS TimeFields;
        PUCHAR Buffer;
        Buffer = (PUCHAR)&DiskEntry1->Signature;

        while (1)
        {
          NtQuerySystemTime (&SystemTime);
          RtlTimeToTimeFields (&SystemTime, &TimeFields);

          Buffer[0] = (UCHAR)(TimeFields.Year & 0xFF) + (UCHAR)(TimeFields.Hour & 0xFF);
          Buffer[1] = (UCHAR)(TimeFields.Year >> 8) + (UCHAR)(TimeFields.Minute & 0xFF);
          Buffer[2] = (UCHAR)(TimeFields.Month & 0xFF) + (UCHAR)(TimeFields.Second & 0xFF);
          Buffer[3] = (UCHAR)(TimeFields.Day & 0xFF) + (UCHAR)(TimeFields.Milliseconds & 0xFF);

          if (DiskEntry1->Signature == 0)
          {
            continue;
          }

          /* check if the signature already exist */
          /* FIXME:
           *   Check also signatures from disks, which are
           *   not visible (bootable) by the bios.
           */
          Entry2 = List->DiskListHead.Flink;
          while (Entry2 != &List->DiskListHead)
          {
            DiskEntry2 = CONTAINING_RECORD(Entry2, DISKENTRY, ListEntry);
            if (DiskEntry1 != DiskEntry2 &&
                DiskEntry1->Signature == DiskEntry2->Signature)
            {
              break;
            }
            Entry2 = Entry2->Flink;
          }
          if (Entry2 == &List->DiskListHead)
          {
            break;
          }
        }

        /* set one partition entry to dirty, this will update the signature */
        DriveLayout->PartitionEntry[0].RewritePartition = TRUE;

      }

      DriveLayout->Signature = DiskEntry1->Signature;


      swprintf (DstPath,
                L"\\Device\\Harddisk%d\\Partition0",
                DiskEntry1->DiskNumber);
      RtlInitUnicodeString (&Name,
                            DstPath);
      InitializeObjectAttributes (&ObjectAttributes,
                                  &Name,
                                  0,
                                  NULL,
                                  NULL);

      Status = NtOpenFile (&FileHandle,
                           FILE_ALL_ACCESS,
                           &ObjectAttributes,
                           &Iosb,
                           0,
                           FILE_SYNCHRONOUS_IO_NONALERT);

      if (!NT_SUCCESS (Status))
      {
        DPRINT1 ("NtOpenFile() failed (Status %lx)\n", Status);
        return FALSE;
      }

      Status = NtDeviceIoControlFile (FileHandle,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &Iosb,
                                      IOCTL_DISK_SET_DRIVE_LAYOUT,
                                      DriveLayout,
                                      DriveLayoutSize,
                                      NULL,
                                      0);
      if (!NT_SUCCESS (Status))
      {
        DPRINT1 ("NtDeviceIoControlFile() failed (Status %lx)\n", Status);
        NtClose (FileHandle);
        return FALSE;
      }

      RtlFreeHeap (ProcessHeap,
                   0,
                   DriveLayout);

      NtClose (FileHandle);

      /* Install MBR code if the disk is new */
      if (DiskEntry1->NewDisk == TRUE &&
          DiskEntry1->BiosDiskNumber == 0)
      {
        wcscpy (SrcPath, SourceRootPath.Buffer);
        wcscat (SrcPath, L"\\loader\\dosmbr.bin");

        DPRINT ("Install MBR bootcode: %S ==> %S\n",
                SrcPath, DstPath);

        /* Install MBR bootcode */
        Status = InstallMbrBootCodeToDisk (SrcPath,
                                           DstPath);
        if (!NT_SUCCESS (Status))
        {
          DPRINT1 ("InstallMbrBootCodeToDisk() failed (Status %lx)\n",
                   Status);
          return FALSE;
        }

        DiskEntry1->NewDisk = FALSE;
        DiskEntry1->NoMbr = FALSE;
      }
    }

    Entry1 = Entry1->Flink;
  }

  return TRUE;
}

BOOL SetMountedDeviceValues(PPARTLIST List)
{
  PLIST_ENTRY Entry1, Entry2;
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  UCHAR i;

  if (List == NULL)
  {
    return FALSE;
  }

  Entry1 = List->DiskListHead.Flink;
  while (Entry1 != &List->DiskListHead)
  {
    DiskEntry = CONTAINING_RECORD (Entry1,
                                   DISKENTRY,
                                   ListEntry);

    Entry2 = DiskEntry->PartListHead.Flink;
    while (Entry2 != &DiskEntry->PartListHead)
    {
      PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
      if (!PartEntry->Unpartitioned)
      {
        for (i=0; i<4; i++)
        {
          if (PartEntry->DriveLetter[i])
          {
            if (!SetMountedDeviceValue(PartEntry->DriveLetter[i], DiskEntry->Signature, PartEntry->PartInfo[i].StartingOffset))
            {
              return FALSE;
            }
          }
        }
      }
      Entry2 = Entry2->Flink;
    }
    Entry1 = Entry1->Flink;
  }
  return TRUE;
}



/* EOF */
