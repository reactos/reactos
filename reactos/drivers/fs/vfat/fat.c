/*
 * $Id: fat.c,v 1.12 2001/01/13 18:38:09 dwelch Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/fat.c
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
Fat32GetNextCluster (PDEVICE_EXTENSION DeviceExt, 
		     ULONG CurrentCluster,
		     PULONG NextCluster)
/*
 * FUNCTION: Retrieve the next FAT32 cluster from the FAT table via a physical
 *           disk read
 */
{
  ULONG FATsector;
  ULONG FATeis;
  PULONG Block;
  NTSTATUS Status;

  Block = ExAllocatePool (NonPagedPool, 1024);
  FATsector = CurrentCluster / (512 / sizeof (ULONG));
  FATeis = CurrentCluster - (FATsector * (512 / sizeof (ULONG)));
  Status = VfatReadSectors (DeviceExt->StorageDevice,
			    (ULONG) (DeviceExt->FATStart + FATsector), 1,
			    (UCHAR *) Block);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  CurrentCluster = Block[FATeis];
  if (CurrentCluster >= 0xffffff8 && CurrentCluster <= 0xfffffff)
    CurrentCluster = 0xffffffff;
  ExFreePool (Block);
  *NextCluster = CurrentCluster;
  return (STATUS_SUCCESS);
}

NTSTATUS
Fat16GetNextCluster (PDEVICE_EXTENSION DeviceExt, 
		     ULONG CurrentCluster,
		     PULONG NextCluster)
/*
 * FUNCTION: Retrieve the next FAT16 cluster from the FAT table from the
 *           in-memory FAT
 */
{
  PVOID BaseAddress;
  BOOLEAN Valid;
  PCACHE_SEGMENT CacheSeg;
  NTSTATUS Status;
  ULONG FATOffset;
  
  FATOffset = (DeviceExt->FATStart * BLOCKSIZE) + (CurrentCluster * 2);
  
  Status = CcRequestCacheSegment(DeviceExt->StorageBcb,
				 PAGE_ROUND_DOWN(FATOffset),
				 &BaseAddress,
				 &Valid,
				 &CacheSeg);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  if (!Valid)
    {
      Status = VfatReadSectors(DeviceExt->StorageDevice,
			      PAGE_ROUND_DOWN(FATOffset) / BLOCKSIZE,
			      PAGESIZE / BLOCKSIZE,
			      BaseAddress);
      if (!NT_SUCCESS(Status))
	{
	  CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, FALSE);
	  return(Status);
	}
    }
  
  CurrentCluster = *((PUSHORT)(BaseAddress + (FATOffset % PAGESIZE)));
  if (CurrentCluster >= 0xfff8 && CurrentCluster <= 0xffff)
    CurrentCluster = 0xffffffff;
  CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, TRUE);
  *NextCluster = CurrentCluster;
  return (STATUS_SUCCESS);
}

NTSTATUS
Fat12GetNextCluster (PDEVICE_EXTENSION DeviceExt, 
		     ULONG CurrentCluster,
		     PULONG NextCluster)
