/*
 * $Id: fat.c,v 1.9 2000/12/29 23:17:12 dwelch Exp $
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

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

ULONG
Fat32GetNextCluster (PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster)
/*
 * FUNCTION: Retrieve the next FAT32 cluster from the FAT table via a physical
 *           disk read
 */
{
  ULONG FATsector;
  ULONG FATeis;
  PULONG Block;

  Block = ExAllocatePool (NonPagedPool, 1024);
  FATsector = CurrentCluster / (512 / sizeof (ULONG));
  FATeis = CurrentCluster - (FATsector * (512 / sizeof (ULONG)));
  VFATReadSectors (DeviceExt->StorageDevice,
		   (ULONG) (DeviceExt->FATStart + FATsector), 1,
		   (UCHAR *) Block);
  CurrentCluster = Block[FATeis];
  if (CurrentCluster >= 0xffffff8 && CurrentCluster <= 0xfffffff)
    CurrentCluster = 0xffffffff;
  ExFreePool (Block);
  return (CurrentCluster);
}

ULONG
Fat16GetNextCluster (PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster)
/*
 * FUNCTION: Retrieve the next FAT16 cluster from the FAT table from the
 *           in-memory FAT
 */
{
  PUSHORT Block;
  Block = (PUSHORT) DeviceExt->FAT;
  CurrentCluster = Block[CurrentCluster];
  if (CurrentCluster >= 0xfff8 && CurrentCluster <= 0xffff)
    CurrentCluster = 0xffffffff;
  DPRINT ("Returning %x\n", CurrentCluster);
  return (CurrentCluster);
}

ULONG
Fat12GetNextCluster (PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster)
/*
 * FUNCTION: Retrieve the next FAT12 cluster from the FAT table from the
 *           in-memory FAT
 */
{
  unsigned char *CBlock;
  ULONG FATOffset;
  ULONG Entry;
  CBlock = DeviceExt->FAT;
  FATOffset = (CurrentCluster * 12) / 8;	//first byte containing value
  if ((CurrentCluster % 2) == 0)
    {
      Entry = CBlock[FATOffset];
      Entry |= ((CBlock[FATOffset + 1] & 0xf) << 8);
    }
  else
    {
      Entry = (CBlock[FATOffset] >> 4);
      Entry |= (CBlock[FATOffset + 1] << 4);
    }
  DPRINT ("Entry %x\n", Entry);
  if (Entry >= 0xff8 && Entry <= 0xfff)
    Entry = 0xffffffff;
  DPRINT ("Returning %x\n", Entry);
  return (Entry);
}

ULONG
GetNextCluster (PDEVICE_EXTENSION DeviceExt, ULONG CurrentCluster)
/*
 * FUNCTION: Retrieve the next cluster depending on the FAT type
 */
{
  ULONG NextCluster;

  DPRINT ("GetNextCluster(DeviceExt %x, CurrentCluster %x)\n",
	  DeviceExt, CurrentCluster);

  ExAcquireResourceSharedLite (&DeviceExt->FatResource, TRUE);

  if (DeviceExt->FatType == FAT16)
    {
      NextCluster = Fat16GetNextCluster (DeviceExt, CurrentCluster);
    }
  else if (DeviceExt->FatType == FAT32)
    {
      NextCluster = Fat32GetNextCluster (DeviceExt, CurrentCluster);
    }
  else
    {
      NextCluster = Fat12GetNextCluster (DeviceExt, CurrentCluster);
    }

  ExReleaseResourceLite (&DeviceExt->FatResource);

  return (NextCluster);
}

ULONG
FAT16FindAvailableCluster (PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Finds the first available cluster in a FAT16 table
 */
{
  PUSHORT Block;
  int i;
  Block = (PUSHORT) DeviceExt->FAT;
  for (i = 2; i < (DeviceExt->Boot->FATSectors * 256); i++)
    if (Block[i] == 0)
      return (i);
  /* Give an error message (out of disk space) if we reach here) */
  return 0;
}

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
      VFATReadSectors (DeviceExt->StorageDevice,
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
      VFATReadSectors (DeviceExt->StorageDevice,
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
	  VFATWriteSectors (DeviceExt->StorageDevice,
			    DeviceExt->FATStart + FATsector
			    + i * DeviceExt->Boot->FATSectors,
			    2, CBlock + FATsector * 512);
	}
      else
	{
	  VFATWriteSectors (DeviceExt->StorageDevice,
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
      VFATWriteSectors (DeviceExt->StorageDevice,
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
  VFATReadSectors (DeviceExt->StorageDevice,
		   DeviceExt->FATStart + FATsector, 1, (UCHAR *) Block);
  Block[FATeis] = NewValue;
  /* Write the changed FAT sector to disk (all FAT's) */
  Start = DeviceExt->FATStart + FATsector;
  pBoot = (struct _BootSector32 *) DeviceExt->Boot;
  for (i = 0; i < pBoot->FATCount; i++)
    {
      VFATWriteSectors (DeviceExt->StorageDevice, Start, 1, (UCHAR *) Block);
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

  VFATReadSectors (DeviceExt->StorageDevice,
		   Sector, DeviceExt->Boot->SectorsPerCluster, Buffer);
  DPRINT ("Finished VFATReadSectors\n");
}

VOID
VFATWriteCluster (PDEVICE_EXTENSION DeviceExt, PVOID Buffer, ULONG Cluster)
/*
 * FUNCTION: Write a cluster to the physical device
 */
{
  ULONG Sector;
  DPRINT ("VFATWriteCluster(DeviceExt %x, Buffer %x, Cluster %d)\n",
	  DeviceExt, Buffer, Cluster);

  Sector = ClusterToSector (DeviceExt, Cluster);

  VFATWriteSectors (DeviceExt->StorageDevice,
		    Sector, DeviceExt->Boot->SectorsPerCluster, Buffer);
}
