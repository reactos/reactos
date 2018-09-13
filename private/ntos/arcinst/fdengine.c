/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    fdengine.c

Abstract:

    This module contains the disk partitioning engine.  The code
    in this module can be compiled for either the NT platform
    or the ARC platform (-DARC).

Author:

    Ted Miller        (tedm)    Nov-1991

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define ARCDBG

// Attached disk devices.

ULONG              CountOfDisks;
PCHAR             *DiskNames;

// Information about attached disks.

DISKGEOM          *DiskGeometryArray;

PPARTITION        *PrimaryPartitions,
                  *LogicalVolumes;

//
// Array keeping track of whether each disk is off line.
//
PBOOLEAN           OffLine;

// Keeps track of whether changes have been made
// to each disk's partition structure.

BOOLEAN           *ChangesMade;


//
// Value used to indicate that the partition entry has changed but in a non-
// destructive way (ie, made active/inactive).
//
#define CHANGED_DONT_ZAP ((BOOLEAN)(5))

// forward declarations


ARC_STATUS
OpenDisks(
    VOID
    );

VOID
CloseDisks(
    VOID
    );

ARC_STATUS
GetGeometry(
    VOID
    );

BOOLEAN
CheckIfDiskIsOffLine(
    IN ULONG Disk
    );

ARC_STATUS
InitializePartitionLists(
    VOID
    );

ARC_STATUS
GetRegions(
    IN  ULONG               Disk,
    IN  PPARTITION          p,
    IN  BOOLEAN             WantUsedRegions,
    IN  BOOLEAN             WantFreeRegions,
    IN  BOOLEAN             WantLogicalRegions,
    OUT PREGION_DESCRIPTOR *Region,
    OUT ULONG              *RegionCount,
    IN  REGION_TYPE         RegionType
    );

BOOLEAN
AddRegionEntry(
    IN OUT PREGION_DESCRIPTOR *Regions,
    IN OUT ULONG              *RegionCount,
    IN     ULONG               SizeMB,
    IN     REGION_TYPE         RegionType,
    IN     PPARTITION          Partition,
    IN     LARGE_INTEGER       AlignedRegionOffset,
    IN     LARGE_INTEGER       AlignedRegionSize
    );

VOID
AddPartitionToLinkedList(
    IN PARTITION **Head,
    IN PARTITION *p
    );

BOOLEAN
IsInLinkedList(
    IN PPARTITION p,
    IN PPARTITION List
    );

BOOLEAN
IsInLogicalList(
    IN ULONG      Disk,
    IN PPARTITION p
    );

BOOLEAN
IsInPartitionList(
    IN ULONG      Disk,
    IN PPARTITION p
    );

LARGE_INTEGER
AlignTowardsDiskStart(
    IN ULONG         Disk,
    IN LARGE_INTEGER Offset
    );

LARGE_INTEGER
AlignTowardsDiskEnd(
    IN ULONG         Disk,
    IN LARGE_INTEGER Offset
    );

VOID
FreeLinkedPartitionList(
    IN PARTITION **q
    );

VOID
MergeFreePartitions(
    IN PPARTITION p
    );

VOID
RenumberPartitions(
    ULONG Disk
    );

VOID
FreePartitionInfoLinkedLists(
    IN PARTITION **ListHeadArray
    );

LARGE_INTEGER
DiskLengthBytes(
    IN ULONG Disk
    );

PPARTITION
AllocatePartitionStructure(
    IN ULONG         Disk,
    IN LARGE_INTEGER Offset,
    IN LARGE_INTEGER Length,
    IN UCHAR         SysID,
    IN BOOLEAN       Update,
    IN BOOLEAN       Active,
    IN BOOLEAN       Recognized
    );



ARC_STATUS
FdiskInitialize(
    VOID
    )

/*++

Routine Description:

    This routine initializes the partitioning engine, including allocating
    arrays, determining attached disk devices, and reading their
    partition tables.

Arguments:

    None.

Return Value:

    OK_STATUS or error code.

--*/

{
    ARC_STATUS status;
    ULONG        i;


    if((status = LowQueryFdiskPathList(&DiskNames,&CountOfDisks)) != OK_STATUS) {
        return(status);
    }

#if 0
#ifdef ARCDBG
    AlPrint("Disk count = %u\r\n",CountOfDisks);
    for(i=0; i<CountOfDisks; i++) {
        AlPrint("Disk %u: %s\r\n",i,DiskNames[i]);
    }
    WaitKey();
#endif
#endif

    DiskGeometryArray = NULL;
    PrimaryPartitions = NULL;
    LogicalVolumes = NULL;

    if(((DiskGeometryArray      = AllocateMemory(CountOfDisks * sizeof(DISKGEOM  ))) == NULL)
    || ((ChangesMade            = AllocateMemory(CountOfDisks * sizeof(BOOLEAN   ))) == NULL)
    || ((PrimaryPartitions      = AllocateMemory(CountOfDisks * sizeof(PPARTITION))) == NULL)
    || ((OffLine                = AllocateMemory(CountOfDisks * sizeof(BOOLEAN   ))) == NULL)
    || ((LogicalVolumes         = AllocateMemory(CountOfDisks * sizeof(PPARTITION))) == NULL))
    {
        RETURN_OUT_OF_MEMORY;
    }

    for(i=0; i<CountOfDisks; i++) {
        PrimaryPartitions[i] = NULL;
        LogicalVolumes[i] = NULL;
        ChangesMade[i] = FALSE;
        OffLine[i] = CheckIfDiskIsOffLine(i);
        if(OffLine[i]) {
            return(ENODEV);
        }
    }

    if(((status = GetGeometry()             ) != OK_STATUS)
    || ((status = InitializePartitionLists()) != OK_STATUS))
    {
        return(status);
    }

    return(OK_STATUS);
}


VOID
FdiskCleanUp(
    VOID
    )

/*++

Routine Description:

    This routine deallocates storage used by the partitioning engine.

Arguments:

    None.

Return Value:

    None.

--*/

{
    LowFreeFdiskPathList(DiskNames,CountOfDisks);

    if(DiskGeometryArray != NULL) {
        FreeMemory(DiskGeometryArray);
    }
    if(PrimaryPartitions != NULL) {
        FreePartitionInfoLinkedLists(PrimaryPartitions);
        FreeMemory(PrimaryPartitions);
    }
    if(LogicalVolumes != NULL) {
        FreePartitionInfoLinkedLists(LogicalVolumes);
        FreeMemory(LogicalVolumes);
    }
    if(ChangesMade != NULL) {
        FreeMemory(ChangesMade);
    }
    if(OffLine != NULL) {
        FreeMemory(OffLine);
    }
}


BOOLEAN
CheckIfDiskIsOffLine(
    IN ULONG Disk
    )

/*++

Routine Description:

    Determine whether a disk is off-line by attempting to open it.
    If this is diskman, also attempt to read from it.

Arguments:

    Disk - supplies number of the disk to check

Return Value:

    TRUE if disk is off-line, FALSE is disk is on-line.

--*/

{
    ULONG Handle;
    BOOLEAN IsOffLine;

    if(LowOpenDisk(GetDiskName(Disk),&Handle) == OK_STATUS) {

        IsOffLine = FALSE;
        LowCloseDisk(Handle);

    } else {

        IsOffLine = TRUE;
    }

    return(IsOffLine);
}


ARC_STATUS
GetGeometry(
    VOID
    )

/*++

Routine Description:

    This routine determines disk geometry for each disk device.
    Disk geometry includes heads, sectors per track, cylinder count,
    and bytes per sector.  It also includes bytes per track and
    bytes per cylinder, which are calculated from the other values
    for the convenience of the rest of this module.

    Geometry information is placed in the DiskGeometryArray global variable.

    Geometry information is undefined for an off-line disk.

Arguments:

    None.

Return Value:

    OK_STATUS or error code.

--*/

{
    ULONG       i;
    ARC_STATUS status;
    ULONG       TotalSectorCount,
                SectorSize,
                SectorsPerTrack,
                Heads;

    for(i=0; i<CountOfDisks; i++) {

        if(OffLine[i]) {
            continue;
        }

        if((status = LowGetDriveGeometry(DiskNames[i],&TotalSectorCount,&SectorSize,&SectorsPerTrack,&Heads)) != OK_STATUS) {
            return(status);
        }

        DiskGeometryArray[i].BytesPerSector   = SectorSize;
        DiskGeometryArray[i].SectorsPerTrack  = SectorsPerTrack;
        DiskGeometryArray[i].Heads            = Heads;
        DiskGeometryArray[i].Cylinders.QuadPart = TotalSectorCount / (SectorsPerTrack * Heads);
        DiskGeometryArray[i].BytesPerTrack    = SectorsPerTrack * SectorSize;
        DiskGeometryArray[i].BytesPerCylinder = SectorsPerTrack * SectorSize * Heads;
    }
    return(OK_STATUS);
}


#if i386
VOID
SetPartitionActiveFlag(
    IN PREGION_DESCRIPTOR Region,
    IN UCHAR              value
    )
{
    PPARTITION p = ((PREGION_DATA)Region->Reserved)->Partition;

    if((UCHAR)p->Active != value) {

        //
        // Unfortuneately, the Update flag becomes the RewritePartition flag
        // at commit time.  This causes us to zap the boot sector.  To avoid
        // this, we use a spacial non-boolean value that can be checked for
        // at commit time and that will cause us NOT to zap the bootsector
        // even though RewritePartition will be TRUE.
        //

        p->Active = value;
        if(!p->Update) {
            p->Update = CHANGED_DONT_ZAP;
        }
        ChangesMade[p->Disk] = TRUE;
    }
}
#endif


VOID
DetermineCreateSizeAndOffset(
    IN  PREGION_DESCRIPTOR Region,
    IN  LARGE_INTEGER      MinimumSize,
    IN  ULONG              CreationSizeMB,
    IN  REGION_TYPE        Type,
    OUT PLARGE_INTEGER     CreationStart,
    OUT PLARGE_INTEGER     CreationSize
    )

