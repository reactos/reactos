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
/* $Id: partlist.c,v 1.10 2003/05/11 18:31:09 chorns Exp $
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
AddPartitionList(ULONG DiskNumber,
		 PDISKENTRY DiskEntry,
		 DRIVE_LAYOUT_INFORMATION *LayoutBuffer)
{
  PPARTENTRY PartEntry;
  ULONG i;
  ULONG EntryCount;
  BOOLEAN LastEntryWasUnused;
  ULONGLONG LastStartingOffset;
  ULONGLONG LastPartitionSize;
  ULONGLONG LastUnusedPartitionSize;
  ULONG LastUnusedEntry;

  /*
   * FIXME:
   * Determine required number of partiton entries.
   * This must include entries for unused disk space.
   */

  /* Check for unpartitioned disk */
  if (LayoutBuffer->PartitionCount == 0)
    {
      EntryCount = 1;
    }
  else
    {
      EntryCount = LayoutBuffer->PartitionCount;
    }


  DiskEntry->PartArray = (PPARTENTRY)RtlAllocateHeap(ProcessHeap,
						     0,
						     EntryCount * sizeof(PARTENTRY));
  DiskEntry->PartCount = EntryCount;

  RtlZeroMemory(DiskEntry->PartArray,
		EntryCount * sizeof(PARTENTRY));

  if (LayoutBuffer->PartitionCount == 0)
    {
      /* Initialize an 'Unpartitioned space' entry */
      PartEntry = &DiskEntry->PartArray[0];

      PartEntry->Unpartitioned = TRUE;
      // Start partition at head 1, cylinder 0
      PartEntry->StartingOffset = DiskEntry->SectorsPerTrack * DiskEntry->BytesPerSector;
      PartEntry->PartSize = DiskEntry->DiskSize - PartEntry->StartingOffset;
      PartEntry->Used = FALSE;
      PartEntry->HidePartEntry = FALSE;
      PartEntry->PartNumber = 1;
    }
  else
    {
      LastEntryWasUnused = FALSE;
      // Start partition at head 1, cylinder 0
      LastStartingOffset = DiskEntry->SectorsPerTrack * DiskEntry->BytesPerSector;
      LastPartitionSize = 0;
      LastUnusedEntry = -1;
      LastUnusedPartitionSize = 0;
      for (i = 0; i < LayoutBuffer->PartitionCount; i++)
	{
	  PartEntry = &DiskEntry->PartArray[i];

	  if ((LayoutBuffer->PartitionEntry[i].PartitionType != PARTITION_ENTRY_UNUSED) &&
	      (!IsContainerPartition(LayoutBuffer->PartitionEntry[i].PartitionType)))
	    {
        LastUnusedPartitionSize = LayoutBuffer->PartitionEntry[i].StartingOffset.QuadPart
          - (LastStartingOffset + LastPartitionSize);
        if (LastUnusedEntry != -1)
          {
            DiskEntry->PartArray[LastUnusedEntry].StartingOffset = LastStartingOffset + LastPartitionSize;
            DiskEntry->PartArray[LastUnusedEntry].PartSize = LastUnusedPartitionSize;
            DiskEntry->PartArray[LastUnusedEntry].PartNumber = LastUnusedEntry + 1; /* FIXME: Is this always correct? */
          }
        LastStartingOffset = LayoutBuffer->PartitionEntry[i].StartingOffset.QuadPart;
        LastPartitionSize = LayoutBuffer->PartitionEntry[i].PartitionLength.QuadPart;

        PartEntry->StartingOffset = LayoutBuffer->PartitionEntry[i].StartingOffset.QuadPart;
	      PartEntry->PartSize = LayoutBuffer->PartitionEntry[i].PartitionLength.QuadPart;
	      PartEntry->PartNumber = LayoutBuffer->PartitionEntry[i].PartitionNumber,
	      PartEntry->PartType = LayoutBuffer->PartitionEntry[i].PartitionType;
	      PartEntry->Active = LayoutBuffer->PartitionEntry[i].BootIndicator;

	      PartEntry->DriveLetter = GetDriveLetter(DiskNumber,
						      LayoutBuffer->PartitionEntry[i].PartitionNumber);

	      PartEntry->Unpartitioned = FALSE;

	      PartEntry->Used = TRUE;
        PartEntry->HidePartEntry = FALSE;
        LastEntryWasUnused = FALSE;
        LastUnusedEntry = -1;
	    }
	  else
	    {
        if (LastEntryWasUnused)
          {
            /* Group unused entries into one unpartitioned disk space area */
            PartEntry->HidePartEntry = TRUE;
            PartEntry->PartSize = 0;
          }
        else
          {
            LastUnusedEntry = i;
          }

        PartEntry->Unpartitioned = TRUE;

	      PartEntry->Used = FALSE;
        LastEntryWasUnused = TRUE;
	    }
	}
      LastUnusedPartitionSize = DiskEntry->DiskSize
        - (LastStartingOffset + LastPartitionSize);
      if (LastUnusedEntry != -1)
        {
          DiskEntry->PartArray[LastUnusedEntry].StartingOffset = LastStartingOffset + LastPartitionSize;
          DiskEntry->PartArray[LastUnusedEntry].PartSize = LastUnusedPartitionSize;
          DiskEntry->PartArray[LastUnusedEntry].PartNumber = LastUnusedEntry + 1; /* FIXME: Is this always correct? */
        }
    }
}


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


