/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2003 ReactOS Team
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
/* $Id: partlist.c,v 1.18 2003/08/18 17:39:26 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/partlist.c
 * PURPOSE:         Partition list functions
 * PROGRAMMER:      Eric Kohl
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include <ddk/ntddk.h>
#include <ddk/ntddscsi.h>

#include <ntdll/rtl.h>

#include <ntos/minmax.h>

#include "usetup.h"
#include "console.h"
#include "partlist.h"
#include "drivesup.h"


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
  PLIST_ENTRY Entry2;
  CHAR Letter;

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
	}

      Entry1 = Entry1->Flink;
    }


  /* Assign drive letters to logical drives */
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
	    ROUND_DOWN (DiskEntry->DiskSize - (LastStartingOffset + LastPartitionLength),
			DiskEntry->CylinderSize);

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

  DiskEntry = (PDISKENTRY)RtlAllocateHeap (ProcessHeap,
					   0,
					   sizeof(DISKENTRY));
  if (DiskEntry == NULL)
    {
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

  DiskEntry->DiskSize =
    DiskGeometry.Cylinders.QuadPart *
    (ULONGLONG)DiskGeometry.TracksPerCylinder *
    (ULONGLONG)DiskGeometry.SectorsPerTrack *
    (ULONGLONG)DiskGeometry.BytesPerSector;
  DiskEntry->CylinderSize =
    (ULONGLONG)DiskGeometry.TracksPerCylinder *
    (ULONGLONG)DiskGeometry.SectorsPerTrack *
    (ULONGLONG)DiskGeometry.BytesPerSector;
  DiskEntry->TrackSize =
    (ULONGLONG)DiskGeometry.SectorsPerTrack *
    (ULONGLONG)DiskGeometry.BytesPerSector;

  DiskEntry->DiskNumber = DiskNumber;
  DiskEntry->Port = ScsiAddress.PortNumber;
  DiskEntry->Bus = ScsiAddress.PathId;
  DiskEntry->Id = ScsiAddress.TargetId;

  DiskEntry->UseLba = (DiskEntry->DiskSize > (1024ULL * 255ULL * 63ULL * 512ULL));

  GetDriverName (DiskEntry);

  InsertTailList (&List->DiskListHead,
		  &DiskEntry->ListEntry);

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
  DISK_GEOMETRY DiskGeometry;
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

  InitializeListHead (&List->DiskListHead);

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
			   FILE_GENERIC_READ,
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
	}
      else
	{
	  List->CurrentPartition =
	    CONTAINING_RECORD (List->CurrentDisk->PartListHead.Flink,
			       PARTENTRY,
			       ListEntry);
	}
    }

  return List;
}


VOID
DestroyPartitionList (PPARTLIST List)
{
  PDISKENTRY DiskEntry;
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

  /* Release list head */
  RtlFreeHeap (ProcessHeap, 0, List);
}


static VOID
PrintEmptyLine (PPARTLIST List)
{
  COORD coPos;
  ULONG Written;
  USHORT Width;
  USHORT Height;

  Width = List->Right - List->Left - 1;
  Height = List->Bottom - List->Top - 1;

  if (List->Line < 0 || List->Line > Height)
    return;

  coPos.X = List->Left + 1;
  coPos.Y = List->Top + 1 + List->Line;

  FillConsoleOutputAttribute (0x17,
			      Width,
			      coPos,
			      &Written);

  FillConsoleOutputCharacter (' ',
			      Width,
			      coPos,
			      &Written);

  List->Line++;
}