/*++

Routine Description:

    Determine the actual offset and size of the partition, given the
    size in megabytes.

Arguments:

    Region  - a region descriptor returned by GetDiskRegions().  Must
              be an unused region.

    MinimumSize - if non-0, this is the minimum size that the partition
        or logical drive can be.

    CreationSizeMB - If MinimumSize is 0, size of partition to create, in MB.

    Type    - REGION_PRIMARY, REGION_EXTENDED, or REGION_LOGICAL, for
              creating a primary partition, extended partition, or
              logical volume, respectively.

    CreationStart - receives the offset where the partition should be placed.

    CreationSize - receives the exact size for the partition.

Return Value:

    None.

--*/

{
    PREGION_DATA CreateData = Region->Reserved;
    LARGE_INTEGER CSize,CStart;
    LARGE_INTEGER Mod;
    ULONG  bpc = DiskGeometryArray[Region->Disk].BytesPerCylinder;
    ULONG  bpt = DiskGeometryArray[Region->Disk].BytesPerTrack;

    ASRT(Region->SysID == SYSID_UNUSED);

    //
    // If we are creating a partition at offset 0, adjust the aligned region
    // offset and the aligned region size, because no partition can actually
    // start at offset 0.
    //

    if(CreateData->AlignedRegionOffset.QuadPart == 0) {

        LARGE_INTEGER Delta;

        if(Type == REGION_EXTENDED) {

            Delta.QuadPart = bpc;

        } else {

            ASRT(Type == REGION_PRIMARY);
            Delta.QuadPart = bpt;
        }

        CreateData->AlignedRegionOffset = Delta;
        CreateData->AlignedRegionSize.QuadPart = (CreateData->AlignedRegionSize.QuadPart - Delta.QuadPart);
    }

    CStart = CreateData->AlignedRegionOffset;
    if(MinimumSize.QuadPart == 0) {
        CSize.QuadPart = UInt32x32To64(CreationSizeMB,ONE_MEG);
    } else {
        CSize = MinimumSize;
        if(Type == REGION_LOGICAL) {
            CSize.QuadPart = CSize.QuadPart + bpt;
        }
    }

    //
    // Decide whether to align the ending cylinder up or down.
    // If the offset of end of the partition is more than half way into the
    // final cylinder, align towrds the disk end.  Otherwise align toward
    // the disk start.
    //

    Mod.QuadPart = (CStart.QuadPart + CSize.QuadPart) % bpc;

    if(Mod.QuadPart != 0) {

        if((MinimumSize.QuadPart != 0) || (Mod.QuadPart > bpc/2)) {
            CSize.QuadPart = (CSize.QuadPart + (bpc - Mod.QuadPart));
        } else {
            CSize.QuadPart = (CSize.QuadPart - Mod.QuadPart);        // snap downwards tp cyl boundary
        }
    }

    if(CSize.QuadPart > CreateData->AlignedRegionSize.QuadPart) {

        //
        // Space available in the free space isn't large enough to accomodate
        // the request in its entirety;  just use the entire free space.
        //

        CSize  = CreateData->AlignedRegionSize;
    }

    ASRT(CStart.QuadPart != 0);
    ASRT(CSize.QuadPart != 0);

    *CreationStart = CStart;
    *CreationSize  = CSize;
}


ARC_STATUS
CreatePartitionEx(
    IN PREGION_DESCRIPTOR Region,
    IN LARGE_INTEGER      MinimumSize,
    IN ULONG              CreationSizeMB,
    IN REGION_TYPE        Type,
    IN UCHAR              SysId
    )

/*++

Routine Description:

    This routine creates a partition from a free region on the disk.  The
    partition is always created at the beginning of the free space, and any
    left over space at the end is kept on the free space list.

Arguments:

    Region  - a region descriptor returned by GetDiskRegions().  Must
              be an unused region.

    CreationSizeMB - size of partition to create, in MB.

    Type    - REGION_PRIMARY, REGION_EXTENDED, or REGION_LOGICAL, for
              creating a primary partition, extended pasrtition, or
              logical volume, respectively.

    SysId - system ID byte to be assigned to the partition

Return Value:

    OK_STATUS or error code.

--*/

{
    PPARTITION    p1,p2,p3;
    PREGION_DATA  CreateData = Region->Reserved;
    LARGE_INTEGER CreationStart,CreationSize,LeftOver;
    PPARTITION   *PartitionList;

    ASRT(Region->SysID == SYSID_UNUSED);

    DetermineCreateSizeAndOffset( Region,
                                  MinimumSize,
                                  CreationSizeMB,
                                  Type,
                                  &CreationStart,
                                  &CreationSize
                                );

    //
    // now we've got the start and size of the partition to be created.
    // If there's left-over at the beginning of the free space (after
    // alignment), make a new PARTITION structure.

    p1 = NULL;
    LeftOver.QuadPart = (CreationStart.QuadPart - CreateData->Partition->Offset.QuadPart);

    if(LeftOver.QuadPart > 0) {

        p1 = AllocatePartitionStructure(Region->Disk,
                                        CreateData->Partition->Offset,
                                        LeftOver,
                                        SYSID_UNUSED,
                                        FALSE,
                                        FALSE,
                                        FALSE
                                       );
        if(p1 == NULL) {
            RETURN_OUT_OF_MEMORY;
        }
    }

    // make a new partition structure for space being left free as
    // a result of this creation.

    p2 = NULL;
    LeftOver.QuadPart = (CreateData->Partition->Offset.QuadPart + CreateData->Partition->Length.QuadPart) -
                        (CreationStart.QuadPart + CreationSize.QuadPart);

    if(LeftOver.QuadPart != 0) {

        LARGE_INTEGER   TmpResult;

        TmpResult.QuadPart = CreationStart.QuadPart + CreationSize.QuadPart;
        p2 = AllocatePartitionStructure(Region->Disk,
                                        TmpResult,
                                        LeftOver,
                                        SYSID_UNUSED,
                                        FALSE,
                                        FALSE,
                                        FALSE
                                       );
        if(p2 == NULL) {
            RETURN_OUT_OF_MEMORY;
        }
    }

    // adjust the free partition's fields.

    CreateData->Partition->Offset = CreationStart;
    CreateData->Partition->Length = CreationSize;
    CreateData->Partition->SysID  = SysId;
    CreateData->Partition->Update = TRUE;
    CreateData->Partition->Recognized = TRUE;

    // if we just created an extended partition, show the whole thing
    // as one free logical region.

    if(Type == REGION_EXTENDED) {

        ASRT(LogicalVolumes[Region->Disk] == NULL);

        p3 = AllocatePartitionStructure(Region->Disk,
                                        CreationStart,
                                        CreationSize,
                                        SYSID_UNUSED,
                                        FALSE,
                                        FALSE,
                                        FALSE
                                       );
        if(p3 == NULL) {
            RETURN_OUT_OF_MEMORY;
        }
        AddPartitionToLinkedList(&LogicalVolumes[Region->Disk],p3);
    }

    PartitionList = (Type == REGION_LOGICAL)
                  ? &LogicalVolumes[Region->Disk]
                  : &PrimaryPartitions[Region->Disk];

    if(p1) {
        AddPartitionToLinkedList(PartitionList,p1);
    }
    if(p2) {
        AddPartitionToLinkedList(PartitionList,p2);
    }

    MergeFreePartitions(*PartitionList);
    RenumberPartitions(Region->Disk);

    ChangesMade[Region->Disk] = TRUE;

    return(OK_STATUS);
}


ARC_STATUS
CreatePartition(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              CreationSizeMB,
    IN REGION_TYPE        Type
    )
{
    LARGE_INTEGER   LargeZero;

    LargeZero.QuadPart = 0;
    return(CreatePartitionEx(Region,
                             LargeZero,
                             CreationSizeMB,
                             Type,
                             (UCHAR)((Type == REGION_EXTENDED) ? SYSID_EXTENDED
                                                               : SYSID_BIGFAT
                                    )
                            )
          );
}



ARC_STATUS
DeletePartition(
    IN PREGION_DESCRIPTOR Region
    )

/*++

Routine Description:

    This routine deletes a partition, returning its space to the
    free space on the disk.  If deleting the extended partition,
    all logical volumes within it are also deleted.

Arguments:

    Region  - a region descriptor returned by GetDiskRegions().  Must
              be a used region.

Return Value:

    OK_STATUS or error code.

--*/

{
    PREGION_DATA  RegionData = Region->Reserved;
    PPARTITION   *PartitionList;

    ASRT(    IsInPartitionList(Region->Disk,RegionData->Partition)
          || IsInLogicalList  (Region->Disk,RegionData->Partition)
        );

    if(IsExtended(Region->SysID)) {

        ASRT(IsInPartitionList(Region->Disk,RegionData->Partition));

        // Deleting extended partition.  Also delete all logical volumes.

        FreeLinkedPartitionList(&LogicalVolumes[Region->Disk]);
    }

    RegionData->Partition->SysID  = SYSID_UNUSED;
    RegionData->Partition->Update = TRUE;
    RegionData->Partition->Active = FALSE;
    RegionData->Partition->OriginalPartitionNumber = 0;

    PartitionList = (Region->RegionType == REGION_LOGICAL)
                  ? &LogicalVolumes[Region->Disk]
                  : &PrimaryPartitions[Region->Disk];

    MergeFreePartitions(*PartitionList);
    RenumberPartitions(Region->Disk);

    ChangesMade[Region->Disk] = TRUE;

    return(OK_STATUS);
}


ARC_STATUS
GetDiskRegions(
    IN  ULONG               Disk,
    IN  BOOLEAN             WantUsedRegions,
    IN  BOOLEAN             WantFreeRegions,
    IN  BOOLEAN             WantPrimaryRegions,
    IN  BOOLEAN             WantLogicalRegions,
    OUT PREGION_DESCRIPTOR *Region,
    OUT ULONG              *RegionCount
    )

