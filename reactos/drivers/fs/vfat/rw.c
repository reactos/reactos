
/* $Id: rw.c,v 1.25 2001/07/20 08:00:20 ekohl Exp $
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
	    PULONG CurrentCluster,
	    BOOLEAN Extend)
     /*
      * Return the next cluster in a FAT chain, possibly extending the chain if
      * necessary
      */
{
  if (FirstCluster == 1)
    {
      (*CurrentCluster) += DeviceExt->Boot->SectorsPerCluster;
      return(STATUS_SUCCESS);
    }
  else
 /* CN: FIXME: Real bug here or in dirwr, where CurrentCluster isn't initialized when 0*/
  if (FirstCluster == 0)
    {
      NTSTATUS Status;

      Status = GetNextCluster(DeviceExt, 0, CurrentCluster,
			      Extend);
      return(Status);
    }
  else
    {
      NTSTATUS Status;

      Status = GetNextCluster(DeviceExt, (*CurrentCluster), CurrentCluster,
			      Extend);
      return(Status);
    }
}

NTSTATUS
OffsetToCluster(PDEVICE_EXTENSION DeviceExt, 
		ULONG FirstCluster, 
		ULONG FileOffset,
		PULONG Cluster,
		BOOLEAN Extend)
     /*
      * Return the cluster corresponding to an offset within a file,
      * possibly extending the file if necessary
      */
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
	  Status = GetNextCluster (DeviceExt, CurrentCluster, &CurrentCluster,
				   Extend);
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
VfatReadBigCluster(PDEVICE_EXTENSION DeviceExt,
		PVFATFCB Fcb,
		ULONG StartOffset,
		ULONG FirstCluster,
		PULONG CurrentCluster,
		PVOID Destination,
		ULONG NoCache,
		ULONG InternalOffset,
		ULONG InternalLength)
{
  BOOLEAN Valid;
  PVOID BaseAddress = NULL;
  PCACHE_SEGMENT CacheSeg = NULL;
  NTSTATUS Status;  
  
  /*
   * In this case the size of a cache segment is the same as a cluster
   */
  
  if (!NoCache)
    {
      Status = CcRosRequestCacheSegment(Fcb->RFCB.Bcb,
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
      if (InternalOffset == 0)
	{
	  BaseAddress = Destination;
	}
      else
	{
	  BaseAddress = ExAllocatePool(NonPagedPool, 
				       DeviceExt->BytesPerCluster);
	  if (BaseAddress == NULL)
	    {
	      return(STATUS_NO_MEMORY);
	    }
	}
    }

  if (!Valid)
    {
      /*
       * If necessary read the cluster from the disk
       */
      Status = VfatRawReadCluster(DeviceExt, FirstCluster, BaseAddress,
				  *CurrentCluster);
      if (!NT_SUCCESS(Status))
	{
	  if (!NoCache)
	    {
	      CcRosReleaseCacheSegment(Fcb->RFCB.Bcb, CacheSeg, FALSE);
	    }
	  else if (InternalOffset != 0)
	    {
	      ExFreePool(BaseAddress);
	    }
	  return(Status);
	}
    }
  /*
   * Copy the data from the cache to the caller
   */
  if (InternalOffset != 0 || !NoCache)
    {
      memcpy(Destination, BaseAddress + InternalOffset, InternalLength);
    }
  if (!NoCache)
    {
      CcRosReleaseCacheSegment(Fcb->RFCB.Bcb, CacheSeg, TRUE);
    }
  else if (InternalOffset != 0)
    {
      ExFreePool(BaseAddress);
    }
  
  Status = NextCluster(DeviceExt, FirstCluster, CurrentCluster, FALSE);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  return(STATUS_SUCCESS);
}

NTSTATUS
VfatReadSmallCluster(PDEVICE_EXTENSION DeviceExt,
		     PVFATFCB Fcb,
		     ULONG StartOffset,
		     ULONG FirstCluster,
		     PULONG CurrentCluster,
		     PVOID Destination,
		     ULONG NoCache,
		     ULONG InternalOffset,
		     ULONG InternalLength)
{
  BOOLEAN Valid;
  PVOID BaseAddress = NULL;
  PCACHE_SEGMENT CacheSeg = NULL; 
  NTSTATUS Status;  
  ULONG i;
  
  /*
   * Otherwise we read a page of clusters together
   */
  if (!NoCache)
    {
      Status = CcRosRequestCacheSegment(Fcb->RFCB.Bcb, 
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
      if (InternalOffset == 0)
	{
	  BaseAddress = Destination;
	}
      else
	{
	  BaseAddress = ExAllocatePool(NonPagedPool, PAGESIZE);
	  if (BaseAddress == NULL)
	    {
	      return(STATUS_NO_MEMORY);
	    }
	}
    }
  
  /*
   * If necessary read all the data for the page, unfortunately the
   * file length may not be page aligned in which case the page will
   * only be partially filled.
   * FIXME: So zero fill the rest?
   */
  if (!Valid)
    {
      for (i = 0; i < (PAGESIZE / DeviceExt->BytesPerCluster); i++)
	{	  
	  Status = VfatRawReadCluster(DeviceExt, 
				      FirstCluster,
				      BaseAddress + 
				      (i * DeviceExt->BytesPerCluster),
				      *CurrentCluster);
	  if (!NT_SUCCESS(Status))
	    {
	      if (!NoCache)
		{
		  CcRosReleaseCacheSegment(Fcb->RFCB.Bcb, CacheSeg, FALSE);
		}
	      else if (InternalOffset != 0)
		{
		  ExFreePool(BaseAddress);
		}
	      return(Status);
	    }
	  Status = NextCluster(DeviceExt, FirstCluster, CurrentCluster, FALSE);
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
	  NextCluster(DeviceExt, FirstCluster, CurrentCluster, FALSE);
	  if ((*CurrentCluster) == 0xFFFFFFFF)
	    {
	      break;
	    }
	}
    }
  /*
   * Copy the data from the cache to the caller
   */
  if (InternalOffset != 0 || !NoCache)
    {
      memcpy(Destination, BaseAddress + InternalOffset, InternalLength);
    }
  if (!NoCache)
    {
      CcRosReleaseCacheSegment(Fcb->RFCB.Bcb, CacheSeg, TRUE);
    }
  else if (InternalOffset != 0)
    {
      ExFreePool(BaseAddress);
    }
  return(STATUS_SUCCESS);
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
  if (DeviceExt->BytesPerCluster >= PAGESIZE)
    {
      return(VfatReadBigCluster(DeviceExt,
				Fcb,
				StartOffset,
				FirstCluster,
				CurrentCluster,
				Destination,
				NoCache,
				InternalOffset,
				InternalLength));
    }
  else
    {
      return(VfatReadSmallCluster(DeviceExt,
				  Fcb,
				  StartOffset,
				  FirstCluster,
				  CurrentCluster,
				  Destination,
				  NoCache,
				  InternalOffset,
				  InternalLength));
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

  DPRINT("VfatReadFile(DeviceExt %x, FileObject %x, Buffer %x, "
	 "Length %d, ReadOffset 0x%x)\n", DeviceExt, FileObject, Buffer,
	 Length, ReadOffset);

  *LengthRead = 0;

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
  DPRINT( "FirstCluster = %x\n", FirstCluster );
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
  
  /*
   * Select an appropiate size for reads
   */
  if (DeviceExt->BytesPerCluster >= PAGESIZE)
    {
      ChunkSize = DeviceExt->BytesPerCluster;
    }
  else
    {
      ChunkSize = PAGESIZE;
    }

  /*
   * Find the cluster to start the read from
   * FIXME: Optimize by remembering the last cluster read and using if 
   * possible.
   */
  Status = OffsetToCluster(DeviceExt,
			   FirstCluster,
			   ROUND_DOWN(ReadOffset, ChunkSize),
			   &CurrentCluster,
			   FALSE);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  
  /*
   * If the read doesn't begin on a chunk boundary then we need special
   * handling
   */
  if ((ReadOffset % ChunkSize) != 0 )
    {      
      TempLength = min (Length, ChunkSize - (ReadOffset % ChunkSize));
      VfatReadCluster(DeviceExt, Fcb, ROUND_DOWN(ReadOffset, ChunkSize),
		      FirstCluster, &CurrentCluster, Buffer, NoCache,
		      ReadOffset % ChunkSize, TempLength);

      (*LengthRead) = (*LengthRead) + TempLength;
      Length = Length - TempLength;
      Buffer = Buffer + TempLength;
      ReadOffset = ReadOffset + TempLength;
    }

  while (Length >= ChunkSize && CurrentCluster)
    {
      VfatReadCluster(DeviceExt, Fcb, ReadOffset,
		      FirstCluster, &CurrentCluster, Buffer, NoCache, 0, 
		      ChunkSize);

      (*LengthRead) = (*LengthRead) + ChunkSize;
      Buffer = Buffer + ChunkSize;
      Length = Length - ChunkSize;
      ReadOffset = ReadOffset + ChunkSize;
    }
  if (Length > 0 && CurrentCluster)
    {
      VfatReadCluster(DeviceExt, Fcb, ReadOffset,
		      FirstCluster, &CurrentCluster, Buffer, NoCache, 0, 
		      Length);
      (*LengthRead) = (*LengthRead) + Length;
    }
  return (STATUS_SUCCESS);
}

NTSTATUS
VfatWriteBigCluster(PDEVICE_EXTENSION DeviceExt,
		    PVFATFCB Fcb,
		    ULONG StartOffset,
		    ULONG FirstCluster,
		    PULONG CurrentCluster,
		    PVOID Source,
		    ULONG NoCache,
		    ULONG InternalOffset,
		    ULONG InternalLength,
		    BOOLEAN Extend)
{
  BOOLEAN Valid;
  PVOID BaseAddress;
  PCACHE_SEGMENT CacheSeg;
  NTSTATUS Status;

  /*
   * In this case the size of a cache segment is the same as a cluster
   */  
  if (!NoCache)
    {
      Status = CcRosRequestCacheSegment(Fcb->RFCB.Bcb,
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
      /*
       * If we are bypassing the cache and not writing starting on
       * cluster boundary then allocate a temporary buffer 
       */
      if (InternalOffset != 0)
	{
	  BaseAddress = ExAllocatePool(NonPagedPool,
				       DeviceExt->BytesPerCluster);
	  if (BaseAddress == NULL)
	    {
	      return(STATUS_NO_MEMORY);
	    }
	}
    }
  if (!Valid && InternalLength != DeviceExt->BytesPerCluster)
    {
      /*
       * If the data in the cache isn't valid or we are bypassing the
       * cache and not writing a cluster aligned, cluster sized region
       * then read data in to base address
       */
      Status = VfatRawReadCluster(DeviceExt, FirstCluster, BaseAddress,
				  *CurrentCluster);
      if (!NT_SUCCESS(Status))
	{
	  if (!NoCache)
	    {
	      CcRosReleaseCacheSegment(Fcb->RFCB.Bcb, CacheSeg, FALSE);
	    }
	  else if (InternalOffset != 0)
	    {
	      ExFreePool(BaseAddress);
	    }
	  return(Status);
	}
    }
  if (!NoCache || InternalLength != DeviceExt->BytesPerCluster)
    {
      /*
       * If we are writing into the cache or we are writing from a
       * temporary buffer then copy the data over
       */
      DPRINT("InternalOffset 0x%x InternalLength 0x%x BA %x\n",
	      InternalOffset, InternalLength, BaseAddress);
      memcpy(BaseAddress + InternalOffset, Source, InternalLength);
    }
  /*
   * Write the data back to disk
   */
  DPRINT("Writing 0x%x\n", *CurrentCluster);
  Status = VfatRawWriteCluster(DeviceExt, FirstCluster, BaseAddress,
			       *CurrentCluster);
  if (!NoCache)
    {
      CcRosReleaseCacheSegment(Fcb->RFCB.Bcb, CacheSeg, TRUE);
    }
  else if (InternalOffset != 0)
    {
      ExFreePool(BaseAddress);
    }
  
  Status = NextCluster(DeviceExt, FirstCluster, CurrentCluster, Extend);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  return(STATUS_SUCCESS);
}

NTSTATUS
VfatWriteSmallCluster(PDEVICE_EXTENSION DeviceExt,
		      PVFATFCB Fcb,
		      ULONG StartOffset,
		      ULONG FirstCluster,
		      PULONG CurrentCluster,
		      PVOID Source,
		      ULONG NoCache,
		      ULONG InternalOffset,
		      ULONG InternalLength,
		      BOOLEAN Extend)
{
  BOOLEAN Valid;
  PVOID BaseAddress;
  PCACHE_SEGMENT CacheSeg;
  NTSTATUS Status;
  ULONG NCluster;
  ULONG i;
  
  /*
   * Otherwise we read a page of clusters together
   */
  
  if (!NoCache)
    {
      Status = CcRosRequestCacheSegment(Fcb->RFCB.Bcb, 
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
      if (InternalOffset != 0)
	{
	  BaseAddress = ExAllocatePool(NonPagedPool, PAGESIZE);
	  if (BaseAddress == NULL)
	    {
	      return(STATUS_NO_MEMORY);
	    }
	}
      else
	{
	  BaseAddress = Source;
	}
    }
      
  /*
   * If necessary read all the data for the page, unfortunately the
   * file length may not be page aligned in which case the page will
   * only be partially filled.
   * FIXME: So zero fill the rest?
   */
  if (!Valid || InternalLength != PAGESIZE)
    {
      NCluster = *CurrentCluster;

      for (i = 0; i < (PAGESIZE / DeviceExt->BytesPerCluster); i++)
	{	  
	  Status = VfatRawReadCluster(DeviceExt, 
				      FirstCluster,
				      BaseAddress + 
				      (i * DeviceExt->BytesPerCluster),
				      NCluster);
	  if (!NT_SUCCESS(Status))
	    {
	      if (!NoCache)
		{
		  CcRosReleaseCacheSegment(Fcb->RFCB.Bcb, CacheSeg, FALSE);
		}
	      else if (InternalOffset != 0)
		{
		  ExFreePool(BaseAddress);
		}
	      return(Status);
	    }
	  Status = NextCluster(DeviceExt, FirstCluster, &NCluster, Extend);
	  if (NCluster == 0xFFFFFFFF)
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
	  NextCluster(DeviceExt, FirstCluster, &NCluster, Extend);
	  if (NCluster == 0xFFFFFFFF)
	    {
	      break;
	    }
	}
    }
  
  if (!NoCache || InternalLength != PAGESIZE)
    {
      /*
       * Copy the caller's data if we are using the cache or writing
       * from temporary buffer
       */
      memcpy(BaseAddress + InternalOffset, Source, InternalLength);
    }
      
  /*
   * Write the data to the disk
   */
  NCluster = *CurrentCluster;
  
  for (i = 0; i < (PAGESIZE / DeviceExt->BytesPerCluster); i++)
    {	  
      Status = VfatRawWriteCluster(DeviceExt, 
				   FirstCluster,
				   BaseAddress + 
				   (i * DeviceExt->BytesPerCluster),
				   NCluster);
      if (!NT_SUCCESS(Status))
	{
	  if (!NoCache)
	    {
	      CcRosReleaseCacheSegment(Fcb->RFCB.Bcb, CacheSeg, FALSE);
	    }
	  else if (InternalOffset != 0)
	    {
	      ExFreePool(BaseAddress);
	    }
	  return(Status);
	}
      Status = NextCluster(DeviceExt, FirstCluster, &NCluster, Extend);
      if (NCluster == 0xFFFFFFFF)
	{
	  break;
	}
    }
  *CurrentCluster = NCluster;
  
  if (!NoCache)
    {
      CcRosReleaseCacheSegment(Fcb->RFCB.Bcb, CacheSeg, TRUE);
    }
  else if (InternalOffset != 0)
    {
      ExFreePool(BaseAddress);
    }
  return(STATUS_SUCCESS);
}

NTSTATUS
VfatWriteCluster(PDEVICE_EXTENSION DeviceExt,
		 PVFATFCB Fcb,
		 ULONG StartOffset,
		 ULONG FirstCluster,
		 PULONG CurrentCluster,
		 PVOID Source,
		 ULONG NoCache,
		 ULONG InternalOffset,
		 ULONG InternalLength,
		 BOOLEAN Extend)
{

  
  if (DeviceExt->BytesPerCluster >= PAGESIZE)
    {
      return(VfatWriteBigCluster(DeviceExt,
				 Fcb,
				 StartOffset,
				 FirstCluster,
				 CurrentCluster,
				 Source,
				 NoCache,
				 InternalOffset,
				 InternalLength,
				 Extend));
    }
  else
    {
      return(VfatWriteSmallCluster(DeviceExt,
				   Fcb,
				   StartOffset,
				   FirstCluster,
				   CurrentCluster,
				   Source,
				   NoCache,
				   InternalOffset,
				   InternalLength,
				   Extend));
    }
}

NTSTATUS
VfatWriteFile (PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
	       PVOID Buffer, ULONG Length, ULONG WriteOffset,
	       ULONG NoCache)
/*
 * FUNCTION: Writes data to file
 */
{
  ULONG CurrentCluster;
  ULONG FirstCluster;
  PVFATFCB Fcb;
  PVFATCCB pCcb;
  ULONG TempLength;
  LARGE_INTEGER SystemTime, LocalTime;
  ULONG ChunkSize;
  NTSTATUS Status;
  BOOLEAN Extend;

  DPRINT ("VfatWriteFile(FileObject %x, Buffer %x, Length %x, "
	   "WriteOffset %x\n", FileObject, Buffer, Length, WriteOffset);

  /* Locate the first cluster of the file */
  assert (FileObject);
  pCcb = (PVFATCCB) (FileObject->FsContext2);
  assert (pCcb);
  Fcb = pCcb->pFcb;
  assert (Fcb);

  if (DeviceExt->BytesPerCluster >= PAGESIZE)
    {
      ChunkSize = DeviceExt->BytesPerCluster;
    }
  else
    {
      ChunkSize = PAGESIZE;
    }

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
  
  /* Find the cluster according to the offset in the file */
  if (CurrentCluster == 0)
    {
      /*
       * File of size zero
       */
      Status = NextCluster (DeviceExt, FirstCluster, &CurrentCluster, 
			    TRUE);
      if (DeviceExt->FatType == FAT32)
	{
	  Fcb->entry.FirstClusterHigh = CurrentCluster >> 16;
	  Fcb->entry.FirstCluster = CurrentCluster;
	}
      else
	{
	  Fcb->entry.FirstCluster = CurrentCluster;
	}
      FirstCluster = CurrentCluster;
    }
  Status = OffsetToCluster(DeviceExt, FirstCluster, WriteOffset,
			   &CurrentCluster, TRUE);
  
  if (WriteOffset + Length > Fcb->entry.FileSize &&
      !(Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY))
    {
      Fcb->entry.FileSize = WriteOffset + Length;
    }

  /*
   * If the offset in the cluster doesn't fall on the cluster boundary 
   * then we have to write only from the specified offset
   */

  if ((WriteOffset % ChunkSize) != 0)
    {
      TempLength = min (Length, ChunkSize - (WriteOffset % ChunkSize));
      if ((Length - TempLength) > 0)
	{
	  Extend = TRUE;
	}
      else
	{
	  Extend = FALSE;
	}
      Status = VfatWriteCluster(DeviceExt,
				Fcb,
				ROUND_DOWN(WriteOffset, ChunkSize),
				FirstCluster,
				&CurrentCluster,
				Buffer,
				NoCache,
				WriteOffset % ChunkSize,
				TempLength,
				Extend);
      Buffer = Buffer + TempLength;
      Length = Length - TempLength;
      WriteOffset = WriteOffset + TempLength;
    }

  while (Length >= ChunkSize)
    {
      if ((Length - ChunkSize) > 0)
	{
	  Extend = TRUE;
	}
      else
	{
	  Extend = FALSE;
	}
      Status = VfatWriteCluster(DeviceExt,
				Fcb,
				ROUND_DOWN(WriteOffset, ChunkSize),
				FirstCluster,
				&CurrentCluster,
				Buffer,
				NoCache,
				0,
				ChunkSize,
				Extend);
      Buffer = Buffer + ChunkSize;
      Length = Length - ChunkSize;
      WriteOffset = WriteOffset + ChunkSize;
    }

  /* Write the remainder */
  if (Length > 0)
    {
      Status = VfatWriteCluster(DeviceExt,
				Fcb,
				ROUND_DOWN(WriteOffset, ChunkSize),
				FirstCluster,
				&CurrentCluster,
				Buffer,
				NoCache,
				0,
				Length,
				FALSE);
    }


  /* set dates and times */
  if (!(Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY))
    {
      KeQuerySystemTime (&SystemTime);
      ExSystemTimeToLocalTime (&SystemTime, &LocalTime);
      FsdFileTimeToDosDateTime ((TIME*)&LocalTime,
				&Fcb->entry.UpdateDate, 
				&Fcb->entry.UpdateTime);
      Fcb->entry.AccessDate = Fcb->entry.UpdateDate;
      updEntry (DeviceExt, FileObject);
    }

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
  ULONG NoCache;

  DPRINT ("VfatWrite(DeviceObject %x Irp %x)\n", DeviceObject, Irp);

  Length = Stack->Parameters.Write.Length;
  Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);
  Offset = Stack->Parameters.Write.ByteOffset.u.LowPart;

  if (Irp->Flags & IRP_PAGING_IO ||
      FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING)
    {
      NoCache = TRUE;
    }
  else
    {
      NoCache = FALSE;
    }

  Status = VfatWriteFile (DeviceExt, FileObject, Buffer, Length, Offset, 
			  NoCache); 

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
