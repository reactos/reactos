/*
 * $Id: fat.c,v 1.33 2001/10/10 22:16:46 hbirr Exp $
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

/* GLOBALS ******************************************************************/

#define ROUND_DOWN(N, S) ((N) - ((N) % (S)))

#define  CACHEPAGESIZE(pDeviceExt) ((pDeviceExt)->BytesPerCluster > PAGESIZE ? \
		   (pDeviceExt)->BytesPerCluster : PAGESIZE)

/* FUNCTIONS ****************************************************************/

NTSTATUS
Fat32GetNextCluster(PDEVICE_EXTENSION DeviceExt,
		    ULONG CurrentCluster,
		    PULONG NextCluster)
/*
 * FUNCTION: Retrieve the next FAT32 cluster from the FAT table via a physical
 *           disk read
 */
{
  PVOID BaseAddress;
  NTSTATUS Status;
  ULONG FATOffset;
  ULONG ChunkSize;
  PVOID Context;
  LARGE_INTEGER Offset;

  ChunkSize = CACHEPAGESIZE(DeviceExt);
  FATOffset = CurrentCluster * sizeof(ULONG);
  Offset.QuadPart = ROUND_DOWN(FATOffset, ChunkSize);
  if(!CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
  {
    return STATUS_UNSUCCESSFUL;
  }
  CurrentCluster = (*(PULONG)(BaseAddress + (FATOffset % ChunkSize))) & 0x0fffffff;
  if (CurrentCluster >= 0xffffff8 && CurrentCluster <= 0xfffffff)
    CurrentCluster = 0xffffffff;
  CcUnpinData(Context);
  *NextCluster = CurrentCluster;
  return (STATUS_SUCCESS);
}

NTSTATUS
Fat16GetNextCluster(PDEVICE_EXTENSION DeviceExt,
		    ULONG CurrentCluster,
		    PULONG NextCluster)
/*
 * FUNCTION: Retrieve the next FAT16 cluster from the FAT table
 */
{
  PVOID BaseAddress;
  NTSTATUS Status;
  ULONG FATOffset;
  ULONG ChunkSize;
  PVOID Context;
  LARGE_INTEGER Offset;

  ChunkSize = CACHEPAGESIZE(DeviceExt);
  FATOffset = CurrentCluster * 2;
  Offset.QuadPart = ROUND_DOWN(FATOffset, ChunkSize);
  if(!CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
  {
    return STATUS_UNSUCCESSFUL;
  }
  CurrentCluster = *((PUSHORT)(BaseAddress + (FATOffset % ChunkSize)));
  if (CurrentCluster >= 0xfff8 && CurrentCluster <= 0xffff)
    CurrentCluster = 0xffffffff;
  CcUnpinData(Context);
  *NextCluster = CurrentCluster;
  return (STATUS_SUCCESS);
}

NTSTATUS
Fat12GetNextCluster(PDEVICE_EXTENSION DeviceExt,
		    ULONG CurrentCluster,
		    PULONG NextCluster)
/*
 * FUNCTION: Retrieve the next FAT12 cluster from the FAT table
 */
{
  PUCHAR CBlock;
  ULONG FATOffset;
  ULONG Entry;
  NTSTATUS Status;
  PVOID BaseAddress;
  PVOID Context;
  LARGE_INTEGER Offset;


  *NextCluster = 0;

  Offset.QuadPart = 0;
  if(!CcMapData(DeviceExt->FATFileObject, &Offset, DeviceExt->Boot->FATSectors * BLOCKSIZE, 1, &Context, &BaseAddress))
  {
    return STATUS_UNSUCCESSFUL;
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
//  DPRINT("Entry %x\n",Entry);
  if (Entry >= 0xff8 && Entry <= 0xfff)
    Entry = 0xffffffff;
//  DPRINT("Returning %x\n",Entry);
  *NextCluster = Entry;
  CcUnpinData(Context);
//  return Entry == 0xffffffff ? STATUS_END_OF_FILE : STATUS_SUCCESS;
  return STATUS_SUCCESS;
}

NTSTATUS
FAT16FindAvailableCluster(PDEVICE_EXTENSION DeviceExt,
			  PULONG Cluster)
/*
 * FUNCTION: Finds the first available cluster in a FAT16 table
 */
{
  ULONG FatLength;
  ULONG i;
  NTSTATUS Status;
  PVOID BaseAddress;
  ULONG ChunkSize;
  PVOID Context = 0;
  LARGE_INTEGER Offset;

  ChunkSize = CACHEPAGESIZE(DeviceExt);
  FatLength = (((DeviceExt->Boot->Sectors ? DeviceExt->Boot->Sectors : DeviceExt->Boot->SectorsHuge)-DeviceExt->dataStart)/DeviceExt->Boot->SectorsPerCluster+2)*2;
  *Cluster = 0;

  for (i = 2; i < FatLength; i+=2)
  {
      if ((i % ChunkSize) == 0 || Context ==0)
	{
      Offset.QuadPart = ROUND_DOWN(i, ChunkSize);
      if (Context != NULL)
      {
        CcUnpinData(Context);
      }
      if(!CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
      {
        return STATUS_UNSUCCESSFUL;
      }
	}

    if (*((PUSHORT)(BaseAddress + (i % ChunkSize))) == 0)
	{
	  DPRINT("Found available cluster 0x%x\n", i / 2);
	  *Cluster = i / 2;
      CcUnpinData(Context);
	  return(STATUS_SUCCESS);
	}

  }
  CcUnpinData(Context);
  return(STATUS_DISK_FULL);
}

NTSTATUS
FAT12FindAvailableCluster(PDEVICE_EXTENSION DeviceExt, PULONG Cluster)
/*
 * FUNCTION: Finds the first available cluster in a FAT12 table
 */
{
  ULONG FATOffset;
  ULONG Entry;
  PUCHAR CBlock;
  ULONG i;
  PVOID BaseAddress;
  NTSTATUS Status;
  ULONG numberofclusters;
  PVOID Context;
  LARGE_INTEGER Offset;

  *Cluster = 0;
  Offset.QuadPart = 0;
  if(!CcMapData(DeviceExt->FATFileObject, &Offset, DeviceExt->Boot->FATSectors * BLOCKSIZE, 1, &Context, &BaseAddress))
  {
    return STATUS_UNSUCCESSFUL;
  }
  CBlock = (PUCHAR)BaseAddress;

  numberofclusters = ((DeviceExt->Boot->Sectors ? DeviceExt->Boot->Sectors : DeviceExt->Boot->SectorsHuge)-DeviceExt->dataStart)/DeviceExt->Boot->SectorsPerCluster+2;

  for (i = 2; i < numberofclusters; i++)
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
	  DPRINT("Found available cluster 0x%x\n", i);
	  *Cluster = i;
      CcUnpinData(Context);
	  return(STATUS_SUCCESS);
	}
    }
  CcUnpinData(Context);
  return (STATUS_DISK_FULL);
}

NTSTATUS
FAT32FindAvailableCluster (PDEVICE_EXTENSION DeviceExt, PULONG Cluster)
/*
 * FUNCTION: Finds the first available cluster in a FAT32 table
 */
{
  ULONG FatLength;
  ULONG i;
  NTSTATUS Status;
  PVOID BaseAddress;
  ULONG ChunkSize;
  PVOID Context = NULL;
  LARGE_INTEGER Offset;

  ChunkSize = CACHEPAGESIZE(DeviceExt);
  FatLength = (((DeviceExt->Boot->Sectors ? DeviceExt->Boot->Sectors : DeviceExt->Boot->SectorsHuge)-DeviceExt->dataStart)/DeviceExt->Boot->SectorsPerCluster+2)*4;

  *Cluster = 0;

  for (i = 4; i < FatLength; i+=4)
  {
    if ((i % ChunkSize) == 0 || Context == NULL)
	{
      Offset.QuadPart = ROUND_DOWN(i, ChunkSize);
      if (Context != NULL)
      {
        CcUnpinData(Context);
      }
      if(!CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
      {
        return STATUS_UNSUCCESSFUL;
      }
	}
    if ((*((PULONG)(BaseAddress + (/*(FatStart +*/ i/*)*/ % ChunkSize))) & 0x0fffffff) == 0)
	{
	  DPRINT("Found available cluster 0x%x\n", i / 4);
	  *Cluster = i / 4;
      CcUnpinData(Context);
	  return(STATUS_SUCCESS);
	}
  }
  CcUnpinData(Context);
  return (STATUS_DISK_FULL);
}


NTSTATUS
FAT12CountAvailableClusters(PDEVICE_EXTENSION DeviceExt,
			    PLARGE_INTEGER Clusters)
/*
 * FUNCTION: Counts free cluster in a FAT12 table
 */
{
  ULONG FATOffset;
  ULONG Entry;
  PUCHAR CBlock;
  ULONG ulCount = 0;
  ULONG i;
  PVOID BaseAddress;
  NTSTATUS Status;
  ULONG numberofclusters;
  LARGE_INTEGER Offset;
  PVOID Context;

  ExAcquireResourceSharedLite (&DeviceExt->FatResource, TRUE);
  Offset.QuadPart = 0;
  if(!CcMapData(DeviceExt->FATFileObject, &Offset, DeviceExt->Boot->FATSectors * BLOCKSIZE, 1, &Context, &BaseAddress))
  {
    ExReleaseResourceLite (&DeviceExt->FatResource);
    return STATUS_UNSUCCESSFUL;
  }
  CBlock = (PUCHAR)BaseAddress;
  numberofclusters = ((DeviceExt->Boot->Sectors ? DeviceExt->Boot->Sectors : DeviceExt->Boot->SectorsHuge)-DeviceExt->dataStart)/DeviceExt->Boot->SectorsPerCluster+2;

  for (i = 2; i < numberofclusters; i++)
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

  CcUnpinData(Context);
  ExReleaseResourceLite (&DeviceExt->FatResource);

  Clusters->QuadPart = ulCount;

  return(STATUS_SUCCESS);
}


NTSTATUS
FAT16CountAvailableClusters(PDEVICE_EXTENSION DeviceExt,
			    PLARGE_INTEGER Clusters)
/*
 * FUNCTION: Counts free clusters in a FAT16 table
 */
{
  PUSHORT Block;
  ULONG ulCount = 0;
  ULONG i;
  ULONG numberofclusters;
  ULONG numberofsectors;
  ULONG sector;
  ULONG forto;
  NTSTATUS Status;

  ExAcquireResourceSharedLite (&DeviceExt->FatResource, TRUE);
  Block = ExAllocatePool (NonPagedPool, BLOCKSIZE);

  numberofclusters = ((DeviceExt->Boot->Sectors ? DeviceExt->Boot->Sectors : DeviceExt->Boot->SectorsHuge)-DeviceExt->dataStart)/DeviceExt->Boot->SectorsPerCluster+2;
  numberofsectors = (numberofclusters + 255) / 256;
  numberofclusters %= 256;

  for (sector = 0; sector < numberofsectors; sector++)
    {
      Status = VfatReadSectors(DeviceExt->StorageDevice,
			       DeviceExt->FATStart + sector,
			       1,
			       (PUCHAR)Block);
      if (!NT_SUCCESS(Status))
	{
	   ExFreePool(Block);
	   ExReleaseResourceLite(&DeviceExt->FatResource);
	   return(Status);
	}

      if (sector == numberofsectors - 1)
	forto = numberofclusters;
      else
	forto = 256;

      for (i = 0; i < forto; i++)
	{
	  if (Block[i] == 0)
	    ulCount++;
	}
    }
  ExReleaseResourceLite (&DeviceExt->FatResource);

  Clusters->QuadPart = ulCount;
  ExFreePool(Block);

  return(STATUS_SUCCESS);
}


NTSTATUS
FAT32CountAvailableClusters(PDEVICE_EXTENSION DeviceExt,
			    PLARGE_INTEGER Clusters)
/*
 * FUNCTION: Counts free clusters in a FAT32 table
 */
{
  ULONG sector;
  PULONG Block;
  ULONG ulCount = 0;
  ULONG i,forto;
  ULONG numberofclusters;
  ULONG numberofsectors;
  NTSTATUS Status;

  ExAcquireResourceSharedLite (&DeviceExt->FatResource, TRUE);

  Block = ExAllocatePool (NonPagedPool, BLOCKSIZE);

  numberofclusters = ((DeviceExt->Boot->Sectors ? DeviceExt->Boot->Sectors : DeviceExt->Boot->SectorsHuge)-DeviceExt->dataStart)/DeviceExt->Boot->SectorsPerCluster+2;
  numberofsectors = (numberofclusters +127) / 128;
  numberofclusters %= 128;

  for (sector = 0; sector < numberofsectors; sector++)
    {
      Status = VfatReadSectors(DeviceExt->StorageDevice,
			       (ULONG) (DeviceExt->FATStart + sector), 1,
			       (UCHAR *) Block);
      if (!NT_SUCCESS(Status))
	{
	  ExFreePool(Block);
	  ExReleaseResourceLite(&DeviceExt->FatResource);
	  return(Status);
	}

      if (sector == numberofsectors - 1)
	forto=numberofclusters;
      else
	forto=128;
      for (i = 0; i < forto; i++)
	{
	  if ((Block[i] & 0x0fffffff) == 0)
	    ulCount++;
	}
    }
  ExFreePool (Block);
  ExReleaseResourceLite (&DeviceExt->FatResource);

  Clusters->QuadPart = ulCount;

  return(STATUS_SUCCESS);
}

NTSTATUS
FAT12WriteCluster(PDEVICE_EXTENSION DeviceExt,
		  ULONG ClusterToWrite,
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
  PVOID Context;
  LARGE_INTEGER Offset;

  Offset.QuadPart = 0;
  if(!CcMapData(DeviceExt->FATFileObject, &Offset, DeviceExt->Boot->FATSectors * BLOCKSIZE, 1, &Context, &BaseAddress))
  {
    return STATUS_UNSUCCESSFUL;
  }
  CBlock = (PUCHAR)BaseAddress;

  FATOffset = (ClusterToWrite * 12) / 8;
  DPRINT("Writing 0x%x for 0x%x at 0x%x\n",
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
  CcSetDirtyPinnedData(Context, NULL);
  CcUnpinData(Context);
  return(STATUS_SUCCESS);
}

NTSTATUS
FAT16WriteCluster(PDEVICE_EXTENSION DeviceExt,
		  ULONG ClusterToWrite,
		  ULONG NewValue)
/*
 * FUNCTION: Writes a cluster to the FAT16 physical and in-memory tables
 */
{
  PVOID BaseAddress;
  NTSTATUS Status;
  ULONG FATOffset;
  ULONG i;
  ULONG ChunkSize;
  PVOID Context;
  LARGE_INTEGER Offset;

  ChunkSize = CACHEPAGESIZE(DeviceExt);
  FATOffset = ClusterToWrite * 2;
  Offset.QuadPart = ROUND_DOWN(FATOffset, ChunkSize);
  if(!CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
  {
    return STATUS_UNSUCCESSFUL;
  }
  DPRINT("Writing 0x%x for offset 0x%x 0x%x\n", NewValue, FATOffset,
	     ClusterToWrite);
  *((PUSHORT)(BaseAddress + (FATOffset % ChunkSize))) = NewValue;
  CcSetDirtyPinnedData(Context, NULL);
  CcUnpinData(Context);
  return(STATUS_SUCCESS);
}

NTSTATUS
FAT32WriteCluster(PDEVICE_EXTENSION DeviceExt,
		  ULONG ClusterToWrite,
		  ULONG NewValue)
/*
 * FUNCTION: Writes a cluster to the FAT32 physical tables
 */
{
  PVOID BaseAddress;
  NTSTATUS Status;
  ULONG FATOffset;
  ULONG i;
  ULONG ChunkSize;
  PVOID Context;
  LARGE_INTEGER Offset;

  ChunkSize = CACHEPAGESIZE(DeviceExt);

  FATOffset = (ClusterToWrite * 4);
  Offset.QuadPart = ROUND_DOWN(FATOffset, ChunkSize);
  if(!CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
  {
    return STATUS_UNSUCCESSFUL;
  }
  DPRINT("Writing 0x%x for offset 0x%x 0x%x\n", NewValue, FATOffset,
	     ClusterToWrite);
  *((PULONG)(BaseAddress + (FATOffset % ChunkSize))) = NewValue & 0x0fffffff;

  CcSetDirtyPinnedData(Context, NULL);
  CcUnpinData(Context);

  DPRINT("DeviceExt->Boot->FATSectors %d\n",
	     ((struct _BootSector32 *)DeviceExt->Boot)->FATSectors32);
  return(STATUS_SUCCESS);
}


NTSTATUS
WriteCluster(PDEVICE_EXTENSION DeviceExt,
	     ULONG ClusterToWrite,
	     ULONG NewValue)
/*
 * FUNCTION: Write a changed FAT entry
 */
{
  NTSTATUS Status;

  if (DeviceExt->FatType == FAT16)
    {
      Status = FAT16WriteCluster(DeviceExt, ClusterToWrite, NewValue);
    }
  else if (DeviceExt->FatType == FAT32)
    {
      Status = FAT32WriteCluster(DeviceExt, ClusterToWrite, NewValue);
    }
  else
    {
      Status = FAT12WriteCluster(DeviceExt, ClusterToWrite, NewValue);
    }
  return(Status);
}

ULONG
ClusterToSector(PDEVICE_EXTENSION DeviceExt,
		ULONG Cluster)
/*
 * FUNCTION: Converts the cluster number to a sector number for this physical
 *           device
 */
{
  return DeviceExt->dataStart +
    ((Cluster - 2) * DeviceExt->Boot->SectorsPerCluster);
}

NTSTATUS
VfatRawReadCluster(PDEVICE_EXTENSION DeviceExt,
		   ULONG FirstCluster,
		   PVOID Buffer,
		   ULONG Cluster,
           ULONG Count)
/*
 * FUNCTION: Load one ore more continus clusters from the physical device
 */
{

  if (FirstCluster == 1)
  {
    return VfatReadSectors(DeviceExt->StorageDevice, Cluster,
			 DeviceExt->Boot->SectorsPerCluster * Count, Buffer);
  }
  else
  {
    ULONG Sector;

    Sector = ClusterToSector(DeviceExt, Cluster);
    return VfatReadSectors(DeviceExt->StorageDevice, Sector,
			 DeviceExt->Boot->SectorsPerCluster * Count, Buffer);
  }
}

NTSTATUS
VfatRawWriteCluster(PDEVICE_EXTENSION DeviceExt,
		    ULONG FirstCluster,
		    PVOID Buffer,
		    ULONG Cluster,
            ULONG Count)
/*
 * FUNCTION: Write a cluster to the physical device
 */
{
  ULONG Sector;
  NTSTATUS Status;

  DPRINT("VfatWriteCluster(DeviceExt %x, Buffer %x, Cluster %d)\n",
	 DeviceExt, Buffer, Cluster);

  if (FirstCluster == 1)
  {
    Status = VfatWriteSectors(DeviceExt->StorageDevice, Cluster,
			   DeviceExt->Boot->SectorsPerCluster * Count, Buffer);
  }
  else
  {
    Sector = ClusterToSector(DeviceExt, Cluster);

    Status = VfatWriteSectors(DeviceExt->StorageDevice, Sector,
				DeviceExt->Boot->SectorsPerCluster * Count, Buffer);
  }
  return(Status);
}

NTSTATUS
GetNextCluster(PDEVICE_EXTENSION DeviceExt,
	       ULONG CurrentCluster,
	       PULONG NextCluster,
	       BOOLEAN Extend)
/*
 * FUNCTION: Retrieve the next cluster depending on the FAT type
 */
{
  NTSTATUS Status;

//  DPRINT ("GetNextCluster(DeviceExt %x, CurrentCluster %x)\n",
//	  DeviceExt, CurrentCluster);

  if (!Extend)
  {
    ExAcquireResourceSharedLite(&DeviceExt->FatResource, TRUE);
  }
  else
  {
    ExAcquireResourceExclusiveLite(&DeviceExt->FatResource, TRUE);
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
	   Status = FAT16FindAvailableCluster(DeviceExt, &NewCluster);
	   if (!NT_SUCCESS(Status))
	   {
          ExReleaseResourceLite(&DeviceExt->FatResource);
	      return(Status);
	   }
	 }
    else if (DeviceExt->FatType == FAT32)
	 {
	    Status = FAT32FindAvailableCluster(DeviceExt, &NewCluster);
	    if (!NT_SUCCESS(Status))
	    {
          ExReleaseResourceLite(&DeviceExt->FatResource);
	      return(Status);
	    }
	 }
    else
	 {
	    Status = FAT12FindAvailableCluster(DeviceExt, &NewCluster);
	    if (!NT_SUCCESS(Status))
	    {
          ExReleaseResourceLite(&DeviceExt->FatResource);
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
     Status = Fat16GetNextCluster(DeviceExt, CurrentCluster, NextCluster);
  }
  else if (DeviceExt->FatType == FAT32)
  {
     Status = Fat32GetNextCluster(DeviceExt, CurrentCluster, NextCluster);
  }
  else
  {
     Status = Fat12GetNextCluster(DeviceExt, CurrentCluster, NextCluster);
  }
  if (Extend && (*NextCluster) == 0xFFFFFFFF)
  {
     ULONG NewCluster;

     /* We are after last existing cluster, we must add one to file */
     /* Firstly, find the next available open allocation unit */
     if (DeviceExt->FatType == FAT16)
	  {
	    Status = FAT16FindAvailableCluster(DeviceExt, &NewCluster);
	    if (!NT_SUCCESS(Status))
	    {
         ExReleaseResourceLite(&DeviceExt->FatResource);
	      return(Status);
	    }
	  }
     else if (DeviceExt->FatType == FAT32)
	  {
	    Status = FAT32FindAvailableCluster(DeviceExt, &NewCluster);
	    if (!NT_SUCCESS(Status))
	    {
         ExReleaseResourceLite(&DeviceExt->FatResource);
	      return(Status);
	    }
	  }
     else
	  {
	    Status = FAT12FindAvailableCluster(DeviceExt, &NewCluster);
	    if (!NT_SUCCESS(Status))
	    {
         ExReleaseResourceLite(&DeviceExt->FatResource);
	      return(Status);
	    }
	  }
     /* Mark the new AU as the EOF */
     WriteCluster(DeviceExt, NewCluster, 0xFFFFFFFF);
     /* Now, write the AU of the LastCluster with the value of the newly
        found AU */
     WriteCluster(DeviceExt, CurrentCluster, NewCluster);
     *NextCluster = NewCluster;
  }

  ExReleaseResourceLite(&DeviceExt->FatResource);

  return(Status);
}


NTSTATUS
GetNextSector(PDEVICE_EXTENSION DeviceExt,
	      ULONG CurrentSector,
	      PULONG NextSector,
	      BOOLEAN Extend)
/* Some functions don't have access to the cluster they're really reading from.
   Maybe this is a dirty solution, but it will allow them to handle fragmentation. */
{
  NTSTATUS Status;

  DPRINT("GetNextSector(DeviceExt %x, CurrentSector %x)\n",
	 DeviceExt,
	 CurrentSector);
  if (CurrentSector<DeviceExt->dataStart || ((CurrentSector - DeviceExt->dataStart + 1) % DeviceExt -> Boot -> SectorsPerCluster))
  /* Basically, if the next sequential sector would be on a cluster border, then we'll need to check in the FAT */
    {
      (*NextSector)=CurrentSector+1;
      return (STATUS_SUCCESS);
    }
  else
    {
      CurrentSector = (CurrentSector - DeviceExt->dataStart) / DeviceExt -> Boot -> SectorsPerCluster + 2;

      Status = GetNextCluster(DeviceExt, CurrentSector, NextSector, Extend);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
      if ((*NextSector) == 0 || (*NextSector) == 0xffffffff)
	{
	  /* The caller wants to know a sector. These FAT codes don't correspond to any sector. */
	  return(STATUS_UNSUCCESSFUL);
	}

      (*NextSector) = ClusterToSector(DeviceExt,(*NextSector));
      return(STATUS_SUCCESS);
    }
}

/* EOF */