/*++

Routine Description:

    This routine returns an array of region descriptors to the caller.
    A region desscriptor describes a space on the disk, either used
    or free.  The caller can control which type of regions are returned.

    The caller must free the returned array via FreeRegionArray().

Arguments:

    Disk            - index of disk whose regions are to be returned

    WantUsedRegions - whether to return used disk regions

    WantFreeRegions - whether to return free disk regions

    WantPrimaryRegions - whether to return regions not in the
                         extended partition

    WantLogicalRegions - whether to return regions within the
                         extended partition

    Region          - where to put a pointer to the array of regions

    RegionCount     - where to put the number of items in the returned
                      Region array

Return Value:

    OK_STATUS or error code.

--*/

{
    *Region = AllocateMemory(0);
    *RegionCount = 0;

    if(WantPrimaryRegions) {
        return(GetRegions(Disk,
                          PrimaryPartitions[Disk],
                          WantUsedRegions,
                          WantFreeRegions,
                          WantLogicalRegions,
                          Region,
                          RegionCount,
                          REGION_PRIMARY
                         )
              );
    } else if(WantLogicalRegions) {
        return(GetRegions(Disk,
                          LogicalVolumes[Disk],
                          WantUsedRegions,
                          WantFreeRegions,
                          FALSE,
                          Region,
                          RegionCount,
                          REGION_LOGICAL
                         )
              );
    }
    return(OK_STATUS);
}


// workers for GetDiskRegions

ARC_STATUS
GetRegions(
    IN  ULONG               Disk,
    IN  PPARTITION          p,
    IN  BOOLEAN             WantUsedRegions,
    IN  BOOLEAN             WantFreeRegions,
    IN  BOOLEAN             WantLogicalRegions,
    OUT PREGION_DESCRIPTOR *Region,
    OUT ULONG              *RegionCount,
    IN  REGION_TYPE         RegionType
    )
{
    ARC_STATUS   status;
    LARGE_INTEGER AlignedOffset,AlignedSize;
    ULONG         SizeMB;

    while(p) {

        if(p->SysID == SYSID_UNUSED) {

            if(WantFreeRegions) {

                LARGE_INTEGER   Result1, Result2;

                AlignedOffset = AlignTowardsDiskEnd(p->Disk,p->Offset);

                Result2.QuadPart = p->Offset.QuadPart + p->Length.QuadPart;
                Result1 = AlignTowardsDiskStart(p->Disk,Result2);
                AlignedSize.QuadPart   = Result1.QuadPart - AlignedOffset.QuadPart;

                SizeMB        = SIZEMB(AlignedSize);


                //
                // Show the space free if it is greater than 1 meg, AND
                // it is not a space starting at the beginning of the disk
                // and of length <= 1 cylinder.
                // This prevents the user from seeing the first cylinder
                // of the disk as free (could otherwise happen with an
                // extended partition starting on cylinder 1 and cylinders
                // of 1 megabyte or larger).
                //

                if(    (AlignedSize.QuadPart > 0)
                    && SizeMB
                    && (    (p->Offset.QuadPart != 0)
                         || ( p->Length.QuadPart >
                              DiskGeometryArray[p->Disk].BytesPerCylinder )
                       )
                  )
                {
                    if(!AddRegionEntry(Region,
                                       RegionCount,
                                       SizeMB,
                                       RegionType,
                                       p,
                                       AlignedOffset,
                                       AlignedSize
                                      ))
                    {
                        RETURN_OUT_OF_MEMORY;
                    }
                }
            }
        } else {

            if(WantUsedRegions) {

                AlignedOffset = p->Offset;
                AlignedSize   = p->Length;
                SizeMB        = SIZEMB(AlignedSize);

                if(!AddRegionEntry(Region,
                                   RegionCount,
                                   SizeMB,
                                   RegionType,
                                   p,
                                   AlignedOffset,
                                   AlignedSize
                                  ))
                {
                    RETURN_OUT_OF_MEMORY;
                }
            }

            if(IsExtended(p->SysID) && WantLogicalRegions) {
                status = GetRegions(Disk,
                                    LogicalVolumes[Disk],
                                    WantUsedRegions,
                                    WantFreeRegions,
                                    FALSE,
                                    Region,
                                    RegionCount,
                                    REGION_LOGICAL
                                   );
                if(status != OK_STATUS) {
                    return(status);
                }
            }
        }
        p = p->Next;
    }
    return(OK_STATUS);
}


BOOLEAN
AddRegionEntry(
    OUT PREGION_DESCRIPTOR *Regions,
    OUT ULONG              *RegionCount,
    IN  ULONG               SizeMB,
    IN  REGION_TYPE         RegionType,
    IN  PPARTITION          Partition,
    IN  LARGE_INTEGER       AlignedRegionOffset,
    IN  LARGE_INTEGER       AlignedRegionSize
    )
{
    PREGION_DESCRIPTOR p;
    PREGION_DATA       data;

    p = ReallocateMemory(*Regions,((*RegionCount) + 1) * sizeof(REGION_DESCRIPTOR));
    if(p == NULL) {
        return(FALSE);
    } else {
        *Regions = p;
        (*RegionCount)++;
    }

    p = &(*Regions)[(*RegionCount)-1];

    if(!(p->Reserved = AllocateMemory(sizeof(REGION_DATA)))) {
        return(FALSE);
    }

    p->Disk                    = Partition->Disk;
    p->SysID                   = Partition->SysID;
    p->SizeMB                  = SizeMB;
    p->Active                  = Partition->Active;
    p->Recognized              = Partition->Recognized;
    p->PartitionNumber         = Partition->PartitionNumber;
    p->OriginalPartitionNumber = Partition->OriginalPartitionNumber;
    p->RegionType              = RegionType;
    p->PersistentData          = Partition->PersistentData;

    data = p->Reserved;

    data->Partition             = Partition;
    data->AlignedRegionOffset   = AlignedRegionOffset;
    data->AlignedRegionSize     = AlignedRegionSize;

    return(TRUE);
}


VOID
FreeRegionArray(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              RegionCount
    )

/*++

Routine Description:

    This routine frees a region array returned by GetDiskRegions().

Arguments:

    Region          - pointer to the array of regions to be freed

    RegionCount     - number of items in the Region array

Return Value:

    None.

--*/

{
    ULONG i;

    for(i=0; i<RegionCount; i++) {

        if(Region[i].Reserved) {
            FreeMemory(Region[i].Reserved);
        }
    }
    FreeMemory(Region);
}



VOID
AddPartitionToLinkedList(
    IN OUT PARTITION **Head,
    IN     PARTITION *p
    )

/*++

Routine Description:

    This routine adds a PARTITION structure to a doubly-linked
    list, sorted by the Offset field in ascending order.

Arguments:

    Head    - pointer to pointer to first element in list

    p       - pointer to item to be added to list

Return Value:

    None.

--*/

{
    PARTITION *cur,*prev;

    if((cur = *Head) == NULL) {
        *Head = p;
        return;
    }

    if(p->Offset.QuadPart < cur->Offset.QuadPart) {
        p->Next = cur;
        cur->Prev = p;
        *Head = p;
        return;
    }

    prev = *Head;
    cur = cur->Next;

    while(cur) {
        if(p->Offset.QuadPart < cur->Offset.QuadPart) {

            p->Next = cur;
            p->Prev = prev;
            prev->Next = p;
            cur->Prev = p;
            return;
        }
        prev = cur;
        cur = cur->Next;
    }

    prev->Next = p;
    p->Prev = prev;
    return;
}


BOOLEAN
IsInLinkedList(
    IN PPARTITION p,
    IN PPARTITION List
    )

/*++

Routine Description:

    This routine determines whether a PARTITION element is in
    a given linked list of PARTITION elements.

Arguments:

    p       - pointer to element to be checked for

    List    - first element in list to be scanned

Return Value:

    true if p found in List, false otherwise

--*/

{
    while(List) {
        if(p == List) {
            return(TRUE);
        }
        List = List->Next;
    }
    return(FALSE);
}


BOOLEAN
IsInLogicalList(
    IN ULONG      Disk,
    IN PPARTITION p
    )

/*++

Routine Description:

    This routine determines whether a PARTITION element is in
    the logical volume list for a given disk.

Arguments:

    Disk    - index of disk to be checked

    p       - pointer to element to be checked for

Return Value:

    true if p found in Disk's logical volume list, false otherwise

--*/

{
    return(IsInLinkedList(p,LogicalVolumes[Disk]));
}


BOOLEAN
IsInPartitionList(
    IN ULONG      Disk,
    IN PPARTITION p
    )

/*++

Routine Description:

    This routine determines whether a PARTITION element is in
    the primary partition list for a given disk.

Arguments:

    Disk    - index of disk to be checked

    p       - pointer to element to be checked for

Return Value:

    true if p found in Disk's primary partition list, false otherwise

--*/

{
    return(IsInLinkedList(p,PrimaryPartitions[Disk]));
}


VOID
MergeFreePartitions(
    IN PPARTITION p
    )

/*++

Routine Description:

    This routine merges adjacent free space elements in the
    given linked list of PARTITION elements.  It is designed
    to be called after adding or deleting a partition.

Arguments:

    p - pointer to first item in list whose free elements are to
        be merged.

Return Value:

    None.

--*/

{
    PPARTITION next;

    while(p && p->Next) {

        if((p->SysID == SYSID_UNUSED) && (p->Next->SysID == SYSID_UNUSED)) {

            next = p->Next;

            p->Length.QuadPart = (next->Offset.QuadPart + next->Length.QuadPart) - p->Offset.QuadPart;

            if(p->Next = next->Next) {
                next->Next->Prev = p;
            }

            FreeMemory(next);

        } else {
            p = p->Next;
        }
    }
}


VOID
RenumberPartitions(
    IN ULONG Disk
    )

/*++

Routine Description:

    This routine determines the partition number for each region
    on a disk.  For a used region, the partition number is the number
    that the system will assign to the partition.  All partitions
    (except the extended partition) are numbered first starting at 1,
    and then all logical volumes in the extended partition.
    For a free region, the partition number is the number that the
    system WOULD assign to the partition if the space were to be
    converted to a partition and all other regions on the disk were
    left as is.

    The partition numbers are stored in the PARTITION elements.

Arguments:

    Disk - index of disk whose partitions are to be renumbered.

Return Value:

    None.

--*/

