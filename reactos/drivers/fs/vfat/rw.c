
/* $Id: rw.c,v 1.17 2001/01/13 18:38:09 dwelch Exp $
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

/* GLOBALS *******************************************************************/

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N, S) ((N) - ((N) % (S)))

/* FUNCTIONS *****************************************************************/

NTSTATUS
NextCluster(PDEVICE_EXTENSION DeviceExt,
	    ULONG FirstCluster,
	    PULONG CurrentCluster)
{
  if (FirstCluster == 1)
    {
      (*CurrentCluster) += DeviceExt->Boot->SectorsPerCluster;
      return(STATUS_SUCCESS);
    }
  else
    {
      NTSTATUS Status;

      Status = GetNextCluster(DeviceExt, (*CurrentCluster), CurrentCluster);
      return(Status);
    }
}

NTSTATUS
OffsetToCluster(PDEVICE_EXTENSION DeviceExt, 
		ULONG FirstCluster, 
		ULONG FileOffset,
		PULONG Cluster)
{
  ULONG CurrentCluster;
  ULONG i;
  NTSTATUS Status;

  if (FirstCluster == 1)
    {				
      /* root of FAT16 or FAT12 */
      *Cluster = DeviceExt->rootStart + FileOffset
	/ (DeviceExt->BytesPerCluster) * DeviceExt->Boot->SectorsPerCluster;
      return(STATUS_SUCCESS);
    }
  else
    {
      CurrentCluster = FirstCluster;
      for (i = 0; i < FileOffset / DeviceExt->BytesPerCluster; i++)
	{
	  Status = GetNextCluster (DeviceExt, CurrentCluster, &CurrentCluster);
	  if (!NT_SUCCESS(Status))
	    {
	      return(Status);
	    }
	}
      *Cluster = CurrentCluster;
      return(STATUS_SUCCESS);
    }
}

NTSTATUS
VfatRawReadCluster (PDEVICE_EXTENSION DeviceExt, 
		    ULONG FirstCluster,
		    PULONG CurrentCluster,
		    PVOID Destination)
{
  NTSTATUS Status;

  if (FirstCluster == 1)
    {
      Status = VfatReadSectors (DeviceExt->StorageDevice,
				(*CurrentCluster),
				DeviceExt->Boot->SectorsPerCluster, 
				Destination);
      return(Status);
    }
  else
    {
      VFATLoadCluster (DeviceExt, Destination, (*CurrentCluster));
      Status = STATUS_SUCCESS;
      return(Status);
    }
}

