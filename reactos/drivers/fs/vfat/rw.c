/* $Id: rw.c,v 1.12 2000/12/29 23:17:12 dwelch Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/rw.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
VfatReadFile (PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
	      PVOID Buffer, ULONG Length, ULONG ReadOffset, PULONG LengthRead)
/*
 * FUNCTION: Reads data from a file
 */
{
  ULONG CurrentCluster;
  ULONG FileOffset;
  ULONG FirstCluster;
  PVFATFCB Fcb;
  PVOID Temp;
  ULONG TempLength;

  /* PRECONDITION */
  assert (DeviceExt != NULL);
  assert (DeviceExt->BytesPerCluster != 0);
  assert (FileObject != NULL);
  assert (FileObject->FsContext != NULL);

  DPRINT ("FsdReadFile(DeviceExt %x, FileObject %x, Buffer %x, "
	  "Length %d, ReadOffset %d)\n", DeviceExt, FileObject, Buffer,
	  Length, ReadOffset);

  Fcb = ((PVFATCCB) (FileObject->FsContext2))->pFcb;
  if (DeviceExt->FatType == FAT32)
    CurrentCluster = Fcb->entry.FirstCluster
      + Fcb->entry.FirstClusterHigh * 65536;
  else
    CurrentCluster = Fcb->entry.FirstCluster;
  FirstCluster = CurrentCluster;
  DPRINT ("DeviceExt->BytesPerCluster %x\n", DeviceExt->BytesPerCluster);

  if (!(Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY))
    {
      if (ReadOffset >= Fcb->entry.FileSize)
	{
	  return (STATUS_END_OF_FILE);
	}
      if ((ReadOffset + Length) > Fcb->entry.FileSize)
	{
	  Length = Fcb->entry.FileSize - ReadOffset;
	}
    }
  CHECKPOINT;
  *LengthRead = 0;
  /* FIXME: optimize by remembering the last cluster read and using if possible */
  Temp = ExAllocatePool (NonPagedPool, DeviceExt->BytesPerCluster);
  if (!Temp)
    return STATUS_UNSUCCESSFUL;
  if (FirstCluster == 1)
    {				//root of FAT16 or FAT12
      CurrentCluster = DeviceExt->rootStart + ReadOffset
	/ (DeviceExt->BytesPerCluster) * DeviceExt->Boot->SectorsPerCluster;
    }
  else
    for (FileOffset = 0; FileOffset < ReadOffset / DeviceExt->BytesPerCluster;
	 FileOffset++)
      {
	CurrentCluster = GetNextCluster (DeviceExt, CurrentCluster);
      }
  CHECKPOINT;
  if ((ReadOffset % DeviceExt->BytesPerCluster) != 0)
    {
      if (FirstCluster == 1)
	{
	  VFATReadSectors (DeviceExt->StorageDevice,
			   CurrentCluster,
			   DeviceExt->Boot->SectorsPerCluster, Temp);
	  CurrentCluster += DeviceExt->Boot->SectorsPerCluster;
	}
      else
	{
	  VFATLoadCluster (DeviceExt, Temp, CurrentCluster);
	  CurrentCluster = GetNextCluster (DeviceExt, CurrentCluster);
	}
      TempLength = min (Length, DeviceExt->BytesPerCluster -
			(ReadOffset % DeviceExt->BytesPerCluster));

      memcpy (Buffer, Temp + ReadOffset % DeviceExt->BytesPerCluster,
	      TempLength);

      (*LengthRead) = (*LengthRead) + TempLength;
      Length = Length - TempLength;
      Buffer = Buffer + TempLength;
    }
  CHECKPOINT;
  while (Length >= DeviceExt->BytesPerCluster)
    {
      if (FirstCluster == 1)
	{
	  VFATReadSectors (DeviceExt->StorageDevice,
			   CurrentCluster,
			   DeviceExt->Boot->SectorsPerCluster, Buffer);
	  CurrentCluster += DeviceExt->Boot->SectorsPerCluster;
	}
      else
	{
	  VFATLoadCluster (DeviceExt, Buffer, CurrentCluster);
	  CurrentCluster = GetNextCluster (DeviceExt, CurrentCluster);
	}
      if (CurrentCluster == 0xffffffff)
	{
	  ExFreePool (Temp);
	  return (STATUS_SUCCESS);
	}

      (*LengthRead) = (*LengthRead) + DeviceExt->BytesPerCluster;
      Buffer = Buffer + DeviceExt->BytesPerCluster;
      Length = Length - DeviceExt->BytesPerCluster;
    }
  CHECKPOINT;
  if (Length > 0)
    {
      (*LengthRead) = (*LengthRead) + Length;
      if (FirstCluster == 1)
	{
	  VFATReadSectors (DeviceExt->StorageDevice,
			   CurrentCluster,
			   DeviceExt->Boot->SectorsPerCluster, Temp);
	  CurrentCluster += DeviceExt->Boot->SectorsPerCluster;
	}
      else
	{
	  VFATLoadCluster (DeviceExt, Temp, CurrentCluster);
	  CurrentCluster = GetNextCluster (DeviceExt, CurrentCluster);
	}
      memcpy (Buffer, Temp, Length);
    }
  ExFreePool (Temp);
  return (STATUS_SUCCESS);
}