{
    PPARTITION p = PrimaryPartitions[Disk];
    ULONG      n = 1;

    while(p) {
        if(!IsExtended(p->SysID)) {
            p->PartitionNumber = n;
            if(p->SysID != SYSID_UNUSED) {
                n++;
            }
        }
        p = p->Next;
    }
    p = LogicalVolumes[Disk];
    while(p) {
        p->PartitionNumber = n;
        if(p->SysID != SYSID_UNUSED) {
            n++;
        }
        p = p->Next;
    }
}


PPARTITION
FindPartitionElement(
    IN ULONG Disk,
    IN ULONG Partition
    )

/*++

Routine Description:

    This routine locates a PARTITION element for a disk/partition
    number pair.  The partition number is the number that the
    system assigns to the partition.

Arguments:

    Disk - index of relevent disk

    Partition - partition number of partition to find

Return Value:

    pointer to PARTITION element, or NULL if not found.

--*/

{
    PPARTITION p;

    ASRT(Partition);

    p = PrimaryPartitions[Disk];
    while(p) {
        if((p->SysID != SYSID_UNUSED)
        && !IsExtended(p->SysID)
        && (p->PartitionNumber == Partition))
        {
            return(p);
        }
        p = p->Next;
    }
    p = LogicalVolumes[Disk];
    while(p) {
        if((p->SysID != SYSID_UNUSED)
        && (p->PartitionNumber == Partition))
        {
            return(p);
        }
        p = p->Next;
    }
    return(NULL);
}


VOID
SetSysID(
    IN ULONG Disk,
    IN ULONG Partition,
    IN UCHAR SysID
    )

/*++

Routine Description:

    This routine sets the system id of the given partition
    on the given disk.

Arguments:

    Disk - index of relevent disk

    Partition - partition number of relevent partition

    SysID - new system ID for Partition on Disk

Return Value:

    None.

--*/

{
    PPARTITION p = FindPartitionElement(Disk,Partition);

    ASRT(p);

    if(p) {
        p->SysID = SysID;
        if(!p->Update) {
            p->Update = CHANGED_DONT_ZAP;
        }
        ChangesMade[p->Disk] = TRUE;
    }
}


VOID
SetSysID2(
    IN PREGION_DESCRIPTOR Region,
    IN UCHAR              SysID
    )
{
    PPARTITION p = ((PREGION_DATA)(Region->Reserved))->Partition;

    p->SysID = SysID;
    if(!p->Update) {
        p->Update = CHANGED_DONT_ZAP;
    }
    ChangesMade[p->Disk] = TRUE;
}


ULONG
GetHiddenSectorCount(
    IN ULONG Disk,
    IN ULONG Partition
    )

/*++

Routine Description:

    This routine determines the hidden sector count for a
    partition.  This value is used by format and is placed in
    the BPB of a FAT partition when the partition is formatted.

Arguments:

    Disk - index of relevent disk

    Partition - partition number of relevent partition

Return Value:

    The number of hidden sectors (will be 0 if the partition
    does not exist).

--*/

{
    PPARTITION p = FindPartitionElement(Disk,Partition);
    ULONG      HiddenSectorCount = 0;
    LARGE_INTEGER   Result;

    ASRT(p);

    if(p) {

        if(IsInLogicalList(Disk,p)) {
            HiddenSectorCount = DiskGeometryArray[Disk].SectorsPerTrack;
        } else {
            ASRT(IsInPartitionList(Disk,p));
            Result.QuadPart = p->Offset.QuadPart / DiskGeometryArray[Disk].BytesPerSector;
            HiddenSectorCount = LOWPART(Result);
        }
    }
    return(HiddenSectorCount);
}


VOID
FreeLinkedPartitionList(
    IN OUT PPARTITION *q
    )

/*++

Routine Description:

    This routine frees a linked list of PARTITION elements. The head
    pointer is set to NULL.

Arguments:

    p - pointer to pointer to first element of list to free.

Return Value:

    None.

--*/

{
    PARTITION *n;
    PARTITION *p = *q;

    while(p) {
        n = p->Next;
        FreeMemory(p);
        p = n;
    }
    *q = NULL;
}


VOID
FreePartitionInfoLinkedLists(
    IN PPARTITION *ListHeadArray
    )

/*++

Routine Description:

    This routine frees the linked lists of PARTITION elements
    for each disk.

Arguments:

    ListHeadArray - pointer to array of pointers to first elements of
                    PARTITION element lists.

Return Value:

    None.

--*/

{
    ULONG i;

    for(i=0; i<CountOfDisks; i++) {

        FreeLinkedPartitionList(&ListHeadArray[i]);
    }
}


PPARTITION
AllocatePartitionStructure(
    IN ULONG         Disk,
    IN LARGE_INTEGER Offset,
    IN LARGE_INTEGER Length,
    IN UCHAR         SysID,
    IN BOOLEAN       Update,
    IN BOOLEAN       Active,
    IN BOOLEAN       Recognized
    )

/*++

Routine Description:

    This routine allocates space for, and initializes a PARTITION
    structure.

Arguments:

    Disk    - index of disk, one of whose regions the new PARTITION
              strucure describes.

    Offset  - byte offset of region on the disk

    Length  - length in bytes of the region

    SysID   - system id of region, of SYSID_UNUSED of this PARTITION
              is actually a free space.

    Update  - whether this PARTITION is dirty, ie, has changed and needs
              to be written to disk.

    Active  - flag for the BootIndicator field in a partition table entry,
              indicates to the x86 master boot program which partition
              is active.

    Recognized - whether the partition is a type recognized by NT

Return Value:

    NULL if allocation failed, or new initialized PARTITION strucure.

--*/

{
    PPARTITION p = AllocateMemory(sizeof(PARTITION));

    if(p) {
        p->Next                    = NULL;
        p->Prev                    = NULL;
        p->Offset                  = Offset;
        p->Length                  = Length;
        p->Disk                    = Disk;
        p->Update                  = Update;
        p->Active                  = Active;
        p->Recognized              = Recognized;
        p->SysID                   = SysID;
        p->OriginalPartitionNumber = 0;
        p->PartitionNumber         = 0;
        p->PersistentData          = 0;
    }
    return(p);
}


ARC_STATUS
InitializeFreeSpace(
    IN ULONG             Disk,
    IN PPARTITION       *PartitionList,      // list the free space goes in
    IN LARGE_INTEGER     StartOffset,
    IN LARGE_INTEGER     Length
    )

/*++

Routine Description:

    This routine determines all the free spaces within a given area
    on a disk, allocates PARTITION structures to describe them,
    and adds these structures to the relevent partition list
    (primary partitions or logical volumes).

    No rounding or alignment is performed here.  Spaces of even one
    byte will be counted and inserted in the partition list.

Arguments:

    Disk    - index of disk whose free spaces are being sought.

    PartitionList - pointer to first element on PARTITION list that
                    the free spaces will go in.

    StartOffset - start offset of area on disk to consider (ie, 0 for
                  primary spaces or the first byte of the extended
                  partition for logical spaces).

    Length - length of area on disk to consider (ie, size of disk
             for primary spaces or size of extended partition for
             logical spaces).

Return Value:

    OK_STATUS or error code.

--*/

{
    PPARTITION              p = *PartitionList,q;
    LARGE_INTEGER           Start,Size;

    Start = StartOffset;

    while(p) {

        Size.QuadPart = p->Offset.QuadPart - Start.QuadPart;

        if(Size.QuadPart > 0) {

            if(!(q = AllocatePartitionStructure(Disk,
                                                Start,
                                                Size,
                                                SYSID_UNUSED,
                                                FALSE,
                                                FALSE,
                                                FALSE
                                               )
                )
              )
            {
                RETURN_OUT_OF_MEMORY;
            }

            AddPartitionToLinkedList(PartitionList,q);
        }

        Start.QuadPart = p->Offset.QuadPart + p->Length.QuadPart;

        p = p->Next;

    }

    Size.QuadPart = (StartOffset.QuadPart + Length.QuadPart) - Start.QuadPart;

    if(Size.QuadPart > 0) {

        if(!(q = AllocatePartitionStructure(Disk,
                                            Start,
                                            Size,
                                            SYSID_UNUSED,
                                            FALSE,
                                            FALSE,
                                            FALSE
                                           )
            )
          )
        {
            RETURN_OUT_OF_MEMORY;
        }

        AddPartitionToLinkedList(PartitionList,q);
    }

    return(OK_STATUS);
}


ARC_STATUS
InitializeLogicalVolumeList(
    IN ULONG                      Disk,
    IN PDRIVE_LAYOUT_INFORMATION  DriveLayout,
    IN ULONG                      PartitionNumber
    )

/*++

Routine Description:

    This routine creates the logical volume linked list of
    PARTITION structures for the given disk.

Arguments:

    Disk    - index of disk

    DriveLayout - pointer to structure describing the raw partition
                  layout of the disk.

    PartitionNumber - number to assign to the first logical volume
                      on the disk.

Return Value:

    OK_STATUS or error code.

--*/

