/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/* $Id: partlist.c,v 1.12 2003/08/03 12:20:22 ekohl Exp $
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
GetDriverName(PDISKENTRY DiskEntry)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  WCHAR KeyName[32];
  NTSTATUS Status;

  RtlInitUnicodeString(&DiskEntry->DriverName,
		       NULL);

  swprintf(KeyName,
	   L"\\Scsi\\Scsi Port %lu",
	   DiskEntry->Port);

  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].Name = L"Driver";
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[0].EntryContext = &DiskEntry->DriverName;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_DEVICEMAP,
				  KeyName,
				  QueryTable,
				  NULL,
				  NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
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

      PartEntry->DriveLetter = GetDriveLetter(DiskNumber,
					      LayoutBuffer->PartitionEntry[i].PartitionNumber);

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
	      if ((!IsContainerPartition(PartEntry->PartInfo[j].PartitionType)) &&
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
	  LastUnusedPartitionLength =
	    DiskEntry->DiskSize - (LastStartingOffset + LastPartitionLength);

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
  if (!NT_SUCCESS(Status))
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

  DPRINT("Cylinders %d\n", DiskEntry->Cylinders);
  DPRINT("TracksPerCylinder %d\n", DiskEntry->TracksPerCylinder);
  DPRINT("SectorsPerTrack %d\n", DiskEntry->SectorsPerTrack);
  DPRINT("BytesPerSector %d\n", DiskEntry->BytesPerSector);

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
InitializePartitionList(VOID)
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

  List = (PPARTLIST)RtlAllocateHeap(ProcessHeap, 0, sizeof(PARTLIST));
  if (List == NULL)
    return(NULL);

  List->Left = 0;
  List->Top = 0;
  List->Right = 0;
  List->Bottom = 0;

  List->Line = 0;

  List->TopDisk = (ULONG)-1;
  List->TopPartition = (ULONG)-1;

  List->CurrentDisk = NULL;
  List->CurrentPartition = NULL;

  InitializeListHead (&List->DiskListHead);

  Status = NtQuerySystemInformation(SystemDeviceInformation,
				    &Sdi,
				    sizeof(SYSTEM_DEVICE_INFORMATION),
				    &ReturnSize);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeHeap(ProcessHeap, 0, List);
      return(NULL);
    }

  for (DiskNumber = 0; DiskNumber < Sdi.NumberOfDisks; DiskNumber++)
    {
      swprintf(Buffer,
	       L"\\Device\\Harddisk%d\\Partition0",
	       DiskNumber);
      RtlInitUnicodeString(&Name,
			   Buffer);

      InitializeObjectAttributes(&ObjectAttributes,
				 &Name,
				 0,
				 NULL,
				 NULL);

      Status = NtOpenFile(&FileHandle,
			  0x10001,
			  &ObjectAttributes,
			  &Iosb,
			  1,
			  FILE_SYNCHRONOUS_IO_NONALERT);
      if (NT_SUCCESS(Status))
	{
	  AddDiskToList (FileHandle,
			 DiskNumber,
			 List);

	  NtClose(FileHandle);
	}
    }

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
      List->CurrentDisk = CONTAINING_RECORD(List->DiskListHead.Flink, DISKENTRY, ListEntry);

      if (IsListEmpty (&List->CurrentDisk->PartListHead))
	{
	  List->CurrentPartition = 0;
	}
      else
	{
	  List->CurrentPartition = CONTAINING_RECORD(List->CurrentDisk->PartListHead.Flink, PARTENTRY, ListEntry);
	}
    }

  return(List);
}


PPARTLIST
CreatePartitionList(SHORT Left,
		    SHORT Top,
		    SHORT Right,
		    SHORT Bottom)
{
  PPARTLIST List;

  List = InitializePartitionList();
  if (List == NULL)
    return(NULL);

  List->Left = Left;
  List->Top = Top;
  List->Right = Right;
  List->Bottom = Bottom;

  DrawPartitionList(List);

  return(List);
}


PPARTENTRY
GetPartitionInformation(PPARTLIST List,
			ULONG DiskNumber,
			ULONG PartitionNumber,
			PULONG PartEntryNumber)
{
  PPARTENTRY PartEntry;
  ULONG i;

  if (IsListEmpty(&List->DiskListHead))
    {
      return NULL;
    }

#if 0
  if (DiskNumber >= List->DiskCount)
    {
      return NULL;
    }

  if (PartitionNumber >= List->DiskArray[DiskNumber].PartCount)
    {
      return NULL;
    }

  if (List->DiskArray[DiskNumber].FixedDisk != TRUE)
    {
      return NULL;
    }

  for (i = 0; i < List->DiskArray[DiskNumber].PartCount; i++)
    {
      PartEntry = &List->DiskArray[DiskNumber].PartArray[i];
      if (PartEntry->PartNumber == PartitionNumber)
        {
          *PartEntryNumber = i;
          return PartEntry;
        }
    }
#endif
  return NULL;
}


VOID
DestroyPartitionList(PPARTLIST List)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  PLIST_ENTRY Entry;
#if 0
  COORD coPos;
  USHORT Width;

  /* clear occupied screen area */
  coPos.X = List->Left;
  Width = List->Right - List->Left + 1;
  for (coPos.Y = List->Top; coPos.Y <= List->Bottom; coPos.Y++)
    {
      FillConsoleOutputAttribute(0x17,
				 Width,
				 coPos,
				 &i);

      FillConsoleOutputCharacter(' ',
				 Width,
				 coPos,
				 &i);
    }