NTSTATUS
VfatWriteFile (PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
	       PVOID Buffer, ULONG Length, ULONG WriteOffset)
/*
 * FUNCTION: Writes data to file
 */
{
  ULONG CurrentCluster;
  ULONG FileOffset;
  ULONG FirstCluster;
  PVFATFCB Fcb;
  PVFATCCB pCcb;
  PVOID Temp;
  ULONG TempLength, Length2 = Length;
  LARGE_INTEGER SystemTime, LocalTime;

  DPRINT1 ("FsdWriteFile(FileObject %x, Buffer %x, Length %x, "
	   "WriteOffset %x\n", FileObject, Buffer, Length, WriteOffset);

  /* Locate the first cluster of the file */
  assert (FileObject);
  pCcb = (PVFATCCB) (FileObject->FsContext2);
  assert (pCcb);
  Fcb = pCcb->pFcb;
  assert (Fcb);
  if (DeviceExt->FatType == FAT32)
    {
      CurrentCluster =
	Fcb->entry.FirstCluster + Fcb->entry.FirstClusterHigh * 65536;
    }
  else
    {
      CurrentCluster = Fcb->entry.FirstCluster;
    }
  FirstCluster = CurrentCluster;

  /* Allocate a buffer to hold 1 cluster of data */
  Temp = ExAllocatePool (NonPagedPool, DeviceExt->BytesPerCluster);
  assert (Temp);

  /* Find the cluster according to the offset in the file */
  if (CurrentCluster == 1)
    {
      CurrentCluster = DeviceExt->rootStart + WriteOffset
	/ DeviceExt->BytesPerCluster * DeviceExt->Boot->SectorsPerCluster;
    }
  else
    {
      if (CurrentCluster == 0)
	{
	  /*
	   * File of size zero
	   */
	  CurrentCluster = GetNextWriteCluster (DeviceExt, 0);
	  if (DeviceExt->FatType == FAT32)
	    {
	      Fcb->entry.FirstClusterHigh = CurrentCluster >> 16;
	      Fcb->entry.FirstCluster = CurrentCluster;
	    }
	  else
	    Fcb->entry.FirstCluster = CurrentCluster;
	}
      else
	{
	  for (FileOffset = 0;
	       FileOffset < WriteOffset / DeviceExt->BytesPerCluster;
	       FileOffset++)
	    {
	      CurrentCluster =
		GetNextWriteCluster (DeviceExt, CurrentCluster);
	    }
	}
      CHECKPOINT;
    }

  /*
   * If the offset in the cluster doesn't fall on the cluster boundary 
   * then we have to write only from the specified offset
   */

  if ((WriteOffset % DeviceExt->BytesPerCluster) != 0)
    {
      CHECKPOINT;
      TempLength = min (Length, DeviceExt->BytesPerCluster -
			(WriteOffset % DeviceExt->BytesPerCluster));
      /* Read in the existing cluster data */
      if (FirstCluster == 1)
	{
	  VFATReadSectors (DeviceExt->StorageDevice,
			   CurrentCluster,
			   DeviceExt->Boot->SectorsPerCluster, Temp);
	}
      else
	{
	  VFATLoadCluster (DeviceExt, Temp, CurrentCluster);
	}

      /* Overwrite the last parts of the data as necessary */
      memcpy (Temp + (WriteOffset % DeviceExt->BytesPerCluster),
	      Buffer, TempLength);

      /* Write the cluster back */
      Length2 -= TempLength;
      if (FirstCluster == 1)
	{
	  VFATWriteSectors (DeviceExt->StorageDevice,
			    CurrentCluster,
			    DeviceExt->Boot->SectorsPerCluster, Temp);
	  CurrentCluster += DeviceExt->Boot->SectorsPerCluster;
	}
      else
	{
	  VFATWriteCluster (DeviceExt, Temp, CurrentCluster);
	  if (Length2 > 0)
	    CurrentCluster = GetNextWriteCluster (DeviceExt, CurrentCluster);
	}
      Buffer = Buffer + TempLength;
    }
  CHECKPOINT;

  /* Write the buffer in chunks of 1 cluster */

  while (Length2 >= DeviceExt->BytesPerCluster)
    {
      CHECKPOINT;
      if (CurrentCluster == 0)
	{
	  ExFreePool (Temp);
	  return (STATUS_UNSUCCESSFUL);
	}
      Length2 -= DeviceExt->BytesPerCluster;
      if (FirstCluster == 1)
	{
	  VFATWriteSectors (DeviceExt->StorageDevice,
			    CurrentCluster,
			    DeviceExt->Boot->SectorsPerCluster, Buffer);
	  CurrentCluster += DeviceExt->Boot->SectorsPerCluster;
	}
      else
	{
	  VFATWriteCluster (DeviceExt, Buffer, CurrentCluster);
	  if (Length2 > 0)
	    CurrentCluster = GetNextWriteCluster (DeviceExt, CurrentCluster);
	}
      Buffer = Buffer + DeviceExt->BytesPerCluster;
    }
  CHECKPOINT;

  /* Write the remainder */

  if (Length2 > 0)
    {
      CHECKPOINT;
      if (CurrentCluster == 0)
	{
	  ExFreePool (Temp);
	  return (STATUS_UNSUCCESSFUL);
	}
      CHECKPOINT;
      /* Read in the existing cluster data */
      if (FirstCluster == 1)
	{
	  VFATReadSectors (DeviceExt->StorageDevice,
			   CurrentCluster,
			   DeviceExt->Boot->SectorsPerCluster, Temp);
	}
      else
	{
	  VFATLoadCluster (DeviceExt, Temp, CurrentCluster);
	  CHECKPOINT;
	  memcpy (Temp, Buffer, Length2);
	  CHECKPOINT;
	  if (FirstCluster == 1)
	    {
	      VFATWriteSectors (DeviceExt->StorageDevice,
				CurrentCluster,
				DeviceExt->Boot->SectorsPerCluster, Temp);
	    }
	  else
	    {
	      VFATWriteCluster (DeviceExt, Temp, CurrentCluster);
	    }
	}
      CHECKPOINT;
    }


  /* set dates and times */
  KeQuerySystemTime (&SystemTime);
  ExSystemTimeToLocalTime (&SystemTime, &LocalTime);
  FsdFileTimeToDosDateTime ((TIME *) & LocalTime,
			    &Fcb->entry.UpdateDate, &Fcb->entry.UpdateTime);
  Fcb->entry.AccessDate = Fcb->entry.UpdateDate;

  if (Fcb->entry.FileSize < WriteOffset + Length
      && !(Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY))
    {
      Fcb->entry.FileSize = WriteOffset + Length;
      /*
       * update entry in directory
       */
      updEntry (DeviceExt, FileObject);
    }

  ExFreePool (Temp);
  return (STATUS_SUCCESS);
}

