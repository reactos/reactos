/*
 * $Id: fat.c,v 1.39 2002/09/08 10:22:12 chorns Exp $
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

#define  CACHEPAGESIZE(pDeviceExt) ((pDeviceExt)->FatInfo.BytesPerCluster > PAGESIZE ? \
		   (pDeviceExt)->FatInfo.BytesPerCluster : PAGESIZE)

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
  PUSHORT CBlock;
  ULONG FATOffset;
  ULONG Entry;
  NTSTATUS Status;
  PVOID BaseAddress;
  PVOID Context;
  LARGE_INTEGER Offset;


  *NextCluster = 0;

  Offset.QuadPart = 0;
  if(!CcMapData(DeviceExt->FATFileObject, &Offset, DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector, 1, &Context, &BaseAddress))
  {
    return STATUS_UNSUCCESSFUL;
  }
  CBlock = (PUSHORT)(BaseAddress + (CurrentCluster * 12) / 8);
  if ((CurrentCluster % 2) == 0)
    {
      Entry = *CBlock & 0x0fff;
    }
  else
    {
      Entry = *CBlock >> 4;
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
  ULONG StartCluster;
  ULONG i, j;
  NTSTATUS Status;
  PVOID BaseAddress;
  ULONG ChunkSize;
  PVOID Context = 0;
  LARGE_INTEGER Offset;
  PUSHORT Block;

  ChunkSize = CACHEPAGESIZE(DeviceExt);
  FatLength = (DeviceExt->FatInfo.NumberOfClusters +2 ) * 2;
  *Cluster = 0;
  StartCluster = DeviceExt->LastAvailableCluster;

  for (j = 0; j < 2; j++)
  {
    for (i = StartCluster * 2; i < FatLength; i += 2, Block++)
    {
      if ((i % ChunkSize) == 0 || Context == NULL)
	   {
        Offset.QuadPart = ROUND_DOWN(i, ChunkSize);
        if (Context != NULL)
        {
          CcUnpinData(Context);
        }
        CHECKPOINT;
        if(!CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
        {
          DPRINT1("CcMapData(Offset %x, Length %d) failed\n", (ULONG)Offset.QuadPart, ChunkSize);
          return STATUS_UNSUCCESSFUL;
        }
        CHECKPOINT;
		Block = (PUSHORT)(BaseAddress + i % ChunkSize);
	   }

      if (*Block == 0)
	   {
	     DPRINT("Found available cluster 0x%x\n", i / 2);
	     DeviceExt->LastAvailableCluster = *Cluster = i / 2;
        CcUnpinData(Context);
	     return(STATUS_SUCCESS);
	   }
    }
    FatLength = StartCluster * 2;
    StartCluster = 2;
    if (Context != NULL)
    {
      CcUnpinData(Context);
      Context =NULL;
    }
  }
  return(STATUS_DISK_FULL);
}

NTSTATUS
FAT12FindAvailableCluster(PDEVICE_EXTENSION DeviceExt, PULONG Cluster)
/*
 * FUNCTION: Finds the first available cluster in a FAT12 table
 */
{
  ULONG FatLength;
  ULONG FATOffset;
  ULONG StartCluster;
  ULONG Entry;
  PUSHORT CBlock;
  ULONG i, j;
  PVOID BaseAddress;
  NTSTATUS Status;
  PVOID Context;
  LARGE_INTEGER Offset;

  FatLength = DeviceExt->FatInfo.NumberOfClusters + 2;
  *Cluster = 0;
  StartCluster = DeviceExt->LastAvailableCluster;
  Offset.QuadPart = 0;
  if(!CcMapData(DeviceExt->FATFileObject, &Offset, DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector, 1, &Context, &BaseAddress))
  {
    DPRINT1("CcMapData(Offset %x, Length %d) failed\n", (ULONG)Offset.QuadPart, DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector);
    return STATUS_UNSUCCESSFUL;
  }

  for (j = 0; j < 2; j++)
  {
    for (i = StartCluster; i < FatLength; i++)
    {
      CBlock = (PUSHORT)(BaseAddress + (i * 12) / 8);
      if ((i % 2) == 0)
	   {
	     Entry = *CBlock & 0xfff;
	   }
      else
	   {
	     Entry = *CBlock >> 4;
	   }
      if (Entry == 0)
	   {
	     DPRINT("Found available cluster 0x%x\n", i);
	     DeviceExt->LastAvailableCluster = *Cluster = i;
        CcUnpinData(Context);
	     return(STATUS_SUCCESS);
	   }
    }
    FatLength = StartCluster;
    StartCluster = 2;
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
  ULONG StartCluster;
  ULONG i, j;
  NTSTATUS Status;
  PVOID BaseAddress;
  ULONG ChunkSize;
  PVOID Context = 0;
  LARGE_INTEGER Offset;
  PULONG Block;

  ChunkSize = CACHEPAGESIZE(DeviceExt);
  FatLength = (DeviceExt->FatInfo.NumberOfClusters + 2) * 4;
  *Cluster = 0;
  StartCluster = DeviceExt->LastAvailableCluster;

  for (j = 0; j < 2; j++)
  {
    for (i = StartCluster * 4; i < FatLength; i += 4, Block++)
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
          DPRINT1("CcMapData(Offset %x, Length %d) failed\n", (ULONG)Offset.QuadPart, ChunkSize);
          return STATUS_UNSUCCESSFUL;
	}
	Block = (PULONG)(BaseAddress + i % ChunkSize);
      }

      if ((*Block & 0x0fffffff) == 0)
      {
        DPRINT("Found available cluster 0x%x\n", i / 4);
        DeviceExt->LastAvailableCluster = *Cluster = i / 4;
        CcUnpinData(Context);
        return(STATUS_SUCCESS);
      }
    }
    FatLength = StartCluster * 4;
    StartCluster = 2;
    if (Context != NULL)
    {
      CcUnpinData(Context);
      Context=NULL;
    }
  }
  return (STATUS_DISK_FULL);
}


