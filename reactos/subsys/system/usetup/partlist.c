/*
 *  partlist.c
 */

#include <ddk/ntddk.h>
#include <ddk/ntddscsi.h>

#include <ntdll/rtl.h>

#include <ntos/minmax.h>
#include <ntos/keyboard.h>

#include "usetup.h"
#include "partlist.h"




PPARTLIST
CreatePartitionList(SHORT Left,
		    SHORT Top,
		    SHORT Right,
		    SHORT Bottom)
{
  PPARTLIST List;
  OBJECT_ATTRIBUTES ObjectAttributes;
  SYSTEM_DEVICE_INFORMATION Sdi;
  DISK_GEOMETRY DiskGeometry;
  ULONG ReturnSize;
  NTSTATUS Status;
  ULONG DiskCount;
  IO_STATUS_BLOCK Iosb;
  WCHAR Buffer[MAX_PATH];
  UNICODE_STRING Name;
  HANDLE FileHandle;
  DRIVE_LAYOUT_INFORMATION *LayoutBuffer;
  SCSI_ADDRESS ScsiAddress;
  ULONG i;

  List = (PPARTLIST)RtlAllocateHeap(ProcessHeap, 0, sizeof(PARTLIST));
  if (List == NULL)
    return(NULL);

  List->Left = Left;
  List->Top = Top;
  List->Right = Right;
  List->Bottom = Bottom;

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

  for (DiskCount = 0; DiskCount < Sdi.NumberOfDisks; DiskCount++)
    {
      swprintf(Buffer,
	       L"\\Device\\Harddisk%d\\Partition0",
	       DiskCount);
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


		  List->DiskArray[DiskCount].DiskSize = 
		    DiskGeometry.Cylinders.QuadPart *
		    (ULONGLONG)DiskGeometry.TracksPerCylinder *
		    (ULONGLONG)DiskGeometry.SectorsPerTrack *
		    (ULONGLONG)DiskGeometry.BytesPerSector;
		  List->DiskArray[DiskCount].DiskNumber = DiskCount;
		  List->DiskArray[DiskCount].Port = ScsiAddress.PortNumber;
		  List->DiskArray[DiskCount].Bus = ScsiAddress.PathId;
		  List->DiskArray[DiskCount].Id = ScsiAddress.TargetId;

		  List->DiskArray[DiskCount].FixedDisk = TRUE;


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

		      List->DiskArray[DiskCount].PartArray = (PPARTENTRY)RtlAllocateHeap(ProcessHeap,
											 0,
											 LayoutBuffer->PartitionCount * sizeof(PARTENTRY));
		      List->DiskArray[DiskCount].PartCount = LayoutBuffer->PartitionCount;

		      for (i = 0; i < LayoutBuffer->PartitionCount; i++)
			{
			  if ((LayoutBuffer->PartitionEntry[i].PartitionType != PARTITION_ENTRY_UNUSED) &&
			      !IsContainerPartition(LayoutBuffer->PartitionEntry[i].PartitionType))
			    {
			      List->DiskArray[DiskCount].PartArray[i].PartSize = LayoutBuffer->PartitionEntry[i].PartitionLength.QuadPart;
			      List->DiskArray[DiskCount].PartArray[i].PartNumber = LayoutBuffer->PartitionEntry[i].PartitionNumber,
			      List->DiskArray[DiskCount].PartArray[i].PartType = LayoutBuffer->PartitionEntry[i].PartitionType;
			      List->DiskArray[DiskCount].PartArray[i].Used = TRUE;
			    }
			  else
			    {
			      List->DiskArray[DiskCount].PartArray[i].Used = FALSE;
			    }
			}
		    }

		  RtlFreeHeap(ProcessHeap, 0, LayoutBuffer);
		}
	      else
		{
		  /* mark removable disk entry */
		  List->DiskArray[DiskCount].FixedDisk = FALSE;
		  List->DiskArray[DiskCount].PartCount = 0;
		  List->DiskArray[DiskCount].PartArray = NULL;
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

  DrawPartitionList(List);

  return(List);
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

  /* free disk and partition info */
  if (List->DiskArray != NULL)
    {
      /* free partition arrays */
      for (i = 0; i < List->DiskCount; i++)
	{
	  if (List->DiskArray[i].PartArray != NULL)
	    {
	      RtlFreeHeap(ProcessHeap, 0, List->DiskArray[i].PartArray);
	      List->DiskArray[i].PartCount = 0;
	      List->DiskArray[i].PartArray = NULL;
	    }
	}

      /* free disk array */
      RtlFreeHeap(ProcessHeap, 0, List->DiskArray);
      List->DiskCount = 0;
      List->DiskArray = NULL;
    }

  /* free list head */
  RtlFreeHeap(ProcessHeap, 0, List);
}