NTSTATUS STDCALL
VfatWrite (PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Write to a file
 */
{
  ULONG Length;
  PVOID Buffer;
  ULONG Offset;
  PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation (Irp);
  PFILE_OBJECT FileObject = Stack->FileObject;
  PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
  NTSTATUS Status;

  DPRINT ("VfatWrite(DeviceObject %x Irp %x)\n", DeviceObject, Irp);

  Length = Stack->Parameters.Write.Length;
  Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);
  Offset = Stack->Parameters.Write.ByteOffset.u.LowPart;

  Status = VfatWriteFile (DeviceExt, FileObject, Buffer, Length, Offset);

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = Length;
  IoCompleteRequest (Irp, IO_NO_INCREMENT);

  return (Status);
}

NTSTATUS STDCALL
VfatRead (PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Read from a file
 */
{
  ULONG Length;
  PVOID Buffer;
  ULONG Offset;
  PIO_STACK_LOCATION Stack;
  PFILE_OBJECT FileObject;
  PDEVICE_EXTENSION DeviceExt;
  NTSTATUS Status;
  ULONG LengthRead;
  PVFATFCB Fcb;

  DPRINT ("VfatRead(DeviceObject %x, Irp %x)\n", DeviceObject, Irp);

  /* Precondition / Initialization */
  assert (Irp != NULL);
  Stack = IoGetCurrentIrpStackLocation (Irp);
  assert (Stack != NULL);
  FileObject = Stack->FileObject;
  assert (FileObject != NULL);
  DeviceExt = DeviceObject->DeviceExtension;
  assert (DeviceExt != NULL);

  Length = Stack->Parameters.Read.Length;
  Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);
  Offset = Stack->Parameters.Read.ByteOffset.u.LowPart;

  /* fail if file is a directory */
  Fcb = ((PVFATCCB) (FileObject->FsContext2))->pFcb;
  if (Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY)
    {
      Status = STATUS_FILE_IS_A_DIRECTORY;
    }
  else
    {
      Status = VfatReadFile (DeviceExt,
			     FileObject, Buffer, Length, Offset, &LengthRead);
    }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = LengthRead;
  IoCompleteRequest (Irp, IO_NO_INCREMENT);

  return (Status);
}