#endif

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
PrintEmptyLine(PPARTLIST List)
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

  FillConsoleOutputAttribute(0x17,
			     Width,
			     coPos,
			     &Written);

  FillConsoleOutputCharacter(' ',
			     Width,
			     coPos,
			     &Written);

  List->Line++;
}


static VOID
PrintPartitionData(PPARTLIST List,
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

      sprintf(LineBuffer,
	      "    Unpartitioned space              %6I64u %s",
	      PartSize,
	      Unit);
    }
  else
    {

      /* Determine partition type */
      PartType = NULL;
      if (PartEntry->Unpartitioned == FALSE)
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

      if (PartEntry->DriveLetter != (CHAR)0)
	{
	  if (PartType == NULL)
	    {
	      sprintf(LineBuffer,
		      "%c:  Type %-3lu                        %6I64u %s",
		      PartEntry->DriveLetter,
		      PartEntry->PartInfo[0].PartitionType,
		      PartSize,
		      Unit);
	    }
	  else
	    {
	      sprintf(LineBuffer,
		      "%c:  %-8s                         %6I64u %s",
		      PartEntry->DriveLetter,
		      PartType,
		      PartSize,
		      Unit);
	    }
	}
      else
	{
	  sprintf(LineBuffer,
		  "--  %-8s  Type %-3lu   %6I64u %s",
		  PartEntry->FileSystemName,
		  PartEntry->PartInfo[0].PartitionType,
		  PartSize,
		  Unit);
	}
    }

  Attribute = (List->CurrentDisk == DiskEntry &&
	       List->CurrentPartition == PartEntry) ? 0x71 : 0x17;

  FillConsoleOutputCharacter(' ',
			     Width,
			     coPos,
			     &Written);

  coPos.X += 4;
  Width -= 8;
  FillConsoleOutputAttribute(Attribute,
			     Width,
			     coPos,
			     &Written);

  coPos.X++;
  Width -= 2;
  WriteConsoleOutputCharacters(LineBuffer,
			       min(strlen(LineBuffer), Width),
			       coPos);

  List->Line++;
}


static VOID
PrintDiskData(PPARTLIST List,
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
      sprintf(LineBuffer,
	      "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ",
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
      sprintf(LineBuffer,
	      "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)",
	      DiskSize,
	      Unit,
	      DiskEntry->DiskNumber,
	      DiskEntry->Port,
	      DiskEntry->Bus,
	      DiskEntry->Id);
    }

  FillConsoleOutputAttribute(0x17,
			     Width,
			     coPos,
			     &Written);

  FillConsoleOutputCharacter(' ',
			     Width,
			     coPos,
			     &Written);

  coPos.X++;
  WriteConsoleOutputCharacters(LineBuffer,
			       min(strlen(LineBuffer), Width - 2),
			       coPos);

  List->Line++;

  /* Print separator line */
  PrintEmptyLine(List);

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
  PrintEmptyLine(List);
}


