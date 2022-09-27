/*
 * PROJECT:     VFAT Filesystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     File Allocation Table routines
 * COPYRIGHT:   Copyright 1998 Jason Filby <jasonfilby@yahoo.com>
 *              Copyright 2015-2018 Pierre Schweitzer <pierre@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

#define  CACHEPAGESIZE(pDeviceExt) ((pDeviceExt)->FatInfo.BytesPerCluster > PAGE_SIZE ? \
		   (pDeviceExt)->FatInfo.BytesPerCluster : PAGE_SIZE)

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Retrieve the next FAT32 cluster from the FAT table via a physical
 *           disk read
 */
NTSTATUS
FAT32GetNextCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG CurrentCluster,
    PULONG NextCluster)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID BaseAddress;
    ULONG FATOffset;
    ULONG ChunkSize;
    PVOID Context = NULL;
    LARGE_INTEGER Offset;

    ChunkSize = CACHEPAGESIZE(DeviceExt);
    FATOffset = CurrentCluster * sizeof(ULONG);
    Offset.QuadPart = ROUND_DOWN(FATOffset, ChunkSize);
    _SEH2_TRY
    {
        if (!CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, MAP_WAIT, &Context, &BaseAddress))
        {
            NT_ASSERT(FALSE);
            return STATUS_UNSUCCESSFUL;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    CurrentCluster = (*(PULONG)((char*)BaseAddress + (FATOffset % ChunkSize))) & 0x0fffffff;
    if (CurrentCluster >= 0xffffff8 && CurrentCluster <= 0xfffffff)
        CurrentCluster = 0xffffffff;

    if (CurrentCluster == 0)
    {
        DPRINT1("WARNING: File system corruption detected. You may need to run a disk repair utility.\n");
        Status = STATUS_FILE_CORRUPT_ERROR;
        if (VfatGlobalData->Flags & VFAT_BREAK_ON_CORRUPTION)
            ASSERT(CurrentCluster != 0);
    }
    CcUnpinData(Context);
    *NextCluster = CurrentCluster;
    return Status;
}

/*
 * FUNCTION: Retrieve the next FAT16 cluster from the FAT table
 */
NTSTATUS
FAT16GetNextCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG CurrentCluster,
    PULONG NextCluster)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID BaseAddress;
    ULONG FATOffset;
    ULONG ChunkSize;
    PVOID Context;
    LARGE_INTEGER Offset;

    ChunkSize = CACHEPAGESIZE(DeviceExt);
    FATOffset = CurrentCluster * 2;
    Offset.QuadPart = ROUND_DOWN(FATOffset, ChunkSize);
    _SEH2_TRY
    {
        CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, MAP_WAIT, &Context, &BaseAddress);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    CurrentCluster = *((PUSHORT)((char*)BaseAddress + (FATOffset % ChunkSize)));
    if (CurrentCluster >= 0xfff8 && CurrentCluster <= 0xffff)
        CurrentCluster = 0xffffffff;

    if (CurrentCluster == 0)
    {
        DPRINT1("WARNING: File system corruption detected. You may need to run a disk repair utility.\n");
        Status = STATUS_FILE_CORRUPT_ERROR;
        if (VfatGlobalData->Flags & VFAT_BREAK_ON_CORRUPTION)
            ASSERT(CurrentCluster != 0);
    }

    CcUnpinData(Context);
    *NextCluster = CurrentCluster;
    return Status;
}

/*
 * FUNCTION: Retrieve the next FAT12 cluster from the FAT table
 */
