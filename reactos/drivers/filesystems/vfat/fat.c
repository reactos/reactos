/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/fat.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "vfat.h"

/* GLOBALS ******************************************************************/

#define  CACHEPAGESIZE(pDeviceExt) ((pDeviceExt)->FatInfo.BytesPerCluster > PAGE_SIZE ? \
		   (pDeviceExt)->FatInfo.BytesPerCluster : PAGE_SIZE)

/* FUNCTIONS ****************************************************************/

NTSTATUS
FAT32GetNextCluster(PDEVICE_EXTENSION DeviceExt,
		    ULONG CurrentCluster,
		    PULONG NextCluster)
/*
 * FUNCTION: Retrieve the next FAT32 cluster from the FAT table via a physical
 *           disk read
 */
{
  PVOID BaseAddress;
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
  CurrentCluster = (*(PULONG)((char*)BaseAddress + (FATOffset % ChunkSize))) & 0x0fffffff;
  if (CurrentCluster >= 0xffffff8 && CurrentCluster <= 0xfffffff)
    CurrentCluster = 0xffffffff;
  CcUnpinData(Context);
  *NextCluster = CurrentCluster;
  return (STATUS_SUCCESS);
}

NTSTATUS
FAT16GetNextCluster(PDEVICE_EXTENSION DeviceExt,
		    ULONG CurrentCluster,
		    PULONG NextCluster)
/*
 * FUNCTION: Retrieve the next FAT16 cluster from the FAT table
 */
{
  PVOID BaseAddress;
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
  CurrentCluster = *((PUSHORT)((char*)BaseAddress + (FATOffset % ChunkSize)));
  if (CurrentCluster >= 0xfff8 && CurrentCluster <= 0xffff)
    CurrentCluster = 0xffffffff;
  CcUnpinData(Context);
  *NextCluster = CurrentCluster;
  return (STATUS_SUCCESS);
}

NTSTATUS
FAT12GetNextCluster(PDEVICE_EXTENSION DeviceExt,
		    ULONG CurrentCluster,
		    PULONG NextCluster)