VOID
DrawPartitionList(PPARTLIST List)
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
  FillConsoleOutputCharacter(0xDA, // '+',
			     1,
			     coPos,
			     &Written);

  /* draw upper edge */
  coPos.X = List->Left + 1;
  coPos.Y = List->Top;
  FillConsoleOutputCharacter(0xC4, // '-',
			     List->Right - List->Left - 1,
			     coPos,
			     &Written);

  /* draw upper right corner */
  coPos.X = List->Right;
  coPos.Y = List->Top;
  FillConsoleOutputCharacter(0xBF, // '+',
			     1,
			     coPos,
			     &Written);

  /* draw left and right edge */
  for (i = List->Top + 1; i < List->Bottom; i++)
    {
      coPos.X = List->Left;
      coPos.Y = i;
      FillConsoleOutputCharacter(0xB3, // '|',
				 1,
				 coPos,
				 &Written);

      coPos.X = List->Right;
      FillConsoleOutputCharacter(0xB3, //'|',
				 1,
				 coPos,
				 &Written);
    }

  /* draw lower left corner */
  coPos.X = List->Left;
  coPos.Y = List->Bottom;
  FillConsoleOutputCharacter(0xC0, // '+',
			     1,
			     coPos,
			     &Written);

  /* draw lower edge */
  coPos.X = List->Left + 1;
  coPos.Y = List->Bottom;
  FillConsoleOutputCharacter(0xC4, // '-',
			     List->Right - List->Left - 1,
			     coPos,
			     &Written);

  /* draw lower right corner */
  coPos.X = List->Right;
  coPos.Y = List->Bottom;
  FillConsoleOutputCharacter(0xD9, // '+',
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
ScrollDownPartitionList(PPARTLIST List)
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
ScrollUpPartitionList(PPARTLIST List)
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


BOOLEAN
GetSelectedPartition(PPARTLIST List,
		     PPARTDATA Data)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;

  if (List->CurrentDisk == NULL)
    return FALSE;

  DiskEntry = List->CurrentDisk;

  if (List->CurrentPartition == NULL)
    return FALSE;

  PartEntry = List->CurrentPartition;

  /* Copy disk-specific data */
  Data->DiskSize = DiskEntry->DiskSize;
  Data->DiskNumber = DiskEntry->DiskNumber;
  Data->Port = DiskEntry->Port;
  Data->Bus = DiskEntry->Bus;
  Data->Id = DiskEntry->Id;

  /* Copy driver name */
  RtlInitUnicodeString(&Data->DriverName,
		       NULL);
  if (DiskEntry->DriverName.Length != 0)
    {
      Data->DriverName.Buffer = RtlAllocateHeap(ProcessHeap,
						0,
						DiskEntry->DriverName.MaximumLength);
      if (Data->DriverName.Buffer != NULL)
	{
	  Data->DriverName.MaximumLength = DiskEntry->DriverName.MaximumLength;
	  Data->DriverName.Length = DiskEntry->DriverName.Length;
	  RtlCopyMemory(Data->DriverName.Buffer,
			DiskEntry->DriverName.Buffer,
			DiskEntry->DriverName.MaximumLength);
	}
    }

  /* Copy partition-specific data */
  Data->CreatePartition = FALSE;
  Data->NewPartSize = 0;
  Data->PartSize = PartEntry->PartInfo[0].PartitionLength.QuadPart;
  Data->PartNumber = PartEntry->PartInfo[0].PartitionNumber;
  Data->PartType = PartEntry->PartInfo[0].PartitionType;
  Data->DriveLetter = PartEntry->DriveLetter;

  return TRUE;
}


