/*
 * $Id: fat.c,v 1.14 2001/01/16 09:55:02 dwelch Exp $
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
 unsigned char* CBlock;
 ULONG FATOffset;
 ULONG Entry;
 BOOLEAN Valid;
 PCACHE_SEGMENT CacheSeg;
 NTSTATUS Status;
 PVOID BaseAddress;

 *NextCluster = 0;

 Status = CcRequestCacheSegment(DeviceExt->Fat12StorageBcb,
				0,
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
			       DeviceExt->FATStart,
			       DeviceExt->Boot->FATSectors,
			       BaseAddress);
      if (!NT_SUCCESS(Status))
	{
	  CcReleaseCacheSegment(DeviceExt->Fat12StorageBcb, CacheSeg, FALSE);
	  return(Status);
	}
    }
  CBlock = (PUCHAR)BaseAddress;
  
  FATOffset = (CurrentCluster * 12) / 8; /* first byte containing value */
  if ((CurrentCluster % 2) == 0)
    {
      Entry = CBlock[FATOffset];
      Entry |= ((CBlock[FATOffset+1] & 0xf)<<8);
    }
  else
    {
      Entry = (CBlock[FATOffset] >> 4);
      Entry |= (CBlock[FATOffset+1] << 4);
    }
  DPRINT("Entry %x\n",Entry);
  if (Entry >= 0xff8 && Entry <= 0xfff)
    Entry = 0xffffffff;
  DPRINT("Returning %x\n",Entry);
  *NextCluster = Entry;
  CcReleaseCacheSegment(DeviceExt->Fat12StorageBcb, CacheSeg, TRUE);
  return(STATUS_SUCCESS);
}

NTSTATUS
FAT16FindAvailableCluster (PDEVICE_EXTENSION DeviceExt,
			   PULONG Cluster)
/*
 * FUNCTION: Finds the first available cluster in a FAT16 table
 */
{
  ULONG FatLength;
  ULONG i;
  NTSTATUS Status;
  PVOID BaseAddress;
  PCACHE_SEGMENT CacheSeg;
  BOOLEAN Valid;  
  ULONG FatStart;
  
  FatStart = DeviceExt->FATStart * BLOCKSIZE;
  FatLength = DeviceExt->Boot->FATSectors * BLOCKSIZE;
  CacheSeg = NULL;
  *Cluster = 0;

  for (i = 2; i < FatLength; i+=2)
    {            
      if (((FatStart + i) % PAGESIZE) == 0 || CacheSeg == NULL)
	{
	  if (CacheSeg != NULL)
	    {
	      CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, TRUE);
	    }
	  Status = CcRequestCacheSegment(DeviceExt->StorageBcb,
					 PAGE_ROUND_DOWN(FatStart + i),
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
				       PAGE_ROUND_DOWN(FatStart + i) 
				       / BLOCKSIZE,
				       PAGESIZE / BLOCKSIZE,
				       BaseAddress);
	      if (!NT_SUCCESS(Status))
		{
		  CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, 
					FALSE);
		  return(Status);
		}
	    }
	}
      if (*((PUSHORT)(BaseAddress + ((FatStart + i) % PAGESIZE))) == 0)
	{
	  DPRINT1("Found available cluster 0x%x\n", i);
	  *Cluster = i / 2;
	  CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, TRUE);
	  return(STATUS_SUCCESS);
	}
    }
  CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, TRUE);
  return(STATUS_DISK_FULL);
}

NTSTATUS
FAT12FindAvailableCluster (PDEVICE_EXTENSION DeviceExt, PULONG Cluster)
/*
 * FUNCTION: Finds the first available cluster in a FAT12 table
 */
{
  ULONG FATOffset;
  ULONG Entry;
  PUCHAR CBlock;
  ULONG i;
  PVOID BaseAddress;
  BOOLEAN Valid;
  PCACHE_SEGMENT CacheSeg;
  NTSTATUS Status;
  
  *Cluster = 0;

  Status = CcRequestCacheSegment(DeviceExt->Fat12StorageBcb,
				 0,
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
			       DeviceExt->FATStart,
			       DeviceExt->Boot->FATSectors,
			       BaseAddress);
      if (!NT_SUCCESS(Status))
	{
	  CcReleaseCacheSegment(DeviceExt->Fat12StorageBcb, CacheSeg, FALSE);
	  return(Status);
	}
    }
  CBlock = (PUCHAR)BaseAddress;

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
	{
	  DPRINT1("Found available cluster 0x%x\n", i);
	  *Cluster = i;
	  CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, TRUE);
	}
    }
  CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, TRUE);
  return (STATUS_DISK_FULL);
}