NTSTATUS
FAT12GetNextCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG CurrentCluster,
    PULONG NextCluster)
{
    PUSHORT CBlock;
    ULONG Entry;
    PVOID BaseAddress;
    PVOID Context;
    LARGE_INTEGER Offset;

    *NextCluster = 0;

    Offset.QuadPart = 0;
    _SEH2_TRY
    {
        CcMapData(DeviceExt->FATFileObject, &Offset, DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector, MAP_WAIT, &Context, &BaseAddress);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    CBlock = (PUSHORT)((char*)BaseAddress + (CurrentCluster * 12) / 8);
    if ((CurrentCluster % 2) == 0)
    {
         Entry = *CBlock & 0x0fff;
    }
    else
    {
        Entry = *CBlock >> 4;
    }

//    DPRINT("Entry %x\n",Entry);
    if (Entry >= 0xff8 && Entry <= 0xfff)
        Entry = 0xffffffff;

//    DPRINT("Returning %x\n",Entry);
    ASSERT(Entry != 0);
    *NextCluster = Entry;
    CcUnpinData(Context);
//    return Entry == 0xffffffff ? STATUS_END_OF_FILE : STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

/*
 * FUNCTION: Finds the first available cluster in a FAT16 table
 */
NTSTATUS
FAT16FindAndMarkAvailableCluster(
    PDEVICE_EXTENSION DeviceExt,
    PULONG Cluster)
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
        for (i = StartCluster; i < FatLength;)
        {
            Offset.QuadPart = ROUND_DOWN(i * 2, ChunkSize);
            _SEH2_TRY
            {
                CcPinRead(DeviceExt->FATFileObject, &Offset, ChunkSize, PIN_WAIT, &Context, &BaseAddress);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                DPRINT1("CcPinRead(Offset %x, Length %u) failed\n", (ULONG)Offset.QuadPart, ChunkSize);
                _SEH2_YIELD(return _SEH2_GetExceptionCode());
            }
            _SEH2_END;

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
                    if (DeviceExt->AvailableClustersValid)
                        InterlockedDecrement((PLONG)&DeviceExt->AvailableClusters);
                    return STATUS_SUCCESS;
                }

                Block++;
                i++;
            }

            CcUnpinData(Context);
        }

        FatLength = StartCluster;
        StartCluster = 2;
    }

    return STATUS_DISK_FULL;
}

/*
 * FUNCTION: Finds the first available cluster in a FAT12 table
 */
NTSTATUS
FAT12FindAndMarkAvailableCluster(
    PDEVICE_EXTENSION DeviceExt,
    PULONG Cluster)
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
    _SEH2_TRY
    {
        CcPinRead(DeviceExt->FATFileObject, &Offset, DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector, PIN_WAIT, &Context, &BaseAddress);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("CcPinRead(Offset %x, Length %u) failed\n", (ULONG)Offset.QuadPart, DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector);
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

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
                if (DeviceExt->AvailableClustersValid)
                    InterlockedDecrement((PLONG)&DeviceExt->AvailableClusters);
                return STATUS_SUCCESS;
            }
        }
        FatLength = StartCluster;
        StartCluster = 2;
    }
    CcUnpinData(Context);
    return STATUS_DISK_FULL;
}

/*
 * FUNCTION: Finds the first available cluster in a FAT32 table
 */
NTSTATUS
FAT32FindAndMarkAvailableCluster(
    PDEVICE_EXTENSION DeviceExt,
    PULONG Cluster)
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
            _SEH2_TRY
            {
                CcPinRead(DeviceExt->FATFileObject, &Offset, ChunkSize, PIN_WAIT, &Context, &BaseAddress);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                DPRINT1("CcPinRead(Offset %x, Length %u) failed\n", (ULONG)Offset.QuadPart, ChunkSize);
                _SEH2_YIELD(return _SEH2_GetExceptionCode());
            }
            _SEH2_END;
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
                    if (DeviceExt->AvailableClustersValid)
                        InterlockedDecrement((PLONG)&DeviceExt->AvailableClusters);
                    return STATUS_SUCCESS;
                }

                Block++;
                i++;
            }

            CcUnpinData(Context);
        }
        FatLength = StartCluster;
        StartCluster = 2;
    }
    return STATUS_DISK_FULL;
}

/*
 * FUNCTION: Counts free cluster in a FAT12 table
 */
static
NTSTATUS
FAT12CountAvailableClusters(
    PDEVICE_EXTENSION DeviceExt)
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
    _SEH2_TRY
    {
        CcMapData(DeviceExt->FATFileObject, &Offset, DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector, MAP_WAIT, &Context, &BaseAddress);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

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

    return STATUS_SUCCESS;
}