PPARTLIST
CreatePartitionListNoGUI()
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
  DRIVE_LAYOUT_INFORMATION *LayoutBuffer;
  SCSI_ADDRESS ScsiAddress;

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

  List->CurrentDisk = (ULONG)-1;
  List->CurrentPartition = (ULONG)-1;

  List->DiskCount = 0;
  List->DiskArray = NULL;


  Status = NtQuerySystemInformation(SystemDeviceInformation,
				    &Sdi,
				    sizeof(SYSTEM_DEVICE_INFORMATION),
				    &ReturnSize);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeHeap(ProcessHeap, 0, List);
      return(NULL);
    }

  List->DiskArray = (PDISKENTRY)RtlAllocateHeap(ProcessHeap,
						0,
						Sdi.NumberOfDisks * sizeof(DISKENTRY));
  List->DiskCount = Sdi.NumberOfDisks;

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
	  Status = NtDeviceIoControlFile(FileHandle,
					 NULL,
					 NULL,
					 NULL,
					 &Iosb,
					 IOCTL_DISK_GET_DRIVE_GEOMETRY,
					 NULL,
					 0,
					 &DiskGeometry,
					 sizeof(DISK_GEOMETRY));
	  if (NT_SUCCESS(Status))
	    {
	      if (DiskGeometry.MediaType == FixedMedia)
		{
		  Status = NtDeviceIoControlFile(FileHandle,
						 NULL,
						 NULL,
						 NULL,
						 &Iosb,
						 IOCTL_SCSI_GET_ADDRESS,
						 NULL,
						 0,
						 &ScsiAddress,
						 sizeof(SCSI_ADDRESS));


		  List->DiskArray[DiskNumber].Cylinders = DiskGeometry.Cylinders.QuadPart;
		  List->DiskArray[DiskNumber].TracksPerCylinder = DiskGeometry.TracksPerCylinder;
		  List->DiskArray[DiskNumber].SectorsPerTrack = DiskGeometry.SectorsPerTrack;
		  List->DiskArray[DiskNumber].BytesPerSector = DiskGeometry.BytesPerSector;

      DPRINT("Cylinders %d\n", List->DiskArray[DiskNumber].Cylinders);
      DPRINT("TracksPerCylinder %d\n", List->DiskArray[DiskNumber].TracksPerCylinder);
      DPRINT("SectorsPerTrack %d\n", List->DiskArray[DiskNumber].SectorsPerTrack);
      DPRINT("BytesPerSector %d\n", List->DiskArray[DiskNumber].BytesPerSector);

		  List->DiskArray[DiskNumber].DiskSize = 
		    DiskGeometry.Cylinders.QuadPart *
		    (ULONGLONG)DiskGeometry.TracksPerCylinder *
		    (ULONGLONG)DiskGeometry.SectorsPerTrack *
		    (ULONGLONG)DiskGeometry.BytesPerSector;
		  List->DiskArray[DiskNumber].DiskNumber = DiskNumber;
		  List->DiskArray[DiskNumber].Port = ScsiAddress.PortNumber;
		  List->DiskArray[DiskNumber].Bus = ScsiAddress.PathId;
		  List->DiskArray[DiskNumber].Id = ScsiAddress.TargetId;

		  List->DiskArray[DiskNumber].FixedDisk = TRUE;

		  GetDriverName(&List->DiskArray[DiskNumber]);

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
		  if (NT_SUCCESS(Status))
		    {
		      AddPartitionList(DiskNumber,
				       &List->DiskArray[DiskNumber],
				       LayoutBuffer);
		    }

		  RtlFreeHeap(ProcessHeap, 0, LayoutBuffer);
		}
	      else
		{
		  /* mark removable disk entry */
		  List->DiskArray[DiskNumber].FixedDisk = FALSE;
		  List->DiskArray[DiskNumber].PartCount = 0;
		  List->DiskArray[DiskNumber].PartArray = NULL;
		}
	    }

	  NtClose(FileHandle);
	}
    }

  List->TopDisk = 0;
  List->TopPartition = 0;

  /* FIXME: search for first usable disk and partition */
  List->CurrentDisk = 0;
  List->CurrentPartition = 0;

  return(List);
}