/*
 * FUNCTION: Retrieve the next FAT12 cluster from the FAT table
 */
{
  PUSHORT CBlock;
  ULONG Entry;
  PVOID BaseAddress;
  PVOID Context;
  LARGE_INTEGER Offset;


  *NextCluster = 0;

  Offset.QuadPart = 0;
  if(!CcMapData(DeviceExt->FATFileObject, &Offset, DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector, 1, &Context, &BaseAddress))
  {
    return STATUS_UNSUCCESSFUL;
  }
  CBlock = (PUSHORT)((char*)BaseAddress + (CurrentCluster * 12) / 8);
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
FAT16FindAndMarkAvailableCluster(PDEVICE_EXTENSION DeviceExt,
                                 PULONG Cluster)
/*
 * FUNCTION: Finds the first available cluster in a FAT16 table
 */
{
  ULONG FatLength;
  ULONG StartCluster;
  ULONG i, j;
  PVOID BaseAddress;
  ULONG ChunkSize;
  PVOID Context = 0;
  LARGE_INTEGER Offset;
  PUSHORT Block;
  PUSHORT BlockEnd;

  ChunkSize = CACHEPAGESIZE(DeviceExt);
  FatLength = (DeviceExt->FatInfo.NumberOfClusters + 2);
  *Cluster = 0;
  StartCluster = DeviceExt->LastAvailableCluster;

  for (j = 0; j < 2; j++)
  {
    for (i = StartCluster; i < FatLength; )
    {
      Offset.QuadPart = ROUND_DOWN(i * 2, ChunkSize);
      if(!CcPinRead(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
      {
        DPRINT1("CcMapData(Offset %x, Length %d) failed\n", (ULONG)Offset.QuadPart, ChunkSize);
        return STATUS_UNSUCCESSFUL;
      }
      Block = (PUSHORT)((ULONG_PTR)BaseAddress + (i * 2) % ChunkSize);
      BlockEnd = (PUSHORT)((ULONG_PTR)BaseAddress + ChunkSize);

      /* Now process the whole block */
      while (Block < BlockEnd && i < FatLength)
      {
        if (*Block == 0)
        {
          DPRINT("Found available cluster 0x%x\n", i);
          DeviceExt->LastAvailableCluster = *Cluster = i;
          *Block = 0xffff;
          CcSetDirtyPinnedData(Context, NULL);
          CcUnpinData(Context);
          return(STATUS_SUCCESS);
        }

        Block++;
        i++;
      }

      CcUnpinData(Context);
    }
    FatLength = StartCluster;
    StartCluster = 2;
  }
  return(STATUS_DISK_FULL);
}

NTSTATUS
FAT12FindAndMarkAvailableCluster(PDEVICE_EXTENSION DeviceExt, PULONG Cluster)
/*
 * FUNCTION: Finds the first available cluster in a FAT12 table
 */
{
  ULONG FatLength;
  ULONG StartCluster;
  ULONG Entry;
  PUSHORT CBlock;
  ULONG i, j;
  PVOID BaseAddress;
  PVOID Context;
  LARGE_INTEGER Offset;

  FatLength = DeviceExt->FatInfo.NumberOfClusters + 2;
  *Cluster = 0;
  StartCluster = DeviceExt->LastAvailableCluster;
  Offset.QuadPart = 0;
  if(!CcPinRead(DeviceExt->FATFileObject, &Offset, DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector, 1, &Context, &BaseAddress))
  {
    DPRINT1("CcMapData(Offset %x, Length %d) failed\n", (ULONG)Offset.QuadPart, DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector);
    return STATUS_UNSUCCESSFUL;
  }

  for (j = 0; j < 2; j++)
  {
    for (i = StartCluster; i < FatLength; i++)
    {
      CBlock = (PUSHORT)((char*)BaseAddress + (i * 12) / 8);
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
	     if ((i % 2) == 0)
	       *CBlock = (*CBlock & 0xf000) | 0xfff;
	     else
	       *CBlock = (*CBlock & 0xf) | 0xfff0;
	     CcSetDirtyPinnedData(Context, NULL);
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
FAT32FindAndMarkAvailableCluster (PDEVICE_EXTENSION DeviceExt, PULONG Cluster)
/*
 * FUNCTION: Finds the first available cluster in a FAT32 table
 */
{
  ULONG FatLength;
  ULONG StartCluster;
  ULONG i, j;
  PVOID BaseAddress;
  ULONG ChunkSize;
  PVOID Context;
  LARGE_INTEGER Offset;
  PULONG Block;
  PULONG BlockEnd;

  ChunkSize = CACHEPAGESIZE(DeviceExt);
  FatLength = (DeviceExt->FatInfo.NumberOfClusters + 2);
  *Cluster = 0;
  StartCluster = DeviceExt->LastAvailableCluster;

  for (j = 0; j < 2; j++)
  {
    for (i = StartCluster; i < FatLength;)
    {
      Offset.QuadPart = ROUND_DOWN(i * 4, ChunkSize);
      if(!CcPinRead(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
      {
        DPRINT1("CcMapData(Offset %x, Length %d) failed\n", (ULONG)Offset.QuadPart, ChunkSize);
        return STATUS_UNSUCCESSFUL;
      }
      Block = (PULONG)((ULONG_PTR)BaseAddress + (i * 4) % ChunkSize);
      BlockEnd = (PULONG)((ULONG_PTR)BaseAddress + ChunkSize);

      /* Now process the whole block */
      while (Block < BlockEnd && i < FatLength)
      {
        if ((*Block & 0x0fffffff) == 0)
        {
          DPRINT("Found available cluster 0x%x\n", i);
          DeviceExt->LastAvailableCluster = *Cluster = i;
          *Block = 0x0fffffff;
          CcSetDirtyPinnedData(Context, NULL);
          CcUnpinData(Context);
          return(STATUS_SUCCESS);
        }

        Block++;
        i++;
      }

      CcUnpinData(Context);
    }
    FatLength = StartCluster;
    StartCluster = 2;
  }
  return (STATUS_DISK_FULL);
}

static NTSTATUS
FAT12CountAvailableClusters(PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Counts free cluster in a FAT12 table
 */
{
  ULONG Entry;
  PVOID BaseAddress;
  ULONG ulCount = 0;
  ULONG i;
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
    CBlock = (PUSHORT)((char*)BaseAddress + (i * 12) / 8);
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


static NTSTATUS
FAT16CountAvailableClusters(PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Counts free clusters in a FAT16 table
 */
{
  PUSHORT Block;
  PUSHORT BlockEnd;
  PVOID BaseAddress = NULL;
  ULONG ulCount = 0;
  ULONG i;
  ULONG ChunkSize;
  PVOID Context = NULL;
  LARGE_INTEGER Offset;
  ULONG FatLength;

  ChunkSize = CACHEPAGESIZE(DeviceExt);
  FatLength = (DeviceExt->FatInfo.NumberOfClusters + 2);

  for (i = 2; i < FatLength; )
  {
    Offset.QuadPart = ROUND_DOWN(i * 2, ChunkSize);
    if(!CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
    {
      return STATUS_UNSUCCESSFUL;
    }
    Block = (PUSHORT)((ULONG_PTR)BaseAddress + (i * 2) % ChunkSize);
    BlockEnd = (PUSHORT)((ULONG_PTR)BaseAddress + ChunkSize);

    /* Now process the whole block */
    while (Block < BlockEnd && i < FatLength)
    {
      if (*Block == 0)
        ulCount++;
      Block++;
      i++;
    }

    CcUnpinData(Context);
  }

  DeviceExt->AvailableClusters = ulCount;
  DeviceExt->AvailableClustersValid = TRUE;

  return(STATUS_SUCCESS);
}


static NTSTATUS
FAT32CountAvailableClusters(PDEVICE_EXTENSION DeviceExt)
/*
 * FUNCTION: Counts free clusters in a FAT32 table
 */
{
  PULONG Block;
  PULONG BlockEnd;
  PVOID BaseAddress = NULL;
  ULONG ulCount = 0;
  ULONG i;
  ULONG ChunkSize;
  PVOID Context = NULL;
  LARGE_INTEGER Offset;
  ULONG FatLength;

  ChunkSize = CACHEPAGESIZE(DeviceExt);
  FatLength = (DeviceExt->FatInfo.NumberOfClusters + 2);

  for (i = 2; i < FatLength; )
  {
    Offset.QuadPart = ROUND_DOWN(i * 4, ChunkSize);
    if(!CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
    {
      DPRINT1("CcMapData(Offset %x, Length %d) failed\n", (ULONG)Offset.QuadPart, ChunkSize);
      return STATUS_UNSUCCESSFUL;
    }
    Block = (PULONG)((ULONG_PTR)BaseAddress + (i * 4) % ChunkSize);
    BlockEnd = (PULONG)((ULONG_PTR)BaseAddress + ChunkSize);

    /* Now process the whole block */
    while (Block < BlockEnd && i < FatLength)
    {
      if ((*Block & 0x0fffffff) == 0)
        ulCount++;
      Block++;
      i++;
    }

    CcUnpinData(Context);
  }

  DeviceExt->AvailableClusters = ulCount;
  DeviceExt->AvailableClustersValid = TRUE;

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
	else if (DeviceExt->FatInfo.FatType == FAT16 || DeviceExt->FatInfo.FatType == FATX16)
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
  PVOID BaseAddress;
  PVOID Context;
  LARGE_INTEGER Offset;

  Offset.QuadPart = 0;
  if(!CcPinRead(DeviceExt->FATFileObject, &Offset, DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector, 1, &Context, &BaseAddress))
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
      CBlock[FATOffset] = (UCHAR)NewValue;
      CBlock[FATOffset + 1] &= 0xf0;
      CBlock[FATOffset + 1] |= (NewValue & 0xf00) >> 8;
    }
  else
    {
      *OldValue = (CBlock[FATOffset] >> 4) + (CBlock[FATOffset + 1] << 4);
      CBlock[FATOffset] &= 0x0f;
      CBlock[FATOffset] |= (NewValue & 0xf) << 4;
      CBlock[FATOffset + 1] = (UCHAR)(NewValue >> 4);
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
  ULONG FATOffset;
  ULONG ChunkSize;
  PVOID Context;
  LARGE_INTEGER Offset;
  PUSHORT Cluster;

  ChunkSize = CACHEPAGESIZE(DeviceExt);
  FATOffset = ClusterToWrite * 2;
  Offset.QuadPart = ROUND_DOWN(FATOffset, ChunkSize);
  if(!CcPinRead(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
  {
    return STATUS_UNSUCCESSFUL;
  }
  DPRINT("Writing 0x%x for offset 0x%x 0x%x\n", NewValue, FATOffset,
	     ClusterToWrite);
  Cluster = ((PUSHORT)((char*)BaseAddress + (FATOffset % ChunkSize)));
  *OldValue = *Cluster;
  *Cluster = (USHORT)NewValue;
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
  ULONG FATOffset;
  ULONG ChunkSize;
  PVOID Context;
  LARGE_INTEGER Offset;
  PULONG Cluster;

  ChunkSize = CACHEPAGESIZE(DeviceExt);

  FATOffset = (ClusterToWrite * 4);
  Offset.QuadPart = ROUND_DOWN(FATOffset, ChunkSize);
  if(!CcPinRead(DeviceExt->FATFileObject, &Offset, ChunkSize, 1, &Context, &BaseAddress))
  {
    return STATUS_UNSUCCESSFUL;
  }
  DPRINT("Writing 0x%x for offset 0x%x 0x%x\n", NewValue, FATOffset,
	     ClusterToWrite);
  Cluster = ((PULONG)((char*)BaseAddress + (FATOffset % ChunkSize)));
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
  Status = DeviceExt->WriteCluster(DeviceExt, ClusterToWrite, NewValue, &OldValue);
  if (DeviceExt->AvailableClustersValid)
  {
      if (OldValue && NewValue == 0)
        InterlockedIncrement((PLONG)&DeviceExt->AvailableClusters);
      else if (OldValue == 0 && NewValue)
        InterlockedDecrement((PLONG)&DeviceExt->AvailableClusters);
  }
  ExReleaseResourceLite(&DeviceExt->FatResource);
  return(Status);
}

ULONGLONG
ClusterToSector(PDEVICE_EXTENSION DeviceExt,
		ULONG Cluster)
/*
 * FUNCTION: Converts the cluster number to a sector number for this physical
 *           device
 */
{
  return DeviceExt->FatInfo.dataStart +
    ((ULONGLONG)(Cluster - 2) * DeviceExt->FatInfo.SectorsPerCluster);

}

NTSTATUS
GetNextCluster(PDEVICE_EXTENSION DeviceExt,
	       ULONG CurrentCluster,
	       PULONG NextCluster)
/*
 * FUNCTION: Retrieve the next cluster depending on the FAT type
 */
{
  NTSTATUS Status;

  DPRINT ("GetNextCluster(DeviceExt %p, CurrentCluster %x)\n",
	  DeviceExt, CurrentCluster);

  if (CurrentCluster == 0)
     return(STATUS_INVALID_PARAMETER);

  ExAcquireResourceSharedLite(&DeviceExt->FatResource, TRUE);
  Status = DeviceExt->GetNextCluster(DeviceExt, CurrentCluster, NextCluster);
  ExReleaseResourceLite(&DeviceExt->FatResource);

  return(Status);
}

NTSTATUS
GetNextClusterExtend(PDEVICE_EXTENSION DeviceExt,
	             ULONG CurrentCluster,
	             PULONG NextCluster)
/*
 * FUNCTION: Retrieve the next cluster depending on the FAT type
 */
{
  NTSTATUS Status;

  DPRINT ("GetNextClusterExtend(DeviceExt %p, CurrentCluster %x)\n",
	  DeviceExt, CurrentCluster);

  ExAcquireResourceExclusiveLite(&DeviceExt->FatResource, TRUE);
  CHECKPOINT;
  /*
   * If the file hasn't any clusters allocated then we need special
   * handling
   */
  if (CurrentCluster == 0)
  {
    ULONG NewCluster;

    Status = DeviceExt->FindAndMarkAvailableCluster(DeviceExt, &NewCluster);
    if (!NT_SUCCESS(Status))
    {
      ExReleaseResourceLite(&DeviceExt->FatResource);
      return Status;
    }

    *NextCluster = NewCluster;
    ExReleaseResourceLite(&DeviceExt->FatResource);
    return(STATUS_SUCCESS);
  }

  Status = DeviceExt->GetNextCluster(DeviceExt, CurrentCluster, NextCluster);

  if ((*NextCluster) == 0xFFFFFFFF)
  {
     ULONG NewCluster;

     /* We are after last existing cluster, we must add one to file */
     /* Firstly, find the next available open allocation unit and
        mark it as end of file */
     Status = DeviceExt->FindAndMarkAvailableCluster(DeviceExt, &NewCluster);
     if (!NT_SUCCESS(Status))
     {
        ExReleaseResourceLite(&DeviceExt->FatResource);
        return Status;
     }

     /* Now, write the AU of the LastCluster with the value of the newly
        found AU */
     WriteCluster(DeviceExt, CurrentCluster, NewCluster);
     *NextCluster = NewCluster;
  }

  ExReleaseResourceLite(&DeviceExt->FatResource);

  return(Status);
}

/* EOF */