/*
 * FUNCTION: Retrieve the next FAT12 cluster from the FAT table
 */
{
  ULONG FATOffset;
  ULONG Entry;
  NTSTATUS Status;
  PVOID BaseAddress;
  BOOLEAN Valid;
  PCACHE_SEGMENT CacheSeg;
  UCHAR Value1, Value2;

  FATOffset = (DeviceExt->FATStart * BLOCKSIZE) + ((CurrentCluster * 12) / 8);

  /*
   * Get the page containing this offset
   */
  Status = CcRequestCacheSegment(DeviceExt->StorageBcb,
				 PAGE_ROUND_DOWN(FATOffset),
				 &BaseAddress,
				 &Valid,
				 &CacheSeg);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  if (!Valid)
    {
      Status = VfatReadSectors(DeviceExt->StorageDevice,
			      PAGE_ROUND_DOWN(FATOffset) / BLOCKSIZE,
			      PAGESIZE / BLOCKSIZE,
			      BaseAddress);
      if (!NT_SUCCESS(Status))
	{
	  CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, FALSE);
	  return(Status);
	}
    }
  Value1 = ((PUCHAR)BaseAddress)[FATOffset % PAGESIZE];

  /*
   * A FAT12 entry may straggle two sectors 
   */
  if ((FATOffset + 1) == PAGESIZE)
    {
      CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, TRUE);
      Status = CcRequestCacheSegment(DeviceExt->StorageBcb,
				     PAGE_ROUND_DOWN(FATOffset) + PAGESIZE,
				     &BaseAddress,
				     &Valid,
				     &CacheSeg);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
      if (!Valid)
	{
	  ULONG NextOffset;

	  NextOffset = (PAGE_ROUND_DOWN(FATOffset) + PAGESIZE) / BLOCKSIZE;
	  Status = 
	    VfatReadSectors(DeviceExt->StorageDevice,
			    NextOffset,
			    PAGESIZE / BLOCKSIZE,
			    BaseAddress);
	  if (!NT_SUCCESS(Status))
	    {
	      CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, FALSE);
	      return(Status);
	    }
	}
      Value2 = ((PUCHAR)BaseAddress)[0];
      CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, TRUE);
    }
  else
    {
      Value2 = ((PUCHAR)BaseAddress)[(FATOffset % PAGESIZE) + 1];
      CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, TRUE);
    }

  if ((CurrentCluster % 2) == 0)
    {
      Entry = Value1;
      Entry |= ((Value2 & 0xf) << 8);
    }
  else
    {
      Entry = (Value1 >> 4);
      Entry |= (Value2 << 4);
    }
  DPRINT ("Entry %x\n", Entry);
  if (Entry >= 0xff8 && Entry <= 0xfff)
    Entry = 0xffffffff;
  DPRINT ("Returning %x\n", Entry);
  *NextCluster = Entry;
  return (STATUS_SUCCESS);
}

NTSTATUS
GetNextCluster (PDEVICE_EXTENSION DeviceExt, 
		ULONG CurrentCluster,
		PULONG NextCluster)
/*
 * FUNCTION: Retrieve the next cluster depending on the FAT type
 */
{
  NTSTATUS Status;

  DPRINT ("GetNextCluster(DeviceExt %x, CurrentCluster %x)\n",
	  DeviceExt, CurrentCluster);

  ExAcquireResourceSharedLite (&DeviceExt->FatResource, TRUE);

  if (DeviceExt->FatType == FAT16)
    {
      Status = Fat16GetNextCluster (DeviceExt, CurrentCluster, NextCluster);
    }
  else if (DeviceExt->FatType == FAT32)
    {
      Status = Fat32GetNextCluster (DeviceExt, CurrentCluster, NextCluster);
    }
  else
    {
      Status = Fat12GetNextCluster (DeviceExt, CurrentCluster, NextCluster);
    }

  ExReleaseResourceLite (&DeviceExt->FatResource);

  return (Status);
}

NTSTATUS
VfatRequestDiskPage(PDEVICE_EXTENSION DeviceExt,
		    ULONG Offset,
		    PVOID* BaseAddress,
		    PCACHE_SEGMENT* CacheSeg)
{
  NTSTATUS Status;
  BOOLEAN Valid;

  Status = CcRequestCacheSegment(DeviceExt->StorageBcb,
				 Offset,
				 BaseAddress,
				 &Valid,
				 CacheSeg);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  if (!Valid)
    {
      Status = VfatReadSectors(DeviceExt->StorageDevice,
			       Offset,
			       PAGESIZE / BLOCKSIZE,
			       *BaseAddress);
      if (!NT_SUCCESS(Status))
	{
	  CcReleaseCacheSegment(DeviceExt->StorageBcb, *CacheSeg, FALSE);
	  return(Status);
	}
    }
  return(STATUS_SUCCESS);
}