/*
 * FUNCTION: Counts free clusters in a FAT16 table
 */
static
NTSTATUS
FAT16CountAvailableClusters(
    PDEVICE_EXTENSION DeviceExt)
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
        _SEH2_TRY
        {
            CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, MAP_WAIT, &Context, &BaseAddress);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
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

    return STATUS_SUCCESS;
}


/*
 * FUNCTION: Counts free clusters in a FAT32 table
 */
static
NTSTATUS
FAT32CountAvailableClusters(
    PDEVICE_EXTENSION DeviceExt)
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
        _SEH2_TRY
        {
            CcMapData(DeviceExt->FATFileObject, &Offset, ChunkSize, MAP_WAIT, &Context, &BaseAddress);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            DPRINT1("CcMapData(Offset %x, Length %u) failed\n", (ULONG)Offset.QuadPart, ChunkSize);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
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

    return STATUS_SUCCESS;
}

NTSTATUS
CountAvailableClusters(
    PDEVICE_EXTENSION DeviceExt,
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
    if (Clusters != NULL)
    {
        Clusters->QuadPart = DeviceExt->AvailableClusters;
    }
    ExReleaseResourceLite (&DeviceExt->FatResource);

    return Status;
}


/*
 * FUNCTION: Writes a cluster to the FAT12 physical and in-memory tables
 */
NTSTATUS
FAT12WriteCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG ClusterToWrite,
    ULONG NewValue,
    PULONG OldValue)
{
    ULONG FATOffset;
    PUCHAR CBlock;
    PVOID BaseAddress;
    PVOID Context;
    LARGE_INTEGER Offset;

    Offset.QuadPart = 0;
    _SEH2_TRY
    {
        CcPinRead(DeviceExt->FATFileObject, &Offset, DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector, PIN_WAIT, &Context, &BaseAddress);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;
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
    CcSetDirtyPinnedData(Context, NULL);
    CcUnpinData(Context);
    return STATUS_SUCCESS;
}

/*
 * FUNCTION: Writes a cluster to the FAT16 physical and in-memory tables
 */
NTSTATUS
FAT16WriteCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG ClusterToWrite,
    ULONG NewValue,
    PULONG OldValue)
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
    _SEH2_TRY
    {
        CcPinRead(DeviceExt->FATFileObject, &Offset, ChunkSize, PIN_WAIT, &Context, &BaseAddress);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    DPRINT("Writing 0x%x for offset 0x%x 0x%x\n", NewValue, FATOffset,
           ClusterToWrite);
    Cluster = ((PUSHORT)((char*)BaseAddress + (FATOffset % ChunkSize)));
    *OldValue = *Cluster;
    *Cluster = (USHORT)NewValue;
    CcSetDirtyPinnedData(Context, NULL);
    CcUnpinData(Context);
    return STATUS_SUCCESS;
}

/*
 * FUNCTION: Writes a cluster to the FAT32 physical tables
 */
NTSTATUS
FAT32WriteCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG ClusterToWrite,
    ULONG NewValue,
    PULONG OldValue)
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
    _SEH2_TRY
    {
        CcPinRead(DeviceExt->FATFileObject, &Offset, ChunkSize, PIN_WAIT, &Context, &BaseAddress);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    DPRINT("Writing 0x%x for offset 0x%x 0x%x\n", NewValue, FATOffset,
           ClusterToWrite);
    Cluster = ((PULONG)((char*)BaseAddress + (FATOffset % ChunkSize)));
    *OldValue = *Cluster & 0x0fffffff;
    *Cluster = (*Cluster & 0xf0000000) | (NewValue & 0x0fffffff);

    CcSetDirtyPinnedData(Context, NULL);
    CcUnpinData(Context);

    return STATUS_SUCCESS;
}


/*
 * FUNCTION: Write a changed FAT entry
 */
NTSTATUS
WriteCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG ClusterToWrite,
    ULONG NewValue)
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
    return Status;
}