{
    PPARTITION             p,q;
    ULONG                   i,j;
    PPARTITION_INFORMATION d;
    LARGE_INTEGER          HiddenBytes;
    ULONG                  BytesPerSector = DiskGeometryArray[Disk].BytesPerSector;
    LARGE_INTEGER          Result1, Result2;

    FreeLinkedPartitionList(&LogicalVolumes[Disk]);

    p = PrimaryPartitions[Disk];
    while(p) {
        if(IsExtended(p->SysID)) {
            break;
        }
        p = p->Next;
    }

    if(p) {
        for(i=ENTRIES_PER_BOOTSECTOR; i<DriveLayout->PartitionCount; i+=ENTRIES_PER_BOOTSECTOR) {

            for(j=i; j<i+ENTRIES_PER_BOOTSECTOR; j++) {

                d = &DriveLayout->PartitionEntry[j];

                if((d->PartitionType != SYSID_UNUSED) && (d->PartitionType != SYSID_EXTENDED)) {

                    HiddenBytes.QuadPart = UInt32x32To64(d->HiddenSectors,BytesPerSector);

                    Result1.QuadPart = d->StartingOffset.QuadPart - HiddenBytes.QuadPart;
                    Result2.QuadPart = d->PartitionLength.QuadPart + HiddenBytes.QuadPart;
                    if(!(q = AllocatePartitionStructure(Disk,
                                                        Result1,
                                                        Result2,
                                                        d->PartitionType,
                                                        FALSE,
                                                        d->BootIndicator,
                                                        d->RecognizedPartition
                                                       )
                        )
                      )
                    {
                        RETURN_OUT_OF_MEMORY;
                    }

                    q->OriginalPartitionNumber = PartitionNumber++;
                    AddPartitionToLinkedList(&LogicalVolumes[Disk],q);

                    break;
                }
            }
        }
        return(InitializeFreeSpace(Disk,
                                   &LogicalVolumes[Disk],
                                   p->Offset,
                                   p->Length
                                  )
              );
    }
    return(OK_STATUS);
}


ARC_STATUS
InitializePrimaryPartitionList(
    IN  ULONG                     Disk,
    IN  PDRIVE_LAYOUT_INFORMATION DriveLayout,
    OUT PULONG                    NextPartitionNumber
    )

/*++

Routine Description:

    This routine creates the primary partition linked list of
    PARTITION structures for the given disk.

Arguments:

    Disk    - index of disk

    DriveLayout - pointer to structure describing the raw partition
                  layout of the disk.

    NextPartitionNumber - where to put partition number that should be
                          assigned to the first logical volume on the
                          disk.

Return Value:

    OK_STATUS or error code.

--*/

{
    ULONG                  i;
    PPARTITION             p;
    PPARTITION_INFORMATION d;
    ULONG                  PartitionNumber = 1;
    LARGE_INTEGER          LargeZero;

    FreeLinkedPartitionList(&PrimaryPartitions[Disk]);

    if(DriveLayout->PartitionCount >= ENTRIES_PER_BOOTSECTOR) {

        for(i=0; i<ENTRIES_PER_BOOTSECTOR; i++) {

            d = &DriveLayout->PartitionEntry[i];

            if(d->PartitionType != SYSID_UNUSED) {

                if(!(p = AllocatePartitionStructure(Disk,
                                                    d->StartingOffset,
                                                    d->PartitionLength,
                                                    d->PartitionType,
                                                    FALSE,
                                                    d->BootIndicator,
                                                    d->RecognizedPartition
                                                   )
                    )
                  )
                {
                    RETURN_OUT_OF_MEMORY;
                }

                p->OriginalPartitionNumber = IsExtended(p->SysID)
                                           ? 0
                                           : PartitionNumber++;

                AddPartitionToLinkedList(&PrimaryPartitions[Disk],p);
            }
        }
    }
    *NextPartitionNumber = PartitionNumber;
    LargeZero.QuadPart = 0;
    return(InitializeFreeSpace(Disk,
                               &PrimaryPartitions[Disk],
                               LargeZero,
                               DiskLengthBytes(Disk)
                              )
          );
}


ARC_STATUS
InitializePartitionLists(
    VOID
    )

/*++

Routine Description:

    This routine scans the PARTITION_INFO array returned for each disk
    by the OS.  A linked list of PARTITION structures is layered on top
    of each array;  the net result is a sorted list that covers an entire
    disk, because free spaces are also factored in as 'dummy' partitions.

Arguments:

    None.

Return Value:

    OK_STATUS or error code.

--*/

{
    ARC_STATUS               status;
    ULONG                     Disk;
    PDRIVE_LAYOUT_INFORMATION DriveLayout;
    ULONG                     PNum;


    for(Disk=0; Disk<CountOfDisks; Disk++) {

        if(OffLine[Disk]) {
            continue;
        }

        if((status = LowGetDiskLayout(DiskNames[Disk],&DriveLayout)) != OK_STATUS) {
            return(status);
        }

        if((status = InitializePrimaryPartitionList(Disk,DriveLayout,&PNum)) != OK_STATUS) {
            FreeMemory(DriveLayout);
            return(status);
        }
        if((status = InitializeLogicalVolumeList(Disk,DriveLayout,PNum)) != OK_STATUS) {
            FreeMemory(DriveLayout);
            return(status);
        }

        FreeMemory(DriveLayout);
        RenumberPartitions(Disk);
    }
    return(OK_STATUS);
}



LARGE_INTEGER
DiskLengthBytes(
    IN ULONG Disk
    )

/*++

Routine Description:

    This routine determines the disk length in bytes.  This value
    is calculated from the disk geometry information.

Arguments:

    Disk - index of disk whose size is desired

Return Value:

    Size of Disk.

--*/

{
    LARGE_INTEGER l;

    l.QuadPart = (ULONGLONG)DiskGeometryArray[Disk].Cylinders.QuadPart
               * (ULONGLONG)DiskGeometryArray[Disk].BytesPerCylinder;

    return(l);
}


ULONG
SIZEMB(
    IN LARGE_INTEGER ByteCount
    )

/*++

Routine Description:

    Calculate the size in megabytes of a given byte count. The value is
    properly rounded (ie, not merely truncated).

    This function replaces a macro of the same name that was truncating
    instead of rounding.

Arguments:

    ByteCount - supplies number of bytes

Return Value:

    Size in MB equivalent to ByteCount.

--*/

{
    ULONG Remainder;
    ULONG SizeMB;

    SizeMB = (ULONG)((ULONGLONG)ByteCount.QuadPart / (ULONGLONG)ONE_MEG);
    Remainder = (ULONG)((ULONGLONG)ByteCount.QuadPart % (ULONGLONG)ONE_MEG);

    if(Remainder >= ONE_MEG/2) {
        SizeMB++;
    }

    return(SizeMB);
}

ULONG
DiskSizeMB(
    IN ULONG Disk
    )

/*++

Routine Description:

    This routine determines the disk length in megabytes.  The returned
    value is rounded down after division by 1024*1024.

Arguments:

    Disk - index of disk whose size is desired

Return Value:

    Size of Disk.

--*/

{
    return(SIZEMB(DiskLengthBytes(Disk)));
}


LARGE_INTEGER
AlignTowardsDiskStart(
    IN ULONG         Disk,
    IN LARGE_INTEGER Offset
    )

/*++

Routine Description:

    This routine snaps a byte offset to a cylinder boundary, towards
    the start of the disk.

Arguments:

    Disk - index of disk whose offset is to be snapped

    Offset - byte offset to be aligned (snapped to cylinder boundary)

Return Value:

    Aligned offset.

--*/

{
    LARGE_INTEGER mod;
    LARGE_INTEGER Result;

    mod.QuadPart = Offset.QuadPart % DiskGeometryArray[Disk].BytesPerCylinder;
    Result.QuadPart = Offset.QuadPart - mod.QuadPart;
    return(Result);
}


LARGE_INTEGER
AlignTowardsDiskEnd(
    IN ULONG         Disk,
    IN LARGE_INTEGER Offset
    )

/*++

Routine Description:

    This routine snaps a byte offset to a cylinder boundary, towards
    the end of the disk.

Arguments:

    Disk - index of disk whose offset is to be snapped

    Offset - byte offset to be aligned (snapped to cylinder boundary)

Return Value:

    Aligned offset.

--*/

{
    LARGE_INTEGER mod;
    LARGE_INTEGER LargeInteger;

    mod.QuadPart = Offset.QuadPart % DiskGeometryArray[Disk].BytesPerCylinder;
    if(mod.QuadPart != 0) {

        LargeInteger.QuadPart = Offset.QuadPart + DiskGeometryArray[Disk].BytesPerCylinder;
        Offset = AlignTowardsDiskStart(Disk,
                                       LargeInteger
                                      );
    }
    return(Offset);
}


BOOLEAN
IsExtended(
    IN UCHAR SysID
    )

/*++

Routine Description:

    This routine determines whether a given system id is for an
    extended type (ie, link) entry.

Arguments:

    SysID - system id to be tested.

Return Value:

    true/false based on whether SysID is for an extended type.

--*/

{
    return((BOOLEAN)(SysID == SYSID_EXTENDED));
}


ARC_STATUS
IsAnyCreationAllowed(
    IN  ULONG    Disk,
    IN  BOOLEAN  AllowMultiplePrimaries,
    OUT PBOOLEAN AnyAllowed,
    OUT PBOOLEAN PrimaryAllowed,
    OUT PBOOLEAN ExtendedAllowed,
    OUT PBOOLEAN LogicalAllowed
    )

/*++

Routine Description:

    This routine determines whether any partition may be created on a
    given disk, based on three sub-queries -- whether creation is allowed
    of a primary partition, an extended partition, or a logical volume.

Arguments:

    Disk            - index of disk to check

    AllowMultiplePrimaries - whether to allow multiple primary partitions

    AnyAllowed - returns whether any creation is allowed

    PrimaryAllowed - returns whether creation of a primary partition
                     is allowed

    ExtendedAllowed - returns whether creation of an extended partition
                      is allowed

    Logical Allowed - returns whether creation of a logical volume is allowed.

Return Value:

    OK_STATUS or error code

--*/

{
    ARC_STATUS status;

    if((status = IsCreationOfPrimaryAllowed(Disk,AllowMultiplePrimaries,PrimaryAllowed)) != OK_STATUS) {
        return(status);
    }
    if((status = IsCreationOfExtendedAllowed(Disk,ExtendedAllowed)) != OK_STATUS) {
        return(status);
    }
    if((status = IsCreationOfLogicalAllowed(Disk,LogicalAllowed)) != OK_STATUS) {
        return(status);
    }
    *AnyAllowed = (BOOLEAN)(*PrimaryAllowed || *ExtendedAllowed || *LogicalAllowed);
    return(OK_STATUS);
}


ARC_STATUS
IsCreationOfPrimaryAllowed(
    IN  ULONG    Disk,
    IN  BOOLEAN  AllowMultiplePrimaries,
    OUT BOOLEAN *Allowed
    )