ULONG
Vfat16FindAvailableClusterInPage (PVOID Page, ULONG Offset, ULONG Length)
{
  ULONG j;

  for (j = Offset ; j < Length; j+=2)
    {
      if ((*((PUSHORT)(Page + j))) == 0)
	{
	  return(j);
	}
    }
  return(0);
}

ULONG
FAT16FindAvailableCluster (PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Finds the first available cluster in a FAT16 table
 */
{
  ULONG i;
  PCACHE_SEGMENT CacheSeg;
  PVOID BaseAddress;
  ULONG StartOffset;
  ULONG FatLength;
  ULONG Length;
  ULONG r;
  ULONG FatStart;
  NTSTATUS Status;

  FatStart = DeviceExt->FATStart * BLOCKSIZE;
  StartOffset = DeviceExt->FATStart * BLOCKSIZE;
  FatLength = DeviceExt->Boot->FATSectors * BLOCKSIZE;
  
  if ((StartOffset % PAGESIZE) != 0)
    {
      Status = VfatRequestDiskPage(DeviceExt,
				   PAGE_ROUND_DOWN(StartOffset),
				   &BaseAddress,
				   &CacheSeg);
      if (!NT_SUCCESS(Status))
	{
	  return(0);
	}
      Length = max(PAGESIZE, FatLength);
      r = Vfat16FindAvailableClusterInPage(BaseAddress,
					   StartOffset % PAGESIZE,
					   Length);
      if (r != 0)
	{
	  return((r - (StartOffset % PAGESIZE)) / 2);
	}
      CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, FALSE);
      StartOffset = StartOffset + (Length - (StartOffset % PAGESIZE));
      FatLength = FatLength - (Length - (StartOffset % PAGESIZE));
    }
  for (i = 0; i < (FatLength / PAGESIZE); i++)
    {
      Status = VfatRequestDiskPage(DeviceExt,
				   PAGE_ROUND_DOWN(StartOffset),
				   &BaseAddress,
				   &CacheSeg);
      if (!NT_SUCCESS(Status))
	{
	  return(0);
	}
      r = Vfat16FindAvailableClusterInPage(BaseAddress, 0, PAGESIZE);
      if (r != 0)
	{
	  return((r + StartOffset - FatStart) / 2);
	}
      CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, TRUE);
      StartOffset = StartOffset + PAGESIZE;
    }

  return(0);
}