/*
 * FUNCTION: Converts the cluster number to a sector number for this physical
 *           device
 */
ULONGLONG
ClusterToSector(
    PDEVICE_EXTENSION DeviceExt,
    ULONG Cluster)
{
    return DeviceExt->FatInfo.dataStart +
           ((ULONGLONG)(Cluster - 2) * DeviceExt->FatInfo.SectorsPerCluster);

}

/*
 * FUNCTION: Retrieve the next cluster depending on the FAT type
 */
NTSTATUS
GetNextCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG CurrentCluster,
    PULONG NextCluster)
{
    NTSTATUS Status;

    DPRINT("GetNextCluster(DeviceExt %p, CurrentCluster %x)\n",
           DeviceExt, CurrentCluster);

    if (CurrentCluster == 0)
    {
        DPRINT1("WARNING: File system corruption detected. You may need to run a disk repair utility.\n");
        if (VfatGlobalData->Flags & VFAT_BREAK_ON_CORRUPTION)
            ASSERT(CurrentCluster != 0);
        return STATUS_FILE_CORRUPT_ERROR;
    }

    ExAcquireResourceSharedLite(&DeviceExt->FatResource, TRUE);
    Status = DeviceExt->GetNextCluster(DeviceExt, CurrentCluster, NextCluster);
    ExReleaseResourceLite(&DeviceExt->FatResource);

    return Status;
}

/*
 * FUNCTION: Retrieve the next cluster depending on the FAT type
 */