/*++

Routine Description:

    This routine determines whether creation of a primary partition is
    allowed.  This is true when there is a free entry in the MBR and
    there is free primary space on the disk.  If multiple primaries
    are not allowed, then there must also not exist any primary partitions
    in order for a primary creation to be allowed.

Arguments:

    Disk            - index of disk to check

    AllowMultiplePrimaries - whether existnace of primary partition
                             disallows creation of a primary partition

    Allowed - returns whether creation of a primary partition
              is allowed

Return Value:

    OK_STATUS or error code

--*/

{
    PREGION_DESCRIPTOR Regions;
    ULONG              RegionCount;
    ULONG              UsedCount,RecogCount,i;
    ARC_STATUS        status;
    BOOLEAN            FreeSpace = FALSE;

    status = GetPrimaryDiskRegions(Disk,&Regions,&RegionCount);
    if(status != OK_STATUS) {
        return(status);
    }

    for(UsedCount = RecogCount = i = 0; i<RegionCount; i++) {
        ASRT(Regions[i].RegionType != REGION_LOGICAL);
        if(Regions[i].SysID == SYSID_UNUSED) {
            FreeSpace = TRUE;
        } else {
            UsedCount++;
            if(!IsExtended(Regions[i].SysID) && Regions[i].Recognized) {
                RecogCount++;
            }
        }
    }
    ASRT(UsedCount <= ENTRIES_PER_BOOTSECTOR);
    ASRT(RecogCount <= ENTRIES_PER_BOOTSECTOR);
    ASRT(RecogCount <= UsedCount);

    if((UsedCount < ENTRIES_PER_BOOTSECTOR)
    && FreeSpace
    && (!RecogCount || AllowMultiplePrimaries))
    {
        *Allowed = TRUE;
    } else {
        *Allowed = FALSE;
    }

    FreeRegionArray(Regions,RegionCount);
    return(OK_STATUS);
}


ARC_STATUS
IsCreationOfExtendedAllowed(
    IN  ULONG    Disk,
    OUT BOOLEAN *Allowed
    )

/*++

Routine Description:

    This routine determines whether creation of an extended partition is
    allowed.  This is true when there is a free entry in the MBR,
    there is free primary space on the disk, and there is no existing
    extended partition.

Arguments:

    Disk            - index of disk to check

    Allowed - returns whether creation of an extended partition
              is allowed

Return Value:

    OK_STATUS or error code

--*/

{
    PREGION_DESCRIPTOR Regions;
    ULONG              RegionCount;
    ULONG              UsedCount,FreeCount,i;
    ARC_STATUS        status;

    status = GetPrimaryDiskRegions(Disk,&Regions,&RegionCount);
    if(status != OK_STATUS) {
        return(status);
    }

    for(UsedCount = FreeCount = i = 0; i<RegionCount; i++) {
        ASRT(Regions[i].RegionType != REGION_LOGICAL);
        if(Regions[i].SysID == SYSID_UNUSED) {
            // BUGBUG should adjust the size here and see if it's non0 first
            // (ie, take into account that the extended partition can't
            // start on cyl 0).
            FreeCount++;
        } else {
            UsedCount++;
            if(IsExtended(Regions[i].SysID)) {
                FreeRegionArray(Regions,RegionCount);
                *Allowed = FALSE;
                return(OK_STATUS);
            }
        }
    }
    *Allowed = (BOOLEAN)((UsedCount < ENTRIES_PER_BOOTSECTOR) && FreeCount);
    FreeRegionArray(Regions,RegionCount);
    return(OK_STATUS);
}


ARC_STATUS
IsCreationOfLogicalAllowed(
    IN  ULONG    Disk,
    OUT BOOLEAN *Allowed
    )

/*++

Routine Description:

    This routine determines whether creation of a logical volume is
    allowed.  This is true when there is an extended partition and
    free space within it.

Arguments:

    Disk            - index of disk to check

    Allowed - returns whether creation of a logical volume is allowed

Return Value:

    OK_STATUS or error code

--*/

{
    PREGION_DESCRIPTOR Regions;
    ULONG              RegionCount;
    ULONG              i;
    ARC_STATUS        status;
    BOOLEAN            ExtendedExists;

    *Allowed = FALSE;

    status = DoesExtendedExist(Disk,&ExtendedExists);
    if(status != OK_STATUS) {
        return(status);
    }
    if(!ExtendedExists) {
        return(OK_STATUS);
    }

    status = GetLogicalDiskRegions(Disk,&Regions,&RegionCount);
    if(status != OK_STATUS) {
        return(status);
    }

    for(i = 0; i<RegionCount; i++) {
        ASRT(Regions[i].RegionType == REGION_LOGICAL);
        if(Regions[i].SysID == SYSID_UNUSED) {
            *Allowed = TRUE;
            break;
        }
    }
    FreeRegionArray(Regions,RegionCount);
    return(OK_STATUS);
}



ARC_STATUS
DoesAnyPartitionExist(
    IN  ULONG    Disk,
    OUT PBOOLEAN AnyExists,
    OUT PBOOLEAN PrimaryExists,
    OUT PBOOLEAN ExtendedExists,
    OUT PBOOLEAN LogicalExists
    )

/*++

Routine Description:

    This routine determines whether any partition exists on a given disk.
    This is based on three sub queries: whether there are any primary or
    extended partitions, or logical volumes on the disk.

Arguments:

    Disk            - index of disk to check

    AnyExists - returns whether any partitions exist on Disk

    PrimaryExists - returns whether any primary partitions exist on Disk

    ExtendedExists - returns whether there is an extended partition on Disk

    LogicalExists - returns whether any logical volumes exist on Disk

Return Value:

    OK_STATUS or error code

--*/

{
    ARC_STATUS status;

    if((status = DoesAnyPrimaryExist(Disk,PrimaryExists )) != OK_STATUS) {
        return(status);
    }
    if((status = DoesExtendedExist  (Disk,ExtendedExists)) != OK_STATUS) {
        return(status);
    }
    if((status = DoesAnyLogicalExist(Disk,LogicalExists )) != OK_STATUS) {
        return(status);
    }
    *AnyExists = (BOOLEAN)(*PrimaryExists || *ExtendedExists || *LogicalExists);
    return(OK_STATUS);
}


ARC_STATUS
DoesAnyPrimaryExist(
    IN  ULONG    Disk,
    OUT BOOLEAN *Exists
    )

/*++

Routine Description:

    This routine determines whether any non-extended primary partition exists
    on a given disk.

Arguments:

    Disk   - index of disk to check

    Exists - returns whether any primary partitions exist on Disk

Return Value:

    OK_STATUS or error code

--*/

{
    PREGION_DESCRIPTOR Regions;
    ULONG               RegionCount,i;
    ARC_STATUS        status;

    status = GetUsedPrimaryDiskRegions(Disk,&Regions,&RegionCount);
    if(status != OK_STATUS) {
        return(status);
    }

    *Exists = FALSE;

    for(i=0; i<RegionCount; i++) {
        ASRT(Regions[i].RegionType != REGION_LOGICAL);
        ASRT(Regions[i].SysID != SYSID_UNUSED);
        if(!IsExtended(Regions[i].SysID)) {
            *Exists = TRUE;
            break;
        }
    }
    FreeRegionArray(Regions,RegionCount);
    return(OK_STATUS);
}


ARC_STATUS
DoesExtendedExist(
    IN  ULONG    Disk,
    OUT BOOLEAN *Exists
    )

/*++

Routine Description:

    This routine determines whether an extended partition exists
    on a given disk.

Arguments:

    Disk   - index of disk to check

    Exists - returns whether an extended partition exists on Disk

Return Value:

    OK_STATUS or error code

--*/

{
    PREGION_DESCRIPTOR Regions;
    ULONG               RegionCount,i;
    ARC_STATUS        status;

    status = GetUsedPrimaryDiskRegions(Disk,&Regions,&RegionCount);
    if(status != OK_STATUS) {
        return(status);
    }

    *Exists = FALSE;

    for(i=0; i<RegionCount; i++) {
        ASRT(Regions[i].RegionType != REGION_LOGICAL);
        ASRT(Regions[i].SysID != SYSID_UNUSED);
        if(IsExtended(Regions[i].SysID)) {
            *Exists = TRUE;
            break;
        }
    }
    FreeRegionArray(Regions,RegionCount);
    return(OK_STATUS);
}


ARC_STATUS
DoesAnyLogicalExist(
    IN  ULONG    Disk,
    OUT BOOLEAN *Exists
    )

/*++

Routine Description:

    This routine determines whether any logical volumes exist
    on a given disk.

Arguments:

    Disk   - index of disk to check

    Exists - returns whether any logical volumes exist on Disk

Return Value:

    OK_STATUS or error code

--*/

{
    PREGION_DESCRIPTOR Regions;
    ULONG               RegionCount;
    ARC_STATUS        status;

    status = GetUsedLogicalDiskRegions(Disk,&Regions,&RegionCount);
    if(status != OK_STATUS) {
        return(status);
    }

    *Exists = (BOOLEAN)(RegionCount != 0);
    FreeRegionArray(Regions,RegionCount);
    return(OK_STATUS);
}


ULONG
GetDiskCount(
    VOID
    )

/*++

Routine Description:

    This routine returns the number of attached partitionable disk
    devices.  The returned value is one greater than the maximum index
    allowed for Disk parameters to partitioning engine routines.

Arguments:

    None.

Return Value:

    Count of disks.

--*/

{
    return(CountOfDisks);
}


PCHAR
GetDiskName(
    ULONG Disk
    )

/*++

Routine Description:

    This routine returns the system name for the disk device whose
    index is given.

Arguments:

    Disk - index of disk whose name is desired.

Return Value:

    System name for the disk device.  The caller must not attempt to
    free this buffer or modify it.

--*/

{
    return(DiskNames[Disk]);
}


// sys ID type names

char    UNKNOWN[] = "Unknown type";