BOOLEAN
GetActiveBootPartition(PPARTLIST List,
		       PPARTDATA Data)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  PLIST_ENTRY Entry;
  ULONG i;

  if (IsListEmpty (&List->DiskListHead))
    return FALSE;

  /* Get first disk entry from the disk list */
  Entry = List->DiskListHead.Flink;
  DiskEntry = CONTAINING_RECORD (Entry, DISKENTRY, ListEntry);

  Entry = DiskEntry->PartListHead.Flink;
  while (Entry != &DiskEntry->PartListHead)
    {
      PartEntry = CONTAINING_RECORD (Entry, PARTENTRY, ListEntry);

      if (PartEntry->PartInfo[0].BootIndicator)
	{
	  /* Copy disk-specific data */
	  Data->DiskSize = DiskEntry->DiskSize;
	  Data->DiskNumber = DiskEntry->DiskNumber;
	  Data->Port = DiskEntry->Port;
	  Data->Bus = DiskEntry->Bus;
	  Data->Id = DiskEntry->Id;

	  /* Copy driver name */
	  RtlInitUnicodeString(&Data->DriverName,
			       NULL);
	  if (DiskEntry->DriverName.Length != 0)
	    {
	      Data->DriverName.Buffer = RtlAllocateHeap(ProcessHeap,
							0,
							DiskEntry->DriverName.MaximumLength);
	      if (Data->DriverName.Buffer != NULL)
		{
		  Data->DriverName.MaximumLength = DiskEntry->DriverName.MaximumLength;
		  Data->DriverName.Length = DiskEntry->DriverName.Length;
		  RtlCopyMemory(Data->DriverName.Buffer,
				DiskEntry->DriverName.Buffer,
				DiskEntry->DriverName.MaximumLength);
		}
	    }

	  /* Copy partition-specific data */
	  Data->PartSize = PartEntry->PartInfo[0].PartitionLength.QuadPart;
	  Data->PartNumber = PartEntry->PartInfo[0].PartitionNumber;
	  Data->PartType = PartEntry->PartInfo[0].PartitionType;
	  Data->DriveLetter = PartEntry->DriveLetter;

	  return TRUE;
	}

      Entry = Entry->Flink;
    }

  return FALSE;
}


BOOLEAN
CreateSelectedPartition(PPARTLIST List,
  ULONG PartType,
  ULONGLONG NewPartSize)
{
#if 0
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  ULONG PartEntryNumber;
  OBJECT_ATTRIBUTES ObjectAttributes;
  DRIVE_LAYOUT_INFORMATION *LayoutBuffer;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  WCHAR Buffer[MAX_PATH];
  UNICODE_STRING Name;
  HANDLE FileHandle;
  LARGE_INTEGER li;

  DiskEntry = List->CurrentDisk;
  PartEntry = List->CurrentPartition;

  PartEntry->PartType = PartType;
  PartEntryNumber = List->CurrentPartition;

  DPRINT("NewPartSize %d (%d MB)\n", NewPartSize, NewPartSize / (1024 * 1024));
  DPRINT("PartEntry->StartingOffset %d\n", PartEntry->StartingOffset);
  DPRINT("PartEntry->PartSize %d\n", PartEntry->PartSize);
  DPRINT("PartEntry->PartNumber %d\n", PartEntry->PartNumber);
  DPRINT("PartEntry->PartType 0x%x\n", PartEntry->PartType);
  DPRINT("PartEntry->FileSystemName %s\n", PartEntry->FileSystemName);

  swprintf(Buffer,
    L"\\Device\\Harddisk%d\\Partition0",
    DiskEntry->DiskNumber);
  RtlInitUnicodeString(&Name, Buffer);

  InitializeObjectAttributes(&ObjectAttributes,
    &Name,
    0,
    NULL,
    NULL);

  Status = NtOpenFile(&FileHandle,
    0x10001,
    &ObjectAttributes,
    &Iosb,
    1,
    FILE_SYNCHRONOUS_IO_NONALERT);
  if (NT_SUCCESS(Status))
    {
	  LayoutBuffer = (DRIVE_LAYOUT_INFORMATION*)RtlAllocateHeap(ProcessHeap, 0, 8192);

	  Status = NtDeviceIoControlFile(FileHandle,
  		NULL,
  		NULL,
  		NULL,
  		&Iosb,
  		IOCTL_DISK_GET_DRIVE_LAYOUT,
  		NULL,
  		0,
  		LayoutBuffer,
  		8192);
	  if (!NT_SUCCESS(Status))
	    {
          DPRINT("IOCTL_DISK_GET_DRIVE_LAYOUT failed() 0x%.08x\n", Status);
          NtClose(FileHandle);
          RtlFreeHeap(ProcessHeap, 0, LayoutBuffer);
          return FALSE;
        }

      li.QuadPart = PartEntry->StartingOffset;
      LayoutBuffer->PartitionEntry[PartEntryNumber].StartingOffset = li;
      /* FIXME: Adjust PartitionLength so the partition will end on the last sector of a track */
      li.QuadPart = NewPartSize;
      LayoutBuffer->PartitionEntry[PartEntryNumber].PartitionLength = li;
      LayoutBuffer->PartitionEntry[PartEntryNumber].HiddenSectors =
        PartEntry->StartingOffset / DiskEntry->BytesPerSector;
      LayoutBuffer->PartitionEntry[PartEntryNumber].PartitionType = PartType;
      LayoutBuffer->PartitionEntry[PartEntryNumber].RecognizedPartition = TRUE;
      LayoutBuffer->PartitionEntry[PartEntryNumber].RewritePartition = TRUE;

      Status = NtDeviceIoControlFile(FileHandle,
        NULL,
        NULL,
        NULL,
        &Iosb,
        IOCTL_DISK_SET_DRIVE_LAYOUT,
        LayoutBuffer,
        8192,
        NULL,
        0);
      if (!NT_SUCCESS(Status))
        {
          DPRINT("IOCTL_DISK_SET_DRIVE_LAYOUT failed() 0x%.08x\n", Status);
          NtClose(FileHandle);
          RtlFreeHeap(ProcessHeap, 0, LayoutBuffer);
          return FALSE;
        }
    }
  else
    {
      DPRINT("NtOpenFile failed() 0x%.08x\n", Status);
      NtClose(FileHandle);
      RtlFreeHeap(ProcessHeap, 0, LayoutBuffer);
      return FALSE;
    }

  NtClose(FileHandle);
  RtlFreeHeap(ProcessHeap, 0, LayoutBuffer);
#endif

  return TRUE;
}