#if 0
ULONG
FAT12FindAvailableCluster (PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Finds the first available cluster in a FAT12 table
 */
{
  ULONG FATOffset;
  ULONG Entry;
  PUCHAR CBlock = DeviceExt->FAT;
  ULONG i;
  for (i = 2; i < ((DeviceExt->Boot->FATSectors * 512 * 8) / 12); i++)
    {
      FATOffset = (i * 12) / 8;
      if ((i % 2) == 0)
	{
	  Entry = CBlock[FATOffset];
	  Entry |= ((CBlock[FATOffset + 1] & 0xf) << 8);
	}
      else
	{
	  Entry = (CBlock[FATOffset] >> 4);
	  Entry |= (CBlock[FATOffset + 1] << 4);
	}
      if (Entry == 0)
	return (i);
    }
  /* Give an error message (out of disk space) if we reach here) */
  DbgPrint ("Disk full, %d clusters used\n", i);
  return 0;
}

ULONG
FAT32FindAvailableCluster (PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Finds the first available cluster in a FAT32 table
 */
{
  ULONG sector;
  PULONG Block;
  int i;
  Block = ExAllocatePool (NonPagedPool, BLOCKSIZE);
  for (sector = 0;
       sector < ((struct _BootSector32 *) (DeviceExt->Boot))->FATSectors32;
       sector++)
    {
      /* FIXME: Check status */
      VfatReadSectors (DeviceExt->StorageDevice,
		       (ULONG) (DeviceExt->FATStart + sector), 1,
		       (UCHAR *) Block);

      for (i = 0; i < 512; i++)
	{
	  if (Block[i] == 0)
	    {
	      ExFreePool (Block);
	      return (i + sector * 128);
	    }
	}
    }
  /* Give an error message (out of disk space) if we reach here) */
  ExFreePool (Block);
  return 0;
}

ULONG
FAT12CountAvailableClusters (PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Counts free cluster in a FAT12 table
 */
{
  ULONG FATOffset;
  ULONG Entry;
  PUCHAR CBlock = DeviceExt->FAT;
  ULONG ulCount = 0;
  ULONG i;

  ExAcquireResourceSharedLite (&DeviceExt->FatResource, TRUE);

  for (i = 2; i < ((DeviceExt->Boot->FATSectors * 512 * 8) / 12); i++)
    {
      FATOffset = (i * 12) / 8;
      if ((i % 2) == 0)
	{
	  Entry = CBlock[FATOffset];
	  Entry |= ((CBlock[FATOffset + 1] & 0xf) << 8);
	}
      else
	{
	  Entry = (CBlock[FATOffset] >> 4);
	  Entry |= (CBlock[FATOffset + 1] << 4);
	}
      if (Entry == 0)
	ulCount++;
    }

  ExReleaseResourceLite (&DeviceExt->FatResource);

  return ulCount;
}

ULONG
FAT16CountAvailableClusters (PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Counts free clusters in a FAT16 table
 */
{
  PUSHORT Block;
  ULONG ulCount = 0;
  ULONG i;

  ExAcquireResourceSharedLite (&DeviceExt->FatResource, TRUE);

  Block = (PUSHORT) DeviceExt->FAT;
  for (i = 2; i < (DeviceExt->Boot->FATSectors * 256); i++)
    {
      if (Block[i] == 0)
	ulCount++;
    }

  ExReleaseResourceLite (&DeviceExt->FatResource);

  return ulCount;
}

ULONG
FAT32CountAvailableClusters (PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Counts free clusters in a FAT32 table
 */
{
  ULONG sector;
  PULONG Block;
  ULONG ulCount = 0;
  ULONG i;

  ExAcquireResourceSharedLite (&DeviceExt->FatResource, TRUE);

  Block = ExAllocatePool (NonPagedPool, BLOCKSIZE);
  for (sector = 0;
       sector < ((struct _BootSector32 *) (DeviceExt->Boot))->FATSectors32;
       sector++)
    {
      /* FIXME: Check status */
      VfatReadSectors (DeviceExt->StorageDevice,
		       (ULONG) (DeviceExt->FATStart + sector), 1,
		       (UCHAR *) Block);

      for (i = 0; i < 512; i++)
	{
	  if (Block[i] == 0)
	    ulCount++;
	}
    }
  /* Give an error message (out of disk space) if we reach here) */
  ExFreePool (Block);
  ExReleaseResourceLite (&DeviceExt->FatResource);
  return ulCount;
}

VOID
FAT12WriteCluster (PDEVICE_EXTENSION DeviceExt, ULONG ClusterToWrite,
		   ULONG NewValue)
/*
 * FUNCTION: Writes a cluster to the FAT12 physical and in-memory tables
 */
{
  ULONG FATsector;
  ULONG FATOffset;
  PUCHAR CBlock = DeviceExt->FAT;
  int i;
  FATOffset = (ClusterToWrite * 12) / 8;
  if ((ClusterToWrite % 2) == 0)
    {
      CBlock[FATOffset] = NewValue;
      CBlock[FATOffset + 1] &= 0xf0;
      CBlock[FATOffset + 1] |= (NewValue & 0xf00) >> 8;
    }
  else
    {
      CBlock[FATOffset] &= 0x0f;
      CBlock[FATOffset] |= (NewValue & 0xf) << 4;
      CBlock[FATOffset + 1] = NewValue >> 4;
    }
  /* Write the changed FAT sector(s) to disk */
  FATsector = FATOffset / BLOCKSIZE;
  for (i = 0; i < DeviceExt->Boot->FATCount; i++)
    {
      if ((FATOffset % BLOCKSIZE) == (BLOCKSIZE - 1))	//entry is on 2 sectors
	{
	  /* FIXME: Check status */
	  VfatWriteSectors (DeviceExt->StorageDevice,
			    DeviceExt->FATStart + FATsector
			    + i * DeviceExt->Boot->FATSectors,
			    2, CBlock + FATsector * 512);
	}
      else
	{
	  /* FIXME: Check status */
	  VfatWriteSectors (DeviceExt->StorageDevice,
			    DeviceExt->FATStart + FATsector
			    + i * DeviceExt->Boot->FATSectors,
			    1, CBlock + FATsector * 512);
	}
    }
}

VOID
FAT16WriteCluster (PDEVICE_EXTENSION DeviceExt, ULONG ClusterToWrite,
		   ULONG NewValue)
/*
 * FUNCTION: Writes a cluster to the FAT16 physical and in-memory tables
 */
{
  ULONG FATsector;
  PUSHORT Block;
  ULONG Start;
  int i;

  DbgPrint ("FAT16WriteCluster %u : %u\n", ClusterToWrite, NewValue);

  Block = (PUSHORT) DeviceExt->FAT;
  FATsector = ClusterToWrite / (512 / sizeof (USHORT));

  /* Update the in-memory FAT */
  Block[ClusterToWrite] = NewValue;

  /* Write the changed FAT sector to disk (all FAT's) */
  Start = DeviceExt->FATStart + FATsector;
  for (i = 0; i < DeviceExt->Boot->FATCount; i++)
    {
      /* FIXME: Check status */
      VfatWriteSectors (DeviceExt->StorageDevice,
			Start, 1, ((UCHAR *) Block) + FATsector * 512);
      Start += DeviceExt->Boot->FATSectors;
    }
}

VOID
FAT32WriteCluster (PDEVICE_EXTENSION DeviceExt, ULONG ClusterToWrite,
		   ULONG NewValue)
/*
 * FUNCTION: Writes a cluster to the FAT32 physical tables
 */
{
  ULONG FATsector;
  ULONG FATeis;
  PUSHORT Block;
  ULONG Start;
  int i;
  struct _BootSector32 *pBoot;
  DbgPrint ("FAT32WriteCluster %u : %u\n", ClusterToWrite, NewValue);
  Block = ExAllocatePool (NonPagedPool, BLOCKSIZE);
  FATsector = ClusterToWrite / 128;
  FATeis = ClusterToWrite - (FATsector * 128);
  /* load sector, change value, then rewrite sector */
  /* FIXME: Check status */
  VfatReadSectors (DeviceExt->StorageDevice,
		   DeviceExt->FATStart + FATsector, 1, (UCHAR *) Block);
  Block[FATeis] = NewValue;
  /* Write the changed FAT sector to disk (all FAT's) */
  Start = DeviceExt->FATStart + FATsector;
  pBoot = (struct _BootSector32 *) DeviceExt->Boot;
  for (i = 0; i < pBoot->FATCount; i++)
    {
      /* FIXME: Check status */
      VfatWriteSectors (DeviceExt->StorageDevice, Start, 1, (UCHAR *) Block);
      Start += pBoot->FATSectors;
    }
  ExFreePool (Block);
}

VOID
WriteCluster (PDEVICE_EXTENSION DeviceExt, ULONG ClusterToWrite,
	      ULONG NewValue)
/*
 * FUNCTION: Write a changed FAT entry
 */
{
  if (DeviceExt->FatType == FAT16)
    {
      FAT16WriteCluster (DeviceExt, ClusterToWrite, NewValue);
    }
  else if (DeviceExt->FatType == FAT32)
    {
      FAT32WriteCluster (DeviceExt, ClusterToWrite, NewValue);
    }
  else
    {
      FAT12WriteCluster (DeviceExt, ClusterToWrite, NewValue);
    }
}

ULONG
GetNextWriteCluster (PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster)
/*
 * FUNCTION: Determines the next cluster to be written
 */
{
  ULONG LastCluster, NewCluster;
  UCHAR *Buffer2;

  DPRINT ("GetNextWriteCluster(DeviceExt %x, CurrentCluster %x)\n",
	  DeviceExt, CurrentCluster);

  /* Find out what was happening in the last cluster's AU */
  LastCluster = GetNextCluster (DeviceExt, CurrentCluster);
  /* Check to see if we must append or overwrite */
  if (LastCluster == 0xffffffff)
    {
      /* we are after last existing cluster : we must add one to file */
      /* Append */
      /* Firstly, find the next available open allocation unit */
      if (DeviceExt->FatType == FAT16)
	{
	  NewCluster = FAT16FindAvailableCluster (DeviceExt);
	  DPRINT1 ("NewCluster %x\n", NewCluster);
	}
      else if (DeviceExt->FatType == FAT32)
	{
	  NewCluster = FAT32FindAvailableCluster (DeviceExt);
	}
      else
	{
	  NewCluster = FAT12FindAvailableCluster (DeviceExt);
	  DPRINT ("NewFat12Cluster: %x\n", NewCluster);
	}
      /* Mark the new AU as the EOF */
      WriteCluster (DeviceExt, NewCluster, 0xFFFFFFFF);
      /* Now, write the AU of the LastCluster with the value of the newly
         found AU */
      if (CurrentCluster)
	{
	  WriteCluster (DeviceExt, CurrentCluster, NewCluster);
	}
      // fill cluster with zero 
      Buffer2 = ExAllocatePool (NonPagedPool, DeviceExt->BytesPerCluster);
      memset (Buffer2, 0, DeviceExt->BytesPerCluster);
      VFATWriteCluster (DeviceExt, Buffer2, NewCluster);
      ExFreePool (Buffer2);
      /* Return NewCluster as CurrentCluster */
      return NewCluster;
    }
  else
    {
      /* Overwrite: Return LastCluster as CurrentCluster */
      return LastCluster;
    }
}
#endif

ULONG
ClusterToSector (PDEVICE_EXTENSION DeviceExt, unsigned long Cluster)
/*
 * FUNCTION: Converts the cluster number to a sector number for this physical
 *           device
 */
{
  return DeviceExt->dataStart +
    ((Cluster - 2) * DeviceExt->Boot->SectorsPerCluster);
}

VOID
VFATLoadCluster (PDEVICE_EXTENSION DeviceExt, PVOID Buffer, ULONG Cluster)
/*
 * FUNCTION: Load a cluster from the physical device
 */
{
  ULONG Sector;

  DPRINT ("VFATLoadCluster(DeviceExt %x, Buffer %x, Cluster %d)\n",
	  DeviceExt, Buffer, Cluster);

  Sector = ClusterToSector (DeviceExt, Cluster);

  /* FIXME: Check status */
  VfatReadSectors (DeviceExt->StorageDevice,
		   Sector, DeviceExt->Boot->SectorsPerCluster, Buffer);
  DPRINT ("Finished VFATReadSectors\n");
}

NTSTATUS
VfatWriteCluster (PDEVICE_EXTENSION DeviceExt, PVOID Buffer, ULONG Cluster)
/*
 * FUNCTION: Write a cluster to the physical device
 */
{
  ULONG Sector;
  NTSTATUS Status;

  DPRINT ("VfatWriteCluster(DeviceExt %x, Buffer %x, Cluster %d)\n",
	  DeviceExt, Buffer, Cluster);

  Sector = ClusterToSector (DeviceExt, Cluster);

  /* FIXME: Check status */
  Status = VfatWriteSectors (DeviceExt->StorageDevice,
			     Sector, DeviceExt->Boot->SectorsPerCluster, 
			     Buffer);
  return(Status);
}