static VOID
PrintEmptyLine(PPARTLIST List, USHORT Line)
{
  COORD coPos;
  ULONG Written;
  USHORT Width;
  USHORT Height;

  Width = List->Right - List->Left - 1;
  Height = List->Bottom - List->Top - 1;

  if (Line > Height)
    return;

  coPos.X = List->Left + 1;
  coPos.Y = List->Top + 1 + Line;


  FillConsoleOutputAttribute(0x17,
			     Width,
			     coPos,
			     &Written);

  FillConsoleOutputCharacter(' ',
			     Width,
			     coPos,
			     &Written);
}


static VOID
PrintDiskLine(PPARTLIST List, USHORT Line, PCHAR Text)
{
  COORD coPos;
  ULONG Written;
  USHORT Width;
  USHORT Height;

  Width = List->Right - List->Left - 1;
  Height = List->Bottom - List->Top - 1;

  if (Line > Height)
    return;

  coPos.X = List->Left + 1;
  coPos.Y = List->Top + 1 + Line;


  FillConsoleOutputAttribute(0x17,
			     Width,
			     coPos,
			     &Written);

  FillConsoleOutputCharacter(' ',
			     Width,
			     coPos,
			     &Written);

  if (Text != NULL)
    {
      coPos.X++;
      WriteConsoleOutputCharacters(Text,
				   min(strlen(Text), Width - 2),
				   coPos);
    }
}


static VOID
PrintPartitionLine(PPARTLIST List, USHORT Line, PCHAR Text, BOOL Selected)
{
  COORD coPos;
  ULONG Written;
  USHORT Width;
  USHORT Height;

  Width = List->Right - List->Left - 1;
  Height = List->Bottom - List->Top - 1;

  if (Line > Height)
    return;

  coPos.X = List->Left + 1;
  coPos.Y = List->Top + 1 + Line;

  FillConsoleOutputCharacter(' ',
			     Width,
			     coPos,
			     &Written);

  coPos.X += 4;
  Width -= 8;
  FillConsoleOutputAttribute((Selected == TRUE)? 0x71 : 0x17,
			     Width,
			     coPos,
			     &Written);

  coPos.X++;
  Width -= 2;
  WriteConsoleOutputCharacters(Text,
			       min(strlen(Text), Width),
			       coPos);
}