BOOLEAN
DeleteSelectedPartition(PPARTLIST List)
{
#if 0
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
//  ULONG PartEntryNumber;
  OBJECT_ATTRIBUTES ObjectAttributes;
  DRIVE_LAYOUT_INFORMATION *LayoutBuffer;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  WCHAR Buffer[MAX_PATH];
  UNICODE_STRING Name;
  HANDLE FileHandle;
  LARGE_INTEGER li;

  DiskEntry = List->CurrentDisk;
  PartEntry = List->CurrentPartition;
  PartEntry->PartType = PARTITION_ENTRY_UNUSED;

//  PartEntryNumber = List->CurrentPartition;

  DPRINT1("DeleteSelectedPartition(PartEntryNumber = %d)\n", PartEntryNumber);
  DPRINT1("PartEntry->StartingOffset %d\n", PartEntry->StartingOffset);
  DPRINT1("PartEntry->PartSize %d\n", PartEntry->PartSize);
  DPRINT1("PartEntry->PartNumber %d\n", PartEntry->PartNumber);
  DPRINT1("PartEntry->PartType 0x%x\n", PartEntry->PartType);
  DPRINT1("PartEntry->FileSystemName %s\n", PartEntry->FileSystemName);

  swprintf(Buffer,
    L"\\Device\\Harddisk%d\\Partition0",
    DiskEntry->DiskNumber);
  RtlInitUnicodeString(&Name, Buffer);

  InitializeObjectAttributes(&ObjectAttributes,
    &Name,
    0,
    NULL,
    NULL);

  Status = NtOpenFile(&FileHandle,
    0x10001,
    &ObjectAttributes,
    &Iosb,
    1,
    FILE_SYNCHRONOUS_IO_NONALERT);
  if (NT_SUCCESS(Status))
    {
	  LayoutBuffer = (DRIVE_LAYOUT_INFORMATION*)RtlAllocateHeap(ProcessHeap, 0, 8192);

	  Status = NtDeviceIoControlFile(FileHandle,
		NULL,
		NULL,
		NULL,
		&Iosb,
		IOCTL_DISK_GET_DRIVE_LAYOUT,
		NULL,
		0,
		LayoutBuffer,
		8192);
	  if (!NT_SUCCESS(Status))
	    {
          DPRINT("IOCTL_DISK_GET_DRIVE_LAYOUT failed() 0x%.08x\n", Status);
          NtClose(FileHandle);
          RtlFreeHeap(ProcessHeap, 0, LayoutBuffer);
          return FALSE;
        }

      li.QuadPart = 0;
      LayoutBuffer->PartitionEntry[PartEntryNumber].StartingOffset = li;
      li.QuadPart = 0;
      LayoutBuffer->PartitionEntry[PartEntryNumber].PartitionLength = li;
      LayoutBuffer->PartitionEntry[PartEntryNumber].HiddenSectors = 0;
      LayoutBuffer->PartitionEntry[PartEntryNumber].PartitionType = 0;
      LayoutBuffer->PartitionEntry[PartEntryNumber].RecognizedPartition = FALSE;
      LayoutBuffer->PartitionEntry[PartEntryNumber].RewritePartition = TRUE;

      Status = NtDeviceIoControlFile(FileHandle,
        NULL,
        NULL,
        NULL,
        &Iosb,
        IOCTL_DISK_SET_DRIVE_LAYOUT,
        LayoutBuffer,
        8192,
        NULL,
        0);
      if (!NT_SUCCESS(Status))
        {
          DPRINT("IOCTL_DISK_SET_DRIVE_LAYOUT failed() 0x%.08x\n", Status);
          NtClose(FileHandle);
          RtlFreeHeap(ProcessHeap, 0, LayoutBuffer);
          return FALSE;
        }
    }
  else
    {
      DPRINT("NtOpenFile failed() 0x%.08x\n", Status);
      NtClose(FileHandle);
      RtlFreeHeap(ProcessHeap, 0, LayoutBuffer);
      return FALSE;
    }

  NtClose(FileHandle);
  RtlFreeHeap(ProcessHeap, 0, LayoutBuffer);
#endif

  return TRUE;
}