NTSTATUS
VfatReadCluster(PDEVICE_EXTENSION DeviceExt,
		PVFATFCB Fcb,
		ULONG StartOffset,
		ULONG FirstCluster,
		PULONG CurrentCluster,
		PVOID Destination,
		ULONG NoCache,
		ULONG InternalOffset,
		ULONG InternalLength)
{
  ULONG BytesPerCluster;
  BOOLEAN Valid;
  PVOID BaseAddress;
  PCACHE_SEGMENT CacheSeg;
  NTSTATUS Status;

  BytesPerCluster = DeviceExt->Boot->SectorsPerCluster * BLOCKSIZE;
  
  if (BytesPerCluster >= PAGESIZE)
    {
      /*
       * In this case the size of a cache segment is the same as a cluster
       */

      if (!NoCache)
	{
	  Status = CcRequestCacheSegment(Fcb->RFCB.Bcb,
					 StartOffset,
					 &BaseAddress,
					 &Valid,
					 &CacheSeg);
	  if (!NT_SUCCESS(Status))
	    {
	      return(Status);
	    }
	}
      else
	{
	  Valid = FALSE;
	}
      if (!Valid)
	{
	  /*
	   * If necessary read the cluster from the disk
	   */
	  Status = VfatRawReadCluster(DeviceExt, FirstCluster, CurrentCluster,
				      BaseAddress);
	  if (!NT_SUCCESS(Status))
	    {
	      if (!NoCache)
		{
		  CcReleaseCacheSegment(Fcb->RFCB.Bcb, CacheSeg, FALSE);
		}
	      return(Status);
	    }
	}
      /*
       * Copy the data from the cache to the caller
       */
      memcpy(Destination, BaseAddress + InternalOffset, InternalLength);
      if (!NoCache)
	{
	  CcReleaseCacheSegment(Fcb->RFCB.Bcb, CacheSeg, TRUE);
	}

      Status = NextCluster(DeviceExt, FirstCluster, CurrentCluster);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
    }
  else
    {
      ULONG i;
      
      /*
       * Otherwise we read a page of clusters together
       */

      if (!NoCache)
	{
	  Status = CcRequestCacheSegment(Fcb->RFCB.Bcb, 
					 StartOffset,
					 &BaseAddress,
					 &Valid,
					 &CacheSeg);
	  if (!NT_SUCCESS(Status))
	    {
	      return(Status);
	    }
	}
      else
	{
	  Valid = FALSE;
	}
      
      /*
       * If necessary read all the data for the page, unfortunately the
       * file length may not be page aligned in which case the page will
       * only be partially filled.
       * FIXME: So zero fill the rest?
       */
      if (!Valid)
	{
	  for (i = 0; i < (PAGESIZE / BytesPerCluster); i++)
	    {	  
	      Status = VfatRawReadCluster(DeviceExt, 
					  FirstCluster,
					  CurrentCluster,
					  BaseAddress + (i * BytesPerCluster));
	      if (!NT_SUCCESS(Status))
		{
		  if (!NoCache)
		    {
		      CcReleaseCacheSegment(Fcb->RFCB.Bcb, CacheSeg, FALSE);
		    }
		  return(Status);
		}
	      Status = NextCluster(DeviceExt, FirstCluster, CurrentCluster);
	      if ((*CurrentCluster) == 0xFFFFFFFF)
		{
		  break;
		}
	    }
	}
      else
	{
	  /*
	   * Otherwise just move onto the next cluster
	   */
	  for (i = 0; i < (PAGESIZE / DeviceExt->BytesPerCluster); i++)
	    {
	      NextCluster(DeviceExt, FirstCluster, CurrentCluster);
	      if ((*CurrentCluster) == 0xFFFFFFFF)
		{
		  break;
		}
	    }
	}
      /*
       * Copy the data from the cache to the caller
       */
      memcpy(Destination, BaseAddress + InternalOffset, InternalLength);
      if (!NoCache)
	{
	  CcReleaseCacheSegment(Fcb->RFCB.Bcb, CacheSeg, TRUE);
	}
    }
  return(STATUS_SUCCESS);
}

NTSTATUS
VfatReadFile (PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
	      PVOID Buffer, ULONG Length, ULONG ReadOffset, 
	      PULONG LengthRead, ULONG NoCache)