NTSTATUS
FAT12CountAvailableClusters(PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Counts free cluster in a FAT12 table
 */
{
  ULONG FATOffset;
  ULONG Entry;
  PVOID BaseAddress;
  ULONG ulCount = 0;
  ULONG i;
  NTSTATUS Status;
  ULONG numberofclusters;
  LARGE_INTEGER Offset;
  PVOID Context;
  PUSHORT CBlock;

  Offset.QuadPart = 0;
  if(!CcMapData(DeviceExt->FATFileObject, &Offset, DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector, 1, &Context, &BaseAddress))
  {
    return STATUS_UNSUCCESSFUL;
  }

  numberofclusters = DeviceExt->FatInfo.NumberOfClusters + 2;

  for (i = 2; i < numberofclusters; i++)
    {
    CBlock = (PUSHORT)(BaseAddress + (i * 12) / 8);
      if ((i % 2) == 0)
	{
      Entry = *CBlock & 0x0fff;
	}
      else
	{
      Entry = *CBlock >> 4;
	}
      if (Entry == 0)
	ulCount++;
    }

  CcUnpinData(Context);
  DeviceExt->AvailableClusters = ulCount;
  DeviceExt->AvailableClustersValid = TRUE;

  return(STATUS_SUCCESS);
}


NTSTATUS
FAT16CountAvailableClusters(PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Counts free clusters in a FAT16 table
 */
{
  PUSHORT Block;
  PVOID BaseAddress = NULL;
  ULONG ulCount = 0;
  ULONG i;
  ULONG ChunkSize;
  NTSTATUS Status;
  PVOID Context = NULL;
  LARGE_INTEGER Offset;
  ULONG FatLength;

  ChunkSize = CACHEPAGESIZE(DeviceExt);
  FatLength = (DeviceExt->FatInfo.NumberOfClusters + 2) * 2;

  for (i = 4; i< FatLength; i += 2, Block++)
  {
    if ((i % ChunkSize) == 0 || Context == NULL)
    {
		DPRINT("%d\n", i/2);
		if (Context)
	{
			CcUnpinData(Context);
	}
		Offset.QuadPart = ROUND_DOWN(i, ChunkSize);
		if(!CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
	{
			return STATUS_UNSUCCESSFUL;
		}
		Block = (PUSHORT)(BaseAddress + i % ChunkSize);
	}
	if (*Block == 0)
	    ulCount++;
	}

  DeviceExt->AvailableClusters = ulCount;
  DeviceExt->AvailableClustersValid = TRUE;
  CcUnpinData(Context);

  return(STATUS_SUCCESS);
}


NTSTATUS
FAT32CountAvailableClusters(PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Counts free clusters in a FAT32 table
 */
{
  PULONG Block;
  PVOID BaseAddress = NULL;
  ULONG ulCount = 0;
  ULONG i;
  ULONG ChunkSize;
  NTSTATUS Status;
  PVOID Context = NULL;
  LARGE_INTEGER Offset;
  ULONG FatLength;

  ChunkSize = CACHEPAGESIZE(DeviceExt);
  FatLength = (DeviceExt->FatInfo.NumberOfClusters + 2) * 4;

  for (i = 8; i< FatLength; i += 4, Block++)
  {
     if ((i % ChunkSize) == 0 || Context == NULL)
     {
	if (Context)
	{
	   CcUnpinData(Context);
	}
	Offset.QuadPart = ROUND_DOWN(i, ChunkSize);
	if(!CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
	{
	   return STATUS_UNSUCCESSFUL;
	}
	Block = (PULONG)(BaseAddress + i % ChunkSize);
     }
     if ((*Block & 0x0fffffff) == 0)
       ulCount++;
  }

  DeviceExt->AvailableClusters = ulCount;
  DeviceExt->AvailableClustersValid = TRUE;
  CcUnpinData(Context);

  return(STATUS_SUCCESS);
}

NTSTATUS
CountAvailableClusters(PDEVICE_EXTENSION DeviceExt,
			    PLARGE_INTEGER Clusters)
{
  NTSTATUS Status = STATUS_SUCCESS;
  ExAcquireResourceExclusiveLite (&DeviceExt->FatResource, TRUE);
  if (!DeviceExt->AvailableClustersValid)
  {
	if (DeviceExt->FatInfo.FatType == FAT12)
	  Status = FAT12CountAvailableClusters(DeviceExt);
	else if (DeviceExt->FatInfo.FatType == FAT16)
	  Status = FAT16CountAvailableClusters(DeviceExt);
	else
	  Status = FAT32CountAvailableClusters(DeviceExt);
    }
  Clusters->QuadPart = DeviceExt->AvailableClusters;
  ExReleaseResourceLite (&DeviceExt->FatResource);

  return Status;
}





NTSTATUS
FAT12WriteCluster(PDEVICE_EXTENSION DeviceExt,
		  ULONG ClusterToWrite,
		  ULONG NewValue,
		  PULONG OldValue)
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
  if(!CcMapData(DeviceExt->FATFileObject, &Offset, DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector, 1, &Context, &BaseAddress))
  {
    return STATUS_UNSUCCESSFUL;
  }
  CBlock = (PUCHAR)BaseAddress;

  FATOffset = (ClusterToWrite * 12) / 8;
  DPRINT("Writing 0x%x for 0x%x at 0x%x\n",
	  NewValue, ClusterToWrite, FATOffset);
  if ((ClusterToWrite % 2) == 0)
    {
      *OldValue = CBlock[FATOffset] + ((CBlock[FATOffset + 1] & 0x0f) << 8);
      CBlock[FATOffset] = NewValue;
      CBlock[FATOffset + 1] &= 0xf0;
      CBlock[FATOffset + 1] |= (NewValue & 0xf00) >> 8;
    }
  else
    {
      *OldValue = (CBlock[FATOffset] >> 4) + (CBlock[FATOffset + 1] << 4);
      CBlock[FATOffset] &= 0x0f;
      CBlock[FATOffset] |= (NewValue & 0xf) << 4;
      CBlock[FATOffset + 1] = NewValue >> 4;
    }
  /* Write the changed FAT sector(s) to disk */
  FATsector = FATOffset / DeviceExt->FatInfo.BytesPerSector;
  CcSetDirtyPinnedData(Context, NULL);
  CcUnpinData(Context);
  return(STATUS_SUCCESS);
}

NTSTATUS
FAT16WriteCluster(PDEVICE_EXTENSION DeviceExt,
		  ULONG ClusterToWrite,
		  ULONG NewValue,
		  PULONG OldValue)
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
  PUSHORT Cluster;

  ChunkSize = CACHEPAGESIZE(DeviceExt);
  FATOffset = ClusterToWrite * 2;
  Offset.QuadPart = ROUND_DOWN(FATOffset, ChunkSize);
  if(!CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
  {
    return STATUS_UNSUCCESSFUL;
  }
  DPRINT("Writing 0x%x for offset 0x%x 0x%x\n", NewValue, FATOffset,
	     ClusterToWrite);
  Cluster = ((PUSHORT)(BaseAddress + (FATOffset % ChunkSize)));
  *OldValue = *Cluster;
  *Cluster = NewValue;
  CcSetDirtyPinnedData(Context, NULL);
  CcUnpinData(Context);
  return(STATUS_SUCCESS);
}

NTSTATUS
FAT32WriteCluster(PDEVICE_EXTENSION DeviceExt,
		  ULONG ClusterToWrite,
		  ULONG NewValue,
		  PULONG OldValue)
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
  PULONG Cluster;

  ChunkSize = CACHEPAGESIZE(DeviceExt);

  FATOffset = (ClusterToWrite * 4);
  Offset.QuadPart = ROUND_DOWN(FATOffset, ChunkSize);
  if(!CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
  {
    return STATUS_UNSUCCESSFUL;
  }
  DPRINT("Writing 0x%x for offset 0x%x 0x%x\n", NewValue, FATOffset,
	     ClusterToWrite);
  Cluster = ((PULONG)(BaseAddress + (FATOffset % ChunkSize)));
  *OldValue = *Cluster & 0x0fffffff;
  *Cluster = (*Cluster & 0xf0000000) | (NewValue & 0x0fffffff);

  CcSetDirtyPinnedData(Context, NULL);
  CcUnpinData(Context);

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
  ULONG OldValue;
  ExAcquireResourceExclusiveLite (&DeviceExt->FatResource, TRUE);
  if (DeviceExt->FatInfo.FatType == FAT16)
    {
      Status = FAT16WriteCluster(DeviceExt, ClusterToWrite, NewValue, &OldValue);
    }
  else if (DeviceExt->FatInfo.FatType == FAT32)
    {
      Status = FAT32WriteCluster(DeviceExt, ClusterToWrite, NewValue, &OldValue);
    }
  else
    {
      Status = FAT12WriteCluster(DeviceExt, ClusterToWrite, NewValue, &OldValue);
    }
  if (DeviceExt->AvailableClustersValid)
	{
      if (OldValue && NewValue == 0)
        InterlockedIncrement(&DeviceExt->AvailableClusters);
      else if (OldValue == 0 && NewValue)
        InterlockedDecrement(&DeviceExt->AvailableClusters);
    }
  ExReleaseResourceLite(&DeviceExt->FatResource);
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
  return DeviceExt->FatInfo.dataStart +
    ((Cluster - 2) * DeviceExt->FatInfo.SectorsPerCluster);
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

  DPRINT ("GetNextCluster(DeviceExt %x, CurrentCluster %x)\n",
	  DeviceExt, CurrentCluster);

  if (!Extend)
  {
    ExAcquireResourceSharedLite(&DeviceExt->FatResource, TRUE);
  }
  else
  {
    ExAcquireResourceExclusiveLite(&DeviceExt->FatResource, TRUE);
  }
  CHECKPOINT;
  /*
   * If the file hasn't any clusters allocated then we need special
   * handling
   */
  if (CurrentCluster == 0 && Extend)
  {
    ULONG NewCluster;

    if (DeviceExt->FatInfo.FatType == FAT16)
	 {
	   CHECKPOINT;
	   Status = FAT16FindAvailableCluster(DeviceExt, &NewCluster);
	   CHECKPOINT;
	   if (!NT_SUCCESS(Status))
	   {
          ExReleaseResourceLite(&DeviceExt->FatResource);
	      return(Status);
	   }
	 }
    else if (DeviceExt->FatInfo.FatType == FAT32)
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

  if (DeviceExt->FatInfo.FatType == FAT16)
  {
     Status = Fat16GetNextCluster(DeviceExt, CurrentCluster, NextCluster);
  }
  else if (DeviceExt->FatInfo.FatType == FAT32)
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
     if (DeviceExt->FatInfo.FatType == FAT16)
	  {
	    Status = FAT16FindAvailableCluster(DeviceExt, &NewCluster);
	    if (!NT_SUCCESS(Status))
	    {
         ExReleaseResourceLite(&DeviceExt->FatResource);
	      return(Status);
	    }
	  }
     else if (DeviceExt->FatInfo.FatType == FAT32)
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
  if (CurrentSector<DeviceExt->FatInfo.dataStart || ((CurrentSector - DeviceExt->FatInfo.dataStart + 1) % DeviceExt->FatInfo.SectorsPerCluster))
  /* Basically, if the next sequential sector would be on a cluster border, then we'll need to check in the FAT */
    {
      (*NextSector)=CurrentSector+1;
      return (STATUS_SUCCESS);
    }
  else
    {
      CurrentSector = (CurrentSector - DeviceExt->FatInfo.dataStart) / DeviceExt->FatInfo.SectorsPerCluster + 2;

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