NTSTATUS
FAT32FindAvailableCluster (PDEVICE_EXTENSION DeviceExt, PULONG Cluster)
/*
 * FUNCTION: Finds the first available cluster in a FAT32 table
 */
{
  ULONG sector;
  PULONG Block;
  int i;

  Block = ExAllocatePool (NonPagedPool, BLOCKSIZE);
  *Cluster = 0;

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
	      *Cluster = (i + sector * 128);
	      return(STATUS_SUCCESS);
	    }
	}
    }
  /* Give an error message (out of disk space) if we reach here) */
  ExFreePool (Block);
  return (STATUS_DISK_FULL);
}

#if 0
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
#endif

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

NTSTATUS
FAT12WriteCluster (PDEVICE_EXTENSION DeviceExt, ULONG ClusterToWrite,
		   ULONG NewValue)
/*
 * FUNCTION: Writes a cluster to the FAT12 physical and in-memory tables
 */
{
  ULONG FATsector;
  ULONG FATOffset;
  PUCHAR CBlock;
  int i;
  NTSTATUS Status;
  PVOID BaseAddress;
  BOOLEAN Valid;
  PCACHE_SEGMENT CacheSeg;

  Status = CcRequestCacheSegment(DeviceExt->Fat12StorageBcb,
				 0,
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
			       DeviceExt->FATStart,
			       DeviceExt->Boot->FATSectors,
			       BaseAddress);
      if (!NT_SUCCESS(Status))
	{
	  CcReleaseCacheSegment(DeviceExt->Fat12StorageBcb, CacheSeg, FALSE);
	  return(Status);
	}
    }
  CBlock = (PUCHAR)BaseAddress;

  FATOffset = (ClusterToWrite * 12) / 8;
  DPRINT1("Writing 0x%x for 0x%x at 0x%x\n",
	  NewValue, ClusterToWrite, FATOffset);
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
  CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, TRUE);
  return(STATUS_SUCCESS);
}

NTSTATUS
FAT16WriteCluster (PDEVICE_EXTENSION DeviceExt, ULONG ClusterToWrite,
		   ULONG NewValue)