/*
 * FUNCTION: Reads data from a file
 */
{
  ULONG CurrentCluster;
  ULONG FirstCluster;
  PVFATFCB Fcb;
  ULONG ChunkSize;
  NTSTATUS Status;
  ULONG TempLength;

  /* PRECONDITION */
  assert (DeviceExt != NULL);
  assert (DeviceExt->BytesPerCluster != 0);
  assert (FileObject != NULL);
  assert (FileObject->FsContext != NULL);

  DPRINT ("VfatReadFile(DeviceExt %x, FileObject %x, Buffer %x, "
	  "Length %d, ReadOffset 0x%x)\n", DeviceExt, FileObject, Buffer,
	  Length, ReadOffset);

  Fcb = ((PVFATCCB)FileObject->FsContext2)->pFcb;

  /*
   * Find the first cluster
   */
  if (DeviceExt->FatType == FAT32)
    CurrentCluster = Fcb->entry.FirstCluster 
      + Fcb->entry.FirstClusterHigh * 65536;
  else
    CurrentCluster = Fcb->entry.FirstCluster;
  FirstCluster = CurrentCluster;
  
  /*
   * Truncate the read if necessary
   */
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

  if (DeviceExt->BytesPerCluster >= PAGESIZE)
    {
      ChunkSize = DeviceExt->BytesPerCluster;
    }
  else
    {
      ChunkSize = PAGESIZE;
    }

  *LengthRead = 0;  

  /*
   * Find the cluster to start the read from
   * FIXME: Optimize by remembering the last cluster read and using if 
   * possible.
   */
  Status = OffsetToCluster(DeviceExt, FirstCluster, ReadOffset, 
			   &CurrentCluster);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  
  /*
   * If the read doesn't begin on a cluster boundary then read a full
   * cluster and copy it.
   */
  if ((ReadOffset % ChunkSize) != 0)
    {      
      TempLength = min (Length, ChunkSize - (ReadOffset % ChunkSize));
      VfatReadCluster(DeviceExt, Fcb, 
		      ROUND_DOWN(ReadOffset, ChunkSize),
		      FirstCluster, &CurrentCluster, Buffer, NoCache,
		      ReadOffset % ChunkSize, TempLength);

      (*LengthRead) = (*LengthRead) + TempLength;
      Length = Length - TempLength;
      Buffer = Buffer + TempLength;
      ReadOffset = ReadOffset + TempLength;
    }

  while (Length >= ChunkSize)
    {
      VfatReadCluster(DeviceExt, Fcb, ReadOffset,
		      FirstCluster, &CurrentCluster, Buffer, NoCache, 0, 
		      ChunkSize);
      if (CurrentCluster == 0xffffffff)
	{
	  return (STATUS_SUCCESS);
	}

      (*LengthRead) = (*LengthRead) + ChunkSize;
      Buffer = Buffer + ChunkSize;
      Length = Length - ChunkSize;
      ReadOffset = ReadOffset + ChunkSize;
    }
  if (Length > 0)
    {
      VfatReadCluster(DeviceExt, Fcb, ReadOffset,
		      FirstCluster, &CurrentCluster, Buffer, NoCache, 0, 
		      Length);
      (*LengthRead) = (*LengthRead) + Length;
    }
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

  DPRINT1 ("VfatWriteFile(FileObject %x, Buffer %x, Length %x, "
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
	  //	  CurrentCluster = GetNextWriteCluster (DeviceExt, 0);
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
#if 0
	      CurrentCluster =
		GetNextWriteCluster (DeviceExt, CurrentCluster);
#endif
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
	  /* FIXME: Check status */
	  VfatReadSectors (DeviceExt->StorageDevice,
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
	  VfatWriteSectors (DeviceExt->StorageDevice,
			    CurrentCluster,
			    DeviceExt->Boot->SectorsPerCluster, Temp);
	  CurrentCluster += DeviceExt->Boot->SectorsPerCluster;
	}
      else
	{
	  VfatWriteCluster (DeviceExt, Temp, CurrentCluster);
#if 0
	  if (Length2 > 0)
	    CurrentCluster = GetNextWriteCluster (DeviceExt, CurrentCluster);
#endif
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
	  VfatWriteSectors (DeviceExt->StorageDevice,
			    CurrentCluster,
			    DeviceExt->Boot->SectorsPerCluster, Buffer);
	  CurrentCluster += DeviceExt->Boot->SectorsPerCluster;
	}
      else
	{
	  VfatWriteCluster (DeviceExt, Buffer, CurrentCluster);
#if 0
	  if (Length2 > 0)	    
	    CurrentCluster = GetNextWriteCluster (DeviceExt, CurrentCluster);
#endif
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
	  /* FIXME: Check status */
	  VfatReadSectors (DeviceExt->StorageDevice,
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
	      VfatWriteSectors (DeviceExt->StorageDevice,
				CurrentCluster,
				DeviceExt->Boot->SectorsPerCluster, Temp);
	    }
	  else
	    {
	      VfatWriteCluster (DeviceExt, Temp, CurrentCluster);
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
      //      updEntry (DeviceExt, FileObject);
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
  ULONG NoCache;

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
  
  if (Irp->Flags & IRP_PAGING_IO ||
      FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING)
    {
      NoCache = TRUE;
    }
  else
    {
      NoCache = FALSE;
    }

  /* fail if file is a directory */
  Fcb = ((PVFATCCB) (FileObject->FsContext2))->pFcb;
  if (Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY)
    {
      Status = STATUS_FILE_IS_A_DIRECTORY;
    }
  else
    {
      Status = VfatReadFile (DeviceExt,
			     FileObject, Buffer, Length, Offset, &LengthRead,
			     NoCache);
    }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = LengthRead;
  IoCompleteRequest (Irp, IO_NO_INCREMENT);

  return (Status);
}