PPARTLIST
CreatePartitionList(SHORT Left,
		    SHORT Top,
		    SHORT Right,
		    SHORT Bottom)
{
  PPARTLIST List;

  List = CreatePartitionListNoGUI();
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

  if (List->DiskArray == NULL)
    {
      return(FALSE);
    }

  if (DiskNumber >= List->DiskCount)
    {
      return(FALSE);
    }

  if (PartitionNumber >= List->DiskArray[DiskNumber].PartCount)
    {
      return(FALSE);
    }

  if (List->DiskArray[DiskNumber].FixedDisk != TRUE)
    {
      return(FALSE);
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
  return NULL;
}

BOOLEAN
MarkPartitionActive(ULONG DiskNumber,
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

  List = CreatePartitionListNoGUI();
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


VOID
DestroyPartitionList(PPARTLIST List)
{
  ULONG i;
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
  if (List->DiskArray != NULL)
    {
      for (i = 0; i < List->DiskCount; i++)
	{
	  /* Release driver name */
	  RtlFreeUnicodeString(&List->DiskArray[i].DriverName);

	  /* Release partition array */
	  if (List->DiskArray[i].PartArray != NULL)
	    {
	      RtlFreeHeap(ProcessHeap, 0, List->DiskArray[i].PartArray);
	      List->DiskArray[i].PartCount = 0;
	      List->DiskArray[i].PartArray = NULL;
	    }
	}

      /* Release disk array */
      RtlFreeHeap(ProcessHeap, 0, List->DiskArray);
      List->DiskCount = 0;
      List->DiskArray = NULL;
    }

  /* Release list head */
  RtlFreeHeap(ProcessHeap, 0, List);
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
		   SHORT DiskIndex,
		   SHORT PartIndex)
{
  PPARTENTRY PartEntry;
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

  PartEntry = &List->DiskArray[DiskIndex].PartArray[PartIndex];

  /* Determine partition type */
  PartType = NULL;
  if (PartEntry->Unpartitioned == FALSE)
    {
      if ((PartEntry->PartType == PARTITION_FAT_12) ||
	  (PartEntry->PartType == PARTITION_FAT_16) ||
	  (PartEntry->PartType == PARTITION_HUGE) ||
	  (PartEntry->PartType == PARTITION_XINT13))
	{
	  PartType = "FAT";
	}
      else if ((PartEntry->PartType == PARTITION_FAT32) ||
	       (PartEntry->PartType == PARTITION_FAT32_XINT13))
	{
	  PartType = "FAT32";
	}
     else if (PartEntry->PartType == PARTITION_IFS)
	{
	  PartType = "NTFS"; /* FIXME: Not quite correct! */
	}
    }


#if 0
  if (PartEntry->PartSize >= 0x280000000ULL) /* 10 GB */
    {
      PartSize = (PartEntry->PartSize + (1 << 29)) >> 30;
      Unit = "GB";
    }
  else
#endif
  if (PartEntry->PartSize >= 0xA00000ULL) /* 10 MB */
    {
      PartSize = (PartEntry->PartSize + (1 << 19)) >> 20;
      Unit = "MB";
    }
  else
    {
      PartSize = (PartEntry->PartSize + (1 << 9)) >> 10;
      Unit = "KB";
    }


  if (PartEntry->Unpartitioned == TRUE)
    {
      sprintf(LineBuffer,
	      "    Unpartitioned space           %I64u %s",
	      PartSize,
	      Unit);
    }
  else if (PartEntry->DriveLetter != (CHAR)0)
    {
      if (PartType == NULL)
	{
	  sprintf(LineBuffer,
		  "%c:  Type %-3lu                        %I64u %s",
		  PartEntry->DriveLetter,
		  PartEntry->PartType,
		  PartSize,
		  Unit);
	}
      else
	{
	  sprintf(LineBuffer,
		  "%c:  %s                         %I64u %s",
		  PartEntry->DriveLetter,
		  PartType,
		  PartSize,
		  Unit);
	}

#if 0
      sprintf(LineBuffer,
	      "%c:  %s  (%d: nr: %d type: %x)  %I64u %s",
	      PartEntry->DriveLetter,
	      PartType,
	      PartIndex,
	      PartEntry->PartNumber,
	      PartEntry->PartType,
	      PartSize,
	      Unit);
#endif
    }
  else
    {
      sprintf(LineBuffer,
	      "--  %s  (%d: nr: %d type: %x)  %I64u %s",
	      PartEntry->FileSystemName,
	      PartIndex,
	      PartEntry->PartNumber,
	      PartEntry->PartType,
	      PartSize,
	      Unit);
    }

  Attribute = (List->CurrentDisk == DiskIndex &&
	       List->CurrentPartition == PartIndex) ? 0x71 : 0x17;

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
	      SHORT DiskIndex)
{
  PDISKENTRY DiskEntry;
  CHAR LineBuffer[128];
  COORD coPos;
  ULONG Written;
  USHORT Width;
  USHORT Height;
  ULONGLONG DiskSize;
  PCHAR Unit;
  SHORT PartIndex;

  DiskEntry = &List->DiskArray[DiskIndex];

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
  for (PartIndex = 0; PartIndex < DiskEntry->PartCount; PartIndex++)
    {
      if (!DiskEntry->PartArray[PartIndex].HidePartEntry)
        {
      	  PrintPartitionData(List,
      			     DiskIndex,
      			     PartIndex);
        }
    }

  /* Print separator line */
  PrintEmptyLine(List);
}


VOID
DrawPartitionList(PPARTLIST List)
{
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
  for (DiskIndex = 0; DiskIndex < List->DiskCount; DiskIndex++)
    {
      if (List->DiskArray[DiskIndex].FixedDisk == TRUE)
	{
	  /* Print disk entry */
	  PrintDiskData(List,
			DiskIndex);
	}
    }
}


VOID
ScrollDownPartitionList(PPARTLIST List)
{
  ULONG i;
  ULONG j;

  /* check for available disks */
  if (List->DiskCount == 0)
    return;

  /* check for next usable entry on current disk */
  for (i = List->CurrentPartition + 1; i < List->DiskArray[List->CurrentDisk].PartCount; i++)
    {
      if (!List->DiskArray[List->CurrentDisk].PartArray[i].HidePartEntry)
        {
      	  List->CurrentPartition = i;
      	  DrawPartitionList(List);
      	  return;
        }
    }

  /* check for first usable entry on next disk */
  for (j = List->CurrentDisk + 1; j < List->DiskCount; j++)
    {
      for (i = 0; i < List->DiskArray[j].PartCount; i++)
	{
    if (!List->DiskArray[j].PartArray[i].HidePartEntry)
      {
	      List->CurrentDisk = j;
	      List->CurrentPartition = i;
	      DrawPartitionList(List);
	      return;
      }
	}
    }
}


VOID
ScrollUpPartitionList(PPARTLIST List)
{
  ULONG i;
  ULONG j;

  /* check for available disks */
  if (List->DiskCount == 0)
    return;

  /* check for previous usable entry on current disk */
  for (i = List->CurrentPartition - 1; i != (ULONG)-1; i--)
    {
      if (!List->DiskArray[List->CurrentDisk].PartArray[i].HidePartEntry)
	{
	  List->CurrentPartition = i;
	  DrawPartitionList(List);
	  return;
	}
    }

  /* check for last usable entry on previous disk */
  for (j = List->CurrentDisk - 1; j != (ULONG)-1; j--)
    {
      for (i = List->DiskArray[j].PartCount - 1; i != (ULONG)-1; i--)
	{
    if (!List->DiskArray[j].PartArray[i].HidePartEntry)
	    {
	      List->CurrentDisk = j;
	      List->CurrentPartition = i;
	      DrawPartitionList(List);
	      return;
	    }
	}
    }
}


BOOL
GetSelectedPartition(PPARTLIST List,
		     PPARTDATA Data)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;

  if (List->CurrentDisk >= List->DiskCount)
    return(FALSE);

  DiskEntry = &List->DiskArray[List->CurrentDisk];

  if (DiskEntry->FixedDisk == FALSE)
    return(FALSE);

  if (List->CurrentPartition >= DiskEntry->PartCount)
    return(FALSE);

  PartEntry = &DiskEntry->PartArray[List->CurrentPartition];

  if (PartEntry->Used == FALSE)
    {
      return(FALSE);
    }

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
  Data->PartSize = PartEntry->PartSize;
  Data->PartNumber = PartEntry->PartNumber;
  Data->PartType = PartEntry->PartType;
  Data->DriveLetter = PartEntry->DriveLetter;
  return(TRUE);
}


BOOL
GetActiveBootPartition(PPARTLIST List,
		       PPARTDATA Data)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  ULONG i;

  if (List->CurrentDisk >= List->DiskCount)
    return(FALSE);

  DiskEntry = &List->DiskArray[List->CurrentDisk];

  if (DiskEntry->FixedDisk == FALSE)
  {
    return(FALSE);
  }

  for (i = 0; i < DiskEntry->PartCount; i++)
    {
      if (DiskEntry->PartArray[i].Active)
	{
	  PartEntry = &DiskEntry->PartArray[i];

	  if (PartEntry->Used == FALSE)
      {
	    return(FALSE);
      }

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
	  Data->PartSize = PartEntry->PartSize;
	  Data->PartNumber = PartEntry->PartNumber;
	  Data->PartType = PartEntry->PartType;
	  Data->DriveLetter = PartEntry->DriveLetter;

	  return(TRUE);
	}
    }

  return(FALSE);
}


BOOL
CreateSelectedPartition(PPARTLIST List,
  ULONG PartType,
  ULONGLONG NewPartSize)
{
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

  DiskEntry = &List->DiskArray[List->CurrentDisk];
  PartEntry = &DiskEntry->PartArray[List->CurrentPartition];
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

  return TRUE;
}


BOOL
DeleteSelectedPartition(PPARTLIST List)
{
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

  DiskEntry = &List->DiskArray[List->CurrentDisk];
  PartEntry = &DiskEntry->PartArray[List->CurrentPartition];
  PartEntry->PartType = PARTITION_ENTRY_UNUSED;
  PartEntryNumber = List->CurrentPartition;

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

  return TRUE;
}

/* EOF */