/*
 * FUNCTION: Writes a cluster to the FAT16 physical and in-memory tables
 */
{
  PVOID BaseAddress;
  BOOLEAN Valid;
  PCACHE_SEGMENT CacheSeg;
  NTSTATUS Status;
  ULONG FATOffset;
  ULONG Start;
  ULONG i;
  
  Start = DeviceExt->FATStart;

  FATOffset = (Start * BLOCKSIZE) + (ClusterToWrite * 2);

  for (i = 0; i < DeviceExt->Boot->FATCount; i++)
    {  
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
      
      DPRINT1("Writing 0x%x for offset 0x%x 0x%x\n", NewValue, FATOffset,
	     ClusterToWrite);
      *((PUSHORT)(BaseAddress + (FATOffset % PAGESIZE))) = NewValue;
      Status = VfatWriteSectors(DeviceExt->StorageDevice,
				PAGE_ROUND_DOWN(FATOffset) / BLOCKSIZE,
				PAGESIZE / BLOCKSIZE,
				BaseAddress);
      CcReleaseCacheSegment(DeviceExt->StorageBcb, CacheSeg, TRUE);
      
      DPRINT1("DeviceExt->Boot->FATSectors %d\n",
	      DeviceExt->Boot->FATSectors);
      FATOffset = FATOffset + DeviceExt->Boot->FATSectors * BLOCKSIZE;
    }

  return (STATUS_SUCCESS);
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

NTSTATUS
WriteCluster (PDEVICE_EXTENSION DeviceExt, ULONG ClusterToWrite,
	      ULONG NewValue)
/*
 * FUNCTION: Write a changed FAT entry
 */
{
  NTSTATUS Status;

  if (DeviceExt->FatType == FAT16)
    {
      Status = FAT16WriteCluster (DeviceExt, ClusterToWrite, NewValue);
    }
  else if (DeviceExt->FatType == FAT32)
    {
      FAT32WriteCluster (DeviceExt, ClusterToWrite, NewValue);
      Status = STATUS_SUCCESS;
    }
  else
    {
      Status = FAT12WriteCluster (DeviceExt, ClusterToWrite, NewValue);
    }
  return(Status);
}

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

NTSTATUS
VfatRawReadCluster (PDEVICE_EXTENSION DeviceExt, 
		    ULONG FirstCluster,
		    PVOID Buffer, 
		    ULONG Cluster)
/*
 * FUNCTION: Load a cluster from the physical device
 */
{
  NTSTATUS Status;

  if (FirstCluster == 1)
    {
      Status = VfatReadSectors (DeviceExt->StorageDevice,
				Cluster,
				DeviceExt->Boot->SectorsPerCluster, 
				Buffer);
      return(Status);
    }
  else
    {
      ULONG Sector;
      
      Sector = ClusterToSector (DeviceExt, Cluster);
      

      Status = VfatReadSectors (DeviceExt->StorageDevice,
				Sector, DeviceExt->Boot->SectorsPerCluster, 
				Buffer);
      return(Status);
    }
}

NTSTATUS
VfatRawWriteCluster (PDEVICE_EXTENSION DeviceExt, 
		     ULONG FirstCluster,
		     PVOID Buffer, 
		     ULONG Cluster)
/*
 * FUNCTION: Write a cluster to the physical device
 */
{
  ULONG Sector;
  NTSTATUS Status;

  DPRINT ("VfatWriteCluster(DeviceExt %x, Buffer %x, Cluster %d)\n",
	  DeviceExt, Buffer, Cluster);

  if (FirstCluster == 1)
    {
      Status = VfatWriteSectors (DeviceExt->StorageDevice,
			        Cluster,
				DeviceExt->Boot->SectorsPerCluster, Buffer);
    }
  else
    {
      Sector = ClusterToSector (DeviceExt, Cluster);
      
      Status = VfatWriteSectors (DeviceExt->StorageDevice,
				 Sector, DeviceExt->Boot->SectorsPerCluster, 
				 Buffer);
    }
  return(Status);
}

NTSTATUS
GetNextCluster (PDEVICE_EXTENSION DeviceExt, 
		ULONG CurrentCluster,
		PULONG NextCluster,
		BOOLEAN Extend)
/*
 * FUNCTION: Retrieve the next cluster depending on the FAT type
 */
{
  NTSTATUS Status;

  DPRINT ("GetNextCluster(DeviceExt %x, CurrentCluster %x)\n",
	  DeviceExt, CurrentCluster);
  
  if (Extend)
    {
      ExAcquireResourceSharedLite (&DeviceExt->FatResource, TRUE);
    }
  else
    {
      ExAcquireResourceExclusiveLite (&DeviceExt->FatResource, TRUE);
    }
  
  /*
   * If the file hasn't any clusters allocated then we need special
   * handling
   */
  if (CurrentCluster == 0 && Extend)
    {
      ULONG NewCluster;

      if (DeviceExt->FatType == FAT16)
	{
	  Status = FAT16FindAvailableCluster (DeviceExt, &NewCluster);
	  if (!NT_SUCCESS(Status))
	    {
	      return(Status);
	    }
	}
      else if (DeviceExt->FatType == FAT32)
	{
	  Status = FAT32FindAvailableCluster (DeviceExt, &NewCluster);
	}
      else
	{
	  Status = FAT12FindAvailableCluster (DeviceExt, &NewCluster);
	  if (!NT_SUCCESS(Status))
	    {
	      return(Status);
	    }
	}
      /* Mark the new AU as the EOF */
      WriteCluster (DeviceExt, NewCluster, 0xFFFFFFFF);
      *NextCluster = NewCluster;
      ExReleaseResourceLite(&DeviceExt->FatResource);
      return(STATUS_SUCCESS);
    }
  else if (CurrentCluster == 0)
    {
      ExReleaseResourceLite(&DeviceExt->FatResource);
      return(STATUS_UNSUCCESSFUL);
    }

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
  if (Extend && (*NextCluster) == 0xFFFFFFFF)
    {
      ULONG NewCluster;

      /* We are after last existing cluster, we must add one to file */
      /* Firstly, find the next available open allocation unit */
      if (DeviceExt->FatType == FAT16)
	{
	  Status = FAT16FindAvailableCluster (DeviceExt, &NewCluster);
	  if (!NT_SUCCESS(Status))
	    {
	      return(Status);
	    }
	}
      else if (DeviceExt->FatType == FAT32)
	{
	  Status = FAT32FindAvailableCluster (DeviceExt, &NewCluster);
	}
      else
	{
	  Status = FAT12FindAvailableCluster (DeviceExt, &NewCluster);
	  if (!NT_SUCCESS(Status))
	    {
	      return(Status);
	    }
	}
      /* Mark the new AU as the EOF */
      WriteCluster (DeviceExt, NewCluster, 0xFFFFFFFF);
      /* Now, write the AU of the LastCluster with the value of the newly
         found AU */
      WriteCluster (DeviceExt, CurrentCluster, NewCluster);
      *NextCluster = NewCluster;
    }

  ExReleaseResourceLite (&DeviceExt->FatResource);

  return (Status);
}