NTSTATUS
GetNextClusterExtend(
    PDEVICE_EXTENSION DeviceExt,
    ULONG CurrentCluster,
    PULONG NextCluster)
{
    ULONG NewCluster;
    NTSTATUS Status;

    DPRINT("GetNextClusterExtend(DeviceExt %p, CurrentCluster %x)\n",
           DeviceExt, CurrentCluster);

    ExAcquireResourceExclusiveLite(&DeviceExt->FatResource, TRUE);
    /*
     * If the file hasn't any clusters allocated then we need special
     * handling
     */
    if (CurrentCluster == 0)
    {
        Status = DeviceExt->FindAndMarkAvailableCluster(DeviceExt, &NewCluster);
        if (!NT_SUCCESS(Status))
        {
            ExReleaseResourceLite(&DeviceExt->FatResource);
            return Status;
        }

        *NextCluster = NewCluster;
        ExReleaseResourceLite(&DeviceExt->FatResource);
        return STATUS_SUCCESS;
    }

    Status = DeviceExt->GetNextCluster(DeviceExt, CurrentCluster, NextCluster);

    if ((*NextCluster) == 0xFFFFFFFF)
    {
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
    return Status;
}

/*
 * FUNCTION: Retrieve the dirty status
 */
NTSTATUS
GetDirtyStatus(
    PDEVICE_EXTENSION DeviceExt,
    PBOOLEAN DirtyStatus)
{
    NTSTATUS Status;

    DPRINT("GetDirtyStatus(DeviceExt %p)\n", DeviceExt);

    /* FAT12 has no dirty bit */
    if (DeviceExt->FatInfo.FatType == FAT12)
    {
        *DirtyStatus = FALSE;
        return STATUS_SUCCESS;
    }

    /* Not really in the FAT, but share the lock because
     * we're really low-level and shouldn't happent that often
     * And call the appropriate function
     */
    ExAcquireResourceSharedLite(&DeviceExt->FatResource, TRUE);
    Status = DeviceExt->GetDirtyStatus(DeviceExt, DirtyStatus);
    ExReleaseResourceLite(&DeviceExt->FatResource);

    return Status;
}

NTSTATUS
FAT16GetDirtyStatus(
    PDEVICE_EXTENSION DeviceExt,
    PBOOLEAN DirtyStatus)
{
    LARGE_INTEGER Offset;
    ULONG Length;
#ifdef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
    NTSTATUS Status;
#else
    PVOID Context;
#endif
    struct _BootSector * Sector;

    /* We'll read the bootsector at 0 */
    Offset.QuadPart = 0;
    Length = DeviceExt->FatInfo.BytesPerSector;
#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
    /* Go through Cc for this */
    _SEH2_TRY
    {
        CcPinRead(DeviceExt->VolumeFcb->FileObject, &Offset, Length, PIN_WAIT, &Context, (PVOID *)&Sector);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;
#else
    /* No Cc, do it the old way:
     * - Allocate a big enough buffer
     * - And read the disk
     */
    Sector = ExAllocatePoolWithTag(NonPagedPool, Length, TAG_BUFFER);
    if (Sector == NULL)
    {
        *DirtyStatus = TRUE;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = VfatReadDisk(DeviceExt->StorageDevice, &Offset, Length, (PUCHAR)Sector, FALSE);
    if  (!NT_SUCCESS(Status))
    {
        *DirtyStatus = TRUE;
        ExFreePoolWithTag(Sector, TAG_BUFFER);
        return Status;
    }
#endif

    /* Make sure we have a boot sector...
     * FIXME: This check is a bit lame and should be improved
     */
    if (Sector->Signatur1 != 0xaa55)
    {
        /* Set we are dirty so that we don't attempt anything */
        *DirtyStatus = TRUE;
#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
        CcUnpinData(Context);
#else
        ExFreePoolWithTag(Sector, TAG_BUFFER);
#endif
        return STATUS_DISK_CORRUPT_ERROR;
    }

    /* Return the status of the dirty bit */
    if (Sector->Res1 & FAT_DIRTY_BIT)
        *DirtyStatus = TRUE;
    else
        *DirtyStatus = FALSE;

#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
    CcUnpinData(Context);
#else
    ExFreePoolWithTag(Sector, TAG_BUFFER);
#endif
    return STATUS_SUCCESS;
}

NTSTATUS
FAT32GetDirtyStatus(
    PDEVICE_EXTENSION DeviceExt,
    PBOOLEAN DirtyStatus)
{
    LARGE_INTEGER Offset;
    ULONG Length;
#ifdef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
    NTSTATUS Status;
#else
    PVOID Context;
#endif
    struct _BootSector32 * Sector;

    /* We'll read the bootsector at 0 */
    Offset.QuadPart = 0;
    Length = DeviceExt->FatInfo.BytesPerSector;
#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
    /* Go through Cc for this */
    _SEH2_TRY
    {
        CcPinRead(DeviceExt->VolumeFcb->FileObject, &Offset, Length, PIN_WAIT, &Context, (PVOID *)&Sector);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;
#else
    /* No Cc, do it the old way:
     * - Allocate a big enough buffer
     * - And read the disk
     */
    Sector = ExAllocatePoolWithTag(NonPagedPool, Length, TAG_BUFFER);
    if (Sector == NULL)
    {
        *DirtyStatus = TRUE;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = VfatReadDisk(DeviceExt->StorageDevice, &Offset, Length, (PUCHAR)Sector, FALSE);
    if  (!NT_SUCCESS(Status))
    {
        *DirtyStatus = TRUE;
        ExFreePoolWithTag(Sector, TAG_BUFFER);
        return Status;
    }
#endif

    /* Make sure we have a boot sector...
     * FIXME: This check is a bit lame and should be improved
     */
    if (Sector->Signature1 != 0xaa55)
    {
        /* Set we are dirty so that we don't attempt anything */
        *DirtyStatus = TRUE;
#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
        CcUnpinData(Context);
#else
        ExFreePoolWithTag(Sector, TAG_BUFFER);
#endif
        return STATUS_DISK_CORRUPT_ERROR;
    }

    /* Return the status of the dirty bit */
    if (Sector->Res4 & FAT_DIRTY_BIT)
        *DirtyStatus = TRUE;
    else
        *DirtyStatus = FALSE;

#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
    CcUnpinData(Context);
#else
    ExFreePoolWithTag(Sector, TAG_BUFFER);
#endif
    return STATUS_SUCCESS;
}

/*
 * FUNCTION: Set the dirty status
 */
NTSTATUS
SetDirtyStatus(
    PDEVICE_EXTENSION DeviceExt,
    BOOLEAN DirtyStatus)
{
    NTSTATUS Status;

    DPRINT("SetDirtyStatus(DeviceExt %p, DirtyStatus %d)\n", DeviceExt, DirtyStatus);

    /* FAT12 has no dirty bit */
    if (DeviceExt->FatInfo.FatType == FAT12)
    {
        return STATUS_SUCCESS;
    }

    /* Not really in the FAT, but share the lock because
     * we're really low-level and shouldn't happent that often
     * And call the appropriate function
     * Acquire exclusive because we will modify ondisk value
     */
    ExAcquireResourceExclusiveLite(&DeviceExt->FatResource, TRUE);
    Status = DeviceExt->SetDirtyStatus(DeviceExt, DirtyStatus);
    ExReleaseResourceLite(&DeviceExt->FatResource);

    return Status;
}

NTSTATUS
FAT16SetDirtyStatus(
    PDEVICE_EXTENSION DeviceExt,
    BOOLEAN DirtyStatus)
{
    LARGE_INTEGER Offset;
    ULONG Length;
#ifdef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
    NTSTATUS Status;
#else
    PVOID Context;
#endif
    struct _BootSector * Sector;

    /* We'll read (and then write) the bootsector at 0 */
    Offset.QuadPart = 0;
    Length = DeviceExt->FatInfo.BytesPerSector;
#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
    /* Go through Cc for this */
    _SEH2_TRY
    {
        CcPinRead(DeviceExt->VolumeFcb->FileObject, &Offset, Length, PIN_WAIT, &Context, (PVOID *)&Sector);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;
#else
    /* No Cc, do it the old way:
     * - Allocate a big enough buffer
     * - And read the disk
     */
    Sector = ExAllocatePoolWithTag(NonPagedPool, Length, TAG_BUFFER);
    if (Sector == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = VfatReadDisk(DeviceExt->StorageDevice, &Offset, Length, (PUCHAR)Sector, FALSE);
    if  (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Sector, TAG_BUFFER);
        return Status;
    }
#endif

    /* Make sure we have a boot sector...
     * FIXME: This check is a bit lame and should be improved
     */
    if (Sector->Signatur1 != 0xaa55)
    {
#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
        CcUnpinData(Context);
#else
        ExFreePoolWithTag(Sector, TAG_BUFFER);
#endif
        return STATUS_DISK_CORRUPT_ERROR;
    }

    /* Modify the dirty bit status according
     * to caller needs
     */
    if (!DirtyStatus)
    {
        Sector->Res1 &= ~FAT_DIRTY_BIT;
    }
    else
    {
        Sector->Res1 |= FAT_DIRTY_BIT;
    }

#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
    /* Mark boot sector dirty so that it gets written to the disk */
    CcSetDirtyPinnedData(Context, NULL);
    CcUnpinData(Context);
    return STATUS_SUCCESS;
#else
    /* Write back the boot sector to the disk */
    Status = VfatWriteDisk(DeviceExt->StorageDevice, &Offset, Length, (PUCHAR)Sector, FALSE);
    ExFreePoolWithTag(Sector, TAG_BUFFER);
    return Status;
#endif
}

NTSTATUS
FAT32SetDirtyStatus(
    PDEVICE_EXTENSION DeviceExt,
    BOOLEAN DirtyStatus)
{
    LARGE_INTEGER Offset;
    ULONG Length;
#ifdef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
    NTSTATUS Status;
#else
    PVOID Context;
#endif
    struct _BootSector32 * Sector;

    /* We'll read (and then write) the bootsector at 0 */
    Offset.QuadPart = 0;
    Length = DeviceExt->FatInfo.BytesPerSector;
#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
    /* Go through Cc for this */
    _SEH2_TRY
    {
        CcPinRead(DeviceExt->VolumeFcb->FileObject, &Offset, Length, PIN_WAIT, &Context, (PVOID *)&Sector);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;
#else
    /* No Cc, do it the old way:
     * - Allocate a big enough buffer
     * - And read the disk
     */
    Sector = ExAllocatePoolWithTag(NonPagedPool, Length, TAG_BUFFER);
    if (Sector == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = VfatReadDisk(DeviceExt->StorageDevice, &Offset, Length, (PUCHAR)Sector, FALSE);
    if  (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Sector, TAG_BUFFER);
        return Status;
    }
#endif

    /* Make sure we have a boot sector...
     * FIXME: This check is a bit lame and should be improved
     */
    if (Sector->Signature1 != 0xaa55)
    {
        ASSERT(FALSE);
#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
        CcUnpinData(Context);
#else
        ExFreePoolWithTag(Sector, TAG_BUFFER);
#endif
        return STATUS_DISK_CORRUPT_ERROR;
    }

    /* Modify the dirty bit status according
     * to caller needs
     */
    if (!DirtyStatus)
    {
        Sector->Res4 &= ~FAT_DIRTY_BIT;
    }
    else
    {
        Sector->Res4 |= FAT_DIRTY_BIT;
    }

#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
    /* Mark boot sector dirty so that it gets written to the disk */
    CcSetDirtyPinnedData(Context, NULL);
    CcUnpinData(Context);
    return STATUS_SUCCESS;
#else
    /* Write back the boot sector to the disk */
    Status = VfatWriteDisk(DeviceExt->StorageDevice, &Offset, Length, (PUCHAR)Sector, FALSE);
    ExFreePoolWithTag(Sector, TAG_BUFFER);
    return Status;
#endif
}

NTSTATUS
FAT32UpdateFreeClustersCount(
    PDEVICE_EXTENSION DeviceExt)
{
    LARGE_INTEGER Offset;
    ULONG Length;
#ifdef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
    NTSTATUS Status;
#else
    PVOID Context;
#endif
    struct _FsInfoSector * Sector;

    if (!DeviceExt->AvailableClustersValid)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* We'll read (and then write) the fsinfo sector */
    Offset.QuadPart = DeviceExt->FatInfo.FSInfoSector * DeviceExt->FatInfo.BytesPerSector;
    Length = DeviceExt->FatInfo.BytesPerSector;
#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
    /* Go through Cc for this */
    _SEH2_TRY
    {
        CcPinRead(DeviceExt->VolumeFcb->FileObject, &Offset, Length, PIN_WAIT, &Context, (PVOID *)&Sector);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;
#else
    /* No Cc, do it the old way:
     * - Allocate a big enough buffer
     * - And read the disk
     */
    Sector = ExAllocatePoolWithTag(NonPagedPool, Length, TAG_BUFFER);
    if (Sector == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = VfatReadDisk(DeviceExt->StorageDevice, &Offset, Length, (PUCHAR)Sector, FALSE);
    if  (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Sector, TAG_BUFFER);
        return Status;
    }
#endif

    /* Make sure we have a FSINFO sector */
    if (Sector->ExtBootSignature2 != 0x41615252 ||
        Sector->FSINFOSignature != 0x61417272 ||
        Sector->Signatur2 != 0xaa550000)
    {
        ASSERT(FALSE);
#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
        CcUnpinData(Context);
#else
        ExFreePoolWithTag(Sector, TAG_BUFFER);
#endif
        return STATUS_DISK_CORRUPT_ERROR;
    }

    /* Update the free clusters count */
    Sector->FreeCluster = InterlockedCompareExchange((PLONG)&DeviceExt->AvailableClusters, 0, 0);

#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
    /* Mark FSINFO sector dirty so that it gets written to the disk */
    CcSetDirtyPinnedData(Context, NULL);
    CcUnpinData(Context);
    return STATUS_SUCCESS;
#else
    /* Write back the FSINFO sector to the disk */
    Status = VfatWriteDisk(DeviceExt->StorageDevice, &Offset, Length, (PUCHAR)Sector, FALSE);
    ExFreePoolWithTag(Sector, TAG_BUFFER);
    return Status;
#endif
}

/* EOF */