#if 0
BOOLEAN
MarkPartitionActive (ULONG DiskNumber,
		     ULONG PartitionNumber,
		     PPARTDATA ActivePartition)
{
  PPARTLIST List;
  PPARTENTRY PartEntry;
  ULONG PartEntryNumber;
  OBJECT_ATTRIBUTES ObjectAttributes;
  DRIVE_LAYOUT_INFORMATION *LayoutBuffer;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  WCHAR Buffer[MAX_PATH];
  UNICODE_STRING Name;
  HANDLE FileHandle;

  List = InitializePartitionList ();
  if (List == NULL)
    {
      return(FALSE);
    }

  PartEntry = GetPartitionInformation(List,
			DiskNumber,
			PartitionNumber,
			&PartEntryNumber);
  if (List == NULL)
    {
      DestroyPartitionList(List);
      return(FALSE);
    }


  swprintf(Buffer,
    L"\\Device\\Harddisk%d\\Partition0",
    DiskNumber);
  RtlInitUnicodeString(&Name, Buffer);

  InitializeObjectAttributes(&ObjectAttributes,
    &Name,
    0,
    NULL,
    NULL);

  Status = NtOpenFile(&FileHandle,
    0x10001,
    &ObjectAttributes,
    &Iosb,
    1,
    FILE_SYNCHRONOUS_IO_NONALERT);
  if (NT_SUCCESS(Status))
    {
	  LayoutBuffer = (DRIVE_LAYOUT_INFORMATION*)RtlAllocateHeap(ProcessHeap, 0, 8192);

	  Status = NtDeviceIoControlFile(FileHandle,
		NULL,
		NULL,
		NULL,
		&Iosb,
		IOCTL_DISK_GET_DRIVE_LAYOUT,
		NULL,
		0,
		LayoutBuffer,
		8192);
	  if (!NT_SUCCESS(Status))
	    {
          NtClose(FileHandle);
          RtlFreeHeap(ProcessHeap, 0, LayoutBuffer);
          DestroyPartitionList(List);
          return FALSE;
        }


      LayoutBuffer->PartitionEntry[PartEntryNumber].BootIndicator = TRUE;

      Status = NtDeviceIoControlFile(FileHandle,
        NULL,
        NULL,
        NULL,
        &Iosb,
        IOCTL_DISK_SET_DRIVE_LAYOUT,
        LayoutBuffer,
        8192,
        NULL,
        0);
      if (!NT_SUCCESS(Status))
        {
          NtClose(FileHandle);
          RtlFreeHeap(ProcessHeap, 0, LayoutBuffer);
          DestroyPartitionList(List);
          return FALSE;
        }
    }
  else
    {
      NtClose(FileHandle);
      RtlFreeHeap(ProcessHeap, 0, LayoutBuffer);
      DestroyPartitionList(List);
      return FALSE;
    }

  NtClose(FileHandle);
  RtlFreeHeap(ProcessHeap, 0, LayoutBuffer);

  PartEntry->Active = TRUE;
  if (!GetActiveBootPartition(List, ActivePartition))
  {
    DestroyPartitionList(List);
    DPRINT("GetActiveBootPartition() failed\n");
    return FALSE;
  }

  DestroyPartitionList(List);

  return TRUE;
}
#endif

/* EOF */