PCHAR   SysIDStrings[256] = { "Free Space","FAT","XENIX1","XENIX2", // 00-03
                              "FAT", "Extended", "FAT", "HPFS/NTFS",// 04-07
                              UNKNOWN, UNKNOWN,                     // 08-09
                              "OS/2 Boot Manager", UNKNOWN,         // 0a-0b
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 0c-0f
                              UNKNOWN, UNKNOWN,                     // 10-11
                              "EISA Utilities", UNKNOWN,            // 12-13
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 14-17
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 18-1b
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 1c-1f
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 20-23
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 24-27
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 28-2b
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 2c-2f
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 30-33
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 34-37
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 38-3b
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 3c-3f
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 40-43
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 44-47
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 48-4b
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 4c-4f
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 50-53
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 54-57
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 58-5b
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 5c-5f
                              UNKNOWN, UNKNOWN, UNKNOWN, "Unix",    // 60-63
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 64-67
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 68-6b
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 6c-6f
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 70-73
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 74-77
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 78-7b
                              UNKNOWN, UNKNOWN, UNKNOWN, "Unused",  // 7c-7f
                              UNKNOWN, "Windows NT Fault Tolerance",// 80-81
                              UNKNOWN, UNKNOWN,                     // 82-83
                              "Windows NT Fault Tolerance", UNKNOWN,// 84-85
                              "Windows NT Fault Tolerance",         // 86-86
                              "Windows NT Fault Tolerance",         // 87-87
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 88-8b
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 8c-8f
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 90-93
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 94-97
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 98-9b
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // 9c-9f
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // a0-a3
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // a4-a7
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // a8-ab
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // ac-af
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // b0-b3
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // b4-b7
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // b8-bb
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // bc-bf
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // c0-c3
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // c4-c7
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // c8-cb
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // cc-cf
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // d0-d3
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // d4-d7
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // d8-db
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // dc-df
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // e0-e3
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // e4-e7
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // e8-eb
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // ec-ef
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // f0-f3
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // f4-f7
                              UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,   // f8-fb
                              UNKNOWN, UNKNOWN, UNKNOWN, "Table",   // fc-ff
                            };


PCHAR
GetSysIDName(
    UCHAR SysID
    )

/*++

Routine Description:

    This routine returns the name for a given system ID.

Arguments:

    SysID - system id in question

Return Value:

    Name of system id.  The caller must not attempt to free or
    modify this buffer.

--*/

{
    return(SysIDStrings[SysID]);
}


// worker routines for WriteDriveLayout

VOID
UnusedEntryFill(
    IN PPARTITION_INFORMATION pinfo,
    IN ULONG                  EntryCount
    )
{
    ULONG         i;
    LARGE_INTEGER Zero;

    Zero.QuadPart = 0;

    for(i=0; i<EntryCount; i++) {

        pinfo[i].StartingOffset   = Zero;
        pinfo[i].PartitionLength  = Zero;
        pinfo[i].HiddenSectors    = 0;
        pinfo[i].PartitionType    = SYSID_UNUSED;
        pinfo[i].BootIndicator    = FALSE;
        pinfo[i].RewritePartition = TRUE;
    }
}


LARGE_INTEGER
MakeBootRec(
    ULONG                  Disk,
    PPARTITION_INFORMATION pinfo,
    PPARTITION             pLogical,
    PPARTITION             pNextLogical
    )
{
    ULONG         Entry = 0;
    LARGE_INTEGER BytesPerTrack;
    LARGE_INTEGER SectorsPerTrack;
    LARGE_INTEGER StartingOffset;

    BytesPerTrack.QuadPart = (DiskGeometryArray[Disk].BytesPerTrack);
    SectorsPerTrack.QuadPart = (DiskGeometryArray[Disk].SectorsPerTrack);
    StartingOffset.QuadPart = 0;

//  ASRT(pLogical || pNextLogical);

    if(pLogical) {

        pinfo[Entry].StartingOffset.QuadPart   = pLogical->Offset.QuadPart + BytesPerTrack.QuadPart;
        pinfo[Entry].PartitionLength.QuadPart  = pLogical->Length.QuadPart - BytesPerTrack.QuadPart;
        pinfo[Entry].HiddenSectors    = SectorsPerTrack.LowPart;
        pinfo[Entry].RewritePartition = pLogical->Update;
        pinfo[Entry].BootIndicator    = pLogical->Active;
        pinfo[Entry].PartitionType    = pLogical->SysID;

        if(pinfo[Entry].RewritePartition) {
            StartingOffset = pinfo[Entry].StartingOffset;
        }

        Entry++;
    }

    if(pNextLogical) {

        pinfo[Entry].StartingOffset   = pNextLogical->Offset;
        pinfo[Entry].PartitionLength  = pNextLogical->Length;
        pinfo[Entry].HiddenSectors    = 0;
        pinfo[Entry].RewritePartition = TRUE;
        pinfo[Entry].BootIndicator    = FALSE;
        pinfo[Entry].PartitionType    = SYSID_EXTENDED;

        Entry++;
    }

    UnusedEntryFill(pinfo+Entry,ENTRIES_PER_BOOTSECTOR-Entry);
    return(StartingOffset);
}


ARC_STATUS
ZapSector(
    ULONG         Disk,
    LARGE_INTEGER Offset
    )

/*++

Routine Description:

    This routine writes zeros into a sector at a given offset.  This is
    used to clear out a new partition's filesystem boot record, so that
    no previous filesystem appears in a new partition; or to clear out the
    first EBR in the extended partition if there are to be no logical vols.

Arguments:

    Disk - disk to write to

    Offset - byte offset to a newly created partition on Disk

Return Value:

    OK_STATUS or error code.

--*/

{
    ULONG       SectorSize = DiskGeometryArray[Disk].BytesPerSector;
    ULONG       i;
    PCHAR       SectorBuffer,AlignedSectorBuffer;
    ARC_STATUS status;
    ULONG    Handle;
    LARGE_INTEGER   LargeInteger;

    if((SectorBuffer = AllocateMemory(2*SectorSize)) == NULL) {
        RETURN_OUT_OF_MEMORY;
    }

    AlignedSectorBuffer = (PCHAR)(((ULONG)SectorBuffer+SectorSize) & ~(SectorSize-1));

    for(i=0; i<SectorSize; AlignedSectorBuffer[i++] = 0);

    if((status = LowOpenDisk(GetDiskName(Disk),&Handle)) != OK_STATUS) {
        FreeMemory(SectorBuffer);
        return(status);
    }

    LargeInteger.QuadPart = Offset.QuadPart / SectorSize;
    status = LowWriteSectors(Handle,
                             SectorSize,
                             LOWPART(LargeInteger),
                             1,
                             AlignedSectorBuffer
                            );

    LowCloseDisk(Handle);
    FreeMemory(SectorBuffer);

    return(status);
}


ARC_STATUS
WriteDriveLayout(
    IN ULONG Disk
    )

/*++

Routine Description:

    This routine writes the current partition layout for a given disk
    out to disk.  The high-level PARTITION lists are transformed into
    a DRIVE_LAYOUT_INFORMATION structure before being passed down
    to the low-level partition table writing routine.

Arguments:

    Disk - index of disk whose on-disk partition structure is to be updated.

Return Value:

    OK_STATUS or error code.

--*/

{
    PDRIVE_LAYOUT_INFORMATION DriveLayout;
    PPARTITION_INFORMATION    pinfo;
    PPARTITION                n,p;
    ULONG                     EntryCount;
    ARC_STATUS               status;
    LARGE_INTEGER             StartingOffset;
    LARGE_INTEGER             ExtendedStartingOffset;

    ExtendedStartingOffset.QuadPart = 0;

    // allocate a huge buffer now to avoid complicated dynamic
    // reallocation schemes later.

    if(!(DriveLayout = AllocateMemory(250 * sizeof(PARTITION_INFORMATION)))) {
        RETURN_OUT_OF_MEMORY;
    }

    pinfo = &DriveLayout->PartitionEntry[0];

    // first do the mbr.

    EntryCount=0;
    p = PrimaryPartitions[Disk];

    while(p) {

        if(p->SysID != SYSID_UNUSED) {

            ASRT(EntryCount < ENTRIES_PER_BOOTSECTOR);

            if(IsExtended(p->SysID)) {
                ExtendedStartingOffset = p->Offset;
            }

            pinfo[EntryCount].StartingOffset   = p->Offset;
            pinfo[EntryCount].PartitionLength  = p->Length;
            pinfo[EntryCount].PartitionType    = p->SysID;
            pinfo[EntryCount].BootIndicator    = p->Active;
            pinfo[EntryCount].RewritePartition = p->Update;

            // BUGBUG we know that this field is not used by the
            //        set drive layout IOCTL or the ARC stub.

            pinfo[EntryCount].HiddenSectors    = 0;

            // if we're creating this partition, clear out the
            // filesystem boot sector.

            if(pinfo[EntryCount].RewritePartition
            && (p->Update != CHANGED_DONT_ZAP)
            && !IsExtended(pinfo[EntryCount].PartitionType))
            {
                status = ZapSector(Disk,pinfo[EntryCount].StartingOffset);
                if(status != OK_STATUS) {
                    FreeMemory(DriveLayout);
                    return(status);
                }
            }

            EntryCount++;
        }
        p = p->Next;
    }

    // fill the remainder of the MBR with unused entries.
    // NOTE that there will thus always be an MBR even if there
    // are no partitions defined.

    UnusedEntryFill(pinfo+EntryCount,ENTRIES_PER_BOOTSECTOR - EntryCount);
    EntryCount = ENTRIES_PER_BOOTSECTOR;

    //
    // now handle the logical volumes.
    // first check to see whether we need a dummy EBR at the beginning
    // of the extended partition.  This is the case when there is
    // free space at the beginning of the extended partition.
#if 0
    // Also handle the case where we are creating an empty extended
    // partition -- need to zap the first sector to eliminate any residue
    // that might start an EBR chain.
#else
    // BUGBUG 4/24/92 tedm:  Currently the io subsystem returns an error
    // status (status_bad_master_boot_record) if any mbr or ebr is bad.
    // Zeroing the first sector of the extended partition therefore causes
    // the whole disk to be seen as empty.  So create a blank, but valid,
    // EBR in the 'empty extended partition' case.  Code is in the 'else'
    // part of the #if 0, below.
#endif
    //

    if((p = LogicalVolumes[Disk]) && (p->SysID == SYSID_UNUSED)) {
        if(p->Next) {
            ASRT(p->Next->SysID != SYSID_UNUSED);

            MakeBootRec(Disk,pinfo+EntryCount,NULL,p->Next);
            EntryCount += ENTRIES_PER_BOOTSECTOR;
            p = p->Next;
        } else {
            ASRT(ExtendedStartingOffset.QuadPart != 0);
#if 0
            status = ZapSector(Disk,ExtendedStartingOffset);
            if(status != OK_STATUS) {
                FreeMemory(DriveLayout);
                return(status);
            }
#else
            MakeBootRec(Disk,pinfo+EntryCount,NULL,NULL);
            EntryCount += ENTRIES_PER_BOOTSECTOR;
#endif
        }
    }

    while(p) {
        if(p->SysID != SYSID_UNUSED) {

            // find the next logical volume.

            n = p->Next;
            while(n) {
                if(n->SysID != SYSID_UNUSED) {
                    break;
                }
                n = n->Next;
            }

            StartingOffset = MakeBootRec(Disk,pinfo+EntryCount,p,n);

            // if we're creating a volume, clear out its filesystem
            // boot sector so it starts out fresh.

            if((StartingOffset.QuadPart != 0) && (p->Update != CHANGED_DONT_ZAP)) {
                status = ZapSector(Disk,StartingOffset);
                if(status != OK_STATUS) {
                    FreeMemory(DriveLayout);
                    return(status);
                }
            }

            EntryCount += ENTRIES_PER_BOOTSECTOR;
        }
        p = p->Next;
    }

    DriveLayout->PartitionCount = EntryCount;
    status = LowSetDiskLayout(DiskNames[Disk],DriveLayout);

    FreeMemory(DriveLayout);

    return(status);
}