VOID
DrawPartitionList(PPARTLIST List)
{
  CHAR LineBuffer[128];
  COORD coPos;
  ULONG Written;
  SHORT i;
  SHORT j;
  ULONGLONG DiskSize;
  PCHAR Unit;
  USHORT Line;
  PCHAR PartType;

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

  /* draw upper right corner */
  coPos.X = List->Right;
  coPos.Y = List->Bottom;
  FillConsoleOutputCharacter(0xD9, // '+',
			     1,
			     coPos,
			     &Written);

  /* print list entries */
  Line = 0;
  for (i = 0; i < List->DiskCount; i++)
    {
      if (List->DiskArray[i].FixedDisk == TRUE)
	{
	  /* print disk entry */
	  if (List->DiskArray[i].DiskSize >= 0x280000000ULL) /* 10 GB */
	    {
	      DiskSize = (List->DiskArray[i].DiskSize + (1 << 29)) >> 30;
	      Unit = "GB";
	    }
	  else
	    {
	      DiskSize = (List->DiskArray[i].DiskSize + (1 << 19)) >> 20;
	      Unit = "MB";
	    }

	  sprintf(LineBuffer,
		  "%I64u %s Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)",
		  DiskSize,
		  Unit,
		  List->DiskArray[i].DiskNumber,
		  List->DiskArray[i].Port,
		  List->DiskArray[i].Bus,
		  List->DiskArray[i].Id);
	  PrintDiskLine(List, Line, LineBuffer);
	  Line++;

	  /* print separator line */
	  PrintEmptyLine(List, Line);
	  Line++;

	  /* print partition lines*/
	  for (j = 0; j < List->DiskArray[i].PartCount; j++)
	    {
	      if (List->DiskArray[i].PartArray[j].Used == TRUE)
		{
		  if ((List->DiskArray[i].PartArray[j].PartType == PARTITION_FAT_12) ||
		      (List->DiskArray[i].PartArray[j].PartType == PARTITION_FAT_16) ||
		      (List->DiskArray[i].PartArray[j].PartType == PARTITION_HUGE) ||
		      (List->DiskArray[i].PartArray[j].PartType == PARTITION_XINT13))
		    {
		      PartType = "FAT";
		    }
		  else if ((List->DiskArray[i].PartArray[j].PartType == PARTITION_FAT32) ||
			   (List->DiskArray[i].PartArray[j].PartType == PARTITION_FAT32_XINT13))
		    {
		      PartType = "FAT32";
		    }
		  else if (List->DiskArray[i].PartArray[j].PartType == PARTITION_IFS)
		    {
		      PartType = "NTFS"; /* FIXME: Not quite correct! */
		    }
		  else
		    {
		      PartType = "Unknown";
		    }

		  sprintf(LineBuffer,
			  "%d: nr: %d type: %x (%s)  %I64u MB",
			  j,
			  List->DiskArray[i].PartArray[j].PartNumber,
			  List->DiskArray[i].PartArray[j].PartType,
			  PartType,
			  (List->DiskArray[i].PartArray[j].PartSize + (1 << 19)) >> 20);
		  PrintPartitionLine(List, Line, LineBuffer,
				(List->CurrentDisk == i && List->CurrentPartition == j)); // FALSE);
		  Line++;


		}
	    }

	  /* print separator line */
	  PrintEmptyLine(List, Line);
	  Line++;
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
      if (List->DiskArray[List->CurrentDisk].PartArray[i].Used == TRUE)
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
	  if (List->DiskArray[j].PartArray[i].Used == TRUE)
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
      if (List->DiskArray[List->CurrentDisk].PartArray[i].Used == TRUE)
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
	  if (List->DiskArray[j].PartArray[i].Used == TRUE)
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
GetPartitionData(PPARTLIST List, PPARTDATA Data)
{
  if (List->CurrentDisk >= List->DiskCount)
    return(FALSE);

  if (List->DiskArray[List->CurrentDisk].FixedDisk == FALSE)
    return(FALSE);

  if (List->CurrentPartition >= List->DiskArray[List->CurrentDisk].PartCount)
    return(FALSE);

  if (List->DiskArray[List->CurrentDisk].PartArray[List->CurrentPartition].Used == FALSE)
    return(FALSE);

  Data->DiskSize = List->DiskArray[List->CurrentDisk].DiskSize;
  Data->DiskNumber = List->DiskArray[List->CurrentDisk].DiskNumber;
  Data->Port = List->DiskArray[List->CurrentDisk].Port;
  Data->Bus = List->DiskArray[List->CurrentDisk].Bus;
  Data->Id = List->DiskArray[List->CurrentDisk].Id;

  Data->PartSize = List->DiskArray[List->CurrentDisk].PartArray[List->CurrentPartition].PartSize;
  Data->PartNumber = List->DiskArray[List->CurrentDisk].PartArray[List->CurrentPartition].PartNumber;
  Data->PartType = List->DiskArray[List->CurrentDisk].PartArray[List->CurrentPartition].PartType;

  return(TRUE);
}

/* EOF */