static VOID
PrintPartitionData (PPARTLIST List,
		    PDISKENTRY DiskEntry,
		    PPARTENTRY PartEntry)
{
  CHAR LineBuffer[128];
  COORD coPos;
  ULONG Written;
  USHORT Width;
  USHORT Height;

  ULONGLONG PartSize;
  PCHAR Unit;
  UCHAR Attribute;
  PCHAR PartType;

  Width = List->Right - List->Left - 1;
  Height = List->Bottom - List->Top - 1;

  if (List->Line < 0 || List->Line > Height)
    return;

  coPos.X = List->Left + 1;
  coPos.Y = List->Top + 1 + List->Line;

  if (PartEntry->Unpartitioned == TRUE)
    {
#if 0
      if (PartEntry->UnpartitionledLength >= 0x280000000ULL) /* 10 GB */
	{
	  PartSize = (PartEntry->UnpartitionedLength + (1 << 29)) >> 30;
	  Unit = "GB";
	}
      else
#endif
      if (PartEntry->UnpartitionedLength >= 0xA00000ULL) /* 10 MB */
	{
	  PartSize = (PartEntry->UnpartitionedLength + (1 << 19)) >> 20;
	  Unit = "MB";
	}
      else
	{
	  PartSize = (PartEntry->UnpartitionedLength + (1 << 9)) >> 10;
	  Unit = "KB";
	}

      sprintf (LineBuffer,
	       "    Unpartitioned space              %6I64u %s",
	       PartSize,
	       Unit);
    }
  else
    {
      /* Determine partition type */
      PartType = NULL;
      if (PartEntry->New == TRUE)
	{
	  PartType = "New (Unformatted)";
	}
      else if (PartEntry->Unpartitioned == FALSE)
	{
	  if ((PartEntry->PartInfo[0].PartitionType == PARTITION_FAT_12) ||
	      (PartEntry->PartInfo[0].PartitionType == PARTITION_FAT_16) ||
	      (PartEntry->PartInfo[0].PartitionType == PARTITION_HUGE) ||
	      (PartEntry->PartInfo[0].PartitionType == PARTITION_XINT13))
	    {
	      PartType = "FAT";
	    }
	  else if ((PartEntry->PartInfo[0].PartitionType == PARTITION_FAT32) ||
		   (PartEntry->PartInfo[0].PartitionType == PARTITION_FAT32_XINT13))
	    {
	      PartType = "FAT32";
	    }
	  else if (PartEntry->PartInfo[0].PartitionType == PARTITION_IFS)
	    {
	      PartType = "NTFS"; /* FIXME: Not quite correct! */
	    }
	}

#if 0
      if (PartEntry->PartInfo[0].PartitionLength.QuadPart >= 0x280000000ULL) /* 10 GB */
	{
	  PartSize = (PartEntry->PartInfo[0].PartitionLength.QuadPart + (1 << 29)) >> 30;
	  Unit = "GB";
	}
      else
#endif
      if (PartEntry->PartInfo[0].PartitionLength.QuadPart >= 0xA00000ULL) /* 10 MB */
	{
	  PartSize = (PartEntry->PartInfo[0].PartitionLength.QuadPart + (1 << 19)) >> 20;
	  Unit = "MB";
	}
      else
	{
	  PartSize = (PartEntry->PartInfo[0].PartitionLength.QuadPart + (1 << 9)) >> 10;
	  Unit = "KB";
	}

      if (PartType == NULL)
	{
	  sprintf (LineBuffer,
		   "%c%c  Type %-3lu                         %6I64u %s",
		   (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter,
		   (PartEntry->DriveLetter == 0) ? '-' : ':',
		   PartEntry->PartInfo[0].PartitionType,
		   PartSize,
		   Unit);
	}
      else
	{
	  sprintf (LineBuffer,
		   "%c%c  %-24s         %6I64u %s",
		   (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter,
		   (PartEntry->DriveLetter == 0) ? '-' : ':',
		   PartType,
		   PartSize,
		   Unit);
	}
    }

  Attribute = (List->CurrentDisk == DiskEntry &&
	       List->CurrentPartition == PartEntry) ? 0x71 : 0x17;

  FillConsoleOutputCharacter (' ',
			      Width,
			      coPos,
			      &Written);

  coPos.X += 4;
  Width -= 8;
  FillConsoleOutputAttribute (Attribute,
			      Width,
			      coPos,
			      &Written);

  coPos.X++;
  Width -= 2;
  WriteConsoleOutputCharacters (LineBuffer,
				min (strlen (LineBuffer), Width),
				coPos);

  List->Line++;
}


static VOID
PrintDiskData (PPARTLIST List,
	       PDISKENTRY DiskEntry)
{
  PPARTENTRY PartEntry;
  PLIST_ENTRY Entry;
  CHAR LineBuffer[128];
  COORD coPos;
  ULONG Written;
  USHORT Width;
  USHORT Height;
  ULONGLONG DiskSize;
  PCHAR Unit;
  SHORT PartIndex;

  Width = List->Right - List->Left - 1;
  Height = List->Bottom - List->Top - 1;

  if (List->Line < 0 || List->Line > Height)
    return;

  coPos.X = List->Left + 1;
  coPos.Y = List->Top + 1 + List->Line;

#if 0
  if (DiskEntry->DiskSize >= 0x280000000ULL) /* 10 GB */
    {
      DiskSize = (DiskEntry->DiskSize + (1 << 29)) >> 30;
      Unit = "GB";
    }
  else
#endif
    {
      DiskSize = (DiskEntry->DiskSize + (1 << 19)) >> 20;
      if (DiskSize == 0)
	DiskSize = 1;
      Unit = "MB";
    }

  if (DiskEntry->DriverName.Length > 0)
    {
      sprintf (LineBuffer,
	       "%6I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ",
	       DiskSize,
	       Unit,
	       DiskEntry->DiskNumber,
	       DiskEntry->Port,
	       DiskEntry->Bus,
	       DiskEntry->Id,
	       &DiskEntry->DriverName);
    }
  else
    {
      sprintf (LineBuffer,
	       "%6I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)",
	       DiskSize,
	       Unit,
	       DiskEntry->DiskNumber,
	       DiskEntry->Port,
	       DiskEntry->Bus,
	       DiskEntry->Id);
    }

  FillConsoleOutputAttribute (0x17,
			      Width,
			      coPos,
			      &Written);

  FillConsoleOutputCharacter (' ',
			      Width,
			      coPos,
			      &Written);

  coPos.X++;
  WriteConsoleOutputCharacters (LineBuffer,
				min (strlen (LineBuffer), Width - 2),
				coPos);

  List->Line++;

  /* Print separator line */
  PrintEmptyLine (List);

  /* Print partition lines*/
  Entry = DiskEntry->PartListHead.Flink;
  while (Entry != &DiskEntry->PartListHead)
    {
      PartEntry = CONTAINING_RECORD (Entry, PARTENTRY, ListEntry);

      /* Print disk entry */
      PrintPartitionData (List,
			  DiskEntry,
			  PartEntry);

      Entry = Entry->Flink;
    }

  /* Print separator line */
  PrintEmptyLine (List);
}


VOID
DrawPartitionList (PPARTLIST List)
{
  PLIST_ENTRY Entry;
  PDISKENTRY DiskEntry;
  CHAR LineBuffer[128];
  COORD coPos;
  ULONG Written;
  SHORT i;
  SHORT DiskIndex;

  /* draw upper left corner */
  coPos.X = List->Left;
  coPos.Y = List->Top;
  FillConsoleOutputCharacter (0xDA, // '+',
			      1,
			      coPos,
			      &Written);

  /* draw upper edge */
  coPos.X = List->Left + 1;
  coPos.Y = List->Top;
  FillConsoleOutputCharacter (0xC4, // '-',
			      List->Right - List->Left - 1,
			      coPos,
			      &Written);

  /* draw upper right corner */
  coPos.X = List->Right;
  coPos.Y = List->Top;
  FillConsoleOutputCharacter (0xBF, // '+',
			      1,
			      coPos,
			      &Written);

  /* draw left and right edge */
  for (i = List->Top + 1; i < List->Bottom; i++)
    {
      coPos.X = List->Left;
      coPos.Y = i;
      FillConsoleOutputCharacter (0xB3, // '|',
				  1,
				  coPos,
				  &Written);

      coPos.X = List->Right;
      FillConsoleOutputCharacter (0xB3, //'|',
				  1,
				  coPos,
				  &Written);
    }

  /* draw lower left corner */
  coPos.X = List->Left;
  coPos.Y = List->Bottom;
  FillConsoleOutputCharacter (0xC0, // '+',
			      1,
			      coPos,
			      &Written);

  /* draw lower edge */
  coPos.X = List->Left + 1;
  coPos.Y = List->Bottom;
  FillConsoleOutputCharacter (0xC4, // '-',
			      List->Right - List->Left - 1,
			      coPos,
			      &Written);

  /* draw lower right corner */
  coPos.X = List->Right;
  coPos.Y = List->Bottom;
  FillConsoleOutputCharacter (0xD9, // '+',
			      1,
			      coPos,
			      &Written);

  /* print list entries */
  List->Line = 0;

  Entry = List->DiskListHead.Flink;
  while (Entry != &List->DiskListHead)
    {
      DiskEntry = CONTAINING_RECORD (Entry, DISKENTRY, ListEntry);

      /* Print disk entry */
      PrintDiskData (List,
		     DiskEntry);

      Entry = Entry->Flink;
    }
}


VOID
ScrollDownPartitionList (PPARTLIST List)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  PLIST_ENTRY Entry1;
  PLIST_ENTRY Entry2;

  /* Check for empty disks */
  if (IsListEmpty (&List->DiskListHead))
    return;

  /* Check for next usable entry on current disk */
  if (List->CurrentPartition != NULL)
    {
      Entry2 = List->CurrentPartition->ListEntry.Flink;
      while (Entry2 != &List->CurrentDisk->PartListHead)
	{
	  PartEntry = CONTAINING_RECORD (Entry2, PARTENTRY, ListEntry);

//	  if (PartEntry->HidePartEntry == FALSE)
	    {
	      List->CurrentPartition = PartEntry;
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
  ULONG i;

  /* Check for empty disks */
  if (IsListEmpty (&List->DiskListHead))
    return;

  /* check for previous usable entry on current disk */
  if (List->CurrentPartition != NULL)
    {
      Entry2 = List->CurrentPartition->ListEntry.Blink;
      while (Entry2 != &List->CurrentDisk->PartListHead)
	{
	  PartEntry = CONTAINING_RECORD (Entry2, PARTENTRY, ListEntry);

//	  if (PartEntry->HidePartEntry == FALSE)
	    {
	      List->CurrentPartition = PartEntry;
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
		  DrawPartitionList (List);
		  return;
		}

	      Entry2 = Entry2->Blink;
	    }

	  Entry1 = Entry1->Blink;
	}
    }
}


VOID
SetActiveBootPartition (PPARTLIST List)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;

  /* Check for empty disk list */
  if (IsListEmpty (&List->DiskListHead))
    {
      List->ActiveBootDisk = NULL;
      List->ActiveBootPartition = NULL;
      return;
    }

  DiskEntry = CONTAINING_RECORD (List->DiskListHead.Flink,
				 DISKENTRY,
				 ListEntry);

  /* Check for empty partition list */
  if (IsListEmpty (&DiskEntry->PartListHead))
    {
      List->ActiveBootDisk = NULL;
      List->ActiveBootPartition = NULL;
      return;
    }

  PartEntry = CONTAINING_RECORD (DiskEntry->PartListHead.Flink,
				 PARTENTRY,
				 ListEntry);

  /* Set active boot partition 1 */
  if (PartEntry->PartInfo[0].BootIndicator == FALSE &&
      PartEntry->PartInfo[1].BootIndicator == FALSE &&
      PartEntry->PartInfo[2].BootIndicator == FALSE &&
      PartEntry->PartInfo[3].BootIndicator == FALSE)
    {
      PartEntry->PartInfo[0].BootIndicator == TRUE;
      PartEntry->PartInfo[0].RewritePartition == TRUE;
      DiskEntry->Modified = TRUE;
    }

  /* FIXME: Might be incorrect if partitions were created by Linux FDISK */
  List->ActiveBootDisk = DiskEntry;
  List->ActiveBootPartition = PartEntry;
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

	  /* FIXME: Update extended partition entries */
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

	  PrevPartEntry->PartInfo[1].PartitionType =
	    (DiskEntry->UseLba == TRUE) ? PARTITION_XINT13_EXTENDED : PARTITION_EXTENDED;
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

	  /* FIXME: Update extended partition entries */
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

	  PrevPartEntry->PartInfo[1].PartitionType =
	    (DiskEntry->UseLba == TRUE) ? PARTITION_XINT13_EXTENDED : PARTITION_EXTENDED;
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
      /* Copy previous container partition data to current entry */
      RtlCopyMemory (&PrevPartEntry->PartInfo[1],
		     &PartEntry->PartInfo[1],
		     sizeof(PARTITION_INFORMATION));
      PrevPartEntry->PartInfo[1].RewritePartition = TRUE;
    }
  else if (PrevPartEntry == NULL && NextPartEntry != NULL)
    {
      /* Current entry is the first entry */

      /* FIXME: Update extended partition entries */
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


BOOLEAN
WritePartitionsToDisk (PPARTLIST List)
{
  PDRIVE_LAYOUT_INFORMATION DriveLayout;
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK Iosb;
  WCHAR Buffer[MAX_PATH];
  UNICODE_STRING Name;
  HANDLE FileHandle;
  PDISKENTRY DiskEntry;
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
      DiskEntry = CONTAINING_RECORD (Entry1,
				     DISKENTRY,
				     ListEntry);

      if (DiskEntry->Modified == TRUE)
	{
	  /* Count partitioned entries */
	  PartitionCount = 0;
	  Entry2 = DiskEntry->PartListHead.Flink;
	  while (Entry2 != &DiskEntry->PartListHead)
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

	  if (PartitionCount > 0)
	    {
	      DriveLayoutSize = sizeof (DRIVE_LAYOUT_INFORMATION) +
		((PartitionCount - 1) * sizeof (PARTITION_INFORMATION));
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

	      DriveLayout->PartitionCount = PartitionCount;

	      Index = 0;
	      Entry2 = DiskEntry->PartListHead.Flink;
	      while (Entry2 != &DiskEntry->PartListHead)
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

	      swprintf (Buffer,
			L"\\Device\\Harddisk%d\\Partition0",
			DiskEntry->DiskNumber);
	      RtlInitUnicodeString (&Name,
				    Buffer);
	      InitializeObjectAttributes (&ObjectAttributes,
					  &Name,
					  0,
					  NULL,
					  NULL);

	      Status = NtOpenFile (&FileHandle,
				   FILE_GENERIC_WRITE,
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

	      /* FIXME: Install MBR code */

	      NtClose (FileHandle);
	    }
	}

      Entry1 = Entry1->Flink;
    }

  return TRUE;
}

/* EOF */