ARC_STATUS
CommitPartitionChanges(
    IN ULONG Disk
    )

/*++

Routine Description:

    This routine is the entry point for updating the on-disk partition
    structures of a disk.  The disk is only written to if the partition
    structure has been changed by adding or deleting partitions.

Arguments:

    Disk - index of disk whose on-disk partition structure is to be updated.

Return Value:

    OK_STATUS or error code.

--*/

{
    PPARTITION  p;
    ARC_STATUS status;

    ASRT(!OffLine[Disk]);

    if(!HavePartitionsBeenChanged(Disk)) {
        return(OK_STATUS);
    }

    if((status = WriteDriveLayout(Disk)) != OK_STATUS) {
        return(status);
    }

    // BUGBUG for ARC and NT MIPS, update NVRAM vars so partitions are right.
    //        Do that here, before partition numbers are reassigned.

    p = PrimaryPartitions[Disk];
    while(p) {
        p->Update = FALSE;
        p->OriginalPartitionNumber = p->PartitionNumber;
        p = p->Next;
    }
    p = LogicalVolumes[Disk];
    while(p) {
        p->Update = FALSE;
        p->OriginalPartitionNumber = p->PartitionNumber;
        p = p->Next;
    }

    ChangesMade[Disk] = FALSE;
    return(OK_STATUS);
}


BOOLEAN
HavePartitionsBeenChanged(
    IN ULONG Disk
    )

/*++

Routine Description:

    This routine returns TRUE if the given disk's partition structures
    have been modified by adding or deleting partitions, since the
    on-disk structures were last written by a call to CommitPartitionChanges
    (or first read).

Arguments:

    Disk - index of disk to check

Return Value:

    true if Disk's partition structure has changed.

--*/

{
    if(OffLine[Disk]) {
        ASRT(!ChangesMade[Disk]);
    }
    return(ChangesMade[Disk]);
}

VOID
FdMarkDiskDirty(
    IN ULONG Disk
    )
{
    ASRT(!OffLine[Disk]);

    ChangesMade[Disk] = TRUE;
}

VOID
FdSetPersistentData(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              Data
    )
{
    ((PREGION_DATA)(Region->Reserved))->Partition->PersistentData = Data;
}


ULONG
FdGetMinimumSizeMB(
    IN ULONG Disk
    )

/*++

Routine Description:

    Return the minimum size for a partition on a given disk.

    This is the rounded size of one cylinder or 1, whichever is greater.

Arguments:

    Region - region describing the partition to check.

Return Value:

    Actual offset

--*/

{
    LARGE_INTEGER   LargeInteger;

    LargeInteger.QuadPart = DiskGeometryArray[Disk].BytesPerCylinder;
    return(max(SIZEMB(LargeInteger),1));
}


ULONG
FdGetMaximumSizeMB(
    IN PREGION_DESCRIPTOR Region,
    IN REGION_TYPE        CreationType
    )
{
    PREGION_DATA CreateData = Region->Reserved;
    LARGE_INTEGER MaxSize = CreateData->AlignedRegionSize;

    ASRT(Region->SysID == SYSID_UNUSED);

    if(CreateData->AlignedRegionOffset.QuadPart == 0) {

        ULONG Delta;

        ASRT((CreationType == REGION_EXTENDED) || (CreationType == REGION_PRIMARY))

        Delta = (CreationType == REGION_EXTENDED)
              ? DiskGeometryArray[Region->Disk].BytesPerCylinder
              : DiskGeometryArray[Region->Disk].BytesPerTrack;

        MaxSize.QuadPart = MaxSize.QuadPart - Delta;
    }

    return(SIZEMB(MaxSize));
}


LARGE_INTEGER
FdGetExactSize(
    IN PREGION_DESCRIPTOR Region,
    IN BOOLEAN            ForExtended
    )
{
    PREGION_DATA  RegionData = Region->Reserved;
    LARGE_INTEGER LargeSize = RegionData->AlignedRegionSize;
    LARGE_INTEGER BytesPerTrack;
    LARGE_INTEGER BytesPerCylinder;

    BytesPerTrack.QuadPart = (DiskGeometryArray[Region->Disk].BytesPerTrack);
    BytesPerCylinder.QuadPart = (DiskGeometryArray[Region->Disk].BytesPerCylinder);

    if(Region->RegionType == REGION_LOGICAL) {

        //
        // The region is within the extended partition.  It doesn't matter
        // whether it's free space or used -- in either case, we need to
        // account for the reserved EBR track.
        //

        LargeSize.QuadPart = LargeSize.QuadPart - BytesPerTrack.QuadPart;

    } else if(Region->SysID == SYSID_UNUSED) {

        //
        // The region is unused space not inside the extended partition.
        // We must know whether the caller will put a primary or extended
        // partition there -- a primary partition can use all the space, but
        // a logical volume in the extended partition won't include the first
        // track.  If the free space starts at offset 0 on the disk, a special
        // calculation must be used to move the start of the partition to
        // skip a track for a primary or a cylinder and a track for an
        // extended+logical.
        //

        if((RegionData->AlignedRegionOffset.QuadPart == 0) || ForExtended) {
            LargeSize.QuadPart = LargeSize.QuadPart - BytesPerTrack.QuadPart;
        }

        if((RegionData->AlignedRegionOffset.QuadPart == 0) && ForExtended) {
            LargeSize.QuadPart = LargeSize.QuadPart - BytesPerCylinder.QuadPart;
        }
    }

    return(LargeSize);
}


LARGE_INTEGER
FdGetExactOffset(
    IN PREGION_DESCRIPTOR Region
    )

/*++

Routine Description:

    Determine where a given partition _actually_ starts, which may be
    different than where is appears because of EBR reserved tracks, etc.

    NOTE: This routine is not meant to operate on unused regions or
    extended partitions.  In these cases, it just returns the apparant offset.

Arguments:

    Region - region describing the partition to check.

Return Value:

    Actual offset

--*/

{
    LARGE_INTEGER Offset = ((PREGION_DATA)(Region->Reserved))->Partition->Offset;

    if((Region->SysID != SYSID_UNUSED) && (Region->RegionType == REGION_LOGICAL)) {

        //
        // The region is a logical volume.
        // Account for the reserved EBR track.
        //

        Offset.QuadPart = Offset.QuadPart +
                                 DiskGeometryArray[Region->Disk].BytesPerTrack;
    }

    return(Offset);
}


BOOLEAN
FdCrosses1024Cylinder(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              CreationSizeMB,
    IN REGION_TYPE        RegionType
    )

/*++

Routine Description:

    Determine whether a used region corsses the 1024th cylinder, or whether
    a partition created within a free space will cross the 1024th cylinder.

Arguments:

    Region - region describing the partition to check.

    CreationSizeMB - if the Region is for a free space, this is the size of
        the partition to be checked.

    RegionType - one of REGION_PRIMARY, REGION_EXTENDED, or REGION_LOGICAL

Return Value:

    TRUE if the end cylinder >= 1024.

--*/

{
    LARGE_INTEGER Start,Size,End, LargeZero;

    if(Region->SysID == SYSID_UNUSED) {

        //
        // Determine the exact size and offset of the partition, according
        // to how CreatePartitionEx() will do it.
        //

        LargeZero.QuadPart = 0;
        DetermineCreateSizeAndOffset( Region,
                                      LargeZero,
                                      CreationSizeMB,
                                      RegionType,
                                      &Start,
                                      &Size
                                    );

    } else {

        Start = ((PREGION_DATA)(Region->Reserved))->Partition->Offset;
        Size  = ((PREGION_DATA)(Region->Reserved))->Partition->Length;
    }

    End.QuadPart = (Start.QuadPart + Size.QuadPart) - 1;

    //
    // End is the last byte in the partition.  Divide by the number of
    // bytes in a cylinder and see whether the result is > 1023.
    //

    return( (End.QuadPart / DiskGeometryArray[Region->Disk].BytesPerCylinder) > 1023 );
}


BOOLEAN
IsDiskOffLine(
    IN ULONG Disk
    )
{
    return(OffLine[Disk]);
}
